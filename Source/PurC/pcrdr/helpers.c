/*
 * helpers.c -- The helpers to manage renderer connections.
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

#include <assert.h>

/* Return Codes and Messages */
#define UNKNOWN_RET_CODE    "Unknown Return Code"

static struct  {
    int ret_code;
    const char* ret_msg;
} ret_code_2_messages[] = {
    { PCRDR_SC_IOERR,               /* 1 */
        "I/O Error" },
    { PCRDR_SC_OK,                  /* 200 */
        "Ok" },
    { PCRDR_SC_CREATED,             /* 201 */
        "Created" },
    { PCRDR_SC_ACCEPTED,            /* 202 */
        "Accepted" },
    { PCRDR_SC_NO_CONTENT,          /* 204 */
        "No Content" },
    { PCRDR_SC_RESET_CONTENT,       /* 205 */
        "Reset Content" },
    { PCRDR_SC_PARTIAL_CONTENT,     /* 206 */
        "Partial Content" },
    { PCRDR_SC_BAD_REQUEST,         /* 400 */
        "Bad Request" },
    { PCRDR_SC_UNAUTHORIZED,        /* 401 */
        "Unauthorized" },
    { PCRDR_SC_FORBIDDEN,           /* 403 */
        "Forbidden" },
    { PCRDR_SC_NOT_FOUND,           /* 404 */
        "Not Found" },
    { PCRDR_SC_METHOD_NOT_ALLOWED,  /* 405 */
        "Method Not Allowed" },
    { PCRDR_SC_NOT_ACCEPTABLE,      /* 406 */
        "Not Acceptable" },
    { PCRDR_SC_CONFLICT,            /* 409 */
        "Conflict" },
    { PCRDR_SC_GONE,                /* 410 */
        "Gone" },
    { PCRDR_SC_PRECONDITION_FAILED, /* 412 */
        "Precondition Failed" },
    { PCRDR_SC_PACKET_TOO_LARGE,    /* 413 */
        "Packet Too Large" },
    { PCRDR_SC_EXPECTATION_FAILED,  /* 417 */
        "Expectation Failed" },
    { PCRDR_SC_IM_A_TEAPOT,         /* 418 */
        "I'm a teapot" },
    { PCRDR_SC_UNPROCESSABLE_PACKET,    /* 422 */
        "Unprocessable Packet" },
    { PCRDR_SC_LOCKED,              /* 423 */
        "Locked" },
    { PCRDR_SC_FAILED_DEPENDENCY,   /* 424 */
        "Failed Dependency" },
    { PCRDR_SC_FAILED_DEPENDENCY,   /* 425 */
        "Failed Dependency" },
    { PCRDR_SC_UPGRADE_REQUIRED,    /* 426 */
        "Upgrade Required" },
    { PCRDR_SC_RETRY_WITH,          /* 449 */
        "Retry With" },
    { PCRDR_SC_UNAVAILABLE_FOR_LEGAL_REASONS,   /* 451 */
        "Unavailable For Legal Reasons" },
    { PCRDR_SC_INTERNAL_SERVER_ERROR,   /* 500 */
        "Internal Server Error" },
    { PCRDR_SC_NOT_IMPLEMENTED,     /* 501 */
        "Not Implemented" },
    { PCRDR_SC_BAD_CALLEE,          /* 502 */
        "Bad Callee" },
    { PCRDR_SC_SERVICE_UNAVAILABLE, /* 503 */
        "Service Unavailable" },
    { PCRDR_SC_CALLEE_TIMEOUT,      /* 504 */
        "Callee Timeout" },
    { PCRDR_SC_INSUFFICIENT_STORAGE,    /* 507 */
        "Insufficient Storage" },
};

#define TABLESIZE(table)    (sizeof(table)/sizeof(table[0]))

