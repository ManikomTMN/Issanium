#include "kernel/process.h"
#include "memory/heap.h"
#include "drivers/serial.h"
#include "arch/x86_64/interrupts.h"
#include <stdint.h>
#include "lib.h"

static Process* current = nullptr;
static uint32_t next_pid = 0;

static constexpr size_t STACK_SIZE = 4096 * 4; // 16KB stack per process (i should probably increase this later)

static uint16_t get_cs() {
    uint16_t cs;
    asm volatile("mov %%cs, %0" : "=r"(cs));
    return cs;
}

void create_process(void (*entry)()) {
    // Allocate process struct
    Process* proc = (Process*)Heap::kmalloc(sizeof(Process));

    // Allocate stack
    uint64_t* stack = (uint64_t*)Heap::kmalloc(STACK_SIZE);
    uint64_t* stack_top = stack + (STACK_SIZE / 8); // points to top

    // fake interrupt frame from top down
    *--stack_top = 0;             // ss
    *--stack_top = (uint64_t)(stack + STACK_SIZE/8); // rsp
    *--stack_top = 0x202;         // rflags
    *--stack_top = get_cs();      // cs
    *--stack_top = (uint64_t)entry; // rip iretq reads upto here
    *--stack_top = 0;             // r15
    *--stack_top = 0;             // r14
    *--stack_top = 0;             // r13
    *--stack_top = 0;             // r12
    *--stack_top = 0;             // r11
    *--stack_top = 0;             // r10
    *--stack_top = 0;             // r9
    *--stack_top = 0;             // r8
    *--stack_top = 0;             // rdi
    *--stack_top = 0;             // rsi
    *--stack_top = 0;             // rdx
    *--stack_top = 0;             // rcx
    *--stack_top = 0;             // rbx
    *--stack_top = 0;             // rax
    *--stack_top = 0;             // rbp the last one

    // Fill in process struct
    proc->rsp   = (uint64_t)stack_top;
    proc->stack = stack;
    proc->cr3   = 0; // using current page tables (for now)
    proc->pid   = next_pid++;
    proc->state = ProcessState::READY;
    proc->next  = nullptr;

    // Add this sh*t to a circular linked list
    if (current == nullptr) {
        current = proc;
        proc->next = proc;
    } else {
        // insert after current
        proc->next = current->next;
        current->next = proc;
    }

    serial_print("Process created! pid=");
    serial_print_hex(proc->pid);
    serial_print("\n");
}

uint64_t schedule(uint64_t rsp) {
    Interrupts::timer_ticks++;
    if (current == nullptr) {
        serial_print("schedule: no processes!\n");
        return rsp;
    }

    serial_print("schedule: switching from pid=");
    char buf[16];
    itoa(current->pid, buf);
    serial_print(buf);
    
    current->rsp = rsp;
    current->state = ProcessState::READY;
    current = current->next;
    current->state = ProcessState::RUNNING;
    
    serial_print(" to pid=");
    itoa(current->pid, buf);
    serial_print(buf);
    serial_print("\n");

    return current->rsp;
}

void start_scheduling() {
    // just switch to the first process manually
    current->state = ProcessState::RUNNING;
    
    // load its stack and jump into it via iretq
    asm volatile(
        "mov %0, %%rsp\n"
        "pop %%r15\n"
        "pop %%r14\n"
        "pop %%r13\n"
        "pop %%r12\n"
        "pop %%r11\n"
        "pop %%r10\n"
        "pop %%r9\n"
        "pop %%r8\n"
        "pop %%rdi\n"
        "pop %%rsi\n"
        "pop %%rdx\n"
        "pop %%rcx\n"
        "pop %%rbx\n"
        "pop %%rax\n"
        "pop %%rbp\n"
        "iretq\n"
        : : "r"(current->rsp)
    );
}