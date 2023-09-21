# $@ = target file
# $< = first dependency
# $^ = all dependencies

GDB = /home/linuxbrew/.linuxbrew/bin/i386-elf-gdb
CC=/usr/local/i386elfgcc/bin/i386-elf-gcc

OBJCP=/usr/local/i386elfgcc/bin/i386-elf-objcopy
## Linker
LD=/usr/local/i386elfgcc/bin/i386-elf-ld

AS=/usr/local/i386elfgcc/bin/i386-elf-as

SRC_DIR = ./Src
BUILD_DIR = ./Build

# First rule is the one executed when no parameters are fed to the Makefile
all: run

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.s
	nasm $< -f elf32 -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	${CC} -m32 --no-pie -ggdb -std=gnu99 -ffreestanding -O2 -Wall -Wextra -c $< -o $@

$(BUILD_DIR)/%.bin: $(SRC_DIR)/%.s
	nasm $< -f bin -o $@

$(BUILD_DIR)/kernel.elf: $(BUILD_DIR)/loader.o $(BUILD_DIR)/kernel.o
	${LD} -T $(SRC_DIR)/linker.ld -melf_i386 $^ -o $@

run: prebuild $(BUILD_DIR)/kernel.elf
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
# qemu-img create disk.img 1G
	qemu-system-i386 -cdrom os.iso
# -hda disk.img
prebuild:
	clear
	make clean
	mkdir $(BUILD_DIR)
	
clean:
	rm -rf $(BUILD_DIR)
	rm -rf ./iso
	rm -f os.iso
	rm -f disk.img