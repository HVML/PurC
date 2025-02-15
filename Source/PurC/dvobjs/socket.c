/*
 * @file socket.c
 * @author Vincent Wei
 * @date 2025/02/05
 * @brief The implementation of socket dynamic variant object.
 *
 * Copyright (C) 2025 FMSoft <https://www.fmsoft.cn>
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
#include "private/ports.h"

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

#define SOCKET_EVENT_NAME               "socket"
#define SOCKET_SUB_EVENT_CONNATTEMPT    "connAttempt"
#define SOCKET_SUB_EVENT_NEWDATAGRAM    "newDatagram"

#define MAX_LEN_KEYWORD             64
#define _KW_DELIMITERS              " \t\n\v\f\r"

#define SOCKET_ATOM_BUCKET              ATOM_BUCKET_DVOBJ

enum {
#define _KW_local                   "local"
    K_KW_local,
#define _KW_unix                    "unix"
    K_KW_unix,
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
#define _KW_fd                      "fd"
    K_KW_fd,
#define _KW_accept                  "accept"
    K_KW_accept,
#define _KW_sendto                  "sendto"
    K_KW_sendto,
#define _KW_recvfrom                "recvfrom"
    K_KW_recvfrom,
#define _KW_close                   "close"
    K_KW_close,
#define _KW_default                 "default"
    K_KW_default,
#define _KW_nonblock                "nonblock"
    K_KW_nonblock,
#define _KW_cloexec                 "cloexec"
    K_KW_cloexec,
#define _KW_global                  "global"
    K_KW_global,
#define _KW_nameless                "nameless"
    K_KW_nameless,
#define _KW_dontwait                "dontwait"
    K_KW_dontwait,
#define _KW_confirm                 "confirm"
    K_KW_confirm,
#define _KW_nosource                "nosource"
    K_KW_nosource,
#define _KW_trunc                   "trunc"
    K_KW_trunc,
};

static struct keyword_to_atom {
    const char *keyword;
    purc_atom_t atom;
} keywords2atoms [] = {
    { _KW_local, 0 },               // "local"
    { _KW_unix, 0 },                // "unix"
    { _KW_inet, 0},                 // "inet"
    { _KW_inet4, 0},                // "inet4"
    { _KW_inet6, 0},                // "inet6"
    { _KW_websocket, 0 },           // "websocket"
    { _KW_message, 0 },             // "message"
    { _KW_hbdbus, 0 },              // "hbdbus"
    { _KW_fd, 0},                   // "fd"
    { _KW_accept, 0},               // "accept"
    { _KW_sendto, 0},               // "sendto"
    { _KW_recvfrom, 0},             // "recvfrom"
    { _KW_close, 0},                // "close"
    { _KW_default, 0},              // "default"
    { _KW_nonblock, 0},             // "nonblock"
    { _KW_cloexec, 0},              // "cloexec"
    { _KW_global, 0},               // "global"
    { _KW_nameless, 0},             // "nameless"
    { _KW_dontwait, 0},             // "dontwait"
    { _KW_confirm, 0},              // "confirm"
    { _KW_nosource, 0},             // "nosource"
    { _KW_trunc, 0},                // "trunc"
};

/* We use the high 32-bit for customized flags */
#define _O_GLOBAL       (0x01L << 32)
#define _O_NAMELESS     (0x02L << 32)
#define _O_DONTWAIT     (0x04L << 32)
#define _O_CONFIRM      (0x08L << 32)
#define _O_NOSOURCE     (0x10L << 32)
#define _O_TRUNC        (0x20L << 32)

static
int64_t parse_socket_stream_option(purc_variant_t option)
{
    purc_atom_t atom = 0;
    size_t parts_len;
    const char *parts;
    int64_t flags = 0;

    if (option == PURC_VARIANT_INVALID) {
        atom = keywords2atoms[K_KW_default].atom;
    }
    else {
        parts = purc_variant_get_string_const_ex(option, &parts_len);
        if (parts == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto done;
        }

        parts = pcutils_trim_spaces(parts, &parts_len);
        if (parts_len == 0) {
            atom = keywords2atoms[K_KW_default].atom;
        }
    }

    if (atom == 0) {
        char tmp[parts_len + 1];
        strncpy(tmp, parts, parts_len);
        tmp[parts_len]= '\0';
        atom = purc_atom_try_string_ex(SOCKET_ATOM_BUCKET, tmp);
    }

    if (atom != keywords2atoms[K_KW_default].atom) {
        size_t length = 0;
        const char *part = pcutils_get_next_token_len(parts, parts_len,
                _KW_DELIMITERS, &length);
        do {
            if (length == 0 || length > MAX_LEN_KEYWORD) {
                atom = 0;
            }
            else {
                char tmp[length + 1];
                strncpy(tmp, part, length);
                tmp[length]= '\0';
                atom = purc_atom_try_string_ex(SOCKET_ATOM_BUCKET, tmp);
            }

            if (atom == keywords2atoms[K_KW_global].atom) {
                flags |= _O_GLOBAL;
            }
            else if (atom == keywords2atoms[K_KW_nonblock].atom) {
                flags |= O_NONBLOCK;
            }
            else if (atom == keywords2atoms[K_KW_cloexec].atom) {
                flags |= O_CLOEXEC;
            }
            else {
                flags = -1;
                break;
            }

            if (parts_len <= length)
                break;

            parts_len -= length;
            part = pcutils_get_next_token_len(part + length, parts_len,
                    _KW_DELIMITERS, &length);
        } while (part);
    }
    else {
        flags = O_CLOEXEC;
    }

done:
    return flags;
}

