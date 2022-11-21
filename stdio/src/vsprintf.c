#include <stdarg.h>
#include <stddef.h>

#include <stdint.h>
#include <stdbool.h>
#include <vsprintf.h>

typedef enum {PRINT_LEFT_JUSTIFIED, PRINT_RIGHT_JUSTIFIED} PrintDirection;
typedef struct PrintfState
{
    size_t step;
    size_t precision;
    bool   conversion;
    char filled_charactor;
    PrintDirection justified;
    struct PrintfState (*handle)(struct PrintfState state, char *des, char charactor, va_list * ap_ptr);
} PrintfState;

static inline PrintfState handle_intmax_t_d(char *des, intmax_t value);
static inline PrintfState handle_intmax_t_u(char *des, uintmax_t value);
static inline PrintfState handle_uintmax_t_octal(char *des, uintmax_t value);
static inline PrintfState handle_uintmax_t_lower_hex(char *des, uintmax_t value);
static inline PrintfState handle_uintmax_t_upper_hex(char *des, uintmax_t value);

static inline PrintfState handle_intmax_t_d_for_precision(PrintfState state, char *des, intmax_t value);
static inline PrintfState handle_intmax_t_u_for_precision(PrintfState state, char *des, uintmax_t value);
static inline PrintfState handle_uintmax_t_octal_for_precision(PrintfState state, char *des, uintmax_t value);
static inline PrintfState handle_uintmax_t_lower_hex_for_precision(PrintfState state, char *des, uintmax_t value);
static inline PrintfState handle_uintmax_t_upper_hex_for_precision(PrintfState state, char *des, uintmax_t value);

static inline PrintfState state_general(PrintfState state, char *des, char charactor, va_list * ap_ptr);
static inline PrintfState state_percent_sign(PrintfState state, char *des, char charactor, va_list * ap_ptr);
static inline PrintfState state_minus_sign(PrintfState state, char *des, char charactor, va_list * ap_ptr);

static char number2charactor[] =
{
    [0] = '0', [1] = '1', [2] = '2', [3] = '3', [4] = '4',
    [5] = '5', [6] = '6', [7] = '7', [8] = '8', [9] = '9'
};

static char oct2charactor[] =
{
    [0] = '0', [1] = '1', [ 2] = '2', [ 3] = '3', [ 4] = '4', [ 5] = '5', [ 6] = '6', [ 7] = '7'
};

static inline int char2integer(char charactor)
{
    switch (charactor)
    {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        default:  return 0;
    }
}

static inline size_t get_lowwer_value(size_t value, size_t system)
{
    return value & (system - 1);
}

static inline size_t remove_lowwer_value(size_t value, size_t system)
{
    if ((system & (system - 1)) != 0)
    {
        return 0;
    }
    else if (system == 8)
    {
        return value >> 3;
    }
    else if (system == 16)
    {
        return  value >> 4;
    }
    else  
    {
        return 0;
    }
}

static char hex2lower_charactor[] =
{
    [0] = '0', [1] = '1', [ 2] = '2', [ 3] = '3', [ 4] = '4', [ 5] = '5', [ 6] = '6', [ 7] = '7', 
    [8] = '8', [9] = '9', [10] = 'a', [11] = 'b', [12] = 'c', [13] = 'd', [14] = 'e', [15] = 'f'
};

static char hex2upper_charactor[] =
{
    [0] = '0', [1] = '1', [ 2] = '2', [ 3] = '3', [ 4] = '4', [ 5] = '5', [ 6] = '6', [ 7] = '7', 
    [8] = '8', [9] = '9', [10] = 'A', [11] = 'B', [12] = 'C', [13] = 'D', [14] = 'E', [15] = 'F'
};

static inline PrintfState helper_handle_char_move(char * src, size_t length, size_t precision, PrintDirection justified, char filled_charactor)
{
    if (length < precision)
    {
        if (justified == PRINT_LEFT_JUSTIFIED)
        {
            char * start = src + length;
            for (size_t i = 0; i < (precision - length); i++)
            {
                start[i] = ' ';
            }
            src[precision] = '\0';
            return (PrintfState)
            {
                .step = precision,
                .handle = state_general,
                .precision = 0,
                .conversion = false,
                .filled_charactor = ' ',
                .justified = PRINT_RIGHT_JUSTIFIED,
            };
        }
        else
        {
            if (filled_charactor != '0' && filled_charactor != ' ')
            {
                filled_charactor = ' ';
            }
            char * end = src + precision;
            for (size_t i = 0; i <= length; i++)
            {
                end[-i] = src[length - i];
            }
            char * start = src;
            for (size_t i = 0; i < (precision - length); i++)
            {
                start[i] = filled_charactor;
            }
            src[precision + 1] = '\0';
            return (PrintfState)
            {
                .step = precision,
                .handle = state_general,
                .precision = 0,
                .conversion = false,
                .filled_charactor = ' ',
                .justified = PRINT_RIGHT_JUSTIFIED,
            };
        }
    }
    else
    {
        return (PrintfState)
        {
            .step = length,
            .handle = state_general,
            .precision = 0,
            .conversion = false,
            .filled_charactor = ' ',
            .justified = PRINT_RIGHT_JUSTIFIED,
        };
    }
}

