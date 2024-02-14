#include "utils.h"

uint32_t strlen(const char* str)
{
    uint32_t length = 0;

    while (str[length] != '\0') {
        length++;
    }

    return length;
}

uint32_t strcmp(const char* str1, const char* str2)
{
    while (*str1 != '\0' && *str2 != '\0' && *str1 == *str2) {
        str1++;
        str2++;
    }

    return (int)(*str1) - (int)(*str2);
}

void memset(void* dst, uint32_t val, uint32_t size)
{
    //in case the value is not one byte, we make sure it is.
    val = (val & 0xFF);
    char* dest = (char*)dst;

    for (uint32_t i = 0; i < size; i++) {
        dest[i] = val;
    }
}

void memcpy(void* dst, const void* src, uint32_t size)
{
    char* dest = (char*) dst;
    const char* source = (const char*)src;

    for (uint32_t i = 0; i < size; i++) {
        dest[i] = source[i];
    }
}

// Read a byte from the specified port
uint8_t port_byte_in(uint16_t port) {
    uint8_t data;
    // Inline assembly to read from the specified port
    __asm__ __volatile__("inb %w1, %b0" : "=a"(data) : "Nd"(port));
    return data;
}

// Write a byte to the specified port
void port_byte_out(uint16_t port, uint8_t data) {
    // Inline assembly to write to the specified port
    __asm__ __volatile__("outb %b0, %w1" : : "a"(data), "Nd"(port));
}
