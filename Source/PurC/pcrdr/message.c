/*
 * message.c -- The implementation of API to make, parse, serialize,
 *      and release a PurCMC message.
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
#include "private/pcrdr.h"
#include "private/instance.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

pcrdr_msg *pcrdr_make_void_message(void)
{
    pcrdr_msg *msg = pcinst_get_message();
    if (msg == NULL)
        return NULL;

    msg->type = PCRDR_MSG_TYPE_VOID;
    return msg;
}

pcrdr_msg *pcrdr_make_request_message(
        pcrdr_msg_target target, uint64_t target_value,
        const char *operation, const char *request_id, const char *source_uri,
        pcrdr_msg_element_type element_type, const char *element_value,
        const char *property,
        pcrdr_msg_data_type data_type, const char* data, size_t data_len)
{
    pcrdr_msg *msg = pcinst_get_message();
    if (msg == NULL)
        return NULL;

    msg->type = PCRDR_MSG_TYPE_REQUEST;
    msg->target = target;
    msg->targetValue = target_value;

    assert(operation);
    msg->operation = purc_variant_make_string(operation, true);
    if (msg->operation == NULL)
        goto failed;

    if (source_uri) {
        msg->sourceURI = purc_variant_make_string(source_uri, true);
        if (msg->sourceURI == NULL)
            goto failed;
    }

    msg->elementType = element_type;
    if (element_type == PCRDR_MSG_ELEMENT_TYPE_VOID) {
        msg->elementValue = NULL;
    }
    else {
        assert(element_value);
        msg->elementValue = purc_variant_make_string(element_value, true);
        if (msg->elementValue == NULL)
            goto failed;
    }

    if (property) {
        msg->property = purc_variant_make_string(property, true);
        if (msg->property == NULL)
            goto failed;
    }

    if (request_id == NULL) {
        char id_buf[PURC_LEN_UNIQUE_ID + 1];
        purc_generate_unique_id(id_buf, "REQ");
        msg->requestId = purc_variant_make_string(id_buf, false);
        if (msg->requestId == NULL)
            goto failed;
    }
    else {
        msg->requestId = purc_variant_make_string(request_id, true);
        if (msg->requestId == NULL)
            goto failed;
    }

    msg->dataType = data_type;
    if (data_type == PCRDR_MSG_DATA_TYPE_TEXT) {
        assert(data);
        msg->data = purc_variant_make_string_ex(data, data_len, true);

        if (msg->data == NULL) {
            goto failed;
        }
    }
    else if (data_type == PCRDR_MSG_DATA_TYPE_JSON) {
        assert(data);
        msg->data = purc_variant_make_from_json_string(data, data_len);

        if (msg->data == NULL) {
            goto failed;
        }
    }

    return msg;

failed:
    pcrdr_release_message(msg);
    return NULL;
}

pcrdr_msg *pcrdr_make_response_message(
        const char *request_id, const char *source_uri,
        unsigned int ret_code, uint64_t result_value,
        pcrdr_msg_data_type data_type, const char* data, size_t data_len)
{
    pcrdr_msg *msg = pcinst_get_message();
    if (msg == NULL)
        return NULL;

    assert(request_id);
    msg->type = PCRDR_MSG_TYPE_RESPONSE;
    msg->requestId = purc_variant_make_string(request_id, true);
    if (msg->requestId == NULL)
        goto failed;

    if (source_uri) {
        msg->sourceURI = purc_variant_make_string(source_uri, true);
        if (msg->sourceURI == NULL)
            goto failed;
    }

    msg->dataType = data_type;
    if (data_type == PCRDR_MSG_DATA_TYPE_TEXT) {
        assert(data);
        msg->data = purc_variant_make_string_ex(data, data_len, true);

        if (msg->data == NULL) {
            goto failed;
        }
    }
    else if (data_type == PCRDR_MSG_DATA_TYPE_JSON) {
        assert(data);
        msg->data = purc_variant_make_from_json_string(data, data_len);

        if (msg->data == NULL) {
            goto failed;
        }
    }

    msg->retCode = ret_code;
    msg->resultValue = result_value;

    return msg;

failed:
    pcrdr_release_message(msg);
    return NULL;
}

pcrdr_msg *pcrdr_make_event_message(
        pcrdr_msg_target target, uint64_t target_value,
        const char *event_name, const char *source_uri,
        pcrdr_msg_element_type element_type, const char *element_value,
        const char *property,
        pcrdr_msg_data_type data_type, const char* data, size_t data_len)
{
    pcrdr_msg *msg = pcinst_get_message();
    if (msg == NULL)
        return NULL;

    msg->type = PCRDR_MSG_TYPE_EVENT;
    msg->target = target;
    msg->targetValue = target_value;

    assert(event_name);
    msg->eventName = purc_variant_make_string(event_name, true);
    if (msg->eventName == NULL)
        goto failed;

    if (source_uri) {
        msg->sourceURI = purc_variant_make_string(source_uri, true);
        if (msg->sourceURI == NULL)
            goto failed;
    }

    msg->elementType = element_type;
    if (element_type == PCRDR_MSG_ELEMENT_TYPE_VOID) {
        msg->elementValue = NULL;
    }
    else {
        assert(element_value);
        msg->elementValue = purc_variant_make_string(element_value, true);
        if (msg->elementValue == NULL)
            goto failed;
    }

    if (property) {
        msg->property = purc_variant_make_string(property, true);
        if (msg->property == NULL)
            goto failed;
    }

    msg->dataType = data_type;
    if (data_type == PCRDR_MSG_DATA_TYPE_TEXT) {
        assert(data);
        msg->data = purc_variant_make_string_ex(data, data_len, true);

        if (msg->data == NULL) {
            goto failed;
        }
    }
    else if (data_type == PCRDR_MSG_DATA_TYPE_JSON) {
        assert(data);
        msg->data = purc_variant_make_from_json_string(data, data_len);

        if (msg->data == NULL) {
            goto failed;
        }
    }

    return msg;

failed:
    pcrdr_release_message(msg);
    return NULL;
}

static inline int
variant_strcmp(purc_variant_t a, purc_variant_t b)
{
    if (a == b)
        return 0;

    return strcmp(
            purc_variant_get_string_const(a),
            purc_variant_get_string_const(b));
}


int pcrdr_compare_messages(const pcrdr_msg *msg_a, const pcrdr_msg *msg_b)
{
    if (msg_a->type > msg_b->type)
        return 1;
    else if (msg_a->type < msg_b->type)
        return -1;

    if (msg_a->target > msg_b->target)
        return 1;
    else if (msg_a->target < msg_b->target)
        return -1;

    if (msg_a->targetValue > msg_b->targetValue)
        return 1;
    else if (msg_a->targetValue < msg_b->targetValue)
        return -1;

    if (msg_a->resultValue > msg_b->resultValue)
        return 1;
    else if (msg_a->resultValue < msg_b->resultValue)
        return -1;

    if (msg_a->elementType > msg_b->elementType)
        return 1;
    else if (msg_a->elementType < msg_b->elementType)
        return -1;

    if (msg_a->dataType > msg_b->dataType)
        return 1;
    else if (msg_a->dataType < msg_b->dataType)
        return -1;

    if (msg_a->retCode > msg_b->retCode)
        return 1;
    else if (msg_a->retCode < msg_b->retCode)
        return -1;

    for (int i = 0; i < PCRDR_NR_MSG_VARIANTS; i++) {
        if (msg_a->variants[i] && msg_b->variants[i]) {
            int ret = variant_strcmp(msg_a->variants[i], msg_b->variants[i]);
            if (ret) return ret;
        }
        else if (msg_a->variants[i]) {
            return 1;
        }
        else if (msg_b->variants[i]) {
            return -1;
        }
    }

    return 0;
}

pcrdr_msg *pcrdr_clone_message(const pcrdr_msg *src)
{
    pcrdr_msg *msg = pcinst_get_message();
    if (msg == NULL)
        return NULL;

    msg->type = src->type;
    msg->target = src->target;
    msg->targetValue = src->targetValue;
    msg->elementType = src->elementType;
    msg->dataType = src->dataType;
    msg->retCode = src->retCode;

    msg->targetValue = src->targetValue;
    msg->resultValue = src->resultValue;

    if (msg->type == PCRDR_MSG_TYPE_REQUEST) {
        assert(src->operation);
        msg->operation = purc_variant_ref(src->operation);

        assert(src->requestId);
        msg->requestId = purc_variant_ref(src->requestId);
    }
    else if (msg->type == PCRDR_MSG_TYPE_RESPONSE) {
        assert(src->requestId);
        msg->requestId = purc_variant_ref(src->requestId);
    }
    else if (msg->type == PCRDR_MSG_TYPE_EVENT) {
        assert(src->eventName);
        msg->eventName = purc_variant_ref(src->eventName);
    }

    if (src->sourceURI) {
        msg->sourceURI = purc_variant_ref(src->sourceURI);
    }

    if (src->elementValue) {
        msg->elementValue = purc_variant_ref(src->elementValue);
    }

    if (src->property) {
        msg->property = purc_variant_ref(src->property);
    }

    if (msg->dataType != PCRDR_MSG_DATA_TYPE_VOID) {
        msg->data = purc_variant_ref(src->data);
    }

    return msg;
}

void pcrdr_release_message(pcrdr_msg *msg)
{
    pcinst_put_message(msg);
}

static inline char *is_blank_line(char *line)
{
    while (*line) {
        if (*line == '\n') {
            return ++line;
        }
        else if (*line != ' ' && *line != '\t') {
            return NULL;
        }

        line++;
    }

    return ++line;
}

static inline char *skip_left_spaces(char *str)
{
    while (*str == ' ' || *str == '\t') {
        str++;
    }

    return str;
}

#define STR_PAIR_SEPARATOR      ":"
#define STR_LINE_SEPARATOR      "\n"
#define STR_VALUE_SEPARATOR     "/"
#define STR_MEMBER_SEPARATOR    ","

#define STR_BLANK_LINE          " \n"

static const char *type_names[] = {
    "void",       // PCRDR_MSG_TYPE_VOID,
    "request",    // PCRDR_MSG_TYPE_REQUEST,
    "response",   // PCRDR_MSG_TYPE_RESPONSE,
    "event",      // PCRDR_MSG_TYPE_EVENT,
};

/* make sure number of type_names matches the enums */
#define _COMPILE_TIME_ASSERT(name, x)           \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(types,
        PCA_TABLESIZE(type_names) == PCRDR_MSG_TYPE_NR);
