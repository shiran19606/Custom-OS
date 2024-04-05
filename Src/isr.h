#ifndef ISR_H
#define ISR_H

//isr.h and isr.c files will implement a common handler function for an interrupt, that will be called when an interrupt happens and will take care of it to make sure the OS can continue running.
#include <stdint.h>
#include "Screen.h"

typedef struct registers
{
   uint32_t ds;                  // Data segment selector
   uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
   uint32_t int_no, err_code;    // Interrupt number and error code (if applicable)
   uint32_t eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
}registers_t;

typedef uint32_t (*isr_t)(registers_t*);
void register_handler(uint8_t num, isr_t handler);

#endif