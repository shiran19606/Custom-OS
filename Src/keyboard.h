#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "Screen.h"
#include "utils.h"
#include "isr.h"
#include "heap.h"
#include "process.h"

#define IRQ1 33

void init_keyboard();

#endif