#undef _COMPILE_TIME_ASSERT

static bool on_type(pcrdr_msg *msg, char *value)
{
    for (size_t i = 0; i < PCA_TABLESIZE(type_names); i++) {
        if (pcutils_strcasecmp(value, type_names[i]) == 0) {
            msg->type = PCRDR_MSG_TYPE_FIRST + i;
            return true;
        }
    }

    return false;
}

static const char *target_names[] = {
    "session",
    "workspace",
    "plainwindow",
    "widget",
    "dom",
    "instance",
    "coroutine",
    "user",
};

/* make sure number of target_names matches the enums */
#define _COMPILE_TIME_ASSERT(name, x)           \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(targets,
        PCA_TABLESIZE(target_names) == PCRDR_MSG_TARGET_NR);
#undef _COMPILE_TIME_ASSERT

static bool on_target(pcrdr_msg *msg, char *value)
{
    char *target, *target_value;
    char *saveptr;

    target = strtok_r(value, STR_VALUE_SEPARATOR, &saveptr);
    if (target == NULL)
        return false;

    target_value = strtok_r(NULL, STR_VALUE_SEPARATOR, &saveptr);
    if (target_value == NULL)
        return false;

    size_t i;
    for (i = 0; i < PCA_TABLESIZE(target_names); i++) {
        if (pcutils_strcasecmp(value, target_names[i]) == 0) {
            msg->target = PCRDR_MSG_TARGET_FIRST + i;
            break;
        }
    }
    if (i >= PCA_TABLESIZE(target_names)) {
        return false;
    }

    errno = 0;
    msg->targetValue = (uint64_t)strtoull(target_value, NULL, 16);

    if (errno)
        return false;

    return true;
}

