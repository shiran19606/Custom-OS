;
; boot.s -- Kernel start location. Also defines multiboot header.
;           Based on Bran's kernel development tutorial file start.asm
;

MBOOT_PAGE_ALIGN    equ 1<<0    ; Load kernel and modules on a page boundary
MBOOT_MEM_INFO      equ 1<<1    ; Provide your kernel with memory info
MBOOT_HEADER_MAGIC  equ 0x1BADB002 ; Multiboot Magic value
; NOTE: We do not use MBOOT_AOUT_KLUDGE. It means that GRUB does not
; pass us a symbol table.
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

GLOBAL KERNEL_VIRTUAL
KERNEL_VIRTUAL equ 0xC0000000


[BITS 32]                       ; All instructions should be 32-bit.

[GLOBAL mboot]                  ; Make 'mboot' accessible from C.
[EXTERN code]                   ; Start of the '.text' section.
[EXTERN bss]                    ; Start of the .bss section.
[EXTERN kernel_physical_end]    ; End of the last loadable section.

mboot:
    dd  MBOOT_HEADER_MAGIC      ; GRUB will search for this value on each
                                ; 4-byte boundary in your kernel file
    dd  MBOOT_HEADER_FLAGS      ; How GRUB should load your file / settings
    dd  MBOOT_CHECKSUM          ; To ensure that the above values are correct
    
    dd  mboot                   ; Location of this descriptor
    dd  code                    ; Start of kernel '.text' (code) section.
    dd  bss                     ; End of kernel '.data' section.
    dd  kernel_physical_end     ; End of kernel.
    dd  start                   ; Kernel entry point (initial EIP).

[GLOBAL start]                  ; Kernel entry point.
[EXTERN kernel_main]            ; This is the entry point of our C code

align 0x1000
section .bss
stack_bottom:
    times 16384 db 0 ;16 kb of stack
stack_top:
page_directory_boot:
    times 1024 dd 0
page_table_boot:
    times 1024 dd 0

section .text

_start:
    GLOBAL PAGE_DIR_VIRTUAL
    PAGE_DIR_VIRTUAL equ page_directory_boot
    GLOBAL PAGE_DIR_PHYSICAL
    PAGE_DIR_PHYSICAL equ (page_directory_boot - KERNEL_VIRTUAL)
    start equ (_start - KERNEL_VIRTUAL)
    mov ecx, 1024
    lea edi, [page_table_boot - KERNEL_VIRTUAL]

    mov esi, 0
    lbl:
        mov edx, esi
        or edx, 3 ;present, read-write
        mov dword [edi], edx
        add esi, 4096
        add edi, 4
        loop lbl

    mov edi, PAGE_DIR_PHYSICAL
    lea edx, [page_table_boot - KERNEL_VIRTUAL]

    or edx, 3
    mov dword [edi], edx
    mov dword [edi + 768 * 4], edx

    mov ecx, PAGE_DIR_PHYSICAL
    mov cr3, ecx

    mov ecx, cr0
    or ecx, 0x80000001
    mov cr0, ecx

    lea ecx, [higher_half]
    jmp ecx

section .text

higher_half:
    mov edi, PAGE_DIR_VIRTUAL
    mov dword [edi], 0

    mov esp, stack_top
    mov ebp, stack_bottom
    ; Load multiboot information:
    add ebx, KERNEL_VIRTUAL
    push ebx
    ; Execute the kernel:
    cli                         ; Disable interrupts.
    call kernel_main            ; call our main() function.
    jmp $                       ; Enter an infinite loop, to stop the processor
                                ; executing whatever rubbish is in the memory
                                ; after our kernel!