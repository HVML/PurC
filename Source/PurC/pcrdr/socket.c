/*
 * socket.c -- The implementation of socket method for PURCMC protocol.
 *
 * Copyright (c) 2021, 2022, 2023 FMSoft (http://www.fmsoft.cn)
 *
 * Authors:
 *  Vincent Wei (https://github.com/VincentWei), 2021, 2022, 2023
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
#include "purc-pcrdr.h"
#include "private/list.h"
#include "private/debug.h"
#include "private/utils.h"
#include "purc-utils.h"
#include "connect.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <assert.h>

#if OS(LINUX) || OS(UNIX)

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netdb.h>

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

#define CLI_PATH    "/var/tmp/"
#define CLI_PERM    S_IRWXU

#define WS_MAGIC_STR        "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_KEY_LEN          16
#define SHA_DIGEST_LEN      20

/* The frame operation codes for UnixSocket */
typedef enum USOpcode_ {
    US_OPCODE_CONTINUATION = 0x00,
    US_OPCODE_TEXT = 0x01,
    US_OPCODE_BIN = 0x02,
    US_OPCODE_END = 0x03,
    US_OPCODE_CLOSE = 0x08,
    US_OPCODE_PING = 0x09,
    US_OPCODE_PONG = 0x0A,
} USOpcode;

/* The frame header for UnixSocket */
typedef struct USFrameHeader_ {
    int op;
    unsigned int fragmented;
    unsigned int sz_payload;
    unsigned char payload[0];
} USFrameHeader;

/* The frame operation codes for WebSocket */
typedef enum WSOpcode_ {
    WS_OPCODE_CONTINUATION = 0x00,
    WS_OPCODE_TEXT = 0x01,
    WS_OPCODE_BIN = 0x02,
    WS_OPCODE_END = 0x03,
    WS_OPCODE_CLOSE = 0x08,
    WS_OPCODE_PING = 0x09,
    WS_OPCODE_PONG = 0x0A,
} WSOpcode;

/* The frame header for WebSocket */
typedef struct WSFrameHeader_ {
    unsigned int fin;
    unsigned int rsv;
    unsigned int op;
    unsigned int mask;
    unsigned int sz_payload;
} WSFrameHeader;

/* packet body types */
enum {
    PT_TEXT = 0,
    PT_BINARY,
};

static inline int conn_read (int fd, void *buff, ssize_t sz)
{
    if (read (fd, buff, sz) == sz) {
        return 0;
    }

    return PCRDR_ERROR_IO;
}

static inline int conn_write (int fd, const void *data, ssize_t sz)
{
    if (write (fd, data, sz) == sz) {
        return 0;
    }

    return PCRDR_ERROR_IO;
}

static ssize_t ws_write(int fd, const void *buf, size_t length)
{
    /* TODO : ssl support */
    return send(fd, buf, length, MSG_NOSIGNAL);
}

static ssize_t ws_read(int fd, void *buf, size_t length)
{
    /* TODO : ssl support */
    return recv(fd, buf, length, 0);
}

static ssize_t ws_conn_read(pcrdr_conn *conn, void *buf, size_t length)
{
    ssize_t nr_result = 0;

    if (conn->sticky) {
        size_t nr_last = 0;
        nr_last = conn->nr_sticky - (conn->sticky_pos - conn->sticky);
        if (nr_last > length) {
            memcpy(buf, conn->sticky_pos, length);
            conn->sticky_pos += length;
            return length;
        }

        memcpy(buf, conn->sticky_pos, nr_last);

        free (conn->sticky);
        conn->sticky = NULL;
        conn->sticky_pos = NULL;
        conn->nr_sticky = 0;

        if (nr_last == length) {
            return length;
        }

        nr_result = nr_last;
    }

    if ((size_t)nr_result == length) {
        return nr_result;
    }

    char *p = (char *)buf + nr_result;
    ssize_t nr_read = 0;
    size_t nr_len = length - nr_result;
    while ((nr_read = ws_read(conn->fd, p, nr_len)) != 0) {
        nr_result += nr_read;
        if ((size_t)nr_read == nr_len) {
            break;
        }
        nr_len = length - nr_result;
        p += nr_read;
    }

    return nr_result;
}

static int ws_send_ctrl_frame(int fd, char code)
{
    char data[6];
    int mask_int;

    srand(time(NULL));
    mask_int = rand();
    memcpy(data + 2, &mask_int, 4);

    data[0] = 0x80 | code;
    data[1] = 0x80;

    if (6 != ws_write(fd, data, 6)) {
        return -1;
    }
    return 0;
}

static int ws_send_data_frame(int fd, int fin, int opcode,
        const void *data, ssize_t sz)
{
    int ret = PCRDR_ERROR_IO;
    int mask_int = 0;
    size_t size = sz;
    char *buf = NULL;
    char *p = NULL;
    size_t nr_buf = 0;
    WSFrameHeader header;
    unsigned char mask[4] = { 0 };

    if (sz <= 0) {
        PC_DEBUG ("Invalid data size %ld.\n", sz);
        goto out;
    }

    header.fin = fin;
    header.rsv = 0;
    header.op = opcode;
    header.mask = 1; /* client must 1 */

    srand(time(NULL));
    mask_int = rand();
    memcpy(mask, &mask_int, 4);

    size = sz;
    if (size > 0xffff) {
        /* header(16b) + Extended payload length(64b) + mask(32b) + data */
        nr_buf = 2 + 8 + 4 + sz;
        header.sz_payload = 127;
    }
    else if (size > 125) {
        /* header(16b) + Extended payload length(16b) + mask(32b) + data */
        nr_buf = 2 + 2 + 4 + sz;
        header.sz_payload = 126;
    }
    else {
        /* header(16b) + data + mask(32b) */
        nr_buf = 2 + 4 + sz;
        header.sz_payload = sz;
    }

    buf = malloc(nr_buf + 1);
    buf[0] = 0;
    buf[1] = 0;
    if (fin) {
        buf[0] |= 0x80;
    }
    buf[0] |= (0xff & opcode);
    buf[1] = 0x80 | header.sz_payload;

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

    /* mask */
    memcpy(p, &mask, 4);

    /* payload */
    p = p + 4;
    memcpy(p, data, sz);

    /* mask payload */
    for (ssize_t i = 0; i < sz; i++) {
        p[i] ^= mask[i % 4] & 0xff;
    }

    if (ws_write(fd, buf, nr_buf) == (ssize_t)nr_buf) {
        ret = 0;
    }

out:
    if (buf) {
        free(buf);
    }
    return ret;
}

