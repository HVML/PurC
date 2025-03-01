/*
 * @file stream-websocket.c
 * @author Xue Shuming, Vincent Wei
 * @date 2023/10/12
 * @brief The implementation of `websocket` protocol for stream object.
 *
 * Copyright (C) 2023 FMSoft <https://www.fmsoft.cn>
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
#undef NDEBUG /* TODO: Remove this before merging to main branch. */
#include "config.h"
#include "stream.h"
#include "socket.h"

#include "purc-variant.h"
#include "purc-runloop.h"
#include "purc-dvobjs.h"

#include "private/debug.h"
#include "private/dvobjs.h"
#include "private/list.h"
#include "private/interpreter.h"
#include "private/timer.h"

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
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

#if defined(__linux__) || defined(__CYGWIN__)
#  include <endian.h>
#if ((__GLIBC__ == 2) && (__GLIBC_MINOR__ < 9))
#if defined(__BYTE_ORDER) && (__BYTE_ORDER == __LITTLE_ENDIAN)
#  include <arpa/inet.h>
#  define htobe16(x) htons(x)
#  define htobe64(x) (((uint64_t)htonl(((uint32_t)(((uint64_t)(x)) >> 32)))) | \
   (((uint64_t)htonl(((uint32_t)(x)))) << 32))
#  define be16toh(x) ntohs(x)
#  define be32toh(x) ntohl(x)
#  define be64toh(x) (((uint64_t)ntohl(((uint32_t)(((uint64_t)(x)) >> 32)))) | \
   (((uint64_t)ntohl(((uint32_t)(x)))) << 32))
#else
#  error Byte Order not supported!
#endif
#endif
#elif defined(__sun__)
#  include <sys/byteorder.h>
#  define htobe16(x) BE_16(x)
#  define htobe64(x) BE_64(x)
#  define be16toh(x) BE_IN16(x)
#  define be32toh(x) BE_IN32(x)
#  define be64toh(x) BE_IN64(x)
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#  include <sys/endian.h>
#elif defined(__OpenBSD__)
#  include <sys/types.h>
#  if !defined(be16toh)
#    define be16toh(x) betoh16(x)
#  endif
#  if !defined(be32toh)
#    define be32toh(x) betoh32(x)
#  endif
#  if !defined(be64toh)
#    define be64toh(x) betoh64(x)
#  endif
#elif defined(__APPLE__)
#  include <libkern/OSByteOrder.h>
#  define htobe16(x) OSSwapHostToBigInt16(x)
#  define htobe64(x) OSSwapHostToBigInt64(x)
#  define be16toh(x) OSSwapBigToHostInt16(x)
#  define be32toh(x) OSSwapBigToHostInt32(x)
#  define be64toh(x) OSSwapBigToHostInt64(x)
#else
#  error Platform not supported!
#endif

#define WS_BAD_REQUEST_STR "HTTP/1.1 400 Invalid Request\r\n\r\n"
#define WS_SWITCH_PROTO_STR "HTTP/1.1 101 Switching Protocols"
#define WS_TOO_BUSY_STR "HTTP/1.1 503 Service Unavailable\r\n\r\n"
#define WS_INTERNAL_ERROR_STR "HTTP/1.1 505 Internal Server Error\r\n\r\n"

#define CRLF "\r\n"

#define WS_MAGIC_STR        "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_KEY_LEN          16
#define SHA_DIGEST_LEN      20

#define MAX_FRAME_PAYLOAD_SIZE      (1024 * 4)
#define MAX_INMEM_MESSAGE_SIZE      (1024 * 64)

/* 512 KiB throttle threshold per stream */
#define SOCK_THROTTLE_THLD          (1024 * 512)

#define MIN_PING_TIMER_INTERVAL             (1 * 1000)      // 1 seconds

#define MIN_NO_RESPONSE_TIME_TO_PING        3
#define NO_RESPONSE_TIME_TO_PING            30
#define MIN_NO_RESPONSE_TIME_TO_CLOSE       6
#define NO_RESPONSE_TIME_TO_CLOSE           90

#define WS_CLOSE_NORMAL                     1000
#define WS_CLOSE_GOING_AWAY                 1001
#define WS_CLOSE_PROTO_ERR                  1002
#define WS_CLOSE_INVALID_UTF8               1007
#define WS_CLOSE_TOO_LARGE                  1009
#define WS_CLOSE_UNEXPECTED                 1011

enum {
    K_EVENT_TYPE_MIN = 0,

#define EVENT_TYPE_HANDSHAKE                "handshake"
    K_EVENT_TYPE_HANDSHAKE = K_EVENT_TYPE_MIN,
#define EVENT_TYPE_MESSAGE                  "message"
    K_EVENT_TYPE_MESSAGE,
#define EVENT_TYPE_ERROR                    "error"
    K_EVENT_TYPE_ERROR,
#define EVENT_TYPE_CLOSE                    "close"
    K_EVENT_TYPE_CLOSE,

    K_EVENT_TYPE_MAX = K_EVENT_TYPE_CLOSE,
};

#define NR_EVENT_TYPES          (K_EVENT_TYPE_MAX - K_EVENT_TYPE_MIN + 1)

/* The frame operation codes for WebSocket */
typedef enum ws_opcode {
    WS_OPCODE_CONTINUATION = 0x00,
    WS_OPCODE_TEXT = 0x01,
    WS_OPCODE_BIN = 0x02,
    WS_OPCODE_END = 0x03,
    WS_OPCODE_CLOSE = 0x08,
    WS_OPCODE_PING = 0x09,
    WS_OPCODE_PONG = 0x0A,
} ws_opcode;

/* WS Client Info */
typedef struct ws_client_info
{
    struct timeval start_proc;
    struct timeval end_proc;
} ws_client_info;

/* The frame header for WebSocket */
typedef struct ws_frame_header {
    uint8_t fin;
    uint8_t rsv;
    uint8_t op;
    uint8_t mask;
    size_t  sz_payload;
} ws_frame_header;

#define WS_OK                   0x00000000
#define WS_READING              0x00001000
#define WS_SENDING              0x00002000
#define WS_CLOSING              0x00004000
#define WS_THROTTLING           0x00008000
#define WS_WAITING4PAYLOAD      0x00010000
#define WS_WAITING4HSREQU       0x00020000
#define WS_WAITING4HSRESP       0x00040000

#define WS_TLS_ACCEPTING        0x00100000
#define WS_TLS_READING          0x00200000
#define WS_TLS_WRITING          0x00400000
#define WS_TLS_SHUTTING         0x00800000
#define WS_TLS_CONNECTING       0x01000000
#define WS_TLS_WANT_READ        0x10000000
#define WS_TLS_WANT_WRITE       0x20000000
#define WS_TLS_WANT_RW          0x30000000

#define WS_ERR_ANY              0x00000FFF

enum ws_error_code {
    WS_ERR_TLR      = 0x00000001,   /* Too long request */
    WS_ERR_OOM      = 0x00000002,
    WS_ERR_SSL      = 0x00000003,
    WS_ERR_IO       = 0x00000004,
    WS_ERR_SRV      = 0x00000005,
    WS_ERR_MSG      = 0x00000006,
    WS_ERR_LTNR     = 0x00000007,   /* Long time no response */
};

typedef struct ws_pending_data {
    struct list_head list;

    /* the size of data */
    size_t  szdata;
    /* the size of sent */
    size_t  szsent;
    /* pointer to the pending data */
    unsigned char data[0];
} ws_pending_data;

typedef ssize_t (*fn_writer)(struct pcdvobjs_stream *, const void *, size_t);
typedef ssize_t (*fn_reader)(struct pcdvobjs_stream *, void *, size_t);
typedef int (*cb_io)(struct pcdvobjs_stream *);

enum {
    WS_ROLE_CLIENT = 0,
    WS_ROLE_SERVER,
    WS_ROLE_SERVER_WORKER,      // a server-side worker handling handshake.
    WS_ROLE_SERVER_WORKER_WOHS, // a server-side worker
                                // without handling handshanke
};

struct stream_extended_data {
    /* the status */
    unsigned            status;
    short               role;
    short               msg_type;

    /* the time last got data from the peer */
    struct timespec     last_live_ts;
    pcintr_timer_t      ping_timer;

    /* configuration */
    size_t              maxmessagesize;
    /* The maximum no response seconds to send a PING message. */
    uint32_t            noresptimetoping;
    /* The maximum no response seconds to close the socket. */
    uint32_t            noresptimetoclose;

    size_t              sz_used_mem;
    size_t              sz_peak_used_mem;

    purc_atom_t         event_cids[NR_EVENT_TYPES];

    fn_reader           reader;
    fn_writer           writer;
    cb_io               on_readable;
    cb_io               on_writable;

    char               *ws_key;         // Server-only: the handshake key.
    purc_variant_t      prot_opts;      // Client-only: the handshake headers.

#if HAVE(OPENSSL)
    SSL_CTX            *ssl_ctx;        /* if this stream acts as a client. */
    SSL                *ssl;
    struct openssl_shctx_wrapper
                       *ssl_shctx_wrapper;
    int                 sslstatus;      /* ssl connection status. */
#endif

    /* fields for pending data to write */
    size_t              sz_pending;
    struct list_head    pending;

    /* buffer for handshake. */
#define SZ_HSBUF_INC        512
#define SZ_HSBUF_MAX        8192    /* a reasonable size for request headers */
    char               *hsbuf;      /* the pointer to the handshake buffer */
    size_t              sz_hsbuf;   /* the current size of handshake buffer */
    size_t              sz_read_hsbuf;  /* read bytes in handshake buffer. */

    /* current frame header */
    ws_frame_header     header;
    char                header_buf[2];
    size_t              sz_header;
    size_t              sz_read_header;

    /* current frame payload length */
    char                ext_paylen_buf[8];
    uint8_t             sz_ext_paylen;
    uint8_t             sz_read_ext_paylen;

    /* current mask */
    unsigned char       mask[4];
    int                 sz_mask;
    int                 sz_read_mask;

    /* fields for current reading message */
    size_t              sz_message;         /* total size of current message */
    size_t              sz_read_message;    /* read size of current message */
    char               *message;            /* message data */

    size_t              sz_payload;         /* total size of current payload */
    size_t              sz_read_payload;    /* read size of current payload */
    char               *payload;            /* payload data */
};

static ssize_t ws_read_data(struct pcdvobjs_stream *stream,
        void *buff, size_t sz);
static ssize_t ws_write_data(struct pcdvobjs_stream *stream,
        const char *buffer, size_t len);
static int ws_handle_reads(struct pcdvobjs_stream *stream);
static int ws_handle_writes(struct pcdvobjs_stream *stream);
static void cleanup_extension(struct pcdvobjs_stream *stream);

/* Set the connection status for the given client and return the given
 * bytes.
 *
 * The given number of bytes are returned. */
static inline ssize_t
ws_set_status(struct stream_extended_data *ext, int status, ssize_t bytes)
{
    ext->status = status;
    return bytes;
}

static inline void ws_update_mem_stats(struct stream_extended_data *ext)
{
    ext->sz_used_mem = ext->sz_pending + ext->sz_message;
    if (ext->sz_used_mem > ext->sz_peak_used_mem)
        ext->sz_peak_used_mem = ext->sz_used_mem;
}

static int ws_status_to_pcerr(struct stream_extended_data *ext)
{
    enum ws_error_code code = (enum ws_error_code)(ext->status & WS_ERR_ANY);
    switch (code) {
        case WS_ERR_TLR:
            return PURC_ERROR_TOO_LONG;
        case WS_ERR_OOM:
            return PURC_ERROR_OUT_OF_MEMORY;
        case WS_ERR_SSL:
            return PURC_ERROR_TLS_FAILURE;
        case WS_ERR_IO:
            return PURC_ERROR_IO_FAILURE;
        case WS_ERR_MSG:
            return PURC_ERROR_PROTOCOL_VIOLATION;
        case WS_ERR_SRV:
            return PURC_ERROR_CONNECTION_ABORTED;
        case WS_ERR_LTNR:
            return PURC_ERROR_TIMEOUT;
    }

    return PURC_ERROR_OK;
}

static void ws_handle_rwerr_close(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;

    if (ext->status & WS_ERR_ANY) {
        stream->ext0.msg_ops->on_error(stream, ws_status_to_pcerr(ext));
    }

    if (ext->status & WS_CLOSING) {
        cleanup_extension(stream);
    }
}

static void
ws_sha1_digest(const char *s, int len, unsigned char *digest)
{
    pcutils_sha1_ctxt sha;

    pcutils_sha1_begin(&sha);
    pcutils_sha1_hash(&sha, (uint8_t *) s, len);
    pcutils_sha1_end(&sha, digest);
}

static void
ws_key_to_accept_encoded(const char *key, char *encoded, size_t sz)
{
    size_t klen = strlen(key);
    size_t mlen = strlen(WS_MAGIC_STR);
    size_t len = klen + mlen;
    char accept[klen + mlen + 1];
    uint8_t digest[SHA_DIGEST_LEN] = { 0 };

    memset(digest, 0, sizeof *digest);
    memcpy(accept, key, klen);
    memcpy(accept + klen, WS_MAGIC_STR, mlen + 1);
    ws_sha1_digest(accept, len, digest);
    pcutils_b64_encode((unsigned char *)digest, sizeof(digest), encoded, sz);
}

