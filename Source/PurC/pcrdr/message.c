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
#include "purc.h"
#include "private/debug.h"
#include "private/pcrdr.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

pcrdr_msg *pcrdr_make_request_message(
        pcrdr_msg_target target, uintptr_t target_value,
        const char *operation,
        const char *request_id,
        pcrdr_msg_element_type element_type, const char *element,
        const char *property,
        pcrdr_msg_data_type data_type, const char* data, size_t data_len)
{
    pcrdr_msg *msg = calloc(1, sizeof(pcrdr_msg));
    if (msg == NULL)
        return NULL;

    msg->type = PCRDR_MSG_TYPE_REQUEST;
    msg->target = target;
    msg->targetValue = target_value;

    assert(operation);
    msg->operation = strdup(operation);
    if (msg->operation == NULL)
        goto failed;

    msg->elementType = element_type;
    if (element_type == PCRDR_MSG_ELEMENT_TYPE_VOID) {
        msg->element = NULL;
    }
    else {
        assert(element);
        msg->element = strdup(element);
        if (msg->element == NULL)
            goto failed;
    }

#if 0
    if (element_type == PCRDR_MSG_ELEMENT_TYPE_CSS ||
            element_type == PCRDR_MSG_ELEMENT_TYPE_XPATH) {
    }
    else { /* PCRDR_MSG_ELEMENT_TYPE_HANDLES */
        assert(element);

        uintptr_t *handles = element;
        size_t nr_handles = 0;
        while (*handles) {
            nr_handles++;
            handles++;
        }

        msg->element = calloc(nr_handles + 1, sizeof(uintptr_t));
        if (msg->element == NULL)
            goto failed;

        memcpy(msg->element, element, sizeof(uintptr_t) * (nr_handles + 1));
    }
#endif

    if (property) {
        msg->property = strdup(property);
        if (msg->property == NULL)
            goto failed;
    }

    if (request_id == NULL) {
        char id_buf[PCRDR_LEN_UNIQUE_ID + 1];
        pcrdr_generate_unique_id(id_buf, "REQ");
        msg->requestId = strdup(id_buf);
        if (msg->requestId == NULL)
            goto failed;
    }
    else {
        msg->requestId = strdup(request_id);
        if (msg->requestId == NULL)
            goto failed;
    }

    msg->dataType = data_type;
    if (data_type != PCRDR_MSG_DATA_TYPE_VOID) {
        assert(data);
        if (data_len) {
            msg->dataLen = data_len;
            msg->data = malloc(data_len);
            if (msg->data)
                memcpy (msg->data, data, data_len);
        }
        else {
            msg->dataLen = strlen(data);
            msg->data = strdup(data);
        }

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
        const char *request_id,
        unsigned int ret_code, uintptr_t result_value,
        pcrdr_msg_data_type data_type, const char* data, size_t data_len)
{
    pcrdr_msg *msg = calloc(1, sizeof(pcrdr_msg));
    if (msg == NULL)
        return NULL;

    assert(request_id);
    msg->type = PCRDR_MSG_TYPE_RESPONSE;
    msg->requestId = strdup(request_id);
    if (msg->requestId == NULL)
        goto failed;

    msg->dataType = data_type;
    if (data_type != PCRDR_MSG_DATA_TYPE_VOID) {
        assert(data);
        if (data_len) {
            msg->dataLen = data_len;
            msg->data = malloc(data_len);
            if (msg->data)
                memcpy (msg->data, data, data_len);
        }
        else {
            msg->dataLen = strlen(data);
            msg->data = strdup(data);
        }

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
        pcrdr_msg_target target, uintptr_t target_value,
        const char *event,
        pcrdr_msg_element_type element_type, const char *element,
        const char *property,
        pcrdr_msg_data_type data_type, const char* data, size_t data_len)
{
    pcrdr_msg *msg = calloc(1, sizeof(pcrdr_msg));
    if (msg == NULL)
        return NULL;

    msg->type = PCRDR_MSG_TYPE_EVENT;
    msg->target = target;
    msg->targetValue = target_value;

    assert(event);
    msg->event = strdup(event);
    if (msg->event == NULL)
        goto failed;

    msg->elementType = element_type;
    if (element_type == PCRDR_MSG_ELEMENT_TYPE_VOID) {
        msg->element = NULL;
    }
    else {
        assert(element);
        msg->element = strdup(element);
        if (msg->element == NULL)
            goto failed;
    }

#if 0
    if (element_type == PCRDR_MSG_ELEMENT_TYPE_CSS ||
            element_type == PCRDR_MSG_ELEMENT_TYPE_XPATH) {
        assert(element);
        msg->element = strdup(element);
        if (msg->element == NULL)
            goto failed;
    }
    else { /* PCRDR_MSG_ELEMENT_TYPE_HANDLES */
        assert(element);

        uintptr_t *handles = element;
        size_t nr_handles = 0;
        while (*handles) {
            nr_handles++;
            handles++;
        }

        msg->element = calloc(nr_handles + 1, sizeof(uintptr_t));
        if (msg->element == NULL)
            goto failed;

        memcpy(msg->element, element, sizeof(uintptr_t) * (nr_handles + 1));
    }
#endif

    if (property) {
        msg->property = strdup(property);
        if (msg->property == NULL)
            goto failed;
    }

    msg->dataType = data_type;
    if (data_type != PCRDR_MSG_DATA_TYPE_VOID) {
        assert(data);
        if (data_len) {
            msg->dataLen = data_len;
            msg->data = malloc(data_len);
            if (msg->data)
                memcpy (msg->data, data, data_len);
        }
        else {
            msg->dataLen = strlen(data);
            msg->data = strdup(data);
        }

        if (msg->data == NULL) {
            goto failed;
        }
    }

    return msg;

failed:
    pcrdr_release_message(msg);
    return NULL;
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

    if (msg_a->operation && msg_b->operation) {
        int ret = strcmp(msg_a->operation, msg_b->operation);
        if (ret) return ret;
    }
    else if (msg_a->operation) {
        return 1;
    }
    else if (msg_b->operation) {
        return -1;
    }

    if (msg_a->element && msg_b->element) {
        int ret = strcmp(msg_a->element, msg_b->element);
        if (ret) return ret;
    }
    else if (msg_a->element) {
        return 1;
    }
    else if (msg_b->element) {
        return -1;
    }

    if (msg_a->property && msg_b->property) {
        int ret = strcmp(msg_a->property, msg_b->property);
        if (ret) return ret;
    }
    else if (msg_a->property) {
        return 1;
    }
    else if (msg_b->property) {
        return -1;
    }

    if (msg_a->event && msg_b->event) {
        int ret = strcmp(msg_a->event, msg_b->event);
        if (ret) return ret;
    }
    else if (msg_a->event) {
        return 1;
    }
    else if (msg_b->event) {
        return -1;
    }

    if (msg_a->requestId && msg_b->requestId) {
        int ret = strcmp(msg_a->requestId, msg_b->requestId);
        if (ret) return ret;
    }
    else if (msg_a->requestId) {
        return 1;
    }
    else if (msg_b->requestId) {
        return -1;
    }

    if (msg_a->data && msg_b->data) {
        int ret = strncmp(msg_a->data, msg_b->data,
                (msg_a->dataLen > msg_b->dataLen) ?
                    msg_b->dataLen : msg_a->dataLen);
        if (ret) return ret;
    }
    else if (msg_a->data) {
        return 1;
    }
    else if (msg_b->data) {
        return -1;
    }

    return 0;
}

void pcrdr_release_message(pcrdr_msg *msg)
{
    if (msg->operation)
        free(msg->operation);

    if (msg->element)
        free(msg->element);

    if (msg->property)
        free(msg->property);

    if (msg->event)
        free(msg->event);

    if (msg->requestId)
        free(msg->requestId);

    if (msg->data)
        free(msg->data);

    free(msg);
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

#define STR_BLANK_LINE          " \n"

static bool on_type(pcrdr_msg *msg, char *value)
{
    if (strcasecmp(value, "request") == 0) {
        msg->type = PCRDR_MSG_TYPE_REQUEST;
    }
    else if (strcasecmp(value, "response") == 0) {
        msg->type = PCRDR_MSG_TYPE_RESPONSE;
    }
    else if (strcasecmp(value, "event") == 0) {
        msg->type = PCRDR_MSG_TYPE_EVENT;
    }
    else {
        return false;
    }

    return true;
}

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

    if (strcasecmp(target, "session") == 0) {
        msg->target = PCRDR_MSG_TARGET_SESSION;
    }
    else if (strcasecmp(target, "window") == 0) {
        msg->target = PCRDR_MSG_TARGET_WINDOW;
    }
    else if (strcasecmp(target, "tab") == 0) {
        msg->target = PCRDR_MSG_TARGET_TAB;
    }
    else if (strcasecmp(target, "dom") == 0) {
        msg->target = PCRDR_MSG_TARGET_DOM;
    }
    else {
        return false;
    }

    errno = 0;
    if (sizeof(uintptr_t) == sizeof(unsigned long int))
        msg->targetValue = strtoul(target_value, NULL, 16);
    else if (sizeof(uintptr_t) == sizeof(unsigned long long int))
        msg->targetValue = strtoull(target_value, NULL, 16);
    else
        assert(0);

    if (errno)
        return false;

    return true;
}

static bool on_operation(pcrdr_msg *msg, char *value)
{
    msg->operation = value;
    return true;
}

static bool on_event(pcrdr_msg *msg, char *value)
{
    msg->event = value;
    return true;
}

static bool on_element(pcrdr_msg *msg, char *value)
{
    char *type;
    char *saveptr;

    type = strtok_r(value, STR_VALUE_SEPARATOR, &saveptr);
    if (type == NULL)
        return false;

    if (strcasecmp(type, "css") == 0) {
        msg->elementType = PCRDR_MSG_ELEMENT_TYPE_CSS;
    }
    else if (strcasecmp(type, "xpath") == 0) {
        msg->elementType = PCRDR_MSG_ELEMENT_TYPE_XPATH;
    }
    else if (strcasecmp(type, "handle") == 0) {
        msg->elementType = PCRDR_MSG_ELEMENT_TYPE_HANDLE;
    }
    else {
        return false;
    }

    msg->element = strtok_r(NULL, STR_VALUE_SEPARATOR, &saveptr);
    if (msg->element == NULL)
        return false;

    return true;
}

static bool on_property(pcrdr_msg *msg, char *value)
{
    msg->property = value;
    return true;
}

static bool on_request_id(pcrdr_msg *msg, char *value)
{
    msg->requestId = value;
    return true;
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
    msg->resultValue = (uintptr_t)strtoull(subtoken, NULL, 16);
    if (errno)
        return false;

    return true;
}

static bool on_data_type(pcrdr_msg *msg, char *value)
{
    if (strcasecmp(value, "void") == 0) {
        msg->dataType = PCRDR_MSG_DATA_TYPE_VOID;
    }
    else if (strcasecmp(value, "ejson") == 0) {
        msg->dataType = PCRDR_MSG_DATA_TYPE_EJSON;
    }
    else if (strcasecmp(value, "text") == 0) {
        msg->dataType = PCRDR_MSG_DATA_TYPE_TEXT;
    }
    else {
        return false;
    }

    return true;
}

static bool on_data_len(pcrdr_msg *msg, char *value)
{
    errno = 0;
    msg->dataLen = strtoul(value, NULL, 16);

    if (errno) {
        return false;
    }

    return true;
}

typedef bool (*key_op)(pcrdr_msg *msg, char *value);

#define STR_KEY_TYPE        "type"
#define STR_KEY_TARGET      "target"
#define STR_KEY_OPERATION   "operation"
#define STR_KEY_ELEMENT     "element"
#define STR_KEY_PROPERTY    "property"
#define STR_KEY_EVENT       "event"
#define STR_KEY_REQUEST_ID  "requestId"
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
    { STR_KEY_EVENT,        on_event },
    { STR_KEY_OPERATION,    on_operation },
    { STR_KEY_PROPERTY,     on_property },
    { STR_KEY_REQUEST_ID,   on_request_id },
    { STR_KEY_RESULT,       on_result },
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
        cmp = strcasecmp(key, key_ops[mid].key);
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
    pcrdr_msg msg;

    char *str1;
    char *line;
    char *saveptr1;

    UNUSED_PARAM(sz_packet);

    memset(&msg, 0, sizeof(msg));

    for (str1 = packet; ; str1 = NULL) {
        line = strtok_r(str1, STR_LINE_SEPARATOR, &saveptr1);
        if (line == NULL) {
            goto failed;
        }

        if ((msg.data = is_blank_line(line))) {
            break;
        }

        char *key, *value;
        char *saveptr2;
        key = strtok_r(line, STR_PAIR_SEPARATOR, &saveptr2);
        if (key == NULL) {
            goto failed;
        }

        value = strtok_r(NULL, STR_PAIR_SEPARATOR, &saveptr2);
        if (value == NULL) {
            goto failed;
        }

        key_op op = find_key_op(key);
        if (op == NULL) {
            goto failed;
        }

        if (!op(&msg, skip_left_spaces(value))) {
            goto failed;
        }
    }

    pcrdr_msg *_msg;
    if (msg.type == PCRDR_MSG_TYPE_REQUEST) {
        _msg = pcrdr_make_request_message(msg.target, msg.targetValue,
                msg.operation,
                msg.requestId,
                msg.elementType, msg.element, msg.property,
                msg.dataType, msg.data, msg.dataLen);
    }
    else if (msg.type == PCRDR_MSG_TYPE_RESPONSE) {
        _msg = pcrdr_make_response_message(
                msg.requestId,
                msg.retCode, msg.resultValue,
                msg.dataType, msg.data, msg.dataLen);
    }
    else if (msg.type == PCRDR_MSG_TYPE_EVENT) {
        _msg = pcrdr_make_event_message(msg.target, msg.targetValue,
                msg.event,
                msg.elementType,
                msg.element,
                msg.property,
                msg.dataType,
                msg.data, msg.dataLen);
    }

    if (_msg == NULL) {
        goto failed;
    }

    *msg_out = _msg;
    return 0;

failed:
    return PCRDR_ERROR_BAD_MESSAGE;
}

static const char *type_names [] = {
    "request",      /* PCRDR_MSG_TYPE_REQUEST */
    "response",     /* PCRDR_MSG_TYPE_RESPONSE */
    "event",        /* PCRDR_MSG_TYPE_EVENT */
};

static const char *target_names [] = {
    "session",
    "window",
    "tab",
    "dom",
};

static const char *element_type_names [] = {
    "void",
    "css",
    "xpath",
    "handle",
};

static const char *data_type_names [] = {
    "void",
    "ejson",
    "text",
};

int pcrdr_serialize_message(const pcrdr_msg *msg, cb_write fn, void *ctxt)
{
    int n;
    char buff[128];

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
            PC_DEBUG ("Too small buffer for serialize message.\n");
            return PCRDR_ERROR_TOO_SMALL_BUFF;
        }
        fn(ctxt, buff, n);
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        /* operation: <operation> */
        fn(ctxt, STR_KEY_OPERATION, sizeof(STR_KEY_OPERATION) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        fn(ctxt, msg->operation, strlen(msg->operation));
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        if (msg->elementType != PCRDR_MSG_ELEMENT_TYPE_VOID) {
            /* element: <css | xpath | handle>/<element> */
            fn(ctxt, STR_KEY_ELEMENT, sizeof(STR_KEY_ELEMENT) - 1);
            fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
            fn(ctxt, element_type_names[msg->elementType],
                    strlen(element_type_names[msg->elementType]));
            fn(ctxt, STR_VALUE_SEPARATOR, sizeof(STR_VALUE_SEPARATOR) - 1);
            fn(ctxt, msg->element, strlen(msg->element));
            fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);
        }

        if (msg->property) {
            /* property: <property> */
            fn(ctxt, STR_KEY_PROPERTY, sizeof(STR_KEY_PROPERTY) - 1);
            fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
            fn(ctxt, msg->property, strlen(msg->property));
            fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);
        }

        /* requestId: <requestId> */
        fn(ctxt, STR_KEY_REQUEST_ID, sizeof(STR_KEY_REQUEST_ID) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        fn(ctxt, msg->requestId, strlen(msg->requestId));
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        /* dataType: <void | ejson | text> */
        fn(ctxt, STR_KEY_DATA_TYPE, sizeof(STR_KEY_DATA_TYPE) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        fn(ctxt, data_type_names[msg->dataType],
                strlen(data_type_names[msg->dataType]));
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        /* dataLen: <data_length> */
        fn(ctxt, STR_KEY_DATA_LEN, sizeof(STR_KEY_DATA_LEN) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        n = snprintf(buff, sizeof(buff), "%lu", (unsigned long int)msg->dataLen);
        if (n < 0)
            return PCRDR_ERROR_UNEXPECTED;
        else if ((size_t)n >= sizeof (buff)) {
            PC_DEBUG ("Too small buffer for serialize message.\n");
            return PCRDR_ERROR_TOO_SMALL_BUFF;
        }
        fn(ctxt, buff, n);
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        /* a blank line */
        fn(ctxt, STR_BLANK_LINE, sizeof(STR_BLANK_LINE) - 1);

        /* the data */
        fn(ctxt, msg->data, msg->dataLen);
    }
    else if (msg->type == PCRDR_MSG_TYPE_RESPONSE) {
        /* requestId: <requestId> */
        fn(ctxt, STR_KEY_REQUEST_ID, sizeof(STR_KEY_REQUEST_ID) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        fn(ctxt, msg->requestId, strlen(msg->requestId));
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        /* result: <retCode>/<resultValue> */
        fn(ctxt, STR_KEY_RESULT, sizeof(STR_KEY_RESULT) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        n = snprintf(buff, sizeof(buff), "%u", msg->retCode);
        if (n < 0)
            return PCRDR_ERROR_UNEXPECTED;
        else if ((size_t)n >= sizeof (buff)) {
            PC_DEBUG ("Too small buffer for serialize message.\n");
            return PCRDR_ERROR_TOO_SMALL_BUFF;
        }
        fn(ctxt, buff, n);
        fn(ctxt, STR_VALUE_SEPARATOR, sizeof(STR_VALUE_SEPARATOR) - 1);
        n = snprintf(buff, sizeof(buff),
                "%llx", (unsigned long long int)msg->resultValue);
        if (n < 0)
            return PCRDR_ERROR_UNEXPECTED;
        else if ((size_t)n >= sizeof (buff)) {
            PC_DEBUG ("Too small buffer for serialize message.\n");
            return PCRDR_ERROR_TOO_SMALL_BUFF;
        }
        fn(ctxt, buff, n);
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        /* dataType: <void | ejson | text> */
        fn(ctxt, STR_KEY_DATA_TYPE, sizeof(STR_KEY_DATA_TYPE) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        fn(ctxt, data_type_names[msg->dataType],
                strlen(data_type_names[msg->dataType]));
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        /* dataLen: <data_length> */
        fn(ctxt, STR_KEY_DATA_LEN, sizeof(STR_KEY_DATA_LEN) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        n = snprintf(buff, sizeof(buff), "%lu", (unsigned long int)msg->dataLen);
        if (n < 0)
            return PCRDR_ERROR_UNEXPECTED;
        else if ((size_t)n >= sizeof (buff)) {
            PC_DEBUG ("Too small buffer for serialize message.\n");
            return PCRDR_ERROR_TOO_SMALL_BUFF;
        }
        fn(ctxt, buff, n);
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        /* a blank line */
        fn(ctxt, STR_BLANK_LINE, sizeof(STR_BLANK_LINE) - 1);

        /* the extra data */
        fn(ctxt, msg->data, msg->dataLen);
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
            PC_DEBUG ("Too small buffer for serialize message.\n");
            return PCRDR_ERROR_TOO_SMALL_BUFF;
        }
        fn(ctxt, buff, n);
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        /* event: <event> */
        fn(ctxt, STR_KEY_EVENT, sizeof(STR_KEY_EVENT) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        fn(ctxt, msg->event, strlen(msg->event));
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        if (msg->elementType != PCRDR_MSG_ELEMENT_TYPE_VOID) {
            /* element: <css | xpath | handle>/<element> */
            fn(ctxt, STR_KEY_ELEMENT, sizeof(STR_KEY_ELEMENT) - 1);
            fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
            fn(ctxt, element_type_names[msg->target],
                    strlen(element_type_names[msg->target]));
            fn(ctxt, STR_VALUE_SEPARATOR, sizeof(STR_VALUE_SEPARATOR) - 1);
            fn(ctxt, msg->element, strlen(msg->element));
            fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);
        }

        if (msg->property) {
            /* property: <property> */
            fn(ctxt, STR_KEY_PROPERTY, sizeof(STR_KEY_PROPERTY) - 1);
            fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
            fn(ctxt, msg->property, strlen(msg->property));
            fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);
        }

        /* dataType: <void | ejson | text> */
        fn(ctxt, STR_KEY_DATA_TYPE, sizeof(STR_KEY_DATA_TYPE) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        fn(ctxt, data_type_names[msg->target],
                strlen(data_type_names[msg->target]));
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        /* dataLen: <data_length> */
        fn(ctxt, STR_KEY_DATA_LEN, sizeof(STR_KEY_DATA_LEN) - 1);
        fn(ctxt, STR_PAIR_SEPARATOR, sizeof(STR_PAIR_SEPARATOR) - 1);
        n = snprintf(buff, sizeof(buff), "%lu", (unsigned long int)msg->dataLen);
        if (n < 0)
            return PCRDR_ERROR_UNEXPECTED;
        else if ((size_t)n >= sizeof (buff)) {
            PC_DEBUG ("Too small buffer for serialize message.\n");
            return PCRDR_ERROR_TOO_SMALL_BUFF;
        }
        fn(ctxt, buff, n);
        fn(ctxt, STR_LINE_SEPARATOR, sizeof(STR_LINE_SEPARATOR) - 1);

        /* a blank line */
        fn(ctxt, STR_BLANK_LINE, sizeof(STR_BLANK_LINE) - 1);

        /* the data */
        fn(ctxt, msg->data, msg->dataLen);
    }
    else {
        assert(0);
    }

    return 0;
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

struct renderer_capabilities *pcrdr_parse_renderer_capabilities(char *data,
        size_t data_len)
{
    struct renderer_capabilities *rdr_caps;

    char *str1;
    char *line;
    char *saveptr1;

    int line_no = 0;

    UNUSED_PARAM(data_len);

    rdr_caps = calloc(1, sizeof(*rdr_caps));
    if (rdr_caps == NULL) {
        purc_set_error(PCRDR_ERROR_NOMEM);
        return NULL;
    }

    for (str1 = data; ; str1 = NULL) {
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

                if (strcasecmp(markup, "html") == 0) {
                    rdr_caps->html_version = strdup(version);
                    if (rdr_caps->html_version == NULL)
                        goto failed;
                }
                else if (strcasecmp(markup, "xgml") == 0) {
                    rdr_caps->xgml_version = strdup(version);
                    if (rdr_caps->xgml_version == NULL)
                        goto failed;
                }
                else if (strcasecmp(markup, "xml") == 0) {
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

                if (strcasecmp(cap, "workspace") == 0) {
                    rdr_caps->workspace = strtol(limit, NULL, 10);
                }
                else if (strcasecmp(cap, "tabbedWindow") == 0) {
                    rdr_caps->tabbedWindow = strtol(limit, NULL, 10);
                }
                else if (strcasecmp(cap, "tabbedPage") == 0) {
                    rdr_caps->tabbedPage = strtol(limit, NULL, 10);
                }
                else if (strcasecmp(cap, "plainWindow") == 0) {
                    rdr_caps->plainWindow = strtol(limit, NULL, 10);
                }
                else if (strcasecmp(cap, "windowLevel") == 0) {
                    rdr_caps->windowLevel = strtol(limit, NULL, 10);
                }
            }
        }

        line_no++;
        if (line_no > 2)
            break;
    }

    return rdr_caps;

failed:
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

    free(rdr_caps);
}

