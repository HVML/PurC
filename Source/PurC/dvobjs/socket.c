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
#undef NDEBUG
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

#if HAVE(OPENSSL)
#include <openssl/err.h>
#endif

#define SOCKET_EVENT_NAME               "socket"
#define SOCKET_SUB_EVENT_CONNATTEMPT    "connAttempt"
#define SOCKET_SUB_EVENT_NEWDATAGRAM    "newDatagram"

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
#define _KW_none                    "none"
    K_KW_none,
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
    { _KW_none, 0},                 // "none"
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
            flags = -1;
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

    if (atom == keywords2atoms[K_KW_none].atom) {
        flags = 0;
    }
    else if (atom == keywords2atoms[K_KW_default].atom) {
        flags = O_CLOEXEC | O_NONBLOCK;
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
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }

            if (parts_len <= length)
                break;

            parts_len -= length;
            part = pcutils_get_next_token_len(part + length, parts_len,
                    PURC_KW_DELIMITERS, &length);
        } while (part);
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
            flags = -1;
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

    if (atom == keywords2atoms[K_KW_none].atom) {
        flags = 0;
    }
    else if (atom == keywords2atoms[K_KW_default].atom) {
        flags = O_CLOEXEC | O_NONBLOCK;
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
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }

            if (parts_len <= length)
                break;

            parts_len -= length;
            part = pcutils_get_next_token_len(part + length, parts_len,
                    PURC_KW_DELIMITERS, &length);
        } while (part);
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
                PURC_KW_DELIMITERS, &length);
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
                    PURC_KW_DELIMITERS, &length);
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
                PURC_KW_DELIMITERS, &length);
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
                    PURC_KW_DELIMITERS, &length);
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

#if HAVE(OPENSSL)
static struct pcdvobjs_option_to_atom access_users_ckws[] = {
    { "group",     0, 0060 },
    { "other",     0, 0006 },
};

static void pcdvobjs_socket_ssl_ctx_delete(struct pcdvobjs_socket *socket)
{
    if (socket->ssl_ctx) {
        SSL_CTX_free(socket->ssl_ctx);
        socket->ssl_ctx = NULL;

        if (socket->ssl_shctx_wrapper) {
            openssl_shctx_destroy(socket->ssl_shctx_wrapper);
            free(socket->ssl_shctx_wrapper);
            socket->ssl_shctx_wrapper = NULL;
        }
    }
}

