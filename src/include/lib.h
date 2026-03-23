
#include <stddef.h>

#pragma once

void *memcpy(void *dest, const void *src, size_t n);

void *memset(void *s, int c, size_t n);

void *memmove(void *dest, const void *src, size_t n);

int memcmp(const void *s1, const void *s2, size_t n);

void itoa(uint32_t n, char* str);

void itoa_hex(uint64_t n, char* str);