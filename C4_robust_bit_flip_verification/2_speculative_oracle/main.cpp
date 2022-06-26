/* See LICENSE file for license and copyright information */

#include "../../common/common.h"
#include "../../common/ptedit_header.h"

#include <math.h>
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define RUNS 1000

#if defined(__x86_64__)
#define RSB 1
#else
#define RSB 0
#endif

/* Alternative metrics: min, max */
#define METRIC (sum / TRIES)

#define LENGTH(x) (sizeof(x) / sizeof((x)[0]))

static uint64_t CACHE_MISS;

static size_t        TRIES = 10000;
static unsigned char throttle[8 * 4096];
static float         runtimes0[10000] = { 0 };
static float         runtimes1[10000] = { 0 };
static float         runtimes2[10000] = { 0 };

void access_array(char *mem, char *mydata, int run) {

#if defined(__x86_64__)
    speculation_start(s);
    maccess(mem + (mydata[0] & 1));
    speculation_end(s);
#else

    // if ( (((float)run) / 1.0) > 1.0 ) {
    //    maccess(mem + (mydata[0] & 1));
    //}
#if FAST == 1
    flush(&run); // not needed

    if ( ((run) / throttle[0]) > (1.0) / throttle[0] ) {
        maccess(mem + (mydata[0] & 1));
    }
#else
    flush(&run);                // not needed
    flush(&throttle[0]);        // not needed
    flush(&throttle[4096]);     // not needed
    flush(&throttle[2 * 4096]); // not needed
    mfence();

    // if (((run) / throttle[0]) > (1.0) / throttle[0]) {
    if ( ((run) / throttle[0]) > (1.0) / throttle[4096] / throttle[2 * 4096] / throttle[3 * 4096] ) {
        maccess(mem + (mydata[0] & 1));
    }
#endif
#endif
}

size_t measure(char *buffer, char *fakebuffer, char *address) {
#if 1
    /* Mistrain */
    for ( size_t j = 0; j < TRIES; j++ ) {
        /* Mistrain */
        for ( int y = 0; y < 10; y++ ) {
            access_array(fakebuffer, fakebuffer, 2);
        }

        flush(buffer);
        mfence();

        access_array(buffer, address, 1);

        mfence();

        if ( flush_reload_t(buffer) < CACHE_MISS ) {
            return 1;
        }
    }

    return 0;
#else
    for ( size_t j = 0; j < TRIES; j++ ) {
        if ( !setjmp(trycatch_buf) ) {
            maccess(0);
            maccess(buffer + (address[0] & 1));
        }

        if ( flush_reload_t(buffer) < CACHE_MISS ) {
            return 1;
        }
    }

    return 0;
#endif
}

void compute_statistics(float *values, size_t n, float *average, float *variance, float *std_deviation,
                        float *std_error) {
    float sum = 0.0;

    for ( size_t i = 0; i < n; i++ ) {
        sum += values[i];
    }

    float _average = sum / (float)n;

    float sum1 = 0.0;
    for ( size_t i = 0; i < n; i++ ) {
        sum1 += powf(values[i] - _average, 2);
    }

    float _variance = sum1 / (float)n;

    if ( average != NULL ) {
        *average = _average;
    }

    if ( variance != NULL ) {
        *variance = _variance;
    }

    float _std_deviation = sqrtf(_variance);
    if ( std_deviation != NULL ) {
        *std_deviation = _std_deviation;
    }

    if ( std_error != NULL ) {
        *std_error = _std_deviation / sqrtf(n);
    }
}

