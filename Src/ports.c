#include "ports.h"

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
