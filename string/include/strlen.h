#ifndef FREESTANDING_STD_C_STRING_STRLEN_H
#define FREESTANDING_STD_C_STRING_STRLEN_H

#include <stddef.h>

static inline size_t strlen(const char * str)
{
    const char * p = str;
    while (*p != '\0') p++;

    return (size_t)(p - str);
}

#endif