static
int64_t parse_socket_dgram_option(purc_variant_t option)
{
    purc_atom_t atom = 0;
    size_t parts_len;
    const char *parts;
    int64_t flags = 0;

    if (option == PURC_VARIANT_INVALID) {
        atom = keywords2atoms[K_KW_default].atom;
    }
    else {
        parts = purc_variant_get_string_const_ex(option, &parts_len);
        if (parts == NULL) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto done;
        }

        parts = pcutils_trim_spaces(parts, &parts_len);
        if (parts_len == 0) {
            atom = keywords2atoms[K_KW_default].atom;
        }
    }

    if (atom == 0) {
        char tmp[parts_len + 1];
        strncpy(tmp, parts, parts_len);
        tmp[parts_len]= '\0';
        atom = purc_atom_try_string_ex(SOCKET_ATOM_BUCKET, tmp);
    }

    if (atom != keywords2atoms[K_KW_default].atom) {
        size_t length = 0;
        const char *part = pcutils_get_next_token_len(parts, parts_len,
                _KW_DELIMITERS, &length);
        do {
            if (length == 0 || length > MAX_LEN_KEYWORD) {
                atom = 0;
            }
            else {
                char tmp[length + 1];
                strncpy(tmp, part, length);
                tmp[length]= '\0';
                atom = purc_atom_try_string_ex(SOCKET_ATOM_BUCKET, tmp);
            }

            if (atom == keywords2atoms[K_KW_global].atom) {
                flags |= _O_GLOBAL;
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
            else {
                flags = -1;
                break;
            }

            if (parts_len <= length)
                break;

            parts_len -= length;
            part = pcutils_get_next_token_len(part + length, parts_len,
                    _KW_DELIMITERS, &length);
        } while (part);
    }
    else {
        flags = O_CLOEXEC;
    }

done:
    return flags;
}

static int64_t
parse_dgram_sendto_option(purc_variant_t option)
{
    purc_atom_t atom = 0;
    size_t parts_len;
    const char *parts;
    int64_t flags = 0;

    parts = purc_variant_get_string_const_ex(option, &parts_len);
    parts = pcutils_trim_spaces(parts, &parts_len);
    if (parts_len == 0) {
        atom = keywords2atoms[K_KW_default].atom;
    }

    if (atom == 0) {
        char tmp[parts_len + 1];
        strncpy(tmp, parts, parts_len);
        tmp[parts_len]= '\0';
        atom = purc_atom_try_string_ex(SOCKET_ATOM_BUCKET, tmp);
    }

    if (atom != keywords2atoms[K_KW_default].atom) {
        size_t length = 0;
        const char *part = pcutils_get_next_token_len(parts, parts_len,
                _KW_DELIMITERS, &length);
        do {
            if (length == 0 || length > MAX_LEN_KEYWORD) {
                atom = 0;
            }
            else {
                char tmp[length + 1];
                strncpy(tmp, part, length);
                tmp[length]= '\0';
                atom = purc_atom_try_string_ex(SOCKET_ATOM_BUCKET, tmp);
            }

            if (atom == keywords2atoms[K_KW_dontwait].atom) {
                flags |= _O_DONTWAIT;
            }
            else if (atom == keywords2atoms[K_KW_confirm].atom) {
                flags |= _O_CONFIRM;
            }
            else {
                flags = -1;
                break;
            }

            if (parts_len <= length)
                break;

            parts_len -= length;
            part = pcutils_get_next_token_len(part + length, parts_len,
                    _KW_DELIMITERS, &length);
        } while (part);
    }

    return flags;
}

static int64_t
parse_dgram_recvfrom_option(purc_variant_t option)
{
    purc_atom_t atom = 0;
    size_t parts_len;
    const char *parts;
    int64_t flags = 0;

    parts = purc_variant_get_string_const_ex(option, &parts_len);
    parts = pcutils_trim_spaces(parts, &parts_len);
    if (parts_len == 0) {
        atom = keywords2atoms[K_KW_default].atom;
    }

    if (atom == 0) {
        char tmp[parts_len + 1];
        strncpy(tmp, parts, parts_len);
        tmp[parts_len]= '\0';
        atom = purc_atom_try_string_ex(SOCKET_ATOM_BUCKET, tmp);
    }

    if (atom != keywords2atoms[K_KW_default].atom) {
        size_t length = 0;
        const char *part = pcutils_get_next_token_len(parts, parts_len,
                _KW_DELIMITERS, &length);
        do {
            if (length == 0 || length > MAX_LEN_KEYWORD) {
                break;
            }
            else {
                char tmp[length + 1];
                strncpy(tmp, part, length);
                tmp[length]= '\0';
                atom = purc_atom_try_string_ex(SOCKET_ATOM_BUCKET, tmp);
            }

            if (atom == keywords2atoms[K_KW_dontwait].atom) {
                flags |= _O_DONTWAIT;
            }
            else if (atom == keywords2atoms[K_KW_nosource].atom) {
                flags |= _O_NOSOURCE;
            }
            else if (atom == keywords2atoms[K_KW_trunc].atom) {
                flags |= _O_TRUNC;
            }
            else {
                goto error;
            }

            if (parts_len <= length)
                break;

            parts_len -= length;
            part = pcutils_get_next_token_len(part + length, parts_len,
                    _KW_DELIMITERS, &length);
        } while (part);
    }

    return flags;

error:
    return -1;
}

static struct pcdvobjs_socket *
dvobjs_socket_new(enum pcdvobjs_socket_type type,
        struct purc_broken_down_url *url)
{
    struct pcdvobjs_socket *socket;

    socket = calloc(1, sizeof(struct pcdvobjs_socket));
    if (!socket) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    socket->type = type;
    socket->url = url;
    socket->fd = -1;
    return socket;
}