static inline PrintfState handle_charactor(char *des, char charactor)
{
    char * handle_ptr = des;
    *(handle_ptr++) = charactor;
    *handle_ptr = '\0';
    return (PrintfState)
    {
        .step = (size_t)(handle_ptr - des),
        .handle = state_general,
        .precision = 0,
        .conversion = false,
        .filled_charactor = ' ',
        .justified = PRINT_RIGHT_JUSTIFIED,
    };
}

static inline PrintfState handle_charactor_for_precision(char *des, char charactor, PrintfState state, char filled_charactor)
{
    size_t precision = state.precision;
    PrintDirection justified = state.justified;
    char * handle_ptr = des;
    *(handle_ptr++) = charactor;
    *handle_ptr = '\0';
    if (precision < 2)
    {
        return (PrintfState)
        {
            .step = (size_t)(handle_ptr - des),
            .handle = state_general,
            .precision = 0,
            .conversion = false,
            .filled_charactor = ' ',
            .justified = PRINT_RIGHT_JUSTIFIED,
        };
    }
    else
    {
        return helper_handle_char_move(des, 1, precision, justified, filled_charactor);
    }
}

static inline PrintfState handle_string(char *des, const char * str)
{
    char * handle_ptr = des;
    while (*str != '\0')
    {
        *(handle_ptr++) = *(str++);
    }
    *handle_ptr = '\0';
    return (PrintfState)
    {
        .step = (size_t)(handle_ptr - des),
        .handle = state_general,
        .precision = 0,
        .conversion = false,
        .filled_charactor = ' ',
        .justified = PRINT_RIGHT_JUSTIFIED,
    };
}

static inline size_t strlen_imp(const char * str)
{
    const char * p = str;
    while (*p != '\0') p++;

    return (size_t)(p - str);
}

static inline void * memcpy_imp(void * restrict des, const void * restrict src, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        ((char *)des)[i] = ((const char *)src)[i];
    }
    return des;
}

static inline PrintfState handle_string_for_precision(char *des, const char * str, PrintfState state, char filled_charactor)
{
    size_t precision = state.precision;
    PrintDirection justified = state.justified;
    size_t str_length = strlen_imp(str);
    memcpy_imp(des, str, str_length + 1);
    return helper_handle_char_move(des, str_length, precision, justified, filled_charactor);
}

static inline PrintfState handle_point(char *des, uintptr_t value)
{
    char * handle_ptr = des;
    while (value >= 16)
    {
        char charactor = hex2upper_charactor[get_lowwer_value(value, 16)];
        *(handle_ptr++) = charactor;
        value = remove_lowwer_value(value, 16);
    }
    *(handle_ptr++) = hex2upper_charactor[get_lowwer_value(value, 16)];
    *handle_ptr = '\0';
    {
        char *left = des, *right = handle_ptr - 1;
        while (left < right)
        {
            char temp = *left;
            *(left++) = *right;
            *(right--) = temp;
        }
    }
    return (PrintfState)
    {
        .step = (size_t)(handle_ptr - des),
        .handle = state_general,
        .precision = 0,
        .conversion = false,
        .filled_charactor = ' ',
        .justified = PRINT_RIGHT_JUSTIFIED,
    };
}


// d, i, o, u, x, X
static inline PrintfState state_percent_hh(PrintfState state, char *des, char charactor, va_list * ap_ptr)
{
    switch (charactor)
    {
        case 'd':
        case 'i':
        {
            char value = va_arg(*ap_ptr, int);
            return handle_intmax_t_d(des, value);
        }
        case 'u':
        {
            unsigned char value = va_arg(*ap_ptr, unsigned int);
            return handle_intmax_t_u(des, value);
        }
        case 'o':
        {
            unsigned char value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_octal(des, value);
        }
        case 'x':
        {
            unsigned char value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_lower_hex(des, value);
        }
        case 'X':
        {
            unsigned char value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_upper_hex(des, value);
        }
        default:
        {
            return state_general(state, des, charactor, ap_ptr);
        }
    }
}

// d, i, o, u, x, X
static inline PrintfState state_percent_hh_for_precision(PrintfState state, char *des, char charactor, va_list * ap_ptr)
{
    switch (charactor)
    {
        case 'd':
        case 'i':
        {
            char value = va_arg(*ap_ptr, int);
            return handle_intmax_t_d_for_precision(state, des, value);
        }
        case 'u':
        {
            unsigned char value = va_arg(*ap_ptr, unsigned int);
            return handle_intmax_t_u_for_precision(state, des, value);
        }
        case 'o':
        {
            unsigned char value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_octal_for_precision(state, des, value);
        }
        case 'x':
        {
            unsigned char value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_lower_hex_for_precision(state, des, value);
        }
        case 'X':
        {
            unsigned char value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_upper_hex_for_precision(state, des, value);
        }
        default:
        {
            return state_general(state, des, charactor, ap_ptr);
        }
    }
}

static inline PrintfState state_percent_short(PrintfState state, char *des, char charactor, va_list * ap_ptr)
{
    switch (charactor)
    {
        case 'h':
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_percent_hh,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case 'd':
        case 'i':
        {
            short int value = va_arg(*ap_ptr, int);
            return handle_intmax_t_d(des, value);
            break;
        }
        case 'u':
        {
            unsigned short int value = va_arg(*ap_ptr, unsigned int);
            return handle_intmax_t_u(des, value);
            break;
        }
        case 'o':
        {
            unsigned short int value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_octal(des, value);
            break;
        }
        case 'x':
        {
            unsigned short int value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_lower_hex(des, value);
            break;
        }
        case 'X':
        {
            unsigned short int value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_upper_hex(des, value);
            break;
        }
        default:
        {
            return state_general(state, des, charactor, ap_ptr);
        }
    }
}

