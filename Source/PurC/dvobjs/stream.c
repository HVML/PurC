/*
 * @file stream.c
 * @author Geng Yue, Xue Shuming, Vincent Wei
 * @date 2021/07/02
 * @brief The implementation of stream dynamic variant object.
 *
 * Copyright (C) 2021 ~ 2025 FMSoft <https://www.fmsoft.cn>
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

#define _GNU_SOURCE

#include "purc-errors.h"
#include "purc-rwstream.h"
#include "purc-utils.h"

#include "config.h"
#include "stream.h"
#include "socket.h"
#include "helper.h"

#include "purc-variant.h"
#include "purc-runloop.h"
#include "purc-dvobjs.h"

#include "private/instance.h"
#include "private/debug.h"
#include "private/dvobjs.h"
#include "private/atom-buckets.h"
#include "private/interpreter.h"

#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>

#define ENDIAN_PLATFORM             0
#define ENDIAN_LITTLE               1
#define ENDIAN_BIG                  2

#define STDIN_NAME                  "stdin"
#define STDOUT_NAME                 "stdout"
#define STDERR_NAME                 "stderr"

#define STREAM_EVENT_NAME           "stream"
#define STREAM_SUB_EVENT_READABLE   "readable"
#define STREAM_SUB_EVENT_WRITABLE   "writable"
#define STREAM_SUB_EVENT_HANGUP     "hangup"
#define STREAM_SUB_EVENT_ERROR      "error"

#define FILE_DEFAULT_MODE           0644
#define FIFO_DEFAULT_MODE           0644

#define STREAM_ATOM_BUCKET          ATOM_BUCKET_DVOBJ

enum {
#define _KW_keep                    "keep"
    K_KW_keep,
#define _KW_default                 "default"
    K_KW_default,
#define _KW_read                    "read"
    K_KW_read,
#define _KW_write                   "write"
    K_KW_write,
#define _KW_append                  "append"
    K_KW_append,
#define _KW_create                  "create"
    K_KW_create,
#define _KW_truncate                "truncate"
    K_KW_truncate,
#define _KW_nameless                "nameless"
    K_KW_nameless,
#define _KW_nonblock                "nonblock"
    K_KW_nonblock,
#define _KW_cloexec                 "cloexec"
    K_KW_cloexec,
#define _KW_set                     "set"
    K_KW_set,
#define _KW_current                 "current"
    K_KW_current,
#define _KW_end                     "end"
    K_KW_end,
#define _KW_file                    "file"
    K_KW_file,
#define _KW_pipe                    "pipe"
    K_KW_pipe,
#define _KW_fifo                    "fifo"
    K_KW_fifo,
#define _KW_unix                    "unix"
    K_KW_unix,
#define _KW_local                   "local"
    K_KW_local,
#define _KW_inet                    "inet"
    K_KW_inet,
#define _KW_inet4                   "inet4"
    K_KW_inet4,
#define _KW_inet6                   "inet6"
    K_KW_inet6,
#define _KW_websocket               "websocket"
    K_KW_websocket,
#define _KW_message                 "message"
    K_KW_message,
#define _KW_hbdbus                  "hbdbus"
    K_KW_hbdbus,
#define _KW_readstruct              "readstruct"
    K_KW_readstruct,
#define _KW_writestruct             "writestruct"
    K_KW_writestruct,
#define _KW_readlines               "readlines"
    K_KW_readlines,
#define _KW_writelines              "writelines"
    K_KW_writelines,
#define _KW_readbytes               "readbytes"
    K_KW_readbytes,
#define _KW_readbytes2buffer        "readbytes2buffer"
    K_KW_readbytes2buffer,
#define _KW_writebytes              "writebytes"
    K_KW_writebytes,
#define _KW_writeeof                "writeeof"
    K_KW_writeeof,
#define _KW_status                  "status"
    K_KW_status,
#define _KW_seek                    "seek"
    K_KW_seek,
#define _KW_getuc                   "getuc"
    K_KW_getuc,
#define _KW_putuc                   "putuc"
    K_KW_putuc,
#define _KW_ungetuc                 "ungetuc"
    K_KW_ungetuc,
#define _KW_close                   "close"
    K_KW_close,
#define _KW_fd                      "fd"
    K_KW_fd,
#define _KW_peer_addr               "peerAddr"
    K_KW_peer_addr,
#define _KW_peer_port               "peerPort"
    K_KW_peer_port,
};

static struct keyword_to_atom {
    const char *keyword;
    purc_atom_t atom;
} keywords2atoms [] = {
    { _KW_keep, 0 },                // "keep"
    { _KW_default, 0 },             // "default"
    { _KW_read, 0 },                // "read"
    { _KW_write, 0 },               // "write"
    { _KW_append, 0 },              // "append"
    { _KW_create, 0 },              // "create"
    { _KW_truncate, 0 },            // "truncate"
    { _KW_nameless, 0 },            // "nameless"
    { _KW_nonblock, 0 },            // "nonblock"
    { _KW_cloexec, 0 },             // "cloexec"
    { _KW_set, 0 },                 // "set"
    { _KW_current, 0 },             // "current"
    { _KW_end, 0 },                 // "end"
    { _KW_file, 0 },                // "file"
    { _KW_pipe, 0 },                // "pipe"
    { _KW_fifo, 0 },                // "fifo"
    { _KW_unix, 0 },                // "unix"
    { _KW_local, 0 },               // "local"
    { _KW_inet, 0},                 // "inet"
    { _KW_inet4, 0},                // "inet4"
    { _KW_inet6, 0},                // "inet6"
    { _KW_websocket, 0 },           // "websocket"
    { _KW_message, 0 },             // "message"
    { _KW_hbdbus, 0 },              // "hbdbus"
    { _KW_readstruct, 0},           // "readstruct"
    { _KW_writestruct, 0},          // "writestruct"
    { _KW_readlines, 0},            // "readlines"
    { _KW_writelines, 0},           // "writelines"
    { _KW_readbytes, 0},            // "readbytes"
    { _KW_readbytes2buffer, 0},     // "readbytes2buffer"
    { _KW_writebytes, 0},           // "writebytes"
    { _KW_writeeof, 0},             // "writeeof"
    { _KW_status, 0},               // "status"
    { _KW_seek, 0},                 // "seek"
    { _KW_getuc, 0},                // "getuc"
    { _KW_putuc, 0},                // "putuc"
    { _KW_ungetuc, 0},              // "ungetuc"
    { _KW_close, 0},                // "close"
    { _KW_fd, 0},                   // "fd"
    { _KW_peer_addr, 0},            // "peerAddr"
    { _KW_peer_port, 0},            // "peerPort"
};

static struct pcdvobjs_stream *
dvobjs_stream_new(enum pcdvobjs_stream_type type,
        struct purc_broken_down_url *url)
{
    struct pcdvobjs_stream *stream;

    stream = calloc(1, sizeof(struct pcdvobjs_stream));
    if (!stream) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    stream->type = type;
    stream->url = url;
    stream->fd4r = -1;
    stream->fd4w = -1;
    return stream;
}

static void native_stream_close(struct pcdvobjs_stream *stream)
{
    if (stream->stm4r) {
        purc_rwstream_destroy(stream->stm4r);
    }

    if (stream->stm4w && stream->stm4w != stream->stm4r) {
        purc_rwstream_destroy(stream->stm4w);
    }

    stream->stm4w = NULL;
    stream->stm4r = NULL;

    if (stream->type == STREAM_TYPE_MEM &&
            stream->kept != PURC_VARIANT_INVALID) {
        purc_variant_unref(stream->kept);
        stream->kept = PURC_VARIANT_INVALID;
    }

    for (int i = 0; i < NR_STREAM_MONITORS; i++) {
        if (stream->monitors[i]) {
            purc_runloop_remove_fd_monitor(purc_runloop_get_current(),
                    stream->monitors[i]);
            stream->monitors[i] = 0;
        }
    }

    if (stream->fp) {
        fclose(stream->fp);
        stream->fp = NULL;
        stream->fd4r = -1; /* fclose will close the file descriptor */
    }

    if (stream->fd4r >= 0) {
        close(stream->fd4r);
    }

    if (stream->fd4w >= 0 && stream->fd4w != stream->fd4r) {
        close(stream->fd4w);
    }

    stream->fd4r = -1;
    stream->fd4w = -1;

    if (stream->type == STREAM_TYPE_PIPE && stream->cpid > 0) {
        int status;
        if (waitpid(stream->cpid, &status, WNOHANG) == 0) {
            if (kill(stream->cpid, SIGKILL) == -1) {
                if (errno == ESRCH) {
                    /* wait agian to avoid zombie */
                    waitpid(stream->cpid, &status, WNOHANG);
                }
                else if (errno == EPERM) {
                    purc_log_error("Failed to kill child process: %d\n",
                            stream->cpid);
                }
            }
        }
        stream->cpid = -1;
    }

    if (stream->peer_addr) {
        free(stream->peer_addr);
        stream->peer_addr = NULL;
    }

    if (stream->peer_port) {
        free(stream->peer_port);
        stream->peer_port = NULL;
    }

    if (stream->socket) {
        pcdvobjs_socket_release(stream->socket);
        stream->socket = NULL;
    }
}

static void dvobjs_stream_delete(struct pcdvobjs_stream *stream)
{
    if (!stream) {
        return;
    }

    native_stream_close(stream);

    /* we keep url for possible reopen() the stream in the future */
    if (stream->url) {
        pcutils_broken_down_url_delete(stream->url);
    }

    free(stream);
}

static inline
struct pcdvobjs_stream *get_stream(void *native_entity)
{
    return (struct pcdvobjs_stream*)native_entity;
}

static purc_variant_t
readstruct_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    struct pcdvobjs_stream *stream;
    purc_rwstream_t rwstream;
    const char *formats = NULL;
    size_t formats_left = 0;

    stream = get_stream(native_entity);
    rwstream = stream->stm4r;
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    formats = purc_variant_get_string_const_ex(argv[0], &formats_left);
    if (formats == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    formats = pcutils_trim_spaces(formats, &formats_left);
    if (formats_left == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    size_t nr_total_read;
    purc_variant_t retv;
    // purc_clr_error();
    retv = purc_dvobj_read_struct(rwstream, formats, formats_left,
            &nr_total_read, (call_flags & PCVRT_CALL_FLAG_SILENTLY));
    if (retv == PURC_VARIANT_INVALID) {
        goto fatal;
    }

    if (nr_total_read == 0) {
        if (retv)
            purc_variant_unref(retv);

        if (purc_get_last_error() == PURC_ERROR_IO_FAILURE) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return purc_variant_make_null();
            }
            else {
                purc_set_error(PURC_ERROR_IO_FAILURE);
                goto failed;
            }
        }
        else if (stream->type == STREAM_TYPE_FILE ||
                stream->type == STREAM_TYPE_MEM) {
            purc_set_error(PURC_ERROR_NO_DATA);
            goto failed;
        }
        else {
            purc_set_error(PURC_ERROR_BROKEN_PIPE);
            goto failed;
        }
    }

    return retv;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_null();
    }

