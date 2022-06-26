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
constexpr size_t _2MB           = 0x200000;
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

using chunks_t = std::vector<Chunk>;

[[gnu::noinline]] static chunks_t find_continuous_memory(volatile uint8_t *mem, size_t len) {

    assert(len > 2 * _2MB);

    uint64_t alignment_offset = _2MB - ((uint64_t)mem % _2MB);

    if ( alignment_offset == _2MB )
        alignment_offset = 0;

    mem += alignment_offset;
    len -= alignment_offset;

    len -= (len % _2MB);

    chunks_t chunks;

    for ( size_t offset = 0; offset < len; offset += _2MB ) {
        chunks.push_back({ mem + offset, _2MB });
    }

    return chunks;
}

int main(int argc, char *argv[]) {

#if defined(DEBUG)
    printf("running in DEBUG!\n");
#endif

    size_t mem_size = 50000UL * 0x1000;

    volatile uint8_t *mem =
        (volatile uint8_t *)mmap(0, mem_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if ( mem == MAP_FAILED ) {
        printf("mmap failed!\n");
        return -1;
    }

    if ( madvise((void *)mem, mem_size, MADV_HUGEPAGE) != 0 ) {
        printf("madvise failed!\n");
        munmap((void *)mem, mem_size);
        return -1;
    }

    // access each page once to make sure it is actually mapped
    for ( size_t i = 0; i < mem_size; i += 0x1000 ) {
        mem[i] = i;
    }

    uint64_t                  start  = ns();
    [[maybe_unused]] chunks_t chunks = find_continuous_memory(mem, mem_size);
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
