#include "limine.h"
#include "lib.h"
#include "graphics/framebuffer.h"
#include "graphics/writer.h"
#include "drivers/serial.h"
#include "arch/x86_64/cpu.h"
#include "arch/x86_64/interrupts.h"
#include "drivers/ps2keyboard.h"
#include "drivers/ps2mouse.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "memory/heap.h"
#include "kernel/process.h"

// boilerplate i boil some plates
__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(5);

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

extern "C" uint8_t _binary_assets_image_tga_start[];

extern "C" uint8_t _binary_assets_cursor_tga_start[];

extern "C" uint8_t _binary_assets_bg_tga_start[];

static int32_t prev_mouse_x = -1;
static int32_t prev_mouse_y = -1;

static void render_mouse() {
    serial_print("render_mouse called\n");
    if (!PS2Mouse::moved) return;
    PS2Mouse::moved = false;

    uint32_t color = PS2Mouse::left_pressed ? 0x00FF00 : 0xFF0000;
    Framebuffer::draw_rect(PS2Mouse::mouse_x, PS2Mouse::mouse_y, 5, 5, color);
    Framebuffer::draw_tga_image(PS2Mouse::mouse_x, PS2Mouse::mouse_y, _binary_assets_cursor_tga_start);

    prev_mouse_x = PS2Mouse::mouse_x;
    prev_mouse_y = PS2Mouse::mouse_y;

    char xbuf[16], ybuf[16];
    itoa(PS2Mouse::mouse_x, xbuf);
    itoa(PS2Mouse::mouse_y, ybuf);
    serial_print("Mouse x="); serial_print(xbuf);
    serial_print(" y=");      serial_print(ybuf);
    serial_print("\n");
}

static void render_ticks() {
    static uint32_t last_ticks = 0;
    if (Interrupts::timer_ticks == last_ticks) return;
    last_ticks = Interrupts::timer_ticks;

    char buf[16];
    itoa(Interrupts::timer_ticks, buf);
    Writer::print("Ticks: ", 200, 120, 0xFFFFFF);
    Writer::print(buf,       300, 120, 0xFFFFFF);
}

static void render_keypress() {
    if (!PS2Keyboard::pressed) return;
    PS2Keyboard::pressed = false;

    char buf[2] = { PS2Keyboard::last_key, '\0' };
    Writer::print("Key: ", 200, 140, 0xFFFFFF);
    Writer::print(buf,     300, 140, 0xFFFFFF);
}

// kernel entry point!

void process_a() {
    while(true) {
        serial_print("A\n");
        for(volatile int i = 0; i < 100000000; i++);
    }
}

void process_b() {
    while(true) {
        serial_print("B\n");
        for(volatile int i = 0; i < 100000000; i++);
    }
}


extern "C" void kmain() {
    serial_init();
    serial_print("OS started\n");
    PMM::init(hhdm_request.response->offset);
    VMM::init(hhdm_request.response->offset);
    Heap::init();
    
    if (!LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision)) hlt();

    Interrupts::init();
    serial_print("Interrupts initialized\n");

    serial_print("about to call PS2Mouse::init\n");
    PS2Mouse::init();
    serial_print("Mouse initialized\n");

    asm volatile ("sti");
    serial_print("Interrupts enabled\n");

    if (!Framebuffer::init()) hlt();
    serial_print("Framebuffer initialized\n"); 

    Writer::init();
    serial_print("Writer initialized\n");

    // Seed previous mouse position so first erase doesn't black out (0,0)
    prev_mouse_x = PS2Mouse::mouse_x;
    prev_mouse_y = PS2Mouse::mouse_y;

    create_process(process_a);
    create_process(process_b);

    create_process([]() {
        while(true) {
            Framebuffer::clear(0x000000);
            Framebuffer::draw_tga_scaled(0, 0, Framebuffer::get_width(), Framebuffer::get_height(), _binary_assets_bg_tga_start);
            Framebuffer::draw_rect(0, 0, Framebuffer::get_width(), 48, 0x444444);
            Writer::print("Issanium Hydrogen", 0, 0, 0xFFFFFF);
            
            render_mouse();
            render_ticks();
            render_keypress();

            Framebuffer::update_frontbuffer();
        }
    });

    start_scheduling();
}