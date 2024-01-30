/* The bootloader will look at this image and start execution at the symbol
   designated at the entry point. */
ENTRY(start)

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
   /* The kernel is situated at 1 MiB */
   . = 0x00100000;

   kernel_start = .;

   /* First put the multiboot header, as it is required to be put very early
      early in the image or the bootloader won't recognize the file format.
      Next we'll put the .text section. */
   .text ALIGN(0x1000) :
   {
      code = .;
      *(.multiboot)
      *(.text)
   }

   /* Read-only data */
   .rodata ALIGN(0x1000) :
   {
      *(.rodata)
   }

   /* Read-write data (initialized) */
   .data ALIGN(0x1000) :
   {
      *(.data)
   }

   /* Read-write data (uninitialized) and stack */
   .bss ALIGN(0x1000) :
   {
      bss = .;
      *(.bss)
      *(.stack)
   }

   kernel_end = .;end = .;
}