/* Make a string uppercase.
 *
 * On error the original string is returned.
 * On success, the uppercased string is returned. */
static char *
strtoupper(char *str)
{
    char *p = str;
    if (str == NULL || *str == '\0')
        return str;

    while (*p != '\0') {
        *p = toupper(*p);
        p++;
    }

    return str;
}

/* Parse a request containing the method and protocol.
 *
 * On error, or unable to parse, NULL is returned.
 * On success, the HTTP request is returned and the method and
 * protocol are assigned to the corresponding buffers. */
static char *
ws_parse_request(char *line, char **method, char **protocol)
{
    char *p, *path = NULL;

    /* Pattern in the line:
     *      GET <path> HTTP/1.1
     */
    if ((p = strchr(line, ' '))) {
        *method = line;
        *p = '\0';
        strtoupper(*method);
        line = p + 1;
    }
    else {
        goto error;
    }

    if ((p = strstr(line, " HTTP/1.1"))) {
        path = line;
        *p = '\0';
        *protocol = p + 1;
    }

error:
    return path;
}

#define MAKE_STRING_PROPERTY(obj, cstr, name)                           \
    if (cstr) {                                                         \
        purc_variant_t tmp = purc_variant_make_string(cstr, true);      \
        if (tmp) {                                                      \
            purc_variant_object_set_by_static_ckey(obj, name, tmp);     \
            purc_variant_unref(tmp);                                    \
        }                                                               \
    }

#define MAKE_NUMBER_PROPERTY(obj, number, name)                         \
    do {                                                                \
        purc_variant_t tmp = purc_variant_make_number(number);          \
        if (tmp) {                                                      \
            purc_variant_object_set_by_static_ckey(obj, name, tmp);     \
            purc_variant_unref(tmp);                                    \
        }                                                               \
    } while (0)

static int ws_verify_handshake_request(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;

    char *path = NULL;
    char *method = NULL;
    char *protocol = NULL;
    char *host = NULL;
    char *agent = NULL;
    char *origin = NULL;
    char *upgrade = NULL;
    char *referer = NULL;
    char *connection = NULL;
    char *ws_key = NULL;
    char *ws_ver = NULL;
    char *ws_protocol = NULL;
    char *ws_extensions = NULL;

    char *line = ext->hsbuf, *next = NULL;

    while (line) {
        size_t len;
        if ((next = strstr(line, CRLF)) != NULL) {
            next[0] = next[1] = 0;
            len = (next - line);
        }
        else {
            len = strlen(line);
        }

        if (len >= 4 && strncasecmp2ltr(line, "GET ", 4) == 0) {
            if ((path = ws_parse_request(line, &method, &protocol)) == NULL) {
                break;
            }
        }
        else {
            char *value = strchr(line, ' ');
            if (value == NULL) {
                break;
            }

            *value = '\0';
            value += 1;
#if 0
            char *end = strstr(value, CRLF);
            if (end == NULL) {
                break;
            }
            *end = '\0';
#endif

            char *key = line;
            if (strcasecmp("Host:", key) == 0)
                host = value;
            else if (strcasecmp("Origin:", key) == 0)
                origin = value;
            else if (strcasecmp("Upgrade:", key) == 0)
                upgrade = value;
            else if (strcasecmp("Connection:", key) == 0)
                connection = value;
            else if (strcasecmp("User-Agent:", key) == 0)
                agent = value;
            else if (strcasecmp("Referer:", key) == 0)
                referer = value;
            else if (strcasecmp("Sec-WebSocket-Key:", key) == 0)
                ws_key = value;
            else if (strcasecmp("Sec-WebSocket-Version:", key) == 0)
                ws_ver = value;
            else if (strcasecmp("Sec-WebSocket-Protocol:", key) == 0)
                ws_protocol = value;
            else if (strcasecmp("Sec-WebSocket-Extensions:", key) == 0)
                ws_extensions = value;
        }

        line = next ? (next + 2) : NULL;
        if (next && strcmp(next, CRLF CRLF) == 0) {
            break;
        }
    }

    PC_DEBUG("parsed request: method(%s), path(%s), protocol(%s)\n",
            method, path, protocol);
    PC_DEBUG("parsed request: host(%s), upgrade(%s), connection(%s)\n",
            host, upgrade, connection);
    PC_DEBUG("parsed request: origin(%s), ws_key(%s), ws_ver(%s)\n",
            origin, ws_key, ws_ver);

    if (path == NULL || host == NULL || upgrade == NULL || connection == NULL ||
            origin == NULL || ws_key == NULL || ws_ver == NULL) {
        // Bad request.
        PC_ERROR("Bad handshake request:\n");
        return -1;
    }

    if (strchr(path, '%') &&
            pcdvobj_url_decode_in_place(path, strlen(path), PURC_K_KW_rfc3986)) {
        PC_ERROR("Failed to decode path (%s)\n", path);
        return -1;
    }

    if (strcasecmp(upgrade, "websocket")) {
        PC_ERROR("Bad upgrade in handshake request headers: %s\n", upgrade);
        return -1;
    }

    if (strcasecmp(connection, "upgrade")) {
        PC_ERROR("Bad connection in handshake request headers: %s\n",
                connection);
        return -1;
    }

    if (strcmp(ws_ver, "13")) {
        PC_ERROR("Bad Sec-WebSocket-Version in handshake request headers: %s\n",
                connection);
        return -1;
    }

    purc_atom_t target = ext->event_cids[K_EVENT_TYPE_HANDSHAKE];
    if (target != 0) {
        purc_variant_t obj = purc_variant_make_object_0();
        if (obj) {
            MAKE_STRING_PROPERTY(obj, path,         "Path");
            MAKE_STRING_PROPERTY(obj, method,       "Method");
            MAKE_STRING_PROPERTY(obj, protocol,     "Protocol");
            MAKE_STRING_PROPERTY(obj, host,         "Host");
            MAKE_STRING_PROPERTY(obj, origin,       "Origin");
            MAKE_STRING_PROPERTY(obj, upgrade,      "Upgrade");
            MAKE_STRING_PROPERTY(obj, connection,   "Connection");
            MAKE_STRING_PROPERTY(obj, agent,        "User-Agent");
            MAKE_STRING_PROPERTY(obj, referer,      "Referer");
            MAKE_STRING_PROPERTY(obj, ws_key,       "Sec-WebSocket-Key");
            MAKE_STRING_PROPERTY(obj, ws_ver,       "Sec-WebSocket-Version");
            MAKE_STRING_PROPERTY(obj, ws_protocol,  "Sec-WebSocket-Protocol");
            MAKE_STRING_PROPERTY(obj, ws_extensions,"Sec-WebSocket-Extensions");

            pcintr_coroutine_post_event(target,
                    PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
                    EVENT_TYPE_HANDSHAKE, NULL,
                    obj, PURC_VARIANT_INVALID);

            purc_variant_unref(obj);
        }
    }

    if (ext->ws_key)
        free(ext->ws_key);
    /* keep this for send_handshake_resp() */
    ext->ws_key = strdup(ws_key);

    return 0;
}

static int ws_verify_handshake_response(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    int ret = -1;

    char accept[SHA_DIGEST_LEN * 4 + 1];
    ws_key_to_accept_encoded(ext->ws_key, accept, sizeof(accept));

    char *line = ext->hsbuf, *next = NULL;

    int status = 0;
    char *upgrade = NULL;
    char *connection = NULL;
    char *ws_accept = NULL;
    char *ws_prot = NULL;
    char *ws_ext = NULL;

    while (line) {
        if ((next = strstr(line, CRLF)) == NULL) {
            break;
        }

        if (strncmp2ltr(line, "HTTP/", 5) == 0) {
            /* check HTTP status code only. */
            char *p = strchr(line, ' ');
            status = atoi(p + 1);
        }
        else {
            char *value = strchr(line, ' ');
            if (value == NULL) {
                break;
            }

            *value = '\0';
            value += 1;
            char *end = strstr(value, CRLF);
            if (end == NULL) {
                break;
            }
            *end = '\0';

            char *key = line;
            if (strcmp(key, "Upgrade:") == 0)
                upgrade = value;
            else if (strcmp(key, "Connection:") == 0)
                connection = value;
            else if (strcmp(key, "Sec-WebSocket-Accept:") == 0)
                ws_accept = value;
            else if (strcmp(key, "Sec-WebSocket-Protocol:") == 0)
                ws_prot = value;
            else if (strcmp(key, "Sec-WebSocket-Extensions:") == 0)
                ws_ext = value;
        }

        line = next + 2;
        if (strcmp(next, CRLF CRLF) == 0) {
            break;
        }
    }

    const char *extra_msg = NULL;
    if (status != 101) {
        extra_msg = "Got a bad HTTP status during handshake";
    }
    else if (connection == NULL || strcasecmp(connection, "upgrade")) {
        extra_msg = "Not 'connection` header found during handshake";
    }
    else if (upgrade == NULL || strcasecmp(upgrade, "websocket")) {
        extra_msg = "No 'upgrade' header found during handshake";
    }
    else if (ws_accept == NULL || strcmp(ws_accept, accept)) {
        extra_msg = "Failed to verify Sec-WebSocket-Accept during handshake";
    }
    else {
        extra_msg = "Everything is ok";
        ret = 0;
    }

    purc_atom_t target = ext->event_cids[K_EVENT_TYPE_HANDSHAKE];
    if (target != 0) {
        purc_variant_t obj = purc_variant_make_object_0();
        if (obj) {
            MAKE_NUMBER_PROPERTY(obj, status, "Status");
            if (status == 101) {
                MAKE_STRING_PROPERTY(obj, upgrade, "Upgrade");
                MAKE_STRING_PROPERTY(obj, connection, "Connection");
                MAKE_STRING_PROPERTY(obj, ws_prot, "Sec-WebSocket-Protocol");
                MAKE_STRING_PROPERTY(obj, ws_ext, "Sec-WebSocket-Extensions");
            }

            if (extra_msg)
                MAKE_STRING_PROPERTY(obj, extra_msg, "Extra-Message");

            pcintr_coroutine_post_event(target,
                    PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
                    EVENT_TYPE_HANDSHAKE, NULL,
                    obj, PURC_VARIANT_INVALID);

            purc_variant_unref(obj);
        }
    }

    free(ext->ws_key);
    ext->ws_key = NULL;

    return ret;
}

enum {
    READ_TLR = -3,
    READ_OOM = -2,
    READ_ERROR = -1,
    READ_NONE,
    READ_SOME,
    READ_WHOLE,
};

/* Tries to read handshake data. */
static int try_to_read_handshake_data(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;

    ssize_t nr_total = 0;
    do {
        if (ext->sz_read_hsbuf + SZ_HSBUF_INC > ext->sz_hsbuf) {
            if (ext->sz_hsbuf + SZ_HSBUF_INC >= SZ_HSBUF_MAX) {
                return READ_TLR;
            }

            ext->hsbuf = realloc(ext->hsbuf, ext->sz_hsbuf + SZ_HSBUF_INC);
            if (ext->hsbuf == NULL)
                return READ_OOM;

            ext->sz_hsbuf += SZ_HSBUF_INC;
        }

        ssize_t nr_one_read;
        nr_one_read = ws_read_data(stream, ext->hsbuf + ext->sz_read_hsbuf,
                SZ_HSBUF_INC - 1);
        if (nr_one_read == -1) {
            return READ_ERROR;
        }

        ext->sz_read_hsbuf += nr_one_read;
        nr_total += nr_one_read;
        PC_DEBUG("handshake buffer info: sz_hsbuf(%zu), sz_read_hsbuf(%zu), nr_total(%zd)\n",
                ext->sz_hsbuf, ext->sz_read_hsbuf, nr_total);

#if HAVE(OPENSSL)
    } while (ext->ssl ? SSL_pending(ext->ssl): 0);
#else
    } while (0);
#endif

    ext->hsbuf[ext->sz_read_hsbuf] = 0; /* set terminating null byte */

    if (ext->sz_read_hsbuf >= 4 &&
            strcmp(ext->hsbuf + ext->sz_read_hsbuf - 4, CRLF CRLF) == 0) {
        return READ_WHOLE;
    }

    return READ_SOME;
}

static void
reset_handshake_buffer(struct stream_extended_data *ext)
{
    free(ext->hsbuf);
    ext->hsbuf = NULL;
    ext->sz_hsbuf = 0;
    ext->sz_read_hsbuf = 0;
}

