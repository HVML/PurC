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
#define HBDBUS_BUBBLE_SYSTEMSHUTTINGDOWN    "SystemShuttingdown"

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

#define ERR_SYM_AGAIN                   "-"
#define ERR_SYM_BADMESSAGE              "badMessage"
#define ERR_SYM_BADMSGCONTENTS          "badMsgContents"
#define ERR_SYM_SERVERREFUSED           "serverRefused"
#define ERR_SYM_WRONGVERSION            "wrongVersion"
#define ERR_SYM_OUTOFMEMORY             "outOfMemory"
#define ERR_SYM_UNEXPECTED              "unexpected"
#define ERR_SYM_TOOSMALLBUFFER          "tooSmallBuffer"
#define ERR_SYM_FAILEDWRITE             "failedWrite"
#define ERR_SYM_FAILEDREAD              "failedRead"
#define ERR_SYM_AUTHFAILED              "authFailed"

#define set_error(stream, sym)          stream->ext->errsym = sym ""

typedef void (*hbdbus_error_handler)(struct pcdvobjs_stream *stream,
        const purc_variant_t jo);
typedef void (*hbdbus_event_handler)(struct pcdvobjs_stream *stream,
        const char *from_endpoint, const char *from_bubble,
        const char *bubble_data);

struct stream_extended {
    const struct pcinst *inst;
    const char *errsym;

    char *srv_host_name;
    char *own_host_name;

    struct pcutils_kvlist method_list;
    struct pcutils_kvlist bubble_list;
    struct pcutils_kvlist call_list;
    struct pcutils_kvlist subscribed_list;

    hbdbus_error_handler error_handler;
    hbdbus_event_handler system_event_handler;

    const struct purc_native_ops *super_ops;
};

typedef enum  {
    MHT_STRING  = 0,
    MHT_CONST_STRING = 1,
} method_handler_type;

struct method_handler_info {
    method_handler_type type;
    void* handler;
};

static size_t mhi_get_len(struct pcutils_kvlist *kv, const void *data)
{
    (void)kv;
    (void)data;
    return sizeof(struct method_handler_info);
}

