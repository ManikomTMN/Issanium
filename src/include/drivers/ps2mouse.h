#pragma once
#include <stdint.h>

namespace PS2Mouse {
    // Called once during kernel init, after Interrupts::init()
    void init();

    // Called by the interrupt dispatcher when IRQ12 fires
    void handle(uint8_t data);

    // Polled by main loop
    extern volatile int32_t mouse_x;
    extern volatile int32_t mouse_y;
    extern volatile bool    left_pressed;
    extern volatile bool    right_pressed;
    extern volatile bool    moved; // cleared by main loop after redraw
}