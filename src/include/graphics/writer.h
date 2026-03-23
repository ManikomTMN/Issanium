#pragma once
#include <stdint.h>

namespace Writer {
    void init();

    void put_char(uint32_t unicode, int x, int y, uint32_t color);
    void print(const char* text, int x, int y, uint32_t color);
}