static int ws_read_data_frame(pcrdr_conn *conn, WSFrameHeader *header,
        char **packet_buf, unsigned int *packet_len)
{
    int ret = -1;
    unsigned char mask[4] = { 0 };
    char *payload = NULL;
    size_t nr_payload = 0;

    if (header->sz_payload == 127) {
        uint64_t v = 0;
        if (ws_conn_read(conn, &v, sizeof(v)) != sizeof(v)) {
            PC_DEBUG ("read websocket extended payload length failed.\n");
            goto out;
        }
        nr_payload = be64toh(v);
    }
    else if (header->sz_payload == 126) {
        uint16_t v = 0;
        if (ws_conn_read(conn, &v, sizeof(v)) != sizeof(v)) {
            PC_DEBUG ("read websocket extended payload length failed.\n");
            goto out;
        }
        nr_payload = be16toh(v);
    }
    else {
        nr_payload = header->sz_payload;
    }

    /* Server to Client may be 0 */
    if (header->mask) {
        if (ws_conn_read(conn, mask, sizeof(mask)) != sizeof(mask)) {
            PC_DEBUG ("read websocket mask failed.\n");
            goto out;
        }
    }

    if (nr_payload == 0) {
        goto succ;
    }

    payload = malloc(nr_payload + 1);
    if (payload == NULL) {
        PC_DEBUG ("Failed to allocate memory for payload.\n");
        goto out;
    }

    if (ws_conn_read(conn, payload, nr_payload) != (ssize_t)nr_payload) {
        PC_DEBUG ("read websocket payload failed.\n");
        goto out;
    }

    /* Server to Client may be 0 */
    /* unmask data */
    if (header->mask) {
        for (size_t i = 0, j = 0; i < nr_payload; ++i, ++j) {
            payload[i] ^= mask[j % 4];
        }
    }

succ:
    *packet_buf = payload;
    *packet_len = nr_payload;
    ret = 0;
    return ret;

out:
    if (ret != 0 && payload) {
        free(payload);
    }
    return ret;
}

static int ws_read_frame_header(pcrdr_conn *conn, WSFrameHeader *header)
{
    char buf[2] = { 0 };
    ssize_t n = ws_conn_read(conn, &buf, 2);
    if (n != 2) {
        return -1;
    }

    header->fin = buf[0] & 0x80 ? 1 : 0;
    header->rsv = buf[0] & 0x70;
    header->op = buf[0] & 0x0F;
    header->mask = buf[1] & 0x80;
    header->sz_payload = buf[1] & 0x7F;

    return 0;
}

static int ws_close(int fd)
{
    return ws_send_ctrl_frame(fd, WS_OPCODE_CLOSE);
}

static int ws_ping(int fd)
{
    return ws_send_ctrl_frame(fd, WS_OPCODE_PING);
}

int ws_pong(int fd)
{
    return ws_send_ctrl_frame(fd, WS_OPCODE_PONG);
}

static int my_wait_message (pcrdr_conn* conn, int timeout_ms)
{
    fd_set rfds;
    struct timeval tv;

    FD_ZERO (&rfds);
    FD_SET (conn->fd, &rfds);

    if (timeout_ms >= 0) {
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        return select (conn->fd + 1, &rfds, NULL, NULL, &tv);
    }

    return select (conn->fd + 1, &rfds, NULL, NULL, NULL);
}

static pcrdr_msg *my_read_message (pcrdr_conn* conn)
{
    void* packet;
    size_t data_len;
    pcrdr_msg* msg = NULL;
    int err_code = 0, retval;

    retval = pcrdr_socket_read_packet_alloc (conn, &packet, &data_len);
    if (retval) {
        PC_DEBUG ("Failed to read packet\n");
        goto done;
    }

    if (data_len == 0) { // no data
        msg = pcrdr_make_void_message();
        goto done;
    }

    conn->stats.bytes_recv += data_len;
    retval = pcrdr_parse_packet (packet, data_len, &msg);
    free (packet);

    if (retval < 0) {
        err_code = PCRDR_ERROR_BAD_MESSAGE;
    }

done:
    if (err_code) {
        purc_set_error (err_code);

        if (msg) {
            pcrdr_release_message (msg);
            msg = NULL;
        }

    }

    return msg;
}

static int my_send_message (pcrdr_conn* conn, pcrdr_msg *msg)
{
    int retv = -1;
    purc_rwstream_t buffer = NULL;

    buffer = purc_rwstream_new_buffer (PCRDR_MIN_PACKET_BUFF_SIZE,
            PCRDR_MAX_INMEM_PAYLOAD_SIZE);

    if (pcrdr_serialize_message (msg,
                (pcrdr_cb_write)purc_rwstream_write, buffer) < 0) {
        goto done;
    }

    size_t packet_len;
    const char * packet = purc_rwstream_get_mem_buffer (buffer, &packet_len);

    if (pcrdr_socket_send_text_packet (conn, packet, packet_len) < 0) {
        goto done;
    }

    conn->stats.bytes_sent += packet_len;
    retv = 0;

done:
    if (buffer) {
        purc_rwstream_destroy(buffer);
    }

    return retv;
}

