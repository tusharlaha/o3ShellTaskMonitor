
// ------------------------------------------------------
// o3ShellTaskMonitor - Version 1.7.0 | Rev 0.1.0
// ------------------------------------------------------
// Copyright (c) openw3rk INVENT
// Licensed under the MIT-License
// ------------------------------------------------------
// o3ShellTaskMonitor comes with ABSOLUTELY NO WARRANTY.
// ------------------------------------------------------

#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

#ifdef _WIN32
void set_color(WORD color) {
    static HANDLE hConsole = NULL;
    if (!hConsole) hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}
#define COLOR_RED  FOREGROUND_RED | FOREGROUND_INTENSITY
#define COLOR_GREEN FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define COLOR_YELLOW FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define COLOR_CYAN FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define COLOR_RESET 7
#else
void set_color(const char* color_code) {
    printf("%s", color_code);
}
#define COLOR_RED    "\x1b[31;1m"
#define COLOR_GREEN  "\x1b[32;1m"
#define COLOR_YELLOW "\x1b[33;1m"
#define COLOR_CYAN   "\x1b[36;1m"
#define COLOR_RESET  "\x1b[0m"
#endif

void msleep(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

#ifdef _WIN32
typedef struct {
    ULONGLONG idleTime;
    ULONGLONG kernelTime;
    ULONGLONG userTime;
} CPU_TIMES;

int get_cpu_times(CPU_TIMES* times) {
    FILETIME idle, kernel, user;
    if (!GetSystemTimes(&idle, &kernel, &user)) return 0;

    ULARGE_INTEGER uIdle, uKernel, uUser;
    uIdle.LowPart = idle.dwLowDateTime;
    uIdle.HighPart = idle.dwHighDateTime;
    uKernel.LowPart = kernel.dwLowDateTime;
    uKernel.HighPart = kernel.dwHighDateTime;
    uUser.LowPart = user.dwLowDateTime;
    uUser.HighPart = user.dwHighDateTime;

    times->idleTime = uIdle.QuadPart;
    times->kernelTime = uKernel.QuadPart;
    times->userTime = uUser.QuadPart;
    return 1;
}

double get_cpu_usage() {
    static CPU_TIMES prev = {0,0,0};
    CPU_TIMES curr;
    if (!get_cpu_times(&curr)) return -1.0;

    ULONGLONG idleDiff = curr.idleTime - prev.idleTime;
    ULONGLONG kernelDiff = curr.kernelTime - prev.kernelTime;
    ULONGLONG userDiff = curr.userTime - prev.userTime;

    ULONGLONG total = kernelDiff + userDiff;

    if (total == 0) return 0.0;

    double usage = (double)(total - idleDiff) * 100.0 / total;

    prev = curr;
    return usage;
}

double get_memory_usage() {
    MEMORYSTATUSEX memStat;
    memStat.dwLength = sizeof(memStat);
    if (!GlobalMemoryStatusEx(&memStat)) return -1.0;
    DWORDLONG total = memStat.ullTotalPhys;
    DWORDLONG avail = memStat.ullAvailPhys;
    return ((double)(total - avail) / total) * 100.0;
}

#else
typedef struct {
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
} CPU_STAT;

int read_cpu_stat(CPU_STAT* stat) {
    FILE* f = fopen("/proc/stat", "r");
    if (!f) return 0;
    char line[256];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return 0;
    }
    fclose(f);
    sscanf(line, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu",
           &stat->user, &stat->nice, &stat->system, &stat->idle,
           &stat->iowait, &stat->irq, &stat->softirq, &stat->steal);
    return 1;
}

double get_cpu_usage() {
    static CPU_STAT prev = {0};
    CPU_STAT curr;
    if (!read_cpu_stat(&curr)) return -1.0;

    unsigned long long prev_idle = prev.idle + prev.iowait;
    unsigned long long curr_idle = curr.idle + curr.iowait;

    unsigned long long prev_non_idle = prev.user + prev.nice + prev.system + prev.irq + prev.softirq + prev.steal;
    unsigned long long curr_non_idle = curr.user + curr.nice + curr.system + curr.irq + curr.softirq + curr.steal;

    unsigned long long prev_total = prev_idle + prev_non_idle;
    unsigned long long curr_total = curr_idle + curr_non_idle;

    unsigned long long totald = curr_total - prev_total;
    unsigned long long idled = curr_idle - prev_idle;

    prev = curr;

    if (totald == 0) return 0.0;
    return ((double)(totald - idled) * 100.0) / totald;
}

