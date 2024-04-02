;
; Gdt.s -- contains global descriptor table and interrupt descriptor table
;          setup code.
;          Based on code from Bran's kernel development tutorials.
;          Rewritten for JamesM's kernel development tutorials.

[GLOBAL gdt_flush]    ; Allows the C code to call gdt_flush().

gdt_flush:
    mov eax, [esp+4]  ; Get the pointer to the GDT, passed as a parameter.
    lgdt [eax]        ; Load the new GDT pointer

    mov ax, 0x10      ; 0x10 is the offset in the GDT to our data segment
    mov ds, ax        ; Load all data segment selectors
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush   ; 0x08 is the offset to our code segment: Far jump!
.flush:
    ret


[GLOBAL idt_flush]    ; Allows the C code to call idt_flush().

idt_flush:
   mov eax, [esp+4]  ; Get the pointer to the IDT, passed as a parameter.
   lidt [eax]        ; Load the IDT pointer.
   ret

[GLOBAL flush_tss]

flush_tss:
    cli
    xor eax, eax
    mov ax, (5* 8) | 0
    ltr ax
    ret

[GLOBAL enter_usermode]
[EXTERN printNumberHex]
[EXTERN put_char]
enter_usermode:
    cli
    mov ax, (4 * 8) | 3 ;4th element in gdt, with ring 3   
    mov ds, ax       
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    mov eax, esp
    push (4 * 8) | 3 ;4th element in gdt, with ring 3   
    push eax

    pushfd
    pop eax
    or eax, 0x200
    push eax

    push (3 * 8) | 3 ;3rd element in gdt, with ring 3   
    lea eax, [label_here]
    push eax
    iretd
label_here:
    ret