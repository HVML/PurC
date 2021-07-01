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
#include <glib.h>

struct purc_rwstream;
typedef struct purc_rwstream purc_rwstream;
typedef struct purc_rwstream* purc_rwstream_t;

typedef struct _RW_FUNCS
{
    off_t   (*seek) (purc_rwstream_t rws, off_t offset, int whence);
    off_t   (*tell) (purc_rwstream_t rws);
    int     (*eof) (purc_rwstream_t rws);
    ssize_t (*read) (purc_rwstream_t rws, void* buf, size_t count);
    int     (*read_utf8_char) (purc_rwstream_t rws, char* buf_utf8,
                wchar_t* buf_wc);
    ssize_t (*write) (purc_rwstream_t rws, const void* buf, size_t count);
    ssize_t (*flush) (purc_rwstream_t rws);
    int     (*close) (purc_rwstream_t rws);
    int     (*destroy) (purc_rwstream_t rws);
} RW_FUNCS;

struct purc_rwstream
{
    RW_FUNCS* rw_funcs;
};

typedef struct _PURC_STDIO_RWSTREAM
{
    purc_rwstream rwstream;
    FILE* fp;
} PURC_STDIO_RWSTREAM;

typedef struct _PURC_MEM_RWSTREAM
{
    purc_rwstream rwstream;
    uint8_t* base;
    uint8_t* here;
    uint8_t* stop;
} PURC_MEM_RWSTREAM;

typedef struct _PURC_GIO_RWSTREAM
{
    purc_rwstream rwstream;
    GIOChannel* gio_channel;
} PURC_GIO_RWSTREAM;

off_t stdio_seek (purc_rwstream_t rws, off_t offset, int whence);
off_t stdio_tell (purc_rwstream_t rws);
int stdio_eof (purc_rwstream_t rws);
ssize_t stdio_read (purc_rwstream_t rws, void* buf, size_t count);
int stdio_read_utf8_char (purc_rwstream_t rws, char* buf_utf8, wchar_t* buf_wc);
ssize_t stdio_write (purc_rwstream_t rws, const void* buf, size_t count);
ssize_t stdio_flush (purc_rwstream_t rws);
int stdio_close (purc_rwstream_t rws);
int stdio_destroy (purc_rwstream_t rws);

RW_FUNCS stdio_rw_funcs = {
    stdio_seek,
    stdio_tell,
    stdio_eof,
    stdio_read,
    stdio_read_utf8_char,
    stdio_write,
    stdio_flush,
    stdio_close,
    stdio_destroy
};

off_t mem_seek (purc_rwstream_t rws, off_t offset, int whence);
off_t mem_tell (purc_rwstream_t rws);
int mem_eof (purc_rwstream_t rws);
ssize_t mem_read (purc_rwstream_t rws, void* buf, size_t count);
int mem_read_utf8_char (purc_rwstream_t rws, char* buf_utf8, wchar_t* buf_wc);
ssize_t mem_write (purc_rwstream_t rws, const void* buf, size_t count);
ssize_t mem_flush (purc_rwstream_t rws);
int mem_close (purc_rwstream_t rws);
int mem_destroy (purc_rwstream_t rws);

RW_FUNCS mem_rw_funcs = {
    mem_seek,
    mem_tell,
    mem_eof,
    mem_read,
    mem_read_utf8_char,
    mem_write,
    mem_flush,
    mem_close,
    mem_destroy
};

off_t gio_seek (purc_rwstream_t rws, off_t offset, int whence);
off_t gio_tell (purc_rwstream_t rws);
int gio_eof (purc_rwstream_t rws);
ssize_t gio_read (purc_rwstream_t rws, void* buf, size_t count);
int gio_read_utf8_char (purc_rwstream_t rws, char* buf_utf8, wchar_t* buf_wc);
ssize_t gio_write (purc_rwstream_t rws, const void* buf, size_t count);
ssize_t gio_flush (purc_rwstream_t rws);
int gio_close (purc_rwstream_t rws);
int gio_destroy (purc_rwstream_t rws);

RW_FUNCS gio_rw_funcs = {
    gio_seek,
    gio_tell,
    gio_eof,
    gio_read,
    gio_read_utf8_char,
    gio_write,
    gio_flush,
    gio_close,
    gio_destroy
};

/* rwstream api */

purc_rwstream_t purc_rwstream_new_from_mem (void* mem, size_t sz)
{
    PURC_MEM_RWSTREAM* mem_rwstream = (PURC_MEM_RWSTREAM*) calloc(
            sizeof(PURC_MEM_RWSTREAM), 1);

    mem_rwstream->rwstream.rw_funcs = &mem_rw_funcs;
    mem_rwstream->base = mem;
    mem_rwstream->here = mem_rwstream->base;
    mem_rwstream->stop = mem_rwstream->base + sz;

    return (purc_rwstream_t) mem_rwstream;
}

purc_rwstream_t purc_rwstream_new_from_file (const char* file, const char* mode)
{
    FILE* fp = fopen(file, mode);
    if (fp == NULL)
    {
        return NULL;
    }
    return purc_rwstream_new_from_fp(fp);
}

