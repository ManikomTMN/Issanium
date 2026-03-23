#include <stdint.h>

#pragma once

enum class ProcessState {
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
};

struct Process {
    uint64_t rsp;
    uint64_t* stack;
    uint64_t cr3;
    uint32_t pid;
    ProcessState state;
    Process* next;  
};

void create_process(void (*entry)());
extern "C" uint64_t schedule(uint64_t rsp);
void start_scheduling();