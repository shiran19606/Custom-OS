#include "Screen.h"
#include "ports.h"
#include "isr.h"
#define IRQ1 33

static void keyboard_callback(struct registers *regs);
void print_letter(uint8_t scancode);
void init_keyboard();