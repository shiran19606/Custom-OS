#ifndef PROCESS_H
#define PROCESS_H

#include "utils.h"
#include "heap.h"
#include "vmm.h"
#include "page_frame_allocator.h"
#include <stdarg.h>

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
    uint32_t kernel_stack_top;
    uint32_t cr3;
    volatile struct process* next;
    uint32_t status;
    uint32_t initial_user_stack;
    uint32_t ring;
    uint32_t initial_kernel_stack;
    uint32_t cr3_virtual;
} process_t;



void add_process(process_t** list, process_t* new_process);
uint32_t create_process(void (*ent)(), uint32_t ring, int argc, ...);
int terminate_process(int exit_code);
uint32_t init_multitasking();
void clean_terminated_list();

extern uint32_t get_esp();
void wait_ticks(uint32_t amount);

#endif