#include "arch/x86_64/interrupts.h"
#include "arch/x86_64/cpu.h"
#include "drivers/ps2keyboard.h"
#include "drivers/ps2mouse.h"
#include "drivers/serial.h"

struct idt_entry {
    uint16_t isr_low;
    uint16_t kernel_cs;
    uint8_t  ist;
    uint8_t  attributes;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
} __attribute__((packed));

struct idtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

__attribute__((aligned(0x10))) static idt_entry idt[256];
__attribute__((aligned(0x10))) static idtr      idtr_reg;

volatile uint32_t Interrupts::timer_ticks = 0;

static uint16_t get_cs() {
    uint16_t cs;
    asm volatile ("mov %%cs, %0" : "=r"(cs));
    return cs;
}

static void io_wait() { outb(0x80, 0); }

static void print_hex(uint64_t value) {
    char buf[19];
    buf[0] = '0'; buf[1] = 'x'; buf[18] = '\0';
    const char* hex = "0123456789ABCDEF";
    for (int i = 0; i < 16; i++) {
        buf[17 - i] = hex[value & 0xF];
        value >>= 4;
    }
    serial_print(buf);
}

static void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    uint64_t addr          = (uint64_t)isr;
    idt[vector].isr_low    = addr & 0xFFFF;
    idt[vector].kernel_cs  = get_cs();
    idt[vector].ist        = 0;
    idt[vector].attributes = flags;
    idt[vector].isr_mid    = (addr >> 16) & 0xFFFF;
    idt[vector].isr_high   = (addr >> 32) & 0xFFFFFFFF;
    idt[vector].reserved   = 0;
}

// ---------------------------------------------------------------
// Dispatcher — just routes, no logic lives here
// ---------------------------------------------------------------
extern "C" void handle_interrupt_cpp(uint64_t vector, uint64_t error_code) {
    switch (vector) {

        // --- CPU exceptions ---
        case 8:
            serial_print("\n!!! DOUBLE FAULT !!! code=");
            print_hex(error_code);
            serial_print("\n");
            while (true) asm volatile("hlt");

        case 13:
            serial_print("\n!!! GENERAL PROTECTION FAULT !!! code=");
            print_hex(error_code);
            serial_print("\n");
            while (true) asm volatile("hlt");

        case 14: {
            uint64_t cr2;
            asm volatile("mov %%cr2, %0" : "=r"(cr2));
            serial_print("\n!!! PAGE FAULT !!! cr2=");
            print_hex(cr2);
            serial_print(" code=");
            print_hex(error_code);
            serial_print("\n");
            while (true) asm volatile("hlt");
        }

        // --- Hardware IRQs ---
        case 32: // Timer IRQ0
            Interrupts::timer_ticks++;
            outb(0x20, 0x20);
            break;

        case 33: // Keyboard IRQ1
            PS2Keyboard::handle(inb(0x60));
            outb(0x20, 0x20);
            break;

        case 44: // Mouse IRQ12
            PS2Mouse::handle(inb(0x60));
            outb(0xA0, 0x20); // slave EOI
            outb(0x20, 0x20); // master EOI
            break;

        default:
            outb(0x20, 0x20);
            break;
    }
}

void Interrupts::init() {
    // Disable APIC so we can use the legacy PIC instead
    uint32_t lo, hi;
    asm volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0x1B));
    lo &= ~(1u << 11);
    asm volatile ("wrmsr" : : "a"(lo), "d"(hi), "c"(0x1B));
    serial_print("APIC disabled\n");

    // Remap PIC: IRQ0-7 -> INT 32-39, IRQ8-15 -> INT 40-47
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();
    outb(0xA1, 0x28); io_wait();
    outb(0x21, 0x04); io_wait();
    outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();

    // Mask all IRQs to start clean
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);

    // PIT channel 0 at ~100 Hz
    uint16_t divisor = 1193180 / 100;
    outb(0x43, 0x36);
    outb(0x40,  divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);

    // Build IDT — default everything to the timer stub (safe no-op)
    idtr_reg.base  = (uint64_t)&idt[0];
    idtr_reg.limit = (uint16_t)(sizeof(idt_entry) * 256 - 1);

    for (int i = 0; i < 256; i++)
        idt_set_descriptor(i, (void*)interrupt_stub_32, 0x8E);

    // Exceptions
    idt_set_descriptor(0,  (void*)interrupt_stub_0,  0x8E);
    idt_set_descriptor(1,  (void*)interrupt_stub_1,  0x8E);
    idt_set_descriptor(2,  (void*)interrupt_stub_2,  0x8E);
    idt_set_descriptor(3,  (void*)interrupt_stub_3,  0x8E);
    idt_set_descriptor(4,  (void*)interrupt_stub_4,  0x8E);
    idt_set_descriptor(5,  (void*)interrupt_stub_5,  0x8E);
    idt_set_descriptor(6,  (void*)interrupt_stub_6,  0x8E);
    idt_set_descriptor(7,  (void*)interrupt_stub_7,  0x8E);
    idt_set_descriptor(8,  (void*)interrupt_stub_8,  0x8E);
    idt_set_descriptor(9,  (void*)interrupt_stub_9,  0x8E);
    idt_set_descriptor(10, (void*)interrupt_stub_10, 0x8E);
    idt_set_descriptor(11, (void*)interrupt_stub_11, 0x8E);
    idt_set_descriptor(12, (void*)interrupt_stub_12, 0x8E);
    idt_set_descriptor(13, (void*)interrupt_stub_13, 0x8E);
    idt_set_descriptor(14, (void*)interrupt_stub_14, 0x8E);
    idt_set_descriptor(15, (void*)interrupt_stub_15, 0x8E);

    // Hardware IRQs
    idt_set_descriptor(32, (void*)interrupt_stub_timer, 0x8E);
    idt_set_descriptor(33, (void*)interrupt_stub_33, 0x8E);
    idt_set_descriptor(44, (void*)interrupt_stub_44, 0x8E);

    asm volatile ("lidt %0" : : "m"(idtr_reg));

    // Unmask timer + keyboard on master, mouse irq12 on slave
    outb(0x21, 0xF8); // Unmasks IRQ0 timer, IRQ1 keyboard, and IRQ2 (Cascade)
    outb(0xA1, 0xEF); // Unmask IRQ12
}