static void dvobjs_socket_close(struct pcdvobjs_socket *socket)
{
    if (socket->monitor) {
        purc_runloop_remove_fd_monitor(purc_runloop_get_current(),
                socket->monitor);
        socket->monitor = 0;
    }

    if (socket->fd >= 0) {
        close(socket->fd);
        socket->fd = -1;
    }

}

static void dvobjs_socket_delete(struct pcdvobjs_socket *socket)
{
    dvobjs_socket_close(socket);

    if (socket->url) {
        pcutils_broken_down_url_delete(socket->url);
    }

    free(socket);
}

static inline
struct pcdvobjs_socket *cast_to_socket(void *native_entity)
{
    return (struct pcdvobjs_socket*)native_entity;
}

static int
local_socket_accept_client(struct pcdvobjs_socket *socket, char **peer_addr)
{
    int                fd = -1;
    struct sockaddr_un addr;
    socklen_t          len = sizeof(addr);

    len = sizeof(addr);
    if ((fd = accept(socket->fd, (struct sockaddr *)&addr, &len)) < 0) {
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    /* obtain the peer address */
    if (strlen(addr.sun_path) == 0) {
        char buf[PURC_LEN_UNIQUE_ID + 1];
        purc_generate_unique_id(buf, "anonymous");
        *peer_addr = strdup(buf);
    }
    else {
        *peer_addr = strndup(addr.sun_path, sizeof(addr.sun_path));
    }

failed:
    return fd;
}

static int
inet_socket_accept_client(struct pcdvobjs_socket *socket,
        enum stream_inet_socket_family isf, char **peer_addr, char **peer_port)
{
    UNUSED_PARAM(isf);

    int fd = -1;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    if ((fd = accept(socket->fd, (struct sockaddr *)&addr, &len)) < 0) {
        PC_DEBUG("Failed accept(): %s\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    if (0 != getnameinfo((struct sockaddr *)&addr, len, hbuf, sizeof(hbuf),
                sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV)) {
        PC_DEBUG("Failed getnameinfo(): %s\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));

        close(fd);
        fd = -1;
    }
    else {
        *peer_addr = strdup(hbuf);
        *peer_port = strdup(sbuf);
    }

failed:
    return fd;
}

static
int parse_accept_option(purc_variant_t option)
{
    purc_atom_t atom = 0;
    size_t parts_len;
    const char *parts;
    int flags = 0;

    parts = purc_variant_get_string_const_ex(option, &parts_len);
    if (parts == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    parts = pcutils_trim_spaces(parts, &parts_len);
    if (parts_len == 0) {
        atom = keywords2atoms[K_KW_default].atom;
    }

    if (atom == 0) {
        char tmp[parts_len + 1];
        strncpy(tmp, parts, parts_len);
        tmp[parts_len]= '\0';
        atom = purc_atom_try_string_ex(SOCKET_ATOM_BUCKET, tmp);
    }

    if (atom != keywords2atoms[K_KW_default].atom) {
        size_t length = 0;
        const char *part = pcutils_get_next_token_len(parts, parts_len,
                _KW_DELIMITERS, &length);
        do {
            if (length == 0 || length > MAX_LEN_KEYWORD) {
                atom = keywords2atoms[K_KW_cloexec].atom;
            }
            else {
                char tmp[length + 1];
                strncpy(tmp, part, length);
                tmp[length]= '\0';
                atom = purc_atom_try_string_ex(SOCKET_ATOM_BUCKET, tmp);
            }

            if (atom == keywords2atoms[K_KW_nonblock].atom) {
                flags |= O_NONBLOCK;
            }
            else if (atom == keywords2atoms[K_KW_cloexec].atom) {
                flags |= O_CLOEXEC;
            }

            if (parts_len <= length)
                break;

            parts_len -= length;
            part = pcutils_get_next_token_len(part + length, parts_len,
                    _KW_DELIMITERS, &length);
        } while (part);
    }
    else {
        flags = O_CLOEXEC;
    }

    return flags;

failed:
    return -1;
}

static purc_variant_t
accept_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);

    assert(native_entity);
    struct pcdvobjs_socket *socket = cast_to_socket(native_entity);
    assert(socket->type == SOCKET_TYPE_STREAM);

    int fd = -1;
    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto error;
    }

    int flags = parse_accept_option(argv[0]);
    if (flags == -1) {
        goto error;
    }

    purc_atom_t schema = purc_atom_try_string_ex(SOCKET_ATOM_BUCKET,
            socket->url->schema);
    if (schema == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    char *peer_addr = NULL;
    char *peer_port = NULL;

    if (schema == keywords2atoms[K_KW_unix].atom ||
            schema == keywords2atoms[K_KW_local].atom) {
        fd = local_socket_accept_client(socket, &peer_addr);
    }
    else if (schema == keywords2atoms[K_KW_inet].atom) {
        fd = inet_socket_accept_client(socket, ISF_UNSPEC,
                &peer_addr, &peer_port);
    }
    else if (schema == keywords2atoms[K_KW_inet4].atom) {
        fd = inet_socket_accept_client(socket, ISF_INET4,
                &peer_addr, &peer_port);
    }
    else if (schema == keywords2atoms[K_KW_inet6].atom) {
        fd = inet_socket_accept_client(socket, ISF_INET6,
                &peer_addr, &peer_port);
    }
    else {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    }

    if (fd < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
        return purc_variant_make_null();
    }

    if (flags & O_CLOEXEC && fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    if (flags & O_NONBLOCK && fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    purc_variant_t stream =
        dvobjs_create_stream_by_accepted(schema, peer_addr, peer_port, fd,
                nr_args > 1 ? argv[1] : NULL,
                nr_args > 2 ? argv[2] : NULL);
    if (!stream) {
        goto error;
    }

    return stream;

error:
    if (fd >= 0)
        close(fd);

    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

struct addrinfo *get_network_address(enum stream_inet_socket_family isf,
        struct purc_broken_down_url *url)
{
    struct addrinfo *ai = NULL;
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

    char port[10] = {0};
    if (url->port == 0 || url->port > 65535) {
        PC_DEBUG("Bad port value: (%d)\n", url->port);
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }
    sprintf(port, "%d", url->port);

    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(url->host, port, &hints, &ai) != 0) {
        PC_DEBUG("Error while getting address info (%s:%d)\n",
                url->host, url->port);
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    return ai;

failed:
    return NULL;
}

static purc_variant_t
sendto_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);

    assert(native_entity);

    if (nr_args < 3) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto error;
    }

    if (!purc_variant_is_string(argv[0])) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    struct purc_broken_down_url *dst = (struct purc_broken_down_url*)
        calloc(1, sizeof(struct purc_broken_down_url));
    if (!pcutils_url_break_down(dst, purc_variant_get_string_const(argv[0]))) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error_free_url;
    }

    if (!purc_variant_is_string(argv[1])) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error_free_url;
    }

    int64_t flags = parse_dgram_sendto_option(argv[1]);
    if (flags == -1L) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error_free_url;
    }

    size_t bsize = 0;
    const char *bytes = NULL;
    if (purc_variant_is_bsequence(argv[2])) {
        bytes = (const char *)purc_variant_get_bytes_const(argv[2], &bsize);
    }
    else if (purc_variant_is_string(argv[2])) {
        bytes = (const char *)purc_variant_get_string_const(argv[2]);
        bsize = strlen(bytes) + 1;
    }
    else {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error_free_url;
    }

    uint64_t offset = 0;
    int64_t length = -1;
    if (nr_args > 3) {
        if (!purc_variant_cast_to_ulongint(argv[3], &offset, false)) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto error_free_url;
        }
    }

    if (nr_args > 4) {
        if (!purc_variant_cast_to_longint(argv[4], &length, false)) {
            purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
            goto error_free_url;
        }
    }

    if (offset > bsize) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error_free_url;
    }

    if (length > 0 && (offset + length) > bsize) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error_free_url;
    }
    else if (length < 0) {
        length = bsize - offset;
    }

    purc_atom_t schema =
        purc_atom_try_string_ex(SOCKET_ATOM_BUCKET, dst->schema);
    if (schema == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error_free_url;
    }

    struct addrinfo *ai = NULL;
    struct sockaddr_un *unix_addr = NULL;
    if (schema == keywords2atoms[K_KW_unix].atom ||
            schema == keywords2atoms[K_KW_local].atom) {

        if (strlen(dst->path) + 1 > sizeof(unix_addr->sun_path)) {
            purc_set_error(PURC_ERROR_TOO_LONG);
            goto error_free_url;
        }

        ai = calloc(1, sizeof(struct addrinfo));
        unix_addr = calloc(1, sizeof(struct sockaddr_un));
        unix_addr->sun_family = AF_UNIX;
        strcpy(unix_addr->sun_path, dst->path);

        ai->ai_addrlen = sizeof(unix_addr->sun_family);
        ai->ai_addrlen += strlen(unix_addr->sun_path) + 1;
        ai->ai_addr = (struct sockaddr *)unix_addr;
    }
    else if (schema == keywords2atoms[K_KW_inet].atom) {
        ai = get_network_address(ISF_UNSPEC, dst);
    }
    else if (schema == keywords2atoms[K_KW_inet4].atom) {
        ai = get_network_address(ISF_INET4, dst);
    }
    else if (schema == keywords2atoms[K_KW_inet6].atom) {
        ai = get_network_address(ISF_INET6, dst);
    }
    else {
        purc_set_error(PURC_ERROR_UNKNOWN);
        goto error_free_url;
    }

    pcutils_broken_down_url_delete(dst);

    if (ai == NULL) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    struct pcdvobjs_socket *socket = cast_to_socket(native_entity);
    ssize_t nr_sent = sendto(socket->fd, bytes + offset, length,
            ((flags & _O_DONTWAIT) ? MSG_DONTWAIT : 0)
#if OS(LINUX)
            | ((flags & _O_CONFIRM) ? MSG_CONFIRM : 0)
#endif
            , ai->ai_addr, ai->ai_addrlen);
    if (unix_addr) {
        free(unix_addr);
        free(ai);
    }
    else {
        freeaddrinfo(ai);
    }

    purc_variant_t retv = purc_variant_make_object_0();
    if (retv) {
        purc_variant_t tmp;

        tmp = purc_variant_make_longint(nr_sent);
        purc_variant_object_set_by_static_ckey(retv, "sent", tmp);
        purc_variant_unref(tmp);

        if (nr_sent >= 0) {
            tmp = purc_variant_make_null();
            purc_variant_object_set_by_static_ckey(retv, "errorname", tmp);
            purc_variant_unref(tmp);
        }
        else {
            tmp = purc_variant_make_string_static(
                    strerrorname_np(errno), false);
            purc_variant_object_set_by_static_ckey(retv, "errorname", tmp);
            purc_variant_unref(tmp);
        }
    }
    else {
        goto error;
    }

    return retv;