static inline PrintfState state_percent_short_for_precision(PrintfState state, char *des, char charactor, va_list * ap_ptr)
{
    switch (charactor)
    {
        case 'h':
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_percent_hh_for_precision,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case 'd':
        case 'i':
        {
            short int value = va_arg(*ap_ptr, int);
            return handle_intmax_t_d_for_precision(state, des, value);
        }
        case 'u':
        {
            unsigned short int value = va_arg(*ap_ptr, unsigned int);
            return handle_intmax_t_u_for_precision(state, des, value);
        }
        case 'o':
        {
            unsigned short int value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_octal_for_precision(state, des, value);
        }
        case 'x':
        {
            unsigned short int value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_lower_hex_for_precision(state, des, value);
        }
        case 'X':
        {
            unsigned short int value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_upper_hex_for_precision(state, des, value);
        }
        default:
        {
            return state_general(state, des, charactor, ap_ptr);
        }
    }
}

// d, i, o, u, x, X
static inline PrintfState state_percent_long_long(PrintfState state, char *des, char charactor, va_list * ap_ptr)
{
    typedef long long int           InnerSignedInteger;
    typedef unsigned long long int  InnerUnsignedInteger;
    switch (charactor)
    {
        case 'd':
        case 'i':
        {
            InnerSignedInteger value = va_arg(*ap_ptr, InnerSignedInteger);
            return handle_intmax_t_d(des, value);
            break;
        }
        case 'u':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_intmax_t_u(des, value);
            break;
        }
        case 'o':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_uintmax_t_octal(des, value);
            break;
        }
        case 'x':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_uintmax_t_lower_hex(des, value);
            break;
        }
        case 'X':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_uintmax_t_upper_hex(des, value);
            break;
        }
        default:
        {
            return state_general(state, des, charactor, ap_ptr);
        }
    }
}

// d, i, o, u, x, X
static inline PrintfState state_percent_long_long_for_precision(PrintfState state, char *des, char charactor, va_list * ap_ptr)
{
    typedef long long int           InnerSignedInteger;
    typedef unsigned long long int  InnerUnsignedInteger;
    switch (charactor)
    {
        case 'd':
        case 'i':
        {
            InnerSignedInteger value = va_arg(*ap_ptr, InnerSignedInteger);
            return handle_intmax_t_d_for_precision(state, des, value);
        }
        case 'u':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_intmax_t_u_for_precision(state, des, value);
        }
        case 'o':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_uintmax_t_octal_for_precision(state, des, value);
        }
        case 'x':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_uintmax_t_lower_hex_for_precision(state, des, value);
        }
        case 'X':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_uintmax_t_upper_hex_for_precision(state, des, value);
        }
        default:
        {
            return state_general(state, des, charactor, ap_ptr);
        }
    }
}

// d, i, o, u, x, X
static inline PrintfState state_percent_long(PrintfState state, char *des, char charactor, va_list * ap_ptr)
{
    typedef long int            InnerSignedInteger;
    typedef unsigned long int   InnerUnsignedInteger;
    switch (charactor)
    {
        case 'l':
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_percent_long_long,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case 'd':
        case 'i':
        {
            InnerSignedInteger value = va_arg(*ap_ptr, InnerSignedInteger);
            return handle_intmax_t_d(des, value);
        }
        case 'u':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_intmax_t_u(des, value);
        }
        case 'o':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_uintmax_t_octal(des, value);
        }
        case 'x':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_uintmax_t_lower_hex(des, value);
        }
        case 'X':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_uintmax_t_upper_hex(des, value);
        }
        default:
        {
            return state_general(state, des, charactor, ap_ptr);
        }
    }
}

// d, i, o, u, x, X
static inline PrintfState state_percent_long_for_precision(PrintfState state, char *des, char charactor, va_list * ap_ptr)
{
    typedef long int            InnerSignedInteger;
    typedef unsigned long int   InnerUnsignedInteger;
    switch (charactor)
    {
        case 'l': // ell
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_percent_long_long_for_precision,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case 'd':
        case 'i':
        {
            InnerSignedInteger value = va_arg(*ap_ptr, InnerSignedInteger);
            return handle_intmax_t_d_for_precision(state, des, value);
        }
        case 'u':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_intmax_t_u_for_precision(state, des, value);
        }
        case 'o':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_uintmax_t_octal_for_precision(state, des, value);
        }
        case 'x':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_uintmax_t_lower_hex_for_precision(state, des, value);
        }
        case 'X':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_uintmax_t_upper_hex_for_precision(state, des, value);
            break;
        }
        default:
        {
            return state_general(state, des, charactor, ap_ptr);
        }
    }
}

static inline PrintfState handle_intmax_t_d(char *des, intmax_t value)
{
    char * handle_ptr = des;
    char * number_start_position = des;
    if (value < 0)
    {
        *(handle_ptr++) = '-';
        value = -value;
        number_start_position++;
    }
    while (value / 10 != 0)
    {
        *(handle_ptr++) = number2charactor[value % 10];
        value /= 10;
    }
    *(handle_ptr++) = number2charactor[value % 10];
    *handle_ptr = '\0';
    {
        char *left = number_start_position, *right = handle_ptr - 1;
        while (left < right)
        {
            char temp = *left;
            *(left++) = *right;
            *(right--) = temp;
        }
    }
    return (PrintfState)
    {
        .step = (size_t)(handle_ptr - des),
        .handle = state_general,
        .precision = 0,
        .conversion = false,
        .filled_charactor = ' ',
        .justified = PRINT_RIGHT_JUSTIFIED,
    };
}

