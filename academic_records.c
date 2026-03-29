/*
 * academic_records.c
 * Course Performance and Academic Records Analyzer
 *
 * Manages a dynamically-allocated array of Student records with CRUD,
 * search, GPA sort, statistics, and pipe-delimited file persistence.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_LEN  50
#define DATA_FILE "students.txt"

typedef struct {
    int   id;
    char  name[NAME_LEN];
    int   age;
    float gpa;
} Student;

static Student *students = NULL;
static int      count    = 0;   /* live records    */
static int      capacity = 0;   /* allocated slots */

/* fgets wrapper that strips the trailing newline; returns 0 on EOF or error. */
static int read_line(char *buf, int size)
{
    if (!fgets(buf, size, stdin)) return 0;
    buf[strcspn(buf, "\n")] = '\0';
    return 1;
}

/* strtol parse — rejects partial matches and trailing non-digit characters. */
static int read_int(int *out)
{
    char buf[32];
    if (!read_line(buf, sizeof buf)) return 0;
    char *end;
    long v = strtol(buf, &end, 10);
    if (end == buf || *end != '\0') return 0;
    *out = (int)v;
    return 1;
}

/* strtod parse — same rejection rules as read_int. */
static int read_float(float *out)
{
    char buf[32];
    if (!read_line(buf, sizeof buf)) return 0;
    char *end;
    double v = strtod(buf, &end);
    if (end == buf || *end != '\0') return 0;
    *out = (float)v;
    return 1;
}

/* Doubles capacity via realloc; seeds at 4 on first call. Returns 0 on failure. */
static int grow(void)
{
    int new_cap = (capacity == 0) ? 4 : capacity * 2;
    Student *tmp = realloc(students, new_cap * sizeof(Student));
    if (!tmp) {
        fprintf(stderr, "Error: realloc failed.\n");
        return 0;
    }
    students = tmp;
    capacity = new_cap;
    return 1;
}

/* Linear scan; returns index or -1. */
static int find_by_id(int id)
{
    for (int i = 0; i < count; i++)
        if (students[i].id == id) return i;
    return -1;
}

static void add_student(void)
{
    if (count == capacity && !grow()) return;

    Student s;

    printf("ID   : ");
    if (!read_int(&s.id)) { printf("Invalid ID.\n"); return; }
    if (find_by_id(s.id) != -1) { printf("ID %d already exists.\n", s.id); return; }

    printf("Name : ");
    if (!read_line(s.name, NAME_LEN) || s.name[0] == '\0') {
        printf("Invalid name.\n"); return;
    }

    printf("Age  : ");
    if (!read_int(&s.age) || s.age < 1 || s.age > 120) {
        printf("Invalid age.\n"); return;
    }

    printf("GPA  : ");
    if (!read_float(&s.gpa) || s.gpa < 0.0f || s.gpa > 4.0f) {
        printf("Invalid GPA (0.0 - 4.0).\n"); return;
    }

    students[count++] = s;
    printf("Student added.\n");
}

static void display_all(void)
{
    if (count == 0) { printf("No students on record.\n"); return; }

    printf("\n%-6s  %-20s  %-5s  %s\n", "ID", "Name", "Age", "GPA");
    printf("%-6s  %-20s  %-5s  %s\n",
           "------", "--------------------", "-----", "------");
    for (int i = 0; i < count; i++) {
        printf("%-6d  %-20s  %-5d  %.2f\n",
               students[i].id, students[i].name,
               students[i].age, students[i].gpa);
    }
    printf("\n");
}

static void update_student(void)
{
    if (count == 0) { printf("No students on record.\n"); return; }

    int id;
    printf("ID to update: ");
    if (!read_int(&id)) { printf("Invalid ID.\n"); return; }

    int idx = find_by_id(id);
    if (idx == -1) { printf("No student with ID %d.\n", id); return; }

    Student *s = &students[idx];
    printf("Updating %s - press Enter to keep current value.\n", s->name);

    char buf[NAME_LEN];
    char *end;

    printf("Name [%s]: ", s->name);
    read_line(buf, NAME_LEN);
    if (buf[0] != '\0') strncpy(s->name, buf, NAME_LEN - 1);

    printf("Age  [%d]: ", s->age);
    read_line(buf, sizeof buf);
    if (buf[0] != '\0') {
        long v = strtol(buf, &end, 10);
        if (end != buf && *end == '\0' && v >= 1 && v <= 120)
            s->age = (int)v;
        else
            printf("Invalid age - unchanged.\n");
    }

    printf("GPA  [%.2f]: ", s->gpa);
    read_line(buf, sizeof buf);
    if (buf[0] != '\0') {
        double v = strtod(buf, &end);
        if (end != buf && *end == '\0' && v >= 0.0 && v <= 4.0)
            s->gpa = (float)v;
        else
            printf("Invalid GPA - unchanged.\n");
    }

    printf("Student updated.\n");
}

