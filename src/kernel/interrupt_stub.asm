[bits 64]

; Called by all stubs: handle_interrupt_cpp(uint64_t vector, uint64_t error_code)
extern handle_interrupt_cpp

; Stub for vectors WITHOUT an error code pushed by the CPU
%macro ISR_NOERR 1
global interrupt_stub_%1
interrupt_stub_%1:
    push rbp
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov rdi, %1         ; arg1 = vector number
    xor rsi, rsi        ; arg2 = error_code (none, pass 0)
    call handle_interrupt_cpp
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    pop rbp
    iretq
%endmacro

; Stub for vectors WITH an error code pushed by the CPU (8, 13, 14)
%macro ISR_ERR 1
global interrupt_stub_%1
interrupt_stub_%1:
    ; Error code is already on the stack, pop it into rsi before saving regs
    pop rsi             ; arg2 = error_code (pushed by CPU)
    push rbp
    push rax
    push rbx
    push rcx
    push rdx
    ; rsi already holds error code, push a dummy to keep stack balanced
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov rdi, %1         ; arg1 = vector number
    ; rsi still has error code from above... but we clobbered it with push rsi
    ; so reload it:
    mov rsi, [rsp + 8*7] ; offset to where we pushed rsi (error code)
    call handle_interrupt_cpp
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    pop rbp
    iretq
%endmacro

; Spurious / catch-all (no error code)
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8    ; Double Fault
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13   ; General Protection Fault
ISR_ERR   14   ; Page Fault
ISR_NOERR 15
ISR_NOERR 32   ; Timer IRQ0
ISR_NOERR 33   ; Keyboard IRQ1
ISR_NOERR 44   ; Mouse IRQ12

extern schedule

global interrupt_stub_timer
interrupt_stub_timer:
    push rbp
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp        ; pass current rsp to schedule
    call schedule       ; returns new rsp in rax
    mov rsp, rax        ; switch to new process's stack

    ; send EOI to PIC before returning
    mov al, 0x20
    out 0x20, al

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    pop rbp
    iretq