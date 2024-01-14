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
