# compile c file to object file
gcc -c gdt.c -o gdt.o 

# compile asm file to object file
nasm -f elf gdt_flush.asm -o gdt_flush.o 

# link both object files
gcc gdt_flush.o gdt.o -o gdt