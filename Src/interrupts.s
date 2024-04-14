[EXTERN isr_handler] ;our C handler function we will call on each interrupt.
[EXTERN irq_handler] ;our c irq_handler function
[GLOBAL Timer_Handler]
[GLOBAL SwitchToTask]
[EXTERN current_process]
[EXTERN schedule]
[EXTERN ticks]
[EXTERN printNumberHex]
[EXTERN put_char]


;we will write a macro handler for a case where no error code is pushed, and a macro handler for a case where an error code is pushed.
;to see on what cases the error code is pushed, we used the intel manual on interrupts: https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-3a-part-1-manual.pdf.
;page 188 on the manual shows a table of interrupts and if they push an error code or not.
%macro ISR_NOERRCODE 1
    global isr%1
    isr%1:
        cli
        push dword 0 ;if the ISR doesnt push an error code, we push a dummy error code before calling the isr_handler function
        push dword %1 ; push the interrupt number
        jmp isr_common_case
%endmacro


%macro ISR_ERRCODE 1
    global isr%1
    isr%1:
        cli
        ;no need to push an error code because on this kind of interrupts the error code is pushed before calling the macro handler.
        push dword %1 ; push the interrupt number
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
    pushad                   ; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

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

    popad                    ; Pops edi,esi,ebp...
    add esp, 8     ; Cleans up the pushed error code and pushed ISR number
    sti            ; re-enable interrupts after we disabled them in the macros
    iret           ; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP


%macro IRQ 2
    global irq%1
    irq%1:
        cli
        push dword 0 
        push dword %2
        jmp irq_common_case
%endmacro

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
    pushad
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
    pop esp

    ; 3. Restore state
    pop ebx
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx
    
    popad
    add esp, 8
    
    sti
    iret

;;
;; -- Some offsets into the PCB sructure
;;    ----------------------------------
TOS     equ         0
VAS     equ         4
STATE   equ         12
RING    equ         20

;;
;; -- These are the different states
;;    ------------------------------
RUNNING equ         0
READY   equ         1
PAUSED  equ         2
SLEEPING    equ     3
TERMINATED  equ     4

;;
;; -- Kernel or User process
;;    ------------------------------
KERNEL_SPACE    equ 0
USER_SPACE      equ 3

Timer_Handler:
    cli
    push        eax
    push        ebx
    push        ecx
    push        edx
    push        ebp
    push        esi
    push        edi
    pushfd
    
    inc dword [ticks]
    mov         eax,0x20
    out         0x20,al

    push eax
    push esp
    call schedule
    pop esp
    mov ebp, eax
    pop eax
    
    cli
    mov         edi,[current_process]        ;; `edi` = previous tasks PCB

    mov         [edi+TOS],esp           ;; save the top of the stack
    mov         eax, cr3
    mov         [edi+VAS], eax

    ;; -- load the next task's state

    mov esi, ebp
    mov [current_process], esi

    mov         esp,[esi+TOS]           ;; load the next process's stack
    mov         eax,[esi+VAS]
    mov         cr3,eax
    mov         dword [esi+STATE],RUNNING     ;; make the current task running

    xor eax, eax
    mov ebx, dword [esi+RING]
    cmp ebx, KERNEL_SPACE
    je is_kernel_mode

    mov eax, 0x23
    jmp finish
is_kernel_mode:
    mov eax, 0x10
finish:
    mov ds, ax       
    mov es, ax
    mov fs, ax
    mov gs, ax

    pop         eax
    or   eax, 0x200
    push        eax

    popfd
    pop         edi
    pop         esi
    pop         ebp
    pop         edx
    pop         ecx
    pop         ebx
    pop         eax
retting:
    iret

[GLOBAL get_esp]

get_esp:
    mov eax, esp
    ret

[GLOBAL syscall_handler]

syscall_handler:
    cli
    push dword 0x60
    push dword 0x80
    jmp irq_common_case

[GLOBAL syscall_run]

syscall_run:
    mov eax, [esp+4]
    int 0x80
    ret
