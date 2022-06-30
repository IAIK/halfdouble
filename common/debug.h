#pragma once

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static uint64_t get_ppn(uintptr_t ptr) {
    constexpr uint64_t mask = ((1ULL << 54) - 1);

    int fd = open("/proc/self/pagemap", O_RDONLY);
    assert(fd > 0);

    // Read the entry in the pagemap.
    uint64_t value;
    uint64_t read = pread(fd, &value, sizeof(uint64_t), ptr / 0x1000 * 8);
    assert(read == 8);
    /*if ( read != 8 ) {
        printf("warning could not read pagemap for ptr=0x%lx\n", ptr);
        return 0;
    }*/
    close(fd);

    return value & mask;
}

static uint64_t get_kpage_fags(uint64_t ppn) {
    int fd = open("/proc/kpageflags", O_RDONLY);
    assert(fd > 0);

    // Read the entry in the kpageflags.
    uint64_t value;
    uint64_t read = pread(fd, &value, sizeof(uint64_t), ppn * 8);
    assert(read == 8);
    /*if ( read != 8 ) {
        printf("warning could not read kpageflags for ppn=%lu\n", ppn);
        return 0;
    }*/
    close(fd);

    return value;
}

static bool is_huge_page(uint64_t ppn) {
    constexpr uint64_t TPH_MASK     = 1 << 17;
    constexpr uint64_t HUGE_MASK    = 1 << 22;
    constexpr uint64_t IS_HUGE_MASK = TPH_MASK | HUGE_MASK;

    return (get_kpage_fags(ppn) & IS_HUGE_MASK) > 0;
}

static uint64_t get_physical_address(uint8_t const *p) {
    constexpr uint64_t SIZE_4kB = 0x1000;
    constexpr uint64_t SIZE_2MB = 0x200000;

    constexpr uint64_t MASK_4kB = SIZE_4kB - 1;
    constexpr uint64_t MASK_2MB = SIZE_2MB - 1;

    uintptr_t vadr = (uintptr_t)p;
    uint64_t  ppn  = get_ppn(vadr);

    bool is_huge = is_huge_page(ppn);

    /*if ( is_huge ) {
        printf("error huge!\n");
    }*/

    return (ppn * 0x1000) | (vadr & (is_huge ? MASK_2MB : MASK_4kB));
}

static bool is_physical_contignious(uint8_t const *start, size_t size) {

    uint64_t last_padr = get_physical_address(start);

    for ( size_t i = 0x1000; i < size; i += 0x1000 ) {
        uint64_t padr = get_physical_address(start + i);

        uint64_t diff = padr - last_padr;
        last_padr     = padr;

        if ( diff != 0x1000 ) {
            return false;
        }
    }
    return true;
}