// THE CODE IS IN THE H FILE SOTHAT WHEN IT RUNS IT WILL JUST COPY AND PASTE THE COMMANDS INTO PLCAE (i think?)

#pragma once
#include <stdint.h>

// Send a byte to a hardware port
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Read a byte from a hardware port
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Tell the CPU to wait for the next interrupt (saves power/heat)
static inline void hlt() {
    asm volatile ("hlt");
}