static int my_ping_peer (pcrdr_conn* conn)
{
    int err_code = 0;

    if (conn->type == CT_UNIX_SOCKET) {
        USFrameHeader header;

        header.op = US_OPCODE_PING;
        header.fragmented = 0;
        header.sz_payload = 0;
        if (conn_write (conn->fd, &header, sizeof (USFrameHeader))) {
            err_code = PCRDR_ERROR_IO;
        }
    }
    else if (conn->type == CT_INET_SOCKET) {
        if (ws_ping(conn->fd) != 0) {
            PC_DEBUG ("Error when ping WebSocket: %s\n", strerror (errno));
            err_code = PCRDR_ERROR_IO;
        }
    }
    else {
        err_code = PCRDR_ERROR_INVALID_VALUE;
    }

    if (err_code) {
        purc_set_error (err_code);
        err_code = -1;
    }

    return err_code;
}

static int my_disconnect (pcrdr_conn* conn)
{
    int err_code = 0;

    if (conn->type == CT_UNIX_SOCKET) {
        USFrameHeader header;

        header.op = US_OPCODE_CLOSE;
        header.fragmented = 0;
        header.sz_payload = 0;
        if (conn_write (conn->fd, &header, sizeof (USFrameHeader))) {
            PC_DEBUG ("Error when wirting to Unix Socket: %s\n", strerror (errno));
            err_code = PCRDR_ERROR_IO;
        }
    }
    else if (conn->type == CT_INET_SOCKET) {
        if (ws_close(conn->fd) != 0) {
            PC_DEBUG ("Error when close WebSocket: %s\n", strerror (errno));
            err_code = PCRDR_ERROR_IO;
        }
    }
    else {
        err_code = PCRDR_ERROR_INVALID_VALUE;
    }

    close (conn->fd);

    return err_code;
}

/* returns fd if all OK, -1 on error */
static int purcmc_connect_via_unix_socket (const char* path_to_socket,
        const char* app_name, const char* runner_name, pcrdr_conn** conn)
{
    int fd, len, err_code = PCRDR_ERROR_BAD_CONNECTION;
    struct sockaddr_un unix_addr;
    char peer_name [33];

    if (!purc_is_valid_app_name(app_name) ||
            !purc_is_valid_runner_name(runner_name)) {
        purc_set_error(PURC_EXCEPT_INVALID_VALUE);
        return -1;
    }

    if ((*conn = calloc (1, sizeof (pcrdr_conn))) == NULL) {
        PC_DEBUG ("Failed to callocate space for connection: %s\n",
                strerror (errno));
        purc_set_error(PCRDR_ERROR_NOMEM);
        return -1;
    }

    /* create a Unix domain stream socket */
    if ((fd = socket (AF_UNIX, SOCK_STREAM, 0)) < 0) {
        PC_DEBUG ("Failed to call `socket` in %s: %s\n", __func__,
                strerror (errno));
        purc_set_error(PCRDR_ERROR_IO);
        return -1;
    }

    {
        pcutils_md5_ctxt ctx;
        unsigned char md5_digest[16];

        pcutils_md5_begin (&ctx);
        pcutils_md5_hash (&ctx, app_name, strlen (app_name));
        pcutils_md5_hash (&ctx, "/", 1);
        pcutils_md5_hash (&ctx, runner_name, strlen (runner_name));
        pcutils_md5_end (&ctx, md5_digest);
        pcutils_bin2hex (md5_digest, 16, peer_name, false);
    }

    /* fill socket address structure w/our address */
    memset (&unix_addr, 0, sizeof(unix_addr));
    unix_addr.sun_family = AF_UNIX;
    /* On Linux sun_path is 108 bytes in size */
    snprintf (unix_addr.sun_path, sizeof (unix_addr.sun_path),
            "%s%s-%05d", CLI_PATH, peer_name, getpid());
    len = sizeof (unix_addr.sun_family) + strlen (unix_addr.sun_path) + 1;

    unlink (unix_addr.sun_path);        /* in case it already exists */
    if (bind (fd, (struct sockaddr *) &unix_addr, len) < 0) {
        PC_WARN ("Failed to call `bind` in %s: %s\n", __func__,
                strerror (errno));
        goto error;
    }
    if (chmod (unix_addr.sun_path, CLI_PERM) < 0) {
        PC_WARN ("Failed to call `chmod` in %s: %s\n", __func__,
                strerror (errno));
        goto error;
    }

    /* fill socket address structure w/server's addr */
    memset (&unix_addr, 0, sizeof(unix_addr));
    unix_addr.sun_family = AF_UNIX;
    strcpy (unix_addr.sun_path, path_to_socket);
    len = sizeof (unix_addr.sun_family) + strlen (unix_addr.sun_path) + 1;

    if (connect (fd, (struct sockaddr *) &unix_addr, len) < 0) {
        PC_WARN ("Failed to call `connect` in %s: %s\n", __func__,
                strerror (errno));
        goto error;
    }

    (*conn)->prot = PURC_RDRCOMM_SOCKET;
    (*conn)->type = CT_UNIX_SOCKET;
    (*conn)->fd = fd;
    (*conn)->timeout_ms = 10;   /* 10 milliseconds */
    (*conn)->srv_host_name = NULL;
    (*conn)->own_host_name = strdup (PCRDR_LOCALHOST);
    (*conn)->app_name = app_name;
    (*conn)->runner_name = runner_name;

    (*conn)->wait_message = my_wait_message;
    (*conn)->read_message = my_read_message;
    (*conn)->send_message = my_send_message;
    (*conn)->ping_peer = my_ping_peer;
    (*conn)->disconnect = my_disconnect;

    list_head_init (&(*conn)->pending_requests);
    return fd;

error:
    close (fd);

    if ((*conn)->own_host_name)
       free((*conn)->own_host_name);
    free(*conn);
    *conn = NULL;

    purc_set_error(err_code);
    return -1;
}

