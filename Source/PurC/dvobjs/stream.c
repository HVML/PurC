/*
 * @file stream.c
 * @author Geng Yue, Xue Shuming, Vincent Wei
 * @date 2021/07/02
 * @brief The implementation of stream dynamic variant object.
 *
 * Copyright (C) 2021, 2022 FMSoft <https://www.fmsoft.cn>
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
#include "config.h"
#include "purc-variant.h"
#include "purc-runloop.h"
#include "purc-dvobjs.h"

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

#define BUFFER_SIZE                 1024

#define ENDIAN_PLATFORM             0
#define ENDIAN_LITTLE               1
#define ENDIAN_BIG                  2

#define STDIN_NAME                  "stdin"
#define STDOUT_NAME                 "stdout"
#define STDERR_NAME                 "stderr"

#define STREAM_EVENT_NAME           "event"
#define STREAM_SUB_EVENT_READ       "readable"
#define STREAM_SUB_EVENT_WRITE      "writable"
#define STREAM_SUB_EVENT_ALL        "*"

#define FILE_DEFAULT_MODE           0644
#define FIFO_DEFAULT_MODE           0644

#define MAX_LEN_KEYWORD             64

#define _KW_DELIMITERS              " \t\n\v\f\r"

#define STREAM_ATOM_BUCKET          ATOM_BUCKET_DVOBJ

enum {
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
#define _KW_nonblock                "nonblock"
    K_KW_nonblock,
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
#define _KW_winsock                 "winsock"
    K_KW_winsock,
#define _KW_ws                      "ws"
    K_KW_ws,
#define _KW_wss                     "wss"
    K_KW_wss,
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
#define _KW_writebytes              "writebytes"
    K_KW_writebytes,
#define _KW_writeeof                "writeeof"
    K_KW_writeeof,
#define _KW_status                  "status"
    K_KW_status,
#define _KW_seek                    "seek"
    K_KW_seek,
#define _KW_close                   "close"
    K_KW_close,
};

static struct keyword_to_atom {
    const char *keyword;
    purc_atom_t atom;
} keywords2atoms [] = {
    { _KW_default, 0 },             // "default"
    { _KW_read, 0 },                // "read"
    { _KW_write, 0 },               // "write"
    { _KW_append, 0 },              // "append"
    { _KW_create, 0 },              // "create"
    { _KW_truncate, 0 },            // "truncate"
    { _KW_nonblock, 0 },            // "nonblock"
    { _KW_set, 0 },                 // "set"
    { _KW_current, 0 },             // "current"
    { _KW_end, 0 },                 // "end"
    { _KW_file, 0 },                // "file"
    { _KW_pipe, 0 },                // "pipe"
    { _KW_fifo, 0 },                // "fifo"
    { _KW_unix, 0 },                // "unix"
    { _KW_winsock, 0 },             // "winsock"
    { _KW_ws, 0 },                  // "ws"
    { _KW_wss, 0 },                 // "wss"
    { _KW_readstruct, 0},           // readstruct
    { _KW_writestruct, 0},          // writestruct
    { _KW_readlines, 0},            // readlines
    { _KW_writelines, 0},           // writelines
    { _KW_readbytes, 0},            // readbytes
    { _KW_writebytes, 0},           // writebytes
    { _KW_writeeof, 0},             // writeeof
    { _KW_status, 0},               // status
    { _KW_seek, 0},                 // seek
    { _KW_close, 0},                // close
};

enum pcdvobjs_stream_type {
    STREAM_TYPE_FILE_STDIN,
    STREAM_TYPE_FILE_STDOUT,
    STREAM_TYPE_FILE_STDERR,
    STREAM_TYPE_FILE,
    STREAM_TYPE_PIPE,
    STREAM_TYPE_FIFO,
    STREAM_TYPE_UNIX_SOCK,
    STREAM_TYPE_WIN_SOCK,
    STREAM_TYPE_WS,
    STREAM_TYPE_WSS,
};

struct pcdvobjs_stream {
    enum pcdvobjs_stream_type type;
    struct purc_broken_down_url *url;
    purc_rwstream_t stm4r;      /* stream for read */
    purc_rwstream_t stm4w;      /* stream for write */
    purc_variant_t option;
    purc_variant_t observed;    /* not inc ref */
    uintptr_t monitor4r, monitor4w;
    int fd4r, fd4w;

    pid_t cpid;                 /* only for pipe, the pid of child */
    purc_atom_t cid;
};

