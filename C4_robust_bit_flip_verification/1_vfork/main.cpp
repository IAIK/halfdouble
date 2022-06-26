#include "../../common/common.h"
#include "../../common/ptedit_header.h"

#include <wait.h>

static int check_address(volatile uint8_t *address, int size, volatile uint64_t *address_check_index) {
    int status;
    int pid = vfork();
    if ( !pid ) {
        *address_check_index = 0;
        for ( int i = 0; i < size / 8; i += 0x1000 / 8, (*address_check_index)++ ) {
            /* Child process */
            flush((void *)(address + i));
            address[i];
        }
        _exit(0);
    }
    /* Parent process resumes */
    wait(&status);

    if ( WIFSIGNALED(status) ) {
        printf("Canary died dereferencing offset: %ld\n", *address_check_index);
        return *address_check_index;
    }

    return -1;
}

static void flip_bit(volatile uint8_t *target, uint64_t bit) {
    // get the pte of the page
    ptedit_entry_t entry = ptedit_resolve((void *)target, 0);
    entry.valid          = PTEDIT_VALID_MASK_PTE;
    entry.pte            = entry.pte ^ (1UL << bit);

    ptedit_update((void *)target, 0, &entry);

    ptedit_invalidate_tlb((void *)target);
    ptedit_full_serializing_barrier();
}

// ---------------------------------------------------------------------------
int main(int argc, char *argv[]) {

    int no_ptedit = 0;

    size_t mem_size = 50001UL * 0x1000;

    int corrupt_bit_index = -1;

    if ( argc > 1 ) {
        corrupt_bit_index = atoi(argv[1]);
        int valid         = 0 <= corrupt_bit_index && corrupt_bit_index <= 63;
        if ( !valid ) {
            printf("0 <= corrupt_bit_index <= 63!\n");
            return -1;
        }
    }

    volatile uint64_t *address_check_index =
        (volatile uint64_t *)mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // this is the memory of which one page will be corrupted
    volatile uint8_t *pt =
        (volatile uint8_t *)mmap(0, mem_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // map all the pages
    for ( size_t i = 0; i < mem_size / 0x1000; i++ ) {
        pt[0x1000UL * i] = i * 64;
    }

    // ptedit is used to corrupt a page
    if ( ptedit_init() ) {
        printf("Error: Could not initalize PTEditor, not really testing?\n");
        no_ptedit = 1;
    }

    volatile uint8_t *target_page_to_corrupt = pt + 0x1000UL * 20;

    // flip bit
    if ( corrupt_bit_index != -1 && !no_ptedit ) {
        flip_bit(target_page_to_corrupt, corrupt_bit_index);
    }

    // benchmark
    uint64_t start = ns();
    check_address(pt, mem_size, address_check_index);
    uint64_t end = ns();

    // flip bit back
    if ( corrupt_bit_index != -1 && !no_ptedit ) {
        flip_bit(target_page_to_corrupt, corrupt_bit_index);
    }

    double mbps = (mem_size * 1000llu * 1000llu * 1000llu) / (double)(1024llu * 1024llu * 1024llu * (end - start));

    printf("Took %ld ns to check %ld bytes = %f GBps\n", end - start, mem_size, mbps);

    if ( !no_ptedit ) {
        ptedit_cleanup();
    }

    munmap((void *)address_check_index, 0x1000);
    munmap((void *)pt, mem_size);
}
