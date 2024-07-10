/*
** @file finder.c
** @author Vincent Wei
** @date 2023/10/20
** @brief The implementation of renderer finder of Seeker.
**
** Copyright (C) 2023 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of purc, which is an HVML interpreter with
** a command line interface (CLI).
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#define _GNU_SOURCE
#include "config.h"

#undef NDEBUG
#include "seeker.h"
#include "finder.h"
#include "purcmc-thread.h"
#include "endpoint.h"

#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

#define UNIX_SOCKET_URI_PREFIX      "unix://"

static void notify_endpoint_about_new_renderer(pcmcth_renderer *rdr,
        const char *comm, const char *uri)
{
    const char* name;
    void *data;

    pcrdr_msg *msg = NULL;
    kvlist_for_each(&rdr->endpoint_list, name, data) {
        pcmcth_endpoint *endpoint = *(pcmcth_endpoint **)data;

        pcrdr_msg *msg = pcrdr_make_void_message();
        if (msg == NULL) {
            goto failed;
        }

        msg->type = PCRDR_MSG_TYPE_EVENT;
        msg->target = PCRDR_MSG_TARGET_SESSION;
        msg->targetValue = 0;
        msg->eventName =
            purc_variant_make_string_static(PCRDR_EVENT_DUP_RENDERER, false);

        /* TODO: use real URI for the sourceURI */
        msg->sourceURI =
            purc_variant_make_string_static(PCRDR_APP_RENDERER, false);
        msg->elementType = PCRDR_MSG_ELEMENT_TYPE_VOID;
        msg->property = PURC_VARIANT_INVALID;

        purc_variant_t data = purc_variant_make_object_0();
        if (data == NULL)
            goto failed;

        purc_variant_t tmp = purc_variant_make_string(comm, false);
        if (tmp) {
            purc_variant_object_set_by_ckey(data, "comm", tmp);
            purc_variant_unref(tmp);
        }

        tmp = purc_variant_make_string(uri, false);
        if (tmp) {
            purc_variant_object_set_by_ckey(data, "uri", tmp);
            purc_variant_unref(tmp);
        }

        msg->dataType = PCRDR_MSG_DATA_TYPE_JSON;
        msg->data = data;

        purc_atom_t rid = get_endpoint_rid(endpoint);
        if (purc_inst_move_message(rid, msg) == 0) {
            goto failed;
        }

        pcrdr_release_message(msg);
        msg = NULL;
    }

    return;

failed:
    LOG_ERROR("Failed when notify endpoints about new renderer: %s\n",
                    purc_get_error_message(purc_get_last_error()));
    if (msg)
        pcrdr_release_message(msg);
}

static int seek_renderer_on_unix_socket(pcmcth_renderer *rdr)
{
#if HAVE(LINUX_MEMFD_H)
    FILE *fp = fopen("/proc/net/unix", "r");
    if (fp) {
        char buf[PATH_MAX + 128];
        while (fgets(buf, sizeof(buf), fp)) {
            char *matched = strstr(buf, PCRDR_PURCMC_US_NAME);
            if (matched) {
                if (matched[sizeof(PCRDR_PURCMC_US_NAME) - 1] == '\n')
                    matched[sizeof(PCRDR_PURCMC_US_NAME) - 1] = 0;

                if (matched[sizeof(PCRDR_PURCMC_US_NAME) - 1] == 0) {
                    char *path = strchr(buf, '/');
                    if (access(path, R_OK | W_OK) == 0) {
                        char *uri = path - sizeof(UNIX_SOCKET_URI_PREFIX) + 1;
                        memcpy(uri, UNIX_SOCKET_URI_PREFIX,
                                sizeof(UNIX_SOCKET_URI_PREFIX) - 1);

                        LOG_DEBUG("Find one renderer at %s.\n", uri);
                        notify_endpoint_about_new_renderer(rdr, "socket", uri);
                        break;
                    }
                }
            }
        }

        fclose(fp);
        return 0;
    }
    else {
        LOG_ERROR("Cannot open /proc/net/unix for read; finder disabled.\n");
        return -1;
    }
#else  /* HAVE(LINUX_MEMFD_H) */
    /* try the default socket */
    if (access(PCRDR_PURCMC_US_PATH, R_OK | W_OK) == 0) {
        int fd;
        if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            LOG_ERROR("Cannot create a socket; finder disabled.\n");
            return -1;
        }

        /* fill socket address structure w/server's addr */
        struct sockaddr_un unix_addr;
        memset(&unix_addr, 0, sizeof(unix_addr));
        unix_addr.sun_family = AF_UNIX;
        strcpy(unix_addr.sun_path, PCRDR_PURCMC_US_PATH);
        size_t len = sizeof(unix_addr.sun_family) + sizeof(PCRDR_PURCMC_US_PATH);
        if (connect(fd, (struct sockaddr *) &unix_addr, len) < 0) {
            LOG_WARN("Cannot connect to the renderer.\n");
        }
        else {
            LOG_WARN("Find one renderer at %s.\n", PCRDR_PURCMC_US_PATH);
            notify_endpoint_about_new_renderer(rdr,
                    "socket", PCRDR_PURCMC_US_PATH);
        }
        close(fd);
    }
