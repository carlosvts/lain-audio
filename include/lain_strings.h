#ifndef LAIN_STRINGS_H
#define LAIN_STRINGS_H

#include "lain.h"

typedef struct
{
    char *data;
    usize len;
} String;

static inline usize lain_stringlen(const char *string)
{
    usize len = 0;

    while (string[len] != '\0')
    {
        len++;
    }

    return len;
}

static inline String lain_string(char *str)
{
    String s = {
        .data = str,
        .len = lain_stringlen(str),
    };

    return s;
}

static inline void lain_reverse(char *str, usize len)
{
    for (usize i = 0, j = len - 1; i < j; i++, j--)
    {
        char tmp = str[i];
        str[i] = str[j];
        str[j] = tmp;
    }
}

static inline void lain_itoa(i32 num, char *str)
{
    usize i = 0;
    bool is_negative = false;

    if (num == 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    if (num < 0)
    {
        is_negative = true;
        num = -num;
    }

    while (num != 0)
    {
        i32 rem = num % 10;
        str[i++] = rem + '0';
        num /= 10;
    }

    if (is_negative)
    {
        str[i++] = '-';
    }

    str[i] = '\0';

    lain_reverse(str, i);
}

#endif /* LAIN_STRINGS_H */
