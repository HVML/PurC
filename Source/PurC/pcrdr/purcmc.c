/*
 * purcmc.c -- The implementation of PurCMC protocol.
 *
 * Copyright (c) 2021, 2022 FMSoft (http://www.fmsoft.cn)
 *
 * Authors:
 *  Vincent Wei (https://github.com/VincentWei), 2021, 2022
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
#include "private/kvlist.h"
#include "private/debug.h"
#include "private/utils.h"
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
int pcrdr_purcmc_connect_via_unix_socket (const char* path_to_socket,
        const char* app_name, const char* runner_name, pcrdr_conn** conn)
{
    int fd, len, err_code = PCRDR_ERROR_BAD_CONNECTION;
    struct sockaddr_un unix_addr;
    char peer_name [33];
    char *ch_code = NULL;

    if ((*conn = calloc (1, sizeof (pcrdr_conn))) == NULL) {
        PC_DEBUG ("Failed to callocate space for connection: %s\n",
                strerror (errno));
        return PCRDR_ERROR_NOMEM;
    }

    /* create a Unix domain stream socket */
    if ((fd = socket (AF_UNIX, SOCK_STREAM, 0)) < 0) {
        PC_DEBUG ("Failed to call `socket` in pcrdr_purcmc_connect_via_unix_socket: %s\n",
                strerror (errno));
        return PCRDR_ERROR_IO;
    }

    {
        pcutils_md5_ctx_t ctx;
        unsigned char md5_digest[16];

        pcutils_md5_begin (&ctx);
        pcutils_md5_hash (app_name, strlen (app_name), &ctx);
        pcutils_md5_hash ("/", 1, &ctx);
        pcutils_md5_hash (runner_name, strlen (runner_name), &ctx);
        pcutils_md5_end (md5_digest, &ctx);
        pcutils_bin2hex (md5_digest, 16, peer_name);
    }

    /* fill socket address structure w/our address */
    memset (&unix_addr, 0, sizeof(unix_addr));
    unix_addr.sun_family = AF_UNIX;
    /* On Linux sun_path is 108 bytes in size */
    sprintf (unix_addr.sun_path, "%s%s-%05d", CLI_PATH, peer_name, getpid());
    len = sizeof (unix_addr.sun_family) + strlen (unix_addr.sun_path);

    unlink (unix_addr.sun_path);        /* in case it already exists */
    if (bind (fd, (struct sockaddr *) &unix_addr, len) < 0) {
        PC_DEBUG ("Failed to call `bind` in pcrdr_purcmc_connect_via_unix_socket: %s\n",
                strerror (errno));
        goto error;
    }
    if (chmod (unix_addr.sun_path, CLI_PERM) < 0) {
        PC_DEBUG ("Failed to call `chmod` in pcrdr_purcmc_connect_via_unix_socket: %s\n",
                strerror (errno));
        goto error;
    }

    /* fill socket address structure w/server's addr */
    memset (&unix_addr, 0, sizeof(unix_addr));
    unix_addr.sun_family = AF_UNIX;
    strcpy (unix_addr.sun_path, path_to_socket);
    len = sizeof (unix_addr.sun_family) + strlen (unix_addr.sun_path);

    if (connect (fd, (struct sockaddr *) &unix_addr, len) < 0) {
        PC_DEBUG ("Failed to call `connect` in pcrdr_purcmc_connect_via_unix_socket: %s\n",
                strerror (errno));
        goto error;
    }

    (*conn)->prot = PURC_RDRPROT_PURCMC;
    (*conn)->type = CT_UNIX_SOCKET;
    (*conn)->fd = fd;
    (*conn)->srv_host_name = NULL;
    (*conn)->own_host_name = strdup (PCRDR_LOCALHOST);
    (*conn)->app_name = strdup (app_name);
    (*conn)->runner_name = strdup (runner_name);

    (*conn)->disconnect = my_disconnect;

    pcutils_kvlist_init (&(*conn)->call_list, NULL);

    return fd;

error:
    close (fd);

    if (ch_code)
        free (ch_code);
    if ((*conn)->own_host_name)
       free ((*conn)->own_host_name);
    if ((*conn)->app_name)
       free ((*conn)->app_name);
    if ((*conn)->runner_name)
       free ((*conn)->runner_name);
    free (*conn);
    *conn = NULL;

    purc_set_error (err_code);
    return -1;
}

