/**
 * @file warc.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of html parser.
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

#include "html/utils/warc.h"

#include "html/core/fs.h"
#include "html/core/conv.h"


#ifndef PCHTML_UTILS_WARC_MAX_HEADER_NAME
    #define PCHTML_UTILS_WARC_MAX_HEADER_NAME 4096 * 4
#endif

#ifndef PCHTML_UTILS_WARC_MAX_HEADER_VALUE
    #define PCHTML_UTILS_WARC_MAX_HEADER_VALUE 4096 * 32
#endif


enum {
    PCHTML_UTILS_WARC_STATE_HEAD_VERSION = 0x00,
    PCHTML_UTILS_WARC_STATE_HEAD_VERSION_AFTER,
    PCHTML_UTILS_WARC_STATE_HEAD_FIELD_NAME,
    PCHTML_UTILS_WARC_STATE_HEAD_FIELD_VALUE,
    PCHTML_UTILS_WARC_STATE_HEAD_FIELD_VALUE_QUOTED,
    PCHTML_UTILS_WARC_STATE_HEAD_FIELD_VALUE_AFTER,
    PCHTML_UTILS_WARC_STATE_HEAD_FIELD_VALUE_WS,
    PCHTML_UTILS_WARC_STATE_HEAD_END,
    PCHTML_UTILS_WARC_STATE_BLOCK,
    PCHTML_UTILS_WARC_STATE_BLOCK_AFTER
};

static const unsigned char pchtml_utils_warc_seporators_ctl[0x80] =
{
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01
};

/*
static const unsigned char pchtml_utils_warc_ctl[0x80] =
{
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};
*/


static inline pchtml_utils_warc_field_t *
pchtml_utils_warc_field_append(pchtml_utils_warc_t *warc, unsigned char *name,
                            size_t len)
{
    pchtml_utils_warc_field_t *field;

    field = pchtml_array_obj_push(warc->fields);
    if (field == NULL) {
        return NULL;
    }

    field->name.data = name;
    field->name.length = len;

    return field;
}

static inline pchtml_utils_warc_field_t *
pchtml_utils_warc_field_last(pchtml_utils_warc_t *warc)
{
    return pchtml_array_obj_last(warc->fields);
}


pchtml_utils_warc_t *
pchtml_utils_warc_create(void)
{
    return pchtml_calloc(1, sizeof(pchtml_utils_warc_t));
}