fatal:
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
writestruct_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    bool silently = call_flags & PCVRT_CALL_FLAG_SILENTLY;
    struct pcdvobj_bytes_buff bf = { NULL, 0, 0 };

    struct pcdvobjs_stream *stream;
    purc_rwstream_t rwstream = NULL;
    const char *formats = NULL;
    size_t formats_left = 0;

    stream = get_stream(native_entity);
    rwstream = stream->stm4w;
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    formats = purc_variant_get_string_const_ex(argv[0], &formats_left);
    if (formats == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    formats = pcutils_trim_spaces(formats, &formats_left);
    if (formats_left == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (purc_dvobj_pack_variants(&bf, argv + 1, nr_args - 1, formats,
                formats_left, silently)) {
        if (bf.bytes == NULL &&
                purc_get_last_error() == PURC_ERROR_OUT_OF_MEMORY) {
            goto fatal;
        }

        goto failed;
    }

    ssize_t nr_written = 0;
    if (bf.bytes && bf.nr_bytes > 0) {
        nr_written = purc_rwstream_write(rwstream, bf.bytes, bf.nr_bytes);
        free(bf.bytes);
        bf.bytes = NULL;
    }

    if (nr_written == -1) {
        switch (errno) {
            case EPIPE:
                purc_set_error(PURC_ERROR_BROKEN_PIPE);
                goto failed;
            case EDQUOT:
            case ENOSPC:
                purc_set_error(PCRWSTREAM_ERROR_NO_SPACE);
                goto failed;
            case EPERM:
                purc_set_error(PURC_ERROR_ACCESS_DENIED);
                goto failed;
            case EFBIG:
                purc_set_error(PURC_ERROR_TOO_LARGE_ENTITY);
                goto failed;
            case EIO:
                purc_set_error(PURC_ERROR_IO_FAILURE);
                goto failed;
            case EAGAIN:
#if EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK:
#endif
            default:
                break;
        }
    }

    return purc_variant_make_longint(nr_written);

failed:
    if (bf.bytes)
        free(bf.bytes);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

fatal:
    return PURC_VARIANT_INVALID;
}

#if 0
#define _LINE_BUFFER_SIZE                 1024

static int read_lines(struct pcdvobjs_stream *entity, int line_num,
        purc_variant_t array, const char *line_seperator)
{
    purc_rwstream_t stream = entity->stm4r;
    size_t total_read = 0;
    unsigned char buffer[_LINE_BUFFER_SIZE + 1];
    ssize_t read_size = 0;
    size_t length = 0;
    const char *head = NULL;

    while (line_num) {
        read_size = purc_rwstream_read(stream, buffer, _LINE_BUFFER_SIZE);
        if (read_size == 0) {
            if (total_read == 0) {
                if (entity->type == STREAM_TYPE_FILE) {
                    PC_WARN("Reached the EOF.\n");
                    purc_set_error(PURC_ERROR_NO_DATA);
                    goto failed;
                }
                else {
                    PC_WARN("The peer has been closed.\n");
                    purc_set_error(PURC_ERROR_BROKEN_PIPE);
                    goto failed;
                }
            }
            break;
        }
        else if (read_size < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            else if (total_read == 0) {
                purc_set_error(PURC_ERROR_IO_FAILURE);
                goto failed;
            }
        }
        else {
            buffer[read_size] = '\0';   // terminating null byte.
        }

        total_read += read_size;

        const char *line = (const char*)buffer, *next_line;
        size_t left = read_size;
        while ((next_line = pcutils_get_next_line_len(line, left,
                        line_seperator, &length))) {

            purc_variant_t var;
            if (length == 0)
                var = purc_variant_make_string_static("", false);
            else
                var = purc_variant_make_string_ex(line, length, false);

            if (!var) {
                return -1;
            }
            if (!purc_variant_array_append(array, var)) {
                purc_variant_unref(var);
                return -1;
            }
            purc_variant_unref(var);
            line_num--;

            if (line_num == 0)
                break;

            left -= next_line - line;
            line = next_line;
        }

        if (read_size < _LINE_BUFFER_SIZE)           // to the end
            break;

        if (line_num == 0)
            break;
    }

    return 0;

failed:
    return -1;
}

#else

static int read_lines(struct pcdvobjs_stream *entity, int line_num,
        purc_variant_t array, const char *line_seperator)
{
    size_t total_ucs = 0;

    if (entity->fp == NULL) {
        if (entity->type == STREAM_TYPE_MEM) {
            void *buff;
            size_t buff_len;
            buff = purc_rwstream_get_mem_buffer_ex(entity->stm4r, &buff_len,
                    NULL, false);
            assert(buff);

            entity->fp = fmemopen(buff, buff_len, "r");
            if (entity->fp == NULL) {
                PC_ERROR("Failed fmemopen(%p, %zu): %s\n", buff, buff_len,
                        strerror(errno));
                purc_set_error(purc_error_from_errno(errno));
                goto failed;
            }
        }
        else {
            entity->fp = fdopen(entity->fd4r, "r");
            if (entity->fp == NULL) {
                PC_ERROR("Failed fdopen(%d): %s\n", entity->fd4r, strerror(errno));
                purc_set_error(purc_error_from_errno(errno));
                goto failed;
            }
        }
    }

    size_t sep_len = strlen(line_seperator);
    size_t sep_ucs = pcutils_string_utf8_chars(line_seperator, sep_len);

    struct pcutils_mystring mystr;
    pcutils_mystring_init(&mystr);

    while (line_num) {

        unsigned char utf8ch[6];
        int uchlen = 0;
        int c = fgetc(entity->fp);
        if (c == EOF || ferror(entity->fp)) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                clearerr(entity->fp);
                break;
            }
            PC_WARN("fgetc() returns EOF\n");
            if (total_ucs == 0) {
                goto nothing;
            }
            else
                break;
        }
        else {
            uchlen = _pcutils_utf8_skip[(unsigned char)c];
            if (uchlen > 1) {
                ungetc(c, entity->fp);
                int n = (int)fread(utf8ch, 1, uchlen, entity->fp);
                if (n < uchlen) {
                    PC_WARN("fread() returns less bytes %d for a uchar (%d)\n",
                            n, uchlen);
                    if (feof(entity->fp) || ferror(entity->fp)) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            clearerr(entity->fp);
                            break;
                        }

                        if (total_ucs == 0)
                            goto nothing;
                        else
                            break;
                    }
                    else {
                        for (int i = 0; i < n; i++)
                            ungetc(utf8ch[n - i - 1], entity->fp);
                        break;
                    }
                }

            }
            else {
                utf8ch[0] = (unsigned char)c;
            }
        }

        total_ucs++;
        pcutils_mystring_append_mchar(&mystr, utf8ch, uchlen);

        if (total_ucs > 0 && (total_ucs % sep_ucs) == 0 &&
                strncmp(mystr.buff + mystr.nr_bytes - sep_len,
                    line_seperator, sep_len) == 0) {
            mystr.buff[mystr.nr_bytes - sep_len] = 0;

            purc_variant_t var;
            var = purc_variant_make_string_reuse_buff(mystr.buff,
                    mystr.sz_space, false);
            pcutils_mystring_init(&mystr);

            purc_variant_array_append(array, var);
            purc_variant_unref(var);
            line_num--;
        }
    }

    if (mystr.nr_bytes > 0) {
        purc_variant_t var;
        pcutils_mystring_done(&mystr);
        var = purc_variant_make_string_reuse_buff(mystr.buff,
                mystr.sz_space, false);
        purc_variant_array_append(array, var);
        purc_variant_unref(var);
    }

    return 0;

nothing:
    if (entity->type == STREAM_TYPE_FILE || entity->type == STREAM_TYPE_MEM) {
        PC_WARN("Reached the EOF.\n");
        purc_set_error(PURC_ERROR_NO_DATA);
    }
    else {
        PC_WARN("The peer has been closed.\n");
        purc_set_error(PURC_ERROR_BROKEN_PIPE);
    }

failed:
    return -1;
}
#endif

#define NEWLINE_SEPERATOR   "\n"

static purc_variant_t
readlines_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);

    struct pcdvobjs_stream *stream;
    purc_rwstream_t rwstream = NULL;
    uint64_t line_num = 0;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    stream = get_stream(native_entity);
    rwstream = stream->stm4r;
    if (rwstream == NULL) {
        PC_ERROR("rwstream is null\n");
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    ret_var = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (!ret_var) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }

    if (!purc_variant_cast_to_ulongint(argv[0], &line_num, false)) {
        PC_ERROR("failed purc_variant_cast_to_longint()\n");
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    if (line_num == 0)
        line_num = UINT64_MAX; // Read all available lines

    const char *line_seperator = NEWLINE_SEPERATOR;
    if (nr_args > 1) {
        size_t len;
        line_seperator = purc_variant_get_string_const_ex(argv[1], &len);
        if (line_seperator == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto out;
        }

        if (len == 0) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto out;
        }
    }

    if (line_num > 0) {
        int ret = read_lines(stream, line_num, ret_var, line_seperator);
        if (ret != 0) {
            goto out;
        }
    }

    return ret_var;

out:
    if (ret_var) {
        purc_variant_unref(ret_var);
    }

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_null();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
writelines_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    struct pcdvobjs_stream *stream;
    purc_rwstream_t rwstream = NULL;
    purc_variant_t data;
    ssize_t nr_total = 0, nr_written;

    stream = get_stream(native_entity);
    rwstream = stream->stm4w;
    if (rwstream == NULL) {
        PC_ERROR("The stream (%p) is not writable.\n", stream);
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    const char *lt = NEWLINE_SEPERATOR;
    size_t sz_lt = sizeof(NEWLINE_SEPERATOR) - 1;

    /* Only if the first argument is a linear container */
    if (purc_variant_linear_container_get_size(argv[0]) >= 0 &&
            nr_args > 1) {
        lt = purc_variant_get_string_const_ex(argv[1], &sz_lt);
        if (lt == NULL) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    for (size_t i = 0; i < nr_args; i++) {
        data = argv[i];
        enum purc_variant_type type = purc_variant_get_type(data);

        switch (type) {
        case PURC_VARIANT_TYPE_STRING:
            break;
        case PURC_VARIANT_TYPE_ARRAY:
        case PURC_VARIANT_TYPE_SET:
        case PURC_VARIANT_TYPE_TUPLE:
            {
                size_t sz_container =
                    purc_variant_linear_container_get_size(data);
                for (size_t i = 0; i < sz_container; i++) {
                    if (!purc_variant_is_string(
                                purc_variant_linear_container_get(data, i))) {
                        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                        goto done;
                    }
                }
            }
            break;
        default:
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto done;
        }

        const char *buf = NULL;
        size_t sz_buf = 0;
        if ((buf = purc_variant_get_string_const_ex(data, &sz_buf))) {
            if (buf && sz_buf > 0) {
                nr_written = purc_rwstream_write(rwstream, buf, sz_buf);
                if (nr_written < 0) {
                    goto done;
                }
                nr_total += nr_written;

            }

            nr_written = purc_rwstream_write(rwstream, lt, sz_lt);
            if (nr_written < 0) {
                goto done;
            }
            nr_total += nr_written;
        }
        else {
            size_t sz_container = purc_variant_linear_container_get_size(data);
            for (size_t i = 0; i < sz_container; i++) {
                purc_variant_t var = purc_variant_linear_container_get(data, i);
                buf = purc_variant_get_string_const_ex(var, &sz_buf);

                if (buf && sz_buf > 0) {
                    nr_written = purc_rwstream_write(rwstream, buf, sz_buf);
                    if (nr_written < 0) {
                        goto done;
                    }
                    nr_total += nr_written;
                }
                nr_written = purc_rwstream_write(rwstream, lt, sz_lt);
                if (nr_written < 0) {
                    goto done;
                }
                nr_total += nr_written;
            }
        }

    }

done:
    if (nr_total == 0 && nr_written < 0) {
        switch (errno) {
            case EPIPE:
                purc_set_error(PURC_ERROR_BROKEN_PIPE);
                goto failed;
            case EDQUOT:
            case ENOSPC:
                purc_set_error(PCRWSTREAM_ERROR_NO_SPACE);
                goto failed;
            case EPERM:
                purc_set_error(PURC_ERROR_ACCESS_DENIED);
                goto failed;
            case EFBIG:
                purc_set_error(PURC_ERROR_TOO_LARGE_ENTITY);
                goto failed;
            case EIO:
                purc_set_error(PURC_ERROR_IO_FAILURE);
                goto failed;
            case EAGAIN:
#if EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK:
#endif
            default:
                break;
        }
    }

    return purc_variant_make_longint(nr_total);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
readbytes_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    struct pcdvobjs_stream *stream;
    purc_rwstream_t rwstream = NULL;
    uint64_t byte_num = 0;

    stream = get_stream(native_entity);
    rwstream = stream->stm4r;
    if (rwstream == NULL) {
        PC_ERROR("The stream (%p) is not readable.\n", stream);
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }

    if (!purc_variant_cast_to_ulongint(argv[0], &byte_num, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    if (byte_num == 0) {
        ret_var = purc_variant_make_byte_sequence_empty();
    }
    else {
        char *content = malloc(byte_num);
        ssize_t size = 0;

        if (content == NULL) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto out;
        }

        // purc_clr_error();
        size = purc_rwstream_read(rwstream, content, byte_num);
        if (size > 0) {
            ret_var = purc_variant_make_byte_sequence_reuse_buff(content,
                    size, byte_num);
        }
        else {
            free(content);

            if (size == 0) {
                if (stream->type == STREAM_TYPE_FILE ||
                        stream->type == STREAM_TYPE_MEM) {
                    purc_set_error(PURC_ERROR_NO_DATA);
                    goto out;
                }
                else {
                    purc_set_error(PURC_ERROR_BROKEN_PIPE);
                    goto out;
                }
            }
            else {  /* size < 0 */
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    ret_var = purc_variant_make_byte_sequence_empty();
                }
                else {
                    purc_set_error(PURC_ERROR_IO_FAILURE);
                    goto out;
                }
            }
        }
    }

    return ret_var;

out:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_null();
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
readbytes2buffer_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    struct pcdvobjs_stream *stream;
    purc_rwstream_t rwstream = NULL;

    stream = get_stream(native_entity);
    rwstream = stream->stm4r;
    if (rwstream == NULL) {
        PC_ERROR("The stream (%p) is not readable.\n", stream);
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }

    unsigned char *buf;
    size_t nr_bytes, sz_buf;
    buf = purc_variant_bsequence_buffer(argv[0], &nr_bytes, &sz_buf);
    if (buf == NULL) {

        if (purc_variant_is_type(argv[0], PURC_VARIANT_TYPE_BSEQUENCE))
            /* read-only */
            purc_set_error(PURC_ERROR_INVALID_VALUE);
        else
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);

        goto out;
    }

    uint64_t nr_to_read = 0;
    if (nr_args > 1) {
        if (!purc_variant_cast_to_ulongint(argv[1], &nr_to_read, false)) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto out;
        }
    }

    if (nr_to_read == 0) {
        nr_to_read = sz_buf - nr_bytes;
    }

    if (nr_bytes + nr_to_read > sz_buf) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    ssize_t sz_read = 0;
    sz_read = purc_rwstream_read(rwstream, buf + nr_bytes, nr_to_read);
    if (sz_read > 0) {
        purc_variant_bsequence_set_bytes(argv[0], nr_bytes + sz_read);
    }
    else {
        if (sz_read == 0) {
            if (stream->type == STREAM_TYPE_FILE ||
                    stream->type == STREAM_TYPE_MEM) {
                purc_set_error(PURC_ERROR_NO_DATA);
                goto out;
            }
            else {
                purc_set_error(PURC_ERROR_BROKEN_PIPE);
                goto out;
            }
        }
        else if (sz_read < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                purc_set_error(PURC_ERROR_IO_FAILURE);
                goto out;
            }
        }
    }

    return purc_variant_make_longint(sz_read);

out:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
writebytes_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);

    struct pcdvobjs_stream *stream;
    purc_rwstream_t rwstream = NULL;
    purc_variant_t data;

    stream = get_stream(native_entity);
    rwstream = stream->stm4w;
    if (rwstream == NULL) {
        PC_ERROR("The stream (%p) is not writable.\n", stream);
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }
    data = argv[0];

    if (data == PURC_VARIANT_INVALID ||
            (!purc_variant_is_bsequence(data) &&
             !purc_variant_is_string(data))
            ) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    size_t bsize = 0;
    const unsigned char *buffer = NULL;
    if (purc_variant_is_bsequence(data)) {
        buffer = purc_variant_get_bytes_const(data, &bsize);
    }
    else {
        buffer = (const unsigned char *)purc_variant_get_string_const_ex(
                data, &bsize);
        // bsize = bsize + 1; do not write terminating null byte.
    }

    ssize_t nr_written;
    if (buffer && bsize) {
        nr_written = purc_rwstream_write(rwstream, buffer, bsize);
        if (nr_written == -1) {
            switch (errno) {
                case EPIPE:
                    purc_set_error(PURC_ERROR_BROKEN_PIPE);
                    goto failed;
                case EDQUOT:
                case ENOSPC:
                    purc_set_error(PCRWSTREAM_ERROR_NO_SPACE);
                    goto failed;
                case EPERM:
                    purc_set_error(PURC_ERROR_ACCESS_DENIED);
                    goto failed;
                case EFBIG:
                    purc_set_error(PURC_ERROR_TOO_LARGE_ENTITY);
                    goto failed;
                case EIO:
                    purc_set_error(PURC_ERROR_IO_FAILURE);
                    goto failed;
                case EAGAIN:
#if EWOULDBLOCK != EAGAIN
                case EWOULDBLOCK:
#endif
                default:
                    break;
            }
        }
    }
    else {
        nr_written = 0;
    }

    return purc_variant_make_longint(nr_written);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
writeeof_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    struct pcdvobjs_stream *stream;

    if (native_entity == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    stream = get_stream(native_entity);
    if (stream->type != STREAM_TYPE_PIPE) {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        goto out;
    }

    bool ret;
    if (stream->stm4w) {
        purc_rwstream_destroy(stream->stm4w);
        stream->stm4w = NULL;
        close(stream->fd4w);
        stream->fd4w = -1;
        ret = true;
    }
    else
        ret = false;

    return purc_variant_make_boolean(ret);

out:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
status_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    struct pcdvobjs_stream *stream;

    if (native_entity == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    stream = get_stream(native_entity);
    if (stream->type != STREAM_TYPE_PIPE) {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        goto out;
    }

    const char *status;
    int value = 0, wstatus;
    int ret = waitpid(stream->cpid, &wstatus, WNOHANG);

    if (ret == 0) {
        status = "running";
        value = 0;
    }
    else if (ret == -1) {
        if (errno == ECHILD) {
            status = "not-exist";
            value = 0;
        }
        else {
            purc_set_error(purc_error_from_errno(errno));
            goto out;
        }
    }
    else {

        if (WIFEXITED(wstatus)) {
            status = "exited";
            value = WEXITSTATUS(wstatus);
        }
        else if (WIFSIGNALED(wstatus)) {
            value = WEXITSTATUS(wstatus);
            if (WCOREDUMP(wstatus)) {
                status = "signaled-coredump";
            }
            else {
                status = "signaled";
            }
        }
    }

    purc_variant_t status_val = purc_variant_make_string_static(status, false);
    purc_variant_t value_val = purc_variant_make_longint(value);
    purc_variant_t val = purc_variant_make_array(2, status_val, value_val);
    purc_variant_unref(status_val);
    purc_variant_unref(value_val);

    return val;
out:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
seek_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    struct pcdvobjs_stream *stream;
    purc_rwstream_t rwstream;
    int64_t byte_num = 0;
    off_t off = 0;
    int64_t whence = 0;
    const char* option = _KW_set;

    if (native_entity == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    stream = get_stream(native_entity);
    if (stream->type == STREAM_TYPE_PIPE) { /* no support */
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        goto out;
    }

    rwstream = stream->stm4r;
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }
    else if (nr_args > 1) {
        if (argv[1] != PURC_VARIANT_INVALID &&
                (!purc_variant_is_string(argv[1]))) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto out;
        }
        option = purc_variant_get_string_const(argv[1]);
    }

    if (argv[0] != PURC_VARIANT_INVALID &&
            !purc_variant_cast_to_longint(argv[0], &byte_num, false)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    purc_atom_t atom = purc_atom_try_string_ex(STREAM_ATOM_BUCKET, option);
    if (atom == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (atom == keywords2atoms[K_KW_set].atom) {
        whence = SEEK_SET;
    }
    else if (atom == keywords2atoms[K_KW_current].atom) {
        whence = SEEK_CUR;
    }
    else if (atom == keywords2atoms[K_KW_end].atom) {
        whence = SEEK_END;
    }

    off = purc_rwstream_seek(rwstream, byte_num, (int)whence);
    if (off < 0) {
        goto out;
    }

    return purc_variant_make_ulongint((uint64_t)off);

out:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
getuc_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);

    int ec = PURC_ERROR_OK;
    struct pcdvobjs_stream *stream;
    purc_rwstream_t rwstream;
    const char *encoding = "utf8";
    const char *format = "utf8";
    uint32_t codepoint = 0;
    uint8_t utf8_bytes[8];
    uint32_t utf8_len = 0;
    ssize_t nr_read = 0;

    stream = get_stream(native_entity);
    rwstream = stream->stm4r;
    if (rwstream == NULL) {
        ec = PURC_ERROR_NOT_DESIRED_ENTITY;
        goto failed;
    }

    // Parse encoding parameter
    if (nr_args > 0) {
        if (!purc_variant_is_string(argv[0])) {
            ec = PURC_ERROR_WRONG_DATA_TYPE;
            goto failed;
        }
        encoding = purc_variant_get_string_const(argv[0]);
    }

    // Parse format parameter 
    if (nr_args > 1) {
        if (!purc_variant_is_string(argv[1])) {
            ec = PURC_ERROR_WRONG_DATA_TYPE;
            goto failed;
        }
        format = purc_variant_get_string_const(argv[1]);
    }

    // Read character according to encoding
    if (strcmp(encoding, "utf8") == 0) {
        if ((nr_read = purc_rwstream_read_utf8_char(rwstream,
                        (char *)utf8_bytes, NULL)) <= 0) {
            goto ioerr;
        }

        utf8_len = nr_read;
        utf8_bytes[utf8_len] = '\0';
        codepoint = pcutils_utf8_to_unichar(utf8_bytes);
    }
    else if (strcmp(encoding, "utf16le") == 0) {
        uint8_t bytes[2];
        if ((nr_read = purc_rwstream_read(rwstream, bytes, 2)) != 2) {
            goto ioerr;
        }

#define MAKEWORD16(low, high)   \
    ((uint16_t)(((uint8_t)(low)) | (((uint16_t)((uint8_t)(high))) << 8)))

        uint16_t w1, w2;
        w1 = MAKEWORD16(bytes[0], bytes[1]); 
        if (w1 < 0xD800 || w1 > 0xDFFF) {
            codepoint = w1;
        }
        else if (w1 >= 0xD800 && w1 <= 0xDBFF) {
            if ((nr_read = purc_rwstream_read(rwstream, bytes, 2)) != 2) {
                goto ioerr;
            }

            w2 = MAKEWORD16(bytes[0], bytes[1]);
            if (w2 < 0xDC00 || w2 > 0xDFFF) {
                ec = PURC_ERROR_BAD_ENCODING;
                goto failed;
            }

            codepoint = ((w1 - 0xD800) << 10) + (w2 - 0xDC00) + 0x10000;
        }
        else {
            ec = PURC_ERROR_BAD_ENCODING;
            goto failed;
        }

        utf8_len = pcutils_unichar_to_utf8(codepoint, (uint8_t*)utf8_bytes);
        utf8_bytes[utf8_len] = '\0';
    }
    else if (strcmp(encoding, "utf16be") == 0) {
        uint8_t bytes[2];
        if ((nr_read = purc_rwstream_read(rwstream, bytes, 2)) != 2) {
            goto ioerr;
        }

        uint16_t w1, w2;
        w1 = MAKEWORD16(bytes[1], bytes[0]);
        if (w1 < 0xD800 || w1 > 0xDFFF) {
            codepoint = w1;
        }
        else if (w1 >= 0xD800 && w1 <= 0xDBFF) {
            if ((nr_read = purc_rwstream_read(rwstream, bytes, 2)) != 2) {
                goto ioerr;
            }

            w2 = MAKEWORD16(bytes[1], bytes[0]);
            if (w2 < 0xDC00 || w2 > 0xDFFF) {
                ec = PURC_ERROR_BAD_ENCODING;
                goto failed;
            }

            codepoint = ((w1 - 0xD800) << 10) | (w2 - 0xDC00);
            codepoint += 0x10000;
        }
        else {
            ec = PURC_ERROR_BAD_ENCODING;
            goto failed;
        }

        utf8_len = pcutils_unichar_to_utf8(codepoint, (uint8_t*)utf8_bytes);
        utf8_bytes[utf8_len] = '\0';
    }
    else if (strcmp(encoding, "utf32le") == 0) {
        uint8_t bytes[4];
        if ((nr_read = purc_rwstream_read(rwstream, bytes, 4)) != 4) {
            goto ioerr;
        }

#define MAKEDWORD32(first, second, third, fourth)       \
    ((uint32_t)(                                        \
        ((uint8_t)(first)) |                            \
        (((uint32_t)((uint8_t)(second))) << 8) |        \
        (((uint32_t)((uint8_t)(third))) << 16) |        \
        (((uint32_t)((uint8_t)(fourth))) << 24)         \
    ))

        codepoint = MAKEDWORD32(bytes[0], bytes[1], bytes[2], bytes[3]);
        utf8_len = pcutils_unichar_to_utf8(codepoint, utf8_bytes);
        utf8_bytes[utf8_len] = '\0';
    }
    else if (strcmp(encoding, "utf32be") == 0) {
        uint8_t bytes[4];
        if ((nr_read = purc_rwstream_read(rwstream, bytes, 4)) != 4) {
            goto ioerr;
        }

        codepoint = MAKEDWORD32(bytes[3], bytes[2], bytes[1], bytes[0]);
        utf8_len = pcutils_unichar_to_utf8(codepoint, utf8_bytes);
        utf8_bytes[utf8_len] = '\0';
    }
    else {
        ec = PURC_ERROR_INVALID_VALUE;
        goto failed;
    }

    // Return result according to format
    if (strcmp(format, "codepoint") == 0) {
        return purc_variant_make_ulongint(codepoint);
    }
    else if (strcmp(format, "utf8") == 0) {
        return purc_variant_make_string_ex(
                (const char *)utf8_bytes, utf8_len, false);
    }
    else {
        ec = PURC_ERROR_INVALID_VALUE;
        goto failed;
    }

ioerr:
    if (nr_read == 0) {
        if (stream->type == STREAM_TYPE_FILE ||
                stream->type == STREAM_TYPE_MEM)
            ec = PURC_ERROR_NO_DATA;
        else
            ec = PURC_ERROR_IO_FAILURE;
    }
    else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            ec = PURC_ERROR_AGAIN;
        }
        else {
            ec = PURC_ERROR_IO_FAILURE;
        }
    }

failed:
    if (ec != PURC_ERROR_OK)
        purc_set_error(ec);
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