static inline PrintfState handle_intmax_t_u(char *des, uintmax_t value)
{
    char * handle_ptr = des;
    char * number_start_position = des;
    while (value / 10 != 0)
    {
        *(handle_ptr++) = number2charactor[value % 10];
        value /= 10;
    }
    *(handle_ptr++) = number2charactor[value % 10];
    *handle_ptr = '\0';
    {
        char *left = number_start_position, *right = handle_ptr - 1;
        while (left < right)
        {
            char temp = *left;
            *(left++) = *right;
            *(right--) = temp;
        }
    }
    return (PrintfState)
    {
        .step = (size_t)(handle_ptr - des),
        .handle = state_general,
        .precision = 0,
        .conversion = false,
        .filled_charactor = ' ',
        .justified = PRINT_RIGHT_JUSTIFIED,
    };
}

static inline PrintfState handle_uintmax_t_octal(char *des, uintmax_t value)
{
    char * handle_ptr = des;
    while (value >= 8)
    {
        char charactor = oct2charactor[get_lowwer_value(value, 8)];
        *(handle_ptr++) = charactor;
        value = remove_lowwer_value(value, 8);
    }
    *(handle_ptr++) = oct2charactor[get_lowwer_value(value, 8)];
    *handle_ptr = '\0';
    {
        char *left = des, *right = handle_ptr - 1;
        while (left < right)
        {
            char temp = *left;
            *(left++) = *right;
            *(right--) = temp;
        }
    }
    return (PrintfState)
    {
        .step = (size_t)(handle_ptr - des),
        .handle = state_general,
        .precision = 0,
        .conversion = false,
        .filled_charactor = ' ',
        .justified = PRINT_RIGHT_JUSTIFIED,
    };
}

static inline PrintfState handle_uintmax_t_lower_hex(char *des, uintmax_t value)
{
    char * handle_ptr = des;
    while (value >= 16)
    {
        char charactor = hex2lower_charactor[get_lowwer_value(value, 16)];
        *(handle_ptr++) = charactor;
        value = remove_lowwer_value(value, 16);
    }
    *(handle_ptr++) = hex2lower_charactor[get_lowwer_value(value, 16)];
    *handle_ptr = '\0';
    {
        char *left = des, *right = handle_ptr - 1;
        while (left < right)
        {
            char temp = *left;
            *(left++) = *right;
            *(right--) = temp;
        }
    }
    return (PrintfState)
    {
        .step = (size_t)(handle_ptr - des),
        .handle = state_general,
        .precision = 0,
        .conversion = false,
        .filled_charactor = ' ',
        .justified = PRINT_RIGHT_JUSTIFIED,
    };
}

static inline PrintfState handle_uintmax_t_upper_hex(char *des, uintmax_t value)
{
    char * handle_ptr = des;
    while (value >= 16)
    {
        char charactor = hex2upper_charactor[get_lowwer_value(value, 16)];
        *(handle_ptr++) = charactor;
        value = remove_lowwer_value(value, 16);
    }
    *(handle_ptr++) = hex2upper_charactor[get_lowwer_value(value, 16)];
    *handle_ptr = '\0';
    {
        char *left = des, *right = handle_ptr - 1;
        while (left < right)
        {
            char temp = *left;
            *(left++) = *right;
            *(right--) = temp;
        }
    }
    return (PrintfState)
    {
        .step = (size_t)(handle_ptr - des),
        .handle = state_general,
        .precision = 0,
        .conversion = false,
        .filled_charactor = ' ',
        .justified = PRINT_RIGHT_JUSTIFIED,
    };
}

static inline PrintfState handle_intmax_t_d_for_precision(PrintfState state, char *des, intmax_t value)
{
    size_t precision = state.precision;
    PrintDirection justified = state.justified; 
    char filled_charactor = state.filled_charactor;
    char * handle_ptr = des;
    char * number_start_position = des;
    if (value < 0)
    {
        *(handle_ptr++) = '-';
        value = -value;
        number_start_position++;
    }
    while (value / 10 != 0)
    {
        *(handle_ptr++) = number2charactor[value % 10];
        value /= 10;
    }
    *(handle_ptr++) = number2charactor[value % 10];
    *handle_ptr = '\0';
    {
        char *left = number_start_position, *right = handle_ptr - 1;
        while (left < right)
        {
            char temp = *left;
            *(left++) = *right;
            *(right--) = temp;
        }
    }
    size_t length = (size_t)(handle_ptr - des);
    if (length < precision)
    {
        if (justified == PRINT_LEFT_JUSTIFIED)
        {
            char * start = handle_ptr;
            for (size_t i = 0; i < (precision - length); i++)
            {
                start[i] = ' ';
            }
            handle_ptr[precision - length] = '\0';
            return (PrintfState)
            {
                .step = (size_t)((handle_ptr + precision - length) - des),
                .handle = state_general,
                .precision = 0,
                .conversion = false,
                .filled_charactor = ' ',
                .justified = PRINT_RIGHT_JUSTIFIED,
            };
        }
        else
        {
            if (filled_charactor != '0' && filled_charactor != ' ')
            {
                filled_charactor = ' ';
            }
            char * end = handle_ptr + (precision - length);
            for (size_t i = 0; i <= length; i++)
            {
                end[-i] = handle_ptr[-i];
            }
            char * start = des;
            for (size_t i = 0; i < (precision - length); i++)
            {
                start[i] = filled_charactor;
            }
            handle_ptr[precision - length] = '\0';
            return (PrintfState)
            {
                .step = (size_t)((handle_ptr + precision - length) - des),
                .handle = state_general,
                .precision = 0,
                .conversion = false,
                .filled_charactor = ' ',
                .justified = PRINT_RIGHT_JUSTIFIED,
            };
        }
    }
    else 
    {
        return (PrintfState)
        {
            .step = (size_t)(handle_ptr - des),
            .handle = state_general,
            .precision = 0,
            .conversion = false,
            .filled_charactor = ' ',
            .justified = PRINT_RIGHT_JUSTIFIED,
        };
    }
}

