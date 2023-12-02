#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>
#include "ports.h"

#define VGA_COMMAND_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5

#define SPACE_ASCII_VALUE 0x20
#define BACKSPACE 0x08
#define TAB 0x09

void setCursorLocation();
void clearScreen();
void printString(char* str);
void printNumber(uint32_t number);
void put_char(uint8_t charToPrint);


#endif /* SCREEN_H */