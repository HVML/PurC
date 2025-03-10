/*
 * @file stream-hbdbus.c
 * @author Vincent Wei
 * @date 2023/05/28
 * @brief The implementation of `HBDBus` protocol for stream object.
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
#include "config.h"

#if ENABLE(STREAM_HBDBUS)

#include "stream.h"

#include "purc-variant.h"
#include "purc-runloop.h"
#include "purc-dvobjs.h"

#include "private/instance.h"
#include "private/debug.h"
#include "private/dvobjs.h"
#include "private/atom-buckets.h"
#include "private/interpreter.h"
#include "private/kvlist.h"

#include <errno.h>

#define strncmp2ltr(str, literal, len) \
    ((len > (sizeof(literal "") - 1)) ? 1 :  \
        (len < (sizeof(literal "") - 1) ? -1 : strncmp(str, literal, len)))

#define strncasecmp2ltr(str, literal, len) \
    ((len > (sizeof(literal "") - 1)) ? 1 :  \
        (len < (sizeof(literal "") - 1) ? -1 : strncasecmp(str, literal, len)))

/* copy from hbdbus.h */
#define HBDBUS_PROTOCOL_NAME            "HBDBUS"
#define HBDBUS_PROTOCOL_VERSION         200
#define HBDBUS_MINIMAL_PROTOCOL_VERSION 200
#define HBDBUS_NOT_AVAILABLE            "<N/A>"

#define HBDBUS_LEN_HOST_NAME             PURC_LEN_HOST_NAME
#define HBDBUS_LEN_APP_NAME              PURC_LEN_APP_NAME
#define HBDBUS_LEN_RUNNER_NAME           PURC_LEN_RUNNER_NAME
#define HBDBUS_LEN_METHOD_NAME           PURC_LEN_IDENTIFIER
#define HBDBUS_LEN_BUBBLE_NAME           PURC_LEN_IDENTIFIER
#define HBDBUS_LEN_ENDPOINT_NAME         \
    (HBDBUS_LEN_HOST_NAME + HBDBUS_LEN_APP_NAME + HBDBUS_LEN_RUNNER_NAME + 3)
#define HBDBUS_LEN_UNIQUE_ID             PURC_LEN_UNIQUE_ID

#define HBDBUS_MIN_PACKET_BUFF_SIZE      512
#define HBDBUS_DEF_PACKET_BUFF_SIZE      1024
#define HBDBUS_DEF_TIME_EXPECTED         5   /* 5 seconds */

/* the maximal size of a payload in a frame (4KiB) */
#define HBDBUS_MAX_FRAME_PAYLOAD_SIZE    4096

/* the maximal size of a payload which will be held in memory (40KiB) */
#define HBDBUS_MAX_INMEM_PAYLOAD_SIZE    40960

/* the maximal time to ping client (60 seconds) */
#define HBDBUS_MAX_PING_TIME             60

/* the maximal no responding time (90 seconds) */
#define HBDBUS_MAX_NO_RESPONDING_TIME    90

#define HBDBUS_LOCALHOST                "localhost"
#define HBDBUS_APP_NAME                 "cn.fmsoft.hybridos.databus"
#define HBDBUS_RUN_MAIN                 "main"
#define HBDBUS_RUN_BUILITIN             "builtin"

#define HBDBUS_SYSTEM_EVENT_ID          "NOTIFICATION"

#define HBDBUS_METHOD_REGISTERPROCEDURE     "registerProcedure"
#define HBDBUS_METHOD_REVOKEPROCEDURE       "revokeProcedure"
#define HBDBUS_METHOD_REGISTEREVENT         "registerEvent"
#define HBDBUS_METHOD_REVOKEEVENT           "revokeEvent"
#define HBDBUS_METHOD_SUBSCRIBEEVENT        "subscribeEvent"
#define HBDBUS_METHOD_UNSUBSCRIBEEVENT      "unsubscribeEvent"
#define HBDBUS_METHOD_LISTENDPOINTS         "listEndpoints"
#define HBDBUS_METHOD_LISTPROCEDURES        "listProcedures"
#define HBDBUS_METHOD_LISTEVENTS            "listEvents"
#define HBDBUS_METHOD_LISTEVENTSUBSCRIBERS  "listEventSubscribers"
#define HBDBUS_METHOD_TERMINATE             "terminate"
#define HBDBUS_METHOD_ECHO                  "echo"

#define HBDBUS_BUBBLE_NEWENDPOINT           "NewEndpoint"
#define HBDBUS_BUBBLE_BROKENENDPOINT        "BrokenEndpoint"
#define HBDBUS_BUBBLE_LOSTEVENTGENERATOR    "LostEventGenerator"
#define HBDBUS_BUBBLE_LOSTEVENTBUBBLE       "LostEventBubble"
#define HBDBUS_BUBBLE_SYSTEMSHUTTINGDOWN    "SystemShuttingDown"

/* JSON packet types */
enum {
    JPT_BAD_JSON = -1,
    JPT_UNKNOWN = 0,
    JPT_ERROR,
    JPT_AUTH,
    JPT_AUTH_PASSED,
    JPT_AUTH_FAILED,
    JPT_CALL,
    JPT_RESULT,
    JPT_RESULT_SENT,
    JPT_EVENT,
    JPT_EVENT_SENT,
};

/* HBDBus connection state */
enum bus_state {
    BS_UNCERTAIN = 0,
    BS_EXPECT_CHALLENGE,
    BS_EXPECT_AUTH_RESULT,
    BS_EXPECT_REGULAR_MSG,
};

enum {
    ERR_CODE_first = 0,
#define ERR_SYMB_OK                     ""
    ERR_CODE_OK = ERR_CODE_first,
#define ERR_SYMB_AGAIN                  "again"
    ERR_CODE_AGAIN,
#define ERR_SYMB_BADMESSAGE             "badMessage"
    ERR_CODE_BADMESSAGE,
#define ERR_SYMB_BADMSGPAYLOAD          "badMsgPayload"
    ERR_CODE_BADMSGPAYLOAD,
#define ERR_SYMB_SERVERREFUSED          "serverRefused"
    ERR_CODE_SERVERREFUSED,
#define ERR_SYMB_SERVERERROR            "serverError"
    ERR_CODE_SERVERERROR,
#define ERR_SYMB_WRONGVERSION           "wrongVersion"
    ERR_CODE_WRONGVERSION,
#define ERR_SYMB_OUTOFMEMORY            "outOfMemory"
    ERR_CODE_OUTOFMEMORY,
#define ERR_SYMB_UNEXPECTED             "unexpected"
    ERR_CODE_UNEXPECTED,
#define ERR_SYMB_TOOSMALLBUFFER         "tooSmallBuffer"
    ERR_CODE_TOOSMALLBUFFER,
#define ERR_SYMB_FAILEDWRITE            "failedWrite"
    ERR_CODE_FAILEDWRITE,
#define ERR_SYMB_FAILEDREAD             "failedRead"
    ERR_CODE_FAILEDREAD,
#define ERR_SYMB_AUTHFAILED             "authFailed"
    ERR_CODE_AUTHFAILED,
#define ERR_SYMB_INVALIDPARAMS          "invalidParams"
    ERR_CODE_INVALIDPARAMS,
#define ERR_SYMB_CONFLICT               "confilct"
    ERR_CODE_CONFLICT,
#define ERR_SYMB_NOTFOUND               "notFound"
    ERR_CODE_NOTFOUND,
#define ERR_SYMB_UNKNOWNMESSAGE         "unknownMessage"
    ERR_CODE_UNKNOWNMESSAGE,

    ERR_CODE_last = ERR_CODE_NOTFOUND,
};

#define EVENT_TYPE_CALL                     "call"
#define EVENT_TYPE_RESULT                   "result"
#define EVENT_TYPE_EVENT                    "event"
#   define EVENT_SUBTYPE_SYSTEM             "SYSTEM"
#define EVENT_TYPE_STATE                    "state"
#   define EVENT_SUBTYPE_READY              "ready"

#define EVENT_TYPE_ERROR                    "error"
#   define EVENT_SUBTYPE_HBDBUS             "hbdbus"

#define EVENT_TYPE_CLOSE                    "close"

typedef int (*hbdbus_result_handler)(struct pcdvobjs_stream *stream,
        const void *ctxt, purc_variant_t jo);
typedef void (*hbdbus_event_handler)(struct pcdvobjs_stream *stream,
        const char *from_endpoint, const char *from_bubble,
        const char *bubble_data);

struct stream_extended_data {
    const struct pcinst *inst;

    int         errcode;
    const char *errsymb;

    enum bus_state state;

    char *srv_host_name;
    char *own_host_name;

    struct pcutils_kvlist method_list;
    struct pcutils_kvlist called_list;
    struct pcutils_kvlist calling_list;

    struct pcutils_kvlist bubble_list;
    struct pcutils_kvlist subscribed_list;

    int (*on_message_super)(struct pcdvobjs_stream *stream, int type,
            char *buf, size_t len, int *owner_taken);
    void (*cleanup_super)(struct pcdvobjs_stream *stream);
};

#define set_error(ext, symb)            \
    do {                                \
        ext->errcode = ERR_CODE_##symb; \
        ext->errsymb = ERR_SYMB_##symb; \
    } while (0)

#define clr_error(ext)          \
    do {                        \
        ext->errcode = 0;       \
        ext->errsymb = NULL;    \
    } while (0)

