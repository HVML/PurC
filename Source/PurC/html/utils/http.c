/**
 * @file http.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of http protocol.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
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

#include "purc.h"
#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "html/utils/http.h"
#include "html/core/conv.h"


#ifndef PCHTML_UTILS_HTTP_MAX_HEADER_FIELD
    #define PCHTML_UTILS_HTTP_MAX_HEADER_FIELD 4096 * 32
#endif


enum {
    PCHTML_UTILS_HTTP_STATE_HEAD_VERSION = 0x00,
    PCHTML_UTILS_HTTP_STATE_HEAD_FIELD,
    PCHTML_UTILS_HTTP_STATE_HEAD_FIELD_WS,
    PCHTML_UTILS_HTTP_STATE_HEAD_END,
    PCHTML_UTILS_HTTP_STATE_BODY,
    PCHTML_UTILS_HTTP_STATE_BODY_END
};


static inline unsigned int
pchtml_utils_http_split_field(pchtml_utils_http_t *http, const pchtml_str_t *str)
{
    unsigned char *p, *end;
    pchtml_utils_http_field_t *field;

    field = pchtml_array_obj_push(http->fields);
    if (field == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    p = memchr(str->data, ':', str->length);

    if (p == NULL) {
        http->error = "Wrong header field format.";

        (void) pchtml_array_obj_pop(http->fields);

        return PCHTML_STATUS_ABORTED;
    }

    field->name.data = str->data;
    field->name.length = p - str->data;

    if (field->name.length == 0) {
        (void) pchtml_array_obj_pop(http->fields);

        return PCHTML_STATUS_OK;
    }

    p++;
    end = str->data + str->length;

    /* Skip whitespaces before */
    for (; p < end; p++) {
        if (*p != ' ' && *p != '\t') {
            break;
        }
    }

    /* Skip whitespaces after */
    while (end > p) {
        end--;

        if (*end != ' ' && *end != '\t') {
            end++;
            break;
        }
    }

    field->value.data = p;
    field->value.length = end - p;

    return PCHTML_STATUS_OK;
}

pchtml_utils_http_t *
pchtml_utils_http_create(void)
{
    return pchtml_calloc(1, sizeof(pchtml_utils_http_t));
}