/* Handle the handshake request from a client. */
static int
ws_handle_handshake_request(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    assert(ext->status & WS_WAITING4HSREQU);

    clock_gettime(CLOCK_MONOTONIC, &ext->last_live_ts);

    PC_DEBUG("Time to read handshake request.\n");

    const char *response = NULL;
    size_t resp_len = 0;

    int retv = 0;
    switch (try_to_read_handshake_data(stream)) {
    case READ_NONE:
        break;
    case READ_SOME:
        if (ext->sz_read_hsbuf >= 4 &&
                strncasecmp2ltr(ext->hsbuf, "GET ", 4) != 0) {
            /* Send the Bad request response to the client. */
            ws_write_data(stream, WS_BAD_REQUEST_STR,
                    sizeof(WS_BAD_REQUEST_STR) - 1);
            reset_handshake_buffer(ext);
            ext->status = WS_ERR_MSG | WS_CLOSING;
            retv = -1;
        }
        break;

    case READ_ERROR:
        /* Send the Internal Server Error response to the client. */
        response = WS_INTERNAL_ERROR_STR;
        resp_len = sizeof(WS_INTERNAL_ERROR_STR) - 1;
        ext->status |= WS_CLOSING;
        retv = -1;
        break;

    case READ_TLR:
        /* Send the Invalid Request response to the client. */
        response = WS_BAD_REQUEST_STR;
        resp_len = sizeof(WS_BAD_REQUEST_STR) - 1;
        ext->status = WS_ERR_TLR | WS_CLOSING;
        retv = -1;
        break;

    case READ_OOM:
        /* Send the Internal Server Error response to the client. */
        response = WS_INTERNAL_ERROR_STR;
        resp_len = sizeof(WS_INTERNAL_ERROR_STR) - 1;
        ext->status = WS_ERR_OOM | WS_CLOSING;
        retv = -1;
        break;

    case READ_WHOLE:
        ext->status &= ~WS_WAITING4HSREQU;
        ext->on_readable = ws_handle_reads; /* switch to regular rw mode. */
        PC_DEBUG("Got handshake request:\n%s", ext->hsbuf);
        if (ws_verify_handshake_request(stream)) {
            /* Send the Invalid Request response to the client. */
            response = WS_BAD_REQUEST_STR;
            resp_len = sizeof(WS_BAD_REQUEST_STR) - 1;
            ext->status = WS_ERR_MSG | WS_CLOSING;
            retv = -1;
        }
        reset_handshake_buffer(ext);
        break;
    }

    if (ext->role != WS_ROLE_CLIENT && response)
        ws_write_data(stream, response, resp_len);

    if (retv)
        ws_handle_rwerr_close(stream);
    return retv;
}

/* Handle the handshake response from the server. */
static int
ws_handle_handshake_response(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    assert(ext->status & WS_WAITING4HSRESP);

    clock_gettime(CLOCK_MONOTONIC, &ext->last_live_ts);

    PC_DEBUG("Time to read handshake response.\n");

    int retv = 0;
    switch (try_to_read_handshake_data(stream)) {
    case READ_NONE:
        break;
    case READ_SOME:
        if (ext->sz_read_hsbuf > 5 &&
                strncmp2ltr(ext->hsbuf, "HTTP/", 5) != 0) {
            reset_handshake_buffer(ext);
            ext->status = WS_ERR_SRV | WS_CLOSING;
            retv = -1;
        }
        break;

    case READ_ERROR:
        ext->status |= WS_CLOSING;
        retv = -1;
        break;

    case READ_TLR:
        ext->status = WS_ERR_TLR | WS_CLOSING;
        retv = -1;
        break;

    case READ_OOM:
        ext->status = WS_ERR_OOM | WS_CLOSING;
        retv = -1;
        break;

    case READ_WHOLE:
        ext->status &= ~WS_WAITING4HSRESP;
        ext->on_readable = ws_handle_reads; /* switch to normal mode */
        PC_DEBUG("Got handshake response:\n%s", ext->hsbuf);
        if (ws_verify_handshake_response(stream)) {
            ext->status = WS_ERR_SRV | WS_CLOSING;
            retv = -1;
        }
        reset_handshake_buffer(ext);
        break;
    }

    if (retv)
        ws_handle_rwerr_close(stream);
    return retv;
}

// Mozilla/5.0 (<system-information>) <platform> (<platform-details>) <extensions>
#if OS(LINUX)
#   define USER_AGENT  "Mozilla/5.0 (Linux) Foil/Chouniu PurC/0.9.22"
#elif OS(MAC_OS_X)
#   define USER_AGENT  "Mozilla/5.0 (macOS) Foil/Chouniu PurC/0.9.22"
#else
#   define USER_AGENT  "Mozilla/5.0 (Unknown) Foil/Chouniu PurC/0.9.22"
#endif

#define DEFINE_STRING_VAR_FROM_OBJECT(name, alternative)        \
    const char *name;                                           \
    tmp = purc_variant_object_get_by_ckey(extra_opts, #name);   \
    name = (tmp == PURC_VARIANT_INVALID) ? alternative :        \
        purc_variant_get_string_const(tmp);                     \
    if (name == NULL) {                                         \
        purc_set_error(PURC_ERROR_INVALID_VALUE);               \
        goto error;                                             \
    }

static int
ws_client_handshake(struct pcdvobjs_stream *stream, purc_variant_t extra_opts)
{
    struct stream_extended_data *ext = stream->ext0.data;
    purc_variant_t tmp;

    DEFINE_STRING_VAR_FROM_OBJECT(path, stream->url ? stream->url->path : NULL);
    DEFINE_STRING_VAR_FROM_OBJECT(host, stream->url ? stream->url->host : NULL);
    DEFINE_STRING_VAR_FROM_OBJECT(origin, "hvml.fmsoft.cn");
    DEFINE_STRING_VAR_FROM_OBJECT(useragent, USER_AGENT);
    DEFINE_STRING_VAR_FROM_OBJECT(referer, "https://hvml.fmsoft.cn/");

    purc_clr_error();       /* XXX: work-around */

    /* generate Sec-WebSocket-Key */
    srandom(time(NULL));
    char key[WS_KEY_LEN];
    for (int i = 0; i < WS_KEY_LEN; i++) {
        key[i] = random() & 0xff;
    }

    ext->ws_key = pcutils_b64_encode_alloc((unsigned char *)key, WS_KEY_LEN);
    if (ext->ws_key == NULL) {
        PC_ERROR("Failed pcutils_b64_encode_alloc() to make the key.\n");
        goto failed_key;
    }

    char *req_headers = NULL;
    int n = asprintf(&req_headers,
            "GET %s HTTP/1.1" CRLF
            "Upgrade: websocket" CRLF
            "Connection: Upgrade" CRLF
            "Host: %s" CRLF
            "Origin: %s" CRLF
            "User-Agent: %s" CRLF
            "Referer: %s:" CRLF
            "Sec-WebSocket-Key: %s" CRLF
            "Sec-WebSocket-Version: 13" CRLF,
            path, host, origin, useragent, referer, ext->ws_key);

    if (n < 0) {
        PC_ERROR("Failed asprintf() to make the request headers.\n");
        goto failed_req;
    }

    /* Send the handshake request to the server. */
    if (ws_write_data(stream, req_headers, n) <= 0) {
        PC_ERROR("Failed ws_write_sock(): no any bytes send to the server .\n");
    }
    else {
        struct extra_header {
            const char *name;
            const char *header;
        } extra_headers[] = {
            { "extensions", "Sec-WebSocket-Extensions: "},
            { "subprotocols","Sec-WebSocket-Protocol: " },
        };

        size_t nr_headers = 0, nr_wrotten = 0;
        for (size_t i = 0; i < PCA_TABLESIZE(extra_headers); i++) {
            tmp = purc_variant_object_get_by_ckey(extra_opts,
                    extra_headers[i].name);

            if (tmp) {
                size_t len;
                const char *value;
                value  = purc_variant_get_string_const_ex(tmp, &len);
                if (value && len > 0) {
                    nr_headers++;
                    if (ws_write_data(stream, extra_headers[i].header,
                                strlen(extra_headers[i].header)) <= 0)
                        break;
                    if (ws_write_data(stream, value, len) <= 0)
                        break;
                    if (ws_write_data(stream, CRLF, sizeof(CRLF) - 1) <= 0)
                        break;
                    nr_wrotten++;
                }
                else {
                    PC_WARN("%s for header %s is defined but invalid.\n",
                            extra_headers[i].name, extra_headers[i].header);
                }
            }
        }

        if (nr_headers == nr_wrotten) {
            if (ws_write_data(stream, CRLF, sizeof(CRLF) - 1) <= 0)
                goto failed_write;
        }
        else {
            goto failed_write;
        }
    }

    free(req_headers);
    ext->status = WS_WAITING4HSRESP;
    return 0;

failed_req:
    free(ext->ws_key);
    ext->ws_key = NULL;

failed_key:
    purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
    ext->status = WS_ERR_OOM | WS_CLOSING;
    return -1;

failed_write:
    free(req_headers);
    free(ext->ws_key);
    ext->ws_key = NULL;

    purc_set_error(PURC_ERROR_IO_FAILURE);
    ext->status |= WS_CLOSING;
    return -1;

error:
    purc_set_error(PURC_ERROR_INVALID_VALUE);
    ext->status |= WS_CLOSING;
    return -1;
}

#if HAVE(OPENSSL)

/* Log result code for TLS/SSL I/O operation */
static void
log_return_message_ssl(int ret, int err, const char *fn)
{
    unsigned long e;

    switch (err) {
        case SSL_ERROR_NONE:
            PC_INFO("SSL: %s -> SSL_ERROR_NONE\n", fn);
            PC_INFO("SSL: TLS/SSL I/O operation completed\n");
            break;
        case SSL_ERROR_WANT_READ:
            PC_INFO("SSL: %s -> SSL_ERROR_WANT_READ\n", fn);
            PC_INFO("SSL: incomplete, data available for reading\n");
            break;
        case SSL_ERROR_WANT_WRITE:
            PC_INFO("SSL: %s -> SSL_ERROR_WANT_WRITE\n", fn);
            PC_INFO("SSL: incomplete, data available for writing\n");
            break;
        case SSL_ERROR_ZERO_RETURN:
            PC_INFO("SSL: %s -> SSL_ERROR_ZERO_RETURN\n", fn);
            PC_INFO("SSL: TLS/SSL connection has been closed\n");
            break;
        case SSL_ERROR_WANT_X509_LOOKUP:
            PC_INFO("SSL: %s -> SSL_ERROR_WANT_X509_LOOKUP\n", fn);
            break;
        case SSL_ERROR_SYSCALL:
            PC_INFO("SSL: %s -> SSL_ERROR_SYSCALL\n", fn);

            e = ERR_get_error();
            if (e > 0)
                PC_INFO("SSL: %s -> %s\n", fn, ERR_error_string(e, NULL));

            /* call was not successful because a fatal error occurred either at the
             * protocol level or a connection failure occurred. */
            if (ret != 0) {
                PC_INFO("SSL bogus handshake interrupt: %s\n", strerror(errno));
                break;
            }
            /* call not yet finished. */
            PC_INFO("SSL: handshake interrupted, got EOF\n");
            if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                PC_INFO("SSL: %s -> not yet finished %s\n", fn, strerror(errno));
            break;
        default:
            PC_INFO("SSL: %s -> failed fatal error code: %d\n", fn, err);
            PC_INFO("SSL: %s\n", ERR_error_string (ERR_get_error(), NULL));
            break;
    }
}

/* Shut down the stream's TLS/SSL connection
 *
 * On fatal error, 1 is returned.
 * If data still needs to be read/written, -1 is returned.
 * On success, the TLS/SSL connection is closed and 0 is returned */
static int
shutdown_ssl(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    int ret = -1, err = 0;

    /* all good */
    if ((ret = SSL_shutdown(ext->ssl)) > 0)
        return ws_set_status(ext, WS_CLOSING, 0);

    err = SSL_get_error(ext->ssl, ret);
    log_return_message_ssl(ret, err, "SSL_shutdown");

    switch (err) {
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE:
        ext->sslstatus = WS_TLS_SHUTTING;
        break;
    case SSL_ERROR_SYSCALL:
        if (ret == 0) {
            PC_INFO("SSL_shutdown, connection unexpectedly closed by peer.\n");
            /* The shutdown is not yet finished. */
            if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
                ext->sslstatus = WS_TLS_SHUTTING;
            break;
        }
        PC_INFO("SSL: SSL_shutdown, probably unrecoverable, forcing close.\n");
        // fallthrough
    case SSL_ERROR_ZERO_RETURN:
    case SSL_ERROR_WANT_X509_LOOKUP:
    default:
        return ws_set_status(ext, WS_ERR_SSL | WS_CLOSING, 1);
    }

    return ret;
}

/* Ask for a TLS/SSL stream to initiate a TLS/SSL handshake
 *
 * On fatal error, the connection is shut down.
 * If data still needs to be read/written, -1 is returned.
 * On success, the TLS/SSL connection is completed and 0 is returned */
static int
handle_connect_ssl(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    int ret = -1, err = 0;

    // clock_gettime(CLOCK_MONOTONIC, &ext->last_live_ts);

    /* all good on TLS handshake */
    if ((ret = SSL_connect(ext->ssl)) > 0) {
        ext->sslstatus &= ~(WS_TLS_CONNECTING | WS_TLS_WANT_RW);

        assert(ext->prot_opts);

        /* Now send a handshake request and wait for the reponse from server. */
        int r = ws_client_handshake(stream, ext->prot_opts);
        purc_variant_unref(ext->prot_opts);
        ext->prot_opts = PURC_VARIANT_INVALID;

        if (r != 0) {
            PC_ERROR("Failed ws_client_handshake(): %s\n", stream->peer_addr);
            ret = ws_set_status(ext, WS_ERR_SSL | WS_CLOSING, -1);
        }
        else {
            ext->on_readable = ws_handle_handshake_response;
            PC_INFO("SSL Connected: %d %s\n", stream->fd4r, stream->peer_addr);
        }
        return 0;
    }

    err = SSL_get_error(ext->ssl, ret);
    log_return_message_ssl(ret, err, "SSL_connect");

    switch (err) {
    case SSL_ERROR_WANT_READ:
        ext->sslstatus = WS_TLS_WANT_READ | WS_TLS_CONNECTING;
        ret = 0;
        break;
    case SSL_ERROR_WANT_WRITE:
        ext->sslstatus = WS_TLS_WANT_WRITE | WS_TLS_CONNECTING;
        ret = 0;
        break;
    case SSL_ERROR_SYSCALL:
        /* Wait for more activity else bail out, for instance if
           the socket is closed during the handshake. */
        if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK ||
                    errno == EINTR)) {
            ext->sslstatus = WS_TLS_WANT_RW | WS_TLS_CONNECTING;
            ret = 0;
            break;
        }
        /* The peer notified that it is shutting down through
           a SSL "close_notify" so we shutdown too */
        // fallthrough
    case SSL_ERROR_ZERO_RETURN:
    case SSL_ERROR_WANT_X509_LOOKUP:
    default:
        ext->sslstatus &= ~WS_TLS_CONNECTING;
        ret = ws_set_status(ext, WS_ERR_SSL | WS_CLOSING, -1);
    }

    if (ret)
        ws_handle_rwerr_close(stream);

    return ret;
}