#define call_super(stream, method, x, ...)                      \
    ((struct stream_messaging_ops*)stream->ext0.msg_ops)->      \
        method(x, ##__VA_ARGS__)

static int get_purc_errcode(struct pcdvobjs_stream *stream)
{
    int errcode = stream->ext1.data->errcode;
    int pcerr = PURC_ERROR_UNKNOWN;

    switch (errcode) {
    case ERR_CODE_OK:
        pcerr = PURC_ERROR_OK;
        break;
    case ERR_CODE_AGAIN:
        pcerr = PURC_ERROR_AGAIN;
        break;
    case ERR_CODE_BADMESSAGE:
        pcerr = PURC_ERROR_NOT_DESIRED_ENTITY;
        break;
    case ERR_CODE_BADMSGPAYLOAD:
        pcerr = PURC_ERROR_INVALID_VALUE;
        break;
    case ERR_CODE_SERVERREFUSED:
        pcerr = PURC_ERROR_NOT_ALLOWED;
        break;
    case ERR_CODE_SERVERERROR:
        pcerr = PURC_ERROR_REQUEST_FAILED;
        break;
    case ERR_CODE_WRONGVERSION:
        pcerr = PURC_ERROR_MISMATCHED_VERSION;
        break;
    case ERR_CODE_OUTOFMEMORY:
        pcerr = PURC_ERROR_OUT_OF_MEMORY;
        break;
    case ERR_CODE_UNEXPECTED:
        pcerr = PURC_ERROR_NOT_ACCEPTABLE;
        break;
    case ERR_CODE_TOOSMALLBUFFER:
        pcerr = PURC_ERROR_TOO_SMALL_BUFF;
        break;
    case ERR_CODE_FAILEDWRITE:
        pcerr = PURC_ERROR_IO_FAILURE;
        break;
    case ERR_CODE_FAILEDREAD:
        pcerr = PURC_ERROR_IO_FAILURE;
        break;
    case ERR_CODE_AUTHFAILED:
        pcerr = PURC_ERROR_ACCESS_DENIED;
        break;
    case ERR_CODE_INVALIDPARAMS:
        pcerr = PURC_ERROR_INVALID_VALUE;
        break;
    case ERR_CODE_CONFLICT:
        pcerr = PURC_ERROR_CONFLICT;
        break;
    case ERR_CODE_NOTFOUND:
        pcerr = PURC_ERROR_NOT_FOUND;
        break;
    }

    return pcerr;
}

static inline bool
hbdbus_is_valid_method_name(const char *method_name)
{
    return purc_is_valid_token(method_name, HBDBUS_LEN_METHOD_NAME);
}

static inline bool
hbdbus_is_valid_bubble_name(const char *bubble_name)
{
    return purc_is_valid_token(bubble_name, HBDBUS_LEN_BUBBLE_NAME);
}

static bool hbdbus_is_valid_wildcard_pattern_list(const char* pattern)
{
    if (*pattern == '!')
        pattern++;
    else if (*pattern == '$')
        return purc_is_valid_token(++pattern, 0);

    while (*pattern) {

        if (!isalnum(*pattern) && *pattern != '_'
                && *pattern != '*' && *pattern != '?' && *pattern != '.'
                && *pattern != ',' && *pattern != ';' && *pattern != ' ')
            return false;

        pattern++;
    }

    return true;
}

struct calling_procedure_info {
    time_t                  calling_time;
    int                     time_expected;

    char                   *method;
    void                   *ctxt;
    hbdbus_result_handler   handler;
};

static int
free_cpi(void *ctxt, const char *name, void *data)
{
    (void)ctxt;
    (void)name;

    struct calling_procedure_info *cpi = data;
    if (cpi->method) {
        free(cpi->method);
        cpi->method = NULL;
    }
    if (cpi->ctxt) {
        free(cpi->ctxt);
        cpi->ctxt = NULL;
    }

    return 0;
}

static size_t
get_cpi_len(struct pcutils_kvlist *kv, const void *data)
{
    (void)kv;
    (void)data;
    return sizeof(struct calling_procedure_info);
}

struct method_called_info {
    struct timespec called_ts;
    char           *method;
    char           *call_id;
};

static size_t
get_mci_len(struct pcutils_kvlist *kv, const void *data)
{
    (void)kv;
    (void)data;
    return sizeof(struct method_called_info);
}

static int
free_mci(void *ctxt, const char *name, void *data)
{
    (void)ctxt;
    (void)name;

    struct method_called_info *mci = data;
    if (mci->method) {
        free(mci->method);
        mci->method = NULL;
    }
    if (mci->call_id) {
        free(mci->call_id);
        mci->call_id = NULL;
    }

    return 0;
}

static int
call_procedure(pcdvobjs_stream *stream,
        const char* endpoint, const char* method,
        const char* param, int time_expected,
        void *ctxt, hbdbus_result_handler result_handler)
{
    int n, retv = 0;
    char call_id_buf[HBDBUS_LEN_UNIQUE_ID + 1];
    char *buff;
    char* escaped_param = NULL;
    struct stream_extended_data *ext = stream->ext1.data;

    if (param[0]) {
        escaped_param = pcutils_escape_string_for_json(param);
        if (escaped_param == NULL) {
            set_error(ext, OUTOFMEMORY);
            goto failed;
        }
    }

    purc_generate_unique_id(call_id_buf, "call");
    n = asprintf(&buff,
            "{"
            "\"packetType\": \"call\","
            "\"callId\": \"%s\","
            "\"toEndpoint\": \"%s\","
            "\"toMethod\": \"%s\","
            "\"expectedTime\": %d,"
            "\"parameter\": \"%s\""
            "}",
            call_id_buf,
            endpoint,
            method,
            time_expected,
            escaped_param ? escaped_param : param);
    if (escaped_param)
        free(escaped_param);

    if (n < 0) {
        set_error(ext, OUTOFMEMORY);
        goto failed;
    }

    retv = call_super(stream, send_message, stream, true, buff, n);
    free(buff);
    if (retv == 0) {
        struct calling_procedure_info cpi;
        cpi.calling_time = purc_monotonic_time_after(0);
        cpi.time_expected = time_expected;
        cpi.method = strdup(method);
        cpi.ctxt = ctxt;
        cpi.handler = result_handler;

        const char* p;
        p = pcutils_kvlist_set_ex(&ext->calling_list, call_id_buf, &cpi);
        if (p == NULL) {
            set_error(ext, OUTOFMEMORY);
            goto failed;
        }
    }
    else {
        PC_ERROR("Failed to send message: %d\n", retv);
        goto failed_sending;
    }

    return 0;

failed:
    retv = get_purc_errcode(stream);

failed_sending:
    return retv;
}

static purc_variant_t
call_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    struct pcdvobjs_stream *stream = entity;
    const char *endpoint = NULL;
    const char *method = NULL;
    const char *param = NULL;
    int time_expected = HBDBUS_DEF_TIME_EXPECTED;

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    endpoint = purc_variant_get_string_const(argv[0]);
    if (endpoint == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (!purc_is_valid_endpoint_name(endpoint)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    method = purc_variant_get_string_const(argv[1]);
    if (method == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (!hbdbus_is_valid_method_name(method)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (nr_args > 2) {
        param = purc_variant_get_string_const(argv[2]);
    }

    if (param == NULL)
        param = "";     /* the default parameter */

    if (nr_args > 3) {
        purc_variant_cast_to_int32(argv[3], &time_expected, false);
    }

    if (stream->ext1.data == NULL) {
        purc_set_error(PURC_ERROR_ENTITY_GONE);
        goto failed;
    }

    clr_error(stream->ext1.data);
    int retv = call_procedure(stream, endpoint, method, param, time_expected,
                NULL, NULL);
    if (retv) {
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

static int
builtin_result_handler(struct pcdvobjs_stream *stream, const void *ctxt,
        purc_variant_t jo)
{
    struct stream_extended_data *ext = stream->ext1.data;
    purc_variant_t jo_tmp;
    int ret_code;

    if ((jo_tmp = purc_variant_object_get_by_ckey (jo, "retCode")) &&
            purc_variant_cast_to_int32(jo_tmp, &ret_code, false)) {
    }
    else {
        set_error(ext, BADMSGPAYLOAD);
        goto failed;
    }

    if (ret_code == PCRDR_SC_OK) {
        char *semicolon = strchr(ctxt, ':');
        assert(semicolon);

        size_t len = semicolon - (const char *)ctxt;
        void *data = NULL;
        if (strncmp2ltr(ctxt, HBDBUS_METHOD_REGISTEREVENT, len) == 0) {
            pcutils_kvlist_set(&ext->bubble_list, semicolon + 1, &data);
        }
        else if (strncmp2ltr(ctxt, HBDBUS_METHOD_REVOKEEVENT, len) == 0) {
            pcutils_kvlist_remove(&ext->bubble_list, semicolon + 1);
        }
        else if (strncmp2ltr(ctxt, HBDBUS_METHOD_REGISTERPROCEDURE, len) == 0) {
            pcutils_kvlist_set(&ext->method_list, semicolon + 1, &data);
        }
        else if (strncmp2ltr(ctxt, HBDBUS_METHOD_REVOKEPROCEDURE, len) == 0) {
            pcutils_kvlist_remove(&ext->method_list, semicolon + 1);
        }
        else if (strncmp2ltr(ctxt, HBDBUS_METHOD_SUBSCRIBEEVENT, len) == 0) {
            pcutils_kvlist_set(&ext->subscribed_list, semicolon + 1, &data);
        }
        else if (strncmp2ltr(ctxt, HBDBUS_METHOD_UNSUBSCRIBEEVENT, len) == 0) {
            pcutils_kvlist_remove(&ext->subscribed_list, semicolon + 1);
        }
    }
    else {
        /* fire an `error:hbdbus` event */
        purc_variant_t data = purc_variant_make_object_0();
        if (data) {
            purc_variant_t tmp;

            tmp = purc_variant_object_get_by_ckey(jo, "retCode");
            if (tmp)
                purc_variant_object_set_by_static_ckey(data, "retCode", tmp);

            tmp = purc_variant_object_get_by_ckey(jo, "retMsg");
            if (tmp)
                purc_variant_object_set_by_static_ckey(data, "retMsg", tmp);
        }

        pcintr_coroutine_post_event(stream->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
                EVENT_TYPE_ERROR, EVENT_SUBTYPE_HBDBUS,
                data, PURC_VARIANT_INVALID);
        if (data)
            purc_variant_unref(data);
    }

    return 0;

failed:
    return -1;
}

static int subscribe_event(pcdvobjs_stream *stream, const char *endpoint,
        const char *bubble)
{
    struct stream_extended_data *ext = stream->ext1.data;
    char builtin_name[HBDBUS_LEN_ENDPOINT_NAME + 1];
    char param_buff[HBDBUS_MIN_PACKET_BUFF_SIZE];
    char event_name[HBDBUS_LEN_ENDPOINT_NAME + HBDBUS_LEN_BUBBLE_NAME + 2];
    int retv = 0;

    int n = purc_name_tolower_copy(endpoint, event_name, HBDBUS_LEN_ENDPOINT_NAME);
    event_name [n++] = '/';
    event_name [n] = '\0';
    strcpy(event_name + n, bubble);
    if (pcutils_kvlist_get(&ext->subscribed_list, event_name)) {
        set_error(ext, CONFLICT);
        goto failed;
    }

    n = snprintf(param_buff, sizeof(param_buff),
            "{"
            "\"endpointName\": \"%s\","
            "\"bubbleName\": \"%s\""
            "}",
            endpoint,
            bubble);

    if (n < 0) {
        set_error(ext, UNEXPECTED);
        goto failed;
    }
    else if ((size_t)n >= sizeof (param_buff)) {
        set_error(ext, TOOSMALLBUFFER);
        goto failed;
    }

    purc_assemble_endpoint_name(ext->srv_host_name,
            HBDBUS_APP_NAME, HBDBUS_RUN_BUILITIN, builtin_name);
    char *ctxt;
    if (asprintf(&ctxt, HBDBUS_METHOD_SUBSCRIBEEVENT ":%s", event_name) < 0) {
        set_error(ext, OUTOFMEMORY);
        goto failed;
    }

    retv = call_procedure(stream, builtin_name,
                    HBDBUS_METHOD_SUBSCRIBEEVENT, param_buff,
                    HBDBUS_DEF_TIME_EXPECTED,
                    ctxt, builtin_result_handler);
    if (retv) {
        free(ctxt);
        goto failed_sending;
    }

    return 0;

failed:
    retv = get_purc_errcode(stream);

failed_sending:
    return retv;
}

static purc_variant_t
subscribe_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    struct pcdvobjs_stream *stream = entity;
    const char *endpoint = NULL;
    const char *bubble = NULL;

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    endpoint = purc_variant_get_string_const(argv[0]);
    if (endpoint == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (!purc_is_valid_endpoint_name(endpoint)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    bubble = purc_variant_get_string_const(argv[1]);
    if (bubble == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (!hbdbus_is_valid_bubble_name(bubble)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (stream->ext1.data == NULL) {
        purc_set_error(PURC_ERROR_ENTITY_GONE);
        goto failed;
    }

    clr_error(stream->ext1.data);
    int retv = subscribe_event(stream, endpoint, bubble);
    if (retv) {
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

static int unsubscribe_event(pcdvobjs_stream *stream, const char *endpoint,
        const char *bubble)
{
    struct stream_extended_data *ext = stream->ext1.data;
    char builtin_name[HBDBUS_LEN_ENDPOINT_NAME + 1];
    char param_buff[HBDBUS_MIN_PACKET_BUFF_SIZE];
    char event_name[HBDBUS_LEN_ENDPOINT_NAME + HBDBUS_LEN_BUBBLE_NAME + 2];
    int retv = 0;

    int n = purc_name_tolower_copy(endpoint, event_name, HBDBUS_LEN_ENDPOINT_NAME);
    event_name [n++] = '/';
    event_name [n] = '\0';
    strcpy(event_name + n, bubble);
    if (pcutils_kvlist_get(&ext->subscribed_list, event_name)) {
        set_error(ext, CONFLICT);
        goto failed;
    }

    n = snprintf(param_buff, sizeof(param_buff),
            "{"
            "\"endpointName\": \"%s\","
            "\"bubbleName\": \"%s\""
            "}",
            endpoint,
            bubble);

    if (n < 0) {
        set_error(ext, UNEXPECTED);
        goto failed;
    }
    else if ((size_t)n >= sizeof (param_buff)) {
        set_error(ext, TOOSMALLBUFFER);
        goto failed;
    }

    purc_assemble_endpoint_name(ext->srv_host_name,
            HBDBUS_APP_NAME, HBDBUS_RUN_BUILITIN, builtin_name);
    char *ctxt;
    if (asprintf(&ctxt, HBDBUS_METHOD_UNSUBSCRIBEEVENT ":%s", event_name) < 0) {
        set_error(ext, OUTOFMEMORY);
        goto failed;
    }

    retv = call_procedure(stream, builtin_name,
                    HBDBUS_METHOD_UNSUBSCRIBEEVENT, param_buff,
                    HBDBUS_DEF_TIME_EXPECTED,
                    ctxt, builtin_result_handler);
    if (retv) {
        free(ctxt);
        goto failed_sending;
    }

    return 0;

failed:
    retv = get_purc_errcode(stream);

failed_sending:
    return retv;
}

static purc_variant_t
unsubscribe_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    struct pcdvobjs_stream *stream = entity;
    const char *endpoint = NULL;
    const char *bubble = NULL;

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    endpoint = purc_variant_get_string_const(argv[0]);
    if (endpoint == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (!purc_is_valid_endpoint_name(endpoint)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    bubble = purc_variant_get_string_const(argv[1]);
    if (bubble == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (!hbdbus_is_valid_bubble_name(bubble)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (stream->ext1.data == NULL) {
        purc_set_error(PURC_ERROR_ENTITY_GONE);
        goto failed;
    }

    clr_error(stream->ext1.data);
    int retv = unsubscribe_event(stream, endpoint, bubble);
    if (retv) {
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

static int fire_event(pcdvobjs_stream *stream,
        const char* bubble_name, const char* bubble_data)
{
    struct stream_extended_data *ext = stream->ext1.data;
    char event_id[HBDBUS_LEN_UNIQUE_ID + 1];
    char *packet_buff;
    char *escaped_data;
    int retv = 0;

    if (!pcutils_kvlist_get(&ext->bubble_list, bubble_name)) {
        set_error(ext, CONFLICT);
        goto failed;
    }

    if (bubble_data[0]) {
        escaped_data = pcutils_escape_string_for_json (bubble_data);
        if (escaped_data == NULL) {
            set_error(ext, OUTOFMEMORY);
            goto failed;
        }
    }
    else
        escaped_data = NULL;

    purc_generate_unique_id(event_id, "event");
    int n = asprintf(&packet_buff,
            "{"
            "\"packetType\": \"event\","
            "\"eventId\": \"%s\","
            "\"bubbleName\": \"%s\","
            "\"bubbleData\": \"%s\""
            "}",
            event_id,
            bubble_name,
            escaped_data ? escaped_data : bubble_data);
    if (escaped_data)
        free(escaped_data);

    if (n < 0) {
        set_error(ext, OUTOFMEMORY);
        goto failed;
    }

    retv = call_super(stream, send_message, stream, true, packet_buff, n);
    free(packet_buff);
    if (retv) {
        PC_ERROR("Failed to send text message to HBDBus server.\n");
        goto failed_sending;
    }

    return 0;

failed:
    retv = get_purc_errcode(stream);

failed_sending:
    return retv;
}

static purc_variant_t
fire_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    struct pcdvobjs_stream *stream = entity;
    const char *bubble_name = NULL;
    const char *bubble_data = NULL;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    bubble_name = purc_variant_get_string_const(argv[0]);
    if (bubble_name == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (!hbdbus_is_valid_bubble_name(bubble_name)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (nr_args > 1) {
        bubble_data = purc_variant_get_string_const(argv[1]);
    }

    if (bubble_data == NULL) {
        bubble_data = "";
    }

    if (stream->ext1.data == NULL) {
        purc_set_error(PURC_ERROR_ENTITY_GONE);
        goto failed;
    }

    clr_error(stream->ext1.data);
    int retv = fire_event(stream, bubble_name, bubble_data);
    if (retv) {
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

static int register_event(pcdvobjs_stream *stream, const char* bubble_name,
        const char* for_host, const char* for_app)
{
    struct stream_extended_data *ext = stream->ext1.data;
    char endpoint_name[HBDBUS_LEN_ENDPOINT_NAME + 1];
    char *param_buff;
    int retv = 0;

    if (pcutils_kvlist_get(&ext->bubble_list, bubble_name)) {
        set_error(ext, CONFLICT);
        goto failed;
    }

    int n = asprintf(&param_buff,
            "{"
            "\"bubbleName\": \"%s\","
            "\"forHost\": \"%s\","
            "\"forApp\": \"%s\""
            "}",
            bubble_name,
            for_host, for_app);

    if (n < 0) {
        set_error(ext, OUTOFMEMORY);
        goto failed;
    }

    purc_assemble_endpoint_name(ext->srv_host_name,
            HBDBUS_APP_NAME, HBDBUS_RUN_BUILITIN, endpoint_name);
    char *ctxt;
    if (asprintf(&ctxt, HBDBUS_METHOD_REGISTEREVENT ":%s", bubble_name) < 0) {
        free(param_buff);
        set_error(ext, OUTOFMEMORY);
        goto failed;
    }

    retv = call_procedure(stream, endpoint_name,
                    HBDBUS_METHOD_REGISTEREVENT, param_buff,
                    HBDBUS_DEF_TIME_EXPECTED,
                    ctxt, builtin_result_handler);
    free(param_buff);

    if (retv) {
        free(ctxt);
        goto failed_sending;
    }

    return 0;

failed:
    retv = get_purc_errcode(stream);

failed_sending:
    return retv;
}

static purc_variant_t
register_event_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    struct pcdvobjs_stream *stream = entity;
    const char *bubble_name = NULL;
    const char *for_host = NULL;
    const char *for_app = NULL;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    bubble_name = purc_variant_get_string_const(argv[0]);
    if (bubble_name == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (!hbdbus_is_valid_bubble_name(bubble_name)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (nr_args > 1) {
        for_host = purc_variant_get_string_const(argv[1]);
        if (for_host) {
            if (!hbdbus_is_valid_wildcard_pattern_list(for_host)) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }
        }
    }

    if (for_host == NULL) {
        for_host = "*";
    }

    if (nr_args > 2) {
        for_app = purc_variant_get_string_const(argv[2]);
        if (for_app) {
            if (!hbdbus_is_valid_wildcard_pattern_list(for_app)) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }
        }
    }

    if (for_app == NULL) {
        for_app = "*";
    }

    if (stream->ext1.data == NULL) {
        purc_set_error(PURC_ERROR_ENTITY_GONE);
        goto failed;
    }

    clr_error(stream->ext1.data);
    int retv = register_event(stream, bubble_name, for_host, for_app);
    if (retv) {
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

static int revoke_event(pcdvobjs_stream *stream, const char* bubble_name)
{
    struct stream_extended_data *ext = stream->ext1.data;
    char endpoint_name[HBDBUS_LEN_ENDPOINT_NAME + 1];
    char param_buff[HBDBUS_MIN_PACKET_BUFF_SIZE];
    int retv = 0;

    if (pcutils_kvlist_get(&ext->bubble_list, bubble_name) == NULL) {
        set_error(ext, NOTFOUND);
        goto failed;
    }

    int n = snprintf(param_buff, sizeof(param_buff),
            "{"
            "\"bubbleName\": \"%s\""
            "}",
            bubble_name);

    if (n < 0) {
        set_error(ext, UNEXPECTED);
        goto failed;
    }
    else if ((size_t)n >= sizeof(param_buff)) {
        set_error(ext, TOOSMALLBUFFER);
        goto failed;
    }

    purc_assemble_endpoint_name(ext->srv_host_name,
            HBDBUS_APP_NAME, HBDBUS_RUN_BUILITIN, endpoint_name);
    char *ctxt;
    if (asprintf(&ctxt, HBDBUS_METHOD_REVOKEEVENT ":%s", bubble_name) < 0) {
        set_error(ext, OUTOFMEMORY);
        goto failed;
    }

    retv = call_procedure(stream, endpoint_name,
                    HBDBUS_METHOD_REVOKEEVENT, param_buff,
                    HBDBUS_DEF_TIME_EXPECTED,
                    ctxt, builtin_result_handler);
    if (retv) {
        free(ctxt);
        goto failed_sending;
    }

    return 0;

failed:
    retv = get_purc_errcode(stream);

failed_sending:
    return retv;
}

static purc_variant_t
revoke_event_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    struct pcdvobjs_stream *stream = entity;
    const char *bubble_name = NULL;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    bubble_name = purc_variant_get_string_const(argv[0]);
    if (bubble_name == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (!hbdbus_is_valid_bubble_name(bubble_name)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (stream->ext1.data == NULL) {
        purc_set_error(PURC_ERROR_ENTITY_GONE);
        goto failed;
    }

    clr_error(stream->ext1.data);
    int retv = revoke_event(stream, bubble_name);
    if (retv) {
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

static int register_procedure(pcdvobjs_stream *stream, const char* method_name,
        const char* for_host, const char* for_app)
{
    struct stream_extended_data *ext = stream->ext1.data;
    char endpoint_name[HBDBUS_LEN_ENDPOINT_NAME + 1];
    char *param_buff;
    int retv = 0;

    if (pcutils_kvlist_get(&ext->method_list, method_name)) {
        set_error(ext, CONFLICT);
        goto failed;
    }

    int n = asprintf(&param_buff,
            "{"
            "\"methodName\": \"%s\","
            "\"forHost\": \"%s\","
            "\"forApp\": \"%s\""
            "}",
            method_name,
            for_host, for_app);

    if (n < 0) {
        set_error(ext, OUTOFMEMORY);
        goto failed;
    }

    purc_assemble_endpoint_name(ext->srv_host_name,
            HBDBUS_APP_NAME, HBDBUS_RUN_BUILITIN, endpoint_name);
    char *ctxt;
    if (asprintf(&ctxt, HBDBUS_METHOD_REGISTERPROCEDURE ":%s", method_name) < 0) {
        free(param_buff);
        set_error(ext, OUTOFMEMORY);
        goto failed;
    }

    retv = call_procedure(stream, endpoint_name,
                    HBDBUS_METHOD_REGISTERPROCEDURE, param_buff,
                    HBDBUS_DEF_TIME_EXPECTED,
                    ctxt, builtin_result_handler);
    free(param_buff);

    if (retv) {
        free(ctxt);
        goto failed_sending;
    }

    return 0;

failed:
    retv = get_purc_errcode(stream);

failed_sending:
    return retv;
}

static purc_variant_t
register_procedure_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    struct pcdvobjs_stream *stream = entity;
    const char *method_name = NULL;
    const char *for_host = NULL;
    const char *for_app = NULL;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    method_name = purc_variant_get_string_const(argv[0]);
    if (method_name == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (!hbdbus_is_valid_method_name(method_name)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (nr_args > 1) {
        for_host = purc_variant_get_string_const(argv[1]);
        if (for_host) {
            if (!hbdbus_is_valid_wildcard_pattern_list(for_host)) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }
        }
    }

    if (for_host == NULL) {
        for_host = "*";
    }

    if (nr_args > 2) {
        for_app = purc_variant_get_string_const(argv[2]);
        if (for_app) {
            if (!hbdbus_is_valid_wildcard_pattern_list(for_app)) {
                purc_set_error(PURC_ERROR_INVALID_VALUE);
                goto failed;
            }
        }
    }

    if (for_app == NULL) {
        for_app = "*";
    }

    if (stream->ext1.data == NULL) {
        purc_set_error(PURC_ERROR_ENTITY_GONE);
        goto failed;
    }

    clr_error(stream->ext1.data);
    int retv = register_procedure(stream, method_name, for_host, for_app);
    if (retv) {
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

static int revoke_procedure(pcdvobjs_stream *stream, const char* method_name)
{
    struct stream_extended_data *ext = stream->ext1.data;
    char endpoint_name[HBDBUS_LEN_ENDPOINT_NAME + 1];
    char param_buff[HBDBUS_MIN_PACKET_BUFF_SIZE];
    int retv = 0;

    if (pcutils_kvlist_get(&ext->method_list, method_name) == NULL) {
        set_error(ext, NOTFOUND);
        goto failed;
    }

    int n = snprintf(param_buff, sizeof(param_buff),
            "{"
            "\"methodName\": \"%s\""
            "}",
            method_name);

    if (n < 0) {
        set_error(ext, UNEXPECTED);
        goto failed;
    }
    else if ((size_t)n >= sizeof(param_buff)) {
        set_error(ext, TOOSMALLBUFFER);
        goto failed;
    }

    purc_assemble_endpoint_name(ext->srv_host_name,
            HBDBUS_APP_NAME, HBDBUS_RUN_BUILITIN, endpoint_name);
    char *ctxt;
    if (asprintf(&ctxt, HBDBUS_METHOD_REVOKEPROCEDURE ":%s", method_name) < 0) {
        set_error(ext, OUTOFMEMORY);
        goto failed;
    }

    retv = call_procedure(stream, endpoint_name,
                    HBDBUS_METHOD_REVOKEPROCEDURE, param_buff,
                    HBDBUS_DEF_TIME_EXPECTED,
                    ctxt, builtin_result_handler);
    if (retv) {
        free(ctxt);
        goto failed_sending;
    }

    return 0;

failed:
    retv = get_purc_errcode(stream);

failed_sending:
    return retv;
}

static purc_variant_t
revoke_procedure_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    struct pcdvobjs_stream *stream = entity;
    const char *method_name = NULL;

    if (nr_args < 1) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    method_name = purc_variant_get_string_const(argv[0]);
    if (method_name == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (!hbdbus_is_valid_method_name(method_name)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (stream->ext1.data == NULL) {
        purc_set_error(PURC_ERROR_ENTITY_GONE);
        goto failed;
    }

    clr_error(stream->ext1.data);
    int retv = revoke_procedure(stream, method_name);
    if (retv) {
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

static int send_result(pcdvobjs_stream *stream, const char* result_id,
        const char *ret_value, int ret_code)
{
    struct stream_extended_data *ext = stream->ext1.data;
    struct method_called_info mci = { };
    int retv = 0;

    void *data;
    data = pcutils_kvlist_get(&ext->called_list, result_id);
    if (data == NULL) {
        set_error(ext, NOTFOUND);
        goto failed;
    }

    mci = *(struct method_called_info *)data;
    pcutils_kvlist_remove(&ext->called_list, result_id);

    double time_consumed = purc_get_elapsed_seconds(&mci.called_ts, NULL);

    char *escaped_value = NULL;
    if (ret_value[0]) {
        escaped_value = pcutils_escape_string_for_json(ret_value);
        if (escaped_value == NULL) {
            set_error(ext, OUTOFMEMORY);
            goto failed;
        }
    }

    char *buf;
    int n = asprintf(&buf,
            "{"
            "\"packetType\": \"result\","
            "\"resultId\": \"%s\","
            "\"callId\": \"%s\","
            "\"fromMethod\": \"%s\","
            "\"timeConsumed\": %.9f,"
            "\"retCode\": %d,"
            "\"retMsg\": \"%s\","
            "\"retValue\": \"%s\""
            "}",
            result_id, mci.call_id,
            mci.method,
            time_consumed,
            ret_code,
            pcrdr_get_ret_message(ret_code),
            escaped_value ? escaped_value : ret_value);
    if (escaped_value)
        free(escaped_value);

    if (n < 0) {
        set_error(ext, OUTOFMEMORY);
        goto failed;
    }

    retv = call_super(stream, send_message, stream, true, buf, n);
    free(buf);
    if (retv) {
        goto failed_sending;
    }

failed:
    retv = get_purc_errcode(stream);

failed_sending:
    if (mci.method)
        free(mci.method);
    if (mci.call_id)
        free(mci.call_id);

    return retv;
}

static purc_variant_t
send_result_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    struct pcdvobjs_stream *stream = entity;
    const char *result_id = NULL;
    const char *ret_value = NULL;
    int ret_code = PCRDR_SC_OK;

    if (nr_args < 2) {
        purc_set_error(PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    result_id = purc_variant_get_string_const(argv[0]);
    if (result_id == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    ret_value = purc_variant_get_string_const(argv[1]);
    if (ret_value == NULL) {
        purc_set_error(PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (nr_args > 2) {
        purc_variant_cast_to_int32(argv[2], &ret_code, false);
    }

    if (stream->ext1.data == NULL) {
        purc_set_error(PURC_ERROR_ENTITY_GONE);
        goto failed;
    }

    clr_error(stream->ext1.data);
    int retv = send_result(stream, result_id, ret_value, ret_code);
    if (retv) {
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

static void cleanup_extension(struct pcdvobjs_stream *stream)
{
    struct stream_extended_data *ext = stream->ext1.data;

    if (ext) {
        if (ext->cleanup_super)
            ext->cleanup_super(stream);

        if (ext->srv_host_name)
            free(ext->srv_host_name);
        free(ext->own_host_name);

        pcutils_kvlist_cleanup(&ext->method_list);

        size_t n;
        n = pcutils_kvlist_for_each(&ext->called_list, NULL, free_mci);
        PC_INFO("Not handled procedure calls: %u\n", (unsigned)n);
        pcutils_kvlist_cleanup(&ext->called_list);

        pcutils_kvlist_cleanup(&ext->bubble_list);

        n = pcutils_kvlist_for_each(&ext->calling_list, NULL, free_cpi);
        PC_INFO("Not returned procedure calls: %u\n", (unsigned)n);
        pcutils_kvlist_cleanup(&ext->calling_list);

        pcutils_kvlist_cleanup(&ext->subscribed_list);
        free(ext);
        stream->ext1.data = NULL;
    }
}

static purc_variant_t
close_getter(void *entity, const char *property_name,
        size_t nr_args, purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    struct pcdvobjs_stream *stream = entity;
    struct stream_extended_data *ext = stream->ext1.data;

    if (ext == NULL) {
        purc_set_error(PURC_ERROR_ENTITY_GONE);
        goto failed;
    }

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

    if (name == NULL) {
        goto failed;
    }

    switch (name[0]) {
    case 'c':
        if (strcmp(name, "call") == 0) {
            method = call_getter;
        }
        else if (strcmp(name, "close") == 0) {
            method = close_getter;
        }
        break;

    case 'f':
        if (strcmp(name, "fire") == 0) {
            method = fire_getter;
        }
        break;

    case 's':
        if (strcmp(name, "subscribe") == 0) {
            method = subscribe_getter;
        }
        else if (strcmp(name, "send_result") == 0) {
            method = send_result_getter;
        }
        break;

    case 'r':
        if (strcmp(name, "register_evnt") == 0) {
            method = register_event_getter;
        }
        else if (strcmp(name, "revoke_evnt") == 0) {
            method = revoke_event_getter;
        }
        else if (strcmp(name, "register_proc") == 0) {
            method = register_procedure_getter;
        }
        else if (strcmp(name, "revoke_proc") == 0) {
            method = revoke_procedure_getter;
        }
        break;

    case 'u':
        if (strcmp(name, "unsubscribe") == 0) {
            method = unsubscribe_getter;
        }
        break;

    default:
        goto failed;
    }

    if (method == NULL && stream->ext1.super_ops->property_getter) {
        return stream->ext1.super_ops->property_getter(entity, name);
    }

    return method;

failed:
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

static bool on_observe(void *entity, const char *event_name,
        const char *event_subname)
{
    (void)entity;
    (void)event_name;
    (void)event_subname;

    return true;
}

static bool on_forget(void *entity, const char *event_name,
        const char *event_subname)
{
    (void)entity;
    (void)event_name;
    (void)event_subname;

    return true;
}

static void on_release(void *entity)
{
    struct pcdvobjs_stream *stream = entity;
    struct purc_native_ops *super_ops = stream->ext1.super_ops;

    cleanup_extension(stream);
    if (super_ops->on_release) {
        return super_ops->on_release(entity);
    }
}

static struct purc_native_ops hbdbus_ops = {
    .property_getter = property_getter,
    .on_observe = on_observe,
    .on_forget = on_forget,
    .on_release = on_release,
};

static int hbdbus_json_packet_to_object(const char* json, unsigned int json_len,
        purc_variant_t *jo)
{
    int jpt = JPT_BAD_JSON;
    purc_variant_t jo_tmp;

    *jo = purc_variant_make_from_json_string(json, json_len);
    if (*jo == NULL || !purc_variant_is_object(*jo)) {
        goto failed;
    }

    if ((jo_tmp = purc_variant_object_get_by_ckey(*jo, "packetType"))) {
        const char *pack_type;
        pack_type = purc_variant_get_string_const(jo_tmp);

        if (pack_type == NULL)
            goto failed;

        if (strcasecmp(pack_type, "error") == 0) {
            jpt = JPT_ERROR;
        }
        else if (strcasecmp(pack_type, "auth") == 0) {
            jpt = JPT_AUTH;
        }
        else if (strcasecmp(pack_type, "authPassed") == 0) {
            jpt = JPT_AUTH_PASSED;
        }
        else if (strcasecmp(pack_type, "authFailed") == 0) {
            jpt = JPT_AUTH_FAILED;
        }
        else if (strcasecmp(pack_type, "call") == 0) {
            jpt = JPT_CALL;
        }
        else if (strcasecmp(pack_type, "result") == 0) {
            jpt = JPT_RESULT;
        }
        else if (strcasecmp(pack_type, "resultSent") == 0) {
            jpt = JPT_RESULT_SENT;
        }
        else if (strcasecmp(pack_type, "event") == 0) {
            jpt = JPT_EVENT;
        }
        else if (strcasecmp(pack_type, "eventSent") == 0) {
            jpt = JPT_EVENT_SENT;
        }
        else {
            jpt = JPT_UNKNOWN;
        }
    }

    return jpt;

failed:
    if (*jo)
        purc_variant_unref(*jo);

    return jpt;
}

static int get_challenge_code(pcdvobjs_stream *stream,
        const char *payload, size_t len, char **challenge)
{
    int ret = -1;
    purc_variant_t jo = NULL, jo_tmp;
    const char *ch_code = NULL;
    struct stream_extended_data *ext = stream->ext1.data;

    jo = purc_variant_make_from_json_string(payload, len);
    if (jo == NULL || !purc_variant_is_object(jo)) {
        set_error(ext, BADMSGPAYLOAD);
        goto failed;
    }

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "packetType"))) {
        const char *pack_type;
        pack_type = purc_variant_get_string_const(jo_tmp);

        if (strcasecmp(pack_type, "error") == 0) {
            const char* prot_name = HBDBUS_NOT_AVAILABLE;
            int prot_ver = 0, ret_code = 0;
            const char *ret_msg = HBDBUS_NOT_AVAILABLE;
            const char *extra_msg = HBDBUS_NOT_AVAILABLE;

            PC_WARN("Refued by server:\n");
            if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "protocolName"))) {
                prot_name = purc_variant_get_string_const(jo_tmp);
            }

            if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "protocolVersion"))) {
                purc_variant_cast_to_int32(jo_tmp, &prot_ver, true);
            }
            PC_WARN("  Protocol: %s/%d\n", prot_name, prot_ver);

            if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "retCode"))) {
                purc_variant_cast_to_int32(jo_tmp, &ret_code, true);
            }
            if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "retMsg"))) {
                ret_msg = purc_variant_get_string_const(jo_tmp);
            }
            if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "extraMsg"))) {
                extra_msg = purc_variant_get_string_const(jo_tmp);
            }
            PC_WARN("  Error Info: %d (%s): %s\n", ret_code, ret_msg, extra_msg);

            set_error(ext, SERVERREFUSED);
            goto failed;
        }
        else if (strcasecmp (pack_type, "auth") == 0) {
            const char *prot_name = HBDBUS_NOT_AVAILABLE;
            int prot_ver = 0;

            if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "challengeCode"))) {
                ch_code = purc_variant_get_string_const(jo_tmp);
            }

            if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "protocolName"))) {
                prot_name = purc_variant_get_string_const (jo_tmp);
            }
            if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "protocolVersion"))) {
                purc_variant_cast_to_int32(jo_tmp, &prot_ver, true);
            }

            if (ch_code == NULL) {
                PC_WARN("Null challenge code\n");
                set_error(ext, BADMSGPAYLOAD);
                goto failed;
            }
            else if (strcasecmp(prot_name, HBDBUS_PROTOCOL_NAME) ||
                    prot_ver < HBDBUS_PROTOCOL_VERSION) {
                PC_WARN("Protocol not matched: %s/%d\n", prot_name, prot_ver);
                set_error(ext, WRONGVERSION);
                goto failed;
            }
        }
    }
    else {
        PC_WARN("No packetType field\n");
        set_error(ext, BADMSGPAYLOAD);
        goto failed;
    }

    assert(ch_code);
    *challenge = strdup(ch_code);
    if (*challenge == NULL)
        set_error(ext, OUTOFMEMORY);
    ret = 0;

failed:
    if (jo)
        purc_variant_unref(jo);

    return ret;
}

static int send_auth_info(pcdvobjs_stream *stream, const char* ch_code)
{
    int n;
    unsigned char* sig;
    unsigned int sig_len;
    char* enc_sig = NULL;
    unsigned int enc_sig_len;
    char buff[HBDBUS_DEF_PACKET_BUFF_SIZE];
    struct stream_extended_data *ext = stream->ext1.data;

    if (pcutils_sign_data(ext->inst->app_name,
            (const unsigned char *)ch_code, strlen(ch_code),
            &sig, &sig_len)) {
        set_error(ext, UNEXPECTED);
        goto failed;
    }

    enc_sig_len = pcutils_b64_encoded_length(sig_len);
    enc_sig = malloc(enc_sig_len);
    if (enc_sig == NULL) {
        set_error(ext, OUTOFMEMORY);
        goto failed;
    }

    // When encode the signature in base64 or exadecimal notation,
    // there will be no any '"' and '\' charecters.
    pcutils_b64_encode(sig, sig_len, enc_sig, enc_sig_len);
    free(sig);
    sig = NULL;

    n = snprintf(buff, sizeof (buff),
            "{"
            "\"packetType\":\"auth\","
            "\"protocolName\":\"%s\","
            "\"protocolVersion\":%d,"
            "\"hostName\":\"%s\","
            "\"appName\":\"%s\","
            "\"runnerName\":\"%s\","
            "\"signature\":\"%s\","
            "\"encodedIn\":\"base64\""
            "}",
            HBDBUS_PROTOCOL_NAME,
            HBDBUS_PROTOCOL_VERSION,
            "localhost", // ext->inst->localhost,
            ext->inst->app_name,
            ext->inst->runner_name, enc_sig);

    if (n < 0) {
        set_error(ext, UNEXPECTED);
        goto failed;
    }
    else if ((size_t)n >= sizeof (buff)) {
        PC_ERROR("Too small buffer for signature (%s).\n", enc_sig);
        set_error(ext, TOOSMALLBUFFER);
        goto failed;
    }

    if (call_super(stream, send_message, stream, true, buff, n)) {
        PC_ERROR("Failed to send text message to HBDBus server.\n");
        set_error(ext, FAILEDWRITE);
        goto failed;
    }

    free(enc_sig);
    return 0;

failed:
    if (sig)
        free(sig);
    if (enc_sig)
        free(enc_sig);
    return -1;
}

static void on_lost_event_generator(pcdvobjs_stream *stream,
        const char* from_endpoint, const char* from_bubble,
        const char* bubble_data)
{
    (void)from_endpoint;
    (void)from_bubble;
    purc_variant_t jo = NULL, jo_tmp;
    const char *endpoint_name = NULL;
    const char* event_name;
    void *next, *data;
    struct stream_extended_data *ext = stream->ext1.data;

    jo = purc_variant_make_from_json_string(bubble_data, strlen(bubble_data));
    if (jo == NULL) {
        PC_ERROR("Failed to parse bubble data for `LostEventGenerator`\n");
        return;
    }

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "endpointName")) &&
            (endpoint_name = purc_variant_get_string_const(jo_tmp))) {
    }
    else {
        PC_ERROR("Fatal error: no endpointName field in the packet!\n");
        goto done;
    }

    kvlist_for_each_safe(&ext->subscribed_list, event_name, next, data) {
        const char* end_of_endpoint = strrchr(event_name, '/');

        if (strncasecmp(event_name, endpoint_name,
                    end_of_endpoint - event_name) == 0) {
            PC_INFO("Matched an event (%s) in subscribed events for %s\n",
                    event_name, endpoint_name);

            pcutils_kvlist_remove(&ext->subscribed_list, event_name);
        }
    }

    pcintr_coroutine_post_event(stream->cid,
            PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
            EVENT_TYPE_STATE, HBDBUS_BUBBLE_LOSTEVENTGENERATOR,
            jo, PURC_VARIANT_INVALID);

done:
    purc_variant_unref(jo);
}

static void on_lost_event_bubble(pcdvobjs_stream *stream,
        const char* from_endpoint, const char* from_bubble,
        const char* bubble_data)
{
    (void)from_endpoint;
    (void)from_bubble;
    int n;
    purc_variant_t jo = NULL, jo_tmp;
    const char *endpoint_name = NULL;
    const char *bubble_name = NULL;
    char event_name [HBDBUS_LEN_ENDPOINT_NAME + HBDBUS_LEN_BUBBLE_NAME + 2];
    struct stream_extended_data *ext = stream->ext1.data;

    jo = purc_variant_make_from_json_string(bubble_data, strlen(bubble_data));
    if (jo == NULL) {
        PC_ERROR("Failed to parse bubble data for bubble `LostEventBubble`\n");
        return;
    }

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "endpointName")) &&
            (endpoint_name = purc_variant_get_string_const(jo_tmp))) {
    }
    else {
        PC_ERROR("Fatal error: no endpointName in the packet!\n");
        goto done;
    }

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "bubbleName")) &&
            (bubble_name = purc_variant_get_string_const(jo_tmp))) {
    }
    else {
        PC_ERROR("Fatal error: no bubbleName in the packet!\n");
        goto done;
    }

    n = purc_name_tolower_copy(endpoint_name, event_name, HBDBUS_LEN_ENDPOINT_NAME);
    event_name[n++] = '/';
    event_name[n] = '\0';
    strcpy(event_name + n, bubble_name);
    if (!pcutils_kvlist_get(&ext->subscribed_list, event_name)) {
        PC_WARN("Not subscribed event: %s!\n", event_name);
        goto done;
    }

    pcutils_kvlist_remove(&ext->subscribed_list, event_name);

    pcintr_coroutine_post_event(stream->cid,
            PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
            EVENT_TYPE_STATE, HBDBUS_BUBBLE_LOSTEVENTBUBBLE,
            jo, PURC_VARIANT_INVALID);