unsigned int
pchtml_utils_warc_init(pchtml_utils_warc_t *warc, pchtml_utils_warc_header_cb_f h_cd,
                    pchtml_utils_warc_content_cb_f c_cb,
                    pchtml_utils_warc_content_end_cb_f c_end_cb, void *ctx)
{
    unsigned int status;

    if (warc == NULL) {
        pcinst_set_error (PCHTML_OBJECT_IS_NULL);
        return PCHTML_STATUS_ERROR_OBJECT_IS_NULL;
    }

    warc->mraw = pchtml_mraw_create();
    status = pchtml_mraw_init(warc->mraw, 4096 * 4);
    if (status) {
        return status;
    }

    warc->fields = pchtml_array_obj_create();
    status = pchtml_array_obj_init(warc->fields, 32,
                                   sizeof(pchtml_utils_warc_field_t));
    if (status) {
        return status;
    }

    pchtml_str_init(&warc->tmp, warc->mraw, 64);
    if (warc->tmp.data == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    pchtml_str_init(&warc->version.type, warc->mraw, 8);
    if (warc->version.type.data == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    warc->header_cb = h_cd;
    warc->content_cb = c_cb;
    warc->content_end_cb = c_end_cb;

    warc->error = NULL;
    warc->state = PCHTML_UTILS_WARC_STATE_HEAD_VERSION;
    warc->count = 0;
    warc->ctx = ctx;
    warc->skip = false;

    return PCHTML_STATUS_OK;
}

unsigned int
pchtml_utils_warc_clear(pchtml_utils_warc_t *warc)
{
    pchtml_mraw_clean(warc->mraw);
    pchtml_array_obj_clean(warc->fields);
    pchtml_str_clean_all(&warc->tmp);

    warc->tmp.data = NULL;

    pchtml_str_init(&warc->tmp, warc->mraw, 64);
    if (warc->tmp.data == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    warc->version.type.data = NULL;
    warc->version.number = 0;

    pchtml_str_init(&warc->version.type, warc->mraw, 8);
    if (warc->version.type.data == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    warc->error = NULL;
    warc->state = PCHTML_UTILS_WARC_STATE_HEAD_VERSION;
    warc->skip = false;

    return PCHTML_STATUS_OK;
}

pchtml_utils_warc_t *
pchtml_utils_warc_destroy(pchtml_utils_warc_t *warc, bool self_destroy)
{
    if (warc == NULL) {
        return NULL;
    }

    warc->mraw = pchtml_mraw_destroy(warc->mraw, true);
    warc->fields = pchtml_array_obj_destroy(warc->fields, true);

    if (self_destroy) {
        return pchtml_free(warc);
    }

    return warc;
}

unsigned int
pchtml_utils_warc_parse_file(pchtml_utils_warc_t *warc, FILE *fh)
{
    size_t size;
    unsigned int status;

    const unsigned char *buf_ref;
    unsigned char buffer[4096 * 2];

    if (fh == NULL) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PCHTML_STATUS_ERROR_WRONG_ARGS;
    }

    do {
        buf_ref = buffer;

        size = fread(buffer, sizeof(unsigned char), sizeof(buffer), fh);
        if (size != sizeof(buffer)) {
            if (feof(fh)) {
                return pchtml_utils_warc_parse(warc, &buf_ref, (buffer + size));
            }

            pcinst_set_error (PCHTML_ERROR);
            return PCHTML_STATUS_ERROR;
        }

        status = pchtml_utils_warc_parse(warc, &buf_ref,
                                      (buffer + sizeof(buffer)));
    }
    while (status == PCHTML_STATUS_OK);

    return pchtml_utils_warc_parse_eof(warc);
}

unsigned int
pchtml_utils_warc_parse_eof(pchtml_utils_warc_t *warc)
{
    if (warc->state != PCHTML_UTILS_WARC_STATE_HEAD_VERSION) {
        warc->error = "Unexpected data termination.";

        return PCHTML_STATUS_ABORTED;
    }

    warc->count = 0;

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_utils_warc_parse_version(pchtml_utils_warc_t *warc, const unsigned char **data,
                             const unsigned char *end)
{
    pchtml_str_t *str;
    const unsigned char *p;

    str = &warc->version.type;

    p = memchr(*data, '\n', (end - *data));

    if (p == NULL) {
        p = pchtml_str_append(str, warc->mraw, *data, (end - *data));
        if (p == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }

        *data = end;

        if (str->length > 9) {
            goto failed;
        }

        return PCHTML_STATUS_OK;
    }

    *data = pchtml_str_append(str, warc->mraw, *data, (p - *data));
    if (*data == NULL) {
        *data = p;
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    *data = p + 1;

    if (str->length < 9 || str->data[(str->length - 1)] != '\r') {
        goto failed;
    }

    pchtml_str_length_set(str, warc->mraw, (str->length - 1));

    if (pchtml_str_data_ncasecmp(str->data,
                                 (const unsigned char *) "warc/", 5) == false)
    {
        goto failed;
    }

    p = str->data + 5;

    warc->version.number = pchtml_conv_data_to_double(&p, 3);
    if (warc->version.number != 1.0) {
        goto failed;
    }

    warc->state = PCHTML_UTILS_WARC_STATE_HEAD_VERSION_AFTER;
    warc->tmp.length = 0;

    return PCHTML_STATUS_OK;

failed:

    warc->error = "Wrong warc version.";

    return PCHTML_STATUS_ABORTED;
}

static inline unsigned int
pchtml_utils_warc_parse_field_version_after(pchtml_utils_warc_t *warc,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(end);

    warc->content_length = 0;

    if (**data != '\r') {
        warc->state = PCHTML_UTILS_WARC_STATE_HEAD_FIELD_NAME;

        return PCHTML_STATUS_OK;
    }

    (*data)++;

    warc->state = PCHTML_UTILS_WARC_STATE_HEAD_END;

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_utils_warc_parse_field_name(pchtml_utils_warc_t *warc, const unsigned char **data,
                          const unsigned char *end)
{
    const unsigned char *p;
    pchtml_utils_warc_field_t *field;

    for (p = *data; p < end; p++) {
        if (*p == ':') {
            *data = pchtml_str_append(&warc->tmp, warc->mraw, *data,
                                      (p - *data));
            if (*data == NULL) {
                *data = p;
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
            }

            *data = p + 1;

            field = pchtml_utils_warc_field_append(warc, warc->tmp.data,
                                          warc->tmp.length);
            if (field == NULL) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
            }

            pchtml_str_init(&field->value, warc->mraw, 0);
            if (field->value.data == NULL) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
            }

            pchtml_str_clean_all(&warc->tmp);

            pchtml_str_init(&warc->tmp, warc->mraw, 64);
            if (warc->tmp.data == NULL) {
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
            }

            warc->state = PCHTML_UTILS_WARC_STATE_HEAD_FIELD_VALUE_WS;

            return PCHTML_STATUS_OK;
        }

        if (*p > 0x7F || pchtml_utils_warc_seporators_ctl[*p] != 0x00) {
            *data = p;

            warc->error = "Wrong header field name.";

            return PCHTML_STATUS_ABORTED;
        }
    }

    p = pchtml_str_append(&warc->tmp, warc->mraw, *data, (p - *data));
    if (p == NULL) {
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    if (warc->tmp.length > PCHTML_UTILS_WARC_MAX_HEADER_NAME) {
        warc->error = "Too large header field name.";

        return PCHTML_STATUS_ABORTED;
    }

    *data = end;

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_utils_warc_parse_field_value(pchtml_utils_warc_t *warc,
                                 const unsigned char **data, const unsigned char *end)
{
    const unsigned char *p;
    pchtml_utils_warc_field_t *field = pchtml_utils_warc_field_last(warc);

    for (p = *data; p < end; p++) {
        if (*p == '"') {
            p++;

            *data = pchtml_str_append(&field->value, warc->mraw,
                                      *data, (p - *data));
            if (*data == NULL) {
                *data = p - 1;
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
            }

            *data = p;

            warc->state = PCHTML_UTILS_WARC_STATE_HEAD_FIELD_VALUE_QUOTED;

            return PCHTML_STATUS_OK;
        }

        if (*p == '\n') {
            p++;

            *data = pchtml_str_append(&field->value, warc->mraw,
                                      *data, (p - *data));
            if (*data == NULL) {
                *data = p - 1;
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
            }

            *data = p;

            if (field->value.length > 1) {
                if (field->value.data[(field->value.length - 2)] == '\r') {

                    pchtml_str_length_set(&field->value, warc->mraw,
                                          (field->value.length - 2));

                    warc->state = PCHTML_UTILS_WARC_STATE_HEAD_FIELD_VALUE_AFTER;

                    return PCHTML_STATUS_OK;
                }
            }

            p--;
        }
/*
        if (*p > 0x7F || pchtml_utils_warc_ctl[*p] != 0x00) {
            *data = p;

            warc->error = "Wrong header field value.";

            return PCHTML_STATUS_ABORTED;
        }
 */
    }

    p = pchtml_str_append(&field->value, warc->mraw, *data, (end - *data));
    if (p == NULL) {
        *data = end;
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    if (field->value.length > PCHTML_UTILS_WARC_MAX_HEADER_VALUE) {
        warc->error = "Too large header field name.";

        return PCHTML_STATUS_ABORTED;
    }

    *data = end;

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_utils_warc_parse_field_value_quoted(pchtml_utils_warc_t *warc,
                                 const unsigned char **data, const unsigned char *end)
{
    const unsigned char *p;
    pchtml_utils_warc_field_t *field = pchtml_utils_warc_field_last(warc);

    for (p = *data; p < end; p++) {
        if (*p == '"') {
            *data = p + 1;

            warc->state = PCHTML_UTILS_WARC_STATE_HEAD_FIELD_VALUE;

            goto done;
        }

        if (*p == '\\') {
            *data = pchtml_str_append(&field->value, warc->mraw,
                                      *data, (p - *data));
            if (*data == NULL) {
                *data = p;
                pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
                return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
            }

            *data = ++p;
        }
    }

done:

    p = pchtml_str_append(&field->value, warc->mraw, *data, (end - *data));
    if (p == NULL) {
        *data = end;
        pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
        return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
    }

    *data = end;

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_utils_warc_parse_field_value_after(pchtml_utils_warc_t *warc,
                                 const unsigned char **data, const unsigned char *end)
{
    UNUSED_PARAM(end);

    const pchtml_utils_warc_field_t *field = pchtml_utils_warc_field_last(warc);
    const unsigned char ch = **data;

    static const unsigned char pchtml_utils_warc_clen[] = "Content-Length";

    if (ch == ' ' || ch == '\t') {
        (*data)++;

        warc->state = PCHTML_UTILS_WARC_STATE_HEAD_FIELD_VALUE_WS;

        return PCHTML_STATUS_OK;
    }

    /* Parse Content-Length value */
    if (warc->content_length == 0
        && field->name.length == (sizeof(pchtml_utils_warc_clen) - 1)
        && pchtml_str_data_ncasecmp(field->name.data, pchtml_utils_warc_clen,
                                    (sizeof(pchtml_utils_warc_clen) - 1)))
    {
        const unsigned char *p = field->value.data;
        const unsigned char *p_end = p + field->value.length;

        warc->content_length = pchtml_conv_data_to_ulong(&p,
                                                         field->value.length);
        if (p != p_end) {
            warc->error = "Wrong \"Content-Length\" value.";

            return PCHTML_STATUS_ABORTED;
        }
    }

    if (ch == '\r') {
        (*data)++;

        warc->state = PCHTML_UTILS_WARC_STATE_HEAD_END;

        return PCHTML_STATUS_OK;
    }

    warc->state = PCHTML_UTILS_WARC_STATE_HEAD_FIELD_NAME;

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_utils_warc_parse_field_value_ws(pchtml_utils_warc_t *warc,
                                 const unsigned char **data, const unsigned char *end)
{
    const unsigned char *p;

    for (p = *data; p < end; p++) {
        if (*p != ' ' && *p != '\t') {
            *data = p;

            warc->state = PCHTML_UTILS_WARC_STATE_HEAD_FIELD_VALUE;

            return PCHTML_STATUS_OK;
        }
    }

    *data = p;

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_utils_warc_parse_header_end(pchtml_utils_warc_t *warc, const unsigned char **data,
                                const unsigned char *end)
{
    UNUSED_PARAM(end);

    unsigned int status;

    if (**data != '\n') {
        warc->error = "Wrong end of header.";

        return PCHTML_STATUS_ABORTED;
    }

    (*data)++;

    if (warc->header_cb != NULL) {
        status = warc->header_cb(warc);
        if (status != PCHTML_STATUS_OK) {
            if (status != PCHTML_STATUS_NEXT) {
                return status;
            }

            warc->skip = true;
        }
    }

    if (warc->content_length != 0) {
        warc->state = PCHTML_UTILS_WARC_STATE_BLOCK;
    }
    else {
        warc->state = PCHTML_UTILS_WARC_STATE_BLOCK_AFTER;
    }

    warc->content_read = 0;
    warc->ends = 0;
    warc->count++;

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_utils_warc_parse_block(pchtml_utils_warc_t *warc, const unsigned char **data,
                           const unsigned char *end)
{
    unsigned int status = PCHTML_STATUS_OK;

    if ((end - *data) >= (int)(warc->content_length - warc->content_read)) {
        end = *data + (warc->content_length - warc->content_read);

        if (warc->skip == false && warc->content_cb != NULL) {
            status = warc->content_cb(warc, *data, end);

            if (status != PCHTML_STATUS_OK) {
                warc->skip = true;
            }
        }

        warc->content_read = warc->content_length;
        warc->state = PCHTML_UTILS_WARC_STATE_BLOCK_AFTER;
        warc->ends = 0;

        *data = end;

        return status;
    }

    if (warc->skip == false && warc->content_cb != NULL) {
        status = warc->content_cb(warc, *data, end);

        if (status != PCHTML_STATUS_OK) {
            warc->skip = true;
        }
    }

    warc->content_read += end - *data;
    *data = end;

    return status;
}

static inline unsigned int
pchtml_utils_warc_parse_block_after(pchtml_utils_warc_t *warc,
                                 const unsigned char **data, const unsigned char *end)
{
    unsigned int status;
    static const unsigned char pchtml_utils_warc_ends[] = "\r\n\r\n";

    while (warc->ends < 4) {
        if (**data != pchtml_utils_warc_ends[warc->ends]) {
            warc->error = "Wrong end of block.";

            pcinst_set_error (PCHTML_ERROR);
            return PCHTML_STATUS_ERROR;
        }

        warc->ends++;

        if (++(*data) == end) {
            return PCHTML_STATUS_OK;
        }
    }

    if (warc->skip == false && warc->content_end_cb != NULL) {
        status = warc->content_end_cb(warc);
        if (status != PCHTML_STATUS_OK) {
            return status;
        }
    }

    return pchtml_utils_warc_clear(warc);
}

unsigned int
pchtml_utils_warc_parse(pchtml_utils_warc_t *warc,
                     const unsigned char **data, const unsigned char *end)
{
    unsigned int status = PCHTML_STATUS_ERROR;

    while (*data < end) {
        switch (warc->state) {

            case PCHTML_UTILS_WARC_STATE_HEAD_VERSION:
                status = pchtml_utils_warc_parse_version(warc, data, end);
                break;

            case PCHTML_UTILS_WARC_STATE_HEAD_VERSION_AFTER:
                status = pchtml_utils_warc_parse_field_version_after(warc, data, end);
                break;

            case PCHTML_UTILS_WARC_STATE_HEAD_FIELD_NAME:
                status = pchtml_utils_warc_parse_field_name(warc, data, end);
                break;

            case PCHTML_UTILS_WARC_STATE_HEAD_FIELD_VALUE:
                status = pchtml_utils_warc_parse_field_value(warc, data, end);
                break;

            case PCHTML_UTILS_WARC_STATE_HEAD_FIELD_VALUE_QUOTED:
                status = pchtml_utils_warc_parse_field_value_quoted(warc, data, end);
                break;

            case PCHTML_UTILS_WARC_STATE_HEAD_FIELD_VALUE_AFTER:
                status = pchtml_utils_warc_parse_field_value_after(warc, data, end);
                break;

            case PCHTML_UTILS_WARC_STATE_HEAD_FIELD_VALUE_WS:
                status = pchtml_utils_warc_parse_field_value_ws(warc, data, end);
                break;

            case PCHTML_UTILS_WARC_STATE_HEAD_END:
                status = pchtml_utils_warc_parse_header_end(warc, data, end);
                break;

            case PCHTML_UTILS_WARC_STATE_BLOCK:
                status = pchtml_utils_warc_parse_block(warc, data, end);
                break;

            case PCHTML_UTILS_WARC_STATE_BLOCK_AFTER:
                status = pchtml_utils_warc_parse_block_after(warc, data, end);
                break;
        }

        if (status != PCHTML_STATUS_OK) {
            return status;
        }
    }

    return PCHTML_STATUS_OK;
}

pchtml_utils_warc_field_t *
pchtml_utils_warc_header_field(pchtml_utils_warc_t *warc, const unsigned char *name,
                            size_t len, size_t offset)
{
    pchtml_utils_warc_field_t *field;

    for (size_t i = 0; i < pchtml_array_obj_length(warc->fields); i++) {
        field = pchtml_array_obj_get(warc->fields, i);

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
pchtml_utils_warc_header_serialize(pchtml_utils_warc_t *warc, pchtml_str_t *str)
{
    unsigned char *data;
    const pchtml_utils_warc_field_t *field;

    if (str->data == NULL) {
        pchtml_str_init(str, warc->mraw, 256);
        if (str->data == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    for (size_t i = 0; i < pchtml_array_obj_length(warc->fields); i++) {
        field = pchtml_array_obj_get(warc->fields, i);

        data = pchtml_str_append(str, warc->mraw, field->name.data,
                                 field->name.length);
        if (data == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }

        data = pchtml_str_append(str, warc->mraw, (unsigned char *) ": ", 2);
        if (data == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }

        data = pchtml_str_append(str, warc->mraw, field->value.data,
                                 field->value.length);
        if (data == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }

        data = pchtml_str_append_one(str, warc->mraw, '\n');
        if (data == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PCHTML_STATUS_ERROR_MEMORY_ALLOCATION;
        }
    }

    return PCHTML_STATUS_OK;
}

/*
 * No inline functions for ABI.
 */
size_t
pchtml_utils_warc_content_length_noi(pchtml_utils_warc_t *warc)
{
    return pchtml_utils_warc_content_length(warc);
}