/*
$stream.putuc(
    < string | ulongint $char: `The character to be written.` >
    [,
        < 'utf8 | utf16le | utf16be | utf32le | utf32be' $encoding = 'utf8':
            - 'utf8':     `The character will be encoded in UTF-8 before writing.`
            - 'utf16le':  `The character will be encoded in UTF-16 little endian before writing.`
            - 'utf16be':  `The character will be encoded in UTF-16 big endian before writing.`
            - 'utf32le':  `The character will be encoded in UTF-32 little endian before writing.`
            - 'utf32be':  `The character will be encoded in UTF-32 big endian before writing.` >
    ]
) ulongint | false
*/
static purc_variant_t
putuc_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);

    int ec = PURC_ERROR_OK;
    struct pcdvobjs_stream *stream;
    purc_rwstream_t rwstream;
    int uc = 0;
    const char *encoding = "utf8";

    stream = get_stream(native_entity);
    rwstream = stream->stm4w;
    if (rwstream == NULL) {
        ec = PURC_ERROR_NOT_DESIRED_ENTITY;
        goto failed;
    }

    if (nr_args < 1) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto failed;
    }

    // Get character codepoint from first argument
    const char *str = NULL;
    if ((str = purc_variant_get_string_const(argv[0]))) {
        uc = pcutils_utf8_to_unichar((const unsigned char*)str);
    }
    else if (purc_variant_cast_to_int32(argv[0], &uc, false)) {
    }
    else {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto failed;
    }

    if (uc < 0 || uc > 0x10FFFF) {
        ec = PURC_ERROR_INVALID_VALUE;
        goto failed;
    }

    // Get encoding from second argument if present
    if (nr_args > 1 &&
            (encoding = purc_variant_get_string_const(argv[1])) == NULL) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto failed;
    }

    unsigned char buff[8];
    size_t bytes_written = 0;

    // Handle different encodings
    if (strcmp(encoding, "utf8") == 0) {
        int len = pcutils_unichar_to_utf8(uc, buff);
        bytes_written = purc_rwstream_write(rwstream, buff, len);
    }
    else if (strcmp(encoding, "utf16le") == 0) {
        if (uc <= 0xFFFF) {
            buff[0] = uc & 0xFF;
            buff[1] = (uc >> 8) & 0xFF;
            bytes_written = purc_rwstream_write(rwstream, buff, 2);
        }
        else {
            // Handle surrogate pairs for characters above U+FFFF
            uint32_t code = uc - 0x10000;
            uint16_t w1 = 0xD800 | (code >> 10);
            uint16_t w2 = 0xDC00 | (code & 0x3FF);

            buff[0] = w1 & 0xFF;
            buff[1] = (w1 >> 8) & 0xFF;
            buff[2] = w2 & 0xFF;
            buff[3] = (w2 >> 8) & 0xFF;
            bytes_written = purc_rwstream_write(rwstream, buff, 4);
        }
    }
    else if (strcmp(encoding, "utf16be") == 0) {
        if (uc <= 0xFFFF) {
            buff[0] = (uc >> 8) & 0xFF;
            buff[1] = uc & 0xFF;
            bytes_written = purc_rwstream_write(rwstream, buff, 2);
        }
        else {
            // Handle surrogate pairs for characters above U+FFFF
            uint32_t code = uc - 0x10000;
            uint16_t w1 = 0xD800 | (code >> 10);
            uint16_t w2 = 0xDC00 | (code & 0x3FF);

            buff[1] = w1 & 0xFF;
            buff[0] = (w1 >> 8) & 0xFF;
            buff[3] = w2 & 0xFF;
            buff[2] = (w2 >> 8) & 0xFF;
            bytes_written = purc_rwstream_write(rwstream, buff, 4);
        }
    }
    else if (strcmp(encoding, "utf32le") == 0) {
        buff[0] = uc & 0xFF;
        buff[1] = (uc >> 8) & 0xFF;
        buff[2] = (uc >> 16) & 0xFF;
        buff[3] = (uc >> 24) & 0xFF;
        bytes_written = purc_rwstream_write(rwstream, buff, 4);
    }
    else if (strcmp(encoding, "utf32be") == 0) {
        buff[0] = (uc >> 24) & 0xFF;
        buff[1] = (uc >> 16) & 0xFF;
        buff[2] = (uc >> 8) & 0xFF;
        buff[3] = uc & 0xFF;
        bytes_written = purc_rwstream_write(rwstream, buff, 4);
    }
    else {
        ec = PURC_ERROR_INVALID_VALUE;
        goto failed;
    }

    if (bytes_written == 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            ec = PURC_ERROR_AGAIN;
        }
        else {
            ec = PURC_ERROR_IO_FAILURE;
        }
        goto failed;
    }

    return purc_variant_make_ulongint(bytes_written);

failed:
    if (ec != PURC_ERROR_OK)
        purc_set_error(ec);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    return PURC_VARIANT_INVALID;
}

/*
$stream.ungetuc(
    < string | ulongint $char: `The character to be put back.` >
    [,
        < 'utf8 | utf16le | utf16be | utf32le | utf32be' $encoding = 'utf8':
            - 'utf8':     `The character will be encoded in UTF-8 before putting back.`
            - 'utf16le':  `The character will be encoded in UTF-16 little endian before putting back.`
            - 'utf16be':  `The character will be encoded in UTF-16 big endian before putting back.`
            - 'utf32le':  `The character will be encoded in UTF-32 little endian before putting back.`
            - 'utf32be':  `The character will be encoded in UTF-32 big endian before putting back.` >
    ]
) ulonging | false
*/
static purc_variant_t
ungetuc_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);

    int ec = PURC_ERROR_OK;
    struct pcdvobjs_stream *stream;
    purc_rwstream_t rwstream;
    int uc = 0;
    const char *encoding = "utf8";

    stream = get_stream(native_entity);
    rwstream = stream->stm4r;
    if (rwstream == NULL) {
        ec = PURC_ERROR_NOT_DESIRED_ENTITY;
        goto failed;
    }

    if (nr_args < 1) {
        ec = PURC_ERROR_ARGUMENT_MISSED;
        goto failed;
    }

    // Get character codepoint from first argument
    const char *str = NULL;
    if ((str = purc_variant_get_string_const(argv[0]))) {
        uc = pcutils_utf8_to_unichar((const unsigned char*)str);
    }
    else if (purc_variant_cast_to_int32(argv[0], &uc, false)) {
    }
    else {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto failed;
    }

    if (uc < 0 || uc > 0x10FFFF) {
        ec = PURC_ERROR_INVALID_VALUE;
        goto failed;
    }

    // Get encoding from second argument if present
    if (nr_args > 1 &&
            (encoding = purc_variant_get_string_const(argv[1])) == NULL) {
        ec = PURC_ERROR_WRONG_DATA_TYPE;
        goto failed;
    }

    char buff[8];
    int len = 0;

    // Handle different encodings
    if (strcmp(encoding, "utf8") == 0) {
        len = pcutils_unichar_to_utf8(uc, (uint8_t *)buff);
    }
    else if (strcmp(encoding, "utf16le") == 0) {
        if (uc <= 0xFFFF) {
            buff[0] = uc & 0xFF;
            buff[1] = (uc >> 8) & 0xFF;
            len = 2;
        }
        else {
            // Handle surrogate pairs for characters above U+FFFF
            uint32_t code = uc - 0x10000;
            uint16_t w1 = 0xD800 | (code >> 10);
            uint16_t w2 = 0xDC00 | (code & 0x3FF);

            buff[0] = w1 & 0xFF;
            buff[1] = (w1 >> 8) & 0xFF;
            buff[2] = w2 & 0xFF;
            buff[3] = (w2 >> 8) & 0xFF;
            len = 4;
        }
    }
    else if (strcmp(encoding, "utf16be") == 0) {
        if (uc <= 0xFFFF) {
            buff[0] = (uc >> 8) & 0xFF;
            buff[1] = uc & 0xFF;
            len = 2;
        }
        else {
            // Handle surrogate pairs for characters above U+FFFF
            uint32_t code = uc - 0x10000;
            uint16_t w1 = 0xD800 | (code >> 10);
            uint16_t w2 = 0xDC00 | (code & 0x3FF);

            buff[1] = w1 & 0xFF;
            buff[0] = (w1 >> 8) & 0xFF;
            buff[3] = w2 & 0xFF;
            buff[2] = (w2 >> 8) & 0xFF;
            len = 4;
        }
    }
    else if (strcmp(encoding, "utf32le") == 0) {
        buff[0] = uc & 0xFF;
        buff[1] = (uc >> 8) & 0xFF;
        buff[2] = (uc >> 16) & 0xFF;
        buff[3] = (uc >> 24) & 0xFF;
        len = 4;
    }
    else if (strcmp(encoding, "utf32be") == 0) {
        buff[0] = (uc >> 24) & 0xFF;
        buff[1] = (uc >> 16) & 0xFF;
        buff[2] = (uc >> 8) & 0xFF;
        buff[3] = uc & 0xFF;
        len = 4;
    }
    else {
        ec = PURC_ERROR_INVALID_VALUE;
        goto failed;
    }

    if (purc_rwstream_ungetc(rwstream, buff, len) != len) {
        goto failed;
    }

    return purc_variant_make_ulongint(len);

failed:
    if (ec != PURC_ERROR_OK)
        purc_set_error(ec);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
close_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    PC_ASSERT(native_entity);

    struct pcdvobjs_stream *stream = get_stream(native_entity);
    native_stream_close(stream);

    return purc_variant_make_boolean(true);
}

static purc_variant_t
fd_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    PC_ASSERT(native_entity);

    struct pcdvobjs_stream *stream = get_stream(native_entity);
    int fd = -1;
    if (stream->fd4r >= 0) {
        fd = stream->fd4r;
    }
    else if (stream->fd4w >= 0) {
        fd = stream->fd4w;
    }

    return purc_variant_make_longint(fd);
}

static purc_variant_t
peer_addr_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    PC_ASSERT(native_entity);

    struct pcdvobjs_stream *stream = get_stream(native_entity);
    if (stream->peer_addr)
        return purc_variant_make_string(stream->peer_addr, false);

    return purc_variant_make_null();
}

static purc_variant_t
peer_port_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    PC_ASSERT(native_entity);

    struct pcdvobjs_stream *stream = get_stream(native_entity);
    if (stream->peer_port)
        return purc_variant_make_string(stream->peer_port, false);

    return purc_variant_make_null();
}

