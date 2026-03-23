
# Issanium
a hobby x86_64 OS I made.

## features:
- Interrupts
- PS/2 Keyboard and mouse
- Serial
- Framebuffer with support for rendering .psf and .tga files
- PMM, VMM and heap
- a very basic process 

## things im probably gonna add soon:
- user space
- multithread
- use APIC instead of PIC
- basic GUI

## how to compile and run
you need these:

- gcc-i686-elf
- binutils
- qemu-system-x86_64
- nasm
- make

and then run ./build.sh