done:
    purc_variant_unref(jo);
}

static void on_system_shutting_down(pcdvobjs_stream *stream,
        const char* from_endpoint, const char* from_bubble,
        const char* bubble_data)
{
    (void)from_endpoint;
    (void)from_bubble;
    purc_variant_t jo = NULL, jo_tmp;
    const char *endpoint_name = NULL;
    uint64_t shutdown_time;

    jo = purc_variant_make_from_json_string(bubble_data, strlen(bubble_data));
    if (jo == NULL) {
        PC_ERROR("Failed to parse bubble data for bubble `LostEventBubble`\n");
        return;
    }

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "endpointName")) &&
            (endpoint_name = purc_variant_get_string_const(jo_tmp))) {
    }
    else {
        PC_ERROR("Fatal error: no endpointName in the packet!\n");
        goto done;
    }

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "shutdownTime")) &&
            purc_variant_cast_to_ulongint(jo_tmp, &shutdown_time, true)) {
    }
    else {
        PC_ERROR("Fatal error: no shutdownTime or bad value in the packet!\n");
        goto done;
    }

    pcintr_coroutine_post_event(stream->cid,
            PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
            EVENT_TYPE_STATE, HBDBUS_BUBBLE_SYSTEMSHUTTINGDOWN,
            jo, PURC_VARIANT_INVALID);