static bool
stream_io_callback(int fd, int event, void *ctxt)
{
    UNUSED_PARAM(fd);

    struct pcdvobjs_stream *stream = (struct pcdvobjs_stream*)ctxt;
    PC_ASSERT(stream);

    if (stream->cid == 0) {
        PC_ERROR("Got an IO event for stream not bound to a coroutine.\n");
        return false;
    }

    if ((event & PCRUNLOOP_IO_HUP)) {
        if ((fd == stream->fd4r && stream->ioevents4r & PCRUNLOOP_IO_HUP) ||
                (fd == stream->fd4w && stream->ioevents4w & PCRUNLOOP_IO_HUP)) {
            pcintr_coroutine_post_event(stream->cid,
                    PCRDR_MSG_EVENT_REDUCE_OPT_IGNORE,
                    stream->observed,
                    STREAM_EVENT_NAME, STREAM_SUB_EVENT_HANGUP,
                    PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        }
        else {
            PC_WARN("Got an IO event HUP for stream (%p), but not observed.\n",
                    stream);
        }
    }

    if ((event & PCRUNLOOP_IO_ERR)) {
        if ((fd == stream->fd4r && stream->ioevents4r & PCRUNLOOP_IO_ERR) ||
                (fd == stream->fd4w && stream->ioevents4w & PCRUNLOOP_IO_ERR)) {
            pcintr_coroutine_post_event(stream->cid,
                    PCRDR_MSG_EVENT_REDUCE_OPT_IGNORE,
                    stream->observed,
                    STREAM_EVENT_NAME, STREAM_SUB_EVENT_ERROR,
                    PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        }
        else {
            PC_WARN("Got an IO event ERR for stream (%p), but not observed.\n",
                    stream);
        }
    }

    if ((event & PCRUNLOOP_IO_HUP) || (event & PCRUNLOOP_IO_ERR) ||
            (event & PCRUNLOOP_IO_NVAL)) {
        if (fd == stream->fd4r) {
            stream->monitor4r = 0;
        }

        if (fd == stream->fd4w) {
            stream->monitor4w = 0;
        }

        if (event & PCRUNLOOP_IO_NVAL) {
            PC_ERROR("Got a weird IO NVAL event for stream (%p).\n", stream);
        }

        return false;
    }

    if (event & PCRUNLOOP_IO_IN) {
        if (stream->ioevents4r & PCRUNLOOP_IO_IN) {
            pcintr_coroutine_post_event(stream->cid,
                    PCRDR_MSG_EVENT_REDUCE_OPT_IGNORE,
                    stream->observed,
                    STREAM_EVENT_NAME, STREAM_SUB_EVENT_READABLE,
                    PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        }
        else {
            PC_WARN("Got an IO event IN for stream (%p), but not observed.\n",
                    stream);
        }
    }

    if (event & PCRUNLOOP_IO_OUT) {
        if (stream->ioevents4w & PCRUNLOOP_IO_OUT) {
            pcintr_coroutine_post_event(stream->cid,
                    PCRDR_MSG_EVENT_REDUCE_OPT_IGNORE,
                    stream->observed,
                    STREAM_EVENT_NAME, STREAM_SUB_EVENT_WRITABLE,
                    PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        }
        else {
            PC_WARN("Got an IO event OUT for stream (%p), but not observed.\n",
                    stream);
        }
    }

    return true;
}

static bool handle_runloop_monitors(struct pcdvobjs_stream *stream,
        int ioevents4r, int ioevents4w)
{
    if (stream->fd4r >= 0) {
        if (stream->fd4r == stream->fd4w) {
            ioevents4r |= ioevents4w;
        }

        if (ioevents4r != stream->ioevents4r) {
            if (stream->monitor4r) {
                purc_runloop_remove_fd_monitor(purc_runloop_get_current(),
                        stream->monitor4r);
                stream->monitor4r = 0;
                stream->ioevents4r = 0;
            }

            if (ioevents4r) {
                stream->monitor4r = purc_runloop_add_fd_monitor(
                        purc_runloop_get_current(), stream->fd4r, ioevents4r,
                        stream_io_callback, stream);
                if (stream->monitor4r == 0) {
                    PC_ERROR("Failed purc_runloop_add_fd_monitor(STREAM, IN)\n");
                    return false;
                }
                else {
                    stream->ioevents4r = ioevents4r;
                    if (stream->fd4w == stream->fd4r) {
                        stream->ioevents4w = stream->ioevents4r;
                    }
                }
            }
        }
    }
    else if (stream->fd4w > 0 && stream->fd4w != stream->fd4r) {
        if (ioevents4w != stream->ioevents4w) {
            if (stream->monitor4w) {
                purc_runloop_remove_fd_monitor(purc_runloop_get_current(),
                        stream->monitor4w);
                stream->monitor4w = 0;
                stream->ioevents4w = 0;
            }

            if (ioevents4w) {
                stream->monitor4w = purc_runloop_add_fd_monitor(
                        purc_runloop_get_current(), stream->fd4w, ioevents4w,
                        stream_io_callback, stream);
                if (stream->monitor4w == 0) {
                    PC_ERROR("Failed purc_runloop_add_fd_monitor(STREAM, OUT)\n");
                    return false;
                }
                else {
                    stream->ioevents4w = ioevents4w;
                }
            }
        }
    }

    return true;
}

static const char *stream_events[] = {
#define MATCHED_READABLE    0x01
    STREAM_EVENT_NAME ":" STREAM_SUB_EVENT_READABLE,
#define MATCHED_WRITABLE    0x02
    STREAM_EVENT_NAME ":" STREAM_SUB_EVENT_WRITABLE,
#define MATCHED_HANGUP      0x04
    STREAM_EVENT_NAME ":" STREAM_SUB_EVENT_HANGUP,
#define MATCHED_ERROR       0x08
    STREAM_EVENT_NAME ":" STREAM_SUB_EVENT_ERROR,
};

static bool
on_observe(void *native_entity, const char *event_name,
        const char *event_subname)
{
    struct pcdvobjs_stream *stream = (struct pcdvobjs_stream*)native_entity;

    if (stream->type == STREAM_TYPE_MEM)
        return false;

    if (stream->cid == 0) {
        pcintr_coroutine_t co = pcintr_get_coroutine();
        if (co)
            stream->cid = co->cid;
        else
            return false;
    }

    int matched = pcdvobjs_match_events(event_name, event_subname,
            stream_events, PCA_TABLESIZE(stream_events));
    if (matched == -1)
        return false;

    int ioevents4r = stream->ioevents4r, ioevents4w = stream->ioevents4w;
    if ((matched & MATCHED_READABLE)) {
        ioevents4r |= PCRUNLOOP_IO_IN;
    }

    if ((matched & MATCHED_WRITABLE)) {
        ioevents4w |= PCRUNLOOP_IO_OUT;
    }

    if (matched & MATCHED_HANGUP) {
        ioevents4r |= PCRUNLOOP_IO_HUP;
        // macOS not allow to poll HUP and OUT at the same time.
        // ioevents4w |= PCRUNLOOP_IO_HUP;
    }

    if (matched & MATCHED_ERROR) {
        ioevents4r |= PCRUNLOOP_IO_ERR;
        ioevents4w |= PCRUNLOOP_IO_ERR;
    }

    return handle_runloop_monitors(stream, ioevents4r, ioevents4w);
}

static bool
on_forget(void *native_entity, const char *event_name,
        const char *event_subname)
{
    struct pcdvobjs_stream *stream = (struct pcdvobjs_stream*)native_entity;

    if (stream->type == STREAM_TYPE_MEM)
        return false;

    int matched = pcdvobjs_match_events(event_name, event_subname,
            stream_events, PCA_TABLESIZE(stream_events));
    PC_NONE("%s(%s, %s): %d\n", __func__, event_name, event_subname, matched);
    if (matched == -1)
        return false;

    int ioevents4r = stream->ioevents4r;
    int ioevents4w = stream->ioevents4w;

    if (matched & MATCHED_READABLE) {
        ioevents4r &= ~PCRUNLOOP_IO_IN;
    }

    if (matched & MATCHED_WRITABLE) {
        ioevents4w &= ~PCRUNLOOP_IO_OUT;
    }

    if (matched & MATCHED_HANGUP) {
        ioevents4r &= ~PCRUNLOOP_IO_HUP;
        ioevents4w &= ~PCRUNLOOP_IO_HUP;
    }

    if (matched & MATCHED_ERROR) {
        ioevents4r &= ~PCRUNLOOP_IO_ERR;
        ioevents4w &= ~PCRUNLOOP_IO_ERR;
    }

    return handle_runloop_monitors(stream, ioevents4r, ioevents4w);
}

static void
on_release(void *native_entity)
{
    dvobjs_stream_delete((struct pcdvobjs_stream *)native_entity);
}

static purc_nvariant_method
property_getter(void *entity, const char *name)
{
    struct pcdvobjs_stream *stream = entity;
    UNUSED_PARAM(entity);

    if (name == NULL) {
        goto failed;
    }

    purc_atom_t atom = purc_atom_try_string_ex(STREAM_ATOM_BUCKET, name);
    if (atom == 0) {
        goto failed;
    }

    if (atom == keywords2atoms[K_KW_fd].atom) {
        return fd_getter;
    }
    else if (atom == keywords2atoms[K_KW_peer_addr].atom) {
        return peer_addr_getter;
    }
    else if (atom == keywords2atoms[K_KW_peer_port].atom) {
        return peer_port_getter;
    }

    /* the following properties are not available if the entity
       has been extended */
    if (stream->ext0.signature[0]) {
        goto failed;
    }

    if (atom == keywords2atoms[K_KW_readstruct].atom) {
        return readstruct_getter;
    }
    else if (atom == keywords2atoms[K_KW_writestruct].atom) {
        return writestruct_getter;
    }
    else if (atom == keywords2atoms[K_KW_readlines].atom) {
        return readlines_getter;
    }
    else if (atom == keywords2atoms[K_KW_writelines].atom) {
        return writelines_getter;
    }
    else if (atom == keywords2atoms[K_KW_readbytes].atom) {
        return readbytes_getter;
    }
    else if (atom == keywords2atoms[K_KW_readbytes2buffer].atom) {
        return readbytes2buffer_getter;
    }
    else if (atom == keywords2atoms[K_KW_writebytes].atom) {
        return writebytes_getter;
    }
    else if (atom == keywords2atoms[K_KW_writeeof].atom) {
        return writeeof_getter;
    }
    else if (atom == keywords2atoms[K_KW_status].atom) {
        return status_getter;
    }
    else if (atom == keywords2atoms[K_KW_seek].atom) {
        return seek_getter;
    }
    else if (atom == keywords2atoms[K_KW_getuc].atom) {
        return getuc_getter;
    }
    else if (atom == keywords2atoms[K_KW_putuc].atom) {
        return putuc_getter;
    }
    else if (atom == keywords2atoms[K_KW_ungetuc].atom) {
        return ungetuc_getter;
    }
    else if (atom == keywords2atoms[K_KW_close].atom) {
        return close_getter;
    }

failed:
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

static bool is_fd_inet_socket(int fd)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if (getsockname(fd, (struct sockaddr *)&addr, &len) == 0 && len > 0) {
        return true;
    }

    return false;
}

static int get_stream_type_by_fd(int fd)
{
    struct stat stat;
    if (fstat(fd, &stat)) {
        PC_ERROR("Failed fstat(): %s\n", strerror(errno));
        return -1;
    }

    enum pcdvobjs_stream_type type = STREAM_TYPE_FILE;
    if (S_ISREG(stat.st_mode)) {
        type = STREAM_TYPE_FILE;
    }
    else if (S_ISFIFO(stat.st_mode)) {
        type = STREAM_TYPE_FIFO;
    }
    else if (S_ISSOCK(stat.st_mode)) {
        if (!is_fd_inet_socket(fd)) {
            type = STREAM_TYPE_UNIX;
        }
        else {
            type = STREAM_TYPE_INET;
        }
    }
    else if (S_ISDIR(stat.st_mode)
#ifdef S_ISWHT
            || S_ISWHT(stat.st_mode)
#endif
            ) {
        return -1;
    }

    return type;
}

static
struct pcdvobjs_stream *create_file_std_stream(enum pcdvobjs_stdio_type stdio)
{
    int fd = -1;
    switch (stdio) {
    case STDIO_TYPE_STDIN:
        fd = dup(STDIN_FILENO);
        break;

    case STDIO_TYPE_STDOUT:
        fd = dup(STDOUT_FILENO);
        break;

    case STDIO_TYPE_STDERR:
        fd = dup(STDERR_FILENO);
        break;
    }

    if (fd < 0) {
        PC_ERROR("Failed dup()\n");
        purc_set_error(PURC_ERROR_SYS_FAULT);
        return NULL;
    }

    int type = get_stream_type_by_fd(fd);
    if (type == -1) {
        purc_set_error(PURC_ERROR_NOT_DESIRED_ENTITY);
        close(fd);
        return NULL;
    }

    struct pcdvobjs_stream* stream = dvobjs_stream_new(type, NULL);
    if (!stream) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    stream->stm4r = purc_rwstream_new_from_unix_fd(fd);
    if (stream->stm4r == NULL) {
        goto out_free_stream;
    }
    stream->stm4w = stream->stm4r;
    stream->fd4r = fd;
    stream->fd4w = fd;

    return stream;

out_free_stream:
    dvobjs_stream_delete(stream);

out:
    close(fd);
    return NULL;
}

static inline
struct pcdvobjs_stream *create_file_stdin_stream()
{
    return create_file_std_stream(STDIO_TYPE_STDIN);
}

static inline
struct pcdvobjs_stream *create_file_stdout_stream()
{
    return create_file_std_stream(STDIO_TYPE_STDOUT);
}

static inline
struct pcdvobjs_stream *create_file_stderr_stream()
{
    return create_file_std_stream(STDIO_TYPE_STDERR);
}

static
bool file_exists(const char* file)
{
    struct stat filestat;
    return (0 == stat(file, &filestat));
}

static
bool file_exists_and_is_executable(const char* file)
{
    struct stat filestat;
    return (0 == stat(file, &filestat) && (filestat.st_mode & S_IRWXU));
}

static
int parse_from_option(purc_variant_t option)
{
    purc_atom_t atom = 0;
    size_t parts_len;
    const char *parts;
    int flags = 0;

    if (option == PURC_VARIANT_INVALID) {
        atom = keywords2atoms[K_KW_keep].atom;
    }
    else {
        parts = purc_variant_get_string_const_ex(option, &parts_len);
        if (parts == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto out;
        }

        parts = pcutils_trim_spaces(parts, &parts_len);
        if (parts_len == 0) {
            atom = keywords2atoms[K_KW_keep].atom;
        }
    }

    if (atom == 0) {
        char tmp[parts_len + 1];
        strncpy(tmp, parts, parts_len);
        tmp[parts_len]= '\0';
        atom = purc_atom_try_string_ex(STREAM_ATOM_BUCKET, tmp);
    }

    if (atom != keywords2atoms[K_KW_keep].atom) {
        size_t length = 0;
        const char *part = pcutils_get_next_token_len(parts, parts_len,
                PURC_KW_DELIMITERS, &length);
        do {
            if (length == 0 || length > MAX_LEN_KEYWORD) {
                atom = 0;
            }
            else {
                char tmp[length + 1];
                strncpy(tmp, part, length);
                tmp[length]= '\0';
                atom = purc_atom_try_string_ex(STREAM_ATOM_BUCKET, tmp);
            }

            if (atom == keywords2atoms[K_KW_nonblock].atom) {
                flags |= O_NONBLOCK;
            }
            else if (atom == keywords2atoms[K_KW_append].atom) {
                flags |= O_APPEND;
            }
            else if (atom == keywords2atoms[K_KW_cloexec].atom) {
                flags |= O_CLOEXEC;
            }

            if (parts_len <= length)
                break;

            parts_len -= length;
            part = pcutils_get_next_token_len(part + length, parts_len,
                    PURC_KW_DELIMITERS, &length);
        } while (part);
    }

    return flags;

out:
    return -1;
}

static
struct pcdvobjs_stream *create_stream_from_mem(void *mem,
        size_t mem_len, bool writable)
{
    struct pcdvobjs_stream* stream = dvobjs_stream_new(STREAM_TYPE_MEM, NULL);
    if (!stream) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    stream->stm4r = purc_rwstream_new_from_mem(mem, mem_len);
    if (stream->stm4r == NULL) {
        goto out_free_stream;
    }

    if (writable)
        stream->stm4w = stream->stm4r;
    stream->fd4r = -1;
    stream->fd4w = -1;

    return stream;

out_free_stream:
    dvobjs_stream_delete(stream);

out:
    return NULL;
}

static
struct pcdvobjs_stream *create_stream_from_buffer(size_t sz_init,
        size_t sz_max)
{
    struct pcdvobjs_stream* stream = dvobjs_stream_new(STREAM_TYPE_MEM, NULL);
    if (!stream) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    stream->stm4r = purc_rwstream_new_buffer(sz_init, sz_max);
    if (stream->stm4r == NULL) {
        goto out_free_stream;
    }

    stream->stm4w = stream->stm4r;
    stream->fd4r = -1;
    stream->fd4w = -1;

    return stream;

out_free_stream:
    dvobjs_stream_delete(stream);

out:
    return NULL;
}

static
struct pcdvobjs_stream *create_stream_from_fd(int fd,
        enum pcdvobjs_stream_type type, purc_variant_t option)
{
    if (option) {
        int flags = parse_from_option(option);
        if (flags == -1) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto out;
        }

        /* Exclude possible O_APPEND for fifo and socket. */
        if (type != STREAM_TYPE_FILE)
            flags &= ~O_APPEND;

        /* O_CLOEXEC is a file descriptor flag. */
        if (flags & O_CLOEXEC && fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
            purc_set_error(purc_error_from_errno(errno));
            goto out;
        }

        /* Others are file descriptor status flags. */
        flags &= ~O_CLOEXEC;
        if (flags && fcntl(fd, F_SETFL, flags) == -1) {
            purc_set_error(purc_error_from_errno(errno));
            goto out;
        }
    }

    struct pcdvobjs_stream* stream = dvobjs_stream_new(type, NULL);
    if (!stream) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    stream->stm4r = purc_rwstream_new_from_unix_fd(fd);
    if (stream->stm4r == NULL) {
        goto out_free_stream;
    }
    stream->stm4w = stream->stm4r;
    stream->fd4r = fd;
    stream->fd4w = fd;

    return stream;

out_free_stream:
    dvobjs_stream_delete(stream);

out:
    return NULL;
}

static
struct pcdvobjs_stream *
create_inet_socket_stream_from_fd(int fd, char *peer_addr, char *peer_port,
        purc_variant_t option)
{
    if (option) {
        int flags = parse_from_option(option);
        if (flags == -1) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto out;
        }

        /* exclude possible O_APPEND for socket */
        flags &= ~O_APPEND;
        if (flags && fcntl(fd, F_SETFL, flags) == -1) {
            purc_set_error(purc_error_from_errno(errno));
            goto out;
        }
    }

    if (peer_addr == NULL) {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        if (getpeername(fd, (struct sockaddr *)&addr, &len) == -1 || len == 0) {
            purc_set_error(purc_error_from_errno(errno));
            goto out;
        }

        char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
        if (0 == getnameinfo((struct sockaddr *)&addr, len,
                    hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),
                    NI_NUMERICHOST | NI_NUMERICSERV)) {
            peer_addr = strdup(hbuf);
            peer_port = strdup(sbuf);
        }
        else {
            purc_set_error(purc_error_from_errno(errno));
            goto out;
        }
    }

    struct pcdvobjs_stream* stream = dvobjs_stream_new(STREAM_TYPE_INET, NULL);
    if (!stream) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    stream->stm4r = purc_rwstream_new_from_unix_fd(fd);
    if (stream->stm4r == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out_free_stream;
    }
    stream->stm4w = stream->stm4r;
    stream->fd4r = fd;
    stream->fd4w = fd;
    stream->peer_addr = peer_addr;
    stream->peer_port = peer_port;

    return stream;

out_free_stream:
    dvobjs_stream_delete(stream);

out:
    return NULL;
}

#define READ_FLAG       0x01
#define WRITE_FLAG      0x02

/* We use the high 32-bit for customized flags */
#define _O_NAMELESS     (0x01L << 32)

static
int64_t parse_open_option(purc_variant_t option)
{
    purc_atom_t atom = 0;
    size_t parts_len;
    const char *parts;
    int rw = 0;
    int64_t flags = 0;

    if (option == PURC_VARIANT_INVALID) {
        atom = keywords2atoms[K_KW_default].atom;
    }
    else {
        parts = purc_variant_get_string_const_ex(option, &parts_len);
        if (parts == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto out;
        }

        parts = pcutils_trim_spaces(parts, &parts_len);
        if (parts_len == 0) {
            atom = keywords2atoms[K_KW_default].atom;
        }

        if (atom == 0) {
            char *tmp = strndup(parts, parts_len);
            atom = purc_atom_try_string_ex(STREAM_ATOM_BUCKET, tmp);
            free(tmp);
        }
    }

    if (atom == keywords2atoms[K_KW_default].atom) {
        rw = READ_FLAG | WRITE_FLAG;
    }
    else {
        size_t length = 0;
        const char *part = pcutils_get_next_token_len(parts, parts_len,
                PURC_KW_DELIMITERS, &length);
        do {
            if (length == 0 || length > MAX_LEN_KEYWORD) {
                atom = 0;
            }
            else {
                char tmp[length + 1];
                strncpy(tmp, part, length);
                tmp[length]= '\0';
                atom = purc_atom_try_string_ex(STREAM_ATOM_BUCKET, tmp);
            }

            if (atom == keywords2atoms[K_KW_read].atom) {
                rw |= READ_FLAG;
            }
            else if (atom == keywords2atoms[K_KW_write].atom) {
                rw |= WRITE_FLAG;
            }
            else if (atom == keywords2atoms[K_KW_nameless].atom) {
                flags |= _O_NAMELESS;
            }
            else if (atom == keywords2atoms[K_KW_nonblock].atom) {
                flags |= O_NONBLOCK;
            }
            else if (atom == keywords2atoms[K_KW_cloexec].atom) {
                flags |= O_CLOEXEC;
            }
            else if (atom == keywords2atoms[K_KW_append].atom) {
                flags |= O_APPEND;
            }
            else if (atom == keywords2atoms[K_KW_create].atom) {
                flags |= O_CREAT;
            }
            else if (atom == keywords2atoms[K_KW_truncate].atom) {
                flags |= O_TRUNC;
            }
            else {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto out;
            }

            if (parts_len <= length)
                break;

            parts_len -= length;
            part = pcutils_get_next_token_len(part + length, parts_len,
                    PURC_KW_DELIMITERS, &length);
        } while (part);
    }

    switch (rw) {
    case 1:
        flags |= O_RDONLY;
        break;
    case 2:
        flags |= O_WRONLY;
        break;
    case 3:
        flags |= O_RDWR;
        break;
    }

    return flags;

out:
    return -1;
}

static
struct pcdvobjs_stream *create_file_stream(struct purc_broken_down_url *url,
        purc_variant_t option)
{
    int fd = -1;
    int flags = (int)parse_open_option(option);

    if (flags == -1) {
        goto error;
    }

    if (flags & O_CREAT) {
        fd = open(url->path, flags, FILE_DEFAULT_MODE);
    }
    else {
        fd = open(url->path, flags);
    }

    if (fd == -1) {
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    struct pcdvobjs_stream* stream = dvobjs_stream_new(STREAM_TYPE_FILE, url);
    if (!stream) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto error;
    }

    stream->stm4r = purc_rwstream_new_from_unix_fd(fd);
    if (stream->stm4r == NULL) {
        goto delete_stream;
    }
    stream->stm4w = stream->stm4r;
    stream->fd4r = fd;
    stream->fd4w = fd;

    return stream;

delete_stream:
    dvobjs_stream_delete(stream);
    return NULL;

error:
    pcutils_broken_down_url_delete(url);
    close(fd);
    return NULL;
}

#define MAX_NR_ARGS 1024

#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclobbered"
#endif

static
struct pcdvobjs_stream *create_pipe_stream(struct purc_broken_down_url *url,
        purc_variant_t option)
{
    unsigned nr_args = 0;
    char **argv = NULL;

    int flags = (int)parse_open_option(option);
    if (flags == -1) {
        goto error;
    }

    if (!file_exists_and_is_executable(url->path)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    do {
        argv = realloc(argv, sizeof(char *) * (nr_args + 1));

        char buff[16];
        bool ret;

        snprintf(buff, sizeof(buff), "ARG%u", nr_args);
        ret = pcutils_url_get_query_value_alloc(url, buff, &argv[nr_args]);
        if (!ret) {
            if (nr_args == 0) {
                argv[0] = NULL;
            }
            else {
                break;
            }
        }
        else {
            if (pcdvobj_url_decode_in_place(argv[nr_args],
                        strlen(argv[nr_args]), PURC_K_KW_rfc3986)) {
                purc_log_warn("Failed to decode argument (%s)\n",
                        argv[nr_args]);
            }
        }

        if (nr_args > MAX_NR_ARGS)
            break;

        nr_args++;
    } while (nr_args <= MAX_NR_ARGS);

    if (argv[0] == NULL) {
        /* use the basename of the path as the first argument
           if not speicified */
        argv[0] = strdup(pcutils_basename(url->path));
    }

    /* make sure the argument array is null terminated */
    argv[nr_args] = NULL;

    int pipefd_stdin[2];
    int pipefd_stdout[2];
    pid_t cpid;


    /* FIXME: handle O_CLOEXEC flag */
#if OS(LINUX)
    if (pipe2(pipefd_stdin, (flags & O_NONBLOCK) ? O_NONBLOCK : 0) == -1) {
         purc_set_error(purc_error_from_errno(errno));
         goto error;
    }

    if (pipe2(pipefd_stdout, (flags & O_NONBLOCK) ? O_NONBLOCK : 0) == -1) {
         purc_set_error(purc_error_from_errno(errno));
         goto error;
    }

    cpid = vfork();
    if (cpid == -1) {
         purc_set_error(purc_error_from_errno(errno));
         goto error;
    }
#else
    if (pipe(pipefd_stdin) == -1) {
         purc_set_error(purc_error_from_errno(errno));
         goto error;
    }

    if (pipe(pipefd_stdout)) {
         purc_set_error(purc_error_from_errno(errno));
         goto error;
    }

    cpid = fork();
    if (cpid == -1) {
         purc_set_error(purc_error_from_errno(errno));
         goto error;
    }
#endif

    if (cpid == 0) {    /* Child reads from pipe */
        /* redirect the pipefd_stdin[0] as the stdin */
        if (dup2(pipefd_stdin[0], 0) == -1)
            _exit(EXIT_FAILURE);
        close(pipefd_stdin[0]);
        close(pipefd_stdin[1]);

        /* redirect the pipefd_stdout[1] as the stdout */
        if (dup2(pipefd_stdout[1], 1) == -1)
            _exit(EXIT_FAILURE);
        close(pipefd_stdout[0]);
        close(pipefd_stdout[1]);

        int nullfd = open("/dev/null", O_WRONLY);
        /* redirect nulfd as the stderr */
        dup2(nullfd, 2);

        if (execv(url->path, argv) == -1)
            _exit(EXIT_FAILURE);
    }

    for (unsigned n = 0; n < nr_args; n++) {
        free(argv[n]);
    }
    free(argv);

    int status;
    /* if the child has exited */
    if (waitpid(cpid, &status, WNOHANG) == -1) {
        goto out_close_fd;
    }

    struct pcdvobjs_stream* stream;
    stream = dvobjs_stream_new(STREAM_TYPE_PIPE, url);
    if (!stream) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out_close_fd;
    }

    close(pipefd_stdin[0]);
    close(pipefd_stdout[1]);

    stream->stm4w = purc_rwstream_new_from_unix_fd(pipefd_stdin[1]);
    if (stream->stm4w == NULL) {
        goto out_free_stream;
    }
    stream->fd4w = pipefd_stdin[1];

    stream->stm4r = purc_rwstream_new_from_unix_fd(pipefd_stdout[0]);
    if (stream->stm4r == NULL) {
        goto out_free_stream;
    }
    stream->fd4r = pipefd_stdout[0];
    stream->cpid = cpid;

    return stream;

out_free_stream:
    dvobjs_stream_delete(stream);
    return NULL;

out_close_fd:
    close(pipefd_stdin[0]);
    close(pipefd_stdin[1]);
    close(pipefd_stdout[0]);
    close(pipefd_stdout[1]);

error:
    pcutils_broken_down_url_delete(url);
    return NULL;
}

#ifndef __clang__
#pragma GCC diagnostic pop
#endif

static
struct pcdvobjs_stream *create_fifo_stream(struct purc_broken_down_url *url,
        purc_variant_t option)
{
    UNUSED_PARAM(url);
    UNUSED_PARAM(option);

    int flags = (int)parse_open_option(option);
    if (flags == -1) {
        goto error;
    }

    if (!file_exists(url->path) && (flags & O_CREAT)) {
         int ret = mkfifo(url->path, FIFO_DEFAULT_MODE);
         if (ret != 0) {
             purc_set_error(purc_error_from_errno(errno));
            goto error;
         }
    }

    int fd = 0;
    if (flags & O_CREAT) {
        fd = open(url->path, flags, FIFO_DEFAULT_MODE);
    }
    else {
        fd = open(url->path, flags);
    }
    if (fd == -1) {
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    struct pcdvobjs_stream* stream =
        dvobjs_stream_new(STREAM_TYPE_FIFO, url);
    if (!stream) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out_close_fd;
    }

    stream->stm4r = purc_rwstream_new_from_unix_fd(fd);
    if (stream->stm4r == NULL) {
        goto out_free_stream;
    }
    stream->stm4w = stream->stm4r;
    stream->fd4r = fd;
    stream->fd4w = fd;

    return stream;

out_free_stream:
    dvobjs_stream_delete(stream);
    return NULL;

out_close_fd:
    close(fd);

error:
    pcutils_broken_down_url_delete(url);
    return NULL;
}

#define US_CLI_PATH             "/var/tmp/"
#define US_CLI_PERM              S_IRWXU

static
struct pcdvobjs_stream *
create_unix_socket_stream(struct purc_broken_down_url *url,
        purc_variant_t option, const char *prot, const struct timeval *timeout)
{
    int64_t flags = parse_open_option(option);
    if (flags == -1) {
        goto error;
    }

    struct sockaddr_un unix_addr;
    socklen_t len;

    if (strlen(url->path) + 1 > sizeof(unix_addr.sun_path)) {
        purc_set_error(PURC_ERROR_TOO_LONG);
        goto error;
    }

    if (!file_exists(url->path)) {
        PC_ERROR("Path does not exist: %s\n", url->path);
        purc_set_error(PURC_ERROR_NOT_EXISTS);
        goto error;
    }

    int fd = 0;
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    if (!(flags & _O_NAMELESS)) {
        char socket_prefix[33];
        struct pcinst* inst = pcinst_current();

        pcutils_md5_ctxt ctx;
        unsigned char md5_digest[16];

        pcutils_md5_begin(&ctx);
        struct timespec tp;
        clock_gettime(CLOCK_REALTIME, &tp);
        pcutils_md5_hash(&ctx, &tp, sizeof(tp));
        pcutils_md5_hash(&ctx, prot, strlen(prot));
        pcutils_md5_end(&ctx, md5_digest);
        pcutils_bin2hex(md5_digest, 16, socket_prefix, false);
        socket_prefix[16] = 0;  /* we only use the first 16 characters */

        const char *runner_name = "_unknown_";
        if (inst) {
            runner_name = inst->runner_name;
        }

        /* Fill socket address structure w/our address.
           Since 0.9.22, we always include runner name. */
        memset(&unix_addr, 0, sizeof(unix_addr));
        unix_addr.sun_family = AF_UNIX;
        /* On Linux sun_path is 108 bytes in size */
        int r = snprintf(unix_addr.sun_path, sizeof(unix_addr.sun_path),
                "%s%s-%s-%05d", US_CLI_PATH,
                socket_prefix, runner_name, getpid());
        if (r < 0) {
            purc_set_error(purc_error_from_errno(errno));
            goto error;
        }

        if ((size_t)r >= sizeof(unix_addr.sun_path)) {
            PC_ERROR("Too long runner name for local socket file: %s\n",
                    runner_name);
            purc_set_error(PURC_ERROR_TOO_LONG);
            goto error;
        }

        len = sizeof(unix_addr.sun_family);
        len += strlen(unix_addr.sun_path) + 1;

        unlink(unix_addr.sun_path);        /* in case it already exists */
        if (bind(fd, (struct sockaddr *) &unix_addr, len) < 0) {
            PC_ERROR("Failed to call `bind`: %s\n", strerror(errno));
            purc_set_error(purc_error_from_errno(errno));
            goto out_close_fd;
        }

        if (chmod(unix_addr.sun_path, US_CLI_PERM) < 0) {
            PC_ERROR("Failed to call `chmod`: %s\n", strerror(errno));
            purc_set_error(purc_error_from_errno(errno));
            goto out_close_fd;
        }
    }

    if (timeout) {
        socklen_t optlen = sizeof(struct timeval);
        if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, timeout, optlen) == -1) {
            PC_ERROR("Failed setsockopt(): %s\n", strerror(errno));
            purc_set_error(purc_error_from_errno(errno));
            goto out_close_fd;
        }
    }

    /* fill socket address structure w/server's addr */
    memset(&unix_addr, 0, sizeof(unix_addr));
    unix_addr.sun_family = AF_UNIX;
    strcpy(unix_addr.sun_path, url->path);
    len = sizeof(unix_addr.sun_family) + strlen(unix_addr.sun_path) + 1;
    if (connect(fd, (struct sockaddr *) &unix_addr, len) < 0) {
        PC_ERROR("Failed connect(): %s\n",strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto out_close_fd;
    }

    struct pcdvobjs_stream* stream =
        dvobjs_stream_new(STREAM_TYPE_UNIX, url);
    if (!stream) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out_close_fd;
    }

    stream->stm4r = purc_rwstream_new_from_unix_fd(fd);
    if (stream->stm4r == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out_free_stream;
    }
    stream->stm4w = stream->stm4r;
    stream->fd4r = fd;
    stream->fd4w = fd;

    return stream;

