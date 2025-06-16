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

#include "config.h"
#include "purc-rwstream.h"
#include "purc-errors.h"
#include "purc-utils.h"
#include "private/errors.h"
#include "private/instance.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#if OS(UNIX)
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#endif // 0S(UNIX)

#include "rwstream_err_msgs.inc"

#define BUFFER_SIZE 4096
#define MIN_BUFFER_SIZE 32

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

static int rwstream_init_once(void)
{
    pcinst_register_error_message_segment(&_rwstream_err_msgs_seg);
    return 0;
}

struct pcmodule _module_rwstream = {
    .id              = PURC_HAVE_UTILS,
    .module_inited   = 0,

    .init_once       = rwstream_init_once,
    .init_instance   = NULL,
};

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

#if OS(LINUX) || OS(UNIX) || OS(DARWIN)
struct fd_rwstream
{
    purc_rwstream rwstream;
    int fd;
};
#endif // OS(LINUX) || OS(UNIX) || OS(DARWIN)

static off_t stdio_seek (purc_rwstream_t rws, off_t offset, int whence);
static off_t stdio_tell (purc_rwstream_t rws);
static ssize_t stdio_read (purc_rwstream_t rws, void* buf, size_t count);
static ssize_t stdio_write (purc_rwstream_t rws, const void* buf, size_t count);
static ssize_t stdio_flush (purc_rwstream_t rws);
static int stdio_destroy (purc_rwstream_t rws);

static rwstream_funcs stdio_funcs = {
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

static rwstream_funcs mem_funcs = {
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

static rwstream_funcs buffer_funcs = {
    buffer_seek,
    buffer_tell,
    buffer_read,
    buffer_write,
    buffer_flush,
    buffer_destroy,
    buffer_get_mem_buffer
};


#if OS(LINUX) || OS(UNIX) || OS(DARWIN)

static off_t fd_seek (purc_rwstream_t rws, off_t offset, int whence);
static off_t fd_tell (purc_rwstream_t rws);
static ssize_t fd_read (purc_rwstream_t rws, void* buf, size_t count);
static ssize_t fd_write (purc_rwstream_t rws, const void* buf, size_t count);
static int fd_destroy (purc_rwstream_t rws);

static rwstream_funcs fd_funcs = {
    fd_seek,
    fd_tell,
    fd_read,
    fd_write,
    NULL,           // flush
    fd_destroy,
    NULL,
};
#endif // OS(LINUX) || OS(UNIX) || OS(DARWIN)

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
    if (sz_max == 0 || sz_max < sz_init) {
        sz_max = SIZE_MAX;
    }

    if (sz_init == 0) {
        sz_init = MIN_BUFFER_SIZE;
    }


    struct buffer_rwstream* rws = (struct buffer_rwstream*) calloc(
            1, sizeof(struct buffer_rwstream));

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
            1, sizeof(struct mem_rwstream));

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
        pcinst_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
        return NULL;
    }
    return purc_rwstream_new_from_fp(fp);
}

purc_rwstream_t purc_rwstream_new_from_fp (FILE* fp)
{
    struct stdio_rwstream* rws = (struct stdio_rwstream*) calloc(
            1, sizeof(struct stdio_rwstream));

    rws->rwstream.funcs = &stdio_funcs;
    rws->fp = fp;
    return (purc_rwstream_t)rws;
}

purc_rwstream_t purc_rwstream_new_from_unix_fd (int fd)
{
#if OS(LINUX) || OS(UNIX) || OS(DARWIN)
    struct fd_rwstream* fd_rws = (struct fd_rwstream*) calloc(
            1, sizeof(struct fd_rwstream));
    if (fd_rws == NULL) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    fd_rws->rwstream.funcs = &fd_funcs;
    fd_rws->fd = fd;
    return (purc_rwstream_t)fd_rws;
#else
    UNUSED_PARAM(fd);
    pcinst_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return NULL;
#endif
}

purc_rwstream_t purc_rwstream_new_from_win32_socket (int socket, size_t sz_buf)
{
    UNUSED_PARAM(socket);
    UNUSED_PARAM(sz_buf);
    pcinst_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return NULL;
}

struct wo_rwstream
{
    purc_rwstream rwstream;
    void *ctxt;
    pcrws_cb_write cb_write;
    off_t wrotten_bytes;
};

