#include "../../common/common.h"
#include "../../common/ptedit_header.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <deque>
#include <iostream>
#include <iterator>
#include <list>
#include <memory.h>
#include <numeric>
#include <optional>
#include <queue>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <time.h>
#include <vector>

constexpr size_t N              = 1000;
constexpr size_t PATTERN_LENGTH = 16;

// structure for contigious chunks
struct Chunk {
    volatile uint8_t *start;
    size_t            size;
};

// DEBUG ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
#if defined(DEBUG)
#include "../../common/debug.h"

// this function prints and verifies the found chunks (only in DEBUG)
static void print_chunck(Chunk const &c) {
    bool is_correct = is_physical_contignious((uint8_t *)c.start, c.size);
    printf("Chunk: %p + [%8.1f kB] -> %s\n", c.start, c.size / 1024.0, is_correct ? "c" : "X");
}

#endif
// ------------------------------------------------------------------------------------------

// we use two periods so we can simplify searching
using pattern_t = std::array<size_t, 2 * PATTERN_LENGTH>;

// the access patterns as in the pattern (Table 6) for 2 periods.
constexpr std::array<pattern_t, 4> patterns = {
    pattern_t { 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 1, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 1 },
    pattern_t { 8, 7, 8, 11, 8, 7, 8, 11, 8, 7, 8, 11, 8, 7, 8, 3, 8, 7, 8, 11, 8, 7, 8, 11, 8, 7, 8, 11, 8, 7, 8, 3 },
    pattern_t { 8, 9, 8, 5, 8, 9, 8, 13, 8, 9, 8, 5, 8, 9, 8, 5, 8, 9, 8, 5, 8, 9, 8, 13, 8, 9, 8, 5, 8, 9, 8, 5 },
    pattern_t { 8, 7, 8, 7, 8, 7, 8, 15, 8, 7, 8, 7, 8, 7, 8, 7, 8, 7, 8, 7, 8, 7, 8, 15, 8, 7, 8, 7, 8, 7, 8, 7 },
};

using chunks_t = std::vector<Chunk>;

// check for bank conflict
[[gnu::noinline]] static bool are_same_bank(volatile uint8_t *a0, volatile uint8_t *a1, uint64_t same_bank_threshold) {
    uint64_t start = ns();
    for ( size_t j = 0; j < N; j++ ) {
        *(a0);
        *(a1);
        flush((void *)a0);
        flush((void *)a1);
    }
    uint64_t duration = ns() - start;
    // printf("%lu\n", duration);
    return duration > (same_bank_threshold * N);
}

[[gnu::noinline]] static chunks_t find_continuous_memory(volatile uint8_t *mem, size_t len,
                                                         uint64_t same_bank_threshold) {
    size_t last_index = 0;

    std::deque<size_t> distance_history;

    std::optional<Chunk> current = std::nullopt;

    chunks_t chunks;

    volatile uint8_t *reference = mem;

    for ( size_t index = 1; index < (len / 0x1000); ++index ) {

#if defined(DEBUG)
        printf("%5.1f%%\r", index * 100.0 / (len / 0x1000));
        fflush(stdout);
#endif

        volatile uint8_t *candidate = mem + index * 0x1000;

        if ( !are_same_bank(reference, candidate, same_bank_threshold) ) {
            continue;
        }

        size_t distance = index - last_index;
        last_index      = index;

        distance_history.push_back(distance);

        if ( distance_history.size() <= PATTERN_LENGTH ) {
            // wait till enough elements to compare with our patterns
            continue;
        }
        // ensure buffer is always one pattern length long
        distance_history.pop_front();
        assert(distance_history.size() == PATTERN_LENGTH);

        // check if one pattern matches
        bool contigious = std::any_of(patterns.begin(), patterns.end(), [&](pattern_t const &p) {
            return std::search(p.begin(), p.end(), distance_history.begin(), distance_history.end()) != p.end();
        });

        // logic to close current pattern if a new chunk is found
        if ( contigious ) {
            if ( current ) {
                // increment current chunck
                current->size += distance * 0x1000;
            }
            else {
                // create new chunk
                size_t size = std::accumulate(distance_history.begin(), distance_history.end(), 0) * 0x1000;
                current     = Chunk { candidate - size + 0x1000, size };
            }
        }
        else if ( current ) {
            // finalize current chunck
            chunks.push_back(*current);
            current = std::nullopt;
        }
    }

    if ( current ) {
        // finalize last chunck
        chunks.push_back(*current);
    }

    printf("\n");

    return chunks;
}

int main(int argc, char *argv[]) {

#if defined(DEBUG)
    printf("running in DEBUG!\n");
#endif

    if ( argc != 2 ) {
        printf("usage: %s same_bank_threshold\n", argv[0]);
        return 0;
    }

    uint64_t same_bank_threshold = strtol(argv[1], nullptr, 10);

    size_t mem_size = 2 * 50000UL * 0x1000;

    volatile uint8_t *mem =
        (volatile uint8_t *)mmap(0, mem_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if ( mem == MAP_FAILED ) {
        printf("mmap failed!\n");
        return -1;
    }

    // access each page once to make sure it is actually mapped
    for ( size_t i = 0; i < mem_size; i += 0x1000 ) {
        mem[i] = i;
    }

    uint64_t                  start  = ns();
    [[maybe_unused]] chunks_t chunks = find_continuous_memory(mem, mem_size, same_bank_threshold);
    uint64_t                  end    = ns();

#if defined(DEBUG)
    for ( Chunk const &x : chunks ) {
        print_chunck(x);
    }
#endif

    double GBps = (mem_size * 1000llu * 1000llu * 1000llu) / (double)(1024llu * 1024llu * 1024llu * (end - start));

    printf("Took %ld ns to check %ld bytes = %f GBps\n", end - start, mem_size, GBps);

    return 0;
}