/* Wait for a TLS/SSL stream to initiate a TLS/SSL handshake
 *
 * On fatal error, the connection is shut down.
 * If data still needs to be read/written, -1 is returned.
 * On success, the TLS/SSL connection is completed and 0 is returned */
static int
handle_accept_ssl(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    int ret = -1, err = 0;

    // clock_gettime(CLOCK_MONOTONIC, &ext->last_live_ts);

    /* all good on TLS handshake */
    if ((ret = SSL_accept(ext->ssl)) > 0) {
        ext->sslstatus &= ~(WS_TLS_ACCEPTING | WS_TLS_WANT_RW);

        /* Now wait for the handshake request from client. */
        ext->on_readable = ws_handle_handshake_request;
        ext->status = WS_WAITING4HSREQU;
        PC_INFO("SSL Accepted: %d %s\n", stream->fd4r, stream->peer_addr);
        return 0;
    }

    err = SSL_get_error(ext->ssl, ret);
    log_return_message_ssl(ret, err, "SSL_accept");

    switch (err) {
    case SSL_ERROR_WANT_READ:
        ext->sslstatus = WS_TLS_WANT_READ | WS_TLS_ACCEPTING;
        ret = 0;
        break;
    case SSL_ERROR_WANT_WRITE:
        ext->sslstatus = WS_TLS_WANT_WRITE | WS_TLS_ACCEPTING;
        ret = 0;
        break;
    case SSL_ERROR_SYSCALL:
        /* Wait for more activity else bail out, for instance if
           the socket is closed during the handshake. */
        if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK ||
                    errno == EINTR)) {
            ext->sslstatus = WS_TLS_WANT_RW | WS_TLS_ACCEPTING;
            ret = 0;
            break;
        }
        /* The peer notified that it is shutting down through
           a SSL "close_notify" so we shutdown too */
        // fallthrough
    case SSL_ERROR_ZERO_RETURN:
    case SSL_ERROR_WANT_X509_LOOKUP:
    default:
        ext->sslstatus &= ~WS_TLS_ACCEPTING;
        ret = ws_set_status(ext, WS_ERR_SSL | WS_CLOSING, -1);
    }

    if (ret)
        ws_handle_rwerr_close(stream);

    return ret;
}

/* Given the current status of the SSL buffer, perform that action.
 *
 * On error or if no SSL pending status, 1 is returned.
 * On success, the TLS/SSL pending action is called and 0 is returned */
static int
handle_pending_rw_ssl(struct pcdvobjs_stream *stream, int rwflag)
{
    struct stream_extended_data *ext = stream->ext0.data;

    if (ext->sslstatus & WS_TLS_CONNECTING && ext->sslstatus & rwflag) {
        PC_INFO("SSL still in connecting.\n");
        handle_connect_ssl(stream);
        return 0;
    }
    if (ext->sslstatus & WS_TLS_ACCEPTING && ext->sslstatus & rwflag) {
        PC_INFO("SSL still in accepting.\n");
        handle_accept_ssl(stream);
        return 0;
    }

    /* trying to read but still waiting for a successful SSL_read */
    if (ext->sslstatus & WS_TLS_READING) {
        ws_handle_reads(stream);
        return 0;
    }
    /* trying to write but still waiting for a successful SSL_write */
    if (ext->sslstatus & WS_TLS_WRITING) {
        ws_handle_writes(stream);
        return 0;
    }
    /* trying to write but still waiting for a successful SSL_shutdown */
    if (ext->sslstatus & WS_TLS_SHUTTING) {
        if (shutdown_ssl(stream) == 0)
            ws_handle_rwerr_close(stream);
        return 0;
    }

    return 1;
}

/* Write bytes to a TLS/SSL connection for a given stream.
 *
 * On error or if no write is performed -1 is returned.
 * On success, the number of bytes actually written to the TLS/SSL
 * connection are returned */
static ssize_t
write_socket_ssl(struct pcdvobjs_stream *stream, const void *buffer, size_t len)
{
    struct stream_extended_data *ext = stream->ext0.data;
    size_t wrotten = 0;
    int ret, err = 0;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    ERR_clear_error();
#endif

    if ((ret = SSL_write_ex(ext->ssl, buffer, len, &wrotten)) > 0)
        return wrotten;

    err = SSL_get_error(ext->ssl, ret);
    log_return_message_ssl(ret, err, "SSL_write");

    switch (err) {
        case SSL_ERROR_WANT_WRITE:
            break;
        case SSL_ERROR_WANT_READ:
            ext->sslstatus = WS_TLS_WRITING;
            break;
        case SSL_ERROR_SYSCALL:
            if ((ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK ||
                            errno == EINTR)))
                break;
            /* The connection was shut down cleanly */
            // fallthrough
        case SSL_ERROR_ZERO_RETURN:
        case SSL_ERROR_WANT_X509_LOOKUP:
        default:
            return ws_set_status(ext, WS_ERR_SSL | WS_CLOSING, -1);
    }

    return wrotten;
}

/* Read data from a TLS/SSL connection for a given stream and set a connection
 * status given the return value of SSL_read().
 *
 * On error, -1 is returned and the connection status is set.
 * On success, the number of bytes read is returned. */
static ssize_t
read_socket_ssl(struct pcdvobjs_stream *stream, void *buffer, size_t size)
{
    struct stream_extended_data *ext = stream->ext0.data;
    size_t read_bytes = 0;
    int ret, done = 0, err = 0;

    do {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
        ERR_clear_error ();
#endif

        done = 0;
        if ((ret = SSL_read_ex(ext->ssl, buffer, size, &read_bytes)) > 0)
            break;

        err = SSL_get_error(ext->ssl, ret);
        log_return_message_ssl(ret, err, "SSL_read");

        switch (err) {
            case SSL_ERROR_WANT_WRITE:
                ext->sslstatus = WS_TLS_READING;
                done = 1;
                break;
            case SSL_ERROR_WANT_READ:
                PC_DEBUG("More data need to read\n");
                done = 1;
                break;
            case SSL_ERROR_SYSCALL:
                if ((ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK ||
                                errno == EINTR)))
                    break;
                // fallthrough
            case SSL_ERROR_ZERO_RETURN:
            case SSL_ERROR_WANT_X509_LOOKUP:
            default:
                return ws_set_status(ext, WS_ERR_SSL | WS_CLOSING, -1);
        }
    } while (SSL_pending(ext->ssl) && !done);

    return read_bytes;
}

#endif // HAVE(OPENSSL)

/* Clear pending data. */
static void ws_clear_pending_data(struct stream_extended_data *ext)
{
    struct list_head *p, *n;

    list_for_each_safe(p, n, &ext->pending) {
        list_del(p);
        free(p);
    }

    ext->sz_pending = 0;
    ws_update_mem_stats(ext);
}

static void cleanup_extension(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;

    if (stream->ext0.data) {
        if (ext->ping_timer) {
            pcintr_timer_stop(ext->ping_timer);
            pcintr_timer_destroy(ext->ping_timer);
            ext->ping_timer = NULL;
        }

#if 0
        purc_atom_t target = ext->event_cids[K_EVENT_TYPE_CLOSE];
        if (target != 0) {
            pcintr_coroutine_post_event(target,
                PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
                EVENT_TYPE_CLOSE, NULL,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        }
#endif

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

        if (ext->prot_opts) {
            purc_variant_unref(ext->prot_opts);
            ext->prot_opts = PURC_VARIANT_INVALID;
        }

        if (ext->ws_key) {
            free(ext->ws_key);
            ext->ws_key = NULL;
        }

        if (ext->hsbuf) {
            free(ext->hsbuf);
            ext->hsbuf = NULL;
        }

        ws_clear_pending_data(ext);
        if (ext->message)
            free(ext->message);

#if HAVE(OPENSSL)
        if (ext->ssl) {
            if (ext->sslstatus)
                shutdown_ssl(stream);
            SSL_free(ext->ssl);
            ext->ssl = NULL;
        }

        if (ext->ssl_shctx_wrapper) {
            openssl_shctx_detach(ext->ssl_shctx_wrapper);
            free(ext->ssl_shctx_wrapper);
            ext->ssl_shctx_wrapper = NULL;
        }

        if (ext->ssl_ctx) {
            SSL_CTX_free(ext->ssl_ctx);
            ext->ssl_ctx = NULL;
        }
#endif

        free(stream->ext0.data);
        stream->ext0.data = NULL;

        free(stream->ext0.msg_ops);
        stream->ext0.msg_ops = NULL;
    }

}

/*
 * Queue new data.
 *
 * On success, true is returned.
 * On error, false is returned and the connection status is set.
 */
static bool ws_queue_data(struct pcdvobjs_stream *stream,
        const char *buf, size_t len)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ws_pending_data *pending_data;

    if ((pending_data = malloc(sizeof(ws_pending_data) + len)) == NULL) {
        ws_clear_pending_data(ext);
        ext->status = WS_ERR_OOM | WS_CLOSING;
        return false;
    }

    memcpy(pending_data->data, buf, len);
    pending_data->szdata = len;
    pending_data->szsent = 0;

    list_add_tail(&pending_data->list, &ext->pending);
    ext->sz_pending += len;
    ws_update_mem_stats(ext);
    ext->status |= WS_SENDING;

    /* the connection probably is too slow, so stop queueing until everything
     * is sent */
    if (ext->sz_pending >= SOCK_THROTTLE_THLD) {
        ext->status |= WS_THROTTLING;
    }

    return true;
}

/*
 * Write data to the socket without SSL.
 *
 * Returns the number of bytes wrotten to the socket;
 * 0 for no any bytes wrotten, -1 for failure.
 */
static inline ssize_t
write_socket_plain(struct pcdvobjs_stream *stream, const void *buf, size_t len)
{
    ssize_t bytes;

    while ((bytes = write(stream->fd4r, buf, len)) == -1 && errno == EINTR);

    if (bytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        /* Reset bytes to be 0 */
        bytes = 0;
    }
    else if (bytes == -1) {
        struct stream_extended_data *ext = stream->ext0.data;
        ext->status = WS_ERR_IO | WS_CLOSING;
    }

    /* Return bytes for other situations. */
    return bytes;

}

/*
 * Read data from the socket without SSL.
 *
 * Returns the number of bytes read from the socket;
 * 0 for no any bytes read, -1 for failure.
 */
static inline ssize_t
read_socket_plain(struct pcdvobjs_stream *stream, void *buf, size_t len)
{
    ssize_t bytes;

    while ((bytes = read(stream->fd4r, buf, len)) == -1 && errno == EINTR);

    if (bytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        /* Reset bytes to be 0 */
        return 0;
    }
    else if (bytes == -1) {
        struct stream_extended_data *ext = stream->ext0.data;
        ext->status = WS_ERR_IO | WS_CLOSING;
    }

    /* Return bytes for other situations. */
    return bytes;
}

/*
 * A wrapper of low-level reader.
 *
 *  - 0: there is no data on the socket.
 *  - > 0: the number of bytes read from the socket.
 *  - -1: for errors.
 */