done:
    purc_variant_unref(jo);
}

/* add systen event handlers here */
static int on_auth_passed(pcdvobjs_stream *stream, const purc_variant_t jo)
{
    int n;
    purc_variant_t jo_tmp;
    char event_name[HBDBUS_LEN_ENDPOINT_NAME + HBDBUS_LEN_BUBBLE_NAME + 2];
    const char* srv_host_name;
    const char* own_host_name;
    hbdbus_event_handler event_handler;
    struct stream_extended_data *ext = stream->ext1.data;

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "serverHostName")) &&
            (srv_host_name = purc_variant_get_string_const(jo_tmp))) {
        if (ext->srv_host_name)
            free(ext->srv_host_name);

        ext->srv_host_name = strdup(srv_host_name);
    }
    else {
        PC_ERROR("Fatal error: no serverHostName in authPassed packet!\n");
        set_error(ext, BADMSGPAYLOAD);
        goto failed;
    }

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "reassignedHostName")) &&
            (own_host_name = purc_variant_get_string_const(jo_tmp))) {
        if (ext->own_host_name)
            free(ext->own_host_name);

        ext->own_host_name = strdup(own_host_name);
    }
    else {
        PC_ERROR("Fatal error: no reassignedHostName in authPassed packet!\n");
        set_error(ext, BADMSGPAYLOAD);
        goto failed;
    }

    n = purc_assemble_endpoint_name(srv_host_name,
            HBDBUS_APP_NAME, HBDBUS_RUN_BUILITIN, event_name);
    event_name[n++] = '/';
    event_name[n] = '\0';
    strcat(event_name, HBDBUS_BUBBLE_LOSTEVENTGENERATOR);

    event_handler = on_lost_event_generator;
    if (!pcutils_kvlist_set(&ext->subscribed_list, event_name,
                &event_handler)) {
        PC_ERROR("Failed to register cb for sys-evt `LostEventGenerator`!\n");
        set_error(ext, OUTOFMEMORY);
        goto failed;
    }

    n = purc_assemble_endpoint_name(srv_host_name,
            HBDBUS_APP_NAME, HBDBUS_RUN_BUILITIN, event_name);
    event_name[n++] = '/';
    event_name[n] = '\0';
    strcat(event_name, HBDBUS_BUBBLE_LOSTEVENTBUBBLE);

    event_handler = on_lost_event_bubble;
    if (!pcutils_kvlist_set(&ext->subscribed_list, event_name,
                &event_handler)) {
        PC_ERROR("Failed to register cb for sys-evt `LostEventBubble`!\n");
        set_error(ext, OUTOFMEMORY);
        goto failed;
    }

    n = purc_assemble_endpoint_name(srv_host_name,
            HBDBUS_APP_NAME, HBDBUS_RUN_BUILITIN, event_name);
    event_name[n++] = '/';
    event_name[n] = '\0';
    strcat(event_name, HBDBUS_BUBBLE_SYSTEMSHUTTINGDOWN);

    event_handler = on_system_shutting_down;
    if (!pcutils_kvlist_set(&ext->subscribed_list, event_name,
                &event_handler)) {
        PC_ERROR("Failed to register cb for sys-evt `SystemShuttingDown`!\n");
        set_error(ext, OUTOFMEMORY);
        goto failed;
    }

    return 0;
