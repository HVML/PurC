/*
** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML parser
** and interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
**
*/


#include "rwstream.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef PURC_BUILD_WITH_GLIB
#include <glib.h>
#endif // PURC_BUILD_WITH_GLIB

struct purc_rwstream;
typedef struct purc_rwstream purc_rwstream;
typedef struct purc_rwstream* purc_rwstream_t;

typedef struct rwstream_funcs
{
    off_t   (*seek) (purc_rwstream_t rws, off_t offset, int whence);
    off_t   (*tell) (purc_rwstream_t rws);
    int     (*eof) (purc_rwstream_t rws);
    ssize_t (*read) (purc_rwstream_t rws, void* buf, size_t count);
    ssize_t (*write) (purc_rwstream_t rws, const void* buf, size_t count);
    ssize_t (*flush) (purc_rwstream_t rws);
    int     (*close) (purc_rwstream_t rws);
    int     (*destroy) (purc_rwstream_t rws);
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

#ifdef PURC_BUILD_WITH_GLIB
struct gio_rwstream
{
    purc_rwstream rwstream;
    GIOChannel* gio_channel;
    int fd;
};
#endif // PURC_BUILD_WITH_GLIB

static off_t stdio_seek (purc_rwstream_t rws, off_t offset, int whence);
static off_t stdio_tell (purc_rwstream_t rws);
static int stdio_eof (purc_rwstream_t rws);
static ssize_t stdio_read (purc_rwstream_t rws, void* buf, size_t count);
static ssize_t stdio_write (purc_rwstream_t rws, const void* buf, size_t count);
static ssize_t stdio_flush (purc_rwstream_t rws);
static int stdio_close (purc_rwstream_t rws);
static int stdio_destroy (purc_rwstream_t rws);

rwstream_funcs stdio_funcs = {
    stdio_seek,
    stdio_tell,
    stdio_eof,
    stdio_read,
    stdio_write,
    stdio_flush,
    stdio_close,
    stdio_destroy
};

static off_t mem_seek (purc_rwstream_t rws, off_t offset, int whence);
static off_t mem_tell (purc_rwstream_t rws);
static int mem_eof (purc_rwstream_t rws);
static ssize_t mem_read (purc_rwstream_t rws, void* buf, size_t count);
static ssize_t mem_write (purc_rwstream_t rws, const void* buf, size_t count);
static ssize_t mem_flush (purc_rwstream_t rws);
static int mem_close (purc_rwstream_t rws);
static int mem_destroy (purc_rwstream_t rws);

rwstream_funcs mem_funcs = {
    mem_seek,
    mem_tell,
    mem_eof,
    mem_read,
    mem_write,
    mem_flush,
    mem_close,
    mem_destroy
};

#ifdef PURC_BUILD_WITH_GLIB
static off_t win_socket_seek (purc_rwstream_t rws, off_t offset, int whence);
static off_t gio_seek (purc_rwstream_t rws, off_t offset, int whence);
static off_t gio_tell (purc_rwstream_t rws);
static int gio_eof (purc_rwstream_t rws);
static ssize_t gio_read (purc_rwstream_t rws, void* buf, size_t count);
static ssize_t gio_write (purc_rwstream_t rws, const void* buf, size_t count);
static ssize_t gio_flush (purc_rwstream_t rws);
static int gio_close (purc_rwstream_t rws);
static int gio_destroy (purc_rwstream_t rws);

rwstream_funcs gio_funcs = {
    gio_seek,
    gio_tell,
    gio_eof,
    gio_read,
    gio_write,
    gio_flush,
    gio_close,
    gio_destroy
};

rwstream_funcs win_socket_funcs = {
    win_socket_seek,
    gio_tell,
    gio_eof,
    gio_read,
    gio_write,
    gio_flush,
    gio_close,
    gio_destroy
};

purc_error_t rwstream_error_code_from_gerror (GError* err)
{
    if (err == NULL)
    {
        return PURC_ERROR_OK;
    }

    switch (err->code)
    {
        case G_IO_CHANNEL_ERROR_FBIG:
            return PCRWSTREAM_ERROR_FBIG;
        case G_IO_CHANNEL_ERROR_INVAL:
            return PCRWSTREAM_ERROR_INVAL;
        case G_IO_CHANNEL_ERROR_IO:
            return PCRWSTREAM_ERROR_IO;
        case G_IO_CHANNEL_ERROR_ISDIR:
            return PCRWSTREAM_ERROR_ISDIR;
        case G_IO_CHANNEL_ERROR_NOSPC:
            return PCRWSTREAM_ERROR_NOSPC;
        case G_IO_CHANNEL_ERROR_NXIO:
            return PCRWSTREAM_ERROR_NXIO;
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
#endif // PURC_BUILD_WITH_GLIB


/* rwstream api */

purc_rwstream_t purc_rwstream_new_from_mem (void* mem, size_t sz)
{
    struct mem_rwstream* rws = (struct mem_rwstream*) calloc(
            sizeof(struct mem_rwstream), 1);

    rws->rwstream.funcs = &mem_funcs;
    rws->base = mem;
    rws->here = rws->base;
    rws->stop = rws->base + sz;

    return (purc_rwstream_t) rws;
}

purc_rwstream_t purc_rwstream_new_from_file (const char* file, const char* mode)
{
    FILE* fp = fopen(file, mode);
    if (fp == NULL)
    {
        purc_set_last_error(PURC_ERROR_BAD_SYSTEM_CALL);
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
    return (purc_rwstream_t) rws;
}

#ifdef PURC_BUILD_WITH_GLIB
purc_rwstream_t purc_rwstream_new_from_unix_fd (int fd, size_t sz_buf)
{
    GIOChannel* gio_channel = g_io_channel_unix_new(fd);
    if (gio_channel == NULL)
    {
        purc_set_last_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

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
#endif // PURC_BUILD_WITH_GLIB

#if defined(PURC_BUILD_WITH_GLIB) && defined(G_OS_WIN32)
purc_rwstream_t purc_rwstream_new_from_win32_socket (int socket, size_t sz_buf)
{
    GIOChannel* gio_channel = g_io_channel_win32_new_socket(socket);
    if (gio_channel == NULL)
    {
        purc_set_last_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    g_io_channel_set_encoding (gio_channel, NULL, NULL);

    if (sz_buf > 0)
    {
        g_io_channel_set_buffer_size(gio_channel, sz_buf);
    }

    struct gio_rwstream* gio = (struct gio_rwstream*) calloc(
            sizeof(struct gio_rwstream), 1);
    gio->rwstream.funcs = &win_socket_funcs;
    gio->gio_channel = gio_channel;
    return (purc_rwstream_t) rws;
}
#endif // defined(PURC_BUILD_WITH_GLIB) && defined(G_OS_WIN32)

int purc_rwstream_destroy (purc_rwstream_t rws)
{
    if (rws == NULL)
    {
        purc_set_last_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    return rws->funcs->destroy(rws);
}

off_t purc_rwstream_seek (purc_rwstream_t rws, off_t offset, int whence)
{
    if (rws == NULL)
    {
        purc_set_last_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    return rws->funcs->seek(rws, offset, whence);
}

off_t purc_rwstream_tell (purc_rwstream_t rws)
{
    if (rws == NULL)
    {
        purc_set_last_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    return rws->funcs->tell(rws);
}

int purc_rwstream_eof (purc_rwstream_t rws)
{
    if (rws == NULL)
    {
        purc_set_last_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    return rws->funcs->eof(rws);
}

ssize_t purc_rwstream_read (purc_rwstream_t rws, void* buf, size_t count)
{
    if (rws == NULL)
    {
        purc_set_last_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    return rws->funcs->read(rws, buf, count);
}

static wchar_t utf8_to_wchar_t (const unsigned char* utf8_char,
        int utf8_char_len)
{
    wchar_t wc = *((unsigned char *)(utf8_char++));
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
        wchar_t* buf_wc)
{
    if (rws == NULL)
    {
        purc_set_last_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    ssize_t ret =  purc_rwstream_read (rws, buf_utf8, 1);
    if (ret != 1)
    {
        purc_set_last_error(PCRWSTREAM_ERROR_IO);
        return -1;
    }

    int n = 1;
    int ch_len = 0;
    int c = buf_utf8[0];
    if (c & 0x80)
    {
        while (c & (0x80 >> n))
            n++;

        ch_len = n;
    }
    else
    {
        ch_len = 1;
    }

    int read_len = ch_len - 1;
    if (read_len > 0)
    {
        ret =  purc_rwstream_read (rws, buf_utf8+1, read_len);
        if (ret != read_len)
        {
            purc_set_last_error(PCRWSTREAM_ERROR_IO);
            return -1;
        }
    }

    *buf_wc = utf8_to_wchar_t((const unsigned char*)buf_utf8, ch_len);
    return ch_len;
}

ssize_t purc_rwstream_write (purc_rwstream_t rws, const void* buf, size_t count)
{
    if (rws == NULL)
    {
        purc_set_last_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    return rws->funcs->write(rws, buf, count);
}

ssize_t purc_rwstream_flush (purc_rwstream_t rws)
{
    if (rws == NULL)
    {
        purc_set_last_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    return rws->funcs->flush(rws);
}

int purc_rwstream_close (purc_rwstream_t rws)
{
    if (rws == NULL)
    {
        purc_set_last_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }
    return rws->funcs->close(rws);
}

/* stdio rwstream functions */
static off_t stdio_seek (purc_rwstream_t rws, off_t offset, int whence)
{
    struct stdio_rwstream* stdio = (struct stdio_rwstream *)rws;
    if ( fseek(stdio->fp, offset, whence) == 0 )
    {
        return ftell(stdio->fp);
    }
    purc_set_last_error(PURC_ERROR_BAD_SYSTEM_CALL);
    return -1;
}

static off_t stdio_tell (purc_rwstream_t rws)
{
    return ftell(((struct stdio_rwstream *)rws)->fp);
}

static int stdio_eof (purc_rwstream_t rws)
{
    return feof(((struct stdio_rwstream *)rws)->fp);
}

static ssize_t stdio_read (purc_rwstream_t rws, void* buf, size_t count)
{
    struct stdio_rwstream* stdio = (struct stdio_rwstream *)rws;
    ssize_t nread = fread(buf, 1, count, stdio->fp);
    if ( nread == 0 && ferror(stdio->fp) ) {
        purc_set_last_error(PCRWSTREAM_ERROR_IO);
    }
    return(nread);

}

static ssize_t stdio_write (purc_rwstream_t rws, const void* buf, size_t count)
{
    struct stdio_rwstream* stdio = (struct stdio_rwstream *)rws;
    ssize_t nwrote = fwrite(buf, 1, count, stdio->fp);
    if ( nwrote == 0 && ferror(stdio->fp) ) {
        purc_set_last_error(PCRWSTREAM_ERROR_IO);
    }
    return(nwrote);

}

static ssize_t stdio_flush (purc_rwstream_t rws)
{
    return fflush(((struct stdio_rwstream *)rws)->fp);
}

static int stdio_close (purc_rwstream_t rws)
{
    int ret = fclose(((struct stdio_rwstream *)rws)->fp);
    if (ret == 0)
    {
        ((struct stdio_rwstream *)rws)->fp = NULL;
    }
    else
    {
        purc_set_last_error(PURC_ERROR_BAD_SYSTEM_CALL);
    }
    return ret;
}

static int stdio_destroy (purc_rwstream_t rws)
{
    if (((struct stdio_rwstream *)rws)->fp)
    {
        stdio_close(rws);
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

static int mem_eof (purc_rwstream_t rws)
{
    struct mem_rwstream* mem = (struct mem_rwstream *)rws;
    if (mem->here >= mem->stop)
        return 1;
    return 0;
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
    memcpy(mem->here, buf, count);
    mem->here += count;
    return count;
}

static ssize_t mem_flush (purc_rwstream_t rws)
{
    return 0;
}

static int mem_close (purc_rwstream_t rws)
{
    struct mem_rwstream* mem = (struct mem_rwstream *)rws;
    mem->base = NULL;
    mem->here = NULL;
    mem->stop = NULL;
    return 0;
}

static int mem_destroy (purc_rwstream_t rws)
{
    free(rws);
    return 0;
}

#ifdef PURC_BUILD_WITH_GLIB
/* glib rwstream functions */
static off_t win_socket_seek (purc_rwstream_t rws, off_t offset, int whence)
{
    purc_set_last_error(PURC_ERROR_NOT_IMPLEMENTED);
    return -1;
}

static off_t gio_seek (purc_rwstream_t rws, off_t offset, int whence)
{
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
            purc_set_last_error(PURC_ERROR_INVALID_VALUE);
            return(-1);
    }

    GError* err = NULL;
    GIOStatus ios = g_io_channel_seek_position (gio->gio_channel, offset,
            type, &err);
    if (ios == G_IO_STATUS_NORMAL)
    {
        return lseek(gio->fd, 0, SEEK_CUR);
    }
    purc_set_last_error(rwstream_error_code_from_gerror(err));

    if (err)
        g_error_free(err);
    return -1;
}

static off_t gio_tell (purc_rwstream_t rws)
{
    purc_set_last_error(PURC_ERROR_NOT_IMPLEMENTED);
    return -1;
}

static int gio_eof (purc_rwstream_t rws)
{
    purc_set_last_error(PURC_ERROR_NOT_IMPLEMENTED);
    return -1;
}

static ssize_t gio_read (purc_rwstream_t rws, void* buf, size_t count)
{
    struct gio_rwstream* gio = (struct gio_rwstream *)rws;
    gsize read = 0;
    GError* err = NULL;
    g_io_channel_read_chars (gio->gio_channel, buf, count, &read, &err);
    purc_set_last_error(rwstream_error_code_from_gerror(err));
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
    purc_set_last_error(rwstream_error_code_from_gerror(err));
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

static int gio_close (purc_rwstream_t rws)
{
    struct gio_rwstream* gio = (struct gio_rwstream *)rws;
    g_io_channel_shutdown(gio->gio_channel, TRUE, NULL);
    g_io_channel_unref(gio->gio_channel);
    gio->gio_channel = NULL;
    gio->fd = 0;
    return 0;
}

static int gio_destroy (purc_rwstream_t rws)
{
    struct gio_rwstream* gio = (struct gio_rwstream *)rws;
    if (gio->gio_channel)
    {
        gio_close(rws);
    }
    free(rws);
    return 0;
}
#endif // PURC_BUILD_WITH_GLIB