static bool on_operation(pcrdr_msg *msg, char *value)
{
    msg->operation = purc_variant_make_string(value, true);
    if (msg->operation)
        return true;
    return false;
}

static bool on_event_name(pcrdr_msg *msg, char *value)
{
    msg->eventName = purc_variant_make_string(value, true);
    if (msg->eventName)
        return true;
    return false;
}

static bool on_source_uri(pcrdr_msg *msg, char *value)
{
    msg->sourceURI = purc_variant_make_string(value, true);
    if (msg->sourceURI)
        return true;
    return false;
}

static const char *element_type_names[] = {
    "void",
    "css",
    "xpath",
    "handle",
    "handles",
    "id",
    "variant",
};

/* make sure number of element_type_names matches the enums */
#define _COMPILE_TIME_ASSERT(name, x)           \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(element_types,
        PCA_TABLESIZE(element_type_names) == PCRDR_MSG_ELEMENT_TYPE_NR);
#undef _COMPILE_TIME_ASSERT

static bool on_element(pcrdr_msg *msg, char *value)
{
    char *type;
    char *saveptr;

    type = strtok_r(value, STR_VALUE_SEPARATOR, &saveptr);
    if (type == NULL)
        return false;

    size_t i;
    for (i = 0; i < PCA_TABLESIZE(element_type_names); i++) {
        if (pcutils_strcasecmp(value, element_type_names[i]) == 0) {
            msg->elementType = PCRDR_MSG_ELEMENT_TYPE_FIRST + i;
            break;
        }
    }
    if (i >= PCA_TABLESIZE(element_type_names)) {
        return false;
    }

    char *element_value = strtok_r(NULL, STR_VALUE_SEPARATOR, &saveptr);
    if (element_value == NULL)
        return false;

    msg->elementValue = purc_variant_make_string(element_value, true);
    if (msg->elementValue)
        return true;
    return false;
}

static bool on_property(pcrdr_msg *msg, char *value)
{
    msg->property = purc_variant_make_string(value, true);
    if (msg->property)
        return true;
    return false;
}

static bool on_request_id(pcrdr_msg *msg, char *value)
{
    msg->requestId = purc_variant_make_string(value, true);
    if (msg->requestId)
        return true;
    return false;
}

