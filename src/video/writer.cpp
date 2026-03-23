#include "graphics/writer.h"
#include "graphics/framebuffer.h"
#include <stdint.h>

// PSF2 Header Structure
struct psf2_t {
    uint32_t magic;         // 0x864ab572
    uint32_t version;       // Usually 0
    uint32_t headersize;    // Offset to the glyphs
    uint32_t flags;         // 0 if no unicode table
    uint32_t numglyph;      // Number of glyphs in font
    uint32_t bytesperglyph; // Size of one glyph in bytes
    uint32_t height;        // Height in pixels
    uint32_t width;         // Width in pixels
};

// THE symbol made my objcopy
extern "C" uint8_t _binary_assets_font_psf_start[];

void Writer::init() {
    psf2_t* font = (psf2_t*)_binary_assets_font_psf_start;
    
    // PSF2 magic is 0x72, 0xB5, 0x4A, 0x86 or 0x864ab572
    if (font->magic != 0x864ab572) {
        while(1); 
    }
}

void Writer::put_char(uint32_t unicode, int x, int y, uint32_t color) {
    psf2_t* font = (psf2_t*)_binary_assets_font_psf_start;
    
    // Fallback for characters not in the font
    if (unicode >= font->numglyph) unicode = 0;

    // Calculate the start of the glyph bitmap
    uint8_t* glyph = _binary_assets_font_psf_start + 
                     font->headersize + (unicode * font->bytesperglyph);

    // Calculate how many bytes are in one horizontal row (2 bytes for 16px width)
    int bytes_per_line = (font->width + 7) / 8;

    for (uint32_t cy = 0; cy < font->height; cy++) {
        for (uint32_t cx = 0; cx < font->width; cx++) {
            // Find the specific byte and bit for this pixel
            uint8_t* line = glyph + (cy * bytes_per_line);
            uint8_t mask = 0x80 >> (cx % 8); //uses bit masking to check if theres a 1 or a zero

            if (line[cx / 8] & mask) {
                Framebuffer::put_pixel(x + cx, y + cy, color);
            }
        }
    }
}

void Writer::print(const char* text, int x, int y, uint32_t color) {
    psf2_t* font = (psf2_t*)_binary_assets_font_psf_start;
    int cur_x = x;
    int cur_y = y;

    for (int i = 0; text[i] != '\0'; i++) {
        // newline handling
        if (text[i] == '\n') {
            cur_x = x;
            cur_y += font->height;
            continue;
        }

        put_char((uint32_t)text[i], cur_x, cur_y, color);

        cur_x += font->width;

        if (cur_x + (int)font->width > (int)Framebuffer::get_width()) {
            cur_x = x;
            cur_y += font->height;
        }
    }
}