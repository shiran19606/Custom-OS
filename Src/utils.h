#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

// Function declarations
uint32_t strlen(const char* str);
uint32_t strcmp(const char* str1, const char* str2);
void memset(void* dst, int val, uint32_t size);
void memcpy(void* dst, const void* src, uint32_t size);

#endif /* UTILS_H */
