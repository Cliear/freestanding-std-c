#ifndef FREESTANDING_STD_C_STRING_MEMCPY_H
#define FREESTANDING_STD_C_STRING_MEMCPY_H

#include <stddef.h>

static inline void * memcpy(void * restrict des, const void * restrict src, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        ((char *)des)[i] = ((const char *)src)[i];
    }
    return des;
}

#endif