purc_rwstream_t purc_rwstream_new_from_fp (FILE* fp)
{
    PURC_STDIO_RWSTREAM* stdio_rwstream = (PURC_STDIO_RWSTREAM*) calloc(
            sizeof(PURC_STDIO_RWSTREAM), 1);

    stdio_rwstream->rwstream.rw_funcs = &stdio_rw_funcs;
    stdio_rwstream->fp = fp;
    return (purc_rwstream_t) stdio_rwstream;
}

purc_rwstream_t purc_rwstream_new_from_unix_fd (int fd, size_t sz_buf)
{
    GIOChannel* gio_channel = g_io_channel_unix_new(fd);
    if (gio_channel == NULL)
    {
        return NULL;
    }

    if (sz_buf > 0)
    {
        g_io_channel_set_buffer_size(gio_channel, sz_buf);
    }

    PURC_GIO_RWSTREAM* gio_rwstream = (PURC_GIO_RWSTREAM*) calloc(
            sizeof(PURC_GIO_RWSTREAM), 1);
    gio_rwstream->rwstream.rw_funcs = &gio_rw_funcs;
    gio_rwstream->gio_channel = gio_channel;
    return (purc_rwstream_t) gio_rwstream;
}

purc_rwstream_t purc_rwstream_new_from_win32_socket (int socket, size_t sz_buf)
{
#ifdef G_OS_WIN32
    GIOChannel* gio_channel = g_io_channel_win32_new_socket(socket);
    if (gio_channel == NULL)
    {
        return NULL;
    }

    if (sz_buf > 0)
    {
        g_io_channel_set_buffer_size(gio_channel, sz_buf);
    }

    PURC_GIO_RWSTREAM* gio_rwstream = (PURC_GIO_RWSTREAM*) calloc(
            sizeof(PURC_GIO_RWSTREAM), 1);
    gio_rwstream->rwstream.rw_funcs = &gio_rw_funcs;
    gio_rwstream->gio_channel = gio_channel;
    return (purc_rwstream_t) gio_rwstream;
#else
    return NULL;
#endif
}

int purc_rwstream_destroy (purc_rwstream_t rws)
{
    return rws ? rws->rw_funcs->destroy(rws) : -1;
}

off_t purc_rwstream_seek (purc_rwstream_t rws, off_t offset, int whence)
{
    return rws ? rws->rw_funcs->seek(rws, offset, whence) : -1;
}

off_t purc_rwstream_tell (purc_rwstream_t rws)
{
    return rws ? rws->rw_funcs->tell(rws) : -1;
}

ssize_t purc_rwstream_read (purc_rwstream_t rws, void* buf, size_t count)
{
    return rws ? rws->rw_funcs->read(rws, buf, count) : -1;
}

int purc_rwstream_read_utf8_char (purc_rwstream_t rws, char* buf_utf8, wchar_t* buf_wc)
{
    return rws ? rws->rw_funcs->read_utf8_char(rws, buf_utf8, buf_wc) : -1;
}

ssize_t purc_rwstream_write (purc_rwstream_t rws, const void* buf, size_t count)
{
    return rws ? rws->rw_funcs->write(rws, buf, count) : -1;
}

ssize_t purc_rwstream_flush (purc_rwstream_t rws)
{
    return rws ? rws->rw_funcs->flush(rws) : -1;
}

int purc_rwstream_close (purc_rwstream_t rws)
{
    return rws ? rws->rw_funcs->close(rws) : -1;
}

/* stdio rwstream functions */
off_t stdio_seek (purc_rwstream_t rws, off_t offset, int whence)
{
    PURC_STDIO_RWSTREAM* stdio = (PURC_STDIO_RWSTREAM *)rws;
    if ( fseek(stdio->fp, offset, whence) == 0 )
    {
        return ftell(stdio->fp);
    } else {
        return -1;
    }
}

off_t stdio_tell (purc_rwstream_t rws)
{
    return ftell(((PURC_STDIO_RWSTREAM *)rws)->fp);
}

int stdio_eof (purc_rwstream_t rws)
{
    return feof(((PURC_STDIO_RWSTREAM *)rws)->fp);
}

ssize_t stdio_read (purc_rwstream_t rws, void* buf, size_t count)
{
    PURC_STDIO_RWSTREAM* stdio = (PURC_STDIO_RWSTREAM *)rws;
    ssize_t nread = fread(buf, count, 1, stdio->fp);
    if ( nread == 0 && ferror(stdio->fp) ) {
        // error
    }
    return(nread);

}

int stdio_read_utf8_char (purc_rwstream_t rws, char* buf_utf8, wchar_t* buf_wc)
{
    // TODO
    return 0;
}

ssize_t stdio_write (purc_rwstream_t rws, const void* buf, size_t count)
{
    PURC_STDIO_RWSTREAM* stdio = (PURC_STDIO_RWSTREAM *)rws;
    ssize_t nwrote = fwrite(buf, count, 1, stdio->fp);
    if ( nwrote == 0 && ferror(stdio->fp) ) {
        // error
    }
    return(nwrote);

}

