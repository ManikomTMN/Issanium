#include "memory/pmm.h"
#include "limine.h"
#include "drivers/serial.h"
#include "lib.h"

// Limine requests
__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

namespace PMM {

static uint64_t* bitmap     = nullptr;
static uint64_t  total_pages = 0;
static uint64_t  free_pages  = 0;
static uint64_t  hhdm        = 0;

// ---------------------------------------------------------------
// Bitmap helpers
// ---------------------------------------------------------------
static void bitmap_set(uint64_t page) {
    bitmap[page / 64] |= (1ULL << (page % 64));
}

static void bitmap_clear(uint64_t page) {
    bitmap[page / 64] &= ~(1ULL << (page % 64));
}

static bool bitmap_test(uint64_t page) {
    return bitmap[page / 64] & (1ULL << (page % 64));
}

// ---------------------------------------------------------------
// Init
// ---------------------------------------------------------------
void init(uint64_t hhdm_offset) {
    hhdm = hhdm_offset;

    struct limine_memmap_response* memmap = memmap_request.response;
    if (!memmap) {
        serial_print("PMM: no memmap!\n");
        return;
    }

    // Step 1 — find the highest physical address so we know
    // how big the bitmap needs to be
    uint64_t highest_addr = 0;
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry* entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t top = entry->base + entry->length;
            if (top > highest_addr) highest_addr = top;
        }
    }

    total_pages = highest_addr / 4096;
    uint64_t bitmap_size = (total_pages + 63) / 64 * 8; // bytes needed

    serial_print("PMM: total pages: ");
    // you can add a serial print of total_pages here if you add
    // a hex print helper, for now just trust it

    // Step 2 — find a usable region big enough to store the bitmap
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry* entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= bitmap_size) {
            // Store bitmap here — use virtual address via hhdm
            bitmap = (uint64_t*)(entry->base + hhdm);
            break;
        }
    }

    if (!bitmap) {
        serial_print("PMM: couldn't find space for bitmap!\n");
        return;
    }

    // Step 3 — mark everything as used to start
    memset(bitmap, 0xFF, bitmap_size);

    // Step 4 — mark usable regions as free
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry* entry = memmap->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t page_start = entry->base / 4096;
            uint64_t page_count = entry->length / 4096;
            for (uint64_t p = page_start; p < page_start + page_count; p++) {
                bitmap_clear(p);
                free_pages++;
            }
        }
    }

    // Step 5 — mark the bitmap itself as used so we don't
    // hand it out as free memory
    uint64_t bitmap_page_start = ((uint64_t)bitmap - hhdm) / 4096;
    uint64_t bitmap_page_count = (bitmap_size + 4095) / 4096;
    for (uint64_t p = bitmap_page_start; p < bitmap_page_start + bitmap_page_count; p++) {
        bitmap_set(p);
        free_pages--;
    }

    serial_print("PMM: init done\n");
}

// ---------------------------------------------------------------
// Alloc / free
// ---------------------------------------------------------------
void* alloc() {
    for (uint64_t i = 0; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            free_pages--;
            return (void*)(i * 4096); // returns PHYSICAL address
        }
    }
    serial_print("PMM: out of memory!\n");
    return nullptr;
}

void free(void* page) {
    uint64_t i = (uint64_t)page / 4096;
    bitmap_clear(i);
    free_pages++;
}

uint64_t get_total_pages() { return total_pages; }
uint64_t get_free_pages()  { return free_pages;  }

} // namespace PMM