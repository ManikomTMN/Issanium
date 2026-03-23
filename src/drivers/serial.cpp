
#include "drivers/serial.h"
#include "arch/x86_64/cpu.h"

void serial_init() {
    outb(COM1 + 1, 0x00);    // Disable all interrupts
    
    // set baud rate
    outb(COM1 + 3, 0x80);    // enable dlab (to set baud rate divisor)
    outb(COM1 + 0, 0x03);    // set divisor to 3 (lo byte) for 38400 baud
    outb(COM1 + 1, 0x00);    // (hi byte)
    outb(COM1 + 3, 0x03); // disable dlab
    outb(COM1 + 2, 0xC7);    // enable FIFO (first in first out), clear them, with 14-byte threshold
    outb(COM1 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

int is_transmit_empty() {
    return inb(COM1 + 5) & 0x20;
}

void serial_put_char(char a) {
    while (is_transmit_empty() == 0);

    outb(COM1, a);
}

void serial_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        serial_put_char(str[i]);
    }
}

void serial_print_hex(uint64_t value) {
    const char* hex = "0123456789ABCDEF";
    char buf[19];
    buf[0] = '0'; buf[1] = 'x'; buf[18] = '\0';
    for (int i = 0; i < 16; i++) {
        buf[17 - i] = hex[value & 0xF];
        value >>= 4;
    }
    serial_print(buf);
}