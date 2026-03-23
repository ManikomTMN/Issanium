#pragma once
#include <stdint.h>
#include <stddef.h>

namespace Heap {
    void init();
    void* kmalloc(size_t size);
    void  kfree(void* ptr);
}