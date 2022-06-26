#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#if defined(__x86_64__)
static void flush(void *p) {
    asm volatile("clflushopt (%0)" : : "r"(p) : "memory");
}
#else
static void flush(void *p) {
    asm volatile("DC CIVAC, %0" ::"r"(p));
    asm volatile("DSB ISH");
    asm volatile("ISB");
}
#endif

#if defined(__x86_64__)
static void mfence() {
    asm volatile("mfence");
}
#else
static void mfence() {
    asm volatile("DSB ISH");
}
#endif

#if defined(__x86_64__)
static void maccess(void *p) {
    asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax");
}
#else
static void maccess(void *p) {
    volatile uint32_t value;
    asm volatile("LDR %0, [%1]\n\t" : "=r"(value) : "r"(p));
    asm volatile("DSB ISH");
    asm volatile("ISB");
}
#endif

static inline uint64_t ns() {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return ((uint64_t)tp.tv_sec) * 1000000000ULL + tp.tv_nsec;
}

#if defined(__x86_64__)
static uint64_t rdtsc() {
    uint64_t a, d;
    asm volatile("mfence");
    asm volatile("rdtsc" : "=a"(a), "=d"(d));
    a = (d << 32) | a;
    asm volatile("mfence");
    return a;
}
#else
static uint64_t rdtsc() {
    return ns();
}
#endif

#if defined(__x86_64__)

#define speculation_start(label) asm goto("call %l0" : : : : label##_retp);

#define speculation_end(label)                                                                        \
    asm goto("jmp %l0" : : : : label);                                                                \
    label##_retp : asm goto("lea %l0(%%rip), %%rax\nmovq %%rax, (%%rsp)\nret\n" : : : "rax" : label); \
label:                                                                                                \
    asm volatile("nop");

#endif

static void setcpu(int cpu) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    if ( sched_setaffinity(getpid(), sizeof(set), &set) == -1 ) {
        printf("sched_setaffinity failed\n");
        exit(1);
    }
}

static uint64_t flush_reload_t(void *ptr) {
    uint64_t start = 0, end = 0;

    start = rdtsc();
    maccess(ptr);
    end = rdtsc();

    mfence();

    flush(ptr);

    return (int)(end - start);
}

static uint64_t reload_t(void *ptr) {
    uint64_t start = 0, end = 0;
    start = rdtsc();
    maccess(ptr);
    end = rdtsc();
    return (int)(end - start);
}

static uint64_t detect_flush_reload_threshold() {
    uint64_t  reload_time = 0, flush_reload_time = 0, i, count = 1000000;
    uint64_t  dummy[16];
    uint64_t *ptr = dummy + 8;

    maccess(ptr);
    for ( i = 0; i < count; i++ ) {
        reload_time += reload_t(ptr);
    }
    for ( i = 0; i < count; i++ ) {
        flush_reload_time += flush_reload_t(ptr);
    }
    reload_time /= count;
    flush_reload_time /= count;

    return (flush_reload_time + reload_time * 2) / 3;
}
