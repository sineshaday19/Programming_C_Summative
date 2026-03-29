#!/usr/bin/env bash
#
# server_health_monitor.sh — sample CPU, memory, and root disk usage against
# fixed thresholds; log outcomes; menu-driven control (options 6–9).

LOG_FILE="./system.log"
MONITOR_INTERVAL=5
THRESHOLD_CPU=80
THRESHOLD_MEM=80
THRESHOLD_DISK=80

# Intentionally no `set -e`; failures are handled per command so the menu keeps running.

log_message() {
  local msg="$1"
  local ts
  ts=$(date '+%Y-%m-%d %H:%M:%S' 2>/dev/null) || ts="(date unavailable)"

  if ! echo "${ts} ${msg}" >>"$LOG_FILE" 2>/dev/null; then
    echo "Sorry: could not write to log file '${LOG_FILE}'."
  fi
}

# Busy CPU % from top(1) (100 − idle on the Cpu(s) line). WSL/procps can vary spacing; awk is more robust than sed.
get_cpu() {
  local line idle
  # Second batch is often more stable than the first on some systems (incl. WSL).
  line=$(top -bn2 2>/dev/null | grep "Cpu(s)" | tail -n 1 || true)
  if [[ -z "$line" ]]; then
    line=$(top -bn1 2>/dev/null | grep "Cpu(s)" || true)
  fi
  if [[ -z "$line" ]]; then
    echo ""
    return 1
  fi

  idle=$(echo "$line" | awk -F',' '{
    for (i = 1; i <= NF; i++) {
      if ($i ~ /id/) {
        gsub(/[^0-9.]/, "", $i)
        if ($i != "") { print $i; exit }
      }
    }
  }' || true)
  if [[ -z "$idle" ]] || ! [[ "$idle" =~ ^[0-9.]+$ ]]; then
    echo ""
    return 1
  fi

  awk -v id="$idle" 'BEGIN { printf "%.0f", 100 - id }'
  return 0
}

# Used RAM % from free(1) Mem line (used/total).
get_memory() {
  local pct
  pct=$(free 2>/dev/null | awk '/^Mem:/ { if ($2+0 > 0) printf "%.0f", ($3 * 100) / $2 }' || true)
  if [[ -z "$pct" ]] || ! [[ "$pct" =~ ^[0-9]+$ ]]; then
    echo ""
    return 1
  fi
  echo "$pct"
  return 0
}

# Disk usage % for / via df -P (GNU/Linux layout). Rejects junk if columns differ (e.g. Git Bash).
get_disk() {
  local pct
  pct=$(df -P / 2>/dev/null | awk 'NR==2 { gsub(/%/,"",$5); print $5 }' || true)
  if [[ -z "$pct" ]] || ! [[ "$pct" =~ ^[0-9]+$ ]]; then
    echo ""
    return 1
  fi
  if (( pct < 0 || pct > 100 )); then
    echo ""
    return 1
  fi
  echo "$pct"
  return 0
}

# Compare metrics to THRESHOLD_*; warn on stderr and log ALERT or OK summary.
check_alerts() {
  local cpu_p="$1" mem_p="$2" disk_p="$3"
  local warned=0

  if [[ -n "$cpu_p" ]] && (( cpu_p > THRESHOLD_CPU )); then
    echo "  *** WARNING: CPU usage (${cpu_p}%) is above ${THRESHOLD_CPU}%."
    log_message "ALERT: CPU ${cpu_p}% exceeds ${THRESHOLD_CPU}%"
    warned=1
  fi

  if [[ -n "$mem_p" ]] && (( mem_p > THRESHOLD_MEM )); then
    echo "  *** WARNING: Memory usage (${mem_p}%) is above ${THRESHOLD_MEM}%."
    log_message "ALERT: Memory ${mem_p}% exceeds ${THRESHOLD_MEM}%"
    warned=1
  fi

  if [[ -n "$disk_p" ]] && (( disk_p > THRESHOLD_DISK )); then
    echo "  *** WARNING: Disk usage on / (${disk_p}%) is above ${THRESHOLD_DISK}%."
    log_message "ALERT: Disk / ${disk_p}% exceeds ${THRESHOLD_DISK}%"
    warned=1
  fi

  if (( warned == 0 )); then
    log_message "OK: CPU=${cpu_p:-?}% MEM=${mem_p:-?}% DISK=${disk_p:-?}% (all within thresholds)"
  fi
}

