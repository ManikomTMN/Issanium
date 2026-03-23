#pragma once
#include <stdint.h>

// ISR stubs that i defined in interrupt_stub.asm
extern "C" void interrupt_stub_0();
extern "C" void interrupt_stub_1();
extern "C" void interrupt_stub_2();
extern "C" void interrupt_stub_3();
extern "C" void interrupt_stub_4();
extern "C" void interrupt_stub_5();
extern "C" void interrupt_stub_6();
extern "C" void interrupt_stub_7();
extern "C" void interrupt_stub_8();
extern "C" void interrupt_stub_9();
extern "C" void interrupt_stub_10();
extern "C" void interrupt_stub_11();
extern "C" void interrupt_stub_12();
extern "C" void interrupt_stub_13();
extern "C" void interrupt_stub_14();
extern "C" void interrupt_stub_15();
extern "C" void interrupt_stub_32();
extern "C" void interrupt_stub_timer();
extern "C" void interrupt_stub_33();
extern "C" void interrupt_stub_44();

// The cpp dispatcher called by every asm stub
extern "C" void handle_interrupt_cpp(uint64_t vector, uint64_t error_code);

namespace Interrupts {
    void init();

    extern volatile uint32_t timer_ticks;
}