failed:
    return -1;
}

static int
check_auth_result(pcdvobjs_stream *stream, const char *payload, size_t len)
{
    purc_variant_t jo = NULL;
    int ret, retv = -1;
    struct stream_extended_data *ext = stream->ext1.data;

    ret = hbdbus_json_packet_to_object(payload, len, &jo);

    if (ret < 0) {
        set_error(ext, BADMSGPAYLOAD);
        goto done;
    }
    else if (ret == JPT_AUTH_PASSED) {
        PC_INFO("Passed the authentication\n");
        retv = on_auth_passed(stream, jo);
        goto done;
    }
    else if (ret == JPT_AUTH_FAILED) {
        PC_WARN("Failed the authentication\n");
        set_error(ext, AUTHFAILED);
        goto done;
    }
    else if (ret == JPT_ERROR) {
        set_error(ext, SERVERREFUSED);
        goto done;
    }
    else {
        set_error(ext, UNEXPECTED);
        goto done;
    }

    retv = 0;

done:
    if (jo)
        purc_variant_unref(jo);
    return retv;
}

static int
dispatch_call_packet(struct pcdvobjs_stream *stream, purc_variant_t jo)
{
    struct stream_extended_data *ext = stream->ext1.data;
    purc_variant_t jo_tmp;
    const char *from_endpoint = NULL, *call_id = NULL, *result_id = NULL;
    const char *to_method;
    const char *parameter;
    void *data;
    char packet_buff[HBDBUS_DEF_PACKET_BUFF_SIZE];
    int ret_code = PCRDR_SC_OK;
    double time_consumed = 0.0f;

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "fromEndpoint")) &&
            (from_endpoint = purc_variant_get_string_const(jo_tmp))) {
    }
    else {
        set_error(ext, BADMSGPAYLOAD);
        ret_code = PCRDR_SC_BAD_REQUEST;
        goto done;
    }

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "toMethod")) &&
            (to_method = purc_variant_get_string_const(jo_tmp))) {
    }
    else {
        set_error(ext, BADMSGPAYLOAD);
        ret_code = PCRDR_SC_BAD_REQUEST;
        goto done;
    }

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "callId")) &&
            (call_id = purc_variant_get_string_const(jo_tmp))) {
    }
    else {
        set_error(ext, BADMSGPAYLOAD);
        ret_code = PCRDR_SC_BAD_REQUEST;
        goto done;
    }

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "resultId")) &&
            (result_id = purc_variant_get_string_const(jo_tmp))) {
    }
    else {
        set_error(ext, BADMSGPAYLOAD);
        ret_code = PCRDR_SC_BAD_REQUEST;
        goto done;
    }

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "parameter")) &&
            (parameter = purc_variant_get_string_const(jo_tmp))) {
    }
    else {
        parameter = "";
    }

    if ((data = pcutils_kvlist_get(&ext->method_list, to_method)) == NULL) {
        ret_code = PCRDR_SC_NOT_FOUND;
        goto done;
    }
    else {
        struct method_called_info mci;
        clock_gettime(CLOCK_MONOTONIC, &mci.called_ts);
        mci.method = strdup(to_method);
        mci.call_id = strdup(call_id);

        if (pcutils_kvlist_set(&ext->called_list, result_id, &mci)) {
            /* fire a `call:<to_method>` event */
            pcintr_coroutine_post_event(stream->cid,
                    PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
                    EVENT_TYPE_CALL, to_method,
                    jo, PURC_VARIANT_INVALID);
        }
        else {
            free_mci(NULL, NULL, &mci);
            set_error(ext, OUTOFMEMORY);
            ret_code = PCRDR_SC_INSUFFICIENT_STORAGE;
        }
    }

