#include "drivers/ps2mouse.h"
#include "arch/x86_64/cpu.h"
#include "drivers/serial.h"

namespace PS2Mouse {

volatile int32_t mouse_x      = 0;
volatile int32_t mouse_y      = 0;
volatile bool    left_pressed  = false;
volatile bool    right_pressed = false;
volatile bool    moved         = false;

// statemachine
static uint8_t cycle = 0;
static uint8_t packet[3];

// Screen clamp limits im gonna set this to the screen resolution soon
static constexpr int32_t SCREEN_W = 1280;
static constexpr int32_t SCREEN_H = 720;

// helpers
static void wait_write() {
    uint32_t timeout = 100000;
    while (timeout-- && (inb(0x64) & 2));
}

static void wait_read() {
    uint32_t timeout = 100000;
    while (timeout-- && !(inb(0x64) & 1));
}

static void write_cmd(uint8_t cmd) {
    wait_write();
    outb(0x64, cmd);
}

static void write_data(uint8_t data) {
    wait_write();
    outb(0x60, data);
}

static uint8_t read_data() {
    wait_read();
    return inb(0x60);
}

static void mouse_cmd(uint8_t cmd) {
    write_cmd(0xD4); // route next byte to mouse
    write_data(cmd);
    read_data();     // ACK
}

void init() {
    serial_print("Mouse init start\n");

    write_cmd(0xA8);
    serial_print("Auxiliary device enabled\n");

    write_cmd(0x20);
    uint8_t cfg = read_data();
    serial_print("Config byte read\n");
    cfg |= 0x02;
    cfg &= ~0x20;  // clear "mouse disabled" bit
    write_cmd(0x60);
    write_data(cfg);
    serial_print("IRQ12 enabled in config\n");

    mouse_cmd(0xF6);
    serial_print("Set defaults ACK'd\n");

    mouse_cmd(0xF4);
    serial_print("Data reporting enabled ACK'd\n");

    serial_print("Mouse init done\n");
}

void handle(uint8_t data) {
    switch (cycle) {
        case 0:
            // Byte 1 must have bit 3 set otherwise itll be out of sinc so discard it
            if (!(data & 0x08)) return;
            packet[0] = data;
            cycle++;
            break;

        case 1:
            packet[1] = data;
            cycle++;
            break;

        case 2:
            packet[2] = data;
            cycle = 0;

            // Sign-extend X delta (bit 4 of flags byte = X sign)
            int32_t dx = packet[1];
            if (packet[0] & 0x10) dx |= ~0xFF;

            // Sign-extend Y delta (bit 5 of flags byte = Y sign)
            // Mouse Y is inverted relative to screen Y
            int32_t dy = packet[2];
            if (packet[0] & 0x20) dy |= ~0xFF;

            mouse_x += dx;
            mouse_y -= dy;

            // Clamp to screen bounds
            if (mouse_x < 0)        mouse_x = 0;
            if (mouse_y < 0)        mouse_y = 0;
            if (mouse_x >= SCREEN_W) mouse_x = SCREEN_W - 1;
            if (mouse_y >= SCREEN_H) mouse_y = SCREEN_H - 1;

            left_pressed  = packet[0] & 0x01;
            right_pressed = packet[0] & 0x02;
            moved         = true;
            break;
    }
}

} // namespace PS2Mouse