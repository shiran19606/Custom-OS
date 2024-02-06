

[GLOBAL load_page_directory]

load_page_directory:
   mov [esp+4], eax
   mov eax, cr3
   ret

[GLOBAL flush_tlb]

flush_tlb:
   mov cr3, eax
   mov eax, cr3