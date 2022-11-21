#include <stdarg.h>

#include <sprintf.h>
#include <vsprintf.h>

int sprintf(char * restrict buffer, const char * restrict format, ...)
{
    va_list ap;
    va_start(ap, format);

    int res = vsprintf(buffer, format, ap);

    va_end(ap);

    return res;
}