error_free_url:
    pcutils_broken_down_url_delete(dst);

error:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
recvfrom_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);

    assert(native_entity);

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto error;
    }

    if (!purc_variant_is_string(argv[0])) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    int64_t flags = parse_dgram_recvfrom_option(argv[0]);
    if (flags == -1L) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    int64_t bsize = 0;
    if (!purc_variant_cast_to_longint(argv[1], &bsize, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    void *buf = malloc(bsize);
    if (buf == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto error;
    }

    struct sockaddr_storage src_addr = { 0, };
    socklen_t addrlen = sizeof(src_addr);

    struct pcdvobjs_socket *socket = cast_to_socket(native_entity);
    ssize_t nr_recved = recvfrom(socket->fd, buf, bsize,
            ((flags & _O_DONTWAIT) ? MSG_DONTWAIT : 0) |
            ((flags & _O_TRUNC) ? MSG_TRUNC : 0),
            (flags & _O_NOSOURCE) ? NULL : (struct sockaddr *)&src_addr,
            (flags & _O_NOSOURCE) ? NULL : &addrlen);

    purc_variant_t retv = purc_variant_make_object_0();
    if (retv) {
        purc_variant_t tmp;

        tmp = purc_variant_make_longint(nr_recved);
        purc_variant_object_set_by_static_ckey(retv, "recved", tmp);
        purc_variant_unref(tmp);

        if (nr_recved >= 0) {
            tmp = purc_variant_make_byte_sequence_reuse_buff(buf,
                    (nr_recved > bsize) ? bsize : nr_recved, bsize);
            purc_variant_object_set_by_static_ckey(retv, "bytes", tmp);
            purc_variant_unref(tmp);

            tmp = purc_variant_make_null();
            purc_variant_object_set_by_static_ckey(retv, "errorname", tmp);
            purc_variant_unref(tmp);
        }
        else {
            free(buf);

            tmp = purc_variant_make_null();
            purc_variant_object_set_by_static_ckey(retv, "bytes", tmp);
            purc_variant_unref(tmp);

            tmp = purc_variant_make_string_static(
                    strerrorname_np(errno), false);
            purc_variant_object_set_by_static_ckey(retv, "errorname", tmp);
            purc_variant_unref(tmp);

            flags |= _O_NOSOURCE;
        }

        if (!(flags & _O_NOSOURCE) && src_addr.ss_family == AF_UNIX) {
            struct sockaddr_un *unix_addr = (struct sockaddr_un *)&src_addr;

            /* make sure there is a null terminate character */
            tmp = purc_variant_make_string_ex(unix_addr->sun_path,
                    sizeof(unix_addr->sun_path), false);
            purc_variant_object_set_by_static_ckey(retv, "source-addr", tmp);
            purc_variant_unref(tmp);

            tmp = purc_variant_make_null();
            purc_variant_object_set_by_static_ckey(retv, "source-port", tmp);
            purc_variant_unref(tmp);
        }
        else if (!(flags & _O_NOSOURCE)) {
            char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
            if (0 != getnameinfo((struct sockaddr *)&src_addr, addrlen,
                        hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),
                        NI_NUMERICHOST | NI_NUMERICSERV)) {
                PC_DEBUG("Failed getnameinfo(): %s\n", strerror(errno));
                flags |= _O_NOSOURCE;
            }
            else {
                tmp = purc_variant_make_string(hbuf, false);
                purc_variant_object_set_by_static_ckey(retv,
                        "source-addr", tmp);
                purc_variant_unref(tmp);

                tmp = purc_variant_make_longint(atol(sbuf));
                purc_variant_object_set_by_static_ckey(retv,
                        "source-port", tmp);
                purc_variant_unref(tmp);
            }
        }

        if (flags & _O_NOSOURCE) {
            tmp = purc_variant_make_null();
            purc_variant_object_set_by_static_ckey(retv, "source-addr", tmp);
            purc_variant_unref(tmp);

            tmp = purc_variant_make_null();
            purc_variant_object_set_by_static_ckey(retv, "source-port", tmp);
            purc_variant_unref(tmp);
        }
    }
    else {
        free(buf);
        goto error;
    }

    return retv;

error:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
fd_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    assert(native_entity);

    struct pcdvobjs_socket *socket = cast_to_socket(native_entity);
    return purc_variant_make_longint(socket->fd);
}

