/*
 * @file rwstream.c
 * @author XueShuming
 * @date 2021/07/02
 * @brief The API for RWStream.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "purc-rwstream.h"
#include "purc-errors.h"
#include "purc-utils.h"
#include "private/errors.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#if ENABLE(SOCKET_STREAM) && HAVE(GLIB)
#if OS(UNIX)
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#endif // 0S(UNIX)
#include <glib.h>
#endif // ENABLE(SOCKET_STREAM) && HAVE(GLIB)

#include "rwstream_err_msgs.inc"

#define BUFFER_SIZE 4096
#define MIN_BUFFER_SIZE 32

#if 1
#define RWSTREAM_SET_ERROR(err) pcinst_set_error(err)
#else
#define RWSTREAM_SET_ERROR(err) do { \
    fprintf(stderr, "error %s:%d\n", __FILE__, __LINE__); \
    pcinst_set_error (err); \
} while (0)
#endif

/* Make sure the number of error messages matches the number of error codes */
#define _COMPILE_TIME_ASSERT(name, x)               \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(msgs,
        PCA_TABLESIZE(rwstream_err_msgs) == PCRWSTREAM_ERROR_NR);

#undef _COMPILE_TIME_ASSERT

static struct err_msg_seg _rwstream_err_msgs_seg = {
    { NULL, NULL },
    PURC_ERROR_FIRST_RWSTREAM,
    PURC_ERROR_FIRST_RWSTREAM + PCA_TABLESIZE(rwstream_err_msgs) - 1,
    rwstream_err_msgs
};

void pcrwstream_init_once(void)
{
    pcinst_register_error_message_segment(&_rwstream_err_msgs_seg);
}

typedef struct rwstream_funcs
{
    off_t   (*seek) (purc_rwstream_t rws, off_t offset, int whence);
    off_t   (*tell) (purc_rwstream_t rws);
    ssize_t (*read) (purc_rwstream_t rws, void* buf, size_t count);
    ssize_t (*write) (purc_rwstream_t rws, const void* buf, size_t count);
    ssize_t (*flush) (purc_rwstream_t rws);
    int     (*destroy) (purc_rwstream_t rws);
    void*   (*get_mem_buffer) (purc_rwstream_t rws, size_t *sz_content,
            size_t *sz_buffer, bool res_buff);
} rwstream_funcs;

struct purc_rwstream
{
    rwstream_funcs* funcs;
};

struct stdio_rwstream
{
    purc_rwstream rwstream;
    FILE* fp;
};

struct mem_rwstream
{
    purc_rwstream rwstream;
    uint8_t* base;
    uint8_t* here;
    uint8_t* stop;
};

struct buffer_rwstream
{
    purc_rwstream rwstream;
    uint8_t* base;
    uint8_t* here;
    uint8_t* stop;
    uint8_t* end;
    size_t sz;
    size_t sz_max;

    bool buff_reserved;
};

#if ENABLE(SOCKET_STREAM) && HAVE(GLIB)
struct gio_rwstream
{
    purc_rwstream rwstream;
    GIOChannel* gio_channel;
    int fd;
};
#endif // ENABLE(SOCKET_STREAM) && HAVE(GLIB)

static off_t stdio_seek (purc_rwstream_t rws, off_t offset, int whence);
static off_t stdio_tell (purc_rwstream_t rws);
static ssize_t stdio_read (purc_rwstream_t rws, void* buf, size_t count);
static ssize_t stdio_write (purc_rwstream_t rws, const void* buf, size_t count);
static ssize_t stdio_flush (purc_rwstream_t rws);
static int stdio_destroy (purc_rwstream_t rws);

rwstream_funcs stdio_funcs = {
    stdio_seek,
    stdio_tell,
    stdio_read,
    stdio_write,
    stdio_flush,
    stdio_destroy,
    NULL
};

