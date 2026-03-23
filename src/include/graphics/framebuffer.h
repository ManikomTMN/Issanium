#pragma once
#include <stdint.h>
#include <stddef.h>
#include "limine.h"

class Framebuffer {
public:
    // Initializes the framebuffer from the Limine request
    static bool init();

    // Core drawing functions
    static void put_pixel(uint32_t x, uint32_t y, uint32_t color);
    static void put_pixel_alpha(uint32_t x, uint32_t y, uint32_t color);
    static void clear(uint32_t color);

    static void update_frontbuffer();

    static void draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);

    static void draw_tga_image(uint32_t x, uint32_t y, const uint8_t* tga_data);
    static void draw_tga_scaled(uint32_t x, uint32_t y, uint32_t target_w, uint32_t target_h, const uint8_t* tga_data);

    static uint64_t get_width();
    static uint64_t get_height();

    static uint32_t* back_buffer;

private:
    static struct limine_framebuffer* fb;
};