static inline PrintfState handle_intmax_t_u_for_precision(PrintfState state, char *des, uintmax_t value)
{
    size_t precision = state.precision;
    PrintDirection justified = state.justified; 
    char filled_charactor = state.filled_charactor;
    char * handle_ptr = des;
    char * number_start_position = des;
    while (value / 10 != 0)
    {
        *(handle_ptr++) = number2charactor[value % 10];
        value /= 10;
    }
    *(handle_ptr++) = number2charactor[value % 10];
    *handle_ptr = '\0';
    {
        char *left = number_start_position, *right = handle_ptr - 1;
        while (left < right)
        {
            char temp = *left;
            *(left++) = *right;
            *(right--) = temp;
        }
    }
    return helper_handle_char_move(des, handle_ptr - des, precision, justified, filled_charactor);
}

static inline PrintfState handle_uintmax_t_octal_for_precision(PrintfState state, char *des, uintmax_t value)
{
    size_t precision = state.precision;
    PrintDirection justified = state.justified; 
    char filled_charactor = state.filled_charactor;
    char * handle_ptr = des;
    while (value >= 8)
    {
        char charactor = oct2charactor[get_lowwer_value(value, 8)];
        *(handle_ptr++) = charactor;
        value = remove_lowwer_value(value, 8);
    }
    *(handle_ptr++) = oct2charactor[get_lowwer_value(value, 8)];
    if (state.conversion)
    {
        *(handle_ptr++) = '0';
        *handle_ptr = '\0';
    }
    *handle_ptr = '\0';
    {
        char *left = des, *right = handle_ptr - 1;
        while (left < right)
        {
            char temp = *left;
            *(left++) = *right;
            *(right--) = temp;
        }
    }
    return helper_handle_char_move(des, handle_ptr - des, precision, justified, filled_charactor);
}

static inline PrintfState handle_uintmax_t_lower_hex_for_precision(PrintfState state, char *des, uintmax_t value)
{
    size_t precision = state.precision;
    PrintDirection justified = state.justified; 
    char filled_charactor = state.filled_charactor;
    char * handle_ptr = des;
    while (value >= 16)
    {
        char charactor = hex2lower_charactor[get_lowwer_value(value, 16)];
        *(handle_ptr++) = charactor;
        value = remove_lowwer_value(value, 16);
    }
    *(handle_ptr++) = hex2lower_charactor[get_lowwer_value(value, 16)];
    size_t str_offset = 0;
    if (state.conversion)
    {
        *(handle_ptr++) = 'x';
        *(handle_ptr++) = '0';
        str_offset = 2;
    }
    *handle_ptr = '\0';
    {
        char *left = des, *right = handle_ptr - 1;
        while (left < right)
        {
            char temp = *left;
            *(left++) = *right;
            *(right--) = temp;
        }
    }
    return helper_handle_char_move(des + str_offset, handle_ptr - des, precision, justified, filled_charactor);
}

static inline PrintfState handle_uintmax_t_upper_hex_for_precision(PrintfState state, char *des, uintmax_t value)
{
    size_t precision = state.precision;
    PrintDirection justified = state.justified; 
    char filled_charactor = state.filled_charactor;
    char * handle_ptr = des;
    while (value >= 16)
    {
        char charactor = hex2upper_charactor[get_lowwer_value(value, 16)];
        *(handle_ptr++) = charactor;
        value = remove_lowwer_value(value, 16);
    }
    *(handle_ptr++) = hex2upper_charactor[get_lowwer_value(value, 16)];
    size_t str_offset = 0;
    if (state.conversion)
    {
        *(handle_ptr++) = 'X';
        *(handle_ptr++) = '0';
        str_offset = 2;
    }
    *handle_ptr = '\0';
    {
        char *left = des, *right = handle_ptr - 1;
        while (left < right)
        {
            char temp = *left;
            *(left++) = *right;
            *(right--) = temp;
        }
    }
    return helper_handle_char_move(des + str_offset, handle_ptr - des, precision, justified, filled_charactor);
}