static bool on_result(pcrdr_msg *msg, char *value)
{
    char *subtoken;
    char *saveptr;

    subtoken = strtok_r(value, STR_VALUE_SEPARATOR, &saveptr);
    if (subtoken == NULL)
        return false;

    errno = 0;
    msg->retCode = (unsigned int)strtoul(subtoken, NULL, 10);
    if (errno)
        return false;

    subtoken = strtok_r(NULL, STR_VALUE_SEPARATOR, &saveptr);
    if (subtoken == NULL)
        return false;

    errno = 0;
    msg->resultValue = (uint64_t)strtoull(subtoken, NULL, 16);
    if (errno)
        return false;

    return true;
}

static const char *data_type_names [] = {
    "void",
    "json",
    "text",
};

/* make sure number of data_type_names matches the enums */
#define _COMPILE_TIME_ASSERT(name, x)           \
       typedef int _dummy_ ## name[(x) * 2 - 1]
_COMPILE_TIME_ASSERT(data_types,
        PCA_TABLESIZE(data_type_names) == PCRDR_MSG_DATA_TYPE_NR);
#undef _COMPILE_TIME_ASSERT

static bool on_data_type(pcrdr_msg *msg, char *value)
{
    for (size_t i = 0; i < PCA_TABLESIZE(data_type_names); i++) {
        if (pcutils_strcasecmp(value, data_type_names[i]) == 0) {
            msg->dataType = PCRDR_MSG_DATA_TYPE_FIRST + i;
            return true;
        }
    }

    return false;
}

static bool on_data_len(pcrdr_msg *msg, char *value)
{
    errno = 0;
    msg->__data_len = strtoul(value, NULL, 10);
    if (errno)
        return false;

    return true;
}

typedef bool (*key_op)(pcrdr_msg *msg, char *value);

#define STR_KEY_TYPE        "type"
#define STR_KEY_TARGET      "target"
#define STR_KEY_OPERATION   "operation"
#define STR_KEY_REQUEST_ID  "requestId"
#define STR_KEY_EVENTNAME   "eventName"
#define STR_KEY_SOURCEURI   "sourceURI"
#define STR_KEY_ELEMENT     "element"
#define STR_KEY_PROPERTY    "property"
#define STR_KEY_RESULT      "result"
#define STR_KEY_DATA_TYPE   "dataType"
#define STR_KEY_DATA_LEN    "dataLen"

static struct key_op_pair {
    const char *key;
    key_op      op;
} key_ops[] = {
    { STR_KEY_DATA_LEN,     on_data_len },
    { STR_KEY_DATA_TYPE,    on_data_type },
    { STR_KEY_ELEMENT,      on_element },
    { STR_KEY_EVENTNAME,    on_event_name },
    { STR_KEY_OPERATION,    on_operation },
    { STR_KEY_PROPERTY,     on_property },
    { STR_KEY_REQUEST_ID,   on_request_id },
    { STR_KEY_RESULT,       on_result },
    { STR_KEY_SOURCEURI,    on_source_uri },
    { STR_KEY_TARGET,       on_target },
    { STR_KEY_TYPE,         on_type },
};

static key_op find_key_op(const char* key)
{
    static ssize_t max = sizeof(key_ops)/sizeof(key_ops[0]) - 1;

    ssize_t low = 0, high = max, mid;
    while (low <= high) {
        int cmp;

        mid = (low + high) / 2;
        cmp = pcutils_strcasecmp(key, key_ops[mid].key);
        if (cmp == 0) {
            goto found;
        }
        else {
            if (cmp < 0) {
                high = mid - 1;
            }
            else {
                low = mid + 1;
            }
        }
    }

    return NULL;

found:
    return key_ops[mid].op;
}

