#include "graphics/framebuffer.h"
#include "memory/heap.h"
#include "lib.h"

// We place the request inside the .cpp so main doesn't need to see Limine internals
__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

// Initialize the static pointer
struct limine_framebuffer* Framebuffer::fb = nullptr;

uint32_t* Framebuffer::back_buffer = nullptr;

size_t framebuffer_bytes;

bool Framebuffer::init() {
    if (framebuffer_request.response == nullptr || 
        framebuffer_request.response->framebuffer_count < 1) {
        return false;
    }

    fb = framebuffer_request.response->framebuffers[0]; // set fb FIRST

    size_t framebuffer_bytes = fb->height * fb->pitch; // 
    back_buffer = (uint32_t*)Heap::kmalloc(framebuffer_bytes);

    return true;
}

void Framebuffer::put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!fb) return;

    // (y * pitch) + (x * bytes_per_pixel)
    // assume 32-bit (4 bytes) treat the address as uint32_t*

    uint32_t* back_ptr = (uint32_t*)back_buffer;
    back_ptr[y * (fb->pitch / 4) + x] = color;
}

void Framebuffer::put_pixel_alpha(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb->width || y >= fb->height) return;

    uint8_t alpha = (color >> 24) & 0xFF;
    if (alpha == 0) return;
    if (alpha == 255) {
        put_pixel(x, y, color);
        return;
    }

    uint32_t* back_ptr = (uint32_t*)back_buffer;
    uint32_t offset = y * (fb->pitch / 4) + x;
    uint32_t bg_color = back_ptr[offset]; // read from backbuffer

    // Extract channels
    uint32_t rb = color & 0xFF00FF;
    uint32_t g  = color & 0x00FF00;
    uint32_t rb_bg = bg_color & 0xFF00FF;
    uint32_t g_bg  = bg_color & 0x00FF00;

    // Blend channels: (src * alpha + dst * (255 - alpha)) / 256
    // i use 256 here for some bit-shift speed trick
    uint32_t rb_out = ((((rb - rb_bg) * alpha) >> 8) + rb_bg) & 0xFF00FF;
    uint32_t g_out  = ((((g - g_bg) * alpha) >> 8) + g_bg) & 0x00FF00;

    back_ptr[offset] = rb_out | g_out | (0xFF << 24); // write to backbuffer
}

void Framebuffer::draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    if (!fb) return;

    for (uint32_t curr_y = y; curr_y < y + height; curr_y++) {
        if (curr_y >= fb->height) break;

        for (uint32_t curr_x = x; curr_x < x + width; curr_x++) {
            if (curr_x >= fb->width) break;

            put_pixel(curr_x, curr_y, color);
        }
    }
}

void Framebuffer::clear(uint32_t color) {
    if (!fb) return;

    for (uint32_t y = 0; y < fb->height; y++) {
        for (uint32_t x = 0; x < fb->width; x++) {
            put_pixel(x, y, color);
        }
    }
}

uint64_t Framebuffer::get_width() { return fb ? fb->width : 0; }
uint64_t Framebuffer::get_height() { return fb ? fb->height : 0; }


struct TGAHeader {
    uint8_t  id_length;
    uint8_t  color_map_type;
    uint8_t  image_type;
    uint16_t color_map_origin;
    uint16_t color_map_length;
    uint8_t  color_map_depth;
    uint16_t x_origin;
    uint16_t y_origin;
    uint16_t width;
    uint16_t height;
    uint8_t  bits_per_pixel;
    uint8_t  image_descriptor;
} __attribute__((packed));

void Framebuffer::draw_tga_image(uint32_t x, uint32_t y, const uint8_t* tga_data) {
    TGAHeader* header = (TGAHeader*)tga_data;
    const uint8_t* pixel_data = tga_data + sizeof(TGAHeader) + header->id_length;
    
    bool flip_y = !(header->image_descriptor & 0x20);

    for (uint32_t i = 0; i < header->height; i++) {
        for (uint32_t j = 0; j < header->width; j++) {
            uint32_t color;
            if (header->bits_per_pixel == 32) {
                const uint32_t* pixels = (const uint32_t*)pixel_data;
                color = pixels[i * header->width + j];
            } else { // 24bpp
                const uint8_t* p = pixel_data + (i * header->width + j) * 3;
                color = (0xFF << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
            }

            uint32_t draw_y = flip_y ? (y + header->height - 1 - i) : (y + i);
            put_pixel_alpha(x + j, draw_y, color);
        }
    }
}

void Framebuffer::draw_tga_scaled(uint32_t x, uint32_t y, uint32_t target_w, uint32_t target_h, const uint8_t* tga_data) {
    TGAHeader* header = (TGAHeader*)tga_data;
    uint32_t* pixel_data = (uint32_t*)(tga_data + sizeof(TGAHeader) + header->id_length);
    
    bool flip_y = !(header->image_descriptor & 0x20);

    // use Fixed Point math (multiply by 65536) to avoid floating point in the kernel
    uint32_t x_ratio = ((uint32_t)header->width  << 16) / (uint32_t)target_w;
    uint32_t y_ratio = ((uint32_t)header->height << 16) / (uint32_t)target_h;

    for (uint32_t i = 0; i < target_h; i++) {
        for (uint32_t j = 0; j < target_w; j++) {
            // Find the source pixel
            uint32_t src_x = (j * x_ratio) >> 16;
            uint32_t src_y = (i * y_ratio) >> 16;

            uint32_t color;
            if (header->bits_per_pixel == 32) {
                uint32_t* pixels = (uint32_t*)(tga_data + sizeof(TGAHeader) + header->id_length);
                color = pixels[src_y * header->width + src_x];
            } else { // 24bpp
                const uint8_t* pixels = tga_data + sizeof(TGAHeader) + header->id_length;
                const uint8_t* p = pixels + (src_y * header->width + src_x) * 3;
                color = (0xFF << 24) | (p[2] << 16) | (p[1] << 8) | p[0]; // BGR -> ARGB
            }

            uint32_t draw_y = flip_y ? (y + target_h - 1 - i) : (y + i);

            // stupid alpha funcion
            put_pixel_alpha(x + j, draw_y, color);
        }
    }
}


void Framebuffer::update_frontbuffer() {
    if (!fb || !back_buffer) return;
    memcpy((void*)fb->address, back_buffer, fb->height * fb->pitch);
}