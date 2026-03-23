#pragma once
#include <stdint.h>

namespace PS2Keyboard {
    // Called by the interrupt dispatcher when IRQ1 fires
    void handle(uint8_t scancode);

    // Polled by main loop
    extern volatile bool pressed;
    extern volatile char last_key;
    extern volatile uint8_t last_scancode;
}