done:
    if (ret_code == PCRDR_SC_OK)
        return 0;

    int n = snprintf(packet_buff, sizeof(packet_buff),
            "{"
            "\"packetType\": \"result\","
            "\"resultId\": \"%s\","
            "\"callId\": \"%s\","
            "\"fromMethod\": \"%s\","
            "\"timeConsumed\": %.9f,"
            "\"retCode\": %d,"
            "\"retMsg\": \"%s\","
            "\"retValue\": \"\""
            "}",
            result_id, call_id,
            to_method,
            time_consumed,
            ret_code,
            ext->errsymb);

    if (n < 0) {
        set_error(ext, UNEXPECTED);
    }
    else if ((size_t)n >= sizeof(packet_buff)) {
        set_error(ext, TOOSMALLBUFFER);
    }
    else {
        if (call_super(stream, send_message, stream, true, packet_buff, n)) {
            set_error(ext, FAILEDWRITE);
        }
        else
            return 0;
    }

    return -1;
}

static int
dispatch_result_packet(struct pcdvobjs_stream *stream, purc_variant_t jo)
{
    struct stream_extended_data *ext = stream->ext1.data;
    purc_variant_t jo_tmp;
    const char* result_id = NULL, *call_id = NULL;
    int ret_code;
    void *data;

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "resultId")) &&
            (result_id = purc_variant_get_string_const(jo_tmp))) {
    }
    else {
        PC_WARN("No resultId\n");
    }

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "callId")) &&
            (call_id = purc_variant_get_string_const(jo_tmp))) {
    }
    else {
        set_error(ext, BADMSGPAYLOAD);
        goto failed;
    }

    if ((jo_tmp = purc_variant_object_get_by_ckey (jo, "retCode")) &&
            purc_variant_cast_to_int32(jo_tmp, &ret_code, false)) {
        if (ret_code == PCRDR_SC_ACCEPTED) {
            goto done;
        }
    }
    else {
        set_error(ext, BADMSGPAYLOAD);
        goto failed;
    }

    data = pcutils_kvlist_get(&ext->calling_list, call_id);
    if (data == NULL) {
        PC_ERROR ("No record for callId: %s\n", call_id);
        set_error(ext, INVALIDPARAMS);
        goto failed;
    }

    struct calling_procedure_info cpi;
    cpi = *(struct calling_procedure_info *)data;
    pcutils_kvlist_remove(&ext->calling_list, call_id);

    int retv = 0;
    if (cpi.handler) {
        /* result of calling a hbdbus builtin procedure */
        retv = cpi.handler(stream, cpi.ctxt, jo);
    }
    else {
        /* fire a `result:<method_name>` event */
        pcintr_coroutine_post_event(stream->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
                EVENT_TYPE_RESULT, cpi.method,
                jo, PURC_VARIANT_INVALID);
    }

    free_cpi(NULL, NULL, &cpi);

    if (retv)
        return -1;

