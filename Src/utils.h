#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

// Function declarations
uint32_t strlen(const char* str);
uint32_t strcmp(const char* str1, const char* str2);
void memset(void* dst, uint32_t val, uint32_t size);
void memcpy(void* dst, const void* src, uint32_t size);

// Function declarations
uint8_t port_byte_in(uint16_t port);
void port_byte_out(uint16_t port, uint8_t data);

void acquire(int* lock);
void release(int* lock);

#endif /* UTILS_H */
