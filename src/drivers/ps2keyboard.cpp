#include "drivers/ps2keyboard.h"

namespace PS2Keyboard {

volatile bool    pressed      = false;
volatile char    last_key     = '\0';
volatile uint8_t last_scancode = 0;

// US QWERTY scancodes
static const char scancode_map[0x60] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t','q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'','`',
    0,   '\\','z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

void handle(uint8_t scancode) {
    last_scancode = scancode;

    // High bit set = key release
    if (scancode & 0x80) return;
    if (scancode >= 0x60) return;

    char key = scancode_map[scancode];
    if (key) {
        last_key = key;
        pressed  = true;
    }
}

} // namespace PS2Keyboard