int pcrdr_parse_packet(char *packet, size_t sz_packet, pcrdr_msg **msg_out)
{
    pcrdr_msg *msg;

    char *str1;
    char *line;
    char *saveptr1;
    char *data;

    UNUSED_PARAM(sz_packet);

    if ((msg = pcinst_get_message()) == NULL) {
        purc_set_error(PCRDR_ERROR_NOMEM);
        return -1;
    }

    for (str1 = packet; ; str1 = NULL) {
        line = strtok_r(str1, STR_LINE_SEPARATOR, &saveptr1);
        if (line == NULL) {
            goto failed;
        }

        if ((data = is_blank_line(line))) {
            break;
        }

        char *key, *value;
        char *saveptr2;
        key = strtok_r(line, STR_PAIR_SEPARATOR, &saveptr2);
        if (key == NULL) {
            goto failed;
        }

#if 0
        value = strtok_r(NULL, STR_PAIR_SEPARATOR, &saveptr2);
        if (value == NULL) {
            goto failed;
        }
#else
        /* XXX: to support pattern: `eventName:create:tabbedwindow` */
        value = line + strlen(key) + 1;
        if (value[0] == '\0') {
            goto failed;
        }
#endif

        key_op op = find_key_op(key);
        if (op == NULL) {
            goto failed;
        }

        if (!op(msg, skip_left_spaces(value))) {
            goto failed;
        }
    }

    if (msg->dataType == PCRDR_MSG_DATA_TYPE_TEXT) {
        // FIXME: check __data_len ???
        assert(data != NULL /* && msg->__data_len > 0 */);
        msg->data = purc_variant_make_string_ex(data, msg->__data_len, true);

        if (msg->data == NULL) {
            goto failed;
        }
    }
    else if (msg->dataType == PCRDR_MSG_DATA_TYPE_JSON) {
        assert(data != NULL && msg->__data_len > 0);
        msg->data = purc_variant_make_from_json_string(data, msg->__data_len);

        if (msg->data == NULL) {
            goto failed;
        }
    }

    *msg_out = msg;
    return 0;

failed:
    for (int i = 0; i < PCRDR_NR_MSG_VARIANTS; i++) {
        if (msg->variants[i])
            purc_variant_unref(msg->variants[i]);
    }

    purc_set_error(PCRDR_ERROR_BAD_MESSAGE);
    return -1;
}

#define LEN_BUFF_LONGLONGINT    128

static int
serialize_message_data(const pcrdr_msg *msg, pcrdr_cb_write fn, void *ctxt)
{
    char buff[LEN_BUFF_LONGLONGINT];
    int n, errcode = 0;
    size_t text_len = 0;
    const char *text = NULL;
    char *text_alloc = NULL;

    if (msg->dataType == PCRDR_MSG_DATA_TYPE_TEXT) {
        text = purc_variant_get_string_const_ex(msg->data, &text_len);
        assert(msg->data != NULL);
        if (msg->textLen > 0)   /* override by textLen */
            text_len = msg->textLen;
    }
    else if (msg->dataType == PCRDR_MSG_DATA_TYPE_JSON) {
        purc_rwstream_t buffer = NULL;
        buffer = purc_rwstream_new_buffer(PCRDR_MIN_PACKET_BUFF_SIZE,
                PCRDR_MAX_INMEM_PAYLOAD_SIZE);

        /* always serialize as a standard JSON */
        if (purc_variant_serialize(msg->data, buffer, 0,
                PCVARIANT_SERIALIZE_OPT_PLAIN, NULL) < 0) {
            errcode = purc_get_last_error();
            goto done;
        }

        text_alloc = purc_rwstream_get_mem_buffer_ex(buffer, &text_len,
                NULL, true);
        text = text_alloc;
        purc_rwstream_destroy(buffer);
    }

    /* dataType: <void | json | text> */
    fn(ctxt, STR_KEY_DATA_TYPE, sizeof(STR_KEY_DATA_TYPE) - 1);
    fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
    fn(ctxt, data_type_names[msg->dataType],
            strlen(data_type_names[msg->dataType]));
    fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

    /* dataLen: <data_length> */
    fn(ctxt, STR_KEY_DATA_LEN, sizeof(STR_KEY_DATA_LEN) - 1);
    fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
    n = snprintf(buff, sizeof(buff), "%lu", (unsigned long int)text_len);
    if (n < 0) {
        errcode = PCRDR_ERROR_UNEXPECTED;
        goto done;
    }
    else if ((size_t)n >= sizeof (buff)) {
        PC_DEBUG ("Too small buffer to serialize message.\n");
        errcode = PCRDR_ERROR_TOO_SMALL_BUFF;
        goto done;
    }

    fn(ctxt, buff, n);
    fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

    /* a blank line */
    fn(ctxt, STR_BLANK_LINE, sizeof(STR_BLANK_LINE) - 1);

    if (text && text_len > 0) {
        /* the data */
        fn(ctxt, text, text_len);
    }

done:
    if (text_alloc)
        free(text_alloc);

    return errcode;
}

