#include "process.h"

process_t* process_list;
process_t* current_process;

process_t* terminated_process_list;

extern page_directory_t* current_page_dir;

void schedule()
{
    current_process->status = READY; //now not running anymore, but ready.
    process_t* next_proc = (current_process->next != NULL) ? current_process->next : process_list;
    if (next_proc != NULL)
        SwitchToTask(next_proc);
    else
    {
        kprintf("No next task\n");
        asm volatile("cli;hlt");
    }
}

void add_process(process_t** list, process_t* new_process)
{
    if (!new_process) return;

    new_process->status = READY;
    new_process->next = (process_t*)0;

    if (*list == (process_t*)0) 
    {
        *list = new_process;
        current_process = new_process;
    } 
    else {
        process_t* temp = *list;
        while (temp->next != NULL && temp->next != process_list) 
            temp = temp->next;
        temp->next = new_process;
    }
}

void create_process(void (*ent)())
{
    process_t* new_proc = (process_t*)kmalloc(sizeof(process_t));
    new_proc->cr3 = (uint32_t)current_page_dir;
    new_proc->next = NULL;
    uint32_t stack_bottom = (uint32_t)kmalloc(0x1000);
    uint32_t stack_top = stack_bottom + 0x1000;
    uint32_t* stack = (uint32_t*)stack_top; //the stack pointer should be at the end of the stack, not at the start.
    new_proc->initial_stack = stack_top;
    PUSH(stack, (uint32_t)ent);     //eip
    PUSH(stack, 0);                 //eax
    PUSH(stack, 0);                 //ebx
    PUSH(stack, 0);                 //ecx
    PUSH(stack, 0);                 //edx
    PUSH(stack, 0);                 //esi
    PUSH(stack, 0);                 //edi
    PUSH(stack, 0);                 //ebp
    PUSH(stack, 0x202);             //eflags

    new_proc->stack_top = stack;
    add_process(&process_list, new_proc);
}

void terminate_process() //terminate the current process.
{
    asm volatile("cli");
    process_t* temp = process_list;
    process_t* last = 0;
    while (temp && temp != current_process)
    {
        last = temp;
        temp = temp->next;
    }
    if (temp == NULL)
        return;
    if (last == 0) //means that temp was the first in the list.
        process_list = temp->next;
    else
        last->next = temp->next;
    add_process(&terminated_process_list, current_process);
    current_process->status = TERMINATED;
    asm volatile("sti");
    while(1);
}

uint32_t init_multitasking()
{
    process_list = 0;
    current_process = 0;

    process_t* tmp_proc = (process_t*)kmalloc(sizeof(process_t));
    tmp_proc->stack_top = 0;
    tmp_proc->initial_stack = 0;
    tmp_proc->cr3 = (uint32_t)current_page_dir;
    tmp_proc->status = RUNNING;
    tmp_proc->next = NULL;
    add_process(&process_list, tmp_proc);
}

uint32_t ticks = 0;
void wait_ticks(uint32_t amount)
{
    while (ticks <= amount) {}
    ticks = 0;
}


void clean_terminated_list()
{
    while(1)
    {
        asm volatile("cli");
        while (terminated_process_list)
        {
            process_t* temp = terminated_process_list;
            terminated_process_list = terminated_process_list->next;
            if (temp->initial_stack)
                kfree((void*)(temp->initial_stack));
            kfree((void*)temp);
        }
        asm volatile("sti");
        wait_ticks(1);
    }
}