static off_t wo_tell (purc_rwstream_t rws)
{
    struct wo_rwstream *wo_rws = (struct wo_rwstream *)rws;

    return wo_rws->wrotten_bytes;
}

static ssize_t wo_write (purc_rwstream_t rws, const void* buf, size_t count)
{
    struct wo_rwstream *wo_rws = (struct wo_rwstream *)rws;
    ssize_t bytes = wo_rws->cb_write (wo_rws->ctxt, buf, count);

    if (bytes > 0)
        wo_rws->wrotten_bytes += bytes;
    return bytes;
}

static rwstream_funcs wo_funcs = {
    NULL,
    wo_tell,
    NULL,
    wo_write,
    NULL,
    NULL,
    NULL
};

purc_rwstream_t
purc_rwstream_new_for_dump (void *ctxt, pcrws_cb_write fn)
{
    if (fn == NULL) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    struct wo_rwstream* rws = (struct wo_rwstream*) calloc(1,
            sizeof (struct wo_rwstream));

    rws->rwstream.funcs = &wo_funcs;
    rws->ctxt = ctxt;
    rws->cb_write = fn;
    rws->wrotten_bytes = 0;
    return (purc_rwstream_t)rws;
}

struct ro_rwstream
{
    purc_rwstream rwstream;
    void *ctxt;
    pcrws_cb_read cb_read;
    off_t read_bytes;
};

static off_t ro_tell (purc_rwstream_t rws)
{
    struct ro_rwstream *ro_rws = (struct ro_rwstream *)rws;

    return ro_rws->read_bytes;
}

static ssize_t ro_read (purc_rwstream_t rws, void* buf, size_t count)
{
    struct ro_rwstream *ro_rws = (struct ro_rwstream *)rws;
    ssize_t bytes = ro_rws->cb_read (ro_rws->ctxt, buf, count);

    if (bytes > 0)
        ro_rws->read_bytes += bytes;
    return bytes;
}

static rwstream_funcs ro_funcs = {
    NULL,
    ro_tell,
    ro_read,
    NULL,
    NULL,
    NULL,
    NULL
};

purc_rwstream_t
purc_rwstream_new_for_read (void *ctxt, pcrws_cb_read fn)
{
    if (fn == NULL) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    struct ro_rwstream* rws = (struct ro_rwstream*) calloc(1,
            sizeof (struct ro_rwstream));

    rws->rwstream.funcs = &ro_funcs;
    rws->ctxt = ctxt;
    rws->cb_read = fn;
    rws->read_bytes = 0;
    return (purc_rwstream_t)rws;
}

int purc_rwstream_destroy (purc_rwstream_t rws)
{
    if (rws == NULL) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    if (rws->funcs->destroy)
        return rws->funcs->destroy(rws);

    free(rws);
    return 0;
}

off_t purc_rwstream_seek (purc_rwstream_t rws, off_t offset, int whence)
{
    if (rws == NULL) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    if (rws->funcs->seek)
        return rws->funcs->seek(rws, offset, whence);

    pcinst_set_error(PURC_ERROR_NOT_SUPPORTED);
    return -1;
}

off_t purc_rwstream_tell (purc_rwstream_t rws)
{
    if (rws == NULL) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    if (rws->funcs->tell)
        return rws->funcs->tell(rws);

    pcinst_set_error(PURC_ERROR_NOT_SUPPORTED);
    return -1;
}