ssize_t stdio_flush (purc_rwstream_t rws)
{
    return fflush(((PURC_STDIO_RWSTREAM *)rws)->fp);
}

int stdio_close (purc_rwstream_t rws)
{
    return fclose(((PURC_STDIO_RWSTREAM *)rws)->fp);
}

int stdio_destroy (purc_rwstream_t rws)
{
    free(rws);
    return 0;
}


/* memory rwstream functions */
off_t mem_seek (purc_rwstream_t rws, off_t offset, int whence)
{
    PURC_MEM_RWSTREAM* mem = (PURC_MEM_RWSTREAM *)rws;
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

off_t mem_tell (purc_rwstream_t rws)
{
    PURC_MEM_RWSTREAM* mem = (PURC_MEM_RWSTREAM *)rws;
    return (mem->here - mem->base);
}

int mem_eof (purc_rwstream_t rws)
{
    PURC_MEM_RWSTREAM* mem = (PURC_MEM_RWSTREAM *)rws;
    if (mem->here >= mem->stop)
        return 1;
    return 0;

}

ssize_t mem_read (purc_rwstream_t rws, void* buf, size_t count)
{
    PURC_MEM_RWSTREAM* mem = (PURC_MEM_RWSTREAM *)rws;
    if ( (mem->here + count) > mem->stop )
    {
        count = mem->stop - mem->here;
    }
    memcpy(buf, mem->here, count);
    mem->here += count;
    return count;

}

int mem_read_utf8_char (purc_rwstream_t rws, char* buf_utf8, wchar_t* buf_wc)
{
//    PURC_MEM_RWSTREAM* mem = (PURC_MEM_RWSTREAM *)rws;
//    TODO
    return 0;
}

ssize_t mem_write (purc_rwstream_t rws, const void* buf, size_t count)
{
    PURC_MEM_RWSTREAM* mem = (PURC_MEM_RWSTREAM *)rws;
    if ( (mem->here + count) > mem->stop ) {
        count = mem->stop - mem->here;
    }
    memcpy(mem->here, buf, count);
    mem->here += count;
    return count;
}

ssize_t mem_flush (purc_rwstream_t rws)
{
    return 0;
}

int mem_close (purc_rwstream_t rws)
{
    PURC_MEM_RWSTREAM* mem = (PURC_MEM_RWSTREAM *)rws;
    mem->base = NULL;
    mem->here = NULL;
    mem->stop = NULL;
    return 0;
}

int mem_destroy (purc_rwstream_t rws)
{
    free(rws);
    return 0;
}

/* gio rwstream functions */
off_t gio_seek (purc_rwstream_t rws, off_t offset, int whence)
{
    PURC_GIO_RWSTREAM* gio = (PURC_GIO_RWSTREAM *)rws;
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
            return(-1);
    }
    g_io_channel_seek_position (gio->gio_channel, offset, type, NULL);
    // TODO
    return 0;
}

off_t gio_tell (purc_rwstream_t rws)
{
    return -1;
}

int gio_eof (purc_rwstream_t rws)
{
    return -1;
}

ssize_t gio_read (purc_rwstream_t rws, void* buf, size_t count)
{
    PURC_GIO_RWSTREAM* gio = (PURC_GIO_RWSTREAM *)rws;
    gsize read = 0;
    g_io_channel_read_chars (gio->gio_channel, buf, count, &read, NULL);
    // TODO
    return read;
}

int gio_read_utf8_char (purc_rwstream_t rws, char* buf_utf8, wchar_t* buf_wc)
{
    PURC_GIO_RWSTREAM* gio = (PURC_GIO_RWSTREAM *)rws;
    gunichar uchar = 0;
    g_io_channel_read_unichar (gio->gio_channel, &uchar, NULL);
    // TODO
    // TODO
    return 0;
}

ssize_t gio_write (purc_rwstream_t rws, const void* buf, size_t count)
{
    PURC_GIO_RWSTREAM* gio = (PURC_GIO_RWSTREAM *)rws;
    gsize write = 0;
    g_io_channel_write_chars (gio->gio_channel, buf, count, &write, NULL);
    // TODO
    return write;
}

ssize_t gio_flush (purc_rwstream_t rws)
{
    PURC_GIO_RWSTREAM* gio = (PURC_GIO_RWSTREAM *)rws;
    g_io_channel_flush (gio->gio_channel, NULL);
    // TODO
    return 0;
}

int gio_close (purc_rwstream_t rws)
{
    PURC_GIO_RWSTREAM* gio = (PURC_GIO_RWSTREAM *)rws;
    g_io_channel_shutdown(gio->gio_channel, TRUE, NULL);
    g_io_channel_unref(gio->gio_channel);
    gio->gio_channel = NULL;
    return 0;
}
int gio_destroy (purc_rwstream_t rws)
{
    free(rws);
    return 0;
}
