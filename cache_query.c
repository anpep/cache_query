/* cache_query(1) -- simple cache reference counter with perf_event_open(2)
 * Copyright (C) 2019 Angel P.
 *
 * This program is licensed under the public domain and may be modified and
 * redistributed at will with no restrictions or licensing constraints.
 */

#include <asm/unistd.h>
#include <fcntl.h>
#include <linux/perf_event.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

/* contains performance event metadata */
struct perf_counter {
    int type;
    int config;
    char name[64];
};

/* measured performance events for child process */
static struct perf_counter g_counters[] = {
    { PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES, "cache_misses" },
    { PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES, "cache_references" },
    { PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, "instructions" },
    { PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, "cycles" }
};

#define NUMCOUNTERS (sizeof(g_counters) / sizeof(struct perf_counter))

/* stub for perf_event_open(2) syscall which as of version 2.30 is not yet
 * included in GNU libc */
static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu,
                int group_fd, unsigned long flags)
{
    return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

/* sets up a new performance event counter, simulating perf(1) behaviour
 * for consistency */
static int setup_count(pid_t pid, int type, int config)
{
    struct perf_event_attr attrs = {0};
    int fd;

    attrs.type = type;
    attrs.config = config;

    attrs.size = sizeof(attrs);
    attrs.sample_type = PERF_SAMPLE_IDENTIFIER; /* emulate perf(1) behavior */
    attrs.disabled = 1; /* initially disabled */
    attrs.enable_on_exec = 1; /* enable counter on exec syscall */
    attrs.inherit = 1; /* children inherit this perf(1) event */
    attrs.exclude_guest = 1; /* emulate perf(1) behavior */
   
    /* PERF_FLAG_FD_CLOEXEC avoids leaking counter descriptor to child process */
    if ((fd = perf_event_open(&attrs, pid, -1, -1, PERF_FLAG_FD_CLOEXEC)) == -1) {
        kill(pid, SIGKILL);
        perror("perf_event_open");
        _exit(EXIT_FAILURE);
    }
    if (ioctl(fd, PERF_EVENT_IOC_RESET, 0) < 0)
        return -1;
    return fd;
}

/* reads an performance event counter and closes the descriptor associated to
 * the event */
static long long read_count(int fd)
{
    long long count;
    
    if (ioctl(fd, PERF_EVENT_IOC_DISABLE, 0) < 0) {
        count = -1;
        goto out;
    }
    if (read(fd, &count, sizeof(long long)) < 0)
        count = -1;
out:
    close(fd);
    return count;
}

/* program entry point */
int main(int argc, char **argv)
{
    int i;
    int status;
    pid_t pid;
    int null_fd;
    int fd[NUMCOUNTERS]; /* descriptors for counter data */

    if (argc < 2) {
        printf("usage: cache_query PROG [ARGS...]\n\n");
        _exit(0);
    }

    switch (pid = fork()) {
        case -1: /* failure */
            perror("fork");
            _exit(EXIT_FAILURE);
        case 0: /* child process */
            null_fd = open("/dev/null", O_WRONLY | O_CREAT, 0666);
            dup2(null_fd, STDOUT_FILENO); /* redirect stdout to /dev/null */
            sleep(1); /* FIXME: hack so that parent has a chance to perform perf_event_open(2) before exec */
            if (execvp(argv[1], argv + 1) < 0)
                perror("execvp");
            _exit(EXIT_FAILURE);
        default: /* parent process */
            /* set up counters */
            for (i = 0; i < NUMCOUNTERS; i++) {
                if ((fd[i] = setup_count(pid, g_counters[i].type, g_counters[i].config)) < 0) {
                    perror("setup_count");
                    _exit(EXIT_FAILURE);
                }
            }
            /* wait for child process to exit and handle child errors */
            if (waitpid(pid, &status, 0) == -1 || WEXITSTATUS(status))
                _exit(EXIT_FAILURE);
            /* read and display results */
            for (i = 0; i < NUMCOUNTERS; i++)
                printf("%s=%lld\n", g_counters[i].name, read_count(fd[i]));
    }
    return 0;
}

