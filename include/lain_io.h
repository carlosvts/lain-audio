#ifndef LAIN_IO_H
#define LAIN_IO_H

#include "lain_strings.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

static inline i32 lain_print(String format, const String args[])
{
    usize argc = 0;

    for (usize i = 0; i < format.len; i++)
    {
        if (format.data[i] == '%' && i + 1 < format.len &&
            format.data[i + 1] == 's')
        {
            if (args[argc].data != NULL)
            {
                write(STDOUT_FILENO, args[argc].data, args[argc].len);

                argc++;
            }

            i++;
            continue;
        }

        char c = format.data[i];

        write(STDOUT_FILENO, &c, sizeof(c));
    }

    write(STDOUT_FILENO, "\n", 1);

    return 0;
}

static inline i32 lain_perror(String message)
{
    write(STDERR_FILENO, message.data, message.len);

    write(STDERR_FILENO, ": ", 2);

    const char *err = strerror(errno);

    write(STDERR_FILENO, err, lain_stringlen(err));

    write(STDERR_FILENO, "\n", 1);

    return -1;
}

#endif /* LAIN_IO_H */