done:
    return 0;

failed:
    return -1;
}

static int
dispatch_event_packet(struct pcdvobjs_stream *stream, purc_variant_t jo)
{
    struct stream_extended_data *ext = stream->ext1.data;
    purc_variant_t jo_tmp;
    const char* from_endpoint = NULL;
    const char* from_bubble = NULL;
    const char* event_id = NULL;
    char event_name [HBDBUS_LEN_ENDPOINT_NAME + HBDBUS_LEN_BUBBLE_NAME + 2];
    int n;
    void *data;

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "fromEndpoint")) &&
            (from_endpoint = purc_variant_get_string_const(jo_tmp))) {
    }
    else {
        set_error(ext, BADMSGPAYLOAD);
        goto failed;
    }

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "fromBubble")) &&
            (from_bubble = purc_variant_get_string_const(jo_tmp))) {
    }
    else {
        set_error(ext, BADMSGPAYLOAD);
        goto failed;
    }

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "eventId")) &&
            (event_id = purc_variant_get_string_const(jo_tmp))) {
    }
    else {
        set_error(ext, BADMSGPAYLOAD);
        goto failed;
    }

    n = purc_name_tolower_copy(from_endpoint, event_name,
            HBDBUS_LEN_ENDPOINT_NAME);
    event_name[n++] = '/';
    event_name[n] = '\0';
    strcpy(event_name + n, from_bubble);
    data = pcutils_kvlist_get(&ext->subscribed_list, event_name);
    if (data == NULL) {
        if (strcmp(event_id, HBDBUS_SYSTEM_EVENT_ID) == 0) {
            /* fire an `event:SYSTEM` event */
            pcintr_coroutine_post_event(stream->cid,
                    PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
                    EVENT_TYPE_EVENT, EVENT_SUBTYPE_SYSTEM,
                    jo, PURC_VARIANT_INVALID);
        }
        else {
            PC_ERROR("Got an unsubscribed event: %s\n", event_name);
        }
    }
    else {
        hbdbus_event_handler event_handler;
        event_handler = *(hbdbus_event_handler *)data;
        if (event_handler) {
            const char* bubble_data;
            if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "bubbleData")) &&
                    (bubble_data = purc_variant_get_string_const(jo_tmp))) {
            }
            else {
                bubble_data = NULL;
            }

            if (bubble_data == NULL) {
                set_error(ext, BADMSGPAYLOAD);
                goto failed;
            }

            event_handler(stream, from_endpoint, from_bubble, bubble_data);
        }
        else {
            /* fire an `event:<from_bubble>` event */
            pcintr_coroutine_post_event(stream->cid,
                    PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
                    EVENT_TYPE_EVENT, from_bubble,
                    jo, PURC_VARIANT_INVALID);
        }
    }

    return 0;

