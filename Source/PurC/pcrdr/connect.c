/*
 * connect.c -- The implementation of API to manage renderer connections.
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

pcrdr_event_handler pcrdr_conn_get_event_handler (pcrdr_conn *conn)
{
    return conn->event_handler;
}

pcrdr_event_handler pcrdr_conn_set_event_handler (pcrdr_conn *conn,
        pcrdr_event_handler event_handler)
{
    pcrdr_event_handler old = conn->event_handler;
    conn->event_handler = event_handler;

    return old;
}

void *pcrdr_conn_get_user_data (pcrdr_conn *conn)
{
    return conn->user_data;
}

void *pcrdr_conn_set_user_data (pcrdr_conn *conn, void *user_data)
{
    void *old = conn->user_data;
    conn->user_data = user_data;

    return old;
}

int pcrdr_conn_get_last_ret_code (pcrdr_conn *conn)
{
    return conn->last_ret_code;
}

const char* pcrdr_conn_srv_host_name (pcrdr_conn* conn)
{
    return conn->srv_host_name;
}

const char* pcrdr_conn_own_host_name (pcrdr_conn* conn)
{
    return conn->own_host_name;
}

const char* pcrdr_conn_app_name (pcrdr_conn* conn)
{
    return conn->app_name;
}

const char* pcrdr_conn_runner_name (pcrdr_conn* conn)
{
    return conn->runner_name;
}

int pcrdr_conn_socket_fd (pcrdr_conn* conn)
{
    return conn->fd;
}

int pcrdr_conn_socket_type (pcrdr_conn* conn)
{
    return conn->type;
}

int pcrdr_free_connection (pcrdr_conn* conn)
{
    assert (conn);

    if (conn->srv_host_name)
        free (conn->srv_host_name);
    free (conn->own_host_name);
    free (conn->app_name);
    free (conn->runner_name);

    pcutils_kvlist_free (&conn->call_list);

    free (conn);

    return 0;
}

int pcrdr_disconnect (pcrdr_conn* conn)
{
    int err_code = 0;

    err_code = conn->disconnect (conn);
    pcrdr_free_connection (conn);

    if (err_code) {
        purc_set_error (err_code);
        err_code = -1;
    }

    return err_code;
}

