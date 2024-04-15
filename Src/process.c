#include "process.h"

volatile process_t* process_list;
process_t* current_process;

volatile process_t* terminated_process_list;

extern page_directory_t* current_page_dir;
extern uint32_t PAGE_DIR_VIRTUAL;
extern uint32_t PAGE_DIR_PHYSICAL;

//function called by assembly code in interrupts.s, responsible for finding the next process, and return it.
process_t* schedule(registers_t* regs)
{
    regs->eflags |= 0x202;
    current_process->status = READY; //now not running anymore, but ready.
    process_t* next_proc = (current_process->next != NULL) ? current_process->next : process_list;
    if (next_proc == NULL)
        asm volatile("cli;hlt");
    return next_proc;
}

//simple help function to add a process to a list of processes.
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

//TODO: now, a processes stack is allocated on the kernel heap, so i had to modify the kernel heap to be mapped as user-pages.
uint32_t create_process(void (*ent)(), uint32_t ring, int argc, ...)
{
    va_list args;
    va_start(args, argc);
    process_t* new_proc = (process_t*)kmalloc(sizeof(process_t));
    if (ring == USER_SPACE)
    {
        //if we have a user-space process, we create a new page directory for it and map the kernel in it.
        page_directory_t* allocated_page = (page_directory_t*)page_allocate();
        map_kernel_to_pd(allocated_page);
        new_proc->cr3 = (uint32_t)(virtual_to_physical(allocated_page));
        new_proc->cr3_virtual = allocated_page;
    }
    else
    {
        //kernel-mode process, use kernel_pd.
        new_proc->cr3 = (uint32_t)(&PAGE_DIR_PHYSICAL);
        new_proc->cr3_virtual = 0; //no need to use the virtual cr3 as it will never be freed.
    }
    new_proc->next = NULL;
    uint32_t stack_bottom = (uint32_t)kmalloc(0x1000); //allocate stack of 0x1000 bytes, stack top is at the "end" of the stack.
    uint32_t stack_top = stack_bottom + 0x1000;
    uint32_t* stack = (uint32_t*)stack_top; //the stack pointer should be at the end of the stack, not at the start.
    new_proc->initial_user_stack = stack_bottom;
    new_proc->ring = ring;
    new_proc->initial_kernel_stack = page_allocate();
    memset(new_proc->initial_kernel_stack, 0, 0x1000);
    new_proc->kernel_stack_top = new_proc->initial_kernel_stack + 0x1000;
    
    //cs and ss for kernel space and user space processes.
    uint32_t ss = (ring == KERNEL_SPACE) ? 0x10 : 0x23;
    uint32_t cs = (ring == KERNEL_SPACE) ? 0x08 : 0x1b;

    void** arr = (void*)kmalloc(argc * sizeof(void*));
    for (int index = 0;index<argc;index++)
    {
        void* ptr = va_arg(args, void*);
        arr[index] = ptr;
    }
    for (int index = argc-1;index>=0;index--)
    {
        PUSH(stack, arr[index]);
    }
    kfree((void*)arr);
    
    registers_t* context = (uint32_t*)((uint32_t)new_proc->kernel_stack_top - sizeof(registers_t));
    context->eip = ent;
    context->esp = new_proc->kernel_stack_top;
    context->useresp = (uint32_t)stack;
    context->ss = ss;
    context->cs = cs;
    context->ds = ss;
    context->eflags = 0x202;
    new_proc->kernel_stack_top = context;

    add_process(&process_list, new_proc);
    return 0;
}

int terminate_process(int exit_code) //terminate the current process.
{
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
    add_process(&terminated_process_list, current_process); //add the process to the terminate process list, so it will be removed by the cleanup function.
    current_process->status = TERMINATED;
    
    return 0;
}

uint32_t init_multitasking()
{
    process_list = 0;
    current_process = 0;
    terminated_process_list = 0;

    process_t* tmp_proc = (process_t*)kmalloc(sizeof(process_t));
    tmp_proc->kernel_stack_top = 0;
    tmp_proc->initial_kernel_stack = 0;
    tmp_proc->cr3 = (uint32_t)current_page_dir;
    tmp_proc->cr3_virtual = 0;
    tmp_proc->status = RUNNING; //when init_multitasking is called, its by the main process, which is currently running.
    tmp_proc->next = NULL;
    tmp_proc->initial_user_stack = 0;
    tmp_proc->ring = KERNEL_SPACE; //main process is kernel mode all the way till termination.
    add_process(&process_list, tmp_proc);
}

volatile uint32_t ticks = 0;
void wait_ticks(uint32_t amount)
{
    while (ticks <= amount);
    ticks = 0;
}

//cleanup function that is used to remove processes of the list. 
void clean_terminated_list()
{
    while(1)
    {
        if (terminated_process_list)
        {
            asm volatile("cli");
            process_t* temp = terminated_process_list;
            terminated_process_list = terminated_process_list->next;

            //all processes have theyre stack on the kernel heap, except for the main process whos stack is created in loader.s. for that main process, proc->initial_stack is set to 0.
            if (temp->initial_kernel_stack)
            {
                kfree(temp->initial_user_stack);
                page_free(temp->initial_kernel_stack);
                page_free(temp->cr3_virtual);
            }

            kfree(temp);
            asm volatile("sti");
        }
        wait_ticks(1);
    }
}