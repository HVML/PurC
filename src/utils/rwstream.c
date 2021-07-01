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

typedef struct stdio_rwstream
{
    purc_rwstream rwstream;
    FILE* fp;
} stdio_rwstream;

typedef struct mem_rwstream
{
    purc_rwstream rwstream;
    uint8_t* base;
    uint8_t* here;
    uint8_t* stop;
} mem_rwstream;

typedef struct gio_rwstream
{
    purc_rwstream rwstream;
    GIOChannel* gio_channel;
} gio_rwstream;

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

/* rwstream api */

purc_rwstream_t purc_rwstream_new_from_mem (void* mem, size_t sz)
{
    mem_rwstream* rws = (mem_rwstream*) calloc(sizeof(mem_rwstream), 1);

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
        return NULL;
    }
    return purc_rwstream_new_from_fp(fp);
}

purc_rwstream_t purc_rwstream_new_from_fp (FILE* fp)
{
    stdio_rwstream* rws = (stdio_rwstream*) calloc(sizeof(stdio_rwstream), 1);

    rws->rwstream.funcs = &stdio_funcs;
    rws->fp = fp;
    return (purc_rwstream_t) rws;
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

    gio_rwstream* rws = (gio_rwstream*) calloc(sizeof(gio_rwstream), 1);
    rws->rwstream.funcs = &gio_funcs;
    rws->gio_channel = gio_channel;
    return (purc_rwstream_t) rws;
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

    gio_rwstream* gio_rwstream = (gio_rwstream*) calloc(
            sizeof(gio_rwstream), 1);
    gio_rwstream->rwstream.funcs = &gio_funcs;
    gio_rwstream->gio_channel = gio_channel;
    return (purc_rwstream_t) gio_rwstream;
#else
    return NULL;
#endif
}

int purc_rwstream_destroy (purc_rwstream_t rws)
{
    return rws ? rws->funcs->destroy(rws) : -1;
}

off_t purc_rwstream_seek (purc_rwstream_t rws, off_t offset, int whence)
{
    return rws ? rws->funcs->seek(rws, offset, whence) : -1;
}

off_t purc_rwstream_tell (purc_rwstream_t rws)
{
    return rws ? rws->funcs->tell(rws) : -1;
}

ssize_t purc_rwstream_read (purc_rwstream_t rws, void* buf, size_t count)
{
    return rws ? rws->funcs->read(rws, buf, count) : -1;
}

int purc_rwstream_read_utf8_char (purc_rwstream_t rws, char* buf_utf8, wchar_t* buf_wc)
{
    //TODO
    return 0;
}

ssize_t purc_rwstream_write (purc_rwstream_t rws, const void* buf, size_t count)
{
    return rws ? rws->funcs->write(rws, buf, count) : -1;
}

ssize_t purc_rwstream_flush (purc_rwstream_t rws)
{
    return rws ? rws->funcs->flush(rws) : -1;
}

int purc_rwstream_close (purc_rwstream_t rws)
{
    return rws ? rws->funcs->close(rws) : -1;
}

/* stdio rwstream functions */
static off_t stdio_seek (purc_rwstream_t rws, off_t offset, int whence)
{
    stdio_rwstream* stdio = (stdio_rwstream *)rws;
    if ( fseek(stdio->fp, offset, whence) == 0 )
    {
        return ftell(stdio->fp);
    } else {
        return -1;
    }
}

static off_t stdio_tell (purc_rwstream_t rws)
{
    return ftell(((stdio_rwstream *)rws)->fp);
}

static int stdio_eof (purc_rwstream_t rws)
{
    return feof(((stdio_rwstream *)rws)->fp);
}

static ssize_t stdio_read (purc_rwstream_t rws, void* buf, size_t count)
{
    stdio_rwstream* stdio = (stdio_rwstream *)rws;
    ssize_t nread = fread(buf, count, 1, stdio->fp);
    if ( nread == 0 && ferror(stdio->fp) ) {
        // error
    }
    return(nread);

}

static ssize_t stdio_write (purc_rwstream_t rws, const void* buf, size_t count)
{
    stdio_rwstream* stdio = (stdio_rwstream *)rws;
    ssize_t nwrote = fwrite(buf, count, 1, stdio->fp);
    if ( nwrote == 0 && ferror(stdio->fp) ) {
        // error
    }
    return(nwrote);

}

static ssize_t stdio_flush (purc_rwstream_t rws)
{
    return fflush(((stdio_rwstream *)rws)->fp);
}

static int stdio_close (purc_rwstream_t rws)
{
    return fclose(((stdio_rwstream *)rws)->fp);
}

static int stdio_destroy (purc_rwstream_t rws)
{
    free(rws);
    return 0;
}


/* memory rwstream functions */
static off_t mem_seek (purc_rwstream_t rws, off_t offset, int whence)
{
    mem_rwstream* mem = (mem_rwstream *)rws;
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
    mem_rwstream* mem = (mem_rwstream *)rws;
    return (mem->here - mem->base);
}

static int mem_eof (purc_rwstream_t rws)
{
    mem_rwstream* mem = (mem_rwstream *)rws;
    if (mem->here >= mem->stop)
        return 1;
    return 0;

}

static ssize_t mem_read (purc_rwstream_t rws, void* buf, size_t count)
{
    mem_rwstream* mem = (mem_rwstream *)rws;
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
    mem_rwstream* mem = (mem_rwstream *)rws;
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
    mem_rwstream* mem = (mem_rwstream *)rws;
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

/* gio rwstream functions */
static off_t gio_seek (purc_rwstream_t rws, off_t offset, int whence)
{
    gio_rwstream* gio = (gio_rwstream *)rws;
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

static off_t gio_tell (purc_rwstream_t rws)
{
    return -1;
}

static int gio_eof (purc_rwstream_t rws)
{
    return -1;
}

static ssize_t gio_read (purc_rwstream_t rws, void* buf, size_t count)
{
    gio_rwstream* gio = (gio_rwstream *)rws;
    gsize read = 0;
    g_io_channel_read_chars (gio->gio_channel, buf, count, &read, NULL);
    // TODO
    return read;
}

static ssize_t gio_write (purc_rwstream_t rws, const void* buf, size_t count)
{
    gio_rwstream* gio = (gio_rwstream *)rws;
    gsize write = 0;
    g_io_channel_write_chars (gio->gio_channel, buf, count, &write, NULL);
    // TODO
    return write;
}

static ssize_t gio_flush (purc_rwstream_t rws)
{
    gio_rwstream* gio = (gio_rwstream *)rws;
    g_io_channel_flush (gio->gio_channel, NULL);
    // TODO
    return 0;
}

static int gio_close (purc_rwstream_t rws)
{
    gio_rwstream* gio = (gio_rwstream *)rws;
    g_io_channel_shutdown(gio->gio_channel, TRUE, NULL);
    g_io_channel_unref(gio->gio_channel);
    gio->gio_channel = NULL;
    return 0;
}

static int gio_destroy (purc_rwstream_t rws)
{
    free(rws);
    return 0;
}
