#include "process.h"

process_t* process_list;
process_t* current_process;

uint32_t main_proc = 0;

extern page_directory_t* current_page_dir;

void schedule()
{
    current_process->status = READY; //now not running anymore, but ready.
    process_t* next_proc = (current_process->next != NULL) ? current_process->next : process_list;
    if (next_proc != NULL)
    {
        SwitchToTask(next_proc);
    }
    else
    {
        kprintf("No next task\n");
        asm volatile("cli;hlt");
    }
}

void add_process(process_t* new_process)
{
    if (!new_process) return;

    new_process->status = READY;
    new_process->next = (process_t*)0;

    if (process_list == (process_t*)0) 
    {
        process_list = new_process;
        current_process = new_process;
    } 
    else {
        process_t* temp = process_list;
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
    uint32_t* stack = (uint32_t*)kmalloc(0x1000) + 0x1000; //the stack pointer should be at the end of the stack, not at the start.
    PUSH(stack, (uint32_t)ent);
    PUSH(stack, 0);
    PUSH(stack, 0);
    PUSH(stack, 0);
    PUSH(stack, 0);

    new_proc->stack_top = stack;
    add_process(new_proc);
}

void terminate_process(process_t* process)
{
    process_t* temp = process_list;
    process_t* last = 0;
    while (temp && temp != process)
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
}

uint32_t init_multitasking()
{
    process_list = 0;
    current_process = 0;

    process_t* tmp_proc = (process_t*)kmalloc(sizeof(process_t));
    main_proc = (uint32_t)tmp_proc;
    tmp_proc->stack_top = 0;
    tmp_proc->cr3 = (uint32_t)current_page_dir;
    tmp_proc->status = RUNNING;
    tmp_proc->next = NULL;
    add_process(tmp_proc);
}