#define call_super(stream, method, x, ...)                              \
    ((struct stream_messaging_ops*)stream->ext->super_ops->priv_ops)->  \
    method(x, ##__VA_ARGS__)

static purc_nvariant_method
property_getter(void *entity, const char *name)
{
    struct pcdvobjs_stream *stream = entity;
    purc_nvariant_method method = NULL;

    if (name == NULL) {
        goto failed;
    }

    if (strcmp(name, "call") == 0) {
    }
    else if (strcmp(name, "subscribe") == 0) {
    }
    else if (strcmp(name, "unsubscribe") == 0) {
    }
    else if (name[0] == 'r') {
        if (strcmp(name, "register_evnt") == 0) {
        }
        else if (strcmp(name, "register_proc") == 0) {
        }
    }

    if (method == NULL && stream->ext->super_ops->property_getter) {
        return stream->ext->super_ops->property_getter(entity, name);
    }

failed:
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

static void on_release(void *entity)
{
    struct pcdvobjs_stream *stream = entity;

    pcutils_kvlist_cleanup(&stream->ext->method_list);
    pcutils_kvlist_cleanup(&stream->ext->bubble_list);
    pcutils_kvlist_cleanup(&stream->ext->call_list);
    pcutils_kvlist_cleanup(&stream->ext->subscribed_list);
    free(stream->ext);

    if (stream->ext->super_ops->on_release) {
        return stream->ext->super_ops->on_release(entity);
    }
}

static const struct purc_native_ops hbdbus_ops = {
    .property_getter = property_getter,
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

static int get_challenge_code(pcdvobjs_stream *stream, char **challenge)
{
    char *payload;
    size_t len;
    int ret = -1;
    int type = MSG_DATA_TYPE_UNKNOWN;
    purc_variant_t jo = NULL, jo_tmp;
    const char *ch_code = NULL;

    ret = call_super(stream, read_message, stream, &payload, &len, &type);
    if (ret > 0) {
        set_error(stream, ERR_SYM_AGAIN);
        goto failed;
    }
    else if (ret < 0) {
        set_error(stream, ERR_SYM_FAILEDREAD);
        goto failed;
    }

    if (type != MSG_DATA_TYPE_TEXT || payload == NULL || len == 0) {
        set_error(stream, ERR_SYM_BADMESSAGE);
        goto failed;
    }

    jo = purc_variant_make_from_json_string(payload, len);
    if (jo == NULL || !purc_variant_is_object(jo)) {
        set_error(stream, ERR_SYM_BADMSGCONTENTS);
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

            set_error(stream, ERR_SYM_SERVERREFUSED);
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
                set_error(stream, ERR_SYM_BADMSGCONTENTS);
                goto failed;
            }
            else if (strcasecmp(prot_name, HBDBUS_PROTOCOL_NAME) ||
                    prot_ver < HBDBUS_PROTOCOL_VERSION) {
                PC_WARN("Protocol not matched: %s/%d\n", prot_name, prot_ver);
                set_error(stream, ERR_SYM_WRONGVERSION);
                goto failed;
            }
        }
    }
    else {
        PC_WARN("No packetType field\n");
        set_error(stream, ERR_SYM_BADMSGCONTENTS);
        goto failed;
    }

    assert(ch_code);
    *challenge = strdup(ch_code);
    if (*challenge == NULL)
        set_error(stream, ERR_SYM_OUTOFMEMORY);

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

    if (pcutils_sign_data(stream->ext->inst->app_name,
            (const unsigned char *)ch_code, strlen(ch_code),
            &sig, &sig_len)) {
        set_error(stream, ERR_SYM_UNEXPECTED);
        goto failed;
    }

    enc_sig_len = pcutils_b64_encoded_length(sig_len);
    enc_sig = malloc(enc_sig_len);
    if (enc_sig == NULL) {
        set_error(stream, ERR_SYM_OUTOFMEMORY);
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
            "localhost", // stream->ext->inst->localhost,
            stream->ext->inst->app_name,
            stream->ext->inst->runner_name, enc_sig);

    if (n < 0) {
        set_error(stream, ERR_SYM_UNEXPECTED);
        goto failed;
    }
    else if ((size_t)n >= sizeof (buff)) {
        PC_ERROR("Too small buffer for signature (%s).\n", enc_sig);
        set_error(stream, ERR_SYM_TOOSMALLBUFFER);
        goto failed;
    }

    if (call_super(stream, send_text, stream, buff, n)) {
        PC_ERROR("Failed to send text message to HBDBus server.\n");
        set_error(stream, ERR_SYM_FAILEDWRITE);
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
        return;
    }

    kvlist_for_each_safe(&stream->ext->subscribed_list, event_name, next, data) {
        const char* end_of_endpoint = strrchr(event_name, '/');

        if (strncasecmp(event_name, endpoint_name,
                    end_of_endpoint - event_name) == 0) {
            PC_INFO("Matched an event (%s) in subscribed events for %s\n",
                    event_name, endpoint_name);

            pcutils_kvlist_remove(&stream->ext->subscribed_list, event_name);
        }
    }
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
        return;
    }

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "bubbleName")) &&
            (bubble_name = purc_variant_get_string_const(jo_tmp))) {
    }
    else {
        PC_ERROR("Fatal error: no bubbleName in the packet!\n");
        return;
    }

    n = purc_name_tolower_copy(endpoint_name, event_name, HBDBUS_LEN_ENDPOINT_NAME);
    event_name [n++] = '/';
    event_name [n] = '\0';
    strcpy(event_name + n, bubble_name);
    if (!pcutils_kvlist_get(&stream->ext->subscribed_list, event_name))
        return;

    pcutils_kvlist_remove(&stream->ext->subscribed_list, event_name);
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

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "serverHostName")) &&
            (srv_host_name = purc_variant_get_string_const(jo_tmp))) {
        if (stream->ext->srv_host_name)
            free(stream->ext->srv_host_name);

        stream->ext->srv_host_name = strdup(srv_host_name);
    }
    else {
        PC_ERROR("Fatal error: no serverHostName in authPassed packet!\n");
        set_error(stream, ERR_SYM_BADMESSAGE);
        goto failed;
    }

    if ((jo_tmp = purc_variant_object_get_by_ckey(jo, "reassignedHostName")) &&
            (own_host_name = purc_variant_get_string_const(jo_tmp))) {
        if (stream->ext->own_host_name)
            free(stream->ext->own_host_name);

        stream->ext->own_host_name = strdup(own_host_name);
    }
    else {
        PC_ERROR("Fatal error: no reassignedHostName in authPassed packet!\n");
        set_error(stream, ERR_SYM_BADMESSAGE);
        goto failed;
    }

    n = purc_assemble_endpoint_name(srv_host_name,
            HBDBUS_APP_NAME, HBDBUS_RUN_BUILITIN, event_name);
    event_name [n++] = '/';
    event_name [n] = '\0';
    strcat (event_name, HBDBUS_BUBBLE_LOSTEVENTGENERATOR);

    event_handler = on_lost_event_generator;
    if (!pcutils_kvlist_set(&stream->ext->subscribed_list, event_name,
                &event_handler)) {
        PC_ERROR("Failed to register cb for sys-evt `LostEventGenerator`!\n");
        set_error(stream, ERR_SYM_OUTOFMEMORY);
        goto failed;
    }

    n = purc_assemble_endpoint_name(srv_host_name,
            HBDBUS_APP_NAME, HBDBUS_RUN_BUILITIN, event_name);
    event_name [n++] = '/';
    event_name [n] = '\0';
    strcat (event_name, HBDBUS_BUBBLE_LOSTEVENTBUBBLE);

    event_handler = on_lost_event_bubble;
    if (!pcutils_kvlist_set(&stream->ext->subscribed_list, event_name,
                &event_handler)) {
        PC_ERROR("Failed to register cb for sys-evt `LostEventBubble`!\n");
        set_error(stream, ERR_SYM_OUTOFMEMORY);
        goto failed;
    }

    return 0;
