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
    uint32_t stack_top;
    uint32_t cr3;
    volatile struct process* next;
    uint32_t status;
    uint32_t initial_stack;
    uint32_t ring;
} process_t;

typedef struct registers1
{
   uint32_t eax1;                  // Data segment selector
   uint32_t eflags1, edi, esi, ebp, edx, ecx, ebx, eax; // Pushed by pusha.
   uint32_t eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
}registers1_t;


void add_process(process_t** list, process_t* new_process);
uint32_t create_process(void (*ent)(), uint32_t ring, int argc, ...);
void terminate_process(uint32_t exit_code);
uint32_t init_multitasking();
void clean_terminated_list();

extern uint32_t get_esp();

#endif