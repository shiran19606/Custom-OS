/* The bootloader will look at this image and start execution at the symbol
   designated at the entry point. */
ENTRY(start)

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
   /* The kernel is situated at 1 MiB */
   . = 0xC0100000;

   kernel_virtual_start = .;
   kernel_physical_start = . - 0xC0000000;

   /* First put the multiboot header, as it is required to be put very early
      early in the image or the bootloader won't recognize the file format.
      Next we'll put the .text section. */
   .text ALIGN(0x1000) : AT(ADDR(.text) - 0xC0000000)
   {
      code = .;
      *(.multiboot)
      *(.text)
   }

   /* Read-only data */
   .rodata ALIGN(0x1000) : AT(ADDR(.rodata) - 0xC0000000)
   {
      *(.rodata)
   }

   /* Read-write data (initialized) */
   .data ALIGN(0x1000) : AT(ADDR(.data) - 0xC0000000)
   {
      *(.data)
   }

   /* Read-write data (uninitialized) and stack */
   .bss ALIGN(0x1000) : AT(ADDR(.bss) - 0xC0000000)
   {
      bss = .;
      *(.bss)
      *(.stack)
   }

   kernel_virtual_end = .;
   kernel_physical_end = . - 0xC0000000;
}