#ifndef PROCESS_H
#define PROCESS_H

#include "utils.h"
#include "heap.h"
#include "vmm.h"

#define RUNNING         0
#define READY           1
#define PAUSED          2
#define SLEEPING        3
#define TERMINATED      4

#define KERNEL_SPACE    0
#define USER_SPACE      3

#define PUSH(tos,val) (tos--); ((*(uint32_t*)tos) = (uint32_t)val)

typedef struct process
{
    uint32_t stack_top;
    uint32_t cr3;
    struct process* next;
    uint32_t status;
    uint32_t initial_stack;
    uint32_t ring;
} process_t;


void add_process(process_t** list, process_t* new_process);
uint32_t create_process(void (*ent)(), uint32_t ring, int argc, char* argv[]);
void terminate_process();
uint32_t init_multitasking();
void clean_terminated_list();

extern void SwitchToTask(process_t* process);
extern uint32_t get_esp();

#endif