double get_memory_usage() {
    FILE* f = fopen("/proc/meminfo", "r");
    if (!f) return -1.0;
    char line[256];
    unsigned long mem_total = 0, mem_free = 0, buffers = 0, cached = 0;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "MemTotal: %lu kB", &mem_total) == 1) continue;
        if (sscanf(line, "MemFree: %lu kB", &mem_free) == 1) continue;
        if (sscanf(line, "Buffers: %lu kB", &buffers) == 1) continue;
        if (sscanf(line, "Cached: %lu kB", &cached) == 1) continue;
        if (mem_total && mem_free && buffers && cached) break;
    }
    fclose(f);
    unsigned long used = mem_total - (mem_free + buffers + cached);
    if (mem_total == 0) return 0.0;
    return (double)used * 100.0 / mem_total;
}
#endif


#ifdef _WIN32
void list_processes_windows() {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        set_color(COLOR_RED);
        printf("PANIC: Process snapshot could not be created.\n");
        set_color(COLOR_RESET);
        return;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32)) {
        set_color(COLOR_RED);
        printf("PANIC: Couldnt read first process.\n");
        set_color(COLOR_RESET);
        CloseHandle(hProcessSnap);
        return;
    }

    set_color(COLOR_YELLOW);
    printf("%-7s %-25s %10s\n", "PID", "Processname", "RAM (MB)");
    set_color(COLOR_RESET);

    do {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        SIZE_T memUsageMB = 0;
        if (hProcess) {
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                memUsageMB = pmc.WorkingSetSize / (1024 * 1024);
            }
            CloseHandle(hProcess);
        }

        set_color(COLOR_GREEN);
        printf("%-7lu ", pe32.th32ProcessID);
        set_color(COLOR_CYAN);
        printf("%-25s ", pe32.szExeFile);
        set_color(COLOR_YELLOW);
        printf("%10llu\n", (unsigned long long)memUsageMB);
        set_color(COLOR_RESET);
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
}
#else
int read_proc_name(int pid, char* buffer, size_t size) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);
    FILE* f = fopen(path, "r");
    if (!f) return 0;
    if (!fgets(buffer, (int)size, f)) {
        fclose(f);
        return 0;
    }
    fclose(f);
    buffer[strcspn(buffer, "\n")] = 0;
    return 1;
}

int read_proc_mem(int pid, size_t* mem_kb) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE* f = fopen(path, "r");
    if (!f) return 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line + 6, "%zu", mem_kb);
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

void list_processes_linux() {
    DIR* proc = opendir("/proc");
    if (!proc) {
        set_color(COLOR_RED);
        printf("PANIC: /proc could not be opened.\n");
        set_color(COLOR_RESET);
        return;
    }

    struct dirent* entry;

    set_color(COLOR_YELLOW);
    printf("%-7s %-25s %10s\n", "PID", "Processsname", "RAM (MB)");
    set_color(COLOR_RESET);

    while ((entry = readdir(proc)) != NULL) {
        if (entry->d_type == DT_DIR) {
            int pid = atoi(entry->d_name);
            if (pid > 0) {
                char proc_name[256] = {0};
                if (read_proc_name(pid, proc_name, sizeof(proc_name))) {
                    size_t mem_kb = 0;
                    if (read_proc_mem(pid, &mem_kb)) {
                        set_color(COLOR_GREEN);
                        printf("%-7d ", pid);
                        set_color(COLOR_CYAN);
                        printf("%-25s ", proc_name);
                        set_color(COLOR_YELLOW);
                        printf("%10zu\n", mem_kb / 1024);
                        set_color(COLOR_RESET);
                    }
                }
            }
        }
    }
    closedir(proc);
}
#endif

void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    printf("\x1b[2J\x1b[H"); 
#endif
}

int main() {
    while (1) {
        clear_screen();

        set_color(COLOR_CYAN);
        printf("o3ShellTaskMonitor - Version 1.7.0 | Rev 0.1.0\n");
		printf("-----------------------------------------------------\n");
        printf("Copyright (c) openw3rk INVENT\n");
		printf("https://openw3rk.de\n");
		printf("develop@openw3rk.de\n");
		printf("-----------------------------------------------------\n");
		printf("o3ShellTaskMonitor comes with ABSOLUTELY NO WARRANTY.\n");
		printf("-----------------------------------------------------\n\n");
		set_color(COLOR_RESET);

        double cpu = get_cpu_usage();
        double mem = get_memory_usage();

        set_color(COLOR_YELLOW);
        if (cpu >= 0)
            printf("CPU-usage: %.2f%%   ", cpu);
        else
            printf("CPU-usage: PANIC   ");

        if (mem >= 0)
            printf("RAM-usage: %.2f%%\n", mem);
        else
            printf("RAM-usage: PANIC\n");

#ifdef _WIN32
        list_processes_windows();
#else
        list_processes_linux();
#endif

        fflush(stdout);
        msleep(2000);
    }

    return 0;
}
