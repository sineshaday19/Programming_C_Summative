# Programming C Summative

Three small programs for the same module: a Bash health monitor, a C student records app, and a threaded scraper that shells out to `curl`. Everything lives in this folder; clone it and follow the commands below.

**What’s in the repo**

- `server_health_monitor.sh` — Bash  
- `academic_records.c` — C  
- `web_scraper.c` — C with pthreads  

When you run things you may get extra files next to them: `system.log`, `students.txt`, `output_1.txt` … `output_3.txt`, and whatever you name the compiled binaries (e.g. `academic_records.exe` on Windows). Add those to `.gitignore` if you don’t want them in git.

**What you need**

The shell script expects a real Unix environment (Linux, macOS terminal, or WSL). It won’t behave properly in plain PowerShell. You need `bash`, and the usual tools it calls: `top`, `free`, `df`, `ps`, `date`, `sleep`, `tail`. Bash should be fairly new (4.3+) because of `nameref`.

For the C files you need `gcc`. The scraper also needs `curl` on your PATH. On Windows people usually use WSL, or a portable kit like w64devkit, then open a new terminal after editing PATH.

### `server_health_monitor.sh`

It samples CPU (via `top`), memory (`free`), disk on `/` (`df`), prints a short `ps` list, and compares numbers to 80% thresholds. Good readings and alerts go into `system.log` with timestamps from `date`. Option 7 runs a loop every 5 seconds until you hit Ctrl+C.

Menu uses 6–9 on purpose for the assignment: 6 snapshot, 7 monitor loop, 8 tail the log, 9 quit.

```bash
chmod +x server_health_monitor.sh
./server_health_monitor.sh
```

Run it from a directory you can write to so `system.log` can be created. If your system language isn’t English, `top`’s text might not match what the script parses for CPU.

### `academic_records.c`

Console menu for students (id, name up to 49 chars, age, gpa). The list grows with `malloc`/`realloc` and is freed on exit. You can add, list, update, delete, search by id, bubble-sort by GPA, show average + best GPA, and save/load `students.txt`. Load wipes whatever was in memory and replaces it from file. Lines in the file look like `id|name|age|gpa`.

```bash
gcc -Wall -Wextra -o academic_records academic_records.c
./academic_records
```

On Windows, same command but name the output `academic_records.exe` if you prefer and run `.\academic_records.exe`.

### `web_scraper.c`

Starts one thread per URL; each thread builds a `curl` command and runs it with `system()`. URLs are hardcoded at the top of `main` (example.com / .org / .net right now — change the array if you want). Saves to `output_1.txt`, `output_2.txt`, `output_3.txt` in whatever directory you run from. The program checks that `curl` exists before starting threads. You must link pthreads:

```bash
gcc -Wall -Wextra -pthread -o web_scraper web_scraper.c
./web_scraper
```

There’s also a one-line compile hint at the top of `web_scraper.c`.

This is not a socket program; it’s only meant to show parallel work with pthreads and file output.

**If something breaks**

No `gcc`: install a toolchain or use WSL, then restart the terminal. Scraper says curl missing: install curl and check `curl --version`. Link errors about pthread: keep `-pthread` on the same gcc line as the source. Health script acts weird on CPU: look at what `top -bn1` prints on your machine. Empty `output_*.txt`: network might block those URLs; try others or HTTPS in the source.

Coursework for Programming C — details are in the source files.
