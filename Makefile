# $@ = target file
# $< = first dependency
# $^ = all dependencies

CC=/usr/local/i386elfgcc/bin/i386-elf-gcc

## Linker
LD=/usr/local/i386elfgcc/bin/i386-elf-ld

SRC_DIR = ./Src
BUILD_DIR = ./Build

OBJECTS = $(BUILD_DIR)/loader.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/gdt.o $(BUILD_DIR)/gdt_flush.o $(BUILD_DIR)/idt.o $(BUILD_DIR)/interrupts.o $(BUILD_DIR)/isr.o $(BUILD_DIR)/Screen.o $(BUILD_DIR)/ports.o $(BUILD_DIR)/keyboard.o	$(BUILD_DIR)/heap.o $(BUILD_DIR)/utils.o

# First rule is the one executed when no parameters are fed to the Makefile
all: run

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.s
	nasm $< -f elf32 -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	${CC} -m32 --no-pie -ggdb -std=gnu99 -ffreestanding -O2 -Wall -Wextra -c $< -o $@

$(BUILD_DIR)/kernel.elf: $(OBJECTS)
	${LD} -T $(SRC_DIR)/linker.ld -m elf_i386 $^ -o $@


build: prebuild $(BUILD_DIR)/kernel.elf
	mkdir -p iso/boot/grub
	cp $(SRC_DIR)/stage2_eltorito iso/boot/grub
	cp $(BUILD_DIR)/kernel.elf iso/boot/
	cp menu.lst iso/boot/grub
	genisoimage -R                          \
            -b boot/grub/stage2_eltorito    \
            -no-emul-boot                   \
            -boot-load-size 4               \
            -A os                           \
            -input-charset utf8             \
            -quiet                          \
            -boot-info-table                \
            -o os.iso                       \
            iso

run: build
	qemu-system-i386 -cdrom os.iso
	
debug: build
	qemu-system-i386 -s -S -cdrom os.iso
	
prebuild:
	clear
	make clean
	mkdir $(BUILD_DIR)
	
clean:
	rm -rf $(BUILD_DIR)
	rm -rf ./iso
	rm -f os.iso
	rm -f disk.img