failed:
    return -1;
}

static int check_auth_result(pcdvobjs_stream *stream)
{
    char *payload;
    size_t len;
    purc_variant_t jo = NULL;
    int ret, type, retv = -1;

    ret = call_super(stream, read_message, stream, &payload, &len, &type);
    if (ret > 0) {
        set_error(stream, ERR_SYM_AGAIN);
        goto done;
    }
    else if (ret < 0) {
        set_error(stream, ERR_SYM_FAILEDREAD);
        goto done;
    }

    if (type != MSG_DATA_TYPE_TEXT || payload == NULL || len == 0) {
        set_error(stream, ERR_SYM_BADMESSAGE);
        goto done;
    }

    ret = hbdbus_json_packet_to_object(payload, len, &jo);

    if (ret < 0) {
        set_error(stream, ERR_SYM_BADMSGCONTENTS);
        goto done;
    }
    else if (ret == JPT_AUTH_PASSED) {
        PC_INFO("Passed the authentication\n");
        retv = on_auth_passed(stream, jo);
        goto done;
    }
    else if (ret == JPT_AUTH_FAILED) {
        PC_WARN("Failed the authentication\n");
        set_error(stream, ERR_SYM_AUTHFAILED);
        goto done;
    }
    else if (ret == JPT_ERROR) {
        set_error(stream, ERR_SYM_SERVERREFUSED);
        goto done;
    }
    else {
        set_error(stream, ERR_SYM_UNEXPECTED);
        goto done;
    }

    retv = 0;

done:
    if (jo)
        purc_variant_unref(jo);
    return retv;
}

const struct purc_native_ops *
dvobjs_extend_stream_by_hbdbus(struct pcdvobjs_stream *stream,
        const struct purc_native_ops *basic_ops, purc_variant_t extra_opts)
{
    (void)extra_opts;
    const struct stream_messaging_ops *msg_ops;

    if (basic_ops == NULL || ((msg_ops = basic_ops->priv_ops) == NULL)
            || strcmp(msg_ops->signature, SIGNATURE_MSG)) {
        PC_ERROR("Not extended from a message extension.\n");
        goto failed;
    }

    struct pcinst* inst = pcinst_current();
    if (inst == NULL) {
        PC_ERROR("No instance.\n");
        goto failed;
    }

    struct stream_extended *ext = calloc(1, sizeof(*ext));
    if (ext == NULL) {
        goto failed;
    }

    ext->inst = inst;
    pcutils_kvlist_init_ex(&ext->method_list, mhi_get_len, true);
    pcutils_kvlist_init_ex(&ext->bubble_list, NULL, true);
    pcutils_kvlist_init_ex(&ext->call_list, NULL, false);
    pcutils_kvlist_init_ex(&ext->subscribed_list, NULL, true);

    stream->ext = ext;
    stream->ext->super_ops = basic_ops;
    return &hbdbus_ops;

failed:
    return NULL;
}

#endif /* ENABLE(STREAM_HBDBUS) */
