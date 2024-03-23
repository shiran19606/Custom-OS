#ifndef PROCESS_H
#define PROCESS_H

#include "utils.h"
#include "heap.h"
#include "vmm.h"

#define RUNNING 0
#define READY 1
#define PAUSED 2
#define SLEEPING 3
#define TERMINATED 4

#define PUSH(tos,val) (tos--); ((*(uint32_t*)tos) = (uint32_t)val)

typedef struct process
{
    uint32_t stack_top;
    uint32_t cr3;
    struct process* next;
    uint32_t status;
} process_t;


void add_process(process_t* new_process);
void create_process(void (*ent)());
void terminate_process(process_t* process);
uint32_t init_multitasking();

extern void SwitchToTask(process_t* process);

#endif