# Writes into caller variables whose names are $1, $2, $3 (bash namerefs).
collect_stats() {
  local -n _cpu="$1"
  local -n _mem="$2"
  local -n _disk="$3"

  _cpu=""
  _mem=""
  _disk=""

  if ! _cpu=$(get_cpu); then
    echo "Note: could not read CPU from top. Showing CPU as n/a."
  fi
  if ! _mem=$(get_memory); then
    echo "Note: could not read memory from free. Showing memory as n/a."
  fi
  if ! _disk=$(get_disk); then
    echo "Note: could not read disk from df. Showing disk as n/a."
  fi
}

# One-off metrics and a short ps(1) listing sorted by CPU.
show_system_health() {
  local cpu_p mem_p disk_p

  echo ""
  echo "========== System health snapshot =========="
  collect_stats cpu_p mem_p disk_p

  echo "  CPU usage:      ${cpu_p:-n/a}%"
  echo "  Memory usage:   ${mem_p:-n/a}%"
  echo "  Disk usage (/): ${disk_p:-n/a}%"

  echo ""
  echo "Top processes by CPU (ps):"
  ps aux --sort=-%cpu 2>/dev/null | head -n 6 || echo "  (ps unavailable)"

  echo ""
  echo "Thresholds — CPU: ${THRESHOLD_CPU}%  Memory: ${THRESHOLD_MEM}%  Disk: ${THRESHOLD_DISK}%"
  echo "=============================================="
}

# Reset SIGINT handler after user stops the monitoring loop.
monitoring_cleanup() {
  echo ""
  echo "Monitoring stopped. Returning to the menu."
  trap - INT
}

# Infinite poll until sleep is interrupted (e.g. Ctrl+C); SIGINT restores menu.
start_monitoring_loop() {
  local cpu_p mem_p disk_p

  echo ""
  echo "Starting continuous monitoring (every ${MONITOR_INTERVAL}s)."
  echo "Press Ctrl+C to stop and return to the menu."
  log_message "--- Monitoring session started ---"

  trap monitoring_cleanup INT

  while true; do
    echo ""
    echo "----- $(date '+%Y-%m-%d %H:%M:%S') -----"
    collect_stats cpu_p mem_p disk_p
    echo "  CPU: ${cpu_p:-n/a}% | Memory: ${mem_p:-n/a}% | Disk (/): ${disk_p:-n/a}%"
    check_alerts "$cpu_p" "$mem_p" "$disk_p"
    sleep "$MONITOR_INTERVAL" || break
  done

  trap - INT
  log_message "--- Monitoring session ended ---"
}

# Tail of LOG_FILE (fixed line count).
view_logs() {
  echo ""
  if [[ ! -f "$LOG_FILE" ]]; then
    echo "No log file yet ('${LOG_FILE}'). Run monitoring or any action that logs first."
    return 0
  fi

  if [[ ! -s "$LOG_FILE" ]]; then
    echo "Log file '${LOG_FILE}' is empty."
    return 0
  fi

  echo "---------- Last lines of '${LOG_FILE}' ----------"
  tail -n 50 "$LOG_FILE" 2>/dev/null || echo "Sorry: could not read the log file."
  echo "-------------------------------------------------"
}

print_menu() {
  echo ""
  echo "======== Linux Server Health Monitor ========"
  echo "  6) Show system health (snapshot)"
  echo "  7) Start monitoring (loop, ${MONITOR_INTERVAL}s; Ctrl+C stops)"
  echo "  8) View logs"
  echo "  9) Exit"
  echo "============================================="
  echo -n "Enter choice [6-9]: "
}

# Drives the UI until option 9 or EOF on stdin.
main() {
  echo "Welcome. This script uses top, free, df, and ps. Logs go to '${LOG_FILE}'."
  if [[ "$(uname -s 2>/dev/null)" != Linux ]]; then
    echo "Note: meant for Linux (or WSL). Git Bash / macOS may show n/a or wrong metrics."
  fi

  while true; do
    print_menu
    read -r choice || {
      echo ""
      echo "Input ended — goodbye."
      exit 0
    }

    case "$choice" in
      6)
        show_system_health
        ;;
      7)
        start_monitoring_loop
        ;;
      8)
        view_logs
        ;;
      9)
        echo "Goodbye."
        exit 0
        ;;
      "")
        echo "Invalid choice: please enter a number from 6 to 9."
        ;;
      *)
        echo "Invalid choice '${choice}' — use 6, 7, 8, or 9."
        ;;
    esac
  done
}

main "$@"
