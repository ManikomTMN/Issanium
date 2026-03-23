#include "memory/heap.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "lib.h"
#include "drivers/serial.h"

namespace Heap {

// ---------------------------------------------------------------
// Heap chunk header
// ---------------------------------------------------------------
struct Header {
    size_t   size;  // size of the data region (not including header)
    bool     used;
    Header*  next;
    Header*  prev;
};

// Where the heap lives in virtual memory
static constexpr uint64_t HEAP_START = 0x0000100000000000;
static constexpr size_t   HEAP_INITIAL_PAGES = 1024; // 4MB to start

static Header* heap_start = nullptr;

// Expand heap by mapping more physical pages
static void* heap_end = (void*)HEAP_START;

static void expand(size_t pages) {
    for (size_t i = 0; i < pages; i++) {
        void* phys = PMM::alloc();
        if (!phys) {
            serial_print("Heap: out of physical memory!\n");
            return;
        }
        VMM::map(heap_end, phys, VMM::FLAG_WRITABLE);
        heap_end = (void*)((uint64_t)heap_end + 4096);
    }
}

// Init
void init() {
    // Map initial heap pages
    expand(HEAP_INITIAL_PAGES);

    // Set up the first chunk covering the whole heap
    heap_start = (Header*)HEAP_START;
    heap_start->size = (HEAP_INITIAL_PAGES * 4096) - sizeof(Header);
    heap_start->used = false;
    heap_start->next = nullptr;
    heap_start->prev = nullptr;

    serial_print("Heap: init done\n");
}

void* kmalloc(size_t size) {
    // Align size to 8 bytes so headers are always aligned
    size = (size + 7) & ~7ULL;

    Header* current = heap_start;

    while (current) {
        // Skip used or too small chunks
        if (current->used || current->size < size) {
            current = current->next;
            continue;
        }

        // Split the chunk if there's enough space left over
        // for a new header + at least 8 bytes of data
        if (current->size >= size + sizeof(Header) + 8) {
            Header* new_chunk = (Header*)((uint8_t*)current + sizeof(Header) + size);
            new_chunk->size = current->size - size - sizeof(Header);
            new_chunk->used = false;
            new_chunk->next = current->next;
            new_chunk->prev = current;

            if (current->next) current->next->prev = new_chunk;
            current->next = new_chunk;
            current->size = size;
        }

        current->used = true;
        return (void*)((uint8_t*)current + sizeof(Header));
    }

    // No free chunk found so expand heap and try again
    serial_print("Heap: expanding...\n");
    expand(64); // 256KB more

    // Add new chunk at the end
    Header* last = heap_start;
    while (last->next) last = last->next;

    Header* new_chunk = (Header*)heap_end;
    new_chunk->size = (64 * 4096) - sizeof(Header);
    new_chunk->used = false;
    new_chunk->next = nullptr;
    new_chunk->prev = last;
    last->next = new_chunk;

    // Try again
    return kmalloc(size);
}


void kfree(void* ptr) {
    if (!ptr) return;

    // Get the header sitting just before the data
    Header* header = (Header*)((uint8_t*)ptr - sizeof(Header));
    header->used = false;

    // Merge with next chunk if it's free
    if (header->next && !header->next->used) {
        header->size += sizeof(Header) + header->next->size;
        header->next = header->next->next;
        if (header->next) header->next->prev = header;
    }

    // Merge with previous chunk if it's free
    if (header->prev && !header->prev->used) {
        header->prev->size += sizeof(Header) + header->size;
        header->prev->next = header->next;
        if (header->next) header->next->prev = header->prev;
    }
}

} // namespace Heap