out_free_stream:
    dvobjs_stream_delete(stream);
    return NULL;

out_close_fd:
    close(fd);

error:
    pcutils_broken_down_url_delete(url);
    return NULL;
}

static int inet_socket_connect(enum stream_inet_socket_family isf,
        const char *host, int port, char **peer_addr, char **peer_port,
        const struct timeval *timeout)
{
    int fd = -1;
    struct addrinfo *addrinfo;
    struct addrinfo *p;
    struct addrinfo hints = { 0 };

    switch (isf) {
        case ISF_UNSPEC:
            hints.ai_family = AF_UNSPEC;
            break;
        case ISF_INET4:
            hints.ai_family = AF_INET;
            break;
        case ISF_INET6:
            hints.ai_family = AF_INET6;
            break;
    }

    if (port <= 0 || port > 65535) {
        PC_WARN("Bad port value: (%d)\n", port);
        goto done;
    }

    char s_port[8] = {0};
    snprintf(s_port, sizeof(s_port), "%d", port);

    hints.ai_socktype = SOCK_STREAM;
    if (0 != getaddrinfo(host, s_port, &hints, &addrinfo)) {
        PC_ERROR("Error while getting address info (%s:%d)\n",
                host, port);
        goto done;
    }

    for (p = addrinfo; p != NULL; p = p->ai_next) {
        if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }

        if (timeout) {
            socklen_t optlen = sizeof(struct timeval);
            if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, timeout,
                        optlen) == -1) {
                PC_ERROR("Failed setsockopt(SO_RCVTIMEO)\n");
                close(fd);
                fd = -1;
                break;
            }
        }

        if (connect(fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(fd);
            fd = -1;
            continue;
        }

        break;
    }

    if (fd >= 0 && *peer_addr == NULL) {
        char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
        if (0 != getnameinfo(p->ai_addr, p->ai_addrlen,
                    hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),
                    NI_NUMERICHOST | NI_NUMERICSERV)) {
            PC_ERROR("Failed getnameinfo(%s:%d)\n", host, port);
            close(fd);
            fd = -1;
        }
        else {
            *peer_addr = strdup(hbuf);
            *peer_port = strdup(sbuf);
        }
    }
    else if (fd < 0) {
        PC_ERROR("Failed to create socket for %s:%d\n", host, port);
    }

    freeaddrinfo(addrinfo);

