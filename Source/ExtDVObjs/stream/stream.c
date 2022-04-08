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
#include "private/instance.h"
#include "private/errors.h"
#include "private/dvobjs.h"
#include "purc-variant.h"
#include "purc-runloop.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

#define BUFFER_SIZE         1024
#define ENDIAN_PLATFORM     0
#define ENDIAN_LITTLE       1
#define ENDIAN_BIG          2

#define SCHEMA_FILE         "file"
#define SCHEMA_PIPE         "pipe"
#define SCHEMA_UNIX_SOCK    "unix"
#define SCHEMA_WIN_SOCK     "winsock"
#define SCHEMA_WS           "ws"
#define SCHEMA_WSS          "wss"

#define STDIN_NAME          "stdin"
#define STDOUT_NAME         "stdout"
#define STDERR_NAME         "stderr"


#define PIPO_DEFAULT_MODE       0777
#define RWSTREAM_FD_BUFFER      1024

#define STREAM_DVOBJ_VERSION  0
#define STREAM_DESCRIPTION    "For io stream operations in PURC"

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
    switch (type) {
    case STREAM_TYPE_FILE_STDIN:
        fp = stdin;
        break;

    case STREAM_TYPE_FILE_STDOUT:
        fp = stdout;
        break;

    case STREAM_TYPE_FILE_STDERR:
        fp = stderr;
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

    return stream;

out_free_stream:
    dvobjs_stream_destroy(stream);

out:
    return NULL;
}

inline
struct pcdvobjs_stream *create_file_stdin_stream()
{
    return create_file_std_stream(STREAM_TYPE_FILE_STDIN);
}

inline
struct pcdvobjs_stream *create_file_stdout_stream()
{
    return create_file_std_stream(STREAM_TYPE_FILE_STDOUT);
}

inline
struct pcdvobjs_stream *create_file_stderr_stream()
{
    return create_file_std_stream(STREAM_TYPE_FILE_STDERR);
}

static
struct pcdvobjs_stream *create_file_stream(struct purc_broken_down_url *url,
        purc_variant_t option)
{
    struct pcdvobjs_stream* stream = dvobjs_stream_create(STREAM_TYPE_FILE,
            url, option);
    if (!stream) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    stream->rws = purc_rwstream_new_from_file(url->path,
            purc_variant_get_string_const(option));
    if (stream->rws == NULL) {
        goto out_free_stream;
    }


    return stream;

out_free_stream:
    dvobjs_stream_destroy(stream);

out:
    return NULL;
}

