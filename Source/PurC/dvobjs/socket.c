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

#define SOCKET_EVENT_NAME               "event"
#define SOCKET_SUB_EVENT_CONNATTEMPT    "connAttempt"
#define SOCKET_SUB_EVENT_NEWMESSAGE     "newMessage"

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
    { _KW_accept, 0},               // "accept"
    { _KW_sendto, 0},               // "sendto"
    { _KW_recvfrom, 0},             // "recvfrom"
    { _KW_close, 0},                // "close"
    { _KW_default, 0},              // "default"
    { _KW_nonblock, 0},             // "nonblock"
    { _KW_cloexec, 0},              // "cloexec"
    { _KW_global, 0},               // "global"
};

/* We use the high 32-bit for customized flags */
#define _O_GLOBAL       (0x01L << 32)

static
int64_t parse_socket_option(purc_variant_t option)
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
            goto out;
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

out:
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
    UNUSED_PARAM(socket);
    UNUSED_PARAM(peer_addr);

    return -1;
}

static int
inet_socket_accept_client(struct pcdvobjs_socket *socket,
        enum stream_inet_socket_family isf, char **peer_addr)
{
    UNUSED_PARAM(socket);
    UNUSED_PARAM(isf);
    UNUSED_PARAM(peer_addr);

    return -1;
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
                atom = keywords2atoms[K_KW_default].atom;
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

    int fd = -1;
    char *peer_addr = NULL;

    if (schema == keywords2atoms[K_KW_unix].atom ||
            schema == keywords2atoms[K_KW_local].atom) {
        fd = local_socket_accept_client(socket, &peer_addr);
    }
    else if (schema == keywords2atoms[K_KW_inet].atom) {
        fd = inet_socket_accept_client(socket, ISF_UNSPEC, &peer_addr);
    }
    else if (schema == keywords2atoms[K_KW_inet4].atom) {
        fd = inet_socket_accept_client(socket, ISF_INET4, &peer_addr);
    }
    else if (schema == keywords2atoms[K_KW_inet6].atom) {
        fd = inet_socket_accept_client(socket, ISF_INET6, &peer_addr);
    }
    else {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    }

    if (fd >= 0) {
        purc_variant_t stream =
            dvobjs_create_stream_by_accepted(schema, peer_addr, fd,
                    nr_args > 0 ? argv[0] : NULL,
                    nr_args > 1 ? argv[1] : NULL);
        if (stream)
            return stream;
    }

error:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
sendto_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    assert(native_entity);

    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return purc_variant_make_boolean(false);
}

static purc_variant_t
recvfrom_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);

    assert(native_entity);

    purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return purc_variant_make_boolean(false);
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

failed:
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

struct io_callback_data {
    int                           fd;
    purc_runloop_io_event         io_event;
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
        sub = SOCKET_SUB_EVENT_NEWMESSAGE;
    }

    if (sub && socket->cid) {
        pcintr_coroutine_post_event(socket->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_IGNORE,
                socket->observed, SOCKET_EVENT_NAME, sub,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    }
}

static bool
socket_io_callback(int fd, purc_runloop_io_event event, void *ctxt)
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