static ssize_t ws_read_data(struct pcdvobjs_stream *stream,
        void *buff, size_t sz)
{
    struct stream_extended_data *ext = stream->ext0.data;
    return ext->reader(stream, buff, sz);
}

/*
 * A wrapper of low-level writer.
 *
 * On error, -1 is returned and the connection status is set.
 * On success, the number of bytes sent is returned.
 */
static ssize_t ws_write_data(struct pcdvobjs_stream *stream,
        const char *buffer, size_t len)
{
    struct stream_extended_data *ext = stream->ext0.data;

    ssize_t bytes = ext->writer(stream, buffer, len);
    if (bytes > 0 && (size_t)bytes < len) {
        /* did not send all of it... buffer it for a later attempt */
        ws_queue_data(stream, buffer + bytes, len - bytes);
    }

    return bytes;
}

/*
 * Send the queued up data to the given socket.
 *
 * On error, -1 is returned and the connection status is set.
 * On success, the number of bytes sent is returned (maybe zero).
 */
static ssize_t ws_write_pending(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ssize_t total_bytes = 0;
    struct list_head *p, *n;

    list_for_each_safe(p, n, &ext->pending) {
        ssize_t bytes;
        ws_pending_data *pending = (ws_pending_data *)p;

        bytes = ext->writer(stream, pending->data + pending->szsent,
                pending->szdata - pending->szsent);

        if (bytes > 0) {
            pending->szsent += bytes;
            if (pending->szsent >= pending->szdata) {
                list_del(p);
                free(p);
            }
            else {
                break;
            }

            total_bytes += bytes;
            ext->sz_pending -= bytes;
            ws_update_mem_stats(ext);
        }
        else if (bytes == -1) {
            goto failed;
        }
    }

    return total_bytes;

failed:
    return -1;
}

/*
 * Write data in buffer or queue it.
 *
 * On error, -1 is returned and the connection status is set as error.
 * On success, the number of bytes sent is returned.
 */
static ssize_t ws_write_or_queue(struct pcdvobjs_stream *stream,
        const void *buffer, size_t len)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ssize_t bytes = 0;

    /* attempt to send the whole buffer */
    if (list_empty(&ext->pending)) {
        bytes = ws_write_data(stream, buffer, len);
    }
    /* the pending list not empty, just append new data if we're not
     * throttling the connection */
    else if (ext->sz_pending < SOCK_THROTTLE_THLD) {
        if (ws_queue_data(stream, buffer, len))
            return bytes;
    }
    /* send from pending buffer */
    else {
        bytes = ws_write_pending(stream);
    }

    return bytes;
}

static int ws_send_data_frame(struct pcdvobjs_stream *stream, int fin,
        int opcode, const void *data, ssize_t sz)
{
    struct stream_extended_data *ext = stream->ext0.data;
    int ret = 0;
    int mask_int = 0;
    char *buf = NULL;
    char *p = NULL;
    size_t sz_buf = 0;
    ws_frame_header header;
    unsigned char mask[4] = { 0 };
    int sz_mask;

    if (sz <= 0) {
        PC_WARN("Invalid data size %zd.\n", sz);
        goto out;
    }

    if (opcode == WS_OPCODE_TEXT)
        PC_DEBUG("Sending data frame: `%s` (%zu)\n", (const char*)data, sz);

    header.fin = fin;
    header.rsv = 0;
    header.op = opcode;
    if (ext->role == WS_ROLE_CLIENT) {
        header.mask = 1; /* client must be 1 */

        srandom(time(NULL));
        mask_int = random();
        memcpy(mask, &mask_int, 4);
        sz_mask = 4;
    }
    else {
        header.mask = 0; /* server must be 0 */
        sz_mask = 0;
    }

    if (sz > 0xffff) {
        /* header(16b) + Extended payload length(64b) + mask(32b) + data */
        sz_buf = 2 + 8 + sz_mask + sz;
        header.sz_payload = 127;
    }
    else if (sz > 125) {
        /* header(16b) + Extended payload length(16b) + mask(32b) + data */
        sz_buf = 2 + 2 + sz_mask + sz;
        header.sz_payload = 126;
    }
    else {
        /* header(16b) + data + mask(32b) */
        sz_buf = 2 + sz_mask + sz;
        header.sz_payload = sz;
    }

    buf = malloc(sz_buf + 1);
    buf[0] = 0;
    if (fin) {
        buf[0] |= 0x80;
    }
    buf[0] |= (0xff & opcode);
    buf[1] = (header.mask << 7);
    buf[1] |= header.sz_payload;

    p = buf + 2;
    if (header.sz_payload == 127) {
        uint64_t v = htobe64(sz);
        memcpy(p, &v, 8);
        p = p + 8;
    }
    else if (header.sz_payload == 126) {
        uint16_t v = htobe16(sz);
        memcpy(p, &v, 2);
        p = p + 2;
    }

    PC_DEBUG("Frame info: fin: %x, rsv: %x, op: %x, mask: %x, sz: %zd\n",
            header.fin, header.rsv, header.op, header.mask,
            header.sz_payload);
    if (sz_mask) {
        /* mask */
        memcpy(p, &mask, 4);
        p = p + sz_mask;
    }

    /* payload */
    memcpy(p, data, sz);

    if (sz_mask) {
        /* mask payload */
        for (ssize_t i = 0; i < sz; i++) {
            p[i] ^= mask[i % 4] & 0xff;
        }
    }

    if (ws_write_or_queue(stream, buf, sz_buf) < 0)
        ret = -1;

out:
    if (buf) {
        free(buf);
    }
    return ret;
}

/*
 * Send a control frame with the payload length less than 126.
 */
static int ws_send_ctrl_frame(struct pcdvobjs_stream *stream, int opcode,
        const char *payload, size_t sz_payload)
{
    struct stream_extended_data *ext = stream->ext0.data;
    char buf[2 + 4 + 126];
    int sz_mask;
    int mask_int;
    const uint8_t *mask = (uint8_t *)&mask_int;

    if (payload != NULL && sz_payload > 125) {
        PC_WARN("Too long payload for a control frame: %zu; truncated\n",
                sz_payload);
        sz_payload = 0;     // force to ignore.
    }

    buf[0] = 0x80 | (uint8_t)opcode;
    if (ext->role == WS_ROLE_CLIENT) {
        srandom(time(NULL));
        mask_int = random();
        memcpy(buf + 2, &mask_int, 4);
        buf[1] = 0x80 | (uint8_t)sz_payload;
        sz_mask = 4;
    }
    else {
        // no mask
        buf[1] = (sz_payload & 0x7f);
        sz_mask = 0;
    }

    if (sz_payload) {
        char *p = buf + 2;
        if (sz_mask) {
            /* mask */
            memcpy(p, &mask_int, 4);
            p += sz_mask;

            /* mask payload */
            for (size_t i = 0; i < sz_payload; i++) {
                p[i] = payload[i] ^ (mask[i % 4] & 0xff);
            }
        }
        else {
            memcpy(p, payload, sz_payload);
        }
    }

    if (ws_write_or_queue(stream, buf, 2 + sz_mask + sz_payload) < 0) {
        return -1;
    }

    return 0;
}

/*
 * Send a CLOSE message to the peer.
 *
 * return zero on success; none-zero on error.
 */
static int ws_notify_to_close(struct pcdvobjs_stream *stream,
        uint16_t err_code, const char *err_msg)
{
    unsigned int len;
    unsigned short code_be;
    char buf[128] = { 0 };

    len = 2;
    code_be = htobe16(err_code);
    memcpy(buf, &code_be, 2);
    if (err_msg)
        len += snprintf(buf + 2, sizeof(buf) - 4, "%s", err_msg);

    return ws_send_ctrl_frame(stream, WS_OPCODE_CLOSE, buf, len);
}

/* Tries to read a frame header. */
static int try_to_read_frame_header(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ws_frame_header *header = &ext->header;
    char *buf = ext->header_buf;
    ssize_t n;

    assert(ext->sz_header > ext->sz_read_header);

    n = ws_read_data(stream, buf + ext->sz_read_header,
            ext->sz_header - ext->sz_read_header);
    if (n > 0) {
        ext->sz_read_header += n;
        if (ext->sz_read_header == ext->sz_header) {
            ext->sz_read_header = 0;

            header->fin = buf[0] & 0x80 ? 1 : 0;
            header->rsv = buf[0] & 0x70;
            header->op = buf[0] & 0x0F;
            header->mask = buf[1] & 0x80;
            header->sz_payload = buf[1] & 0x7F;

            if (header->mask) {
                ext->sz_mask = 4;
                ext->sz_read_mask = 0;
            }
            else {
                ext->sz_mask = 0;
                ext->sz_read_mask = 0;
            }

            PC_DEBUG("Frame info: fin: %x, rsv: %x, op: %x, mask: %x, sz: %zd\n",
                    header->fin, header->rsv, header->op, header->mask,
                    header->sz_payload);

            switch (header->sz_payload) {
            case 127:
                memset(ext->ext_paylen_buf, 0, sizeof(ext->ext_paylen_buf));
                ext->sz_ext_paylen = (uint8_t)sizeof(uint64_t);
                ext->sz_read_ext_paylen = 0;
                break;
            case 126:
                memset(ext->ext_paylen_buf, 0, sizeof(ext->ext_paylen_buf));
                ext->sz_ext_paylen = (uint8_t)sizeof(uint16_t);
                ext->sz_read_ext_paylen = 0;
                break;
            default:
                ext->sz_ext_paylen = 0;
                ext->sz_payload = header->sz_payload;
                ext->payload = malloc(ext->sz_payload + 1);
                if (ext->payload == NULL) {
                    PC_ERROR("Failed to allocate memory for payload (%zu)\n",
                            ext->sz_payload);
                    ws_notify_to_close(stream, WS_CLOSE_UNEXPECTED,
                            "Out of memory");
                    ext->status = WS_ERR_OOM | WS_CLOSING;
                    return READ_ERROR;
                }
                ext->sz_read_payload = 0;
                break;
            }
            return READ_WHOLE;
        }
        ext->status |= WS_READING;
    }
    else if (n < 0) {
        PC_ERROR("Failed to read frame header from WebSocket: %s\n",
                strerror(errno));
        ws_notify_to_close(stream, WS_CLOSE_UNEXPECTED,
                "Unable to read header");
        return READ_ERROR;
    }
    else {
        /* no data */
        ext->status |= WS_READING;
        return READ_NONE;
    }

    return READ_SOME;
}

static int try_to_read_ext_payload_length(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ws_frame_header *header = &ext->header;
    ssize_t n;

    char *buf = (char *)ext->ext_paylen_buf;
    PC_DEBUG("sz_ext_paylen: %d, sz_read_ext_paylen: %d\n",
            (int)ext->sz_ext_paylen, (int)ext->sz_read_ext_paylen);
    assert(ext->sz_ext_paylen > ext->sz_read_ext_paylen);

    n = ws_read_data(stream, buf + ext->sz_read_ext_paylen,
            ext->sz_ext_paylen - ext->sz_read_ext_paylen);
    if (n > 0) {
        ext->sz_read_ext_paylen += n;
        if (ext->sz_read_ext_paylen == ext->sz_ext_paylen) {
            ext->sz_read_ext_paylen = 0;
            if (ext->sz_ext_paylen == sizeof(uint16_t)) {
                uint16_t v;
                memcpy(&v, ext->ext_paylen_buf, 2);
                header->sz_payload = be16toh(v);
            }
            else if (ext->sz_ext_paylen == sizeof(uint64_t)) {
                uint64_t v;
                memcpy(&v, ext->ext_paylen_buf, 8);
                header->sz_payload = be64toh(v);
            }
            else {
                // never be here.
                assert(0);
            }

            if (header->sz_payload > ext->maxmessagesize) {
                ws_notify_to_close(stream, WS_CLOSE_TOO_LARGE,
                        "Frame is too big");
                ext->status = WS_ERR_MSG | WS_CLOSING;
                return READ_ERROR;
            }

            return READ_WHOLE;
        }
        ext->status |= WS_READING;
    }
    else if (n < 0) {
        PC_ERROR("Failed to read ext frame payload length from WebSocket\n");
        ws_notify_to_close(stream, WS_CLOSE_UNEXPECTED,
                "Unable to read ext payload length");
        return READ_ERROR;
    }
    else {
        /* no data */
        ext->status |= WS_READING;
        return READ_NONE;
    }
    return READ_SOME;
}

static int try_to_read_mask(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ssize_t n;

    char *buf = (char *)ext->mask;
    assert(ext->sz_mask > ext->sz_read_mask);

    n = ws_read_data(stream, buf + ext->sz_read_mask,
            ext->sz_mask - ext->sz_read_mask);
    if (n > 0) {
        ext->sz_read_mask += n;
        if (ext->sz_read_mask == ext->sz_mask) {
            return READ_WHOLE;
        }
        ext->status |= WS_READING;
    }
    else if (n < 0) {
        PC_ERROR("Failed to read frame mask from WebSocket.\n");
        ws_notify_to_close(stream, WS_CLOSE_UNEXPECTED,
                "Unable to read mask");
        return READ_ERROR;
    }
    else {
        /* no data */
        ext->status |= WS_READING;
        return READ_NONE;
    }
    return READ_SOME;
}