static off_t mem_seek (purc_rwstream_t rws, off_t offset, int whence);
static off_t mem_tell (purc_rwstream_t rws);
static ssize_t mem_read (purc_rwstream_t rws, void* buf, size_t count);
static ssize_t mem_write (purc_rwstream_t rws, const void* buf, size_t count);
static ssize_t mem_flush (purc_rwstream_t rws);
static int mem_destroy (purc_rwstream_t rws);
static void* mem_get_mem_buffer (purc_rwstream_t rws,
        size_t *sz_content, size_t *sz_buffer, bool res_buff);

rwstream_funcs mem_funcs = {
    mem_seek,
    mem_tell,
    mem_read,
    mem_write,
    mem_flush,
    mem_destroy,
    mem_get_mem_buffer
};

static off_t buffer_seek (purc_rwstream_t rws, off_t offset, int whence);
static off_t buffer_tell (purc_rwstream_t rws);
static ssize_t buffer_read (purc_rwstream_t rws, void* buf, size_t count);
static ssize_t buffer_write (purc_rwstream_t rws, const void* buf, size_t count);
static ssize_t buffer_flush (purc_rwstream_t rws);
static int buffer_destroy (purc_rwstream_t rws);
static void* buffer_get_mem_buffer (purc_rwstream_t rws,
        size_t *sz_content, size_t *sz_buffer, bool res_buff);

rwstream_funcs buffer_funcs = {
    buffer_seek,
    buffer_tell,
    buffer_read,
    buffer_write,
    buffer_flush,
    buffer_destroy,
    buffer_get_mem_buffer
};


#if ENABLE(SOCKET_STREAM) && HAVE(GLIB)
static off_t win_socket_seek (purc_rwstream_t rws, off_t offset, int whence);
static off_t gio_seek (purc_rwstream_t rws, off_t offset, int whence);
static off_t gio_tell (purc_rwstream_t rws);
static ssize_t gio_read (purc_rwstream_t rws, void* buf, size_t count);
static ssize_t gio_write (purc_rwstream_t rws, const void* buf, size_t count);
static ssize_t gio_flush (purc_rwstream_t rws);
static int gio_destroy (purc_rwstream_t rws);

rwstream_funcs gio_funcs = {
    gio_seek,
    gio_tell,
    gio_read,
    gio_write,
    gio_flush,
    gio_destroy,
    NULL,
};

rwstream_funcs win_socket_funcs = {
    win_socket_seek,
    gio_tell,
    gio_read,
    gio_write,
    gio_flush,
    gio_destroy,
    NULL,
};

int rwstream_error_code_from_gerror (GError* err)
{
    if (err == NULL)
    {
        return PURC_ERROR_OK;
    }

    switch (err->code)
    {
        case G_IO_CHANNEL_ERROR_FBIG:
            return PCRWSTREAM_ERROR_FILE_TOO_BIG;
        case G_IO_CHANNEL_ERROR_INVAL:
            return PURC_ERROR_INVALID_VALUE;
        case G_IO_CHANNEL_ERROR_IO:
            return PCRWSTREAM_ERROR_IO;
        case G_IO_CHANNEL_ERROR_ISDIR:
            return PCRWSTREAM_ERROR_IS_DIR;
        case G_IO_CHANNEL_ERROR_NOSPC:
            return PCRWSTREAM_ERROR_NO_SPACE;
        case G_IO_CHANNEL_ERROR_NXIO:
            return PCRWSTREAM_ERROR_NO_DEVICE_OR_ADDRESS;
        case G_IO_CHANNEL_ERROR_OVERFLOW:
            return PCRWSTREAM_ERROR_OVERFLOW;
        case G_IO_CHANNEL_ERROR_PIPE:
            return PCRWSTREAM_ERROR_PIPE;
        case G_IO_CHANNEL_ERROR_FAILED:
            return PCRWSTREAM_ERROR_FAILED;
        default:
            return PCRWSTREAM_ERROR_FAILED;
    }
}
#endif // ENABLE(SOCKET_STREAM) && HAVE(GLIB)