static inline PrintfState state_intmax_t_for_precision(PrintfState state, char *des, char charactor, va_list * ap_ptr)
{
    typedef intmax_t    InnerSignedInteger;
    typedef uintmax_t   InnerUnsignedInteger;
    switch (charactor)
    {
        case 'd':
        case 'i':
        {
            InnerSignedInteger value = va_arg(*ap_ptr, InnerSignedInteger);
            return handle_intmax_t_d_for_precision(state, des, value);
        }
        case 'u':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_intmax_t_u_for_precision(state, des, value);
        }
        case 'o':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_uintmax_t_octal_for_precision(state, des, value);
        }
        case 'x':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_uintmax_t_lower_hex_for_precision(state, des, value);
        }
        case 'X':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, InnerUnsignedInteger);
            return handle_uintmax_t_upper_hex_for_precision(state, des, value);
        }
        default:
        {
            return state_general(state, des, charactor, ap_ptr);
        }
    }
}

static inline PrintfState state_size_t(PrintfState state, char *des, char charactor, va_list * ap_ptr)
{
    switch (charactor)
    {
        case 'd':
        case 'i':
        {
            union{intmax_t integer_value; size_t size_value;} value = {.size_value = va_arg(*ap_ptr, size_t)};
            return handle_intmax_t_d(des, value.integer_value);
        }
        case 'u':
        {
            size_t value = va_arg(*ap_ptr, size_t);
            return handle_intmax_t_u(des, value);
        }
        case 'o':
        {
            size_t value = va_arg(*ap_ptr, size_t);
            return handle_uintmax_t_octal(des, value);
        }
        case 'x':
        {
            size_t value = va_arg(*ap_ptr, size_t);
            return handle_uintmax_t_lower_hex(des, value);
        }
        case 'X':
        {
            size_t value = va_arg(*ap_ptr, size_t);
            return handle_uintmax_t_upper_hex(des, value);
        }
        default:
        {
            return state_general(state, des, charactor, ap_ptr);
        }
    }
}

static inline PrintfState state_size_t_for_precision(PrintfState state, char *des, char charactor, va_list * ap_ptr)
{
    switch (charactor)
    {
        case 'd':
        case 'i':
        {
            union{intmax_t integer_value; size_t size_value;} value = {.size_value = va_arg(*ap_ptr, size_t)};
            return handle_intmax_t_d_for_precision(state, des, value.integer_value);
        }
        case 'u':
        {
            size_t value = va_arg(*ap_ptr, size_t);
            return handle_intmax_t_u_for_precision(state, des, value);
        }
        case 'o':
        {
            size_t value = va_arg(*ap_ptr, size_t);
            return handle_uintmax_t_octal_for_precision(state, des, value);
        }
        case 'x':
        {
            size_t value = va_arg(*ap_ptr, size_t);
            return handle_uintmax_t_lower_hex_for_precision(state, des, value);
        }
        case 'X':
        {
            size_t value = va_arg(*ap_ptr, size_t);
            return handle_uintmax_t_upper_hex_for_precision(state, des, value);
        }
        default:
        {
            return state_general(state, des, charactor, ap_ptr);
        }
    }
}

static inline PrintfState state_ptrdiff_t(PrintfState state, char *des, char charactor, va_list * ap_ptr)
{
    typedef intmax_t    InnerSignedInteger;
    typedef uintmax_t   InnerUnsignedInteger;
    switch (charactor)
    {
        case 'd':
        case 'i':
        {
            InnerSignedInteger value = va_arg(*ap_ptr, ptrdiff_t);
            return handle_intmax_t_d(des, value);
        }
        case 'u':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, ptrdiff_t);
            return handle_intmax_t_u(des, value);
        }
        case 'o':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, ptrdiff_t);
            return handle_uintmax_t_octal(des, value);
        }
        case 'x':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, ptrdiff_t);
            return handle_uintmax_t_lower_hex(des, value);
        }
        case 'X':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, ptrdiff_t);
            return handle_uintmax_t_upper_hex(des, value);
        }
        default:
        {
            return state_general(state, des, charactor, ap_ptr);
        }
    }
}

static inline PrintfState state_ptrdiff_t_for_precision(PrintfState state, char *des, char charactor, va_list * ap_ptr)
{
    typedef intmax_t    InnerSignedInteger;
    typedef uintmax_t   InnerUnsignedInteger;
    switch (charactor)
    {
        case 'd':
        case 'i':
        {
            InnerSignedInteger value = va_arg(*ap_ptr, ptrdiff_t);
            return handle_intmax_t_d_for_precision(state, des, value);
        }
        case 'u':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, ptrdiff_t);
            return handle_intmax_t_u_for_precision(state, des, value);
        }
        case 'o':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, ptrdiff_t);
            return handle_uintmax_t_octal_for_precision(state, des, value);
        }
        case 'x':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, ptrdiff_t);
            return handle_uintmax_t_lower_hex_for_precision(state, des, value);
        }
        case 'X':
        {
            InnerUnsignedInteger value = va_arg(*ap_ptr, ptrdiff_t);
            return handle_uintmax_t_upper_hex_for_precision(state, des, value);
        }
        default:
        {
            return state_general(state, des, charactor, ap_ptr);
        }
    }
}

