#include "../../common/common.h"
#include "../../common/debug.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <iterator>
#include <list>
#include <memory.h>
#include <optional>
#include <queue>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <time.h>
#include <vector>

constexpr size_t N = 10000;
constexpr size_t T = 10000;

static uint64_t get_bank_id(volatile uint8_t *vadr) {

    constexpr uint64_t functions[] = { 0x0000000000000100, 0x0000000000011000, 0x0000000000022000, 0x0000000000044000,
                                       0xc0000000 };

    uintptr_t padr   = get_physical_address((uint8_t *)vadr);
    uint64_t  result = 0;
    size_t    index  = 0;
    for ( uint64_t fn : functions ) {
        result |= (__builtin_parityl(padr & fn) << index++);
    }
    return result;
}

static uint64_t measure(volatile uint8_t *a0, volatile uint8_t *a1) {
    uint64_t start = ns();
    for ( size_t j = 0; j < N; j++ ) {
        *(a0);
        *(a1);
        flush((void *)a0);
        flush((void *)a1);
    }
    return ns() - start;
}

// ---------------------------------------------------------------------------
int main() {
    size_t            mem_size = 5 * 1024lu * 1024lu;
    volatile uint8_t *mem =
        (volatile uint8_t *)mmap(0, mem_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    for ( size_t i = 0; i < mem_size; i += 0x1000 ) {
        mem[i] = 0;
    }

    auto get_random_address = [&] {
        return mem + ((rand() * 0x1000) % mem_size);
    };

    volatile uint8_t *reference = get_random_address();

    std::vector<volatile uint8_t *> same_bank, cross_bank;

    for ( size_t i = 0; i < T; ++i ) {
        printf("\r%5lu/%5lu", i, T);
        fflush(stdout);
        volatile uint8_t *current = get_random_address();

        if ( get_bank_id(current) == get_bank_id(reference) )
            same_bank.push_back(current);

        if ( get_bank_id(current) != get_bank_id(reference) )
            cross_bank.push_back(current);
    }
    printf("\n");

    if ( same_bank.size() < 100 || cross_bank.size() < 100 ) {
        printf("to few addresses ... please rerun!\n");
        return -1;
    }

    uint64_t same_bank_timing = 0;

    for ( auto &sb : same_bank )
        same_bank_timing += measure(reference, sb);

    uint64_t cross_bank_timing = 0;

    for ( auto &cb : cross_bank )
        cross_bank_timing += measure(reference, cb);

    printf("same  bank: %f ns\n", same_bank_timing / (double)(same_bank.size() * N));
    printf("cross bank: %f ns\n", cross_bank_timing / (double)(cross_bank.size() * N));

    return 0;
}
