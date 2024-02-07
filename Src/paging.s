

[GLOBAL load_page_directory]
[EXTERN current_page_dir]

load_page_directory:
   mov eax, [current_page_dir]
   mov cr3, eax
   ret

[GLOBAL flush_tlb]

flush_tlb:
   mov eax, cr3
   mov cr3, eax
   ret

[GLOBAL enable_paging]

enable_paging:
   mov eax, cr0
   or eax, 80000001h
   mov cr0, eax
   ret