static inline PrintfState state_integer_precision(PrintfState state, char *des, char charactor, va_list * ap_ptr)
{
    switch (charactor)
    {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
            size_t precision = char2integer(charactor) + state.precision * 10;
            return (PrintfState)
            {
                .step = 0,
                .handle = state_integer_precision,
                .precision = precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case 'i':
        case 'd':
        {
            int value = va_arg(*ap_ptr, int);
            return handle_intmax_t_d_for_precision(state, des, value);
        }
        case 'u':
        {
            unsigned int value = va_arg(*ap_ptr, unsigned int);
            return handle_intmax_t_u_for_precision(state, des, value);
        }
        case 'o':
        {
            unsigned int value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_octal_for_precision(state, des, value);
        }
        case 'x':
        {
            unsigned int value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_lower_hex_for_precision(state, des, value);
        }
        case 'X':
        {
            unsigned int value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_upper_hex_for_precision(state, des, value);
        }
        case 'h':
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_percent_short_for_precision,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case 'l': // ell
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_percent_long_for_precision,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case 'j': // d, i, o, u, x, X intmax_t
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_intmax_t_for_precision,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case 'z': // d, i, o, u, x, X size_t
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_size_t_for_precision,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case 't': // d, i, o, u, x, X ptrdiff_t
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_ptrdiff_t_for_precision,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case 'c':
        {
            char charactor = va_arg(*ap_ptr, int);
            return handle_charactor_for_precision(des, charactor, state, ' ');
        }
        case 's':
        {
            const char * str = va_arg(*ap_ptr, const char *);
            return handle_string_for_precision(des, str, state, ' ');
        }
        case 'p':
        {
            uintptr_t value = (uintptr_t)va_arg(*ap_ptr, void *);
            return handle_uintmax_t_upper_hex_for_precision(state, des, value);
        }
        default:
        {
            *des = charactor;
            return (PrintfState)
            {
                .step = 1,
                .handle = state_general,
                .precision = 0,
                .conversion = state.conversion,
                .filled_charactor = ' ',
                .justified = PRINT_RIGHT_JUSTIFIED,
            };
        }
    }
}

static inline PrintfState state_minus_sign(PrintfState state, char *des, char charactor, va_list * ap_ptr)
{
    char * handle_ptr = des;
    switch (charactor)
    {
        case '%':
        {
            *(handle_ptr++) = '%';
            *handle_ptr = '\0';
            return (PrintfState)
            {
                .step = (size_t)(handle_ptr - des),
                .handle = state_general,
                .precision = 0,
                .conversion = false,
                .filled_charactor = ' ',
                .justified = PRINT_RIGHT_JUSTIFIED,
            };
        }
        case '-':
        {
            return (PrintfState)
            {
                .step = (size_t)(handle_ptr - des),
                .handle = state_general,
                .precision = 0,
                .conversion = false,
                .filled_charactor = ' ',
                .justified = PRINT_RIGHT_JUSTIFIED,
            };
        }
        case '+':
        {
            // ignore the opt
            return (PrintfState)
            {
                .step = 0,
                .handle = state_minus_sign,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case ' ':
        {
            // ignore the opt
            return (PrintfState)
            {
                .step = 0,
                .handle = state_general,
                .precision = 0,
                .conversion = false,
                .filled_charactor = ' ',
                .justified = state.justified,
            };
        }
        case '#':
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_percent_sign,
                .precision = state.precision,
                .conversion = true,
                .filled_charactor = state.filled_charactor,
                .justified = PRINT_LEFT_JUSTIFIED,
            };
        }
        case '0':
        {
            // ignore the opt
            return (PrintfState)
            {
                .step = 0,
                .handle = state_minus_sign,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = ' ',
                .justified = PRINT_LEFT_JUSTIFIED,
            };
        }
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
            size_t precision = char2integer(charactor);
            return (PrintfState)
            {
                .step = 0,
                .handle = state_integer_precision,
                .precision = precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = PRINT_LEFT_JUSTIFIED,
            };
        }
        case 'i':
        case 'd':
        {
            int value = va_arg(*ap_ptr, int);
            return handle_intmax_t_d_for_precision(state, des, value);
        }
        case 'u':
        {
            unsigned int value = va_arg(*ap_ptr, unsigned int);
            return handle_intmax_t_u_for_precision(state, des, value);
        }
        case 'o':
        {
            unsigned int value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_octal_for_precision(state, des, value);
        }
        case 'x':
        {
            unsigned int value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_lower_hex_for_precision(state, des, value);
        }
        case 'X':
        {
            unsigned int value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_upper_hex_for_precision(state, des, value);
        }
        case 'h':
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_percent_short,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = PRINT_LEFT_JUSTIFIED,
            };
            ;
        }
        case 'l': // ell
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_percent_long,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = PRINT_LEFT_JUSTIFIED,
            };
        }
        case 'j': // d, i, o, u, x, X intmax_t
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_intmax_t_for_precision,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = PRINT_LEFT_JUSTIFIED,
            };
            ;
        }
        case 'z': // d, i, o, u, x, X size_t
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_size_t,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = PRINT_LEFT_JUSTIFIED,
            };
            ;
        }
        case 't': // d, i, o, u, x, X ptrdiff_t
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_ptrdiff_t,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = PRINT_LEFT_JUSTIFIED,
            };
        }
        case 'c':
        {
            char charactor = va_arg(*ap_ptr, int);
            return handle_charactor_for_precision(des, charactor, state, ' ');
        }
        case 's':
        {
            const char * str = va_arg(*ap_ptr, const char *);
            return handle_string_for_precision(des, str, state, ' ');
        }
        case 'p':
        {
            uintmax_t value = (uintptr_t)va_arg(*ap_ptr, void *);
            return handle_uintmax_t_upper_hex_for_precision(state, des, value);
        }
        default:
        {
            *(des++) = charactor;
            *des = '\0';
            return (PrintfState)
            {
                .step = 1,
                .handle = state_general,
                .precision = 0,
                .conversion = false,
                .filled_charactor = ' ',
                .justified = PRINT_RIGHT_JUSTIFIED,
            };
        }
    }
}