static int
create_ssl_ctx(struct pcdvobjs_socket *socket, purc_variant_t opt_obj)
{
    const char *ssl_cert;
    const char *ssl_key;
    const char *ssl_session_cache_id;
    int cache_mode;
    uint64_t cache_size = OPENSSL_SHCTX_CACHESZ_DEF;
    int error = PURC_ERROR_OK;

    purc_variant_t tmp;

    tmp = purc_variant_object_get_by_ckey_ex(opt_obj, "sslcert", true);
    ssl_cert = (!tmp) ? NULL : purc_variant_get_string_const(tmp);

    tmp = purc_variant_object_get_by_ckey_ex(opt_obj, "sslkey", true);
    ssl_key = (!tmp) ? NULL : purc_variant_get_string_const(tmp);

    if (ssl_cert == NULL || ssl_key == NULL) {
        PC_WARN("Missing SSL certification or key; skip SSL.\n");
        goto skip;
    }

    tmp = purc_variant_object_get_by_ckey_ex(opt_obj, "sslsessioncacheid", true);
    ssl_session_cache_id = (!tmp) ? NULL : purc_variant_get_string_const(tmp);
    if (ssl_session_cache_id) {
        if (strlen(ssl_session_cache_id) > OPENSSL_SHCTX_ID_LEN) {
            error = PURC_ERROR_INVALID_VALUE;
            goto opt_failed;
        }

        tmp = purc_variant_object_get_by_ckey_ex(opt_obj,
                "sslsessioncacheusers", true);
        cache_mode = pcdvobjs_parse_options(tmp, NULL, 0,
            access_users_ckws, PCA_TABLESIZE(access_users_ckws), 0, -1);
        if (cache_mode == -1) {
            error = PURC_ERROR_INVALID_VALUE;
            goto opt_failed;
        }
        cache_mode |= 0600;

        tmp = purc_variant_object_get_by_ckey_ex(opt_obj,
                "sslsessioncachesize", true);
        if ((tmp && !purc_variant_cast_to_ulongint(tmp,
                    &cache_size, false)) ||
                cache_size < OPENSSL_SHCTX_CACHESZ_MIN) {
            error = PURC_ERROR_INVALID_VALUE;
            goto opt_failed;
        }
    }

    SSL_CTX *ctx = NULL;

    /* ssl context */
    if (!(ctx = SSL_CTX_new(TLS_server_method()))) {
        PC_ERROR("Failed SSL_CTX_new(): %s\n",
                ERR_error_string(ERR_get_error(), NULL));
        error = PURC_ERROR_TLS_FAILURE;
        goto ssl_failed;
    }

    /* set certificate */
    if (!SSL_CTX_use_certificate_file(ctx, ssl_cert, SSL_FILETYPE_PEM)) {
        PC_ERROR("Failed SSL_CTX_use_certificate_file(%s): %s\n",
                ssl_cert, ERR_error_string(ERR_get_error(), NULL));
        error = PURC_ERROR_TLS_FAILURE;
        goto ssl_failed;
    }

    /* ssl private key */
    if (!SSL_CTX_use_PrivateKey_file(ctx, ssl_key, SSL_FILETYPE_PEM)) {
        PC_ERROR("Failed SSL_CTX_use_PrivateKey_file(%s): %s\n",
                ssl_key, ERR_error_string(ERR_get_error(), NULL));
        error = PURC_ERROR_TLS_FAILURE;
        goto ssl_failed;
    }

    if (!SSL_CTX_check_private_key(ctx)) {
        PC_ERROR("Failed SSL_CTX_check_private_key(): %s\n",
                ERR_error_string(ERR_get_error(), NULL));
        error = PURC_ERROR_TLS_FAILURE;
        goto ssl_failed;
    }

    /* since we queued up the send data, a retry won't be the same buffer,
     * thus we need the following flags */
    SSL_CTX_set_mode(ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER |
            SSL_MODE_ENABLE_PARTIAL_WRITE);

    if (ssl_session_cache_id) {
        SSL_CTX_set_options(ctx,
                SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);

        socket->ssl_shctx_wrapper = calloc(1, sizeof(*socket->ssl_shctx_wrapper));

        switch (openssl_shctx_create(socket->ssl_shctx_wrapper,
                ssl_session_cache_id, (mode_t)cache_mode, ctx, cache_size)) {
            case HELPER_RETV_BAD_SYSCALL:
                error = purc_error_from_errno(errno);
                break;
            case HELPER_RETV_BAD_LIBCALL:
                error = PURC_ERROR_TLS_FAILURE;
                break;
            case HELPER_RETV_BAD_ARGS:
                error = PURC_ERROR_INVALID_VALUE;
                break;
        }

        SSL_CTX_set_session_id_context(ctx,
                (const unsigned char *)ssl_session_cache_id,
                strlen(ssl_session_cache_id));
        if (error) {
            PC_ERROR("Failed openssl_shctx_create(): %s\n",
                    purc_get_error_message(error));
            goto ssl_failed;
        }
    }

    socket->ssl_ctx = ctx;

skip:
    purc_clr_error();
    return 0;

ssl_failed:
    if (ctx) {
        SSL_CTX_free(ctx);
    }

    if (socket->ssl_shctx_wrapper) {
        free(socket->ssl_shctx_wrapper);
        socket->ssl_shctx_wrapper = NULL;
    }

opt_failed:
    purc_set_error(error);
    return -1;
}

#endif  // HAVE(OPENSSL)

static void dvobjs_socket_close(struct pcdvobjs_socket *socket)
{
    if (socket->monitor) {
        purc_runloop_remove_fd_monitor(purc_runloop_get_current(),
                socket->monitor);
        socket->monitor = 0;
    }

#if HAVE(OPENSSL)
    pcdvobjs_socket_ssl_ctx_delete(socket);
#endif

    if (socket->fd >= 0) {
        close(socket->fd);
        socket->fd = -1;
    }
}

