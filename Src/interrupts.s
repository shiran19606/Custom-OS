[EXTERN isr_handler] ;our C handler function we will call on each interrupt.
[EXTERN irq_handler] ;our c irq_handler function

;we will write a macro handler for a case where no error code is pushed, and a macro handler for a case where an error code is pushed.
;to see on what cases the error code is pushed, we used the intel manual on interrupts: https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-3a-part-1-manual.pdf.
;page 188 on the manual shows a table of interrupts and if they push an error code or not.
%macro ISR_NOERRCODE 1
    global isr%1
    isr%1:
        cli
        push 0 ;if the ISR doesnt push an error code, we push a dummy error code before calling the isr_handler function
        push %1 ; push the interrupt number
        jmp isr_common_case
%endmacro


%macro ISR_ERRCODE 1
    global isr%1
    isr%1:
        cli
        ;no need to push an error code because on this kind of interrupts the error code is pushed before calling the macro handler.
        push %1 ; push the interrupt number
        jmp isr_common_case
%endmacro

;declare the handler function using the macros declared above. handlers 8 and 10-14 should have an error code pushed before the macro is called, so we use the ERRCODE macro
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

isr_common_case: ;we jump to this common case no matter if we had an error code pushed or a dummy error code, because the common case has nothing to do with the error code but only with calling the isr_handler function.
    pusha                    ; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

    mov ax, ds               ; Lower 16-bits of eax = ds.
    push eax                 ; save the data segment descriptor

    mov ax, 0x10  ; load the kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call isr_handler
    add esp, 4

    pop eax        ; reload the original data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                     ; Pops edi,esi,ebp...
    add esp, 8     ; Cleans up the pushed error code and pushed ISR number
    sti            ; re-enable interrupts after we disabled them in the macros
    iret           ; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP


%macro IRQ 2
    global irq%1
    irq%1:
        cli
        push byte 0 
        push byte %2
        jmp irq_common_case
%endmacro

IRQ 0,  32
IRQ 1,  33
IRQ 2,  34
IRQ 3,  35
IRQ 4,  36
IRQ 5,  37
IRQ 6,  38
IRQ 7,  39
IRQ 8,  40
IRQ 9,  41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

irq_common_case:
    ; 1. Save CPU state
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 2. Call C handler
    push esp
    call irq_handler
    pop ebx

    ; 3. Restore state
    pop ebx
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx
    
    popa
    add esp, 8
    sti
    iret