ssize_t purc_rwstream_read (purc_rwstream_t rws, void* buf, size_t count)
{
    if (rws == NULL) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    if (rws->funcs->read)
        return rws->funcs->read(rws, buf, count);

    pcinst_set_error(PURC_ERROR_NOT_SUPPORTED);
    return -1;
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
    if (rws == NULL) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    ssize_t ret =  purc_rwstream_read (rws, buf_utf8, 1);
    if (ret != 1) {
        return ret;
    }

    int n = 1;
    int ch_len = 0;
    uint8_t c = buf_utf8[0];
    if (c > 0xFD) {
        pcinst_set_error(PCRWSTREAM_ERROR_IO);
        return -1;
    }

    if (c & 0x80) {
        while (c & (0x80 >> n))
            n++;

        if (n < 2) {
            pcinst_set_error(PURC_ERROR_BAD_ENCODING);
            return -1;
        }
        ch_len = n;
    }
    else {
        ch_len = 1;
    }

    int read_len = ch_len - 1;
    char* p = buf_utf8 + 1;
    while (read_len > 0) {
        ret =  purc_rwstream_read (rws, p, 1);
        if (ret != 1)
        {
            pcinst_set_error(PCRWSTREAM_ERROR_IO);
            return -1;
        }
        c = *p;
        if ((c & 0xC0) != 0x80) {
            pcinst_set_error(PCRWSTREAM_ERROR_IO);
            return -1;
        }
        p++;
        read_len--;
    }

    // FIXME
    if (ch_len > 3) {
        pcinst_set_error(PURC_ERROR_BAD_ENCODING);
        return -1;
    }

    uint32_t uc = -1;
    size_t nr_chars;
    if (buf_utf8[0] == 0) {
        uc = 0;
    }
    else if(pcutils_string_check_utf8_len(buf_utf8, ch_len, &nr_chars, NULL)) {
        uc = utf8_to_uint32_t((const unsigned char*)buf_utf8, ch_len);
    }
    else {
        ch_len = -1;
        pcinst_set_error(PURC_ERROR_BAD_ENCODING);
    }

    if (buf_wc)
        *buf_wc = uc;

    return ch_len;
}

ssize_t purc_rwstream_write (purc_rwstream_t rws, const void* buf, size_t count)
{
    if (rws == NULL) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    if (rws->funcs->write)
        return rws->funcs->write(rws, buf, count);

    pcinst_set_error(PURC_ERROR_NOT_SUPPORTED);
    return -1;
}

ssize_t purc_rwstream_flush (purc_rwstream_t rws)
{
    if (rws == NULL) {
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    if (rws->funcs->flush)
        return rws->funcs->flush(rws);

    pcinst_set_error(PURC_ERROR_NOT_SUPPORTED);
    return -1;
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

            if (read_len == 0) {
                break;
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
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    if (rws->funcs->get_mem_buffer == NULL) {
        pcinst_set_error(PURC_ERROR_NOT_SUPPORTED);
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
    pcinst_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
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
        pcinst_set_error(PCRWSTREAM_ERROR_IO);
    }
    return(nread);

}

static ssize_t stdio_write (purc_rwstream_t rws, const void* buf, size_t count)
{
    struct stdio_rwstream* stdio = (struct stdio_rwstream *)rws;
    ssize_t nwrote = fwrite(buf, 1, count, stdio->fp);
    if ( nwrote == 0 && ferror(stdio->fp) ) {
        pcinst_set_error(PCRWSTREAM_ERROR_IO);
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
    pcinst_set_error(PCRWSTREAM_ERROR_NO_SPACE);
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
        pcinst_set_error(PCRWSTREAM_ERROR_IO);
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
                pcinst_set_error(PCRWSTREAM_ERROR_NO_SPACE);
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
        *buffer->here = 0;
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

#if OS(LINUX) || OS(UNIX) || OS(DARWIN)

static off_t fd_seek (purc_rwstream_t rws, off_t offset, int whence)
{
    struct fd_rwstream* fd_rws = (struct fd_rwstream *)rws;
    off_t ret = lseek(fd_rws->fd, offset, whence);
    if (ret == -1) {
        purc_set_error(purc_error_from_errno(errno));
    }
    return ret;
}

static off_t fd_tell (purc_rwstream_t rws)
{
    struct fd_rwstream* fd_rws = (struct fd_rwstream *)rws;
    off_t ret = lseek(fd_rws->fd, 0, SEEK_CUR);
    if (ret == -1) {
        purc_set_error(purc_error_from_errno(errno));
    }
    return ret;
}

static ssize_t fd_read (purc_rwstream_t rws, void* buf, size_t count)
{
    struct fd_rwstream* fd_rws = (struct fd_rwstream *)rws;
    ssize_t ret = read(fd_rws->fd, buf, count);
    if (ret == -1) {
        purc_set_error(purc_error_from_errno(errno));
    }
    return ret;
}

static ssize_t fd_write (purc_rwstream_t rws, const void* buf, size_t count)
{
    struct fd_rwstream* fd_rws = (struct fd_rwstream *)rws;
    ssize_t ret = write(fd_rws->fd, buf, count);
    if (ret == -1) {
        purc_set_error(purc_error_from_errno(errno));
    }
    return ret;
}

static int fd_destroy (purc_rwstream_t rws)
{
    free(rws);
    return 0;
}

#endif // OS(LINUX) || OS(UNIX) || OS(DARWIN)
