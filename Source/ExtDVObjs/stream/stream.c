/*
 * @file stream.c
 * @author Geng Yue, Xue Shuming
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

#include "config.h"
#include "purc-variant.h"
#include "purc-runloop.h"
#include "purc-dvobjs.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

#define DVOBJ_STREAM_NAME           "STREAM"
#define DVOBJ_STREAM_DESC           "For io stream operations in PURC"
#define DVOBJ_STREAM_VERSION        0

#define BUFFER_SIZE                 1024

#define ENDIAN_PLATFORM             0
#define ENDIAN_LITTLE               1
#define ENDIAN_BIG                  2

#define STDIN_NAME                  "stdin"
#define STDOUT_NAME                 "stdout"
#define STDERR_NAME                 "stderr"

#define STREAM_EVENT_NAME           "event"
#define STREAM_SUB_EVENT_READ       "read"
#define STREAM_SUB_EVENT_WRITE      "write"
#define STREAM_SUB_EVENT_ALL        "*"

#define FILE_DEFAULT_MODE           0644
#define PIPO_DEFAULT_MODE           0777

#define MAX_LEN_KEYWORD             64

#define _KW_DELIMITERS              " \t\n\v\f\r"

#define STREAM_ATOM_BUCKET          PURC_ATOM_BUCKET_USER // ATOM_BUCKET_DVOBJ

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
#define _KW_unix                    "unix"
    K_KW_unix,
#define _KW_winsock                 "winsock"
    K_KW_winsock,
#define _KW_ws                      "ws"
    K_KW_ws,
#define _KW_wss                     "wss"
    K_KW_wss,
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
    { _KW_unix, 0 },                // "unix"
    { _KW_winsock, 0 },             // "winsock"
    { _KW_ws, 0 },                  // "ws"
    { _KW_wss, 0 },                 // "wss"
};

enum pcdvobjs_stream_type {
    STREAM_TYPE_FILE_STDIN,
    STREAM_TYPE_FILE_STDOUT,
    STREAM_TYPE_FILE_STDERR,
    STREAM_TYPE_FILE,
    STREAM_TYPE_PIPE,
    STREAM_TYPE_UNIX_SOCK,
    STREAM_TYPE_WIN_SOCK,
    STREAM_TYPE_WS,
    STREAM_TYPE_WSS,
};

struct pcdvobjs_stream {
    enum pcdvobjs_stream_type type;
    struct purc_broken_down_url *url;
    purc_rwstream_t rws;
    purc_variant_t option;
    purc_variant_t observed;  // not inc ref
    uintptr_t monitor;
    int fd;
};


static
void purc_broken_down_url_destroy(struct purc_broken_down_url *url)
{
    if (!url) {
        return;
    }

    if (url->schema) {
        free(url->schema);
    }

    if (url->user) {
        free(url->user);
    }

    if (url->passwd) {
        free(url->passwd);
    }

    if (url->host) {
        free(url->host);
    }

    if (url->path) {
        free(url->path);
    }

    if (url->query) {
        free(url->query);
    }

    if (url->fragment) {
        free(url->fragment);
    }
}

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
    return stream;
}

static
void dvobjs_stream_destroy(struct pcdvobjs_stream *stream)
{
    if (!stream) {
        return;
    }

    if (stream->url) {
        purc_broken_down_url_destroy(stream->url);
    }

    if (stream->rws) {
        purc_rwstream_destroy(stream->rws);
    }

    if (stream->option) {
        purc_variant_unref(stream->option);
    }

    if (stream->monitor) {
        purc_runloop_remove_fd_monitor(purc_runloop_get_current(),
                stream->monitor);
    }

    if (stream->fd) {
        close(stream->fd);
    }

    free(stream);
}

static
bool is_file_exists(const char* file)
{
    struct stat filestat;
    return (0 == stat(file, &filestat));
}

static
struct pcdvobjs_stream *create_file_std_stream(enum pcdvobjs_stream_type type)
{
    FILE* fp = NULL;
    int fd = -1;
    switch (type) {
    case STREAM_TYPE_FILE_STDIN:
        fp = stdin;
        fd = dup(STDIN_FILENO);
        break;

    case STREAM_TYPE_FILE_STDOUT:
        fp = stdout;
        fd = dup(STDOUT_FILENO);
        break;

    case STREAM_TYPE_FILE_STDERR:
        fp = stderr;
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

    stream->rws = purc_rwstream_new_from_fp(fp);
    if (stream->rws == NULL) {
        goto out_free_stream;
    }
    stream->fd = fd;

    return stream;

out_free_stream:
    dvobjs_stream_destroy(stream);

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

#define READ_FLAG       0x01
#define WRITE_FLAG      0x02

int parse_option(purc_variant_t option)
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
    int flags = parse_option(option);
    if (flags == -1) {
        return NULL;
    }

    int fd = 0;
    if (flags & O_CREAT) {
        fd = open(url->path, flags, FILE_DEFAULT_MODE);
    }
    else if ((flags & O_WRONLY) || (flags & O_RDWR)) {
        fd = open(url->path, flags);
    }

    if (fd == -1) {
        purc_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
        return NULL;
    }

    struct pcdvobjs_stream* stream = dvobjs_stream_create(STREAM_TYPE_FILE,
            url, option);
    if (!stream) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    stream->rws = purc_rwstream_new_from_unix_fd(fd, BUFFER_SIZE);
    if (stream->rws == NULL) {
        goto out_free_stream;
    }
    stream->fd = fd;

    return stream;

out_free_stream:
    dvobjs_stream_destroy(stream);

out:
    close(fd);
    return NULL;
}

static
struct pcdvobjs_stream *create_pipe_stream(struct purc_broken_down_url *url,
        purc_variant_t option)
{
    UNUSED_PARAM(url);
    UNUSED_PARAM(option);

    int flags = parse_option(option);
    if (flags == -1) {
        return NULL;
    }

    if (!is_file_exists(url->path) && (flags & O_CREAT)) {
         int ret = mkfifo(url->path, PIPO_DEFAULT_MODE);
         if (ret != 0) {
             purc_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
             return NULL;
         }
    }

    int fd = open(url->path, flags);
    if (fd == -1) {
        purc_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
        return NULL;
    }

    struct pcdvobjs_stream* stream = dvobjs_stream_create(STREAM_TYPE_PIPE,
            url, option);
    if (!stream) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out_close_fd;
    }

    stream->rws = purc_rwstream_new_from_unix_fd(fd, BUFFER_SIZE);
    if (stream->rws == NULL) {
        goto out_free_stream;
    }
    stream->fd = fd;

    return stream;

out_free_stream:
    dvobjs_stream_destroy(stream);

out_close_fd:
    close(fd);

    return NULL;
}

static
struct pcdvobjs_stream *create_unix_sock_stream(struct purc_broken_down_url *url,
        purc_variant_t option)
{
    UNUSED_PARAM(option);

    if (!is_file_exists(url->path)) {
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

    stream->rws = purc_rwstream_new_from_unix_fd(fd, BUFFER_SIZE);
    if (stream->rws == NULL) {
        goto out_free_stream;
    }
    stream->fd = fd;

    return stream;

out_free_stream:
    dvobjs_stream_destroy(stream);

out_close_fd:
    close (fd);

    return NULL;
}

static bool
stream_io_callback(int fd, purc_runloop_io_event event, void *ctxt)
{
    UNUSED_PARAM(fd);
    // dispatch event
    struct pcdvobjs_stream *stream = (struct pcdvobjs_stream*) ctxt;
    const char* sub = NULL;
    if (event == PCRUNLOOP_IO_IN) {
        sub = STREAM_SUB_EVENT_READ;
    }
    else if (event == PCRUNLOOP_IO_OUT) {
        sub = STREAM_SUB_EVENT_WRITE;
    }
    if (sub) {
        purc_variant_t type = purc_variant_make_string(STREAM_EVENT_NAME, false);
        purc_variant_t sub_type = purc_variant_make_string(sub, false);

        purc_runloop_dispatch_message(purc_runloop_get_current(),
                stream->observed, type, sub_type, PURC_VARIANT_INVALID);

        purc_variant_unref(type);
        purc_variant_unref(sub_type);
    }
    return true;
}

// stream native variant
static bool
on_observe(void *native_entity, const char *event_name,
        const char *event_subname)
{
    if (strcmp(event_name, STREAM_EVENT_NAME) != 0) {
        return false;
    }

    purc_runloop_io_event event;
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
    if (stream->fd) {
        stream->monitor = purc_runloop_add_fd_monitor(
                purc_runloop_get_current(), stream->fd, event,
                stream_io_callback, stream);
        if (stream->monitor) {
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
    if (stream->monitor) {
        purc_runloop_remove_fd_monitor(purc_runloop_get_current(),
                stream->monitor);
        stream->monitor = 0;
    }
    return true;
}

static void
on_release(void *native_entity)
{
    dvobjs_stream_destroy((struct pcdvobjs_stream *)native_entity);
}

purc_rwstream_t get_rwstream_from_variant(purc_variant_t v)
{
    struct pcdvobjs_stream *stream = purc_variant_native_get_entity(v);
    if (stream) {
        return stream->rws;
    }
    return NULL;
}

// for file to get '\n'
static const char *pcdvobjs_stream_get_next_option(const char *data,
        const char *delims, size_t *length)
{
    const char *head = data;
    char *temp = NULL;

    if ((delims == NULL) || (data == NULL) || (*delims == 0x00))
        return NULL;

    *length = 0;

    while (*data != 0x00) {
        temp = strchr(delims, *data);
        if (temp)
            break;
        data++;
    }

    *length = data - head;

    return head;
}

static inline bool is_little_endian(void)
{
#if CPU(BIG_ENDIAN)
    return false;
#elif CPU(LITTLE_ENDIAN)
    return true;
#endif
}

static ssize_t find_line_stream(purc_rwstream_t stream, int line_num)
{
    size_t pos = 0;
    unsigned char buffer[BUFFER_SIZE];
    ssize_t read_size = 0;
    size_t length = 0;
    const char *head = NULL;

    purc_rwstream_seek(stream, 0L, SEEK_SET);

    while (line_num) {
        read_size = purc_rwstream_read(stream, buffer, BUFFER_SIZE);
        if (read_size < 0)
            break;

        head = pcdvobjs_stream_get_next_option((char *)buffer,
                "\n", &length);
        while (head) {
            pos += length + 1;          // to be checked
            line_num --;

            if (line_num == 0)
                break;

            head = pcdvobjs_stream_get_next_option(head + length + 1,
                    "\n", &length);
        }
        if (read_size < BUFFER_SIZE)           // to the end
            break;

        if (line_num == 0)
            break;
    }

    return pos;
}


static purc_variant_t
stream_open_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

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

    struct purc_broken_down_url *url= (struct purc_broken_down_url*)
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
    purc_broken_down_url_destroy(url);

out:
    if (silently)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static inline void change_order(unsigned char *buf, size_t size)
{
    size_t time = 0;
    unsigned char temp = 0;

    for (time = 0; time < size / 2; size ++) {
        temp = *(buf + time);
        *(buf + time) = *(buf + size - 1 - time);
        *(buf + size - 1 - time) = temp;
    }
    return;
}

static inline void read_rwstream(purc_rwstream_t rwstream,
                    unsigned char *buf, int type, int bytes)
{
    purc_rwstream_read(rwstream, buf, bytes);
    switch (type) {
        case ENDIAN_PLATFORM:
            break;
        case ENDIAN_LITTLE:
            if (!is_little_endian())
                change_order(buf, bytes);
            break;
        case ENDIAN_BIG:
            if (is_little_endian())
                change_order(buf, bytes);
            break;
    }
}

static purc_variant_t
stream_readstruct_getter(purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(root);

    const char *formats = NULL;
    size_t formats_left = 0;
    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_native(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    if (argv[1] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string(argv[1]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    purc_rwstream_t rwstream = get_rwstream_from_variant(argv[0]);
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    formats = purc_variant_get_string_const_ex(argv[1], &formats_left);
    if (formats == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    formats = pcutils_trim_spaces(formats, &formats_left);
    if (formats_left == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    return purc_dvobj_read_struct(rwstream, formats, formats_left, silently);

out:
    if (silently) {
        return purc_variant_make_array(0, PURC_VARIANT_INVALID);
    }

    return PURC_VARIANT_INVALID;
}

static inline void write_rwstream_int(purc_rwstream_t rwstream,
        purc_variant_t arg, int *index, int type, int bytes, size_t *length)
{
    purc_variant_t val = PURC_VARIANT_INVALID;
    int64_t i64 = 0;

    val = purc_variant_array_get(arg, *index);
    (*index)++;
    purc_variant_cast_to_longint(val, &i64, false);
    switch (type) {
        case ENDIAN_PLATFORM:
            break;
        case ENDIAN_LITTLE:
            if (!is_little_endian())
                change_order((unsigned char *)&i64, sizeof(int64_t));
            break;
        case ENDIAN_BIG:
            if (is_little_endian())
                change_order((unsigned char *)&i64, sizeof(int64_t));
            break;
    }

    if (is_little_endian())
        purc_rwstream_write(rwstream, (char *)&i64, bytes);
    else
        purc_rwstream_write(rwstream,
                (char *)&i64 + sizeof(int64_t) - bytes, bytes);
    *length += bytes;
}

static inline unsigned short  write_double_to_16 (double d, int type)
{
    unsigned long long number = 0;
    unsigned long long sign = 0;
    unsigned long long e = 0;
    unsigned long long base = 0;
    unsigned short ret = 0;

    memcpy(&number, &d, sizeof(double));

    sign = number >> 63;
    e = (number >> 52) & 0x7FFFF;
    base = (number & 0xFFFFFFFFFFFFF) >> (52 - 10);

    e = 15 + e - 1023;
    e = e << 10;
    base |= e;
    base |= (sign << 15);
    ret = base;

    switch (type) {
        case ENDIAN_PLATFORM:
            break;
        case ENDIAN_LITTLE:
            if (!is_little_endian())
                change_order((unsigned char *)&ret, 2);
            break;
        case ENDIAN_BIG:
            if (is_little_endian())
                change_order((unsigned char *)&ret, 2);
            break;
    }
    return ret;
}

static inline void write_rwstream_uint(purc_rwstream_t rwstream,
        purc_variant_t arg, int *index, int type, int bytes, size_t *length)
{
    purc_variant_t val = PURC_VARIANT_INVALID;
    uint64_t u64 = 0;

    val = purc_variant_array_get(arg, *index);
    (*index)++;
    purc_variant_cast_to_ulongint(val, &u64, false);
    switch (type) {
        case ENDIAN_PLATFORM:
            break;
        case ENDIAN_LITTLE:
            if (!is_little_endian())
                change_order((unsigned char *)&u64, sizeof(uint64_t));
            break;
        case ENDIAN_BIG:
            if (is_little_endian())
                change_order((unsigned char *)&u64, sizeof(uint64_t));
            break;
    }

    if (is_little_endian())
        purc_rwstream_write(rwstream, (char *)&u64, bytes);
    else
        purc_rwstream_write(rwstream,
                (char *)&u64 + sizeof(uint64_t) - bytes, bytes);
    *length += bytes;
}

static purc_variant_t
stream_writestruct_getter(purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    struct pcdvobj_bytes_buff bf = { NULL, 0, 0 };

    purc_rwstream_t rwstream = NULL;
    size_t write_length = 0;
    const char *formats = NULL;
    size_t formats_left = 0;
    if (nr_args < 3) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_native(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    if (argv[1] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string(argv[1]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    rwstream = get_rwstream_from_variant(argv[0]);
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    formats = purc_variant_get_string_const_ex(argv[1], &formats_left);
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

    if (silently)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
stream_readlines_getter(purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = NULL;
    int64_t line_num = 0;

    if (nr_args != 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_native(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }
    rwstream = get_rwstream_from_variant(argv[0]);
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (argv[1] != PURC_VARIANT_INVALID) {
        purc_variant_cast_to_longint(argv[1], &line_num, false);
        if (line_num < 0)
            line_num = 0;
    }

    if (line_num == 0)
        ret_var = purc_variant_make_string("", false);
    else {
        size_t pos = find_line_stream(rwstream, line_num);

        char * content = malloc(pos + 1);
        if (content == NULL) {
            return purc_variant_make_string("", false);
        }

        purc_rwstream_seek(rwstream, 0L, SEEK_SET);
        pos = purc_rwstream_read(rwstream, content, pos);
        *(content + pos - 1) = 0x00;

        ret_var = purc_variant_make_string_reuse_buff(content, pos, false);
    }
    return ret_var;

out:
    if (silently)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
stream_writelines_getter(purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = NULL;

    if (nr_args != 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_native(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }
    if (argv[1] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string(argv[1]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    rwstream = get_rwstream_from_variant(argv[0]);
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    const char *buffer = (const char *)purc_variant_get_string_const (argv[1]);
    ssize_t buffer_size = purc_variant_string_size(argv[1]);
    if (buffer && buffer_size > 0) {
        ssize_t nr_write = purc_rwstream_write (rwstream, buffer, buffer_size);
        return  purc_variant_make_ulongint(nr_write);
    }
    return ret_var;

out:
    if (silently)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
stream_readbytes_getter(purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = NULL;
    uint64_t byte_num = 0;

    if (nr_args != 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }
    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_native(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }
    rwstream = get_rwstream_from_variant(argv[0]);
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (argv[1] != PURC_VARIANT_INVALID) {
        purc_variant_cast_to_ulongint(argv[1], &byte_num, false);
    }

    if (byte_num == 0) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        ret_var = PURC_VARIANT_INVALID;
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
    if (silently)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
stream_writebytes_getter(purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = NULL;

    if (nr_args != 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }
    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_native(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }
    if (argv[1] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_bsequence(argv[1]) &&
             !purc_variant_is_string(argv[1]))
            ) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    rwstream = get_rwstream_from_variant(argv[0]);
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    size_t bsize = 0;
    const unsigned char *buffer = NULL;
    if (purc_variant_is_bsequence(argv[1])) {
        buffer = purc_variant_get_bytes_const (argv[1], &bsize);
    }
    else {
        buffer = (const unsigned char *)purc_variant_get_string_const(argv[1]);
        bsize = strlen((const char*)buffer);
    }
    if (buffer && bsize) {
        ssize_t nr_write = purc_rwstream_write (rwstream, buffer, bsize);
        return purc_variant_make_ulongint(nr_write);
    }

    return ret_var;

out:
    if (silently)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
stream_seek_getter(purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = NULL;
    int64_t byte_num = 0;
    off_t off = 0;
    int64_t whence = 0;

    if (nr_args != 3) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto out;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_native(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }
    if (argv[2] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string(argv[2]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    const char* option = purc_variant_get_string_const(argv[2]);
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

    rwstream = get_rwstream_from_variant(argv[0]);
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    if (argv[1] != PURC_VARIANT_INVALID) {
        purc_variant_cast_to_longint(argv[1], &byte_num, false);
    }

    off = purc_rwstream_seek(rwstream, byte_num, (int)whence);
    ret_var = purc_variant_make_longint(off);

    return ret_var;

out:
    if (silently)
        return purc_variant_make_undefined();
    return PURC_VARIANT_INVALID;
}

bool add_stdio_property(purc_variant_t v)
{
    static const struct purc_native_ops ops = {
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

    return true;

out_unref_var:
    purc_variant_unref(var);

out:
    return false;
}

purc_variant_t pcdvobjs_create_stream(void)
{
    static struct purc_dvobj_method  stream[] = {
        {"open",        stream_open_getter,        NULL},
        {"readstruct",  stream_readstruct_getter,  NULL},
        {"writestruct", stream_writestruct_getter, NULL},
        {"readlines",   stream_readlines_getter,   NULL},
        {"writelines",  stream_writelines_getter,  NULL},
        {"readbytes",   stream_readbytes_getter,   NULL},
        {"writebytes",  stream_writebytes_getter,  NULL},
        {"seek",        stream_seek_getter,        NULL},
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

#if 0
    if (add_stdio_property(v)) {
        return v;
    }
#else
    return v;
#endif

    purc_variant_unref(v);
    return PURC_VARIANT_INVALID;

}

purc_variant_t __purcex_load_dynamic_variant(const char *name, int *ver_code)
{
    UNUSED_PARAM(name);
    *ver_code = DVOBJ_STREAM_VERSION;

    return pcdvobjs_create_stream();
}

size_t __purcex_get_number_of_dynamic_variants(void)
{
    return 1;
}

const char * __purcex_get_dynamic_variant_name(size_t idx)
{
    if (idx != 0)
        return NULL;

    return DVOBJ_STREAM_NAME;
}

const char * __purcex_get_dynamic_variant_desc(size_t idx)
{
    if (idx != 0)
        return NULL;

    return DVOBJ_STREAM_DESC;
}