static inline PrintfState state_percent_sign(PrintfState state, char *des, char charactor, va_list * ap_ptr)
{
    char * handle_ptr = des;
    switch (charactor)
    {
        case '%':
        {
            *(handle_ptr++) = '%';
            *handle_ptr = '\0';
            return (PrintfState)
            {
                .step = (size_t)(handle_ptr - des),
                .handle = state_general,
                .precision = 0,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case '-':
        {
            return (PrintfState)
            {
                .step = (size_t)(handle_ptr - des),
                .handle = state_minus_sign,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = PRINT_LEFT_JUSTIFIED,
            };
        }
        case '+':
        {
            // ignore the opt
            return (PrintfState)
            {
                .step = 0,
                .handle = state_percent_sign,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case ' ':
        {
            // ignore the opt
            return (PrintfState)
            {
                .step = 0,
                .handle = state_general,
                .precision = 0,
                .conversion = false,
                .filled_charactor = ' ',
                .justified = PRINT_RIGHT_JUSTIFIED,
            };
        }
        case '#':
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_percent_sign,
                .precision = state.precision,
                .conversion = true,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case '0':
        {
            // ignore the opt
            return (PrintfState)
            {
                .step = 0,
                .handle = state_percent_sign,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = '0',
                .justified = state.justified,
            };
        }
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
            size_t precision = char2integer(charactor);
            return (PrintfState)
            {
                .step = 0,
                .handle = state_integer_precision,
                .precision = precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case 'i':
        case 'd':
        {
            int value = va_arg(*ap_ptr, int);
            return handle_intmax_t_d(des, value);
        }
        case 'u':
        {
            unsigned int value = va_arg(*ap_ptr, unsigned int);
            return handle_intmax_t_u(des, value);
        }
        case 'o':
        {
            unsigned int value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_octal(des, value);
        }
        case 'x':
        {
            unsigned int value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_lower_hex(des, value);
        }
        case 'X':
        {
            unsigned int value = va_arg(*ap_ptr, unsigned int);
            return handle_uintmax_t_upper_hex(des, value);
        }
        case 'h':
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_percent_short,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case 'l': // ell
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_percent_long,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case 'j': // d, i, o, u, x, X intmax_t
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_intmax_t_for_precision,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case 'z': // d, i, o, u, x, X size_t
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_size_t,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case 't': // d, i, o, u, x, X ptrdiff_t
        {
            return (PrintfState)
            {
                .step = 0,
                .handle = state_ptrdiff_t,
                .precision = state.precision,
                .conversion = state.conversion,
                .filled_charactor = state.filled_charactor,
                .justified = state.justified,
            };
        }
        case 'c':
        {
            char charactor = va_arg(*ap_ptr, int);
            return handle_charactor(des, charactor);
        }
        case 's':
        {
            const char * str = va_arg(*ap_ptr, const char *);
            return handle_string(des, str);
        }
        case 'p':
        {
            void * value = va_arg(*ap_ptr, void *);
            return handle_point(des, (uintptr_t)value);
        }
        default:
        {
            *des = charactor;
            return (PrintfState)
            {
                .step = 1,
                .handle = state_general,
                .precision = 0,
                .conversion = false,
                .filled_charactor = ' ',
                .justified = PRINT_RIGHT_JUSTIFIED,
            };
        }
    }
}

static inline PrintfState state_general(PrintfState state, char *des, char charactor, va_list * ap_ptr)
{
    char * handle_ptr = des;
    if (charactor == '%')
    {
            return (PrintfState)
            {
                .step = (size_t)(handle_ptr - des),
                .handle = state_percent_sign,
                .precision = 0,
                .conversion = false,
                .filled_charactor = ' ',
                .justified = PRINT_RIGHT_JUSTIFIED,
            };
    }
    else
    {
        *handle_ptr++ = charactor;
        *handle_ptr = '\0';
        return (PrintfState)
        {
            .step = (size_t)(handle_ptr - des),
            .handle = state_general,
            .precision = 0,
            .conversion = false,
            .filled_charactor = ' ',
            .justified = PRINT_RIGHT_JUSTIFIED,
        };
    }
}

static inline PrintfState start(PrintfState state, char *des, char charactor, va_list * ap_ptr)
{
    return state_general(state, des, charactor, ap_ptr);
}

static inline PrintfState init_state(void)
{
    return (PrintfState)
    {
        .step = 0,
        .handle = start,
        .precision = 0,
        .conversion = false,
        .filled_charactor = ' ',
        .justified = PRINT_RIGHT_JUSTIFIED,
    };
}

/*
 * @retval return value description
 * @format %[flag][width][length][type]
 * @flag: + - % # 0
 * @width: integer
 * @length: h hh l(ell) ll(ell ell) j z t
 * @type: c s d i o x X
 */
int vsprintf(char *buffer, const char *format, va_list parament)
{
    int res = 0;
    PrintfState state = init_state();
    for (const char *position = format; *position != '\0'; position++)
    {
        state = state.handle(state, buffer + res, *position, &parament);
        res += state.step;
    }

    return res;
}