int pcrdr_serialize_message(const pcrdr_msg *msg, pcrdr_cb_write fn, void *ctxt)
{
    int n = 0;
    char buff[LEN_BUFF_LONGLONGINT];
    const char *value;

    /* type: <request | response | event> */
    fn(ctxt, STR_KEY_TYPE, sizeof(STR_KEY_TYPE) - 1);
    fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
    fn(ctxt, type_names[msg->type], strlen(type_names[msg->type]));
    fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

    if (msg->type == PCRDR_MSG_TYPE_REQUEST) {
        /* target: <session | window | tab | dom>/<handle> */
        fn(ctxt, STR_KEY_TARGET, sizeof(STR_KEY_TARGET) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        fn(ctxt, target_names[msg->target], strlen(target_names[msg->target]));
        fn(ctxt, STR_VALUE_SEPARATOR, sizeof(STR_VALUE_SEPARATOR) - 1);

        n = snprintf(buff, sizeof(buff),
                "%llx", (unsigned long long int)msg->targetValue);
        if (n < 0)
            return PCRDR_ERROR_UNEXPECTED;
        else if ((size_t)n >= sizeof (buff)) {
            PC_DEBUG ("Too small buffer to serialize message.\n");
            return PCRDR_ERROR_TOO_SMALL_BUFF;
        }
        fn(ctxt, buff, n);
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        /* operation: <operation> */
        fn(ctxt, STR_KEY_OPERATION, sizeof(STR_KEY_OPERATION) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        value = purc_variant_get_string_const(msg->operation);
        fn(ctxt, value, strlen(value));
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        if (msg->elementType != PCRDR_MSG_ELEMENT_TYPE_VOID) {
            /* element: <css | xpath | handle | handles>/<elementValue> */
            fn(ctxt, STR_KEY_ELEMENT, sizeof(STR_KEY_ELEMENT) - 1);
            fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
            fn(ctxt, element_type_names[msg->elementType],
                    strlen(element_type_names[msg->elementType]));
            fn(ctxt, STR_VALUE_SEPARATOR, sizeof(STR_VALUE_SEPARATOR) - 1);
            value = purc_variant_get_string_const(msg->elementValue);
            fn(ctxt, value, strlen(value));
            fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);
        }

        if (msg->property) {
            /* property: <property> */
            fn(ctxt, STR_KEY_PROPERTY, sizeof(STR_KEY_PROPERTY) - 1);
            fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
            value = purc_variant_get_string_const(msg->property);
            fn(ctxt, value, strlen(value));
            fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);
        }

        /* requestId: <requestId> */
        fn(ctxt, STR_KEY_REQUEST_ID, sizeof(STR_KEY_REQUEST_ID) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        value = purc_variant_get_string_const(msg->requestId);
        fn(ctxt, value, strlen(value));
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        /* sourceURI: <event> */
        fn(ctxt, STR_KEY_SOURCEURI, sizeof(STR_KEY_SOURCEURI) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        if (msg->sourceURI)
            value = purc_variant_get_string_const(msg->sourceURI);
        else
            value = PCRDR_SOURCEURI_ANONYMOUS;
        fn(ctxt, value, strlen(value));
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        n = serialize_message_data(msg, fn, ctxt);
    }
    else if (msg->type == PCRDR_MSG_TYPE_RESPONSE) {
        /* requestId: <requestId> */
        fn(ctxt, STR_KEY_REQUEST_ID, sizeof(STR_KEY_REQUEST_ID) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        value = purc_variant_get_string_const(msg->requestId);
        fn(ctxt, value, strlen(value));
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        /* sourceURI: <event> */
        fn(ctxt, STR_KEY_SOURCEURI, sizeof(STR_KEY_SOURCEURI) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        if (msg->sourceURI)
            value = purc_variant_get_string_const(msg->sourceURI);
        else
            value = PCRDR_SOURCEURI_ANONYMOUS;
        fn(ctxt, value, strlen(value));
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        /* result: <retCode>/<resultValue> */
        fn(ctxt, STR_KEY_RESULT, sizeof(STR_KEY_RESULT) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        n = snprintf(buff, sizeof(buff), "%u", msg->retCode);
        if (n < 0)
            return PCRDR_ERROR_UNEXPECTED;
        else if ((size_t)n >= sizeof (buff)) {
            PC_DEBUG ("Too small buffer to serialize message.\n");
            return PCRDR_ERROR_TOO_SMALL_BUFF;
        }
        fn(ctxt, buff, n);
        fn(ctxt, STR_VALUE_SEPARATOR, sizeof(STR_VALUE_SEPARATOR) - 1);
        n = snprintf(buff, sizeof(buff),
                "%llx", (unsigned long long int)msg->resultValue);
        if (n < 0)
            return PCRDR_ERROR_UNEXPECTED;
        else if ((size_t)n >= sizeof (buff)) {
            PC_DEBUG ("Too small buffer to serialize message.\n");
            return PCRDR_ERROR_TOO_SMALL_BUFF;
        }
        fn(ctxt, buff, n);
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        n = serialize_message_data(msg, fn, ctxt);
    }
    else if (msg->type == PCRDR_MSG_TYPE_EVENT) {
        /* target: <session | window | tab | dom>/<handle> */
        fn(ctxt, STR_KEY_TARGET, sizeof(STR_KEY_TARGET) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        fn(ctxt, target_names[msg->target], strlen(target_names[msg->target]));
        fn(ctxt, STR_VALUE_SEPARATOR, sizeof(STR_VALUE_SEPARATOR) - 1);

        n = snprintf(buff, sizeof(buff),
                "%llx", (unsigned long long int)msg->targetValue);
        if (n < 0)
            return PCRDR_ERROR_UNEXPECTED;
        else if ((size_t)n >= sizeof (buff)) {
            PC_DEBUG ("Too small buffer to serialize message.\n");
            return PCRDR_ERROR_TOO_SMALL_BUFF;
        }
        fn(ctxt, buff, n);
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        /* eventName: <event> */
        fn(ctxt, STR_KEY_EVENTNAME, sizeof(STR_KEY_EVENTNAME) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        value = purc_variant_get_string_const(msg->eventName);
        fn(ctxt, value, strlen(value));
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        /* sourceURI: <event> */
        fn(ctxt, STR_KEY_SOURCEURI, sizeof(STR_KEY_SOURCEURI) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        if (msg->sourceURI)
            value = purc_variant_get_string_const(msg->sourceURI);
        else
            value = PCRDR_SOURCEURI_ANONYMOUS;
        fn(ctxt, value, strlen(value));
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        if (msg->elementType != PCRDR_MSG_ELEMENT_TYPE_VOID) {
            /* element: <css | xpath | handle | handles>/<elementValue> */
            fn(ctxt, STR_KEY_ELEMENT, sizeof(STR_KEY_ELEMENT) - 1);
            fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
            fn(ctxt, element_type_names[msg->elementType],
                    strlen(element_type_names[msg->elementType]));
            fn(ctxt, STR_VALUE_SEPARATOR, sizeof(STR_VALUE_SEPARATOR) - 1);
            value = purc_variant_get_string_const(msg->elementValue);
            fn(ctxt, value, strlen(value));
            fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);
        }

        if (msg->property) {
            /* property: <property> */
            fn(ctxt, STR_KEY_PROPERTY, sizeof(STR_KEY_PROPERTY) - 1);
            fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
            value = purc_variant_get_string_const(msg->property);
            fn(ctxt, value, strlen(value));
            fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);
        }

        n = serialize_message_data(msg, fn, ctxt);
    }
    else {
        assert(0);
        n = PCRDR_ERROR_BAD_MESSAGE;
    }

    return n;
}

struct buff_info {
    char *  buf;    /* buffer address */
    size_t  size;   /* size of buffer */

    size_t  n;      /* number of characters will be written */
    off_t   pos;    /* current position */
};

static ssize_t write_to_buff (void *ctxt, const void *buf, size_t count)
{
    struct buff_info *info = (struct buff_info *)ctxt;

    info->n += count;
    if (info->pos + count <= info->size) {
        memcpy (info->buf + info->pos, buf, count);
        info->pos += count;
        return count;
    }
    else {
        ssize_t n = info->size - info->pos;

        if (n > 0) {
            memcpy (info->buf + info->pos, buf, n);
            info->pos += n;
            return n;
        }

        return 0;
    }

    return -1;
}

size_t pcrdr_serialize_message_to_buffer(const pcrdr_msg *msg,
        void *buff, size_t sz)
{
    struct buff_info buff_info = { buff, sz, 0, 0 };

    pcrdr_serialize_message(msg, write_to_buff, &buff_info);

    return buff_info.n;
}

struct renderer_capabilities *
pcrdr_parse_renderer_capabilities(const char *data)
{
    struct renderer_capabilities *rdr_caps;

    char *lines;
    char *str1;
    char *line;
    char *saveptr1;

    int line_no = 0;

    lines = strdup(data);
    rdr_caps = calloc(1, sizeof(*rdr_caps));
    if (lines == NULL || rdr_caps == NULL) {
        purc_set_error(PCRDR_ERROR_NOMEM);
        return NULL;
    }

    for (str1 = lines; ; str1 = NULL) {
        line = strtok_r(str1, STR_LINE_SEPARATOR, &saveptr1);
        if (line == NULL) {
            break;
        }

        if (line_no == 0) {
            char *prot_name, *prot_version;
            char *saveptr2;
            prot_name = strtok_r(line, STR_PAIR_SEPARATOR, &saveptr2);
            if (prot_name == NULL) {
                goto failed;
            }

            prot_version = strtok_r(NULL, STR_PAIR_SEPARATOR, &saveptr2);
            if (prot_version == NULL) {
                goto failed;
            }

            rdr_caps->prot_name = strdup(prot_name);
            if (rdr_caps->prot_name == NULL)
                goto failed;

            rdr_caps->prot_version = strtol(prot_name, NULL, 10);
        }
        else if (line_no == 1) {    /* markup versions */
            char *str2, *value;
            char *saveptr2;

            for (str2 = line; ; str2 = NULL) {
                value = strtok_r(str2, STR_VALUE_SEPARATOR, &saveptr2);
                if (value == NULL) {
                    break;
                }

                char *markup, *version;
                char *saveptr3;

                markup = strtok_r(value, STR_PAIR_SEPARATOR, &saveptr3);
                if (markup == NULL) {
                    goto failed;
                }

                version = strtok_r(NULL, STR_PAIR_SEPARATOR, &saveptr3);
                if (version == NULL) {
                    goto failed;
                }

                if (pcutils_strcasecmp(markup, "html") == 0) {
                    rdr_caps->html_version = strdup(version);
                    if (rdr_caps->html_version == NULL)
                        goto failed;
                }
                else if (pcutils_strcasecmp(markup, "xgml") == 0) {
                    rdr_caps->xgml_version = strdup(version);
                    if (rdr_caps->xgml_version == NULL)
                        goto failed;
                }
                else if (pcutils_strcasecmp(markup, "xml") == 0) {
                    rdr_caps->xml_version = strdup(version);
                    if (rdr_caps->xml_version == NULL)
                        goto failed;
                }
            }
        }
        else if (line_no == 2) {    /* windowing capabilities */
            char *str2, *value;
            char *saveptr2;

            for (str2 = line; ; str2 = NULL) {
                value = strtok_r(str2, STR_VALUE_SEPARATOR, &saveptr2);
                if (value == NULL) {
                    break;
                }

                char *cap, *limit;
                char *saveptr3;

                cap = strtok_r(value, STR_PAIR_SEPARATOR, &saveptr3);
                if (cap == NULL) {
                    goto failed;
                }

                limit = strtok_r(NULL, STR_PAIR_SEPARATOR, &saveptr3);
                if (limit == NULL) {
                    goto failed;
                }

                if (pcutils_strcasecmp(cap, "workspace") == 0) {
                    rdr_caps->workspace = strtol(limit, NULL, 10);
                }
                else if (pcutils_strcasecmp(cap, "tabbedWindow") == 0) {
                    rdr_caps->tabbedWindow = strtol(limit, NULL, 10);
                }
                else if (pcutils_strcasecmp(cap, "widgetInTabbedWindow") == 0) {
                    rdr_caps->widgetInTabbedWindow = strtol(limit, NULL, 10);
                }
                else if (pcutils_strcasecmp(cap, "plainWindow") == 0) {
                    rdr_caps->plainWindow = strtol(limit, NULL, 10);
                }
            }
        }
        else if (line_no >= 3) {
            char *cap, *value;
            char *saveptr2;

            cap = strtok_r(line, STR_PAIR_SEPARATOR, &saveptr2);
            if (cap == NULL) {
                break;
            }

            value = strtok_r(NULL, STR_PAIR_SEPARATOR, &saveptr2);
            if (value == NULL) {
                break;
            }

#if 0
            if (pcutils_strcasecmp(cap, "windowLevels") == 0) {
                if (rdr_caps->windowLevel <= 0) {
                    PC_WARN("Found windowLevels but windowLevel is <= 0");
                    break;
                }

                rdr_caps->window_levels =
                    calloc(rdr_caps->windowLevel, sizeof(char *));

                char *str3, *member;
                char *saveptr3;
                int n = 0;
                for (str3 = value; ; str3 = NULL) {
                    member = strtok_r(str3, STR_MEMBER_SEPARATOR, &saveptr3);
                    if (member == NULL) {
                        break;
                    }

                    if (n < rdr_caps->windowLevel) {
                        rdr_caps->window_levels[n] = strdup(member);
                        n++;
                    }
                    else
                        break;
                }

                // adjust windowLevel
                rdr_caps->windowLevel = n;
            }
            else {
                PC_WARN("Unknown renderer capability: %s\n", cap);
                break;
            }

            if (rdr_caps->windowLevel > 0 &&
                    rdr_caps->window_levels == NULL) {
                PC_WARN("windowLevels does not match windowLevel\n");
                rdr_caps->windowLevel = 0;
            }
#endif
            PC_WARN("Unknown renderer capability: %s\n", cap);
            break;
        }

        line_no++;
    }

    free(lines);
    return rdr_caps;

failed:
    free(lines);
    pcrdr_release_renderer_capabilities(rdr_caps);
    purc_set_error(PCRDR_ERROR_BAD_MESSAGE);
    return NULL;
}

void pcrdr_release_renderer_capabilities(
        struct renderer_capabilities *rdr_caps)
{
    assert (rdr_caps != NULL);

    if (rdr_caps->prot_name)
        free(rdr_caps->prot_name);

    if (rdr_caps->html_version)
        free(rdr_caps->html_version);

    if (rdr_caps->xgml_version)
        free(rdr_caps->xgml_version);

    if (rdr_caps->xml_version)
        free(rdr_caps->xml_version);

#if 0
    if (rdr_caps->windowLevel > 0) {
        assert(rdr_caps->window_levels);

        for (int i = 0; i < rdr_caps->windowLevel; i++) {
            if (rdr_caps->window_levels[i])
                free(rdr_caps->window_levels[i]);
        }
        free(rdr_caps->window_levels);
    }
#endif

    free(rdr_caps);
}