static int ws_open_connection(const char *host, const char *port)
{
    int fd = -1;
    struct addrinfo *addrinfo;
    struct addrinfo *p;
    struct addrinfo hints = { 0 };

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (0 != getaddrinfo(host, port, &hints, &addrinfo)) {
        PC_DEBUG ("Error while getting address info (%s:%s)\n",
                host, port);
        goto out;
    }

    for (p = addrinfo; p != NULL; p = p->ai_next) {
        if((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }

        if (connect(fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(fd);
            continue;
        }
        break;
    }
    freeaddrinfo(addrinfo);

    if (p == NULL) {
        PC_DEBUG ("Connect to websocket server failed! (%s:%s)\n",
                host, port);
        goto out;
    }

out:
    return fd;
}

static void ws_sha1_digest(const char *s, int len, unsigned char *digest)
{
  pcutils_sha1_ctxt sha;

  pcutils_sha1_begin(&sha);
  pcutils_sha1_hash(&sha, (uint8_t *) s, len);
  pcutils_sha1_end(&sha, digest);
}

static int ws_verify_handshake(const char *ws_key, char *header)
{
    (void) header;
    int ret = -1;

    size_t klen = strlen(ws_key);
    size_t mlen = strlen(WS_MAGIC_STR);
    size_t len = klen + mlen;
    char *s = malloc (klen + mlen + 1);
    uint8_t digest[SHA_DIGEST_LEN] = { 0 };

    memset(digest, 0, sizeof *digest);
    memcpy(s, ws_key, klen);
    memcpy(s + klen, WS_MAGIC_STR, mlen + 1);
    ws_sha1_digest(s, len, digest);

    char *encode = pcutils_b64_encode_alloc((unsigned char *)digest,
            sizeof(digest));

    char *tmp = NULL;
    const char *line = header, *next = NULL;
    bool valid_status = false;
    bool valid_accept = false;
    bool valid_upgrade = false;
    bool valid_connection = false;

    while (line) {
        if ((next = strstr (line, "\r\n")) != NULL) {
            len = (next - line);
        }
        else {
            len = strlen (line);
        }

        if (len <= 0) {
            PC_DEBUG ("Bad http header during handshake\n");
            goto out;
        }

        tmp = malloc(len + 1);
        memcpy (tmp, line, len);
        tmp[len] = '\0';

        if(tmp[0] == 'H' && tmp[1] == 'T' && tmp[2] == 'T'
                && tmp[3] == 'P') {
            if(strcmp(tmp, "HTTP/1.1 101 Switching Protocols") != 0 &&
                    strcmp(tmp, "HTTP/1.0 101 Switching Protocols") != 0) {
                PC_DEBUG ("Peer protocol invalid : %s\n", tmp);
                goto out;
            }
            valid_status = true;
        }
        else {
            char *p = strchr(tmp, ' ');
            if (p) {
                *p = '\0';
            }

            if (strcmp(tmp, "Upgrade:") == 0 &&
                    strcasecmp(p + 1, "websocket") == 0) {
                valid_upgrade = true;
            }
            else if (strcmp(tmp, "Connection:") == 0 &&
                    strcasecmp(p + 1, "upgrade") == 0) {
                valid_connection = true;
            }
            else if (strcmp(tmp, "Sec-WebSocket-Accept:") == 0 &&
                    strcmp(p + 1, encode) == 0) {
                    valid_accept = true;
            }
        }

        free (tmp);
        tmp = NULL;

        line = next ? (next + 2) : NULL;
        if (strcmp(next, "\r\n\r\n") == 0) {
            break;
        }
    }

    if (!valid_status) {
        PC_DEBUG ("Bad http status during handshake\n");
        goto out;
    }

    if (!valid_accept) {
        PC_DEBUG ("Verify Sec-WebSocket-Accept failed during handshake\n");
        goto out;
    }

    if (!valid_upgrade) {
        PC_DEBUG ("Not found upgrade header during handshake\n");
        goto out;
    }

    if (!valid_connection) {
        PC_DEBUG ("Not found connection header during handshake\n");
        goto out;
    }

    ret = 0;
out:
    if (tmp) {
        free(tmp);
    }

    if (s) {
        free(s);
    }

    if (encode) {
        free(encode);
    }
    return ret;
}

static int ws_handshake(pcrdr_conn *conn, const char *host_name, const char *port,
        const char* app_name, const char* runner_name)
{
    (void)app_name;
    (void)runner_name;
    int ret = -1;

    /* generate Sec-WebSocket-Key */
    srand(time(NULL));
    char key[WS_KEY_LEN];
    for (int i = 0; i < WS_KEY_LEN; i++) {
        key[i] = rand() & 0xff;
    }
    char *ws_key =  pcutils_b64_encode_alloc ((unsigned char *) key, WS_KEY_LEN);
    char req_headers[1024] = { 0 };

    snprintf(req_headers, 1024,
            "GET / HTTP/1.1\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Host: %s:%s\r\n"
            "Sec-WebSocket-Key: %s\r\n"
            "Sec-WebSocket-Version: 13\r\n\r\n",
            host_name, port, ws_key);

    /* send to server */
    ws_write(conn->fd, req_headers, strlen(req_headers));

    char buf[1024] = { 0 };
#if 0
    char *p = buf;
    while (true) {
        if (ws_read(fd, p, 1) != 1) {
            PC_DEBUG ("Error receiving data during handshake\n");
            goto out;
        }
        p++;
        if (p - buf >= 4 && strcmp(p - 4, "\r\n\r\n") == 0) {
            break;
        }
    }
#else

    char *p = NULL;
    ssize_t n = ws_read(conn->fd, buf, 1024);
    if (n == 0) {
        PC_DEBUG ("Peer closed during handshake\n");
        goto out;
    }

    if (n < 0) {
        PC_DEBUG ("Error receiving data during handshake\n");
        goto out;
    }

    p = strstr(buf, "\r\n\r\n");
    if (p == NULL) {
        PC_DEBUG ("Received invalid data during handshake (%s)\n", buf);
        goto out;
    }

    p += 4;
    if (*p != 0) {
        int sz = n - (p - buf);
        conn->sticky = malloc(sz + 1);
        if (conn->sticky == NULL) {
            PC_DEBUG ("Failed to allocate memory.\n");
            ret = PCRDR_ERROR_NOMEM;
            goto out;
        }

        memcpy(conn->sticky, p, sz);
        conn->sticky_pos = conn->sticky;
        conn->nr_sticky = sz;
        *p = 0;
    }
#endif

    ret = ws_verify_handshake(ws_key, buf);

out:
    if (ws_key) {
        free(ws_key);
    }
    return ret;
}

int pcrdr_socket_connect_via_web_socket (const char* host_name, int port,
        const char* app_name, const char* runner_name, pcrdr_conn** conn)
{
    UNUSED_PARAM(host_name);
    UNUSED_PARAM(port);
    UNUSED_PARAM(app_name);
    UNUSED_PARAM(runner_name);
    UNUSED_PARAM(conn);

    int fd, err_code = PCRDR_ERROR_BAD_CONNECTION;

    if (!purc_is_valid_app_name(app_name) ||
            !purc_is_valid_runner_name(runner_name)) {
        purc_set_error(PURC_EXCEPT_INVALID_VALUE);
        return -1;
    }

    if ((*conn = calloc (1, sizeof (pcrdr_conn))) == NULL) {
        PC_DEBUG ("Failed to callocate space for connection: %s\n",
                strerror (errno));
        purc_set_error(PCRDR_ERROR_NOMEM);
        return -1;
    }

    char s_port[16];
    snprintf(s_port, sizeof(s_port), "%d", port);
    if ((fd = ws_open_connection(host_name, s_port)) < 0) {
        PC_WARN("ws_open_connection failed %s:%s\n", host_name, s_port);
        goto error;
    }

    (*conn)->prot = PURC_RDRCOMM_SOCKET;
    (*conn)->type = CT_INET_SOCKET;
    (*conn)->fd = fd;

    if (ws_handshake(*conn, host_name, s_port, app_name,
                runner_name) != 0) {
        PC_WARN("ws_handshake failed %s:%s\n", host_name, s_port);
        goto error;
    }

    (*conn)->timeout_ms = 10;   /* 10 milliseconds */
    (*conn)->srv_host_name = NULL;
    (*conn)->own_host_name = strdup (PCRDR_LOCALHOST);
    (*conn)->app_name = app_name;
    (*conn)->runner_name = runner_name;

    (*conn)->wait_message = my_wait_message;
    (*conn)->read_message = my_read_message;
    (*conn)->send_message = my_send_message;
    (*conn)->ping_peer = my_ping_peer;
    (*conn)->disconnect = my_disconnect;

    list_head_init (&(*conn)->pending_requests);
    return fd;

error:
    close (fd);

    if ((*conn)->own_host_name) {
       free((*conn)->own_host_name);
    }

    if ((*conn)->sticky) {
       free ((*conn)->sticky);
    }
    free(*conn);
    *conn = NULL;

    purc_set_error(err_code);
    return -1;
}

int pcrdr_socket_read_packet (pcrdr_conn* conn, char* packet_buf, size_t *sz_packet)
{
    unsigned int offset;
    int err_code = 0;

    if (conn->type == CT_UNIX_SOCKET) {
        USFrameHeader header;

        if (conn_read (conn->fd, &header, sizeof (USFrameHeader))) {
            PC_DEBUG ("Failed to read frame header from Unix socket\n");
            err_code = PCRDR_ERROR_IO;
            goto done;
        }

        if (header.op == US_OPCODE_PONG) {
            // TODO
            *sz_packet = 0;
            return 0;
        }
        else if (header.op == US_OPCODE_PING) {
            header.op = US_OPCODE_PONG;
            header.sz_payload = 0;
            if (conn_write (conn->fd, &header, sizeof (USFrameHeader))) {
                err_code = PCRDR_ERROR_IO;
                goto done;
            }
            *sz_packet = 0;
            return 0;
        }
        else if (header.op == US_OPCODE_CLOSE) {
            PC_INFO ("Peer closed\n");
            err_code = PCRDR_ERROR_PEER_CLOSED;
            goto done;
        }
        else if (header.op == US_OPCODE_TEXT ||
                header.op == US_OPCODE_BIN) {
            unsigned int left;

            if (header.fragmented > PCRDR_MAX_INMEM_PAYLOAD_SIZE) {
                err_code = PCRDR_ERROR_TOO_LARGE;
                goto done;
            }

            int is_text;
            if (header.op == US_OPCODE_TEXT) {
                is_text = 1;
            }
            else {
                is_text = 0;
            }

            if (conn_read (conn->fd, packet_buf, header.sz_payload)) {
                PC_DEBUG ("Failed to read packet from Unix socket\n");
                err_code = PCRDR_ERROR_IO;
                goto done;
            }

            if (header.fragmented > header.sz_payload) {
                left = header.fragmented - header.sz_payload;
            }
            else
                left = 0;
            offset = header.sz_payload;
            while (left > 0) {
                if (conn_read (conn->fd, &header, sizeof (USFrameHeader))) {
                    PC_DEBUG ("Failed to read frame header from Unix socket\n");
                    err_code = PCRDR_ERROR_IO;
                    goto done;
                }

                if (header.op != US_OPCODE_CONTINUATION &&
                        header.op != US_OPCODE_END) {
                    PC_DEBUG ("Not a continuation frame\n");
                    err_code = PCRDR_ERROR_PROTOCOL;
                    goto done;
                }

                if (conn_read (conn->fd, packet_buf + offset, header.sz_payload)) {
                    PC_DEBUG ("Failed to read packet from Unix socket\n");
                    err_code = PCRDR_ERROR_IO;
                    goto done;
                }

                offset += header.sz_payload;
                left -= header.sz_payload;

                if (header.op == US_OPCODE_END) {
                    break;
                }
            }

            if (is_text) {
                ((char *)packet_buf) [offset] = '\0';
                *sz_packet = offset + 1;
            }
            else {
                *sz_packet = offset;
            }
        }
        else {
            PC_DEBUG ("Bad packet op code: %d\n", header.op);
            err_code = PCRDR_ERROR_PROTOCOL;
        }
    }
    else if (conn->type == CT_INET_SOCKET) {
        WSFrameHeader header;

        if (ws_read_frame_header(conn, &header) != 0) {
            PC_DEBUG ("Failed to read frame header from websocket\n");
            err_code = PCRDR_ERROR_IO;
            goto done;
        }

        if (header.op == WS_OPCODE_PONG) {
            // TODO
            char *buf = NULL;
            unsigned int nr_buf = 0;
            ws_read_data_frame(conn, &header, &buf, &nr_buf);
            if (buf) {
                free(buf);
            }
            *sz_packet = 0;
            PC_DEBUG ("Receive PONG message from websocket\n");
            return 0;
        }
        else if (header.op == WS_OPCODE_PING) {
            char *buf = NULL;
            unsigned int nr_buf = 0;
            ws_read_data_frame(conn, &header, &buf, &nr_buf);
            if (buf) {
                free(buf);
            }

            if (ws_pong(conn->fd) != 0) {
                err_code = PCRDR_ERROR_IO;
                goto done;
            }
            *sz_packet = 0;
            return 0;
        }
        else if (header.op == WS_OPCODE_CLOSE) {
            PC_DEBUG ("Peer closed\n");
            char *buf = NULL;
            unsigned int nr_buf = 0;
            ws_read_data_frame(conn, &header, &buf, &nr_buf);
            if (buf) {
                free(buf);
            }

            err_code = PCRDR_ERROR_PEER_CLOSED;
            goto done;
        }
        else if (header.op == WS_OPCODE_TEXT ||
                header.op == WS_OPCODE_BIN) {
            char *p = packet_buf;
            char *buf = NULL;
            unsigned int nr_buf = 0;
            unsigned int offset;
            int is_text;

            offset = 0;

            if (header.op == US_OPCODE_TEXT) {
                is_text = 1;
            }
            else {
                is_text = 0;
            }
            do {
                if (ws_read_data_frame(conn, &header, &buf, &nr_buf) != 0) {
                    PC_DEBUG ("Failed to read packet from WebSocket\n");
                    err_code = PCRDR_ERROR_IO;
                    goto done;
                }

                memcpy(p, buf, nr_buf);
                free(buf);

                p += nr_buf;
                offset += nr_buf;

                if (header.fin == 1) {
                    break;
                }

                if (ws_read_frame_header(conn, &header) != 0) {
                    PC_DEBUG ("Failed to read frame header from WebSocket\n");
                    err_code = PCRDR_ERROR_IO;
                    goto done;
                }

                if (header.op != WS_OPCODE_CONTINUATION) {
                    PC_DEBUG ("Not a continuation frame\n");
                    err_code = PCRDR_ERROR_PROTOCOL;
                    goto done;
                }
            } while(true);

            if (is_text) {
                ((char *)packet_buf) [offset] = '\0';
                *sz_packet = offset + 1;
            }
            else {
                *sz_packet = offset;
            }
        }
        else {
            PC_DEBUG ("Bad packet op code a: %d\n", header.op);
            err_code = PCRDR_ERROR_PROTOCOL;
        }
    }
    else {
        err_code = PCRDR_ERROR_INVALID_VALUE;
    }

done:
    if (err_code) {
        purc_set_error (err_code);
        err_code = -1;
    }

    return err_code;
}

int pcrdr_socket_read_packet_alloc (pcrdr_conn* conn, void **packet, size_t *sz_packet)
{
    char* packet_buf = NULL;
    int err_code = 0;

    if (conn->type == CT_UNIX_SOCKET) {
        USFrameHeader header;

        if (conn_read (conn->fd, &header, sizeof (USFrameHeader))) {
            PC_DEBUG ("Failed to read frame header from Unix socket\n");
            err_code = PCRDR_ERROR_IO;
            goto done;
        }

        if (header.op == US_OPCODE_PONG) {
            // TODO
            *packet = NULL;
            *sz_packet = 0;
            return 0;
        }
        else if (header.op == US_OPCODE_PING) {
            header.op = US_OPCODE_PONG;
            header.sz_payload = 0;
            if (conn_write (conn->fd, &header, sizeof (USFrameHeader))) {
                err_code = PCRDR_ERROR_IO;
                goto done;
            }

            *packet = NULL;
            *sz_packet = 0;
            return 0;
        }
        else if (header.op == US_OPCODE_CLOSE) {
            PC_INFO ("Peer closed\n");
            err_code = PCRDR_ERROR_PEER_CLOSED;
            goto done;
        }
        else if (header.op == US_OPCODE_TEXT ||
                header.op == US_OPCODE_BIN) {
            unsigned int total_len, left;
            unsigned int offset;
            int is_text;

            if (header.fragmented > PCRDR_MAX_INMEM_PAYLOAD_SIZE) {
                err_code = PCRDR_ERROR_TOO_LARGE;
                goto done;
            }

            if (header.op == US_OPCODE_TEXT) {
                is_text = 1;
            }
            else {
                is_text = 0;
            }

            if (header.fragmented > header.sz_payload) {
                total_len = header.fragmented;
                offset = header.sz_payload;
                left = total_len - header.sz_payload;
            }
            else {
                total_len = header.sz_payload;
                offset = header.sz_payload;
                left = 0;
            }

            if ((packet_buf = malloc (total_len + 1)) == NULL) {
                err_code = PCRDR_ERROR_NOMEM;
                goto done;
            }

            if (conn_read (conn->fd, packet_buf, header.sz_payload)) {
                PC_DEBUG ("Failed to read packet from Unix socket\n");
                err_code = PCRDR_ERROR_IO;
                goto done;
            }

            while (left > 0) {
                if (conn_read (conn->fd, &header, sizeof (USFrameHeader))) {
                    PC_DEBUG ("Failed to read frame header from Unix socket\n");
                    err_code = PCRDR_ERROR_IO;
                    goto done;
                }

                if (header.op != US_OPCODE_CONTINUATION &&
                        header.op != US_OPCODE_END) {
                    PC_DEBUG ("Not a continuation frame\n");
                    err_code = PCRDR_ERROR_PROTOCOL;
                    goto done;
                }

                if (conn_read (conn->fd, packet_buf + offset, header.sz_payload)) {
                    PC_DEBUG ("Failed to read packet from Unix socket\n");
                    err_code = PCRDR_ERROR_IO;
                    goto done;
                }

                left -= header.sz_payload;
                offset += header.sz_payload;
                if (header.op == US_OPCODE_END) {
                    break;
                }
            }

            if (is_text) {
                ((char *)packet_buf) [offset] = '\0';
                *sz_packet = offset + 1;
            }
            else {
                *sz_packet = offset;
            }

            goto done;
        }
        else {
            PC_DEBUG ("Bad packet op code: %d\n", header.op);
            err_code = PCRDR_ERROR_PROTOCOL;
            goto done;
        }
    }
    else if (conn->type == CT_INET_SOCKET) {
        WSFrameHeader header;

        if (ws_read_frame_header(conn, &header) != 0) {
            PC_DEBUG ("Failed to read frame header from websocket\n");
            err_code = PCRDR_ERROR_IO;
            goto done;
        }

        if (header.op == WS_OPCODE_PONG) {
            // TODO
            char *buf = NULL;
            unsigned int nr_buf = 0;
            ws_read_data_frame(conn, &header, &buf, &nr_buf);
            if (buf) {
                free(buf);
            }
            *sz_packet = 0;
            PC_DEBUG ("Receive PONG message from websocket\n");
            return 0;
        }
        else if (header.op == WS_OPCODE_PING) {
            char *buf = NULL;
            unsigned int nr_buf = 0;
            ws_read_data_frame(conn, &header, &buf, &nr_buf);
            if (buf) {
                free(buf);
            }

            if (ws_pong(conn->fd) != 0) {
                err_code = PCRDR_ERROR_IO;
                goto done;
            }
            *sz_packet = 0;
            return 0;
        }
        else if (header.op == WS_OPCODE_CLOSE) {
            PC_DEBUG ("Peer closed\n");
            char *buf = NULL;
            unsigned int nr_buf = 0;
            ws_read_data_frame(conn, &header, &buf, &nr_buf);
            if (buf) {
                free(buf);
            }

            err_code = PCRDR_ERROR_PEER_CLOSED;
            goto done;
        }
        else if (header.op == WS_OPCODE_TEXT ||
                header.op == WS_OPCODE_BIN) {
            unsigned int offset = 0;
            char *p = NULL;
            char *buf = NULL;
            unsigned int nr_buf = 0;
            int is_text;

            if (header.op == US_OPCODE_TEXT) {
                is_text = 1;
            }
            else {
                is_text = 0;
            }
            do {
                if (ws_read_data_frame(conn, &header, &buf, &nr_buf) != 0) {
                    PC_DEBUG ("Failed to read packet from WebSocket\n");
                    err_code = PCRDR_ERROR_IO;
                    goto done;
                }

                if (packet_buf == NULL) {
                    packet_buf = malloc(nr_buf + 1);
                    p = packet_buf;
                }
                else {
                    packet_buf = realloc(packet_buf, offset + nr_buf + 1);
                    p = packet_buf + offset;
                }

                if (packet_buf == NULL) {
                    err_code = PCRDR_ERROR_NOMEM;
                    goto done;
                }

                memcpy(p, buf, nr_buf);
                free(buf);
                offset += nr_buf;

                if (header.fin == 1) {
                    break;
                }

                if (ws_read_frame_header(conn, &header) != 0) {
                    PC_DEBUG ("Failed to read frame header from WebSocket\n");
                    err_code = PCRDR_ERROR_IO;
                    goto done;
                }

                if (header.op != WS_OPCODE_CONTINUATION) {
                    PC_DEBUG ("Not a continuation frame\n");
                    err_code = PCRDR_ERROR_PROTOCOL;
                    goto done;
                }
            } while(true);

            if (is_text) {
                ((char *)packet_buf) [offset] = '\0';
                *sz_packet = offset + 1;
            }
            else {
                *sz_packet = offset;
            }
        }
        else {
            PC_DEBUG ("Bad packet op code b: %d\n", header.op);
            err_code = PCRDR_ERROR_PROTOCOL;
        }
    }
    else {
        assert (0);
        err_code = PCRDR_ERROR_INVALID_VALUE;
        goto done;
    }

done:
    if (err_code) {
        if (packet_buf)
            free (packet_buf);

        *packet = NULL;
        purc_set_error (err_code);
        return -1;
    }

    *packet = packet_buf;
    return 0;
}

int pcrdr_socket_send_text_packet (pcrdr_conn* conn, const char* text, size_t len)
{
    int retv = 0;

    if (conn->type == CT_UNIX_SOCKET) {
        USFrameHeader header;

        if (len > PCRDR_MAX_FRAME_PAYLOAD_SIZE) {
            size_t left = len;

            do {
                if (left == len) {
                    header.op = US_OPCODE_TEXT;
                    header.fragmented = len;
                    header.sz_payload = PCRDR_MAX_FRAME_PAYLOAD_SIZE;
                    left -= PCRDR_MAX_FRAME_PAYLOAD_SIZE;
                }
                else if (left > PCRDR_MAX_FRAME_PAYLOAD_SIZE) {
                    header.op = US_OPCODE_CONTINUATION;
                    header.fragmented = 0;
                    header.sz_payload = PCRDR_MAX_FRAME_PAYLOAD_SIZE;
                    left -= PCRDR_MAX_FRAME_PAYLOAD_SIZE;
                }
                else {
                    header.op = US_OPCODE_END;
                    header.fragmented = 0;
                    header.sz_payload = left;
                    left = 0;
                }

                if (conn_write (conn->fd, &header, sizeof (USFrameHeader)) == 0) {
                    retv = conn_write (conn->fd, text, header.sz_payload);
                    text += header.sz_payload;
                }

            } while (left > 0 && retv == 0);
        }
        else {
            header.op = US_OPCODE_TEXT;
            header.fragmented = 0;
            header.sz_payload = len;
            if (conn_write (conn->fd, &header, sizeof (USFrameHeader)) == 0)
                retv = conn_write (conn->fd, text, len);
        }
    }
    else if (conn->type == CT_INET_SOCKET) {
        if (len > PCRDR_MAX_INMEM_PAYLOAD_SIZE) {
            PC_DEBUG("Sending a too large packet, size: %lu\n", len);
            retv = PCRDR_ERROR_TOO_LARGE;
        }
        else if (len > PCRDR_MAX_FRAME_PAYLOAD_SIZE) {
            unsigned int left = len;
            int fin;
            int opcode;
            size_t sz_payload;

            do {
                if (left == len) {
                    fin = 0;
                    opcode = WS_OPCODE_TEXT;
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

                if(ws_send_data_frame(conn->fd, fin, opcode, text, sz_payload) == 0) {
                    text += sz_payload;
                }
            } while (left > 0 && retv == 0);
        }
        else {
            retv = ws_send_data_frame(conn->fd, 1, WS_OPCODE_TEXT, text, len);
        }
    }
    else
        retv = PCRDR_ERROR_INVALID_VALUE;

    return retv;
}

#define SCHEMA_UNIX_SOCKET  "unix://"

pcrdr_msg *pcrdr_local_socket_connect(const char* renderer_uri,
        const char* app_name, const char* runner_name, pcrdr_conn** conn)
{
    pcrdr_msg *msg = NULL;

    if (strncasecmp (SCHEMA_UNIX_SOCKET, renderer_uri,
            sizeof(SCHEMA_UNIX_SOCKET) - 1)) {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        return NULL;
    }

    if (purcmc_connect_via_unix_socket(
            renderer_uri + sizeof(SCHEMA_UNIX_SOCKET) - 1,
            app_name, runner_name, conn) < 0) {
        return NULL;
    }

    /* read the initial response from the server */
    char buff[PCRDR_DEF_PACKET_BUFF_SIZE];
    size_t len = sizeof(buff);

    if (pcrdr_socket_read_packet(*conn, buff, &len) < 0)
        goto failed;

    (*conn)->stats.bytes_recv += len;
    if (pcrdr_parse_packet(buff, len, &msg) < 0)
        goto failed;

    return msg;

failed:
    if (msg)
        pcrdr_release_message(msg);

    if (*conn) {
        pcrdr_disconnect(*conn);
    }

    return NULL;
}

#else   /* for OS not Linux or Unix */

pcrdr_msg *pcrdr_local_socket_connect(const char* renderer_uri,
        const char* app_name, const char* runner_name, pcrdr_conn** conn)
{
    purc_set_error(PCRDR_ERROR_NOT_IMPLEMENTED);
    return NULL;
}

#endif

#define SCHEMA_WEBSOCKET  "ws://"

pcrdr_msg *
pcrdr_websocket_connect(const char* renderer_uri,
        const char* app_name, const char* runner_name, pcrdr_conn** conn)
{
    pcrdr_msg *msg = NULL;
    char *host_name = NULL;
    int port;

    if (strncasecmp (SCHEMA_WEBSOCKET, renderer_uri,
            sizeof(SCHEMA_WEBSOCKET) - 1)) {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        return NULL;
    }

    const char *s_port;
    const char *p = renderer_uri + sizeof(SCHEMA_WEBSOCKET) - 1;
    char *q = strstr(p, ":");
    if (q == NULL) {
        s_port = PCRDR_PURCMC_WS_PORT;
        host_name = strdup(p);
    }
    else {
        s_port = q + 1;
        host_name = strndup(p, q - p);
    }

    if (!s_port[0]) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    port = atoi(s_port);
    if (port <=0 || port > 65535) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (pcrdr_socket_connect_via_web_socket(
            host_name, port,
            app_name, runner_name, conn) < 0) {
        goto failed;
    }

    /* read the initial response from the server */
    char buff[PCRDR_DEF_PACKET_BUFF_SIZE];
    size_t len = sizeof(buff);

    if (pcrdr_socket_read_packet(*conn, buff, &len) < 0)
        goto failed;

    (*conn)->stats.bytes_recv += len;
    if (pcrdr_parse_packet(buff, len, &msg) < 0)
        goto failed;

    if (host_name) {
        free(host_name);
    }

    return msg;

failed:
    if (msg)
        pcrdr_release_message(msg);

    if (*conn) {
        pcrdr_disconnect(*conn);
    }

    if (host_name) {
        free(host_name);
    }

    return NULL;
}

