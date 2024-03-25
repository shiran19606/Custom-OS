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
    uint32_t initial_stack;
} process_t;


void add_process(process_t** list, process_t* new_process);
void create_process(void (*ent)());
void terminate_process();
uint32_t init_multitasking();
void clean_terminated_list();

extern void SwitchToTask(process_t* process);

#endif