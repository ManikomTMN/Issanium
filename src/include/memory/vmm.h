#pragma once
#include <stdint.h>

namespace VMM {
    void init(uint64_t hhdm_offset);

    // Map a virtual address to a physical address
    void map(void* virt, void* phys, uint64_t flags);

    // Unmap a virtual address
    void unmap(void* virt);

    // Get the physical address a virtual address maps to
    // returns nullptr if not mapped
    void* virt_to_phys(void* virt);

    // Page flags
    static constexpr uint64_t FLAG_PRESENT   = 1 << 0;
    static constexpr uint64_t FLAG_WRITABLE  = 1 << 1;
    static constexpr uint64_t FLAG_USER      = 1 << 2;
    static constexpr uint64_t FLAG_NX        = 1ULL << 63; // no execute
}