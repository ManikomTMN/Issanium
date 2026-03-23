#pragma once
#include <stdint.h>
#include <stddef.h>

namespace PMM {
    void init(uint64_t hhdm_offset);

    void* alloc();
    void  free(void* page);

    uint64_t get_total_pages();
    uint64_t get_free_pages();
}