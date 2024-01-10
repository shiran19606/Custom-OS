#include "utils.h"

int strlen(const char* str)
{
    int length = 0;

    while (str[length] != '\0') {
        length++;
    }

    return length;
}

int strcmp(const char* str1, const char* str2)
{
    while (*str1 != '\0' && *str2 != '\0' && *str1 == *str2) {
        str1++;
        str2++;
    }

    return (int)(*str1) - (int)(*str2);
}

void memset(void* dst, int val, size_t size)
{
    char* dest = (char*)dst;

    for (size_t i = 0; i < n; i++) {
        destination[i] = (char)value;
    }
}

void memcpy(void* dst, const void* src, size_t size)
{
    char* dest = (char*) dst;
    const char* source = (const char*)src;

    for (size_t i = 0; i < n; i++) {
        destination[i] = source[i];
    }
}
