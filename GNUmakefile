
#help me makefile totally sucks and i dont wanna write it
#this is totally my original that i didnt copy from somewhere else and modify a bit
#why did i choose to use c++ and not c? maybe im just barmy or something idk

#toolchain 'n stuff
OUTPUT := myos
TOOLCHAIN_PREFIX ?=

CXX := $(TOOLCHAIN_PREFIX)g++
CC  := $(TOOLCHAIN_PREFIX)gcc
AS  := $(TOOLCHAIN_PREFIX)nasm
LD  := $(TOOLCHAIN_PREFIX)ld
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy

#flags
CXXFLAGS := -g -O2 -ffreestanding -fno-exceptions -fno-rtti \
            -Wall -Wextra -std=gnu++20 -m64 -mcmodel=kernel \
            -ffunction-sections -fdata-sections -mno-red-zone -fno-pic -fno-pie -I src/include -mgeneral-regs-only

NASMFLAGS := -f elf64

LDFLAGS := -T linker.lds -nostdlib -static -z max-page-size=0x1000 --gc-sections -m elf_x86_64

# source files
SRCFILES := $(shell find src -type f \( -name "*.cpp" -o -name "*.c" -o -name "*.S" -o -name "*.asm" \))
OBJFILES := $(patsubst src/%,obj/%,$(SRCFILES))
OBJFILES := $(OBJFILES:.cpp=.o)
OBJFILES := $(OBJFILES:.c=.o)
OBJFILES := $(OBJFILES:.S=.o)
OBJFILES := $(OBJFILES:.asm=.o)

# Automatically find all TGA and PSF files
TGA_FILES := $(wildcard assets/*.tga)
PSF_FILES := $(wildcard assets/*.psf)

TGA_OBJS := $(patsubst assets/%.tga,obj/%.o,$(TGA_FILES))
PSF_OBJS := $(patsubst assets/%.psf,obj/%.o,$(PSF_FILES))

OBJFILES += $(TGA_OBJS) $(PSF_OBJS)


DEPFILES := $(OBJFILES:.o=.d)

#default target
all: bin/$(OUTPUT)

bin/$(OUTPUT): $(OBJFILES) linker.lds
	mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(OBJFILES) -o $@

# rules

# TGA files
obj/%.o: assets/%.tga
	mkdir -p $(dir $@)
	$(OBJCOPY) -I binary -O elf64-x86-64 -B i386 $< $@

# PSF files
obj/%.o: assets/%.psf
	mkdir -p $(dir $@)
	$(OBJCOPY) -I binary -O elf64-x86-64 -B i386 $< $@

obj/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

obj/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CXXFLAGS) -MMD -MP -c $< -o $@

obj/%.o: src/%.S
	mkdir -p $(dir $@)
	$(CC) $(CXXFLAGS) -c $< -o $@

obj/%.o: src/%.asm
	mkdir -p $(dir $@)
	$(AS) $(NASMFLAGS) $< -o $@


-include $(DEPFILES)


.PHONY: clean
clean:
	rm -rf obj bin