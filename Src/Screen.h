#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>
#include "utils.h"

#define VGA_COMMAND_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5

#define SPACE_ASCII_VALUE 0x20
#define BACKSPACE 0x08
#define TAB 0x09

#define LINES_VGA 25
#define COLUMNS_VGA 80
#define LINES_BACKBUFFER 75
#define COLUMNS_BACKBUFFER 80

void scrollUp();
void scrollDown();
void setX(uint8_t num);
void setY(uint8_t num);
uint8_t getX();
uint8_t getY();
void setCursorLocation();
void clearScreen();
void printString(const char* str);
void printNumber(uint32_t number);
void put_char(uint8_t charToPrint);
void printNumberHex(uint32_t intNumber);
void kprintf(const char* format, ...);


#endif /* SCREEN_H */