static purc_variant_t
close_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    assert(native_entity);

    struct pcdvobjs_socket *socket = cast_to_socket(native_entity);
    dvobjs_socket_close(socket);

    return purc_variant_make_boolean(true);
}

static purc_nvariant_method
property_getter(void *entity, const char *name)
{
    UNUSED_PARAM(entity);

    if (name == NULL) {
        goto failed;
    }

    purc_atom_t atom = purc_atom_try_string_ex(SOCKET_ATOM_BUCKET, name);
    if (atom == 0) {
        goto failed;
    }

    struct pcdvobjs_socket *socket = cast_to_socket(entity);
    if (socket->type == SOCKET_TYPE_STREAM) {
        if (atom == keywords2atoms[K_KW_accept].atom) {
            return accept_getter;
        }
    }
    else if (socket->type == SOCKET_TYPE_DGRAM) {
        if (atom == keywords2atoms[K_KW_sendto].atom) {
            return sendto_getter;
        }
        else if (atom == keywords2atoms[K_KW_recvfrom].atom) {
            return recvfrom_getter;
        }
    }

    if (atom == keywords2atoms[K_KW_close].atom) {
        return close_getter;
    }
    else if (atom == keywords2atoms[K_KW_fd].atom) {
        return fd_getter;
    }

failed:
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

struct io_callback_data {
    int                           fd;
    int                           io_event;
    struct pcdvobjs_socket       *socket;
};

static void on_socket_io_callback(struct io_callback_data *data)
{
    struct pcdvobjs_socket *socket = data->socket;

    const char* sub = NULL;
    if (socket->type == SOCKET_TYPE_STREAM) {
        sub = SOCKET_SUB_EVENT_CONNATTEMPT;
    }
    else if (socket->type == SOCKET_TYPE_DGRAM) {
        sub = SOCKET_SUB_EVENT_NEWDATAGRAM;
    }

    if (sub && socket->cid) {
        pcintr_coroutine_post_event(socket->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_IGNORE,
                socket->observed, SOCKET_EVENT_NAME, sub,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    }
}

static bool
socket_io_callback(int fd, int event, void *ctxt)
{
    struct pcdvobjs_socket *socket = (struct pcdvobjs_socket*)ctxt;
    PC_ASSERT(socket);

    struct io_callback_data data;
    data.fd = fd;
    data.io_event = event;
    data.socket = socket;

    on_socket_io_callback(&data);
    return true;
}

static const char *socket_events[] = {
#define MATCHED_CONNATTEMPT 0x01
    SOCKET_EVENT_NAME ":" SOCKET_SUB_EVENT_CONNATTEMPT,
#define MATCHED_NEWDATAGRAM 0x02
    SOCKET_EVENT_NAME ":" SOCKET_SUB_EVENT_NEWDATAGRAM,
};

static bool
on_observe(void *native_entity, const char *event_name,
        const char *event_subname)
{
    struct pcdvobjs_socket *socket = (struct pcdvobjs_socket*)native_entity;

    pcintr_coroutine_t co = pcintr_get_coroutine();
    if (co && socket->cid == 0) {
        socket->cid = co->cid;
    }

    int matched = pcdvobjs_match_events(event_name, event_subname,
            socket_events, PCA_TABLESIZE(socket_events));
    if (matched == -1)
        return false;

    uint32_t event = 0;
    if (socket->type == SOCKET_TYPE_STREAM &&
            (matched & MATCHED_CONNATTEMPT)) {
        event = PCRUNLOOP_IO_IN;
    }
    else if (socket->type == SOCKET_TYPE_DGRAM &&
            (matched & MATCHED_NEWDATAGRAM)) {
        event = PCRUNLOOP_IO_IN;
    }
    else {
        return true;    /* false? */
    }

    if ((event & PCRUNLOOP_IO_IN) && socket->fd >= 0) {
        socket->monitor = purc_runloop_add_fd_monitor(
                purc_runloop_get_current(), socket->fd, PCRUNLOOP_IO_IN,
                socket_io_callback, socket);
        if (socket->monitor == 0) {
            PC_ERROR("Failed purc_runloop_add_fd_monitor(SOCKET, IN)\n");
            return false;
        }
    }

    return true;
}

static bool
on_forget(void *native_entity, const char *event_name,
        const char *event_subname)
{
    int matched = pcdvobjs_match_events(event_name, event_subname,
            socket_events, PCA_TABLESIZE(socket_events));
    if (matched == -1)
        return false;

    struct pcdvobjs_socket *socket = (struct pcdvobjs_socket*)native_entity;
    if (socket->monitor) {
        purc_runloop_remove_fd_monitor(purc_runloop_get_current(),
                socket->monitor);
        socket->monitor = 0;
    }

    socket->cid = 0;
    return true;
}

static void
on_release(void *native_entity)
{
    dvobjs_socket_delete((struct pcdvobjs_socket *)native_entity);
}

static struct pcdvobjs_socket *
create_local_stream_socket(struct purc_broken_down_url *url,
        purc_variant_t option, int backlog)
{
    int    fd = -1, len;
    struct sockaddr_un unix_addr;

    if (purc_check_unix_socket(url->path) == 0) {
        purc_set_error(PURC_ERROR_CONFLICT);
        goto error;
    }

    /* in case it already exists */
    unlink(url->path);

    int64_t flags = parse_socket_stream_option(option);
    if (flags == -1) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    /* create a Unix domain stream socket */
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        PC_DEBUG("Failed socket(): %s\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    if ((flags & O_NONBLOCK) && fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        PC_DEBUG("Failed fcntl(O_NONBLOCK): %s\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    if ((flags & O_CLOEXEC) && fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        PC_DEBUG("Failed fcntl(FD_CLOEXEC): %s\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    /* fill in socket address structure */
    memset(&unix_addr, 0, sizeof(unix_addr));
    unix_addr.sun_family = AF_UNIX;
    if (strlen(url->path) + 1 > sizeof(unix_addr.sun_path)) {
        purc_set_error(PURC_ERROR_TOO_LONG);
        goto error;
    }
    strcpy(unix_addr.sun_path, url->path);
    len = sizeof(unix_addr.sun_family) + strlen(unix_addr.sun_path) + 1;

    /* bind the name to the descriptor */
    if (bind(fd, (struct sockaddr *)&unix_addr, len) < 0) {
        PC_DEBUG("Failed bind(%s): %s\n", url->path, strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    if (flags & _O_GLOBAL) {
        if (chmod(url->path, 0666) < 0) {
            PC_DEBUG("Failed chmod(0666): %s\n", strerror(errno));
            purc_set_error(purc_error_from_errno(errno));
            goto error;
        }
    }

    /* tell kernel we're a server */
    if (listen(fd, backlog) < 0) {
        PC_DEBUG("Failed listen(): %s\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    struct pcdvobjs_socket* socket =
        dvobjs_socket_new(SOCKET_TYPE_STREAM, url);
    if (!socket) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto error;
    }

    socket->fd = fd;
    return socket;

error:
    if (fd >= 0)
        close(fd);
    return NULL;
}

static struct pcdvobjs_socket *
create_inet_stream_socket(enum stream_inet_socket_family isf,
        struct purc_broken_down_url *url, purc_variant_t option, int backlog)
{
    int fd = -1;
    struct addrinfo *ai = NULL;
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

    char port[10] = {0};
    if (url->port > 65535) {    /* 0 is acceptable for a stream socket */
        PC_DEBUG("Bad port value: (%d)\n", url->port);
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }
    sprintf(port, "%d", url->port);

    /* get a socket and bind it */
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(url->host, port, &hints, &ai) != 0) {
        PC_DEBUG("Error while getting address info (%s:%d)\n",
                url->host, url->port);
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if ((fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1) {
        PC_DEBUG("Failed socket(%s:%d)\n", url->host, url->port);
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    int64_t flags = parse_socket_stream_option(option);
    if (flags == -1) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if ((flags & O_NONBLOCK) && fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        PC_DEBUG("Failed fcntl(O_NONBLOCK): %s\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    if ((flags & O_CLOEXEC) && fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        PC_DEBUG("Failed fcntl(FD_CLOEXEC): %s\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    /* Options */
    int ov = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &ov, sizeof(ov)) == -1) {
        PC_DEBUG("Failed setsockopt(): %s.\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    /* Bind the socket to the address. */
    if (bind(fd, ai->ai_addr, ai->ai_addrlen) != 0) {
        PC_DEBUG("Failed bind(): %s.\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    freeaddrinfo(ai);

    /* Tell the socket to accept connections. */
    if (listen(fd, backlog) == -1) {
        PC_DEBUG("Failed listen(): %s.\n", strerror (errno));
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    struct pcdvobjs_socket* socket =
        dvobjs_socket_new(SOCKET_TYPE_STREAM, url);
    if (!socket) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    socket->fd = fd;
    return socket;

failed:
    if (ai)
        freeaddrinfo(ai);
    if (fd >= 0)
        close(fd);

    return NULL;
}

static purc_variant_t
socket_stream_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto error;
    }

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    purc_variant_t option = nr_args > 1 ? argv[1] : PURC_VARIANT_INVALID;
    if (option != PURC_VARIANT_INVALID &&
            (!purc_variant_is_string(option))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    int64_t tmp_l = DEF_BACKLOG;
    purc_variant_t tmp_v = nr_args > 2 ? argv[2] : PURC_VARIANT_INVALID;
    if (tmp_v != PURC_VARIANT_INVALID &&
            !purc_variant_cast_to_longint(tmp_v, &tmp_l, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    int backlog = (int)tmp_l;

    struct purc_broken_down_url *url = (struct purc_broken_down_url*)
        calloc(1, sizeof(struct purc_broken_down_url));
    if (!pcutils_url_break_down(url, purc_variant_get_string_const(argv[0]))) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error_free_url;
    }

    purc_atom_t schema =
        purc_atom_try_string_ex(SOCKET_ATOM_BUCKET, url->schema);
    if (schema == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error_free_url;
    }

    static const struct purc_native_ops ops = {
        .property_getter = property_getter,
        .on_observe = on_observe,
        .on_forget = on_forget,
        .on_release = on_release,
    };

    struct pcdvobjs_socket *socket = NULL;
    if (schema == keywords2atoms[K_KW_unix].atom ||
            schema == keywords2atoms[K_KW_local].atom) {
        socket = create_local_stream_socket(url, option, backlog);
    }
    else if (schema == keywords2atoms[K_KW_inet].atom) {
        socket = create_inet_stream_socket(ISF_UNSPEC, url, option, backlog);
    }
    else if (schema == keywords2atoms[K_KW_inet4].atom) {
        socket = create_inet_stream_socket(ISF_INET4, url, option, backlog);
    }
    else if (schema == keywords2atoms[K_KW_inet6].atom) {
        socket = create_inet_stream_socket(ISF_INET6, url, option, backlog);
    }
    else {
        purc_set_error(PURC_ERROR_UNKNOWN);
        goto error_free_url;
    }

    if (!socket) {
        goto error_free_url;
    }

    const char *entity_name  = NATIVE_ENTITY_NAME_SOCKET ":stream";
    ret_var = purc_variant_make_native_entity(socket, &ops, entity_name);
    if (ret_var) {
        socket->url = url;
        socket->observed = ret_var;
    }
    else {
        dvobjs_socket_delete(socket);
    }

    return ret_var;

error_free_url:
    pcutils_broken_down_url_delete(url);

error:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static struct pcdvobjs_socket *
create_local_dgram_socket(struct purc_broken_down_url *url,
        int64_t flags)
{
    int    fd = -1, len;
    struct sockaddr_un unix_addr;

    /* create a Unix domain dgram socket */
    if ((fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        PC_DEBUG("Failed socket(): %s\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    if ((flags & O_CLOEXEC) && fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    if ((flags & O_NONBLOCK) && fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    if (!(flags & _O_NAMELESS)) {
        if (strlen(url->path) + 1 > sizeof(unix_addr.sun_path)) {
            purc_set_error(PURC_ERROR_TOO_LONG);
            goto error;
        }

        /* in case it already exists */
        unlink(url->path);

        /* fill in socket address structure */
        memset(&unix_addr, 0, sizeof(unix_addr));
        unix_addr.sun_family = AF_UNIX;
        strcpy(unix_addr.sun_path, url->path);
        len = sizeof(unix_addr.sun_family) + strlen(unix_addr.sun_path) + 1;

        /* bind the name to the descriptor */
        if (bind(fd, (struct sockaddr *)&unix_addr, len) < 0) {
            PC_DEBUG("Failed bind(%s): %s\n", url->path, strerror(errno));
            purc_set_error(purc_error_from_errno(errno));
            goto error;
        }

        if (flags & _O_GLOBAL) {
            if (chmod(url->path, 0666) < 0) {
                PC_DEBUG("Failed chmod(): %s\n", strerror(errno));
                purc_set_error(purc_error_from_errno(errno));
                goto error;
            }
        }
    }

    struct pcdvobjs_socket* socket =
        dvobjs_socket_new(SOCKET_TYPE_DGRAM, url);
    if (!socket) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto error;
    }

    socket->fd = fd;
    return socket;

error:
    if (fd >= 0)
        close(fd);
    return NULL;
}

static struct pcdvobjs_socket *
create_inet_dgram_socket(enum stream_inet_socket_family isf,
        struct purc_broken_down_url *url, int64_t flags)
{
    int fd = -1;
    struct addrinfo *ai = NULL;
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

    char port[10] = {0};
    if (url->port > 65535) {    /* 0 is acceptable for dgram socket. */
        PC_DEBUG("Bad port value: (%d)\n", url->port);
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }
    sprintf(port, "%d", url->port);

    /* get a socket and bind it */
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(url->host, port, &hints, &ai) != 0) {
        PC_DEBUG("Error while getting address info (%s:%d)\n",
                url->host, url->port);
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if ((fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1) {
        PC_DEBUG("Failed to create socket for %s:%d\n", url->host, url->port);
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    if ((flags & O_CLOEXEC) && fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    if ((flags & O_NONBLOCK) && fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    /* FIXME: different manners amony OSes.
    int ov = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &ov, sizeof(ov)) == -1) {
        PC_DEBUG("Failed setsockopt(): %s.\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    } */

    if (!(flags & _O_NAMELESS)) {
        /* Bind the socket to the address. */
        if (bind(fd, ai->ai_addr, ai->ai_addrlen) != 0) {
            PC_DEBUG("Failed bind(): %s.\n", strerror(errno));
            purc_set_error(purc_error_from_errno(errno));
            goto failed;
        }
    }

    freeaddrinfo(ai);

    struct pcdvobjs_socket* socket =
        dvobjs_socket_new(SOCKET_TYPE_DGRAM, url);
    if (!socket) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    socket->fd = fd;
    return socket;

failed:
    if (ai)
        freeaddrinfo(ai);
    if (fd >= 0)
        close(fd);

    return NULL;
}

static purc_variant_t
socket_dgram_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto error;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    purc_variant_t option = nr_args > 1 ? argv[1] : PURC_VARIANT_INVALID;
    if (option != PURC_VARIANT_INVALID &&
            (!purc_variant_is_string(option))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    struct purc_broken_down_url *url = (struct purc_broken_down_url*)
        calloc(1, sizeof(struct purc_broken_down_url));
    if (!pcutils_url_break_down(url, purc_variant_get_string_const(argv[0]))) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error_free_url;
    }

    purc_atom_t schema =
        purc_atom_try_string_ex(SOCKET_ATOM_BUCKET, url->schema);
    if (schema == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error_free_url;
    }

    int64_t flags = parse_socket_dgram_option(option);
    if (flags == -1L) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error_free_url;
    }

    struct pcdvobjs_socket *socket = NULL;
    if (schema == keywords2atoms[K_KW_unix].atom ||
            schema == keywords2atoms[K_KW_local].atom) {
        socket = create_local_dgram_socket(url, flags);
    }
    else if (schema == keywords2atoms[K_KW_inet].atom) {
        socket = create_inet_dgram_socket(ISF_UNSPEC, url, flags);
    }
    else if (schema == keywords2atoms[K_KW_inet4].atom) {
        socket = create_inet_dgram_socket(ISF_INET4, url, flags);
    }
    else if (schema == keywords2atoms[K_KW_inet6].atom) {
        socket = create_inet_dgram_socket(ISF_INET6, url, flags);
    }
    else {
        purc_set_error(PURC_ERROR_UNKNOWN);
        goto error_free_url;
    }

    if (!socket) {
        goto error_free_url;
    }

    static const struct purc_native_ops ops = {
        .property_getter = property_getter,
        .on_observe = on_observe,
        .on_forget = on_forget,
        .on_release = on_release,
    };

    const char *entity_name  = NATIVE_ENTITY_NAME_SOCKET ":dgram";
    ret_var = purc_variant_make_native_entity(socket, &ops, entity_name);
    if (ret_var) {
        socket->url = url;
        socket->observed = ret_var;
    }
    else {
        dvobjs_socket_delete(socket);
    }

    return ret_var;

    return purc_variant_make_boolean(false);

error_free_url:
    pcutils_broken_down_url_delete(url);

error:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
socket_close_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
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

    const char *entity_name = purc_variant_native_get_name(argv[0]);
    if (entity_name == NULL ||
            strncmp(entity_name, NATIVE_ENTITY_NAME_SOCKET,
                sizeof(NATIVE_ENTITY_NAME_SOCKET) - 1) != 0) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto out;
    }

    struct pcdvobjs_socket *socket = purc_variant_native_get_entity(argv[0]);
    dvobjs_socket_close(socket);
    return purc_variant_make_boolean(true);

out:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);

    return PURC_VARIANT_INVALID;
}

purc_variant_t purc_dvobj_socket_new(void)
{
    static struct purc_dvobj_method  socket[] = {
        { "stream", socket_stream_getter,   NULL },
        { "dgram",  socket_dgram_getter,    NULL },
        { "close",  socket_close_getter,    NULL },
    };

    if (keywords2atoms[0].atom == 0) {
        for (size_t i = 0; i < PCA_TABLESIZE(keywords2atoms); i++) {
            keywords2atoms[i].atom =
                purc_atom_from_static_string_ex(SOCKET_ATOM_BUCKET,
                    keywords2atoms[i].keyword);
        }
    }

    purc_variant_t v = purc_dvobj_make_from_methods(socket,
            PCA_TABLESIZE(socket));
    if (v == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    return v;
}