done:
    return fd;
}

static
struct pcdvobjs_stream *
create_inet_socket_stream(purc_atom_t scheme,
        struct purc_broken_down_url *url, purc_variant_t option,
        const struct timeval *timeout)
{
    int flags = (int)parse_open_option(option);
    if (flags == -1) {
        goto error;
    }

    enum stream_inet_socket_family isf = ISF_UNSPEC;
    char *peer_addr = NULL;
    char *peer_port = NULL;

    if (scheme == keywords2atoms[K_KW_inet4].atom) {
        isf = ISF_INET4;
    }
    else if (scheme == keywords2atoms[K_KW_inet6].atom) {
        isf = ISF_INET6;
    }
    else if (scheme != keywords2atoms[K_KW_inet].atom) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    int fd = inet_socket_connect(isf, url->hostname, url->port,
            &peer_addr, &peer_port, timeout);
    if (fd < 0) {
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    if (flags & O_CLOEXEC && fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        purc_set_error(purc_error_from_errno(errno));
        goto out_close_fd;
    }

    if (flags & O_NONBLOCK && fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        purc_set_error(purc_error_from_errno(errno));
        goto out_close_fd;
    }

    struct pcdvobjs_stream* stream = dvobjs_stream_new(STREAM_TYPE_INET, url);
    if (!stream) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out_close_fd;
    }

    stream->stm4r = purc_rwstream_new_from_unix_fd(fd);
    if (stream->stm4r == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out_free_stream;
    }
    stream->stm4w = stream->stm4r;
    stream->fd4r = fd;
    stream->fd4w = fd;
    stream->peer_addr = peer_addr;
    stream->peer_port = peer_port;

    return stream;

out_free_stream:
    dvobjs_stream_delete(stream);
    return NULL;

out_close_fd:
    close(fd);

error:
    pcutils_broken_down_url_delete(url);
    return NULL;
}

static struct purc_native_ops stream_basic_ops = {
    .property_getter = property_getter,
    .on_observe = on_observe,
    .on_forget = on_forget,
    .on_release = on_release,
};

purc_variant_t
dvobjs_create_stream_from_fd(int fd, purc_variant_t option,
        const char *ext_prot, purc_variant_t extra_opts)
{
    struct purc_native_ops *ops = &stream_basic_ops;
    struct pcdvobjs_stream *stream = NULL;
    const char *entity_name  = NATIVE_ENTITY_NAME_STREAM ":raw";

    int type = get_stream_type_by_fd(fd);
    if (type == STREAM_TYPE_FILE) {
        stream = create_stream_from_fd(fd, STREAM_TYPE_FILE, option);
    }
    else if (type == STREAM_TYPE_FIFO) {
        stream = create_stream_from_fd(fd, STREAM_TYPE_FIFO, option);
    }
    else if (type == STREAM_TYPE_UNIX) {
        stream = create_stream_from_fd(fd, STREAM_TYPE_UNIX, option);

        if (ext_prot && stream) {
            purc_atom_t atom;
            atom = purc_atom_try_string_ex(STREAM_ATOM_BUCKET, ext_prot);
            if (atom == keywords2atoms[K_KW_message].atom
#if ENABLE(STREAM_HBDBUS)
                    || atom == keywords2atoms[K_KW_hbdbus].atom
#endif
                    ) {
                entity_name = NATIVE_ENTITY_NAME_STREAM ":message";
                ops = dvobjs_extend_stream_by_message(stream, ops,
                        extra_opts);

#if ENABLE(STREAM_HBDBUS)
                if (atom == keywords2atoms[K_KW_hbdbus].atom) {
                    entity_name = NATIVE_ENTITY_NAME_STREAM ":hbdbus";
                    ops = dvobjs_extend_stream_by_hbdbus(stream, ops,
                            extra_opts);
                }
#endif
            }

            if (ops == NULL) {
                dvobjs_stream_delete(stream);
                stream = NULL;
            }
        }
    }
    else if (type == STREAM_TYPE_INET) {

        stream = create_inet_socket_stream_from_fd(fd, NULL, NULL, option);

        if (ext_prot && stream) {
            purc_atom_t atom;
            atom = purc_atom_try_string_ex(STREAM_ATOM_BUCKET, ext_prot);
            if (atom == keywords2atoms[K_KW_websocket].atom
#if ENABLE(STREAM_HBDBUS)
                    || atom == keywords2atoms[K_KW_hbdbus].atom
#endif
                    ) {
                entity_name = NATIVE_ENTITY_NAME_STREAM ":websocket";
                ops = dvobjs_extend_stream_by_websocket(stream, ops,
                        extra_opts);

#if ENABLE(STREAM_HBDBUS)
                if (atom == keywords2atoms[K_KW_hbdbus].atom) {
                    entity_name = NATIVE_ENTITY_NAME_STREAM ":hbdbus";
                    ops = dvobjs_extend_stream_by_hbdbus(stream, ops,
                            extra_opts);
                }
#endif
            }

            if (ops == NULL) {
                dvobjs_stream_delete(stream);
                stream = NULL;
            }
        }
    }
    else {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    }

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    if (!stream) {
        goto done;
    }

    // setup a callback for `on_release` to destroy the stream automatically
    ret_var = purc_variant_make_native_entity(stream, ops, entity_name);
    if (ret_var) {
        stream->observed = ret_var;
    }

done:
    return ret_var;
}

/*
$STREAM.from(
    < string | bsequence | longint $source: `A string, a byte sequence, or a file descriptor.` >
    [, <'[append || nonblock || cloexec] | keep' $flags = 'keep':
           - 'append':      `Set the file descriptor in append mode.`
           - 'nonblock':    `Set the file descriptor in nonblocking mode.`
           - 'cloexec':     `Set the file descriptor flag close-on-exec.`
           - 'keep':        `Do not change the file descriptor (status) flags.`
       >
       [, < 'raw | message | websocket | hbdbus' $ext_protocol = 'raw': `The extended protocol will be used on the stream.`
           - 'raw':                 `No extended protocol.`
           - 'message':             `WebSocket-like message-based protocol; only for 'local://' and 'fifo://' connections.`
           - 'websocket':           `WebSocket protocol; only for 'inetN:// connections.`
           - 'hbdbus':              `HybridOS data bus protocol; only for 'local://' and 'inetN://' connections.`
          >
            [, <object $extra_options: `The extra options.` >
            ]
       ]
    ]
) native/stream | undefined
 */
static purc_variant_t
stream_from_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto error;
    }

    const void *bytes = NULL;
    size_t bytes_len;
    int64_t tmp_l = -1;
    if ((bytes = purc_variant_get_string_const_ex(argv[0], &bytes_len))) {
    }
    else if ((bytes = purc_variant_get_bytes_const(argv[0], &bytes_len))) {
    }
    else if (purc_variant_cast_to_longint(argv[0], &tmp_l, false)) {
        if (tmp_l < 0 || tmp_l > INT_MAX) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            goto error;
        }
    }
    else {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    if (tmp_l >= 0) {
        int fd = (int)tmp_l;
        purc_variant_t option = nr_args > 1 ? argv[1] : PURC_VARIANT_INVALID;
        if (option != PURC_VARIANT_INVALID &&
                (!purc_variant_is_string(option))) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto error;
        }

        const char *ext_prot = "raw";
        if (nr_args > 2 && !purc_variant_is_string(argv[2])) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto error;
        }
        else if (nr_args > 2)
            ext_prot = purc_variant_get_string_const(argv[2]);

        return dvobjs_create_stream_from_fd(fd, option, ext_prot,
                (nr_args > 3) ? argv[3] : PURC_VARIANT_INVALID);
    }
    else {
        struct pcdvobjs_stream *stream;
        stream = create_stream_from_mem((void *)bytes, bytes_len, false);
        if (stream == NULL)
            goto error;

        struct purc_native_ops *ops = &stream_basic_ops;
        const char *entity_name = NATIVE_ENTITY_NAME_STREAM ":raw";
        purc_variant_t ret;
        ret = purc_variant_make_native_entity(stream, ops, entity_name);
        if (ret) {
            stream->kept = purc_variant_ref(argv[0]);
        }

        return ret;
    }

error:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

/*
$STREAM.from_buffer(
    < ulongint $init_size: `The initial size of the buffer.` >,
    [,
        < ulongint $max_size = 0: `The maximum size of the buffer; 0 means no limitation.`>
    ]
) native/stream | undefined
 */
