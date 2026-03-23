
#include <stdint.h>

#pragma once

#define COM1 0x3F8

void serial_init();

int is_transmit_empty();

void serial_put_char(char a);

void serial_print(const char* str);

void serial_print_hex(uint64_t value);