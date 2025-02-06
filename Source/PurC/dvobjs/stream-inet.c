/*
 * @file stream-inet.c
 *
 * @date 2025/02/06
 * @brief The implementation of inet stream object.
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

#include "purc-variant.h"
#include "purc-runloop.h"
#include "purc-dvobjs.h"

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>

int dvobjs_inet_socket_connect(enum stream_inet_socket_family isf,
        const char *host, int port, char **peer_addr)
{
    int fd = -1;
    struct addrinfo *addrinfo;
    struct addrinfo *p;
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

    char s_port[10] = {0};
    if (port <= 0 || port > 65535) {
        PC_DEBUG ("Bad port value: (%d)\n", port);
        goto out;
    }
    sprintf(s_port, "%d", port);

    hints.ai_socktype = SOCK_STREAM;
    if (0 != getaddrinfo(host, s_port, &hints, &addrinfo)) {
        PC_DEBUG ("Error while getting address info (%s:%d)\n",
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

        if (*peer_addr == NULL) {
            char addr[NI_MAXHOST];
            if (0 != getnameinfo(p->ai_addr, p->ai_addrlen, addr, sizeof(addr),
                           NULL, 0, NI_NUMERICHOST)) {
                PC_DEBUG ("Error while getting name info (%s:%d)\n",
                        host, port);
                close(fd);
                fd = -1;
            }
            else {
                *peer_addr = strdup(addr);
            }
        }

        break;
    }
    freeaddrinfo(addrinfo);

    if (p == NULL) {
        PC_DEBUG ("Failed to create socket for %s:%d\n",
                host, port);
        goto out;
    }

out:
    return fd;
}

