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

#define HBDBUS_SYSTEM_EVENT_ID          "NOTIFICATION"

#define ERR_SYM_BADINITIALMSG           "badInitialMsg"
#define ERR_SYM_BADMSGCONTENTS          "badMsgContents"
#define ERR_SYM_SERVERREFUSED           "serverRefused"
#define ERR_SYM_WRONGVERSION            "wrongVersion"
#define ERR_SYM_OUTOFMEMORY             "outOfMemory"
#define ERR_SYM_UNEXPECTED              "unexpected"
#define ERR_SYM_TOOSMALLBUFFER          "tooSmallBuffer"
#define ERR_SYM_FAILEDWRITE             "failedWrite"

#define set_error(stream, sym)          stream->ext->errsym = sym ""

typedef void (*hbdbus_error_handler)(struct pcdvobjs_stream *stream,
        const purc_variant_t jo);
typedef void (*hbdbus_event_handler)(struct pcdvobjs_stream *stream,
        const char *from_endpoint, const char *from_bubble,
        const char *bubble_data);

struct stream_extended {
    const struct pcinst *inst;
    const char *errsym;

    struct pcutils_kvlist method_list;
    struct pcutils_kvlist bubble_list;
    struct pcutils_kvlist call_list;
    struct pcutils_kvlist subscribed_list;

    hbdbus_error_handler error_handler;
    hbdbus_event_handler system_event_handler;
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

static const struct purc_native_ops *super_ops;
#define call_super(method, x, ...)    \
    ((struct stream_messaging_ops*)super_ops->priv_ops)->method(x, ##__VA_ARGS__)

static purc_nvariant_method
property_getter(void *entity, const char *name)
{
    UNUSED_PARAM(entity);

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

    if (method == NULL && super_ops && super_ops->property_getter) {
        return super_ops->property_getter(entity, name);
    }

failed:
    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
    return NULL;
}

static const struct purc_native_ops hbdbus_ops = {
    .property_getter = property_getter,
};

static int get_challenge_code(pcdvobjs_stream *stream, char **challenge)
{
    char *payload;
    size_t len;
    int ret = -1;
    int type = MSG_DATA_TYPE_UNKNOWN;
    purc_variant_t jo = NULL, jo_tmp;
    const char *ch_code = NULL;

    call_super(read_message_alloc, stream, &payload, &len, &type);
    if (type != MSG_DATA_TYPE_TEXT || payload == NULL) {
        set_error(stream, ERR_SYM_BADINITIALMSG);
        goto failed;
    }

    jo = purc_variant_make_from_json_string(payload, len);
    if (jo == NULL || !purc_variant_is_object(jo)) {
        set_error(stream, ERR_SYM_BADMSGCONTENTS);
        goto failed;
    }

    free(payload);
    payload = NULL;

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
    if (payload)
        free(payload);

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

    if (call_super(send_text, stream, buff, n)) {
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
    super_ops = basic_ops;
    return &hbdbus_ops;

failed:
    return NULL;
}

#endif /* ENABLE(STREAM_HBDBUS) */
