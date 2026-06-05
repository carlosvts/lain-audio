#ifndef LAIN_FS_H
#define LAIN_FS_H

#include "lain.h"
#include "lain_io.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static inline i32 lain_open(String path)
{
    i32 fd = open(path.data, O_RDONLY);

    if (fd == -1)
    {
        lain_perror(path);
        return -1;
    }

    return fd;
}

static inline i32 lain_create(String path)
{
    i32 fd = open(path.data, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1)
    {
        lain_perror(path);
        return -1;
    }

    return fd;
}

static inline isize lain_read(i32 fd, void *buffer, usize size)
{
    isize bytes = read(fd, buffer, size);

    if (bytes == -1)
    {
        lain_perror(lain_string("read"));
        return -1;
    }

    return bytes;
}

static inline isize lain_write(i32 fd, const void *buffer, usize size)
{
    isize bytes = write(fd, buffer, size);

    if (bytes == -1)
    {
        lain_perror(lain_string("write"));
        return -1;
    }

    return bytes;
}

static inline i32 lain_close(i32 fd)
{
    i32 result = close(fd);

    if (result == -1)
    {
        lain_perror(lain_string("close"));
        return -1;
    }

    return result;
}

static inline i32 lain_mkdir(String path, mode_t mode)
{
    i32 result = mkdir(path.data, mode);

    if (result == -1)
    {
        lain_perror(path);
        return -1;
    }

    return result;
}

static inline bool lain_exists(String path)
{
    struct stat st;

    return stat(path.data, &st) == 0;
}

static inline bool lain_is_dir(String path)
{
    struct stat st;

    if (stat(path.data, &st) != 0)
    {
        return false;
    }

    return S_ISDIR(st.st_mode);
}

#endif /* LAIN_FS_H */