unsigned int
pchtml_utils_http_init(pchtml_utils_http_t *http, pchtml_mraw_t *mraw)
{
    unsigned int status;

    if (http == NULL) {
        pcinst_set_error (PCHTML_OBJECT_IS_NULL);
        return PCHTML_STATUS_ERROR_OBJECT_IS_NULL;
    }

    if (mraw == NULL) {
        mraw = pchtml_mraw_create();
        status = pchtml_mraw_init(mraw, 4096 * 4);
        if (status) {
            return status;
        }
    }

    http->mraw = mraw;

    http->fields = pchtml_array_obj_create();
    status = pchtml_array_obj_init(http->fields, 32,
                                   sizeof(pchtml_utils_http_field_t));
    if (status) {
        return status;
    }

    pchtml_str_init(&http->tmp, http->mraw, 64);
    if (http->tmp.data == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    pchtml_str_init(&http->version.name, http->mraw, 8);
    if (http->version.name.data == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    http->error = NULL;
    http->state = PCHTML_UTILS_HTTP_STATE_HEAD_VERSION;

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_utils_http_clear(pchtml_utils_http_t *http)
{
    pchtml_mraw_clean(http->mraw);
    pchtml_array_obj_clean(http->fields);
    pchtml_str_clean_all(&http->tmp);

    http->tmp.data = NULL;

    pchtml_str_init(&http->tmp, http->mraw, 64);
    if (http->tmp.data == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    http->version.name.data = NULL;
    http->version.number = 0;

    pchtml_str_init(&http->version.name, http->mraw, 8);
    if (http->version.name.data == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    http->error = NULL;
    http->state = PCHTML_UTILS_HTTP_STATE_HEAD_VERSION;

    return PCHTML_STATUS_OK;
}

pchtml_utils_http_t *
pchtml_utils_http_destroy(pchtml_utils_http_t *http, bool self_destroy)
{
    if (http == NULL) {
        return NULL;
    }

    http->mraw = pchtml_mraw_destroy(http->mraw, true);
    http->fields = pchtml_array_obj_destroy(http->fields, true);

    if (self_destroy) {
        return pchtml_free(http);
    }

    return http;
}

unsigned int
pchtml_utils_http_header_parse_eof(pchtml_utils_http_t *http)
{
    if (http->state != PCHTML_UTILS_HTTP_STATE_HEAD_END) {
        http->error = "Unexpected data termination.";

        return PCHTML_STATUS_ABORTED;
    }

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_utils_http_parse_version(pchtml_utils_http_t *http, const unsigned char **data,
                             const unsigned char *end)
{
    pchtml_str_t *str;
    const unsigned char *p;

    str = &http->version.name;

    p = memchr(*data, '\n', (end - *data));

    if (p == NULL) {
        p = pchtml_str_append(str, http->mraw, *data, (end - *data));
        if (p == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }

        *data = end;

        if (str->length > PCHTML_UTILS_HTTP_MAX_HEADER_FIELD) {
            goto to_large;
        }

        return PCHTML_STATUS_OK;
    }

    *data = pchtml_str_append(str, http->mraw, *data, (p - *data));
    if (*data == NULL) {
        *data = p;
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    *data = p + 1;

    if (str->length < 9 || str->data[(str->length - 1)] != '\r') {
        goto failed;
    }

    (void) pchtml_str_length_set(str, http->mraw, (str->length - 1));

    if (str->length > PCHTML_UTILS_HTTP_MAX_HEADER_FIELD) {
        goto to_large;
    }

    if (pchtml_str_data_ncasecmp(str->data,
                                 (const unsigned char *) "HTTP/", 5) == false)
    {
        goto failed;
    }

    /* Skip version */
    p = str->data + 5;

    http->version.number = pchtml_conv_data_to_double(&p, 3);
    if (http->version.number < 1.0 || http->version.number > 1.1) {
        goto failed;
    }

    /* Skip version */
    end = str->data + str->length;

    if (p != end) {
        if (*p != ' ' && *p != '\t') {
            goto failed;
        }

        /* Skip space */
        for (; p < end; p++) {
            if (*p != ' ' && *p != '\t') {
                break;
            }
        }

        http->version.status = pchtml_conv_data_to_uint(&p, end - p);
        if (http->version.status < 100 || http->version.status >= 600) {
            goto failed;
        }
    }

    http->state = PCHTML_UTILS_HTTP_STATE_HEAD_FIELD;
    http->tmp.length = 0;

    return PCHTML_STATUS_OK;

to_large:

    http->error = "Too large header version field.";

    return PCHTML_STATUS_ABORTED;

failed:

    http->error = "Wrong HTTP version.";

    return PCHTML_STATUS_ABORTED;
}

static inline unsigned int
pchtml_utils_http_parse_field(pchtml_utils_http_t *http, const unsigned char **data,
                           const unsigned char *end)
{
    pchtml_str_t *str;
    const unsigned char *p;

    str = &http->tmp;

    p = memchr(*data, '\n', (end - *data));

    if (p == NULL) {
        p = pchtml_str_append(str, http->mraw, *data, (end - *data));
        if (p == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }

        *data = end;

        if (str->length > PCHTML_UTILS_HTTP_MAX_HEADER_FIELD) {
            goto to_large;
        }

        return PCHTML_STATUS_OK;
    }

    *data = pchtml_str_append(str, http->mraw, *data, (p - *data));
    if (*data == NULL) {
        *data = p;
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    *data = p + 1;

    if (str->length == 0 || str->data[(str->length - 1)] != '\r') {
        goto failed;
    }

    (void) pchtml_str_length_set(str, http->mraw, (str->length - 1));

    /* Check for end of header */
    if (str->length != 0) {
        if (str->length > PCHTML_UTILS_HTTP_MAX_HEADER_FIELD) {
            goto to_large;
        }

        http->state = PCHTML_UTILS_HTTP_STATE_HEAD_FIELD_WS;

        return PCHTML_STATUS_OK;
    }

    http->state = PCHTML_UTILS_HTTP_STATE_HEAD_END;

    return PCHTML_STATUS_OK;

to_large:

    http->error = "Too large header field.";

    return PCHTML_STATUS_ABORTED;

failed:

    http->error = "Wrong HTTP header filed.";

    return PCHTML_STATUS_ABORTED;
}

static inline unsigned int
pchtml_utils_http_parse_field_ws(pchtml_utils_http_t *http, const unsigned char **data,
                              const unsigned char *end)
{
    unsigned int status;
    const unsigned char *p = *data;

    if (*p != ' ' && *p != '\t') {
        status = pchtml_utils_http_split_field(http, &http->tmp);
        if (status != PCHTML_STATUS_OK) {
            return status;
        }

        pchtml_str_clean_all(&http->tmp);

        pchtml_str_init(&http->tmp, http->mraw, 64);
        if (http->tmp.data == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }

        http->state = PCHTML_UTILS_HTTP_STATE_HEAD_FIELD;

        return PCHTML_STATUS_OK;
    }

    for (; p < end; p++) {
        if (*p != ' ' && *p != '\t') {
            *data = p;

            http->state = PCHTML_UTILS_HTTP_STATE_HEAD_FIELD;

            return PCHTML_STATUS_OK;
        }
    }

    *data = p;

    return PCHTML_STATUS_OK;
}

/*
static inline unsigned int
pchtml_utils_http_parse_body(pchtml_utils_http_t *http, const unsigned char **data,
                          const unsigned char *end)
{
    return PCHTML_STATUS_OK;
}
*/

static inline unsigned int
pchtml_utils_http_parse_body_end(pchtml_utils_http_t *http,
                              const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(http);
    UNUSED_PARAM(data);
    UNUSED_PARAM(end);

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_utils_http_parse(pchtml_utils_http_t *http,
                     const unsigned char **data, const unsigned char *end)
{
    unsigned int status = PCHTML_STATUS_ERROR;

    while (*data < end) {
        switch (http->state) {

            case PCHTML_UTILS_HTTP_STATE_HEAD_VERSION:
                status = pchtml_utils_http_parse_version(http, data, end);
                break;

            case PCHTML_UTILS_HTTP_STATE_HEAD_FIELD:
                status = pchtml_utils_http_parse_field(http, data, end);
                break;

            case PCHTML_UTILS_HTTP_STATE_HEAD_FIELD_WS:
                status = pchtml_utils_http_parse_field_ws(http, data, end);
                break;

            case PCHTML_UTILS_HTTP_STATE_HEAD_END:
                return PCHTML_STATUS_OK;

            case PCHTML_UTILS_HTTP_STATE_BODY_END:
                status = pchtml_utils_http_parse_body_end(http, data, end);
                break;
        }

        if (status != PCHTML_STATUS_OK) {
            return status;
        }
    }

    /*
     * TODO:
     * We cannot know whether we have a body or not.
     * Need to implementation reading of the body.
     *
     * Please, see Content-Length and Transfer-Encoding with "chunked".
     */
    if (http->state == PCHTML_UTILS_HTTP_STATE_HEAD_END) {
        return PCHTML_STATUS_OK;
    }

    return PCHTML_STATUS_NEXT;
}

pchtml_utils_http_field_t *
pchtml_utils_http_header_field(pchtml_utils_http_t *http, const unsigned char *name,
                            size_t len, size_t offset)
{
    pchtml_utils_http_field_t *field;

    for (size_t i = 0; i < pchtml_array_obj_length(http->fields); i++) {
        field = pchtml_array_obj_get(http->fields, i);

        if (field->name.length == len
            && pchtml_str_data_ncasecmp(field->name.data, name, len))
        {
            if (offset == 0) {
                return field;
            }

            offset--;
        }
    }

    return NULL;
}

unsigned int
pchtml_utils_http_header_serialize(pchtml_utils_http_t *http, pchtml_str_t *str)
{
    unsigned int status;
    const pchtml_utils_http_field_t *field;

    if (str->data == NULL) {
        pchtml_str_init(str, http->mraw, 256);
        if (str->data == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    for (size_t i = 0; i < pchtml_array_obj_length(http->fields); i++) {
        field = pchtml_array_obj_get(http->fields, i);

        status = pchtml_utils_http_field_serialize(http, str, field);
        if (status != PCHTML_STATUS_OK) {
            return status;
        }
    }

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_utils_http_field_serialize(pchtml_utils_http_t *http, pchtml_str_t *str,
                               const pchtml_utils_http_field_t *field)
{
    unsigned char *data;

    data = pchtml_str_append(str, http->mraw, field->name.data,
                             field->name.length);
    if (data == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    data = pchtml_str_append(str, http->mraw, (unsigned char *) ": ", 2);
    if (data == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    data = pchtml_str_append(str, http->mraw, field->value.data,
                             field->value.length);
    if (data == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    data = pchtml_str_append_one(str, http->mraw, '\n');
    if (data == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    return PCHTML_STATUS_OK;
}