failed:
    return -1;
}

static int handle_regular_message(struct pcdvobjs_stream *stream,
            const char *payload, size_t len)
{
    struct stream_extended_data *ext = stream->ext1.data;
    purc_variant_t jo = NULL;

    int retval = hbdbus_json_packet_to_object(payload, len, &jo);
    if (retval < 0) {
        PC_ERROR("Failed to parse JSON packet; quit...\n");
        set_error(ext, BADMSGPAYLOAD);
    }
    else if (retval == JPT_ERROR) {
        PC_INFO("The server gives an error packet: %s\n", payload);
        pcintr_coroutine_post_event(stream->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
                EVENT_TYPE_ERROR, EVENT_SUBTYPE_HBDBUS,
                jo, PURC_VARIANT_INVALID);
    }
    else if (retval == JPT_AUTH) {
        PC_ERROR("Should not be here for packetType `auth`; quit...\n");
        set_error(ext, UNEXPECTED);
    }
    else if (retval == JPT_CALL) {
        dispatch_call_packet(stream, jo);
    }
    else if (retval == JPT_RESULT) {
        dispatch_result_packet(stream, jo);
    }
    else if (retval == JPT_RESULT_SENT) {
    }
    else if (retval == JPT_EVENT) {
        dispatch_event_packet(stream, jo);
    }
    else if (retval == JPT_EVENT_SENT) {
    }
    else if (retval == JPT_AUTH_PASSED) {
        PC_ERROR("Unexpected authPassed packet\n");
        set_error(ext, UNEXPECTED);
    }
    else if (retval == JPT_AUTH_FAILED) {
        PC_ERROR("Unexpected authFailed packet\n");
        set_error(ext, UNEXPECTED);
    }
    else {
        PC_ERROR("Unknown packet type; quit...\n");
        set_error(ext, UNEXPECTED);
    }

    if (jo)
        purc_variant_unref(jo);

    if (ext->errsymb)
        return -1;

    return 0;
}

static int on_message(struct pcdvobjs_stream *stream, int type,
            char *payload, size_t len, int *owner_taken)
{
    struct stream_extended_data *ext = stream->ext1.data;

    if (ext == NULL) {
        return 0;
    }

    clr_error(ext);

    if (type != MT_TEXT) {
        /* call the method of Layer 0. */
        if (ext->on_message_super) {
            return ext->on_message_super(stream, type, payload, len,
                    owner_taken);
        }

        set_error(ext, UNKNOWNMESSAGE);
    }

    switch (ext->state) {
    case BS_EXPECT_CHALLENGE: {
        char *ch_code;
        int ret;

        if ((ret = get_challenge_code(stream, payload, len, &ch_code))) {
            ext->state = BS_UNCERTAIN;
            goto done;
        }

        send_auth_info(stream, ch_code);
        free(ch_code);
        if (ret == 0) {
            ext->state = BS_EXPECT_AUTH_RESULT;
        }
        break;
    }

    case BS_EXPECT_AUTH_RESULT:
        if (check_auth_result(stream, payload, len)) {
            ext->state = BS_UNCERTAIN;
            goto done;
        }

        /*  fire `state:ready` event */
        pcintr_coroutine_post_event(stream->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
                EVENT_TYPE_STATE, EVENT_SUBTYPE_READY,
                PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
        ext->state = BS_EXPECT_REGULAR_MSG;
        break;

    case BS_EXPECT_REGULAR_MSG:
        if (handle_regular_message(stream, payload, len)) {
            ext->state = BS_UNCERTAIN;
            goto done;
        }
        break;

    case BS_UNCERTAIN:
        set_error(ext, UNEXPECTED);
        goto done;
        break;
    }

done:
    if (ext->errsymb) {
        /* fire an `error:hbdbus` event */
        purc_variant_t data = purc_variant_make_object_0();
        if (data) {
            purc_variant_t tmp;

            tmp = purc_variant_make_number(ext->errcode);
            if (tmp) {
                purc_variant_object_set_by_static_ckey(data, "errCode",
                        tmp);
                purc_variant_unref(tmp);
            }

            tmp = purc_variant_make_string_static(ext->errsymb, false);
            if (tmp) {
                purc_variant_object_set_by_static_ckey(data, "errMsg",
                        tmp);
                purc_variant_unref(tmp);
            }
        }

        pcintr_coroutine_post_event(stream->cid,
                PCRDR_MSG_EVENT_REDUCE_OPT_KEEP, stream->observed,
                EVENT_TYPE_ERROR, EVENT_SUBTYPE_HBDBUS,
                data, PURC_VARIANT_INVALID);
        if (data)
            purc_variant_unref(data);
    }

    if (ext->state == BS_UNCERTAIN) {
        // close the connection
        call_super(stream, shut_off, stream);
    }

    return 0;
}

struct purc_native_ops *
dvobjs_extend_stream_by_hbdbus(struct pcdvobjs_stream *stream,
        struct purc_native_ops *super_ops, purc_variant_t extra_opts)
{
    (void)extra_opts;

    if (super_ops == NULL ||
            strcmp(stream->ext0.signature, STREAM_EXT_SIG_MSG)) {
        PC_ERROR("Layer 0 is not a message extension.\n");
        purc_set_error(PURC_ERROR_CONFLICT);
        goto failed;
    }

    struct pcinst* inst = pcinst_current();
    if (inst == NULL) {
        PC_ERROR("No instance.\n");
        purc_set_error(PURC_ERROR_NO_INSTANCE);
        goto failed;
    }

    struct stream_extended_data *ext = calloc(1, sizeof(*ext));
    if (ext == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    ext->state = BS_EXPECT_CHALLENGE;
    ext->inst = inst;
    ext->srv_host_name = NULL;
    ext->own_host_name = strdup(HBDBUS_LOCALHOST);
    pcutils_kvlist_init_ex(&ext->method_list, NULL, true);
    pcutils_kvlist_init_ex(&ext->called_list, get_mci_len, false);
    pcutils_kvlist_init_ex(&ext->calling_list, get_cpi_len, false);

    pcutils_kvlist_init_ex(&ext->bubble_list, NULL, true);
    pcutils_kvlist_init_ex(&ext->subscribed_list, NULL, true);

    strcpy(stream->ext1.signature, STREAM_EXT_SIG_HBS);
    stream->ext1.data = ext;
    stream->ext1.super_ops = super_ops;
    stream->ext1.bus_ops = NULL;

    /* override the `on_message` method of Layer 0 */
    ext->on_message_super = stream->ext0.msg_ops->on_message;
    stream->ext0.msg_ops->on_message = on_message;
    ext->cleanup_super = stream->ext0.msg_ops->cleanup;
    stream->ext0.msg_ops->cleanup = cleanup_extension;

    PC_INFO("This socket is extended by Layer 1 protocol: hbdbus\n");
    return &hbdbus_ops;

failed:
    return NULL;
}

#endif /* ENABLE(STREAM_HBDBUS) */
