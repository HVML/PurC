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

#define CLI_PATH    "/var/tmp/"
#define CLI_PERM    S_IRWXU

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
    else if (conn->type == CT_WEB_SOCKET) {
        /* TODO */
        err_code = PCRDR_ERROR_NOT_IMPLEMENTED;
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
    else if (conn->type == CT_WEB_SOCKET) {
        /* TODO */
        err_code = PCRDR_ERROR_NOT_IMPLEMENTED;
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
    sprintf (unix_addr.sun_path, "%s%s-%05d", CLI_PATH, peer_name, getpid());
    len = sizeof (unix_addr.sun_family) + strlen (unix_addr.sun_path) + 1;

    unlink (unix_addr.sun_path);        /* in case it already exists */
    if (bind (fd, (struct sockaddr *) &unix_addr, len) < 0) {
        PC_DEBUG ("Failed to call `bind` in %s: %s\n", __func__,
                strerror (errno));
        goto error;
    }
    if (chmod (unix_addr.sun_path, CLI_PERM) < 0) {
        PC_DEBUG ("Failed to call `chmod` in %s: %s\n", __func__,
                strerror (errno));
        goto error;
    }

    /* fill socket address structure w/server's addr */
    memset (&unix_addr, 0, sizeof(unix_addr));
    unix_addr.sun_family = AF_UNIX;
    strcpy (unix_addr.sun_path, path_to_socket);
    len = sizeof (unix_addr.sun_family) + strlen (unix_addr.sun_path) + 1;

    if (connect (fd, (struct sockaddr *) &unix_addr, len) < 0) {
        PC_DEBUG ("Failed to call `connect` in %s: %s\n", __func__,
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

int pcrdr_socket_connect_via_web_socket (const char* host_name, int port,
        const char* app_name, const char* runner_name, pcrdr_conn** conn)
{
    UNUSED_PARAM(host_name);
    UNUSED_PARAM(port);
    UNUSED_PARAM(app_name);
    UNUSED_PARAM(runner_name);
    UNUSED_PARAM(conn);

    purc_set_error (PCRDR_ERROR_NOT_IMPLEMENTED);
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
    else if (conn->type == CT_WEB_SOCKET) {
        /* TODO */
        err_code = PCRDR_ERROR_NOT_IMPLEMENTED;
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
    else if (conn->type == CT_WEB_SOCKET) {
        /* TODO */
        err_code = PCRDR_ERROR_NOT_IMPLEMENTED;
        goto done;
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
    else if (conn->type == CT_WEB_SOCKET) {
        /* TODO */
        retv = PCRDR_ERROR_NOT_IMPLEMENTED;
    }
    else
        retv = PCRDR_ERROR_INVALID_VALUE;

    return retv;
}

#define SCHEMA_UNIX_SOCKET  "unix://"

pcrdr_msg *pcrdr_socket_connect(const char* renderer_uri,
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

pcrdr_msg *pcrdr_socket_connect(const char* renderer_uri,
        const char* app_name, const char* runner_name, pcrdr_conn** conn)
{
    purc_set_error(PCRDR_ERROR_NOT_IMPLEMENTED);
    return NULL;
}

#endif