void pcdvobjs_socket_delete(struct pcdvobjs_socket *socket)
{
    assert(socket->refc == 0);

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
    struct sockaddr_un addr = { 0 };
    socklen_t          len = sizeof(addr);

    len = sizeof(addr);
    if ((fd = accept(socket->fd, (struct sockaddr *)&addr, &len)) < 0) {
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    /* obtain the peer address */
    if (strlen(addr.sun_path) == 0) {
        char buf[PURC_LEN_UNIQUE_ID + 1];
        purc_generate_unique_id(buf, "unknown");
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
        PC_ERROR("Failed accept(): %s\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
    if (0 != getnameinfo((struct sockaddr *)&addr, len, hbuf, sizeof(hbuf),
                sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV)) {
        PC_ERROR("Failed getnameinfo(): %s\n", strerror(errno));
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
        flags = -1;
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto done;
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

    if (atom == keywords2atoms[K_KW_none].atom) {
        flags = 0;
    }
    else if (atom == keywords2atoms[K_KW_default].atom) {
        flags = O_CLOEXEC | O_NONBLOCK;
    }
    else {
        size_t length = 0;
        const char *part = pcutils_get_next_token_len(parts, parts_len,
                PURC_KW_DELIMITERS, &length);
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
            else {
                flags = -1;
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                break;
            }

            if (parts_len <= length)
                break;

            parts_len -= length;
            part = pcutils_get_next_token_len(part + length, parts_len,
                    PURC_KW_DELIMITERS, &length);
        } while (part);
    }

done:
    return flags;
}

static purc_variant_t
accept_getter(void *native_entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);

    PC_ASSERT(native_entity);
    struct pcdvobjs_socket *socket = cast_to_socket(native_entity);
    PC_ASSERT(socket->type <= SOCKET_TYPE_STREAM_MAX);

    int fd = -1;
    if (nr_args == 0) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto error;
    }

    int flags = parse_accept_option(argv[0]);
    if (flags == -1) {
        goto error;
    }

    purc_atom_t scheme = purc_atom_try_string_ex(SOCKET_ATOM_BUCKET,
            socket->url->scheme);
    if (scheme == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    char *peer_addr = NULL;
    char *peer_port = NULL;

    if (scheme == keywords2atoms[K_KW_unix].atom ||
            scheme == keywords2atoms[K_KW_local].atom) {
        fd = local_socket_accept_client(socket, &peer_addr);
    }
    else if (scheme == keywords2atoms[K_KW_inet].atom) {
        fd = inet_socket_accept_client(socket, ISF_UNSPEC,
                &peer_addr, &peer_port);
    }
    else if (scheme == keywords2atoms[K_KW_inet4].atom) {
        fd = inet_socket_accept_client(socket, ISF_INET4,
                &peer_addr, &peer_port);
    }
    else if (scheme == keywords2atoms[K_KW_inet6].atom) {
        fd = inet_socket_accept_client(socket, ISF_INET6,
                &peer_addr, &peer_port);
    }
    else {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    }

    if (fd < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
        return purc_variant_make_null();
    }
    else if (fd < 0 && (errno == ETIMEDOUT || errno == EINPROGRESS)) {
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

#if 0
    int ov = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &ov, sizeof(ov)) == -1) {
        PC_ERROR("Failed setsockopt(SO_KEEPALIVE): %s.\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &ov, sizeof(ov)) == -1) {
        PC_ERROR("Failed setsockopt(SO_NOSIGPIPE): %s.\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }
#endif

    purc_variant_t stream =
        dvobjs_create_stream_by_accepted(socket,
                scheme, peer_addr, peer_port, fd,
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

    if (url->port == 0 || url->port > 65535) {
        PC_ERROR("Bad port value: (%d)\n", url->port);
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }
    char port[8] = {0};
    snprintf(port, sizeof(port), "%d", url->port);

    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(url->hostname, port, &hints, &ai) != 0) {
        PC_ERROR("Error while getting address info (%s:%d)\n",
                url->hostname, url->port);
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

    PC_ASSERT(native_entity);

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

    purc_atom_t scheme =
        purc_atom_try_string_ex(SOCKET_ATOM_BUCKET, dst->scheme);
    if (scheme == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error_free_url;
    }

    struct addrinfo *ai = NULL;
    struct sockaddr_un *unix_addr = NULL;
    if (scheme == keywords2atoms[K_KW_unix].atom ||
            scheme == keywords2atoms[K_KW_local].atom) {

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
    else if (scheme == keywords2atoms[K_KW_inet].atom) {
        ai = get_network_address(ISF_UNSPEC, dst);
    }
    else if (scheme == keywords2atoms[K_KW_inet4].atom) {
        ai = get_network_address(ISF_INET4, dst);
    }
    else if (scheme == keywords2atoms[K_KW_inet6].atom) {
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

    PC_ASSERT(native_entity);

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto error;
    }

    if (!purc_variant_is_string(argv[0])) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    int64_t flags = parse_dgram_recvfrom_option(argv[0]);
    if (flags == -1) {
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
            purc_variant_object_set_by_static_ckey(retv, "sourceaddr", tmp);
            purc_variant_unref(tmp);

            tmp = purc_variant_make_null();
            purc_variant_object_set_by_static_ckey(retv, "sourceport", tmp);
            purc_variant_unref(tmp);
        }
        else if (!(flags & _O_NOSOURCE)) {
            char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
            if (0 != getnameinfo((struct sockaddr *)&src_addr, addrlen,
                        hbuf, sizeof(hbuf), sbuf, sizeof(sbuf),
                        NI_NUMERICHOST | NI_NUMERICSERV)) {
                PC_ERROR("Failed getnameinfo(): %s\n", strerror(errno));
                flags |= _O_NOSOURCE;
            }
            else {
                tmp = purc_variant_make_string(hbuf, false);
                purc_variant_object_set_by_static_ckey(retv,
                        "sourceaddr", tmp);
                purc_variant_unref(tmp);

                tmp = purc_variant_make_longint(atol(sbuf));
                purc_variant_object_set_by_static_ckey(retv,
                        "sourceport", tmp);
                purc_variant_unref(tmp);
            }
        }

        if (flags & _O_NOSOURCE) {
            tmp = purc_variant_make_null();
            purc_variant_object_set_by_static_ckey(retv, "sourceaddr", tmp);
            purc_variant_unref(tmp);

            tmp = purc_variant_make_null();
            purc_variant_object_set_by_static_ckey(retv, "sourceport", tmp);
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

    PC_ASSERT(native_entity);

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

    PC_ASSERT(native_entity);

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
    if (socket->type <= SOCKET_TYPE_STREAM_MAX) {
        if (atom == keywords2atoms[K_KW_accept].atom) {
            return accept_getter;
        }
    }
    else if (socket->type >= SOCKET_TYPE_DGRAM_MIN) {
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

static bool
socket_io_callback(int fd, int event, void *ctxt)
{
    UNUSED_PARAM(fd);

    struct pcdvobjs_socket *socket = (struct pcdvobjs_socket*)ctxt;
    PC_ASSERT(socket);

    if ((event & PCRUNLOOP_IO_HUP) || (event & PCRUNLOOP_IO_ERR) ||
            (event & PCRUNLOOP_IO_NVAL)) {
        socket->monitor = 0;
        PC_ERROR("Got a weird IO event for socket.\n");
        return false;
    }

    if (event & PCRUNLOOP_IO_IN) {
        const char* sub = NULL;
        if (socket->type <= SOCKET_TYPE_STREAM_MAX) {
            sub = SOCKET_SUB_EVENT_CONNATTEMPT;
        }
        else if (socket->type >= SOCKET_TYPE_DGRAM_MIN) {
            sub = SOCKET_SUB_EVENT_NEWDATAGRAM;
        }

        if (sub && socket->cid) {
            pcintr_coroutine_post_event(socket->cid,
                    PCRDR_MSG_EVENT_REDUCE_OPT_IGNORE,
                    socket->observed, SOCKET_EVENT_NAME, sub,
                    PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        }
    }

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

    if (socket->cid == 0) {
        pcintr_coroutine_t co = pcintr_get_coroutine();
        if (co)
            socket->cid = co->cid;
        else
            return false;
    }

    int matched = pcdvobjs_match_events(event_name, event_subname,
            socket_events, PCA_TABLESIZE(socket_events));
    if (matched == -1)
        return false;

    uint32_t event = 0;
    if (socket->type <= SOCKET_TYPE_STREAM_MAX &&
            (matched & MATCHED_CONNATTEMPT)) {
        event = PCRUNLOOP_IO_IN;
    }
    else if (socket->type >= SOCKET_TYPE_DGRAM_MIN &&
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
    pcdvobjs_socket_release((struct pcdvobjs_socket *)native_entity);
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
        goto error;
    }

    /* create a Unix domain stream socket */
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        PC_ERROR("Failed socket(): %s\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    if ((flags & O_NONBLOCK) && fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        PC_ERROR("Failed fcntl(O_NONBLOCK): %s\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    if ((flags & O_CLOEXEC) && fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        PC_ERROR("Failed fcntl(FD_CLOEXEC): %s\n", strerror(errno));
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
        PC_ERROR("Failed bind(%s): %s\n", url->path, strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    if (flags & _O_GLOBAL) {
        if (chmod(url->path, 0666) < 0) {
            PC_ERROR("Failed chmod(0666): %s\n", strerror(errno));
            purc_set_error(purc_error_from_errno(errno));
            goto error;
        }
    }

    /* tell kernel we're a server */
    if (listen(fd, backlog) < 0) {
        PC_ERROR("Failed listen(): %s\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto error;
    }

    struct pcdvobjs_socket* socket =
        dvobjs_socket_new(SOCKET_TYPE_STREAM_LOCAL, url);
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

    if (url->port > 65535) {    /* 0 is acceptable for a stream socket */
        PC_ERROR("Bad port value: (%u)\n", url->port);
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }
    char port[8] = {0};
    snprintf(port, sizeof(port), "%u", url->port);

    /* get a socket and bind it */
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(url->hostname, port, &hints, &ai) != 0) {
        PC_ERROR("Error while getting address info (%s:%d)\n",
                url->hostname, url->port);
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if ((fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1) {
        PC_ERROR("Failed socket(%s:%d)\n", url->hostname, url->port);
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    int64_t flags = parse_socket_stream_option(option);
    if (flags == -1) {
        goto failed;
    }

    if ((flags & O_NONBLOCK) && fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
        PC_ERROR("Failed fcntl(O_NONBLOCK): %s\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    if ((flags & O_CLOEXEC) && fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
        PC_ERROR("Failed fcntl(FD_CLOEXEC): %s\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    /* Options */
    int ov = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &ov, sizeof(ov)) == -1) {
        PC_ERROR("Failed setsockopt(): %s.\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    /* Bind the socket to the address. */
    if (bind(fd, ai->ai_addr, ai->ai_addrlen) != 0) {
        PC_ERROR("Failed bind(): %s.\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    /* Tell the socket to accept connections. */
    if (listen(fd, backlog) == -1) {
        PC_ERROR("Failed listen(): %s.\n", strerror (errno));
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    }

    struct pcdvobjs_socket* socket =
        dvobjs_socket_new(ai->ai_family == AF_INET ?
                    SOCKET_TYPE_STREAM_INET4 :
                    SOCKET_TYPE_STREAM_INET6, url);
    if (!socket) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    freeaddrinfo(ai);
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

    if (nr_args > 3 && !purc_variant_is_object(argv[3])) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto error;
    }

    struct purc_broken_down_url *url = (struct purc_broken_down_url*)
        calloc(1, sizeof(struct purc_broken_down_url));
    if (!pcutils_url_break_down(url, purc_variant_get_string_const(argv[0]))) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error_free_url;
    }

    purc_atom_t scheme =
        purc_atom_try_string_ex(SOCKET_ATOM_BUCKET, url->scheme);
    if (scheme == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error_free_url;
    }

    static struct purc_native_ops ops = {
        .property_getter = property_getter,
        .on_observe = on_observe,
        .on_forget = on_forget,
        .on_release = on_release,
    };

    struct pcdvobjs_socket *socket = NULL;
    if (scheme == keywords2atoms[K_KW_unix].atom ||
            scheme == keywords2atoms[K_KW_local].atom) {
        socket = create_local_stream_socket(url, option, backlog);
    }
    else if (scheme == keywords2atoms[K_KW_inet].atom) {
        socket = create_inet_stream_socket(ISF_UNSPEC, url, option, backlog);
    }
    else if (scheme == keywords2atoms[K_KW_inet4].atom) {
        socket = create_inet_stream_socket(ISF_INET4, url, option, backlog);
    }
    else if (scheme == keywords2atoms[K_KW_inet6].atom) {
        socket = create_inet_stream_socket(ISF_INET6, url, option, backlog);
    }
    else {
        purc_set_error(PURC_ERROR_UNKNOWN);
        goto error_free_url;
    }

    if (!socket) {
        goto error_free_url;
    }

#if HAVE(OPENSSL)
    if ((socket->type == SOCKET_TYPE_STREAM_INET4 ||
                socket->type == SOCKET_TYPE_STREAM_INET6)) {
        if (nr_args > 3) {
            // initilize SSL_CTX and shared context wrapper here.
            if (create_ssl_ctx(socket, argv[3])) {
                pcdvobjs_socket_delete(socket);
                goto error;
            }
        }
    }
#endif

    const char *entity_name  = NATIVE_ENTITY_NAME_SOCKET ":stream";
    ret_var = purc_variant_make_native_entity(socket, &ops, entity_name);
    if (ret_var) {
        socket->url = url;
        socket->observed = ret_var;
        socket->refc = 1;
    }
    else {
        pcdvobjs_socket_delete(socket);
        goto error_free_url;
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
        PC_ERROR("Failed socket(): %s\n", strerror(errno));
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
            PC_ERROR("Failed bind(%s): %s\n", url->path, strerror(errno));
            purc_set_error(purc_error_from_errno(errno));
            goto error;
        }

        if (flags & _O_GLOBAL) {
            if (chmod(url->path, 0666) < 0) {
                PC_ERROR("Failed chmod(): %s\n", strerror(errno));
                purc_set_error(purc_error_from_errno(errno));
                goto error;
            }
        }
    }

    struct pcdvobjs_socket* socket =
        dvobjs_socket_new(SOCKET_TYPE_DGRAM_LOCAL, url);
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

    if (url->port > 65535) {    /* 0 is acceptable for dgram socket. */
        PC_ERROR("Bad port value: (%d)\n", url->port);
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }
    char port[8] = {0};
    snprintf(port, sizeof(port), "%u", url->port);

    /* get a socket and bind it */
    hints.ai_socktype = SOCK_DGRAM;
    if (getaddrinfo(url->hostname, port, &hints, &ai) != 0) {
        PC_ERROR("Error while getting address info (%s:%d)\n",
                url->hostname, url->port);
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if ((fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1) {
        PC_ERROR("Failed to create socket for %s:%d\n", url->hostname, url->port);
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
        PC_ERROR("Failed setsockopt(): %s.\n", strerror(errno));
        purc_set_error(purc_error_from_errno(errno));
        goto failed;
    } */

    if (!(flags & _O_NAMELESS)) {
        /* Bind the socket to the address. */
        if (bind(fd, ai->ai_addr, ai->ai_addrlen) != 0) {
            PC_ERROR("Failed bind(): %s.\n", strerror(errno));
            purc_set_error(purc_error_from_errno(errno));
            goto failed;
        }
    }

    struct pcdvobjs_socket* socket = dvobjs_socket_new(
            ai->ai_family == AF_INET ?
            SOCKET_TYPE_DGRAM_INET4 : SOCKET_TYPE_DGRAM_INET6, url);
    if (!socket) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    freeaddrinfo(ai);
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

    int64_t flags = parse_socket_dgram_option(option);
    if (flags == -1) {
        goto error;
    }

    struct purc_broken_down_url *url = (struct purc_broken_down_url*)
        calloc(1, sizeof(struct purc_broken_down_url));
    if (!pcutils_url_break_down(url, purc_variant_get_string_const(argv[0]))) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error_free_url;
    }

    purc_atom_t scheme =
        purc_atom_try_string_ex(SOCKET_ATOM_BUCKET, url->scheme);
    if (scheme == 0) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto error_free_url;
    }

    struct pcdvobjs_socket *socket = NULL;
    if (scheme == keywords2atoms[K_KW_unix].atom ||
            scheme == keywords2atoms[K_KW_local].atom) {
        socket = create_local_dgram_socket(url, flags);
    }
    else if (scheme == keywords2atoms[K_KW_inet].atom) {
        socket = create_inet_dgram_socket(ISF_UNSPEC, url, flags);
    }
    else if (scheme == keywords2atoms[K_KW_inet4].atom) {
        socket = create_inet_dgram_socket(ISF_INET4, url, flags);
    }
    else if (scheme == keywords2atoms[K_KW_inet6].atom) {
        socket = create_inet_dgram_socket(ISF_INET6, url, flags);
    }
    else {
        purc_set_error(PURC_ERROR_UNKNOWN);
        goto error_free_url;
    }

    if (!socket) {
        goto error_free_url;
    }

    static struct purc_native_ops ops = {
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
        socket->refc = 1;
    }
    else {
        pcdvobjs_socket_delete(socket);
        goto error_free_url;
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

    static struct dvobjs_option_set {
        struct pcdvobjs_option_to_atom *opts;
        size_t sz;
    } opts_set[] = {
#if HAVE(OPENSSL)
        { access_users_ckws,    PCA_TABLESIZE(access_users_ckws) },
#endif
    };

    for (size_t i = 0; i < PCA_TABLESIZE(opts_set); i++) {
        struct pcdvobjs_option_to_atom *opts = opts_set[i].opts;
        if (opts[0].atom == 0) {
            for (size_t j = 0; j < opts_set[i].sz; j++) {
                opts[j].atom = purc_atom_from_static_string_ex(
                        SOCKET_ATOM_BUCKET, opts[j].option);
            }
        }
    }
    purc_variant_t v = purc_dvobj_make_from_methods(socket,
            PCA_TABLESIZE(socket));
    if (v == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    return v;
}


