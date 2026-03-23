#include "memory/vmm.h"
#include "memory/pmm.h"
#include "lib.h"
#include "drivers/serial.h"

namespace VMM {

static uint64_t hhdm = 0;
static uint64_t* pml4 = nullptr;

// ---------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------

// Convert a physical address to a virtual one we can actually access
static void* phys_to_virt(void* phys) {
    return (void*)((uint64_t)phys + hhdm);
}

static void* virt_to_phys_internal(void* virt) {
    return (void*)((uint64_t)virt - hhdm);
}

// Get or create a page table entry at the given level
// table   = virtual address of current level table
// index   = which entry in the table
// returns virtual address of the next level table
static uint64_t* get_or_create_table(uint64_t* table, uint64_t index, uint64_t flags) {
    if (table[index] & FLAG_PRESENT) {
        // already exists — mask off the flags to get the physical address
        return (uint64_t*)phys_to_virt((void*)(table[index] & ~0xFFFULL));
    }

    // allocate a new page for this table
    void* new_table_phys = PMM::alloc();
    if (!new_table_phys) {
        serial_print("VMM: out of memory allocating page table!\n");
        return nullptr;
    }

    // zero it out
    void* new_table_virt = phys_to_virt(new_table_phys);
    memset(new_table_virt, 0, 4096);

    // write the entry — physical address + flags
    table[index] = (uint64_t)new_table_phys | flags | FLAG_PRESENT;

    return (uint64_t*)new_table_virt;
}

// ---------------------------------------------------------------
// Init
// ---------------------------------------------------------------
void init(uint64_t hhdm_offset) {
    hhdm = hhdm_offset;

    // Allocate and zero our PML4
    void* pml4_phys = PMM::alloc();
    pml4 = (uint64_t*)phys_to_virt(pml4_phys);
    memset(pml4, 0, 4096);

    // Copy Limine's higher half mappings (indices 256-511)
    // so we don't lose kernel/framebuffer/hhdm mappings
    uint64_t* old_pml4;
    asm volatile("mov %%cr3, %0" : "=r"(old_pml4));
    old_pml4 = (uint64_t*)phys_to_virt(old_pml4);

    for (int i = 256; i < 512; i++) {
        pml4[i] = old_pml4[i];
    }

    // Load our new PML4 into CR3
    asm volatile("mov %0, %%cr3" : : "r"(pml4_phys) : "memory");

    serial_print("VMM: init done\n");
}

// ---------------------------------------------------------------
// Map
// ---------------------------------------------------------------
void map(void* virt, void* phys, uint64_t flags) {
    uint64_t v = (uint64_t)virt;

    // Extract the 4 indices from the virtual address
    uint64_t pml4_idx = (v >> 39) & 0x1FF;
    uint64_t pdpt_idx = (v >> 30) & 0x1FF;
    uint64_t pd_idx   = (v >> 21) & 0x1FF;
    uint64_t pt_idx   = (v >> 12) & 0x1FF;

    // Walk/create each level
    uint64_t* pdpt = get_or_create_table(pml4, pml4_idx, FLAG_WRITABLE | FLAG_USER);
    if (!pdpt) return;

    uint64_t* pd = get_or_create_table(pdpt, pdpt_idx, FLAG_WRITABLE | FLAG_USER);
    if (!pd) return;

    uint64_t* pt = get_or_create_table(pd, pd_idx, FLAG_WRITABLE | FLAG_USER);
    if (!pt) return;

    // Write the final mapping
    pt[pt_idx] = (uint64_t)phys | flags | FLAG_PRESENT;

    // Tell CPU to flush this virtual address from its TLB cache
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

// ---------------------------------------------------------------
// Unmap
// ---------------------------------------------------------------
void unmap(void* virt) {
    uint64_t v = (uint64_t)virt;

    uint64_t pml4_idx = (v >> 39) & 0x1FF;
    uint64_t pdpt_idx = (v >> 30) & 0x1FF;
    uint64_t pd_idx   = (v >> 21) & 0x1FF;
    uint64_t pt_idx   = (v >> 12) & 0x1FF;

    if (!(pml4[pml4_idx] & FLAG_PRESENT)) return;
    uint64_t* pdpt = (uint64_t*)phys_to_virt((void*)(pml4[pml4_idx] & ~0xFFFULL));

    if (!(pdpt[pdpt_idx] & FLAG_PRESENT)) return;
    uint64_t* pd = (uint64_t*)phys_to_virt((void*)(pdpt[pdpt_idx] & ~0xFFFULL));

    if (!(pd[pd_idx] & FLAG_PRESENT)) return;
    uint64_t* pt = (uint64_t*)phys_to_virt((void*)(pd[pd_idx] & ~0xFFFULL));

    pt[pt_idx] = 0;

    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

// ---------------------------------------------------------------
// Virt to phys
// ---------------------------------------------------------------
void* virt_to_phys(void* virt) {
    uint64_t v = (uint64_t)virt;

    uint64_t pml4_idx = (v >> 39) & 0x1FF;
    uint64_t pdpt_idx = (v >> 30) & 0x1FF;
    uint64_t pd_idx   = (v >> 21) & 0x1FF;
    uint64_t pt_idx   = (v >> 12) & 0x1FF;

    if (!(pml4[pml4_idx] & FLAG_PRESENT)) return nullptr;
    uint64_t* pdpt = (uint64_t*)phys_to_virt((void*)(pml4[pml4_idx] & ~0xFFFULL));

    if (!(pdpt[pdpt_idx] & FLAG_PRESENT)) return nullptr;
    uint64_t* pd = (uint64_t*)phys_to_virt((void*)(pdpt[pdpt_idx] & ~0xFFFULL));

    if (!(pd[pd_idx] & FLAG_PRESENT)) return nullptr;
    uint64_t* pt = (uint64_t*)phys_to_virt((void*)(pd[pd_idx] & ~0xFFFULL));

    if (!(pt[pt_idx] & FLAG_PRESENT)) return nullptr;
    return (void*)(pt[pt_idx] & ~0xFFFULL);
}

} // namespace VMM