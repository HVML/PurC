/**
 * @file connect.h
 * @date 2022/02/22
 * @brief The internal interfaces to manage renderer connection.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * Authors:
 *  Vincent Wei (<https://github.com/VincentWei>), 2021, 2022
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

#ifndef PURC_PCRDR_CONN_H
#define PURC_PCRDR_CONN_H

#include <time.h>

#include "purc-pcrdr.h"
#include "private/list.h"

#include "purc.h"

struct pending_request {
    struct list_head        list;

    purc_variant_t          request_id;
    pcrdr_response_handler  response_handler;
    void   *context;

    time_t  time_expected;
};

struct pcrdr_page_handle {
    struct list_head        list;

    char                   *workspace_name;
    char                   *group_name;
    char                   *page_name;

    pcrdr_page_type_k       page_type;
    uint64_t                page_handle;
    uint64_t                workspace_handle;
    uint64_t                dom_handle;
};

struct pcrdr_prot_data;

struct pcrdr_conn {
    struct list_head              ln;

    int prot;
    int type;
    int fd;
    int timeout_ms;
    time_t  async_close_expected;

    purc_atom_t                  id;
    char* name;
    char* uid;
    struct renderer_capabilities *caps;

    char* srv_host_name;
    char* own_host_name;
    const char* app_name;
    const char* runner_name;

    struct pcrdr_conn_stats stats;

    purc_atom_t                  uri_atom;
    char* uri;

    void *user_data;
    struct pcrdr_prot_data *prot_data;

    pcrdr_extra_message_source source_fn;
    void *source_ctxt; /* context for extra message source */

    char *sticky;       /* websocket sticky package after receive handshake */
    char *sticky_pos;
    size_t nr_sticky;

    pcrdr_request_handler request_handler;
    pcrdr_event_handler event_handler;

    /* the pending requests queue */
    struct list_head pending_requests;

    /* operations */
    int (*wait_message) (pcrdr_conn* conn, int timeout_ms);
    pcrdr_msg *(*read_message) (pcrdr_conn* conn);
    int (*send_message) (pcrdr_conn* conn, pcrdr_msg *msg);
    int (*ping_peer) (pcrdr_conn* conn);
    int (*disconnect) (pcrdr_conn* conn);
};

#endif  /* PURC_PCRDR_CONN_H */