static
struct pcdvobjs_stream *dvobjs_stream_create(enum pcdvobjs_stream_type type,
        struct purc_broken_down_url *url, purc_variant_t option)
{
    struct pcdvobjs_stream *stream = (struct pcdvobjs_stream*)calloc(1,
            sizeof(struct pcdvobjs_stream));
    if (!stream) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }
    stream->type = type;
    stream->url = url;
    if (option) {
        stream->option = option;
        purc_variant_ref(stream->option);
    }

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

    if (stream->option) {
        purc_variant_unref(stream->option);
        stream->option = PURC_VARIANT_INVALID;
    }

    if (stream->monitor4r) {
        purc_runloop_remove_fd_monitor(purc_runloop_get_current(),
                stream->monitor4r);
        stream->monitor4r = 0;
    }

    if (stream->monitor4w) {
        purc_runloop_remove_fd_monitor(purc_runloop_get_current(),
                stream->monitor4w);
        stream->monitor4w = 0;
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
}

static void native_stream_destroy(struct pcdvobjs_stream *stream)
{
    if (!stream) {
        return;
    }

    native_stream_close(stream);

    if (stream->url) {
        pcutils_broken_down_url_delete(stream->url);
    }

    if (stream->option) {
        purc_variant_unref(stream->option);
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

    if (native_entity == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    stream = get_stream(native_entity);
    rwstream = stream->stm4r;
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    formats = purc_variant_get_string_const_ex(argv[0], &formats_left);
    if (formats == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    formats = pcutils_trim_spaces(formats, &formats_left);
    if (formats_left == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    return purc_dvobj_read_struct(rwstream, formats, formats_left,
            (call_flags & PCVRT_CALL_FLAG_SILENTLY));

out:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_array(0, PURC_VARIANT_INVALID);
    }

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
    size_t write_length = 0;
    const char *formats = NULL;
    size_t formats_left = 0;

    if (native_entity == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    stream = get_stream(native_entity);
    rwstream = stream->stm4w;
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
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
        if (bf.bytes == NULL)
            goto out;

        goto failed;
    }

    silently = true;    // fall through

failed:
    if (silently) {
        if (bf.bytes) {
            write_length = purc_rwstream_write(rwstream, bf.bytes, bf.nr_bytes);
            free(bf.bytes);
            bf.bytes = NULL;
        }
        return purc_variant_make_ulongint(write_length);
    }

out:
    if (bf.bytes)
        free(bf.bytes);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_ulongint(write_length);
    return PURC_VARIANT_INVALID;
}

#define LINE_FLAG           "\n"
static int read_lines(purc_rwstream_t stream, int line_num,
        purc_variant_t array)
{
    unsigned char buffer[BUFFER_SIZE];
    ssize_t read_size = 0;
    size_t length = 0;
    const char *head = NULL;
    const char *end = NULL;

    while (line_num) {
        read_size = purc_rwstream_read(stream, buffer, BUFFER_SIZE);
        if (read_size < 0)
            break;

        end = (const char*)(buffer + read_size);

        head = pcutils_get_next_token_len((const char*)buffer, read_size,
                LINE_FLAG, &length);
        while (head && head < end) {
            purc_variant_t var = purc_variant_make_string_ex(head, length,
                    false);
            if (!var) {
                return -1;
            }
            if (!purc_variant_array_append(array, var)) {
                purc_variant_unref(var);
                return -1;
            }
            purc_variant_unref(var);
            line_num --;

            if (line_num == 0)
                break;

            head = pcutils_get_next_token_len(head + length, end - head - length,
                LINE_FLAG, &length);
        }
        if (read_size < BUFFER_SIZE)           // to the end
            break;

        if (line_num == 0)
            break;
    }

    return 0;
}

static purc_variant_t
readlines_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    struct pcdvobjs_stream *stream;
    purc_rwstream_t rwstream = NULL;
    int64_t line_num = 0;
    if (native_entity == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    stream = get_stream(native_entity);
    rwstream = stream->stm4r;
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    purc_variant_t ret_var = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (!ret_var) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }

    if (argv[0] != PURC_VARIANT_INVALID &&
            !purc_variant_cast_to_longint(argv[0], &line_num, false)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (line_num > 0) {
        int ret = read_lines(rwstream, line_num, ret_var);
        if (ret != 0) {
            goto out;
        }
    }

    return ret_var;

out:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return ret_var;

    if (ret_var) {
        purc_variant_unref(ret_var);
    }

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
writelines_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    struct pcdvobjs_stream *stream;
    purc_rwstream_t rwstream = NULL;
    ssize_t nr_write = 0;
    purc_variant_t data;

    if (native_entity == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    stream = get_stream(native_entity);
    rwstream = stream->stm4w;
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }

    if (argv[0] == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
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
                size_t sz_container = purc_variant_linear_container_get_size(data);
                for (size_t i = 0; i < sz_container; i++) {
                    if (!purc_variant_is_string(
                                purc_variant_linear_container_get(data, i))) {
                        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
                        goto out;
                    }
                }
            }
            break;
        default:
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto out;
        }

        const char *buffer = NULL;
        size_t buffer_size = 0;
        if (purc_variant_is_string(data)) {
            buffer = (const char *)purc_variant_get_string_const_ex(data,
                    &buffer_size);
            if (buffer && buffer_size > 0) {
                nr_write += purc_rwstream_write(rwstream, buffer, buffer_size);
                nr_write += purc_rwstream_write(rwstream, "\n", 1);
            }
        }
        else {
            size_t sz_container = purc_variant_linear_container_get_size(data);
            for (size_t i = 0; i < sz_container; i++) {
                purc_variant_t var = purc_variant_linear_container_get(data, i);
                buffer = (const char *)purc_variant_get_string_const_ex(var,
                        &buffer_size);
                if (buffer && buffer_size > 0) {
                    nr_write += purc_rwstream_write(rwstream, buffer, buffer_size);
                    nr_write += purc_rwstream_write(rwstream, "\n", 1);
                }
            }
        }
    }

    return purc_variant_make_ulongint(nr_write);

out:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_ulongint(nr_write);
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

    if (native_entity == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    stream = get_stream(native_entity);
    rwstream = stream->stm4r;
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }

    if (argv[0] != PURC_VARIANT_INVALID && 
            !purc_variant_cast_to_ulongint(argv[0], &byte_num, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    if (byte_num == 0) {
        ret_var = purc_variant_make_byte_sequence_empty();
    }
    else {
        char * content = malloc(byte_num);
        size_t size = 0;

        if (content == NULL) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto out;
        }

        size = purc_rwstream_read(rwstream, content, byte_num);
        if (size > 0) {
            ret_var = purc_variant_make_byte_sequence_reuse_buff(content,
                    size, size);
        }
        else {
            free(content);
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            ret_var = PURC_VARIANT_INVALID;
        }
    }

    return ret_var;

out:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_byte_sequence_empty();
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
writebytes_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    struct pcdvobjs_stream *stream;
    purc_rwstream_t rwstream = NULL;
    purc_variant_t data;

    if (native_entity == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    stream = get_stream(native_entity);
    rwstream = stream->stm4w;
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }
    data = argv[0];

    if (data == PURC_VARIANT_INVALID ||
            (!purc_variant_is_bsequence(data) &&
             !purc_variant_is_string(data))
            ) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    size_t bsize = 0;
    const unsigned char *buffer = NULL;
    if (purc_variant_is_bsequence(data)) {
        buffer = purc_variant_get_bytes_const (data, &bsize);
    }
    else {
        buffer = (const unsigned char *)purc_variant_get_string_const(data);
        bsize = strlen((const char*)buffer) + 1;
    }
    if (buffer && bsize) {
        ssize_t nr_write = purc_rwstream_write (rwstream, buffer, bsize);
        return purc_variant_make_ulongint(nr_write);
    }

    return ret_var;

out:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_ulongint(0);
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
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
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
    if (off == -1) {
        goto out;
    }
    ret_var = purc_variant_make_longint(off);

    return ret_var;

out:
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

    if (native_entity == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return purc_variant_make_boolean(false);
    }

    struct pcdvobjs_stream *stream = get_stream(native_entity);
    native_stream_close(stream);

    return purc_variant_make_boolean(true);
}

struct io_callback_data {
    int                           fd;
    purc_runloop_io_event         io_event;
    struct pcdvobjs_stream       *stream;
};

static void on_stream_io_callback(struct io_callback_data *data)
{
    purc_runloop_io_event event = data->io_event;
    struct pcdvobjs_stream *stream = data->stream;

    const char* sub = NULL;
    if (event & PCRUNLOOP_IO_IN) {
        sub = STREAM_SUB_EVENT_READ;
    }
    else if (event & PCRUNLOOP_IO_OUT) {
        sub = STREAM_SUB_EVENT_WRITE;
    }
    if (sub && stream->cid) {
        pcintr_coroutine_post_event(stream->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_IGNORE,
                stream->observed, STREAM_EVENT_NAME, sub,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    }

    free(data);
}

static bool
stream_io_callback(int fd, purc_runloop_io_event event, void *ctxt)
{
    struct pcdvobjs_stream *stream = (struct pcdvobjs_stream*) ctxt;
    PC_ASSERT(stream);

    struct io_callback_data *data;
    data = (struct io_callback_data*)calloc(1, sizeof(*data));
    PC_ASSERT(data);

    data->fd = fd;
    data->io_event = event;
    data->stream = stream;

    on_stream_io_callback(data);

    return true;
}


static bool
on_observe(void *native_entity, const char *event_name,
        const char *event_subname)
{
    if (strcmp(event_name, STREAM_EVENT_NAME) != 0) {
        return false;
    }

    purc_runloop_io_event event = PCRUNLOOP_IO_IN;
    if (strcmp(event_subname, STREAM_SUB_EVENT_READ) == 0) {
        event = PCRUNLOOP_IO_IN;
    }
    else if (strcmp(event_subname, STREAM_SUB_EVENT_WRITE) == 0) {
        event = PCRUNLOOP_IO_OUT;
    }
    else if (strcmp(event_subname, STREAM_SUB_EVENT_ALL) == 0) {
        event = PCRUNLOOP_IO_IN | PCRUNLOOP_IO_OUT;
    }

    struct pcdvobjs_stream *stream = (struct pcdvobjs_stream*)native_entity;
    if (event & PCRUNLOOP_IO_IN && stream->fd4r >= 0) {
        stream->monitor4r = purc_runloop_add_fd_monitor(
                purc_runloop_get_current(), stream->fd4r, PCRUNLOOP_IO_IN,
                stream_io_callback, stream);
        if (stream->monitor4r) {
            pcintr_coroutine_t co = pcintr_get_coroutine();
            if (co) {
                stream->cid = co->cid;
            }
            return true;
        }
        return false;
    }

    if (event & PCRUNLOOP_IO_OUT && stream->fd4w >= 0) {
        stream->monitor4w= purc_runloop_add_fd_monitor(
                purc_runloop_get_current(), stream->fd4w, PCRUNLOOP_IO_OUT,
                stream_io_callback, stream);
        if (stream->monitor4w) {
            pcintr_coroutine_t co = pcintr_get_coroutine();
            if (co) {
                stream->cid = co->cid;
            }
            return true;
        }
        return false;
    }

    return true;
}

static bool
on_forget(void *native_entity, const char *event_name,
        const char *event_subname)
{
    UNUSED_PARAM(event_name);
    UNUSED_PARAM(event_subname);
    struct pcdvobjs_stream *stream = (struct pcdvobjs_stream*)native_entity;

    if (stream->monitor4r) {
        purc_runloop_remove_fd_monitor(purc_runloop_get_current(),
                stream->monitor4r);
        stream->monitor4r = 0;
    }

    if (stream->monitor4w) {
        purc_runloop_remove_fd_monitor(purc_runloop_get_current(),
                stream->monitor4w);
        stream->monitor4w = 0;
    }
    stream->cid = 0;

    return true;
}

static void
on_release(void *native_entity)
{
    native_stream_destroy((struct pcdvobjs_stream *)native_entity);
}

static purc_nvariant_method
property_getter(void *entity, const char *name)
{
    UNUSED_PARAM(entity);
    purc_atom_t atom = purc_atom_try_string_ex(STREAM_ATOM_BUCKET, name);
    if (atom == 0) {
        return NULL;
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
    else if (atom == keywords2atoms[K_KW_close].atom) {
        return close_getter;
    }
    return NULL;
}

static
struct pcdvobjs_stream *create_file_std_stream(enum pcdvobjs_stream_type type)
{
    int fd = -1;
    switch (type) {
    case STREAM_TYPE_FILE_STDIN:
        fd = dup(STDIN_FILENO);
        break;

    case STREAM_TYPE_FILE_STDOUT:
        fd = dup(STDOUT_FILENO);
        break;

    case STREAM_TYPE_FILE_STDERR:
        fd = dup(STDERR_FILENO);
        break;

    default:
        return NULL;
    }

    struct pcdvobjs_stream* stream = dvobjs_stream_create(type,
            NULL, PURC_VARIANT_INVALID);
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
    native_stream_destroy(stream);

out:
    close(fd);
    return NULL;
}

static inline
struct pcdvobjs_stream *create_file_stdin_stream()
{
    return create_file_std_stream(STREAM_TYPE_FILE_STDIN);
}

static inline
struct pcdvobjs_stream *create_file_stdout_stream()
{
    return create_file_std_stream(STREAM_TYPE_FILE_STDOUT);
}

static inline
struct pcdvobjs_stream *create_file_stderr_stream()
{
    return create_file_std_stream(STREAM_TYPE_FILE_STDERR);
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

#define READ_FLAG       0x01
#define WRITE_FLAG      0x02

static
int parse_open_option(purc_variant_t option)
{
    purc_atom_t atom = 0;
    size_t parts_len;
    const char *parts;
    int rw = 0;
    int flags = 0;

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
    }

    if (atom == 0) {
        char *tmp = strndup(parts, parts_len);
        atom = purc_atom_try_string_ex(STREAM_ATOM_BUCKET, tmp);
        free(tmp);
    }

    if (atom == keywords2atoms[K_KW_default].atom) {
        rw = READ_FLAG | WRITE_FLAG;
    }
    else {
        size_t length = 0;
        const char *part = pcutils_get_next_token_len(parts, parts_len,
                _KW_DELIMITERS, &length);
        do {
            if (length == 0 || length > MAX_LEN_KEYWORD) {
                atom = keywords2atoms[K_KW_read].atom;
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
            else if (atom == keywords2atoms[K_KW_nonblock].atom) {
                flags |= O_NONBLOCK;
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

            if (parts_len <= length)
                break;

            parts_len -= length;
            part = pcutils_get_next_token_len(part + length, parts_len,
                    _KW_DELIMITERS, &length);
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
    int flags = parse_open_option(option);
    if (flags == -1) {
        return NULL;
    }

    int fd = 0;
    if (flags & O_CREAT) {
        fd = open(url->path, flags, FILE_DEFAULT_MODE);
    }
    else {
        fd = open(url->path, flags);
    }

    if (fd == -1) {
        purc_set_error(purc_error_from_errno(errno));
        return NULL;
    }

    struct pcdvobjs_stream* stream = dvobjs_stream_create(STREAM_TYPE_FILE,
            url, option);
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
    native_stream_destroy(stream);

out:
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

    int flags = parse_open_option(option);
    if (flags == -1) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    if (!file_exists_and_is_executable(url->path)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    do {
        argv = realloc(argv, sizeof(char *) * (nr_args + 1));

        char buff[12];
        bool ret;

        sprintf(buff, "ARG%u", nr_args);
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


#if OS(LINUX)
    if (pipe2(pipefd_stdin, (flags & O_NONBLOCK) ? O_NONBLOCK : 0) == -1) {
         purc_set_error(purc_error_from_errno(errno));
         return NULL;
    }

    if (pipe2(pipefd_stdout, (flags & O_NONBLOCK) ? O_NONBLOCK : 0) == -1) {
         purc_set_error(purc_error_from_errno(errno));
         return NULL;
    }

    cpid = vfork();
    if (cpid == -1) {
         purc_set_error(purc_error_from_errno(errno));
         return NULL;
    }
#else
    if (pipe(pipefd_stdin) == -1) {
         purc_set_error(purc_error_from_errno(errno));
         return NULL;
    }

    if (pipe(pipefd_stdout)) {
         purc_set_error(purc_error_from_errno(errno));
         return NULL;
    }

    cpid = fork();
    if (cpid == -1) {
         purc_set_error(purc_error_from_errno(errno));
         return NULL;
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
    stream = dvobjs_stream_create(STREAM_TYPE_PIPE,
            url, option);
    if (!stream) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out_close_fd;
    }

    stream->stm4w = purc_rwstream_new_from_unix_fd(pipefd_stdin[1]);
    if (stream->stm4w == NULL) {
        goto out_free_stream;
    }
    stream->fd4w = pipefd_stdin[1];
    close(pipefd_stdin[0]);

    stream->stm4r = purc_rwstream_new_from_unix_fd(pipefd_stdout[0]);
    if (stream->stm4r == NULL) {
        goto out_free_stream;
    }
    stream->fd4r = pipefd_stdout[0];
    close(pipefd_stdout[1]);
    stream->cpid = cpid;

    return stream;

out_free_stream:
    native_stream_destroy(stream);

out_close_fd:
    close(pipefd_stdin[0]);
    close(pipefd_stdin[1]);
    close(pipefd_stdout[0]);
    close(pipefd_stdout[1]);
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

    int flags = parse_open_option(option);
    if (flags == -1) {
        return NULL;
    }

    if (!file_exists(url->path) && (flags & O_CREAT)) {
         int ret = mkfifo(url->path, FIFO_DEFAULT_MODE);
         if (ret != 0) {
             purc_set_error(purc_error_from_errno(errno));
             return NULL;
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
        return NULL;
    }

    struct pcdvobjs_stream* stream = dvobjs_stream_create(STREAM_TYPE_FIFO,
            url, option);
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
    native_stream_destroy(stream);

out_close_fd:
    close(fd);

    return NULL;
}

static
struct pcdvobjs_stream *create_unix_sock_stream(struct purc_broken_down_url *url,
        purc_variant_t option)
{
    UNUSED_PARAM(option);

    if (!file_exists(url->path)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }
    int fd = 0;
    if ((fd = socket (AF_UNIX, SOCK_STREAM, 0)) < 0) {
        purc_set_error(PCRDR_ERROR_IO);
        return NULL;
    }

    struct sockaddr_un unix_addr;
    memset (&unix_addr, 0, sizeof(unix_addr));
    unix_addr.sun_family = AF_UNIX;
    strcpy(unix_addr.sun_path, url->path);
    int len = sizeof (unix_addr.sun_family) + strlen (unix_addr.sun_path);
    if (connect(fd, (struct sockaddr *) &unix_addr, len) < 0) {
        goto out_close_fd;
    }

    struct pcdvobjs_stream* stream = dvobjs_stream_create(STREAM_TYPE_UNIX_SOCK,
            url, option);
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
    native_stream_destroy(stream);

out_close_fd:
    close (fd);

    return NULL;
}

static purc_variant_t
stream_open_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    purc_variant_t option = nr_args > 1 ? argv[1] : PURC_VARIANT_INVALID;
    if (option != PURC_VARIANT_INVALID &&
            (!purc_variant_is_string(option))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    struct purc_broken_down_url *url = (struct purc_broken_down_url*)
        calloc(1, sizeof(struct purc_broken_down_url));
    if (!pcutils_url_break_down(url, purc_variant_get_string_const(argv[0]))) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    purc_atom_t atom = purc_atom_try_string_ex(STREAM_ATOM_BUCKET, url->schema);
    if (atom == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out_free_url;
    }

    struct pcdvobjs_stream* stream = NULL;
    if (atom == keywords2atoms[K_KW_file].atom) {
        stream = create_file_stream(url, option);
    }
    else if (atom == keywords2atoms[K_KW_pipe].atom) {
        stream = create_pipe_stream(url, option);
    }
    else if (atom == keywords2atoms[K_KW_fifo].atom) {
        stream = create_fifo_stream(url, option);
    }
    else if (atom == keywords2atoms[K_KW_unix].atom) {
        stream = create_unix_sock_stream(url, option);
    }
#if 0
    else if (atom == keywords2atoms[K_KW_winsock].atom) {
    }
    else if (atom == keywords2atoms[K_KW_ws].atom) {
    }
    else if (atom == keywords2atoms[K_KW_wss].atom) {
    }
#endif

    if (!stream) {
        goto out_free_url;
    }

    // setup a callback for `on_release` to destroy the stream automatically
    static const struct purc_native_ops ops = {
        .property_getter = property_getter,
        .on_observe = on_observe,
        .on_forget = on_forget,
        .on_release = on_release,
    };
    ret_var = purc_variant_make_native(stream, &ops);
    if (ret_var) {
        stream->observed = ret_var;
    }
    return ret_var;

out_free_url:
    pcutils_broken_down_url_delete(url);

out:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
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

    if ((!purc_variant_is_native(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    struct pcdvobjs_stream *stream = purc_variant_native_get_entity(argv[0]);
    native_stream_close(stream);
    return purc_variant_make_boolean(true);

out:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

static bool add_stdio_property(purc_variant_t v)
{
    static const struct purc_native_ops ops = {
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
    var = purc_variant_make_native(stream, &ops);
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
    var = purc_variant_make_native(stream, &ops);
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
    var = purc_variant_make_native(stream, &ops);
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