int main(int argc, char *argv[]) {
    // signal(SIGSEGV, trycatch_segfault_handler);
    // signal(SIGFPE, trycatch_segfault_handler);
    int correct_measurement_counter = 0;
    int false_negative_counter      = 0;
    int false_positive_counter      = 0;

    /* Setup */
    if ( ptedit_init() ) {
        printf("Error: Could not initalize PTEditor, did you load the kernel module?\n");
        return 1;
    }

    for ( int j = 0; j < 8; j++ ) {
        throttle[j * 4096] = 1;
    }

    CACHE_MISS = detect_flush_reload_threshold();
    fprintf(stderr, "[+] Flush+Reload Threshold: %zu\n", CACHE_MISS);

    char *mapping_valid =
        (char *)mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    memset(mapping_valid, 9, 4096);

    char *mapping_invalid =
        (char *)mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    memset(mapping_invalid, 157, 4096);

    char *buffer = (char *)mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    memset(buffer, 0, 4096);

    char *fakebuffer = (char *)mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    memset(fakebuffer, 0, 4096);

    // create the invalid mapping
    ptedit_entry_t entry_inv = ptedit_resolve(mapping_invalid, 0);
    entry_inv.pte            = entry_inv.pte | (1UL << 41);
    entry_inv.valid          = PTEDIT_VALID_MASK_PTE;
    ptedit_update(mapping_invalid, 0, &entry_inv);
    ptedit_invalidate_tlb(mapping_invalid);

    FILE *log = fopen("log.csv", "w+");
    if ( log == NULL ) {
        fprintf(stderr, "Can not open log file\n");
        return -1;
    }
    fprintf(log, "Tries,SuccessRate,Average,StdError,Average0,StdError0,Average1,StdError1\n");

    fprintf(stderr, "[+] n: %d\n", RUNS);

    for ( int j = 0; j <= 10000; j += 1 ) {
        if ( j == 0 ) {
            TRIES = 1;
        }
        else {
            TRIES = j;
        }

        size_t correct_counter   = 0;
        size_t incorrect_counter = 0;

        for ( size_t i = 0; i < RUNS; i++ ) {

            // 50/50 corrupt the page or not

            bool correct = rand() % 2 == 0;

            char *current_mapping = correct ? mapping_valid : mapping_invalid;

            uint64_t start = ns();
            size_t   res   = measure(buffer, fakebuffer, current_mapping);
            uint64_t end   = ns();

            float diff = (float)end - start;

            if ( correct ) {
                runtimes1[correct_counter] = diff;
                correct_counter++;
            }
            else {
                runtimes0[incorrect_counter] = diff;
                incorrect_counter++;
            }

            runtimes2[i] = diff;

            if ( res > 0 ) {
                if ( correct ) {
                    correct_measurement_counter++;
                }
                else {
                    false_positive_counter++;
                }
            }
            else {
                if ( correct ) {
                    false_negative_counter++;
                }
                else {
                    correct_measurement_counter++;
                }
            }

            sched_yield();
        }

        float average       = 0;
        float std_deviation = 0;
        float std_error     = 0;
        compute_statistics(runtimes2, RUNS, &average, NULL, &std_deviation, &std_error);

        float average0       = 0;
        float std_deviation0 = 0;
        float std_error0     = 0;
        compute_statistics(runtimes0, incorrect_counter, &average0, NULL, &std_deviation0, &std_error0);

        float average1       = 0;
        float std_deviation1 = 0;
        float std_error1     = 0;
        compute_statistics(runtimes1, correct_counter, &average1, NULL, &std_deviation1, &std_error1);

        float success_rate = ((float)correct_measurement_counter / RUNS) * 100.;

        printf("%5ld: Success rate: %5.2f%% - %5.3f ms (+-%.3f) [ %5.3f ms (+-%.3f) - %5.3f ms (+-%.3f)]\n", TRIES,
               success_rate, average / 1e6, std_error / 1e6, average0 / 1e6, std_error0 / 1e6, average1 / 1e6,
               std_error1 / 1e6);
        /* printf("%d,%d,%d,%d,%.3f ms,%.3f ms\n", TRIES, correct_measurement_counter, false_negative_counter, */
        /*        false_positive_counter, average0 / 1e6, average1 / 1e6); */
        fprintf(log, "%ld,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n", TRIES, success_rate, average / 1e6, std_error / 1e6,
                average0 / 1e6, std_error0 / 1e6, average1 / 1e6, std_error1 / 1e6);
        fflush(log);

        correct_measurement_counter = 0;
        false_negative_counter      = 0;
        false_positive_counter      = 0;
    }

    fclose(log);

    /* Clean-up */
    munmap(mapping_valid, 4096);
    munmap(mapping_invalid, 4096);
    munmap(buffer, 4096);

    ptedit_cleanup();

    return 0;
}