int pcrdr_purcmc_connect_via_web_socket (const char* host_name, int port,
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

int pcrdr_purcmc_read_packet (pcrdr_conn* conn, char* packet_buf, size_t *sz_packet)
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

static inline void my_log (const char* str)
{
    ssize_t n = write (2, str, strlen (str));
    n = n & n;
}

int pcrdr_purcmc_read_packet_alloc (pcrdr_conn* conn, void **packet, size_t *sz_packet)
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

int pcrdr_purcmc_send_text_packet (pcrdr_conn* conn, const char* text, size_t len)
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

int pcrdr_purcmc_ping_server (pcrdr_conn* conn)
{
    int err_code = 0;

    if (conn->type == CT_UNIX_SOCKET) {
        USFrameHeader header;

        header.op = US_OPCODE_PING;
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

    if (err_code) {
        purc_set_error (err_code);
        err_code = -1;
    }

    return err_code;
}

int pcrdr_purcmc_read_and_dispatch_packet (pcrdr_conn* conn)
{
    void* packet;
    size_t data_len;
    pcrdr_msg* msg = NULL;
    int err_code, retval;

    err_code = pcrdr_purcmc_read_packet_alloc (conn, &packet, &data_len);
    if (err_code) {
        PC_DEBUG ("Failed to read packet\n");
        goto done;
    }

    if (data_len == 0) { // no data
        return 0;
    }

    retval = pcrdr_parse_packet (packet, data_len, &msg);
    free (packet);

    if (retval < 0) {
        PC_DEBUG ("Failed to parse JSON packet; quit...\n");
        err_code = PCRDR_ERROR_BAD_MESSAGE;
    }
    else if (msg->type == PCRDR_MSG_TYPE_EVENT) {
        PC_INFO ("The server gives an event packet\n");
        if (conn->event_handler) {
            conn->event_handler (conn, msg);
        }
    }
    else if (msg->type == PCRDR_MSG_TYPE_REQUEST) {
        PC_INFO ("The server gives a request packet\n");
    }
    else if (msg->type == PCRDR_MSG_TYPE_RESPONSE) {
        PC_INFO ("The server gives a response packet\n");
    }
    else {
        PC_DEBUG ("Unknown packet type; quit...\n");
        err_code = PCRDR_ERROR_PROTOCOL;
    }

done:
    if (msg)
        pcrdr_release_message (msg);

    if (err_code) {
        purc_set_error (err_code);
        err_code = -1;
    }

    return err_code;
}

int pcrdr_purcmc_wait_and_dispatch_packet (pcrdr_conn* conn, int timeout_ms)
{
    fd_set rfds;
    struct timeval tv;
    int err_code = 0;
    int retval;

    FD_ZERO (&rfds);
    FD_SET (conn->fd, &rfds);

    if (timeout_ms >= 0) {
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        retval = select (conn->fd + 1, &rfds, NULL, NULL, &tv);
    }
    else {
        retval = select (conn->fd + 1, &rfds, NULL, NULL, NULL);
    }

    if (retval == -1) {
        err_code = PCRDR_ERROR_BAD_SYSTEM_CALL;
    }
    else if (retval) {
        err_code = pcrdr_purcmc_read_and_dispatch_packet (conn);
    }
    else {
        err_code = PCRDR_ERROR_TIMEOUT;
    }

    if (err_code) {
        purc_set_error (err_code);
        err_code = -1;
    }

    return err_code;
}

#else   /* for OS not Linux or Unix */

int pcrdr_purcmc_connect_via_unix_socket (const char* path_to_socket,
        const char* app_name, const char* runner_name, pcrdr_conn** conn)
{
    UNUSED_PARAM(host_name);
    UNUSED_PARAM(port);
    UNUSED_PARAM(app_name);
    UNUSED_PARAM(runner_name);
    UNUSED_PARAM(conn);

    purc_set_error (PCRDR_ERROR_NOT_SUPPORTED);
    return -1;
}

int pcrdr_purcmc_connect_via_web_socket (const char* host_name, int port,
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

#endif