const char* pcrdr_get_ret_message (int ret_code)
{
    unsigned int lower = 0;
    unsigned int upper = TABLESIZE (ret_code_2_messages) - 1;
    int mid = TABLESIZE (ret_code_2_messages) / 2;

    if (ret_code < ret_code_2_messages[lower].ret_code ||
            ret_code > ret_code_2_messages[upper].ret_code)
        return UNKNOWN_RET_CODE;

    do {
        if (ret_code < ret_code_2_messages[mid].ret_code)
            upper = mid - 1;
        else if (ret_code > ret_code_2_messages[mid].ret_code)
            lower = mid + 1;
        else
            return ret_code_2_messages [mid].ret_msg;

        mid = (lower + upper) / 2;

    } while (lower <= upper);

    return UNKNOWN_RET_CODE;
}

int pcrdr_errcode_to_retcode (int err_code)
{
    switch (err_code) {
        case 0:
            return PCRDR_SC_OK;
        case PCRDR_ERROR_IO:
            return PCRDR_SC_IOERR;
        case PCRDR_ERROR_PEER_CLOSED:
            return PCRDR_SC_SERVICE_UNAVAILABLE;
        case PCRDR_ERROR_NOMEM:
            return PCRDR_SC_INSUFFICIENT_STORAGE;
        case PCRDR_ERROR_TOO_LARGE:
            return PCRDR_SC_PACKET_TOO_LARGE;
        case PCRDR_ERROR_PROTOCOL:
            return PCRDR_SC_UNPROCESSABLE_PACKET;
        case PCRDR_ERROR_NOT_IMPLEMENTED:
            return PCRDR_SC_NOT_IMPLEMENTED;
        case PCRDR_ERROR_INVALID_VALUE:
            return PCRDR_SC_BAD_REQUEST;
        case PCRDR_ERROR_DUPLICATED:
            return PCRDR_SC_CONFLICT;
        case PCRDR_ERROR_TOO_SMALL_BUFF:
            return PCRDR_SC_INSUFFICIENT_STORAGE;
        case PCRDR_ERROR_BAD_SYSTEM_CALL:
            return PCRDR_SC_INTERNAL_SERVER_ERROR;
        case PCRDR_ERROR_AUTH_FAILED:
            return PCRDR_SC_UNAUTHORIZED;
        case PCRDR_ERROR_SERVER_ERROR:
            return PCRDR_SC_INTERNAL_SERVER_ERROR;
        case PCRDR_ERROR_TIMEOUT:
            return PCRDR_SC_CALLEE_TIMEOUT;
        case PCRDR_ERROR_UNKNOWN_EVENT:
            return PCRDR_SC_NOT_FOUND;
        case PCRDR_ERROR_UNKNOWN_REQUEST:
            return PCRDR_SC_NOT_FOUND;
        default:
            break;
    }

    return PCRDR_SC_INTERNAL_SERVER_ERROR;
}

static const char *workspace_resnames[] = {
    PCRDR_RESNAME_WORKSPACE_default,
    PCRDR_RESNAME_WORKSPACE_active,
    PCRDR_RESNAME_WORKSPACE_first,
    PCRDR_RESNAME_WORKSPACE_last,
};

static const char * page_resnames[] = {
    PCRDR_RESNAME_PAGE_active,
    PCRDR_RESNAME_PAGE_first,
    PCRDR_RESNAME_PAGE_last,
};

/* make sure number of type_names matches the enums */
#define _COMPILE_TIME_ASSERT(name, x)           \
       typedef int _dummy_ ## name[(x) * 2 - 1]

_COMPILE_TIME_ASSERT(workspace,
        TABLESIZE(workspace_resnames) == PCRDR_NR_RESNAME_WORKSPACE);
_COMPILE_TIME_ASSERT(page,
        TABLESIZE(page_resnames) == PCRDR_NR_RESNAME_PAGE);

#undef _COMPILE_TIME_ASSERT

int pcrdr_check_reserved_workspace_name(const char *name)
{
    for (int i = 0; i < (int)TABLESIZE(workspace_resnames); i++) {
        if (strcmp(name, workspace_resnames[i]) == 0) {
            return PCRDR_K_RESNAME_WORKSPACE_FIRST + i;
        }
    }

    return -1;
}

int pcrdr_check_reserved_page_name(const char *name)
{
    for (int i = 0; i < (int)TABLESIZE(page_resnames); i++) {
        if (strcmp(name, page_resnames[i]) == 0) {
            return PCRDR_K_RESNAME_PAGE_FIRST + i;
        }
    }

    return -1;
}