/* Unmask the payload given the current frame's masking key. */
static void
ws_unmask_payload(char *buf, size_t len, size_t offset,
        const unsigned char mask[])
{
    size_t i, j = 0;

    /* unmask data */
    for (i = offset; i < len; ++i, ++j) {
        buf[i] ^= mask[j % 4];
    }
}

static int try_to_read_payload(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ssize_t n;

    char *buf = (char *)ext->payload;
    PC_DEBUG("sz_payload: %zu, sz_read_payload: %zu\n",
            ext->sz_payload, ext->sz_read_payload);
    assert(ext->sz_payload >= ext->sz_read_payload);

    n = ws_read_data(stream, buf + ext->sz_read_payload,
            ext->sz_payload - ext->sz_read_payload);
    if (n > 0) {
        if (ext->sz_mask) {
            ws_unmask_payload(buf, n, ext->sz_read_payload, ext->mask);
        }

        ext->sz_read_payload += n;
        if (ext->sz_read_payload == ext->sz_payload) {
            ext->sz_read_payload = 0;
            return READ_WHOLE;
        }
        ext->status |= WS_READING;
    }
    else if (n < 0) {
        PC_ERROR("Failed to read frame payload from WebSocket: %s\n",
                strerror(errno));
        ws_notify_to_close(stream, WS_CLOSE_UNEXPECTED,
                "Unable to read payload");
        return READ_ERROR;
    }
    else {
        /* no data */
        ext->status |= WS_READING;
        return READ_NONE;
    }
    return READ_SOME;
}

/*
 * Tries to read a frame payload. */
static int try_to_read_frame_payload(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    ws_frame_header *header = &ext->header;
    int retv;

    /* read extended payload length */
    if (ext->sz_ext_paylen != 0) {
        retv = try_to_read_ext_payload_length(stream);
        if (retv != READ_WHOLE) {
            return retv;
        }

        ext->sz_payload = ext->header.sz_payload;
        ext->payload = malloc(ext->sz_payload + 1);
        if (ext->payload == NULL) {
            PC_ERROR("Failed to allocate memory for payload (%zu)\n",
                    ext->sz_payload);
            ws_notify_to_close(stream, WS_CLOSE_UNEXPECTED, "Out of memory.");
            ext->status |= WS_CLOSING;
            return READ_ERROR;
        }

        ext->sz_read_payload = 0;
    }

    /* read mask */
    if (header->mask && ext->sz_read_mask < ext->sz_mask) {
        retv = try_to_read_mask(stream);
        if (retv != READ_WHOLE) {
            return retv;
        }
    }

    /* read websocket payload */
    return ext->sz_payload ? try_to_read_payload(stream) : 0;
}

static int ws_validate_ctrl_frame(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    /* RFC states that Control frames themselves MUST NOT be fragmented. */
    if (!ext->header.fin) {
        goto failed;
    }

    /* Control frames are only allowed to have payload up to and
     * including 125 octets */
    if (ext->header.sz_payload > 125) {
        goto failed;
    }

    return 0;

failed:
    ws_notify_to_close(stream, WS_CLOSE_PROTO_ERR, NULL);
    ext->status = WS_ERR_MSG | WS_CLOSING;
    return -1;
}

static int ws_handle_reads(struct pcdvobjs_stream *stream)
{
#if HAVE(OPENSSL)
    if (handle_pending_rw_ssl(stream, WS_TLS_WANT_READ) == 0)
        return 0;
#endif

    struct stream_extended_data *ext = stream->ext0.data;
    int retv;

    clock_gettime(CLOCK_MONOTONIC, &ext->last_live_ts);

    do {
        if (ext->status & WS_CLOSING) {
            goto failed;
        }

        if (!(ext->status & WS_WAITING4PAYLOAD)) {
            retv = try_to_read_frame_header(stream);
            if (retv == READ_NONE) {
                break;
            }
            else if (retv == READ_SOME) {
                continue;
            }
            else if (retv == READ_ERROR) {
                ext->status |= WS_CLOSING;
                goto failed;
            }

            switch (ext->header.op) {
            case WS_OPCODE_PING:
                if (ws_validate_ctrl_frame(stream))
                    goto failed;

                ext->msg_type = MT_PING;
                ext->status |= WS_WAITING4PAYLOAD;
                break;

            case WS_OPCODE_PONG:
                if (ws_validate_ctrl_frame(stream))
                    goto failed;

                ext->msg_type = MT_PONG;
                ext->status |= WS_WAITING4PAYLOAD;
                break;

            case WS_OPCODE_CLOSE:
                if (ws_validate_ctrl_frame(stream))
                    goto failed;
                ext->msg_type = MT_CLOSE;
                ext->status |= WS_WAITING4PAYLOAD;
                break;

            case WS_OPCODE_TEXT:
                ext->msg_type = MT_TEXT;
                ext->status |= WS_WAITING4PAYLOAD;
                break;

            case WS_OPCODE_BIN:
                ext->msg_type = MT_BINARY;
                ext->status |= WS_WAITING4PAYLOAD;
                break;

            case WS_OPCODE_CONTINUATION:
                ext->status |= WS_WAITING4PAYLOAD;
                break;

            default:
                PC_ERROR("Unknown frame opcode: %d\n", ext->header.op);
                ws_notify_to_close(stream, WS_CLOSE_UNEXPECTED,
                        "Unknown frame opcode");
                ext->status = WS_ERR_MSG | WS_CLOSING;
                goto failed;
                break;
            }

            PC_INFO("Got a frame header: %d\n", ext->header.op);
        }
        else if (ext->status & WS_WAITING4PAYLOAD) {
            retv = try_to_read_frame_payload(stream);
            if (retv == READ_NONE) {
                break;
            }
            else if (retv == READ_SOME) {
                continue;
            }
            else if (retv == READ_ERROR) {
                ext->status |= WS_CLOSING;
                goto failed;
            }
            else if (retv == READ_WHOLE) {
                if (!ext->message) {
                    ext->sz_message = ext->sz_payload;
                    ext->message = malloc(ext->sz_message + 1);
                    ext->sz_read_message = 0;
                }
                else {
                    ext->sz_message += ext->sz_payload;
                    ext->message = realloc(ext->message, ext->sz_message + 1);
                }

                if (ext->message == NULL) {
                    PC_ERROR("failed to allocate memory for payload (%zu)\n",
                            ext->sz_message);
                    ws_notify_to_close(stream, WS_CLOSE_UNEXPECTED,
                            "Out of memory");
                    ext->status |= WS_CLOSING;
                    goto failed;
                }

                memcpy(ext->message + ext->sz_read_message, ext->payload,
                        ext->sz_payload);
                ext->sz_read_message += ext->sz_payload;
                free(ext->payload);

                ext->payload = NULL;
                ext->sz_payload = 0;
                ext->sz_read_payload = 0;

                if (ext->header.fin == 0) {
                    ext->status &= ~WS_WAITING4PAYLOAD;
                    continue;
                }

                /* whole message */
                switch (ext->header.op) {
                case WS_OPCODE_PING:
                    retv = stream->ext0.msg_ops->on_message(stream, MT_PING,
                            NULL, 0);
                    break;

                case WS_OPCODE_PONG:
                    retv = stream->ext0.msg_ops->on_message(stream, MT_PONG,
                            NULL, 0);
                    break;

                case WS_OPCODE_CLOSE:
                    /* TODO: payload of CLOSE message */
                    retv = stream->ext0.msg_ops->on_message(stream, MT_CLOSE,
                            NULL, 0);
                    ext->status = WS_CLOSING;
                    break;

                case WS_OPCODE_TEXT:
                    if (!pcutils_string_check_utf8_len(ext->message,
                            ext->sz_message, NULL, NULL)) {
                        PC_ERROR("Got an invalid UTF-8 text message: %s (%zu).\n",
                                ext->message, ext->sz_message);
                        ws_notify_to_close(stream, WS_CLOSE_INVALID_UTF8, NULL);
                        ext->status = WS_ERR_MSG | WS_CLOSING;
                        goto failed;
                    }

                    retv = stream->ext0.msg_ops->on_message(stream,
                            ext->msg_type, ext->message, ext->sz_message);
                    free(ext->message);
                    ext->message = NULL;
                    ext->sz_message = 0;
                    ext->sz_read_payload = 0;
                    ext->sz_read_message = 0;
                    ws_update_mem_stats(ext);
                    break;

                case WS_OPCODE_BIN:
                    retv = stream->ext0.msg_ops->on_message(stream,
                            ext->msg_type, ext->message, ext->sz_message);
                    free(ext->message);
                    ext->message = NULL;
                    ext->sz_message = 0;
                    ext->sz_read_payload = 0;
                    ext->sz_read_message = 0;
                    ws_update_mem_stats(ext);
                    break;

                default:
                    /* never reach here */
                    PC_ERROR("Unknown frame opcode: %d\n", ext->header.op);
                    ext->status = WS_ERR_MSG | WS_CLOSING;
                    goto failed;
                    break;
                }
                break;
            }
        }
    } while (true);

    return 0;

failed:
    ws_handle_rwerr_close(stream);
    return -1;
}

static bool
io_callback_for_read(int fd, int event, void *ctxt)
{
    struct pcdvobjs_stream *stream = ctxt;
    struct stream_extended_data *ext = stream->ext0.data;

    if ((event & PCRUNLOOP_IO_HUP) && ext->event_cids[K_EVENT_TYPE_ERROR]) {
        PC_ERROR("Got hang up event on fd (%d).\n", fd);
        stream->ext0.msg_ops->on_error(stream, PURC_ERROR_BROKEN_PIPE);
        return false;
    }

    if ((event & PCRUNLOOP_IO_ERR) && ext->event_cids[K_EVENT_TYPE_ERROR]) {
        PC_ERROR("Got error event on fd (%d).\n", fd);
        stream->ext0.msg_ops->on_error(stream, PCRDR_ERROR_UNEXPECTED);
        return false;
    }

    assert(ext->on_readable);
    return ext->on_readable(stream) == 0;
}

static int
ws_handle_writes(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;

#if HAVE(OPENSSL)
    if (handle_pending_rw_ssl(stream, WS_TLS_WANT_WRITE) == 0)
        return 0;
#endif

    if (ext->status & WS_CLOSING) {
        ws_handle_rwerr_close(stream);
        return -1;
    }

    ws_write_pending(stream);
    if (list_empty(&ext->pending)) {
        ext->status &= ~WS_SENDING;
    }

    return 0;
}

static bool
io_callback_for_write(int fd, int event, void *ctxt)
{
    struct pcdvobjs_stream *stream = ctxt;
    struct stream_extended_data *ext = stream->ext0.data;

    if ((event & PCRUNLOOP_IO_HUP) && ext->event_cids[K_EVENT_TYPE_ERROR]) {
        PC_ERROR("Got hang up event on fd (%d).\n", fd);
        stream->ext0.msg_ops->on_error(stream, PURC_ERROR_BROKEN_PIPE);
        return false;
    }

    if ((event & PCRUNLOOP_IO_ERR) && ext->event_cids[K_EVENT_TYPE_ERROR]) {
        PC_ERROR("Got error event on fd (%d).\n", fd);
        stream->ext0.msg_ops->on_error(stream, PCRDR_ERROR_UNEXPECTED);
        return false;
    }

    assert(ext->on_writable);
    return (ext->on_writable(stream) == 0);
}

/*
 * Send a PING message to the peer.
 *
 * return zero on success; none-zero on error.
 */
static inline int ws_ping_peer(struct pcdvobjs_stream *stream)
{
    return ws_send_ctrl_frame(stream, WS_OPCODE_PING, NULL, 0);
}

/*
 * Send a PONG message to the peer.
 *
 * return zero on success; none-zero on error.
 */
static inline int ws_pong_peer(struct pcdvobjs_stream *stream)
{
    return ws_send_ctrl_frame(stream, WS_OPCODE_PONG, NULL, 0);
}

static void mark_closing(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;
    if (ext->sz_pending == 0) {
        ws_notify_to_close(stream, WS_CLOSE_NORMAL, NULL);
    }

    ext->status = WS_CLOSING;
}

static int ws_can_send_data(struct stream_extended_data *ext, size_t sz)
{
    if (sz > MAX_FRAME_PAYLOAD_SIZE) {
        size_t frames = sz / MAX_FRAME_PAYLOAD_SIZE + 1;
        if (ext->sz_pending + sz + (frames * ext->sz_header) >=
                SOCK_THROTTLE_THLD) {
            goto failed;
        }
    }
    else {
        if (ext->sz_pending + sz + ext->sz_header >= SOCK_THROTTLE_THLD) {
            goto failed;
        }
    }

    return 0;

failed:
    return -1;
}

/*
 * Send a message
 *
 * return zero on success; none-zero on error.
 */