static void delete_student(void)
{
    if (count == 0) { printf("No students on record.\n"); return; }

    int id;
    printf("ID to delete: ");
    if (!read_int(&id)) { printf("Invalid ID.\n"); return; }

    int idx = find_by_id(id);
    if (idx == -1) { printf("No student with ID %d.\n", id); return; }

    for (int i = idx; i < count - 1; i++)
        students[i] = students[i + 1];
    count--;

    printf("Student %d deleted.\n", id);
}

static void search_student(void)
{
    if (count == 0) { printf("No students on record.\n"); return; }

    int id;
    printf("ID to search: ");
    if (!read_int(&id)) { printf("Invalid ID.\n"); return; }

    int idx = find_by_id(id);
    if (idx == -1) { printf("No student with ID %d.\n", id); return; }

    Student *s = &students[idx];
    printf("Found  ->  ID: %d | Name: %s | Age: %d | GPA: %.2f\n",
           s->id, s->name, s->age, s->gpa);
}

/* Bubble sort, ascending by GPA. */
static void sort_by_gpa(void)
{
    if (count < 2) { printf("Nothing to sort.\n"); return; }

    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - 1 - i; j++) {
            if (students[j].gpa > students[j + 1].gpa) {
                Student tmp     = students[j];
                students[j]     = students[j + 1];
                students[j + 1] = tmp;
            }
        }
    }
    printf("Sorted by GPA (ascending).\n");
    display_all();
}

static void show_statistics(void)
{
    if (count == 0) { printf("No students on record.\n"); return; }

    float sum = 0.0f;
    int   top = 0;

    for (int i = 0; i < count; i++) {
        sum += students[i].gpa;
        if (students[i].gpa > students[top].gpa) top = i;
    }

    printf("Total students : %d\n",   count);
    printf("Average GPA    : %.2f\n", sum / (float)count);
    printf("Top student    : %s  (GPA %.2f)\n",
           students[top].name, students[top].gpa);
}

/* Format: one record per line, pipe-delimited — id|name|age|gpa */
static void save_to_file(void)
{
    FILE *fp = fopen(DATA_FILE, "w");
    if (!fp) { perror("Cannot open " DATA_FILE); return; }

    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d|%s|%d|%.2f\n",
                students[i].id, students[i].name,
                students[i].age, students[i].gpa);
    }

    fclose(fp);
    printf("Saved %d record(s) to %s.\n", count, DATA_FILE);
}

/* Replaces in-memory state; no merge with existing records. */
static void load_from_file(void)
{
    FILE *fp = fopen(DATA_FILE, "r");
    if (!fp) { perror("Cannot open " DATA_FILE); return; }

    count = 0;
    char line[128];

    while (fgets(line, sizeof line, fp)) {
        Student s;
        char *tok;

        tok = strtok(line, "|"); if (!tok) continue; s.id = atoi(tok);
        tok = strtok(NULL, "|"); if (!tok) continue;
        strncpy(s.name, tok, NAME_LEN - 1); s.name[NAME_LEN - 1] = '\0';
        tok = strtok(NULL, "|"); if (!tok) continue; s.age = atoi(tok);
        tok = strtok(NULL, "\n"); if (!tok) continue; s.gpa = (float)atof(tok);

        if (count == capacity && !grow()) break;
        students[count++] = s;
    }

    fclose(fp);
    printf("Loaded %d record(s) from %s.\n", count, DATA_FILE);
}

static void print_menu(void)
{
    printf("\n====== Academic Records Analyzer ======\n");
    printf("  1) Add Student\n");
    printf("  2) Display All Students\n");
    printf("  3) Update Student\n");
    printf("  4) Delete Student\n");
    printf("  5) Search Student\n");
    printf("  6) Sort by GPA\n");
    printf("  7) Show Statistics\n");
    printf("  8) Save to File\n");
    printf("  9) Load from File\n");
    printf(" 10) Exit\n");
    printf("=======================================\n");
    printf("Choice: ");
}

int main(void)
{
    students = malloc(4 * sizeof(Student));
    if (!students) {
        fprintf(stderr, "Fatal: malloc failed.\n");
        return EXIT_FAILURE;
    }
    capacity = 4;

    int choice;
    while (1) {
        print_menu();
        if (!read_int(&choice)) {
            printf("Invalid input - enter a number 1-10.\n");
            continue;
        }
        switch (choice) {
            case 1:  add_student();     break;
            case 2:  display_all();     break;
            case 3:  update_student();  break;
            case 4:  delete_student();  break;
            case 5:  search_student();  break;
            case 6:  sort_by_gpa();     break;
            case 7:  show_statistics(); break;
            case 8:  save_to_file();    break;
            case 9:  load_from_file();  break;
            case 10:
                free(students);
                printf("Goodbye.\n");
                return EXIT_SUCCESS;
            default:
                printf("Invalid choice - use 1 to 10.\n");
        }
    }
}