static size_t get_min_size(size_t sz_min, size_t sz_max) {
    size_t min = pcutils_get_next_fibonacci_number(sz_min);
    if (min < MIN_BUFFER_SIZE) {
        min = MIN_BUFFER_SIZE;
    } else if (min > sz_max) {
        min = sz_max;
    }
    return min;
}

/* rwstream api */
purc_rwstream_t purc_rwstream_new_buffer (size_t sz_init, size_t sz_max)
{
    if (sz_init == 0 || sz_max <= sz_init)
    {
        RWSTREAM_SET_ERROR(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    struct buffer_rwstream* rws = (struct buffer_rwstream*) calloc(
            sizeof(struct buffer_rwstream), 1);

    sz_max = sz_max < sz_init ? sz_init : sz_max;

    size_t sz = get_min_size(sz_init, sz_max);

    rws->rwstream.funcs = &buffer_funcs;
    rws->base = (uint8_t*) calloc(sz + 1, 1);
    rws->here = rws->base;
    rws->stop = rws->here;
    rws->end = rws->base + sz;
    rws->sz = sz;
    rws->sz_max = sz_max;

    rws->buff_reserved = false;

    return (purc_rwstream_t)rws;
}

purc_rwstream_t purc_rwstream_new_from_mem (void* mem, size_t sz)
{
    struct mem_rwstream* rws = (struct mem_rwstream*) calloc(
            sizeof(struct mem_rwstream), 1);

    rws->rwstream.funcs = &mem_funcs;
    rws->base = mem;
    rws->here = rws->base;
    rws->stop = rws->base + sz;

    return (purc_rwstream_t)rws;
}

purc_rwstream_t purc_rwstream_new_from_file (const char* file, const char* mode)
{
    FILE* fp = fopen(file, mode);
    if (fp == NULL)
    {
        RWSTREAM_SET_ERROR(PURC_ERROR_BAD_SYSTEM_CALL);
        return NULL;
    }
    return purc_rwstream_new_from_fp(fp);
}

purc_rwstream_t purc_rwstream_new_from_fp (FILE* fp)
{
    struct stdio_rwstream* rws = (struct stdio_rwstream*) calloc(
            sizeof(struct stdio_rwstream), 1);

    rws->rwstream.funcs = &stdio_funcs;
    rws->fp = fp;
    return (purc_rwstream_t)rws;
}

#if ENABLE(SOCKET_STREAM) && HAVE(GLIB)
purc_rwstream_t purc_rwstream_new_from_unix_fd (int fd, size_t sz_buf)
{
    GIOChannel* gio_channel = g_io_channel_unix_new(fd);
    if (gio_channel == NULL)
    {
        RWSTREAM_SET_ERROR(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    GIOFlags flags = g_io_channel_get_flags(gio_channel);
    g_io_channel_set_flags(gio_channel, flags & ~G_IO_FLAG_NONBLOCK, NULL);

    g_io_channel_set_encoding (gio_channel, NULL, NULL);

    if (sz_buf > 0)
    {
        g_io_channel_set_buffer_size(gio_channel, sz_buf);
    }

    struct gio_rwstream* gio = (struct gio_rwstream*) calloc(
            sizeof(struct gio_rwstream), 1);
    gio->rwstream.funcs = &gio_funcs;
    gio->gio_channel = gio_channel;
    gio->fd = fd;
    return (purc_rwstream_t) gio;
}
#else
purc_rwstream_t purc_rwstream_new_from_unix_fd (int fd, size_t sz_buf)
{
    UNUSED_PARAM(fd);
    UNUSED_PARAM(sz_buf);
    RWSTREAM_SET_ERROR(PURC_ERROR_NOT_IMPLEMENTED);
    return NULL;
}
#endif // ENABLE(SOCKET_STREAM) && HAVE(GLIB)

#if ENABLE(SOCKET_STREAM) && HAVE(GLIB) && OS(WINDOWS) && defined(G_OS_WIN32)
purc_rwstream_t purc_rwstream_new_from_win32_socket (int socket, size_t sz_buf)
{
    GIOChannel* gio_channel = g_io_channel_win32_new_socket(socket);
    if (gio_channel == NULL)
    {
        RWSTREAM_SET_ERROR(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    GIOFlags flags = g_io_channel_get_flags(gio_channel);
    g_io_channel_set_flags(gio_channel, flags & ~G_IO_FLAG_NONBLOCK, NULL);

    g_io_channel_set_encoding (gio_channel, NULL, NULL);

    if (sz_buf > 0)
    {
        g_io_channel_set_buffer_size(gio_channel, sz_buf);
    }

    struct gio_rwstream* gio = (struct gio_rwstream*) calloc(
            sizeof(struct gio_rwstream), 1);
    gio->rwstream.funcs = &win_socket_funcs;
    gio->gio_channel = gio_channel;
    return (purc_rwstream_t)rws;
}
#else
purc_rwstream_t purc_rwstream_new_from_win32_socket (int socket, size_t sz_buf)
{
    UNUSED_PARAM(socket);
    UNUSED_PARAM(sz_buf);
    RWSTREAM_SET_ERROR(PURC_ERROR_NOT_IMPLEMENTED);
    return NULL;
}
#endif // ENABLE(SOCKET_STREAM) && HAVE(GLIB) && OS(WINDOWS) && defined(G_OS_WIN32)

int purc_rwstream_destroy (purc_rwstream_t rws)
{
    if (rws == NULL)
    {
        RWSTREAM_SET_ERROR(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    return rws->funcs->destroy(rws);
}

off_t purc_rwstream_seek (purc_rwstream_t rws, off_t offset, int whence)
{
    if (rws == NULL)
    {
        RWSTREAM_SET_ERROR(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    return rws->funcs->seek(rws, offset, whence);
}

off_t purc_rwstream_tell (purc_rwstream_t rws)
{
    if (rws == NULL)
    {
        RWSTREAM_SET_ERROR(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    return rws->funcs->tell(rws);
}

ssize_t purc_rwstream_read (purc_rwstream_t rws, void* buf, size_t count)
{
    if (rws == NULL)
    {
        RWSTREAM_SET_ERROR(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    return rws->funcs->read(rws, buf, count);
}

static uint32_t utf8_to_uint32_t (const unsigned char* utf8_char,
        int utf8_char_len)
{
    uint32_t wc = *((unsigned char *)(utf8_char++));
    int n = utf8_char_len;
    int t = 0;

    if (wc & 0x80) {
        wc &= (1 << (8-n)) - 1;
        while (--n > 0) {
            t = *((unsigned char *)(utf8_char++));
            wc = (wc << 6) | (t & 0x3F);
        }
    }

    return wc;
}

int purc_rwstream_read_utf8_char (purc_rwstream_t rws, char* buf_utf8,
        uint32_t* buf_wc)
{
    if (rws == NULL)
    {
        RWSTREAM_SET_ERROR(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    ssize_t ret =  purc_rwstream_read (rws, buf_utf8, 1);
    if (ret != 1)
    {
        return ret;
    }

    int n = 1;
    int ch_len = 0;
    uint8_t c = buf_utf8[0];
    if (c > 0xFD) {
        RWSTREAM_SET_ERROR(PCRWSTREAM_ERROR_IO);
        return -1;
    }

    if (c & 0x80)
    {
        while (c & (0x80 >> n))
            n++;

        if (n < 2) {
            RWSTREAM_SET_ERROR(PURC_ERROR_BAD_ENCODING);
            return -1;
        }
        ch_len = n;
    }
    else
    {
        ch_len = 1;
    }

    int read_len = ch_len - 1;
    char* p = buf_utf8 + 1;
    while (read_len > 0) {
        ret =  purc_rwstream_read (rws, p, 1);
        if (ret != 1)
        {
            RWSTREAM_SET_ERROR(PCRWSTREAM_ERROR_IO);
            return -1;
        }
        c = *p;
        if ((c & 0xC0) != 0x80) {
            RWSTREAM_SET_ERROR(PCRWSTREAM_ERROR_IO);
            return -1;
        }
        p++;
        read_len--;
    }

    // FIXME
    if (ch_len > 3) {
        RWSTREAM_SET_ERROR(PURC_ERROR_BAD_ENCODING);
        return -1;
    }

    size_t nr_chars;
    if (buf_utf8[0] == 0) {
        *buf_wc = 0;
    }
    else if(pcutils_string_check_utf8_len(buf_utf8, ch_len, &nr_chars, NULL)) {
        *buf_wc = utf8_to_uint32_t((const unsigned char*)buf_utf8, ch_len);
    }
    else {
        ch_len = -1;
        RWSTREAM_SET_ERROR(PURC_ERROR_BAD_ENCODING);
    }
    return ch_len;
}

ssize_t purc_rwstream_write (purc_rwstream_t rws, const void* buf, size_t count)
{
    if (rws == NULL)
    {
        RWSTREAM_SET_ERROR(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    return rws->funcs->write(rws, buf, count);
}

ssize_t purc_rwstream_flush (purc_rwstream_t rws)
{
    if (rws == NULL)
    {
        RWSTREAM_SET_ERROR(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    return rws->funcs->flush(rws);
}

ssize_t purc_rwstream_dump_to_another (purc_rwstream_t in,
        purc_rwstream_t out, ssize_t count)
{
    char buffer[BUFFER_SIZE] = {0};
    ssize_t ret_count = 0;
    ssize_t write_len = 0;
    int read_len = 0;

    size_t read_size = 0;

    if (count == -1)
    {
        while ((read_len = purc_rwstream_read(in, buffer, BUFFER_SIZE)) > 0)
        {
            write_len = purc_rwstream_write (out, buffer, read_len);
            if (write_len != read_len) {
                return -1;
            }
            ret_count += read_len;
        }
    }
    else
    {
        read_size = count > BUFFER_SIZE ? BUFFER_SIZE : count;
        while (read_size > 0 )
        {
            read_len = purc_rwstream_read(in, buffer, read_size);
            if (read_len == -1)
            {
                return -1;
            }

            write_len = purc_rwstream_write (out, buffer, read_len);
            if (write_len != read_len) {
                return -1;
            }

            ret_count += read_len;
            count = count - write_len;
            read_size = count > BUFFER_SIZE ? BUFFER_SIZE : count;
        }
    }

    return ret_count;
}

void* purc_rwstream_get_mem_buffer_ex (purc_rwstream_t rws,
        size_t *sz_content, size_t *sz_buffer, bool res_buff)
{
    if (rws == NULL) {
        RWSTREAM_SET_ERROR(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    if (rws->funcs->get_mem_buffer == NULL) {
        RWSTREAM_SET_ERROR(PURC_ERROR_NOT_SUPPORTED);
        return NULL;
    }

    return rws->funcs->get_mem_buffer(rws, sz_content, sz_buffer, res_buff);
}

/* stdio rwstream functions */
static off_t stdio_seek (purc_rwstream_t rws, off_t offset, int whence)
{
    struct stdio_rwstream* stdio = (struct stdio_rwstream *)rws;
    if ( fseek(stdio->fp, offset, whence) == 0 )
    {
        return ftell(stdio->fp);
    }
    RWSTREAM_SET_ERROR(PURC_ERROR_BAD_SYSTEM_CALL);
    return -1;
}

static off_t stdio_tell (purc_rwstream_t rws)
{
    return ftell(((struct stdio_rwstream *)rws)->fp);
}

static ssize_t stdio_read (purc_rwstream_t rws, void* buf, size_t count)
{
    struct stdio_rwstream* stdio = (struct stdio_rwstream *)rws;
    ssize_t nread = fread(buf, 1, count, stdio->fp);
    if ( nread == 0 && ferror(stdio->fp) ) {
        RWSTREAM_SET_ERROR(PCRWSTREAM_ERROR_IO);
    }
    return(nread);

}

static ssize_t stdio_write (purc_rwstream_t rws, const void* buf, size_t count)
{
    struct stdio_rwstream* stdio = (struct stdio_rwstream *)rws;
    ssize_t nwrote = fwrite(buf, 1, count, stdio->fp);
    if ( nwrote == 0 && ferror(stdio->fp) ) {
        RWSTREAM_SET_ERROR(PCRWSTREAM_ERROR_IO);
    }
    return(nwrote);

}

static ssize_t stdio_flush (purc_rwstream_t rws)
{
    return fflush(((struct stdio_rwstream *)rws)->fp);
}

static int stdio_destroy (purc_rwstream_t rws)
{
    if (((struct stdio_rwstream *)rws)->fp)
    {
        fclose(((struct stdio_rwstream *)rws)->fp);
    }
    free(rws);
    return 0;
}

/* memory rwstream functions */
static off_t mem_seek (purc_rwstream_t rws, off_t offset, int whence)
{
    struct mem_rwstream* mem = (struct mem_rwstream *)rws;
    uint8_t* newpos;

    switch (whence) {
        case SEEK_SET:
            newpos = mem->base + offset;
            break;
        case SEEK_CUR:
            newpos = mem->here + offset;
            break;
        case SEEK_END:
            newpos = mem->stop + offset;
            break;
        default:
            return(-1);
    }
    if ( newpos < mem->base ) {
        newpos = mem->base;
    }
    if ( newpos > mem->stop ) {
        newpos = mem->stop;
    }
    mem->here = newpos;
    return(mem->here - mem->base);
}

static off_t mem_tell (purc_rwstream_t rws)
{
    struct mem_rwstream* mem = (struct mem_rwstream *)rws;
    return (mem->here - mem->base);
}

static ssize_t mem_read (purc_rwstream_t rws, void* buf, size_t count)
{
    struct mem_rwstream* mem = (struct mem_rwstream *)rws;
    if ( (mem->here + count) > mem->stop )
    {
        count = mem->stop - mem->here;
    }
    memcpy(buf, mem->here, count);
    mem->here += count;
    return count;
}

static ssize_t mem_write (purc_rwstream_t rws, const void* buf, size_t count)
{
    struct mem_rwstream* mem = (struct mem_rwstream *)rws;
    if ( (mem->here + count) > mem->stop ) {
        count = mem->stop - mem->here;
    }
    if (count > 0)
    {
        memcpy(mem->here, buf, count);
        mem->here += count;
        return count;
    }
    RWSTREAM_SET_ERROR(PCRWSTREAM_ERROR_NO_SPACE);
    return -1;
}

static ssize_t mem_flush (purc_rwstream_t rws)
{
    UNUSED_PARAM(rws);
    return 0;
}

static int mem_destroy (purc_rwstream_t rws)
{
    struct mem_rwstream* mem = (struct mem_rwstream *)rws;
    mem->base = NULL;
    mem->here = NULL;
    mem->stop = NULL;
    free(rws);
    return 0;
}

static void* mem_get_mem_buffer (purc_rwstream_t rws,
        size_t *sz_content, size_t *sz_buffer, bool res_buff)
{
    struct mem_rwstream* mem = (struct mem_rwstream *)rws;

    if (sz_content) {
        *sz_content = mem->stop - mem->base;
    }

    if (sz_buffer) {
        *sz_content = mem->stop - mem->base;
    }

    UNUSED_PARAM(res_buff);
    return mem->base;
}

/* buffer rwstream functions */
static int buffer_extend (struct buffer_rwstream* buffer, size_t size)
{
    if (buffer->sz > size || buffer->sz == buffer->sz_max) {
        return 0;
    }

    size_t new_size = get_min_size(size, buffer->sz_max);
    off_t here_offset = buffer->here - buffer->base;
    off_t stop_offset = buffer->stop - buffer->base;

    uint8_t* newbuf = (uint8_t*) realloc(buffer->base, new_size + 1);
    if (newbuf == NULL)
    {
        RWSTREAM_SET_ERROR(PCRWSTREAM_ERROR_IO);
        return -1;
    }

    buffer->base = newbuf;
    buffer->here = buffer->base + here_offset;
    buffer->stop = buffer->base + stop_offset;
    buffer->end = buffer->base + new_size;
    buffer->sz = new_size;
    *buffer->here = 0;

    return 0;
}

static off_t buffer_seek (purc_rwstream_t rws, off_t offset, int whence)
{
    struct buffer_rwstream* buffer = (struct buffer_rwstream *)rws;
    uint8_t* newpos;

    switch (whence) {
        case SEEK_SET:
            newpos = buffer->base + offset;
            break;
        case SEEK_CUR:
            newpos = buffer->here + offset;
            break;
        case SEEK_END:
            newpos = buffer->stop + offset;
            break;
        default:
            return(-1);
    }
    if ( newpos < buffer->base ) {
        newpos = buffer->base;
    }

    if ( newpos > buffer->stop ) {
        newpos = buffer->stop;
    }
    buffer->here = newpos;
    return(buffer->here - buffer->base);
}

static off_t buffer_tell (purc_rwstream_t rws)
{
    struct buffer_rwstream* buffer = (struct buffer_rwstream *)rws;
    return (buffer->here - buffer->base);
}

static ssize_t buffer_read (purc_rwstream_t rws, void* buf, size_t count)
{
    struct buffer_rwstream* buffer = (struct buffer_rwstream *)rws;
    if ( (buffer->here + count) > buffer->stop )
    {
        count = buffer->stop - buffer->here;
    }
    memcpy(buf, buffer->here, count);
    buffer->here += count;
    return count;
}

static ssize_t buffer_write (purc_rwstream_t rws, const void* buf, size_t count)
{
    struct buffer_rwstream* buffer = (struct buffer_rwstream *)rws;
    uint8_t* newpos = buffer->here + count;
    if ( newpos > buffer->stop ) {
        if (newpos <= buffer->end) {
            buffer->stop = newpos;
        }
        else if (buffer->sz < buffer->sz_max) {
            int ret = buffer_extend (buffer, newpos - buffer->base);
            if (ret == -1) {
                RWSTREAM_SET_ERROR(PCRWSTREAM_ERROR_NO_SPACE);
                return -1;
            }
            newpos = buffer->here + count;
            if(newpos > buffer->end) {
                buffer->stop = buffer->end;
                count = buffer->end - buffer->here;
            }
            else {
                buffer->stop = newpos;
            }
        }
        else {
            buffer->stop = buffer->end;
            count = buffer->end - buffer->here;
        }
    }
    if (count > 0)
    {
        memcpy(buffer->here, buf, count);
        buffer->here += count;
        return count;
    }
    return 0;
}

static ssize_t buffer_flush (purc_rwstream_t rws)
{
    UNUSED_PARAM(rws);
    return 0;
}

static int buffer_destroy (purc_rwstream_t rws)
{
    struct buffer_rwstream* buffer = (struct buffer_rwstream *)rws;
    if (buffer->base && !buffer->buff_reserved) {
        free(buffer->base);
    }
    free(rws);
    return 0;
}

static void* buffer_get_mem_buffer (purc_rwstream_t rws,
        size_t *sz_content, size_t *sz_buffer, bool res_buff)
{
    struct buffer_rwstream* buffer = (struct buffer_rwstream *)rws;

    if (sz_content) {
        *sz_content = buffer->stop - buffer->base;
    }

    if (sz_buffer) {
        *sz_buffer = buffer->end - buffer->base;
    }

    buffer->buff_reserved = res_buff;

    return buffer->base;
}

#if ENABLE(SOCKET_STREAM) && HAVE(GLIB)
/* glib rwstream functions */
static off_t win_socket_seek (purc_rwstream_t rws, off_t offset, int whence)
{
    UNUSED_PARAM(rws);
    UNUSED_PARAM(offset);
    UNUSED_PARAM(whence);
    RWSTREAM_SET_ERROR(PURC_ERROR_NOT_IMPLEMENTED);
    return -1;
}

static off_t gio_seek (purc_rwstream_t rws, off_t offset, int whence)
{
#if OS(UNIX)
    struct gio_rwstream* gio = (struct gio_rwstream *)rws;
    GSeekType type;
    switch (whence) {
        case SEEK_SET:
            type = G_SEEK_SET;
            break;
        case SEEK_CUR:
            type = G_SEEK_CUR;
            break;
        case SEEK_END:
            type = G_SEEK_END;
            break;
        default:
            RWSTREAM_SET_ERROR(PURC_ERROR_INVALID_VALUE);
            return(-1);
    }

    GError* err = NULL;
    GIOStatus ios = g_io_channel_seek_position (gio->gio_channel, offset,
            type, &err);
    if (ios == G_IO_STATUS_NORMAL)
    {
        return lseek(gio->fd, 0, SEEK_CUR);
    }
    RWSTREAM_SET_ERROR(rwstream_error_code_from_gerror(err));

    if (err)
        g_error_free(err);
    return -1;
#else
    UNUSED_PARAM(rws);
    UNUSED_PARAM(offset);
    UNUSED_PARAM(whence);
    RWSTREAM_SET_ERROR(PURC_ERROR_NOT_IMPLEMENTED);
    return -1;
#endif // OS(UNIX)
}

static off_t gio_tell (purc_rwstream_t rws)
{
    UNUSED_PARAM(rws);
    RWSTREAM_SET_ERROR(PURC_ERROR_NOT_IMPLEMENTED);
    return -1;
}

static ssize_t gio_read (purc_rwstream_t rws, void* buf, size_t count)
{
    struct gio_rwstream* gio = (struct gio_rwstream *)rws;
    gsize read = 0;
    GError* err = NULL;
    g_io_channel_read_chars (gio->gio_channel, buf, count, &read, &err);
    RWSTREAM_SET_ERROR(rwstream_error_code_from_gerror(err));
    if (err)
        g_error_free(err);
    return read;
}

static ssize_t gio_write (purc_rwstream_t rws, const void* buf, size_t count)
{
    struct gio_rwstream* gio = (struct gio_rwstream *)rws;
    gsize write = 0;
    GError* err = NULL;
    g_io_channel_write_chars (gio->gio_channel, buf, count, &write, &err);
    RWSTREAM_SET_ERROR(rwstream_error_code_from_gerror(err));
    if (err)
        g_error_free(err);
    return write;
}

static ssize_t gio_flush (purc_rwstream_t rws)
{
    struct gio_rwstream* gio = (struct gio_rwstream *)rws;
    GError* err = NULL;
    GIOStatus ios = g_io_channel_flush (gio->gio_channel, &err);
    if (ios == G_IO_STATUS_NORMAL)
    {
        return 0;
    }
    if (err)
        g_error_free(err);
    return -1;
}

static int gio_destroy (purc_rwstream_t rws)
{
    struct gio_rwstream* gio = (struct gio_rwstream *)rws;
    if (gio->gio_channel)
    {
        g_io_channel_shutdown(gio->gio_channel, TRUE, NULL);
        g_io_channel_unref(gio->gio_channel);
    }
    free(rws);
    return 0;
}

#endif // ENABLE(SOCKET_STREAM)