static int send_message(struct pcdvobjs_stream *stream,
        bool text_or_binary, const char *data, size_t sz)
{
    struct stream_extended_data *ext = stream->ext0.data;

    if (ext == NULL) {
        return PURC_ERROR_ENTITY_GONE;
    }

#if HAVE(OPENSSL)
    if ((ext->sslstatus & WS_TLS_ACCEPTING) ||
            (ext->sslstatus & WS_TLS_CONNECTING)) {
        return PURC_ERROR_NOT_READY;
    }
#endif

    if (sz > ext->maxmessagesize) {
        return PURC_ERROR_TOO_LARGE_ENTITY;
    }

    if ((ext->status & WS_THROTTLING) || ws_can_send_data(ext, sz)) {
        return PURC_ERROR_AGAIN;
    }

    ext->status = WS_OK;

    if (sz > MAX_FRAME_PAYLOAD_SIZE) {
        unsigned int left = sz;
        int fin;
        int opcode;
        size_t sz_payload;

        do {
            if (left == sz) {
                fin = 0;
                opcode = text_or_binary ? WS_OPCODE_TEXT : WS_OPCODE_BIN;
                sz_payload = PCRDR_MAX_FRAME_PAYLOAD_SIZE;
                left -= PCRDR_MAX_FRAME_PAYLOAD_SIZE;
            }
            else if (left > PCRDR_MAX_FRAME_PAYLOAD_SIZE) {
                fin = 0;
                opcode = WS_OPCODE_CONTINUATION;
                sz_payload = PCRDR_MAX_FRAME_PAYLOAD_SIZE;
                left -= PCRDR_MAX_FRAME_PAYLOAD_SIZE;
            }
            else {
                fin = 1;
                opcode = WS_OPCODE_CONTINUATION;
                sz_payload = left;
                left = 0;
            }

            ws_send_data_frame(stream, fin, opcode, data, sz_payload);
            data += sz_payload;
        } while (left > 0);
    }
    else {
        ws_send_data_frame(stream, 1,
                text_or_binary ? WS_OPCODE_TEXT : WS_OPCODE_BIN, data, sz);
    }

    if (ext->status & WS_ERR_ANY) {
        PC_ERROR("Error when sending data: %s\n", strerror(errno));
        return ws_status_to_pcerr(ext);
    }

    return PURC_ERROR_OK;
}

static int on_error(struct pcdvobjs_stream *stream, int errcode)
{
    struct stream_extended_data *ext = stream->ext0.data;

    purc_atom_t target = ext->event_cids[K_EVENT_TYPE_ERROR];
    if (target == 0)
        goto done;

    purc_variant_t data = purc_variant_make_object_0();
    if (data) {
        purc_variant_t tmp;

        tmp = purc_variant_make_number(errcode);
        if (tmp) {
            purc_variant_object_set_by_static_ckey(data, "errCode", tmp);
            purc_variant_unref(tmp);
        }

        tmp = purc_variant_make_string_static(
            purc_get_error_message(errcode), false);
        if (tmp) {
            purc_variant_object_set_by_static_ckey(data, "errMsg", tmp);
            purc_variant_unref(tmp);
        }

        pcintr_coroutine_post_event(target,
                PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
                EVENT_TYPE_ERROR, NULL,
                data, PURC_VARIANT_INVALID);

        purc_variant_unref(data);
    }

done:
    return 0;
}

static void on_ping_timer(pcintr_timer_t timer, const char *id, void *data)
{
    (void)id;
    (void)timer;

    struct pcdvobjs_stream *stream = data;
    struct stream_extended_data *ext = stream->ext0.data;

    assert(timer == ext->ping_timer);

    double elapsed = purc_get_elapsed_seconds(&ext->last_live_ts, NULL);
    PC_DEBUG("ping timer elapsed: %f\n", elapsed);

    if (elapsed > ext->noresptimetoclose) {
        if (ext->on_readable == ws_handle_reads) {
            ws_notify_to_close(stream, WS_CLOSE_GOING_AWAY, NULL);
        }
        ext->status = WS_ERR_LTNR | WS_CLOSING;
        ws_handle_rwerr_close(stream);
    }
    else if (elapsed > ext->noresptimetoping) {
        if (ext->on_readable == ws_handle_reads) {
            ws_ping_peer(stream);
        }
    }
}

static void ws_start_ping_timer(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext0.data;

    assert(ext->ping_timer == NULL);

    clock_gettime(CLOCK_MONOTONIC, &ext->last_live_ts);

    purc_runloop_t runloop = purc_runloop_get_current();
    if (runloop) {
        ext->ping_timer = pcintr_timer_create(runloop, NULL,
            on_ping_timer, stream);
        if (ext->ping_timer == NULL) {
            PC_WARN("Failed to create PING timer\n");
        }
        else {
            uint32_t interval = ext->noresptimetoping / 3;
            if (interval < MIN_PING_TIMER_INTERVAL)
                interval = MIN_PING_TIMER_INTERVAL;
            pcintr_timer_set_interval(ext->ping_timer, interval);
            pcintr_timer_start(ext->ping_timer);
        }
    }
}

static int on_message(struct pcdvobjs_stream *stream, int type,
        const char *buf, size_t len)
{
    int retv = 0;
    struct stream_extended_data *ext = stream->ext0.data;

    purc_atom_t target = ext->event_cids[K_EVENT_TYPE_MESSAGE];
    purc_variant_t data = PURC_VARIANT_INVALID;
    const char *event = EVENT_TYPE_MESSAGE;
    switch (type) {
        case MT_TEXT:
            // fire a `message` event
            data = purc_variant_make_string_ex(buf, len, false);
            break;

        case MT_BINARY:
            // fire a `message` event
            data = purc_variant_make_byte_sequence(buf, len);
            break;

        case MT_PING:
            retv = ws_pong_peer(stream);
            break;

        case MT_PONG:
            // TODO: update the alive timestamp
            break;

        case MT_CLOSE:
            // fire a `close` event
            if (buf) {
                data = purc_variant_make_string_ex(buf, len, false);
            }
            else {
                data = purc_variant_make_string_static("Peer closed", false);
            }
            event = EVENT_TYPE_CLOSE;
            target = ext->event_cids[K_EVENT_TYPE_CLOSE];
            break;
    }

    if (data) {
        PC_DEBUG("Fire event: `%s`\n", event);
        if (target)
            pcintr_coroutine_post_event(target,
                    PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
                    event, NULL,
                    data, PURC_VARIANT_INVALID);
        purc_variant_unref(data);
    }

    return retv;
}

/* Serer-only method */
static purc_variant_t
send_handshake_resp(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    struct pcdvobjs_stream *stream = entity;
    struct stream_extended_data *ext = stream->ext0.data;
    struct pcutils_mystring mystr;
    ssize_t nr_bytes;

#if HAVE(OPENSSL)
    if ((ext->sslstatus & WS_TLS_ACCEPTING) ||
            (ext->sslstatus & WS_TLS_CONNECTING)) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }
#endif

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    int status;
    if (!purc_variant_cast_to_int32(argv[0], &status, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    const char *response = NULL;
    switch (status) {
    case 101:
        break;
    case 400:
        response = WS_BAD_REQUEST_STR;
        break;
    case 503:
        response = WS_TOO_BUSY_STR;
        break;
    case 505:
    default:
        response = WS_INTERNAL_ERROR_STR;
        break;
    }

    size_t len;
    if (response) {
        len = strlen(response);
        goto done;
    }

    char accept[SHA_DIGEST_LEN * 4 + 1];
    ws_key_to_accept_encoded(ext->ws_key, accept, sizeof(accept));

    pcutils_mystring_init(&mystr);

    pcutils_mystring_append_string(&mystr, WS_SWITCH_PROTO_STR);
    pcutils_mystring_append_string(&mystr, CRLF);
    pcutils_mystring_append_string(&mystr, "Upgrade: ");
    pcutils_mystring_append_string(&mystr, "websocket");
    pcutils_mystring_append_string(&mystr, CRLF);

    pcutils_mystring_append_string(&mystr, "Connection: ");
    pcutils_mystring_append_string(&mystr, "upgrade");
    pcutils_mystring_append_string(&mystr, CRLF);

    pcutils_mystring_append_string(&mystr, "Sec-WebSocket-Accept: ");
    pcutils_mystring_append_string(&mystr, accept);

    const char *subprot = NULL;
    if (nr_args > 1 && (subprot = purc_variant_get_string_const(argv[1]))) {
        pcutils_mystring_append_string(&mystr, CRLF);
        pcutils_mystring_append_string(&mystr, "Sec-WebSocket-Protocol: ");
        pcutils_mystring_append_string(&mystr, subprot);
    }

    const char *exts = NULL;
    if (nr_args > 2 && (exts = purc_variant_get_string_const(argv[2]))) {
        pcutils_mystring_append_string(&mystr, CRLF);
        pcutils_mystring_append_string(&mystr, "Sec-WebSocket-Extensions: ");
        pcutils_mystring_append_string(&mystr, exts);
    }

    pcutils_mystring_append_string(&mystr, CRLF CRLF);
    pcutils_mystring_done(&mystr);
    response = mystr.buff;
    len = mystr.nr_bytes - 1;

done:
    /* Send the handshake response to the client */
    nr_bytes = ws_write_data(stream, response, len);
    pcutils_mystring_free(&mystr);

    if (nr_bytes < 0) {
        purc_set_error(PURC_ERROR_IO_FAILURE);
        goto failed;
    }

    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_boolean(false);
    }

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
send_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    struct pcdvobjs_stream *stream = entity;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    bool text_or_binary = true;
    const void *data = NULL;
    size_t len;
    if (purc_variant_is_string(argv[0])) {
        data = purc_variant_get_string_const_ex(argv[0], &len);
    }
    else if (purc_variant_is_bsequence(argv[0])) {
        text_or_binary = false;
        data = purc_variant_get_bytes_const(argv[0], &len);
    }

    if (data == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    int retv;
    if ((retv = send_message(stream, text_or_binary, data, len))) {
        purc_set_error(retv);
        goto failed;
    }

    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_boolean(false);
    }

    return PURC_VARIANT_INVALID;
}

static purc_variant_t
close_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    struct pcdvobjs_stream *stream = entity;
    struct stream_extended_data *ext = stream->ext0.data;

    if (ext == NULL) {
        purc_set_error(PURC_ERROR_ENTITY_GONE);
        goto failed;
    }

    ws_notify_to_close(stream, WS_CLOSE_NORMAL, "Bye");
    cleanup_extension(stream);
    return purc_variant_make_boolean(true);

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY) {
        return purc_variant_make_boolean(false);
    }

    return PURC_VARIANT_INVALID;
}