// option: r(read), w(write), n(nonblock)
static
struct pcdvobjs_stream *create_pipe_stream(struct purc_broken_down_url *url,
        purc_variant_t option)
{
    UNUSED_PARAM(url);
    UNUSED_PARAM(option);
    const char* option_s = purc_variant_get_string_const(option);

    if (!is_file_exists(url->path)) {
         int ret = mkfifo(url->path, PIPO_DEFAULT_MODE);
         if (ret != 0) {
             purc_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
             return NULL;
         }
    }

    int rw = 0;
    int flags = 0;
    // parse option
    while (*option_s) {
        switch (*option_s) {
            break;
            rw = rw | 0x1;

        case 'w':
            rw = rw | 0x2;
            break;

        case 'n':
            flags = flags | O_NONBLOCK;
            break;
        }
        option_s++;
    }

    switch (rw) {
    case 1:
        flags = flags | O_RDONLY;
        break;
    case 2:
        flags = flags | O_WRONLY;
        break;
    case 3:
        flags = flags | O_RDWR;
        break;
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

    stream->rws = purc_rwstream_new_from_unix_fd(fd, RWSTREAM_FD_BUFFER);
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

    stream->rws = purc_rwstream_new_from_unix_fd(fd, RWSTREAM_FD_BUFFER);
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

// stream native variant

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

    if (nr_args != 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if (argv[1] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string(argv[1]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    struct purc_broken_down_url *url= (struct purc_broken_down_url*)
        calloc(1, sizeof(struct purc_broken_down_url));
    if (!pcutils_url_break_down(url, purc_variant_get_string_const(argv[0]))) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    struct pcdvobjs_stream* stream = NULL;
    if (strcmp(SCHEMA_FILE, url->schema) == 0) {
        stream = create_file_stream(url, argv[1]);
    }
    else if (strcmp(SCHEMA_PIPE, url->schema) == 0) {
        stream = create_pipe_stream(url, argv[1]);
    }
    else if (strcmp(SCHEMA_UNIX_SOCK, url->schema) == 0) {
        stream = create_unix_sock_stream(url, argv[1]);
    }
#if 0
    // TODO
    else if (strcmp(SCHEMA_WIN_SOCK, url.schema) == 0) {
    }
    else if (strcmp(SCHEMA_WS, url.schema) == 0) {
    }
    else if (strcmp(SCHEMA_WSS, url.schema) == 0) {
    }
#endif

    if (!stream) {
        goto out_free_url;
    }

    // setup a callback for `on_release` to destroy the stream automatically
    static const struct purc_native_ops ops = {
        .on_release = on_release,
    };
    ret_var = purc_variant_make_native(stream, &ops);
    return ret_var;

out_free_url:
    purc_broken_down_url_destroy(url);

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

/*
   According to IEEE 754
    sign    e      base   offset
16   1      5       10      15
32   1      8       23     127
64   1      11      52     1023
96   1      15      64     16383
128  1      15      64     16383
*/

static purc_variant_t
read_rwstream_float(purc_rwstream_t rwstream, int type, int bytes)
{
    purc_variant_t val = PURC_VARIANT_INVALID;
    unsigned char buf[128];
    float f = 0.0F;
    double d = 0.0;
    long double ld = 0.0L;
    unsigned long long sign = 0;
    unsigned long long e = 0;
    unsigned long long base = 0;
    unsigned short number = 0;

    // change byte order to little endian
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

    switch (bytes) {
        case 2:
            number = *((unsigned short *)buf);
            sign = number >> 15;
            e = (number >> 10) & 0x1F;
            base = number & 0x3FF;

            sign = sign << 63;
            e = 1023 + e - 15;
            e = e << 52;
            sign |= e;
            base = base << (52 - 10);
            sign |= base;
            memcpy(buf, &sign, 8);

            d = *((double *)buf);
            val = purc_variant_make_number(d);
            break;
        case 4:
            f = *((float *)buf);
            d = (double)f;
            val = purc_variant_make_number(d);
            break;
        case 8:
            d = *((double *)buf);
            val = purc_variant_make_number(d);
            break;
        case 12:
        case 16:
            ld = *((long double *)buf);
            val = purc_variant_make_longdouble(ld);
            break;
    }

    return val;
}


static purc_variant_t
stream_readstruct_getter(purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, bool silently)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(silently);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = NULL;
    const char *format = NULL;
    const char *head = NULL;
    size_t length = 0;
    unsigned char buf[64];
    unsigned char * buffer = NULL;  // for string and bytes sequence
    int64_t i64 = 0;
    uint64_t u64 = 0;
    int read_number = 0;

    if (nr_args != 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_native(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    rwstream = get_rwstream_from_variant(argv[0]);
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if (argv[1] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string(argv[1]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    format = purc_variant_get_string_const(argv[1]);
    head = pcutils_get_next_token(format, " \t\n", &length);

    ret_var = purc_variant_make_array(0, PURC_VARIANT_INVALID);

    while (head) {
        switch (* head)
        {
            case 'i':
            case 'I':
                *((int64_t *)buf) = 0;
                if (pcutils_strncasecmp(head, "i8", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_PLATFORM, 1);
                else if (pcutils_strncasecmp(head, "i16", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_PLATFORM, 2);
                else if (pcutils_strncasecmp(head, "i32", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_PLATFORM, 4);
                else if (pcutils_strncasecmp(head, "i64", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_PLATFORM, 8);
                else if (pcutils_strncasecmp(head, "i16le", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_LITTLE, 2);
                else if (pcutils_strncasecmp(head, "i32le", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_LITTLE, 4);
                else if (pcutils_strncasecmp(head, "i64le", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_LITTLE, 8);
                else if (pcutils_strncasecmp(head, "i16be", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_BIG, 2);
                else if (pcutils_strncasecmp(head, "i32be", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_BIG, 4);
                else if (pcutils_strncasecmp(head, "i64be", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_BIG, 8);

                i64 = (int64_t)(*((int64_t *)buf));
                val = purc_variant_make_longint(i64);
                break;
            case 'f':
            case 'F':
                *((float *)buf) = 0;
                if (pcutils_strncasecmp(head, "f16", length) == 0)
                    val = read_rwstream_float(rwstream, ENDIAN_PLATFORM, 2);
                else if (pcutils_strncasecmp(head, "f32", length) == 0)
                    val = read_rwstream_float(rwstream, ENDIAN_PLATFORM, 4);
                else if (pcutils_strncasecmp(head, "f64", length) == 0)
                    val = read_rwstream_float(rwstream, ENDIAN_PLATFORM, 8);
                else if (pcutils_strncasecmp(head, "f96", length) == 0)
                    val = read_rwstream_float(rwstream, ENDIAN_PLATFORM, 12);
                else if (pcutils_strncasecmp(head, "f128", length) == 0)
                    val = read_rwstream_float(rwstream, ENDIAN_PLATFORM, 16);

                else if (pcutils_strncasecmp(head, "f16le", length) == 0)
                    val = read_rwstream_float(rwstream, ENDIAN_LITTLE, 2);
                else if (pcutils_strncasecmp(head, "f32le", length) == 0)
                    val = read_rwstream_float(rwstream, ENDIAN_LITTLE, 4);
                else if (pcutils_strncasecmp(head, "f64le", length) == 0)
                    val = read_rwstream_float(rwstream, ENDIAN_LITTLE, 8);
                else if (pcutils_strncasecmp(head, "f96le", length) == 0)
                    val = read_rwstream_float(rwstream, ENDIAN_LITTLE, 12);
                else if (pcutils_strncasecmp(head, "f128le", length) == 0)
                    val = read_rwstream_float(rwstream, ENDIAN_LITTLE, 16);

                else if (pcutils_strncasecmp(head, "f16be", length) == 0)
                    val = read_rwstream_float(rwstream, ENDIAN_BIG, 2);
                else if (pcutils_strncasecmp(head, "f32be", length) == 0)
                    val = read_rwstream_float(rwstream, ENDIAN_BIG, 4);
                else if (pcutils_strncasecmp(head, "f64be", length) == 0)
                    val = read_rwstream_float(rwstream, ENDIAN_BIG, 8);
                else if (pcutils_strncasecmp(head, "f96be", length) == 0)
                    val = read_rwstream_float(rwstream, ENDIAN_BIG, 12);
                else if (pcutils_strncasecmp(head, "f128be", length) == 0)
                    val = read_rwstream_float(rwstream, ENDIAN_BIG, 16);
                break;
            case 'u':
            case 'U':
                *((uint64_t *)buf) = 0;
                if (pcutils_strncasecmp(head, "u8", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_PLATFORM, 1);
                else if (pcutils_strncasecmp(head, "u16", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_PLATFORM, 2);
                else if (pcutils_strncasecmp(head, "u32", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_PLATFORM, 4);
                else if (pcutils_strncasecmp(head, "u64", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_PLATFORM, 8);
                else if (pcutils_strncasecmp(head, "u16le", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_LITTLE, 2);
                else if (pcutils_strncasecmp(head, "u32le", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_LITTLE, 4);
                else if (pcutils_strncasecmp(head, "u64le", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_LITTLE, 8);
                else if (pcutils_strncasecmp(head, "u16be", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_BIG, 2);
                else if (pcutils_strncasecmp(head, "u32be", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_BIG, 4);
                else if (pcutils_strncasecmp(head, "u64be", length) == 0)
                    read_rwstream(rwstream, buf, ENDIAN_BIG, 8);

                u64 = (uint64_t)(*((uint64_t *)buf));
                val = purc_variant_make_ulongint(u64);
                break;
            case 'b':
            case 'B':
                if (length > 1) { // get length
                    strncpy((char *)buf, head + 1, length - 1);
                    *(buf + length - 1)= 0x00;
                    read_number = atoi((char *)buf);

                    if (read_number) {
                        buffer = malloc(read_number);
                        if (buffer == NULL)
                            val = purc_variant_make_null();
                        else {
                            purc_rwstream_read(rwstream, buffer, read_number);
                            val = purc_variant_make_byte_sequence_reuse_buff(
                                        buffer, read_number, read_number);
                        }
                    }
                    else
                        val = purc_variant_make_null();
                }
                else
                    val = purc_variant_make_null();

                break;
            case 'p':
            case 'P':
                if (length > 1) { // get length
                    strncpy((char *)buf, head + 1, length - 1);
                    *(buf + length - 1)= 0x00;
                    read_number = atoi((char *)buf);

                    if (read_number) {
                        int i = 0;
                        int times = read_number / sizeof(long double);
                        int rest = read_number % sizeof(long double);
                        long double ld = 0;
                        for (i = 0; i < times; i++)
                            purc_rwstream_read(rwstream,
                                    &ld, sizeof(long double));
                        purc_rwstream_read(rwstream, &ld, rest);
                    }
                }
                break;
            case 's':
            case 'S':
                if (length > 1) {          // get length
                    strncpy((char *)buf, head + 1, length - 1);
                    *(buf + length - 1)= 0x00;
                    read_number = atoi((char *)buf);

                    if (read_number) {
                        buffer = malloc(read_number + 1);
                        if (buffer == NULL)
                            val = purc_variant_make_string("", false);
                        else {
                            purc_rwstream_read(rwstream, buffer, read_number);
                            *(buffer + read_number) = 0x00;
                            val = purc_variant_make_string_reuse_buff(
                                    (char *)buffer, read_number, false);
                        }
                    }
                    else
                        val = purc_variant_make_string("", false);
                }
                else {
                    int i = 0;
                    int j = 0;
                    size_t mem_size = BUFFER_SIZE;

                    buffer = malloc(mem_size);
                    for (i = 0, j = 0; ; i++, j++) {
                        ssize_t r = purc_rwstream_read(rwstream, buffer + i, 1);
                        if (r <= 0)
                            break;
                        if (*(buffer + i) == 0x00)
                            break;

                        if (j == 1023) {
                            j = 0;
                            mem_size += BUFFER_SIZE;
                            buffer = realloc(buffer, mem_size);
                        }
                    }
                    val = purc_variant_make_string_reuse_buff(
                            (char *)buffer, i, false);
                }
                break;
        }

        purc_variant_array_append(ret_var, val);
        purc_variant_unref(val);
        head = pcutils_get_next_token(head + length, " \t\n", &length);
    }
    return ret_var;
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

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = NULL;
    const char *format = NULL;
    const char *head = NULL;
    size_t length = 0;
    unsigned char buf[64];
    const unsigned char * buffer = NULL;  // for string and bytes sequence
    float f = 0.0F;
    double d = 0.0;
    long double ld = 0.0L;
    int write_number = 0;
    int i = 0;
    size_t bsize = 0;
    unsigned short ui16 = 0;
    size_t write_length = 0;

    if (nr_args != 3) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_native(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    rwstream = get_rwstream_from_variant(argv[0]);
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if (argv[1] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string(argv[1]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    if (argv[2] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_array(argv[2]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }

    format = purc_variant_get_string_const(argv[1]);
    head = pcutils_get_next_token(format, " \t\n", &length);

    while (head) {
        switch (* head)
        {
            case 'i':
            case 'I':
                if (pcutils_strncasecmp(head, "i8", length) == 0)
                    write_rwstream_int(rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 1, &write_length);
                else if (pcutils_strncasecmp(head, "i16", length) == 0)
                    write_rwstream_int(rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 2, &write_length);
                else if (pcutils_strncasecmp(head, "i32", length) == 0)
                    write_rwstream_int(rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 4, &write_length);
                else if (pcutils_strncasecmp(head, "i64", length) == 0)
                    write_rwstream_int(rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 8, &write_length);
                else if (pcutils_strncasecmp(head, "i16le", length) == 0)
                    write_rwstream_int(rwstream,
                            argv[2], &i, ENDIAN_LITTLE, 2, &write_length);
                else if (pcutils_strncasecmp(head, "i32le", length) == 0)
                    write_rwstream_int(rwstream,
                            argv[2], &i, ENDIAN_LITTLE, 4, &write_length);
                else if (pcutils_strncasecmp(head, "i64le", length) == 0)
                    write_rwstream_int(rwstream,
                            argv[2], &i, ENDIAN_LITTLE, 8, &write_length);
                else if (pcutils_strncasecmp(head, "i16be", length) == 0)
                    write_rwstream_int(rwstream,
                            argv[2], &i, ENDIAN_BIG, 2, &write_length);
                else if (pcutils_strncasecmp(head, "i32be", length) == 0)
                    write_rwstream_int(rwstream,
                            argv[2], &i, ENDIAN_BIG, 4, &write_length);
                else if (pcutils_strncasecmp(head, "i64be", length) == 0)
                    write_rwstream_int(rwstream,
                            argv[2], &i, ENDIAN_BIG, 8, &write_length);
                break;
            case 'f':
            case 'F':
                if (pcutils_strncasecmp(head, "f16", length) == 0) {
                    val = purc_variant_array_get(argv[2], i);
                    i++;
                    purc_variant_cast_to_number(val, &d, false);
                    ui16 = write_double_to_16 (d, ENDIAN_PLATFORM);
                    purc_rwstream_write(rwstream, &ui16, 2);
                    write_length += 2;
                }
                else if (pcutils_strncasecmp(head, "f32", length) == 0) {
                    val = purc_variant_array_get(argv[2], i);
                    i++;
                    purc_variant_cast_to_number(val, &d, false);
                    f = (float)d;
                    purc_rwstream_write(rwstream, (char *)&f, 4);
                    write_length += 4;
                }
                else if (pcutils_strncasecmp(head, "f64", length) == 0) {
                    val = purc_variant_array_get(argv[2], i);
                    i++;
                    purc_variant_cast_to_number(val, &d, false);
                    purc_rwstream_write(rwstream, (char *)&d, 8);
                    write_length += 8;
                }
                else if (pcutils_strncasecmp(head, "f96", length) == 0) {
                    val = purc_variant_array_get(argv[2], i);
                    i++;
                    purc_variant_cast_to_longdouble(val, &ld, false);
                    purc_rwstream_write(rwstream, (char *)&ld, 12);
                    write_length += 12;
                }
                else if (pcutils_strncasecmp(head, "f128", length) == 0) {
                    val = purc_variant_array_get(argv[2], i);
                    i++;
                    purc_variant_cast_to_longdouble(val, &ld, false);
                    purc_rwstream_write(rwstream, (char *)&ld, 16);
                    write_length += 16;
                }
                else if (pcutils_strncasecmp(head, "f16be", length) == 0) {
                    val = purc_variant_array_get(argv[2], i);
                    i++;
                    purc_variant_cast_to_number(val, &d, false);
                    ui16 = write_double_to_16 (d, ENDIAN_BIG);
                    purc_rwstream_write(rwstream, &ui16, 2);
                    write_length += 2;
                }
                else if (pcutils_strncasecmp(head, "f32be", length) == 0) {
                    val = purc_variant_array_get(argv[2], i);
                    i++;
                    purc_variant_cast_to_number(val, &d, false);
                    f = (float)d;
                    if (is_little_endian())
                        change_order((unsigned char *)&f, sizeof(float));
                    purc_rwstream_write(rwstream, (char *)&f, 4);
                    write_length += 4;
                }
                else if (pcutils_strncasecmp(head, "f64be", length) == 0) {
                    val = purc_variant_array_get(argv[2], i);
                    i++;
                    purc_variant_cast_to_number(val, &d, false);
                    if (is_little_endian())
                        change_order((unsigned char *)&d, sizeof(double));
                    purc_rwstream_write(rwstream, (char *)&d, 8);
                    write_length += 8;
                }
                else if (pcutils_strncasecmp(head, "f96be", length) == 0) {
                    val = purc_variant_array_get(argv[2], i);
                    i++;
                    purc_variant_cast_to_longdouble(val, &ld, false);
                    if (is_little_endian())
                        change_order((unsigned char *)&ld,
                                sizeof(long double));
                    purc_rwstream_write(rwstream, (char *)&ld, 12);
                    write_length += 12;
                }
                else if (pcutils_strncasecmp(head, "f128be", length) == 0) {
                    val = purc_variant_array_get(argv[2], i);
                    i++;
                    purc_variant_cast_to_longdouble(val, &ld, false);
                    if (is_little_endian())
                        change_order((unsigned char *)&ld,
                                sizeof(long double));
                    purc_rwstream_write(rwstream, (char *)&ld, 16);
                    write_length += 16;
                }
                else if (pcutils_strncasecmp(head, "f16le", length) == 0) {
                    val = purc_variant_array_get(argv[2], i);
                    i++;
                    purc_variant_cast_to_number(val, &d, false);
                    ui16 = write_double_to_16 (d, ENDIAN_LITTLE);
                    purc_rwstream_write(rwstream, &ui16, 2);
                    write_length += 2;
                }
                else if (pcutils_strncasecmp(head, "f32le", length) == 0) {
                    val = purc_variant_array_get(argv[2], i);
                    i++;
                    purc_variant_cast_to_number(val, &d, false);
                    f = (float)d;
                    if (!is_little_endian())
                        change_order((unsigned char *)&f, sizeof(float));
                    purc_rwstream_write(rwstream, (char *)&f, 4);
                    write_length += 4;
                }
                else if (pcutils_strncasecmp(head, "f64le", length) == 0) {
                    val = purc_variant_array_get(argv[2], i);
                    i++;
                    purc_variant_cast_to_number(val, &d, false);
                    if (!is_little_endian())
                        change_order((unsigned char *)&d, sizeof(double));
                    purc_rwstream_write(rwstream, (char *)&d, 8);
                    write_length += 8;
                }
                else if (pcutils_strncasecmp(head, "f96le", length) == 0) {
                    val = purc_variant_array_get(argv[2], i);
                    i++;
                    purc_variant_cast_to_longdouble(val, &ld, false);
                    if (!is_little_endian())
                        change_order((unsigned char *)&ld,
                                sizeof(long double));
                    purc_rwstream_write(rwstream, (char *)&ld, 12);
                    write_length += 12;
                }
                else if (pcutils_strncasecmp(head, "f128le", length) == 0) {
                    val = purc_variant_array_get(argv[2], i);
                    i++;
                    purc_variant_cast_to_longdouble(val, &ld, false);
                    if (!is_little_endian())
                        change_order((unsigned char *)&ld,
                                sizeof(long double));
                    purc_rwstream_write(rwstream, (char *)&ld, 16);
                    write_length += 16;
                }
                break;
            case 'u':
            case 'U':
                if (pcutils_strncasecmp(head, "u8", length) == 0)
                    write_rwstream_uint(rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 1, &write_length);
                else if (pcutils_strncasecmp(head, "u16", length) == 0)
                    write_rwstream_uint(rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 2, &write_length);
                else if (pcutils_strncasecmp(head, "u32", length) == 0)
                    write_rwstream_uint(rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 4, &write_length);
                else if (pcutils_strncasecmp(head, "u64", length) == 0) {
                    write_rwstream_uint(rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 8, &write_length);
                }
                else if (pcutils_strncasecmp(head, "u16le", length) == 0)
                    write_rwstream_uint(rwstream,
                            argv[2], &i, ENDIAN_LITTLE, 2, &write_length);
                else if (pcutils_strncasecmp(head, "u32le", length) == 0)
                    write_rwstream_uint(rwstream,
                            argv[2], &i, ENDIAN_LITTLE, 4, &write_length);
                else if (pcutils_strncasecmp(head, "u64le", length) == 0)
                    write_rwstream_uint(rwstream,
                            argv[2], &i, ENDIAN_LITTLE, 8, &write_length);
                else if (pcutils_strncasecmp(head, "u16be", length) == 0)
                    write_rwstream_uint(rwstream,
                            argv[2], &i, ENDIAN_BIG, 2, &write_length);
                else if (pcutils_strncasecmp(head, "u32be", length) == 0)
                    write_rwstream_uint(rwstream,
                            argv[2], &i, ENDIAN_BIG, 4, &write_length);
                else if (pcutils_strncasecmp(head, "u64be", length) == 0)
                    write_rwstream_uint(rwstream,
                            argv[2], &i, ENDIAN_BIG, 8, &write_length);

                break;
            case 'b':
            case 'B':
                val = purc_variant_array_get(argv[2], i);
                i++;

                // get sequence length
                if (length > 1) {
                    strncpy((char *)buf, head + 1, length - 1);
                    *(buf + length - 1)= 0x00;
                    write_number = atoi((char *)buf);

                    if (write_number) {
                        bsize = write_number;
                        buffer = purc_variant_get_bytes_const(val, &bsize);
                        purc_rwstream_write(rwstream, buffer, write_number);
                        write_length += write_number;
                    }
                }
                break;

            case 'p':
            case 'P':
                val = purc_variant_array_get(argv[2], i);
                i++;

                // get white space length
                if (length > 1) {
                    strncpy((char *)buf, head + 1, length - 1);
                    *(buf + length - 1)= 0x00;
                    write_number = atoi((char *)buf);

                    if (write_number) {
                        int i = 0;
                        int times = write_number / sizeof(long double);
                        int rest = write_number % sizeof(long double);
                        ld = 0;
                        for (i = 0; i < times; i++)
                            purc_rwstream_write(rwstream,
                                    &ld, sizeof(long double));
                        purc_rwstream_write(rwstream, &ld, rest);
                        write_length += write_number;
                    }
                }
                break;

            case 's':
            case 'S':
                val = purc_variant_array_get(argv[2], i);
                i++;

                // get string length
                if (length > 1) {
                    strncpy((char *)buf, head + 1, length - 1);
                    *(buf + length - 1)= 0x00;
                    write_number = atoi((char *)buf);
                } else {
                    write_number = purc_variant_string_size(val);
                }

                if (write_number) {
                    buffer = (unsigned char *)purc_variant_get_string_const
                        (val);
                    purc_rwstream_write(rwstream, buffer, write_number);
                    write_length += write_number;
                }
                break;
        }
        head = pcutils_get_next_token(head + length, " \t\n", &length);
    }

    ret_var = purc_variant_make_ulongint(write_length);
    return ret_var;
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
        return PURC_VARIANT_INVALID;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_native(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    rwstream = get_rwstream_from_variant(argv[0]);
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
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
        return PURC_VARIANT_INVALID;
    }
    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_native(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    rwstream = get_rwstream_from_variant(argv[0]);
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
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
            return PURC_VARIANT_INVALID;
        }

        size = purc_rwstream_read(rwstream, content, byte_num);
        if (size > 0)
            ret_var =
                purc_variant_make_byte_sequence_reuse_buff(content, size, size);
        else {
            free(content);
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            ret_var = PURC_VARIANT_INVALID;
        }
    }

    return ret_var;
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
        return PURC_VARIANT_INVALID;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_native(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        return PURC_VARIANT_INVALID;
    }
    rwstream = get_rwstream_from_variant(argv[0]);
    if (rwstream == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if (argv[1] != PURC_VARIANT_INVALID) {
        purc_variant_cast_to_longint(argv[1], &byte_num, false);
    }

    if (argv[2] != PURC_VARIANT_INVALID) {
        purc_variant_cast_to_longint(argv[2], &whence, false);
    }

    off = purc_rwstream_seek(rwstream, byte_num, (int)whence);
    ret_var = purc_variant_make_longint(off);

    return ret_var;
}

bool add_stdio_property(purc_variant_t v)
{
    static const struct purc_native_ops ops = {
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
    if (!purc_variant_object_set_by_static_ckey(v, STDIN_NAME, var)) {
        goto out_unref_var;;
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
    if (!purc_variant_object_set_by_static_ckey(v, STDOUT_NAME, var)) {
        goto out_unref_var;;
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
    if (!purc_variant_object_set_by_static_ckey(v, STDERR_NAME, var)) {
        goto out_unref_var;;
    }

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
        {"readbytes",   stream_readbytes_getter,   NULL},
        {"seek",        stream_seek_getter,        NULL},
    };

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

purc_variant_t __purcex_load_dynamic_variant(const char *name, int *ver_code)
{
    UNUSED_PARAM(name);
    *ver_code = STREAM_DVOBJ_VERSION;

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

    return "STREAM";
}

const char * __purcex_get_dynamic_variant_desc(size_t idx)
{
    if (idx != 0)
        return NULL;

    return STREAM_DESCRIPTION;
}