static bool
on_observe(void *native_entity, const char *event_name,
        const char *event_subname)
{
    if (strcmp(event_name, SOCKET_EVENT_NAME) != 0) {
        return false;
    }

    struct pcdvobjs_socket *socket = (struct pcdvobjs_socket*)native_entity;

    purc_runloop_io_event event = 0;
    if (socket->type == SOCKET_TYPE_STREAM &&
            strcmp(event_subname, SOCKET_SUB_EVENT_CONNATTEMPT) == 0) {
        event = PCRUNLOOP_IO_IN;
    }
    else if (socket->type == SOCKET_TYPE_DGRAM &&
            strcmp(event_subname, SOCKET_SUB_EVENT_NEWMESSAGE) == 0) {
        event = PCRUNLOOP_IO_IN;
    }

    if ((event & PCRUNLOOP_IO_IN) && socket->fd >= 0) {
        socket->monitor = purc_runloop_add_fd_monitor(
                purc_runloop_get_current(), socket->fd, PCRUNLOOP_IO_IN,
                socket_io_callback, socket);
        if (socket->monitor) {
            pcintr_coroutine_t co = pcintr_get_coroutine();
            if (co) {
                socket->cid = co->cid;
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

    int64_t flags = parse_socket_option(option);

    /* create a Unix domain stream socket */
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        PC_ERROR("Failed to call `socket`: %s\n", strerror(errno));
        purc_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
        goto error;
    }

    if (flags & O_CLOEXEC)
        fcntl(fd, F_SETFD, FD_CLOEXEC);

    /* fill in socket address structure */
    memset(&unix_addr, 0, sizeof(unix_addr));
    unix_addr.sun_family = AF_UNIX;
    strcpy(unix_addr.sun_path, url->path);
    len = sizeof(unix_addr.sun_family) + strlen(unix_addr.sun_path) + 1;

    /* bind the name to the descriptor */
    if (bind(fd, (struct sockaddr *)&unix_addr, len) < 0) {
        PC_ERROR("Failed to call `bind`: %s\n", strerror(errno));
        purc_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
        goto error;
    }

    if (flags & _O_GLOBAL) {
        if (chmod(url->path, 0666) < 0) {
            PC_ERROR("Failed to call `chmod`: %s\n", strerror(errno));
            purc_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
            goto error;
        }
    }

    /* tell kernel we're a server */
    if (listen(fd, backlog) < 0) {
        PC_ERROR("Failed to call `listen`: %s\n", strerror(errno));
        purc_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
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
    if (url->port <= 0 || url->port > 65535) {
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
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    if ((fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1) {
        PC_DEBUG("Failed to create socket for %s:%d\n", url->host, url->port);
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    int64_t flags = parse_socket_option(option);
    if (flags & O_CLOEXEC)
        fcntl(fd, F_SETFD, FD_CLOEXEC);

    /* Options */
    int ov = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &ov, sizeof(ov)) == -1) {
        PC_DEBUG("Unable to set setsockopt: %s.", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    /* Bind the socket to the address. */
    if (bind(fd, ai->ai_addr, ai->ai_addrlen) != 0) {
        PC_DEBUG("Unable to set bind: %s.", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    freeaddrinfo(ai);

    /* Tell the socket to accept connections. */
    if (listen(fd, backlog) == -1) {
        PC_DEBUG("Unable to listen: %s.", strerror (errno));
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
    UNUSED_PARAM(call_flags);

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
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    int backlog = (int)tmp_l;

    struct purc_broken_down_url *url = (struct purc_broken_down_url*)
        calloc(1, sizeof(struct purc_broken_down_url));
    if (!pcutils_url_break_down(url, purc_variant_get_string_const(argv[0]))) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    purc_atom_t atom = purc_atom_try_string_ex(SOCKET_ATOM_BUCKET, url->schema);
    if (atom == 0) {
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
    if (atom == keywords2atoms[K_KW_unix].atom ||
            atom == keywords2atoms[K_KW_local].atom) {
        socket = create_local_stream_socket(url, option, backlog);
    }
    else if (atom == keywords2atoms[K_KW_inet].atom) {
        socket = create_inet_stream_socket(ISF_UNSPEC, url, option, backlog);
    }
    else if (atom == keywords2atoms[K_KW_inet4].atom) {
        socket = create_inet_stream_socket(ISF_INET4, url, option, backlog);
    }
    else if (atom == keywords2atoms[K_KW_inet6].atom) {
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
        socket->observed = ret_var;
    }

    return ret_var;

error_free_url:
    pcutils_broken_down_url_delete(url);

error:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
socket_dgram_getter(purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto error;
    }

    if ((!purc_variant_is_native(argv[0]))) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    return purc_variant_make_boolean(false);

error:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_undefined();

    return PURC_VARIANT_INVALID;
}

purc_variant_t purc_dvobj_socket_new(void)
{
    static struct purc_dvobj_method  socket[] = {
        { "stream", socket_stream_getter,   NULL },
        { "dgram",  socket_dgram_getter,    NULL },
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