static purc_nvariant_method
property_getter(void *entity, const char *name)
{
    struct pcdvobjs_stream *stream = entity;
    purc_nvariant_method method = NULL;

    assert(name);

    struct stream_extended_data *ext = stream->ext0.data;
    if (strcmp(name, "send") == 0) {
        method = send_getter;
    }
    else if (strcmp(name, "close") == 0) {
        method = close_getter;
    }
    else if ((ext->role == WS_ROLE_SERVER ||
                ext->role == WS_ROLE_SERVER_WORKER) &&
            strcmp(name, "send_handshake_resp") == 0) {
        method = send_handshake_resp;
    }
    else {
        const struct purc_native_ops *super_ops = stream->ext0.super_ops;
        if (super_ops->property_getter)
            method = super_ops->property_getter(entity, name);

        if (method == NULL)
            goto failed;
    }

    /* override the getters of parent */
    return method;

failed:
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

static const char *websocket_events[] = {
#define EVENT_MASK_HANDSHAKE    (0x01 << K_EVENT_TYPE_HANDSHAKE)
    EVENT_TYPE_HANDSHAKE,
#define EVENT_MASK_MESSAGE      (0x01 << K_EVENT_TYPE_MESSAGE)
    EVENT_TYPE_MESSAGE,
#define EVENT_MASK_ERROR        (0x01 << K_EVENT_TYPE_ERROR)
    EVENT_TYPE_ERROR,
#define EVENT_MASK_CLOSE        (0x01 << K_EVENT_TYPE_CLOSE)
    EVENT_TYPE_CLOSE,
};

static bool on_observe(void *entity, const char *event_name,
        const char *event_subname)
{
    struct pcdvobjs_stream *stream = (struct pcdvobjs_stream*)entity;
    pcintr_coroutine_t co = pcintr_get_coroutine();
    if (co == NULL)
        return false;

    int matched = pcdvobjs_match_events(event_name, event_subname,
            websocket_events, PCA_TABLESIZE(websocket_events));
    if (matched == -1)
        return false;

    struct stream_extended_data *ext = stream->ext0.data;
    if ((matched & EVENT_MASK_HANDSHAKE)) {
        ext->event_cids[K_EVENT_TYPE_HANDSHAKE] = co->cid;
    }
    if ((matched & EVENT_MASK_MESSAGE)) {
        ext->event_cids[K_EVENT_TYPE_MESSAGE] = co->cid;
    }
    if ((matched & EVENT_MASK_ERROR)) {
        ext->event_cids[K_EVENT_TYPE_ERROR] = co->cid;
    }
    if ((matched & EVENT_MASK_CLOSE)) {
        ext->event_cids[K_EVENT_TYPE_CLOSE] = co->cid;
    }

    return true;
}

static bool on_forget(void *entity, const char *event_name,
        const char *event_subname)
{
    struct pcdvobjs_stream *stream = (struct pcdvobjs_stream*)entity;
    assert(stream);

    pcintr_coroutine_t co = pcintr_get_coroutine();
    if (co == NULL)
        return false;

    int matched = pcdvobjs_match_events(event_name, event_subname,
            websocket_events, PCA_TABLESIZE(websocket_events));
    if (matched == -1)
        return false;

    struct stream_extended_data *ext = stream->ext0.data;
    if (ext) {
        if ((matched & EVENT_MASK_HANDSHAKE)) {
            ext->event_cids[K_EVENT_TYPE_HANDSHAKE] = 0;
        }
        if ((matched & EVENT_MASK_MESSAGE)) {
            ext->event_cids[K_EVENT_TYPE_MESSAGE] = 0;
        }
        if ((matched & EVENT_MASK_ERROR)) {
            ext->event_cids[K_EVENT_TYPE_ERROR] = 0;
        }
        if ((matched & EVENT_MASK_CLOSE)) {
            ext->event_cids[K_EVENT_TYPE_CLOSE] = 0;
        }
    }

    return true;
}

static void on_release(void *entity)
{
    struct pcdvobjs_stream *stream = entity;
    const struct purc_native_ops *super_ops = stream->ext0.super_ops;

    cleanup_extension(stream);

    if (super_ops->on_release) {
        return super_ops->on_release(entity);
    }
}

static const struct purc_native_ops msg_entity_ops = {
    .property_getter = property_getter,
    .on_observe = on_observe,
    .on_forget = on_forget,
    .on_release = on_release,
};

const struct purc_native_ops *
dvobjs_extend_stream_by_websocket(struct pcdvobjs_stream *stream,
        const struct purc_native_ops *super_ops, purc_variant_t extra_opts)
{
    struct stream_extended_data *ext = NULL;
    struct stream_messaging_ops *msg_ops = NULL;

    if (super_ops == NULL || stream->ext0.signature[0]) {
        PC_ERROR("This stream has already extended by a Layer 0: %s\n",
                stream->ext0.signature);
        purc_set_error(PURC_ERROR_CONFLICT);
        goto failed;
    }

    if (stream->socket == NULL && extra_opts == PURC_VARIANT_INVALID) {
        PC_ERROR("No any WebSocket options given.\n");
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }
    else if (extra_opts != PURC_VARIANT_INVALID &&
            !purc_variant_is_object(extra_opts)) {
        PC_ERROR("Not an object for websocket options.\n");
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    purc_variant_t tmp;
    tmp = purc_variant_object_get_by_ckey(extra_opts, "maxmessagesize");
    uint64_t maxmessagesize = 0;
    if (tmp && !purc_variant_cast_to_ulongint(tmp, &maxmessagesize, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    tmp = purc_variant_object_get_by_ckey(extra_opts, "noresptimetoping");
    uint32_t noresptimetoping = 0;
    if (tmp && !purc_variant_cast_to_uint32(tmp, &noresptimetoping, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    tmp = purc_variant_object_get_by_ckey(extra_opts, "noresptimetoclose");
    uint32_t noresptimetoclose = 0;
    if (tmp && !purc_variant_cast_to_uint32(tmp, &noresptimetoclose, false)) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    /* Override the socket option to be have O_NONBLOCK */
    if (fcntl(stream->fd4r, F_SETFL,
                fcntl(stream->fd4r, F_GETFL, 0) | O_NONBLOCK) == -1) {
        PC_ERROR("Unable to set socket as non-blocking: %s.", strerror(errno));
        purc_set_error(PURC_ERROR_BAD_SYSTEM_CALL);
        goto failed;
    }

    ext = calloc(1, sizeof(*ext));
    if (ext == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    if (maxmessagesize == 0)
        ext->maxmessagesize = MAX_INMEM_MESSAGE_SIZE;
    else if (maxmessagesize <= MAX_FRAME_PAYLOAD_SIZE)
        ext->maxmessagesize = MAX_FRAME_PAYLOAD_SIZE;
    else
        ext->maxmessagesize = maxmessagesize;

    if (noresptimetoping == 0)
        ext->noresptimetoping = NO_RESPONSE_TIME_TO_PING;
    else if (noresptimetoping < MIN_NO_RESPONSE_TIME_TO_PING)
        ext->noresptimetoping = MIN_NO_RESPONSE_TIME_TO_PING;
    else
        ext->noresptimetoping = noresptimetoping;

    if (noresptimetoclose == 0)
        ext->noresptimetoclose = NO_RESPONSE_TIME_TO_CLOSE;
    else if (noresptimetoclose < MIN_NO_RESPONSE_TIME_TO_CLOSE)
        ext->noresptimetoclose = MIN_NO_RESPONSE_TIME_TO_CLOSE;
    else
        ext->noresptimetoclose = noresptimetoclose;

    PC_DEBUG("Configuration: maxmessagesize(%zu/%llu), noresptimetoping(%u/%u), "
            "noresptimetoclose(%u/%u)\n",
            ext->maxmessagesize, maxmessagesize,
            ext->noresptimetoping, noresptimetoping,
            ext->noresptimetoclose, noresptimetoclose);

    list_head_init(&ext->pending);
    ext->sz_header = sizeof(ext->header_buf);
    memset(ext->header_buf, 0, ext->sz_header);

    strcpy(stream->ext0.signature, STREAM_EXT_SIG_MSG);

    msg_ops = calloc(1, sizeof(*msg_ops));
    if (msg_ops) {
        msg_ops->send_message = send_message;
        msg_ops->on_error = on_error;
        msg_ops->mark_closing = mark_closing;

        msg_ops->on_message = on_message;
        msg_ops->cleanup = cleanup_extension;

        stream->ext0.data = ext;
        stream->ext0.super_ops = super_ops;
        stream->ext0.msg_ops = msg_ops;
    }
    else {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    pcintr_coroutine_t co = pcintr_get_coroutine();
    if (co) {
        stream->monitor4r = purc_runloop_add_fd_monitor(
                purc_runloop_get_current(), stream->fd4r,
                PCRUNLOOP_IO_IN | PCRUNLOOP_IO_HUP | PCRUNLOOP_IO_ERR,
                io_callback_for_read, stream);
        if (stream->monitor4r) {
            stream->cid = co->cid;
        }
        else {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }

        stream->monitor4w = purc_runloop_add_fd_monitor(
                purc_runloop_get_current(), stream->fd4w,
                // macOS not allow to poll HUP and OUT at the same time.
                PCRUNLOOP_IO_OUT | PCRUNLOOP_IO_ERR,
                io_callback_for_write, stream);
        if (stream->monitor4w) {
            stream->cid = co->cid;
        }
        else {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }

    }

    /* destroy rwstreams */
    if (stream->stm4r) {
        purc_rwstream_destroy(stream->stm4r);
    }

    if (stream->stm4w && stream->stm4w != stream->stm4r) {
        purc_rwstream_destroy(stream->stm4w);
    }

    stream->stm4w = NULL;
    stream->stm4r = NULL;

    if (stream->socket == NULL) {
        bool secure = false;
        tmp = purc_variant_object_get_by_ckey(extra_opts, "secure");
        secure = tmp == PURC_VARIANT_INVALID ? false :
            purc_variant_booleanize(tmp);

        if (secure) {
#if HAVE(OPENSSL)
            ext->ssl_ctx = SSL_CTX_new(SSLv23_client_method());
            if (ext->ssl_ctx == NULL) {
                PC_ERROR("Failed SSL_CTX_new(): %s\n",
                        ERR_error_string(ERR_get_error(), NULL));
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                goto failed;
            }

            if (!(ext->ssl = SSL_new(ext->ssl_ctx))) {
                PC_ERROR("Failed SSL_new(): %s.\n",
                        ERR_error_string(ERR_get_error(), NULL));
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                goto failed;
            }

            if (SSL_set_fd(ext->ssl, stream->fd4r) != 1) {
                PC_ERROR("Failed SSL_set_fd(): %s.\n",
                        ERR_error_string(ERR_get_error(), NULL));
                purc_set_error(PURC_ERROR_BAD_STDC_CALL);
                goto failed;
            }

            tmp = purc_variant_object_get_by_ckey(extra_opts,
                    "ssl-session-cache-id");
            const char *ssl_session_cache_id = (tmp == NULL) ? NULL :
                purc_variant_get_string_const(tmp);

            if (ssl_session_cache_id) {
                /* This is a server-side worker process. */

                ext->ssl_shctx_wrapper = calloc(1,
                        sizeof(*ext->ssl_shctx_wrapper));
                if (openssl_shctx_attach(ext->ssl_shctx_wrapper,
                            ssl_session_cache_id, ext->ssl_ctx)) {
                    PC_ERROR("Failed openssl_shctx_attach(): %s.\n",
                            strerror(errno));
                    free(ext->ssl_shctx_wrapper);
                    ext->ssl_shctx_wrapper = NULL;
                    purc_set_error(purc_error_from_errno(errno));
                    goto failed;
                }
            }
            else {
                /* This is a client process. */
                ext->prot_opts = purc_variant_ref(extra_opts);
                if (handle_connect_ssl(stream) != 0) {
                    PC_ERROR("Failed SSL_connect(): %s.\n",
                            ERR_error_string(ERR_get_error(), NULL));
                    purc_set_error(PURC_ERROR_CONNECTION_ABORTED);
                    goto failed;
                }
            }
#else
            PC_ERROR("`secure` is true, but OpenSSL not enabled.\n");
            purc_set_error(PURC_ERROR_NOT_SUPPORTED);
            goto failed;
#endif

            ext->reader = read_socket_ssl;
            ext->writer = write_socket_ssl;
        }
        else {
            ext->reader = read_socket_plain;
            ext->writer = write_socket_plain;
        }

        tmp = purc_variant_object_get_by_ckey(extra_opts, "handshake");
        if (tmp) {
            /* this is a server-side worker process. */
            if (purc_variant_booleanize(tmp)) {
                ext->role = WS_ROLE_SERVER_WORKER_WOHS;
                ext->on_readable = ws_handle_handshake_request;
                ext->on_writable = ws_handle_writes;
                ext->status = WS_WAITING4HSREQU;
            }
            else {
                ext->role = WS_ROLE_SERVER_WORKER;
                ext->on_readable = ws_handle_reads;
                ext->on_writable = ws_handle_writes;
            }

        }
        else {
            ext->role = WS_ROLE_CLIENT;
            // this is a client; do handshake first if ext->prot_opts not set.
            if (ext->prot_opts == NULL &&
                    ws_client_handshake(stream, extra_opts) != 0) {
                PC_ERROR("Failed ws_client_handshake()\n");
                goto failed;
            }

            ext->status |= WS_WAITING4HSRESP;

#if HAVE(OPENSSL)
            if (ext->prot_opts) {
                ext->on_readable = handle_connect_ssl;
                // ext->sslstatus = WS_TLS_CONNECTING | WS_TLS_WANT_RW;
            }
            else
#endif
                ext->on_readable = ws_handle_handshake_response;
            ext->on_writable = ws_handle_writes;
        }

    }
    else {
        /* this is the server */
        ext->role = WS_ROLE_SERVER;
#if HAVE(OPENSSL)
        if (stream->socket->ssl_ctx) {
            if (!(ext->ssl = SSL_new(stream->socket->ssl_ctx))) {
                PC_ERROR("Failed SSL_new(): %s.\n",
                        ERR_error_string(ERR_get_error(), NULL));
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                goto failed;
            }

            if (SSL_set_fd(ext->ssl, stream->fd4r) != 1) {
                PC_ERROR("Failed SSL_set_fd(): %s.\n",
                        ERR_error_string(ERR_get_error(), NULL));
                purc_set_error(PURC_ERROR_BAD_STDC_CALL);
                goto failed;
            }

            ext->reader = read_socket_ssl;
            ext->writer = write_socket_ssl;
            ext->on_readable = handle_accept_ssl;
            ext->on_writable = ws_handle_writes;
            ext->sslstatus = WS_TLS_ACCEPTING | WS_TLS_WANT_RW;
        }
        else {
            ext->reader = read_socket_plain;
            ext->writer = write_socket_plain;
            ext->on_readable = ws_handle_handshake_request;
            ext->on_writable = ws_handle_writes;
            ext->status = WS_WAITING4HSREQU;
        }
#else
        ext->reader = read_socket_plain;
        ext->writer = write_socket_plain;
        ext->on_readable = ws_handle_handshake_request;
        ext->on_writable = ws_handle_writes;
        ext->status = WS_WAITING4HSREQU;
#endif
    }

    ws_start_ping_timer(stream);

    PC_INFO("This stream is extended by Layer 0 prot: websocket; role(%d)\n",
            ext->role);

    assert(ext->reader);
    assert(ext->writer);
    assert(ext->on_readable);
    assert(ext->on_writable);

    return &msg_entity_ops;

failed:
    cleanup_extension(stream);
    return NULL;
}