static purc_variant_t
stream_from_buffer_getter(purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto error;
    }

    uint64_t sz_init, sz_max  = 0;
    if (!purc_variant_cast_to_ulongint(argv[0], &sz_init, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    if (nr_args > 1) {
        if (!purc_variant_cast_to_ulongint(argv[1], &sz_max, false)) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto error;
        }
    }

    struct pcdvobjs_stream *stream;
    stream = create_stream_from_buffer(sz_init, sz_max);
    if (stream == NULL)
        goto error;

    struct purc_native_ops *ops = &stream_basic_ops;
    const char *entity_name = NATIVE_ENTITY_NAME_STREAM ":raw";
    return purc_variant_make_native_entity(stream, ops, entity_name);

error:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

purc_variant_t
dvobjs_create_stream_from_url(const char *url, purc_variant_t option,
        const char *ext_prot, purc_variant_t extra_opts)
{
    struct purc_broken_down_url *bdurl = (struct purc_broken_down_url*)
        calloc(1, sizeof(struct purc_broken_down_url));
    if (!pcutils_url_break_down(bdurl, url)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    purc_atom_t atom;
    atom = purc_atom_try_string_ex(STREAM_ATOM_BUCKET, bdurl->scheme);
    if (atom == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    struct timeval tv = { };
    const struct timeval *timeout = NULL;
    if (extra_opts) {
        purc_variant_t tmp;
        tmp = purc_variant_object_get_by_ckey_ex(extra_opts,
                "recv-timeout", true);
        if (tmp) {
            pcdvobjs_cast_to_timeval(&tv, tmp);
            timeout = &tv;
        }
    }

    struct purc_native_ops *ops = &stream_basic_ops;
    struct pcdvobjs_stream *stream = NULL;
    const char *entity_name  = NATIVE_ENTITY_NAME_STREAM ":raw";

    if (atom == keywords2atoms[K_KW_file].atom) {
        stream = create_file_stream(bdurl, option);
    }
    else if (atom == keywords2atoms[K_KW_pipe].atom) {
        stream = create_pipe_stream(bdurl, option);
    }
    else if (atom == keywords2atoms[K_KW_fifo].atom) {
        stream = create_fifo_stream(bdurl, option);
    }
    else if (atom == keywords2atoms[K_KW_local].atom ||
            atom == keywords2atoms[K_KW_unix].atom) {

        stream = create_unix_socket_stream(bdurl, option, ext_prot, timeout);

        if (ext_prot && stream) {
            atom = purc_atom_try_string_ex(STREAM_ATOM_BUCKET, ext_prot);
            if (atom == keywords2atoms[K_KW_message].atom
#if ENABLE(STREAM_HBDBUS)
                    || atom == keywords2atoms[K_KW_hbdbus].atom
#endif
                    ) {
                entity_name = NATIVE_ENTITY_NAME_STREAM ":message";
                ops = dvobjs_extend_stream_by_message(stream, ops,
                        extra_opts);

#if ENABLE(STREAM_HBDBUS)
                if (atom == keywords2atoms[K_KW_hbdbus].atom) {
                    entity_name = NATIVE_ENTITY_NAME_STREAM ":hbdbus";
                    ops = dvobjs_extend_stream_by_hbdbus(stream, ops,
                            extra_opts);
                }
#endif
            }

            if (ops == NULL) {
                dvobjs_stream_delete(stream);
                stream = NULL;
            }
        }
    }
    else if (atom == keywords2atoms[K_KW_inet].atom ||
            atom == keywords2atoms[K_KW_inet4].atom ||
            atom == keywords2atoms[K_KW_inet6].atom) {

        stream = create_inet_socket_stream(atom, bdurl, option, timeout);

        if (ext_prot && stream) {
            atom = purc_atom_try_string_ex(STREAM_ATOM_BUCKET, ext_prot);
            if (atom == keywords2atoms[K_KW_websocket].atom
#if ENABLE(STREAM_HBDBUS)
                    || atom == keywords2atoms[K_KW_hbdbus].atom
#endif
                    ) {
                entity_name = NATIVE_ENTITY_NAME_STREAM ":websocket";
                ops = dvobjs_extend_stream_by_websocket(stream, ops,
                        extra_opts);

#if ENABLE(STREAM_HBDBUS)
                if (atom == keywords2atoms[K_KW_hbdbus].atom) {
                    entity_name = NATIVE_ENTITY_NAME_STREAM ":hbdbus";
                    ops = dvobjs_extend_stream_by_hbdbus(stream, ops,
                            extra_opts);
                }
#endif
            }

            if (ops == NULL) {
                dvobjs_stream_delete(stream);
                stream = NULL;
            }
        }
    }

    if (stream == NULL && errno == EINPROGRESS) {
        return purc_variant_make_null();
    }

    purc_variant_t retv = PURC_VARIANT_INVALID;
    if (stream) {
        retv = purc_variant_make_native_entity(stream, ops, entity_name);
        if (retv) {
            stream->observed = retv;
        }
    }

    return retv;

failed:
    pcutils_broken_down_url_delete(bdurl);
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
stream_open_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    const char *url, *ext_prot;
    purc_variant_t option;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto error;
    }

    if (!purc_variant_is_string(argv[0])) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }
    url = purc_variant_get_string_const(argv[0]);

    option = nr_args > 1 ? argv[1] : PURC_VARIANT_INVALID;
    if (option != PURC_VARIANT_INVALID &&
            (!purc_variant_is_string(option))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    ext_prot = "raw";
    if (nr_args > 2 && !purc_variant_is_string(argv[2])) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }
    else if (nr_args > 2)
        ext_prot = purc_variant_get_string_const(argv[2]);

    if (nr_args > 3 && !purc_variant_is_object(argv[3])) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    return dvobjs_create_stream_from_url(url, option, ext_prot,
            (nr_args > 3) ? argv[3] : PURC_VARIANT_INVALID);

error:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

pcdvobjs_stream *dvobjs_stream_check_entity(purc_variant_t v,
        struct purc_native_ops **ops)
{
    if ((!purc_variant_is_native(v))) {
        goto error;
    }

    const char *entity_name = purc_variant_native_get_name(v);
    if (entity_name == NULL ||
            strncmp(entity_name, NATIVE_ENTITY_NAME_STREAM,
                sizeof(NATIVE_ENTITY_NAME_STREAM) - 1) != 0) {
        goto error;
    }

    pcdvobjs_stream *stream = purc_variant_native_get_entity(v);
    if (ops) {
       *ops = purc_variant_native_get_ops(v);
    }

    return stream;

error:
    return NULL;
}

static purc_variant_t
stream_close_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }

    struct purc_native_ops *ops;
    struct pcdvobjs_stream *stream = dvobjs_stream_check_entity(argv[0], &ops);
    if (stream == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    PC_ASSERT(ops->property_getter);
    purc_nvariant_method closer = ops->property_getter(stream, _KW_close);
    return closer(stream, _KW_close, nr_args - 1, argv + 1, call_flags);

out:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static bool add_stdio_property(purc_variant_t v)
{
    static struct purc_native_ops ops = {
        .property_getter = property_getter,
        .on_observe = on_observe,
        .on_forget = on_forget,
        .on_release = on_release,
    };
    struct pcdvobjs_stream* stream = NULL;
    purc_variant_t var;

    // stdin
    stream = create_file_stdin_stream();
    if (!stream) {
        goto out;
    }
    var = purc_variant_make_native_entity(stream, &ops,
            NATIVE_ENTITY_NAME_STREAM ":raw/stdin");
    if (var == PURC_VARIANT_INVALID) {
        goto out;
    }
    stream->observed = var;
    if (!purc_variant_object_set_by_static_ckey(v, STDIN_NAME, var)) {
        goto out_unref_var;
    }
    purc_variant_unref(var);

    // stdout
    stream = create_file_stdout_stream();
    if (!stream) {
        goto out;
    }
    var = purc_variant_make_native_entity(stream, &ops,
            NATIVE_ENTITY_NAME_STREAM ":raw/stdout");
    if (var == PURC_VARIANT_INVALID) {
        goto out;
    }
    stream->observed = var;
    if (!purc_variant_object_set_by_static_ckey(v, STDOUT_NAME, var)) {
        goto out_unref_var;
    }
    purc_variant_unref(var);

    // stderr
    stream = create_file_stderr_stream();
    if (!stream) {
        goto out;
    }
    var = purc_variant_make_native_entity(stream, &ops,
            NATIVE_ENTITY_NAME_STREAM ":raw/stderr");
    if (var == PURC_VARIANT_INVALID) {
        goto out;
    }
    stream->observed = var;
    if (!purc_variant_object_set_by_static_ckey(v, STDERR_NAME, var)) {
        goto out_unref_var;
    }
    purc_variant_unref(var);

    return true;

out_unref_var:
    purc_variant_unref(var);

out:
    return false;
}

purc_variant_t purc_dvobj_stream_new(void)
{
    static struct purc_dvobj_method  stream[] = {
        { "open",   stream_open_getter,     NULL },
        { "from",   stream_from_getter,     NULL },
        { "from_buffer",    stream_from_buffer_getter,     NULL },
        { "close",  stream_close_getter,    NULL },
    };

    if (keywords2atoms[0].atom == 0) {
        for (size_t i = 0; i < PCA_TABLESIZE(keywords2atoms); i++) {
            keywords2atoms[i].atom =
                purc_atom_from_static_string_ex(STREAM_ATOM_BUCKET,
                    keywords2atoms[i].keyword);
        }
    }

    purc_variant_t v = purc_dvobj_make_from_methods(stream,
            PCA_TABLESIZE(stream));
    if (v == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    if (add_stdio_property(v)) {
        return v;
    }

    purc_variant_unref(v);
    return PURC_VARIANT_INVALID;
}

purc_variant_t
dvobjs_create_stream_by_accepted(struct pcdvobjs_socket *socket,
        purc_atom_t scheme, char *peer_addr, char *peer_port, int fd,
        purc_variant_t prot_v, purc_variant_t prot_opts)
{
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    struct purc_native_ops *ops = &stream_basic_ops;
    struct pcdvobjs_stream *stream = NULL;
    const char *entity_name = NATIVE_ENTITY_NAME_STREAM ":raw";

    if (scheme == keywords2atoms[K_KW_unix].atom ||
            scheme == keywords2atoms[K_KW_local].atom) {

        const char *prot = "raw";
        if (prot_v) {
            prot = purc_variant_get_string_const(prot_v);
        }

        stream = create_stream_from_fd(fd, STREAM_TYPE_UNIX, NULL);
        stream->peer_addr = peer_addr;
        stream->peer_port = peer_port;

        if (prot && stream) {
            purc_atom_t atom;
            atom = purc_atom_try_string_ex(STREAM_ATOM_BUCKET, prot);
            if (atom == keywords2atoms[K_KW_message].atom
#if ENABLE(STREAM_HBDBUS)
                    || atom == keywords2atoms[K_KW_hbdbus].atom
#endif
                    ) {
                entity_name = NATIVE_ENTITY_NAME_STREAM ":message";
                ops = dvobjs_extend_stream_by_message(stream, ops, prot_opts);

#if ENABLE(STREAM_HBDBUS)
                if (atom == keywords2atoms[K_KW_hbdbus].atom) {
                    entity_name = NATIVE_ENTITY_NAME_STREAM ":hbdbus";
                    ops = dvobjs_extend_stream_by_hbdbus(stream,
                            ops, prot_opts);
                }
#endif
            }

            if (ops == NULL) {
                dvobjs_stream_delete(stream);
                stream = NULL;
            }
        }
    }
    else if (scheme == keywords2atoms[K_KW_inet].atom ||
            scheme == keywords2atoms[K_KW_inet4].atom ||
            scheme == keywords2atoms[K_KW_inet6].atom) {

        const char *prot = "raw";
        if (prot_v) {
            prot = purc_variant_get_string_const(prot_v);
        }

        stream = create_inet_socket_stream_from_fd(fd, peer_addr, peer_port,
                NULL);

        if (stream)
            stream->socket = pcdvobjs_socket_acquire(socket);

        if (prot && stream) {
            purc_atom_t atom;
            atom = purc_atom_try_string_ex(STREAM_ATOM_BUCKET, prot);
            if (atom == keywords2atoms[K_KW_websocket].atom
#if ENABLE(STREAM_HBDBUS)
                    || atom == keywords2atoms[K_KW_hbdbus].atom
#endif
                    ) {
                entity_name = NATIVE_ENTITY_NAME_STREAM ":websocket";
                ops = dvobjs_extend_stream_by_websocket(stream,
                        ops, prot_opts);

#if ENABLE(STREAM_HBDBUS)
                if (atom == keywords2atoms[K_KW_hbdbus].atom) {
                    entity_name = NATIVE_ENTITY_NAME_STREAM ":hbdbus";
                    ops = dvobjs_extend_stream_by_hbdbus(stream,
                            ops, prot_opts);
                }
#endif
            }

            if (ops == NULL) {
                dvobjs_stream_delete(stream);
                stream = NULL;
            }
        }
    }
    else {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    }

    if (stream) {
        // setup a callback for `on_release` to destroy the stream automatically
        ret_var = purc_variant_make_native_entity(stream, ops, entity_name);
        if (ret_var) {
            stream->observed = ret_var;
        }
    }

    PC_DEBUG("Extended result: (stream: %p, ops: %p, retv: %p)\n",
            stream, ops, ret_var);

    return ret_var;
}