#endif /* !HAVE(LINUX_MEMFD_H) */

    return 0;
}

int seeker_look_for_local_renderer(const char *name, void *ctxt)
{
    pcmcth_renderer *rdr = ctxt;
    LOG_DEBUG("It is time to find a new local renderer: %s for rdr: %p\n",
            name, rdr);

    if (strcmp(name, SEEKER_UNIX_FINDER_NAME) == 0) {
        return seek_renderer_on_unix_socket(rdr);
    }
#if PCA_ENABLE_DNSSD
    else if (strcmp(name, SEEKER_NET_FINDER_NAME) == 0) {
        assert(rdr->impl->browsing_handle);
        purc_dnssd_stop_browsing(rdr->impl->dnssd,
                rdr->impl->browsing_handle);

        rdr->impl->browsing_handle = purc_dnssd_start_browsing(rdr->impl->dnssd,
                PCRDR_PURCMC_DNSSD_TYPE, NULL);
        if (rdr->impl->browsing_handle == NULL) {
            LOG_WARN("Failed to start browsing; finder disabled.\n");
            purc_dnssd_disconnect(rdr->impl->dnssd);
            rdr->impl->dnssd = NULL;
            return -1;
        }
    }
#endif

    return 0;
}

#if PCA_ENABLE_DNSSD
void seeker_dnssd_on_service_discovered(struct purc_dnssd_conn *dnssd,
        void *browsing_handle,
        unsigned int flags, uint32_t if_index, int error_code,
        const char *service_name, const char *reg_type, const char *hostname,
        uint16_t port, uint16_t len_txt_record, const char *txt_record,
        void *ctxt)
{
    pcmcth_renderer *rdr = ctxt;
    (void)dnssd;
    (void)browsing_handle;
    (void)flags;

    if (error_code == 0) {
        LOG_WARN("Find a service `%s` with type `%s` on `%s` at port (%u)\n",
                service_name, reg_type, hostname, (unsigned)port);
        LOG_WARN("    The interface index: %u\n", if_index);
        if (len_txt_record) {
            LOG_WARN("    The TXT record: %s\n", txt_record);
        }

        char *uri;
        if (asprintf(&uri, "ws://%s:%u", hostname, (unsigned)port) >= 0) {
            notify_endpoint_about_new_renderer(rdr, "websocket", uri);
            free(uri);
        }
        else {
            LOG_ERROR("Failed to assemble URI for new renderer: ws://%s:%u\n",
                    hostname, (unsigned)port);
        }
    }
    else {
        LOG_WARN("Error occurred when browsing service: %d.\n", error_code);
    }
}
#endif /* PCA_ENABLE_DNSSD */

