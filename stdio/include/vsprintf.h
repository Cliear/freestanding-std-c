#ifndef FREESTANDING_STD_C_STDIO_VSPRINTF_H
#define FREESTANDING_STD_C_STDIO_VSPRINTF_H

#include <stdarg.h>

/*
 * @retval return value description
 * @format %[flag][width][length][type]
 * @flag: + - % # 0
 * @width: integer
 * @length: h hh l(ell) ll(ell ell) j z t
 * @type: c s d i o x X
 */
int vsprintf(char *buffer, const char *format, va_list parament);

#endif