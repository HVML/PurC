/**
 * @file encoding.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for text encoding.
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


#ifndef PCHTML_ENCODING_ENCODING_H
#define PCHTML_ENCODING_ENCODING_H

#ifdef __cplusplus
extern "C" {
#endif


#include "config.h"
#include "html/encoding/base.h"
#include "html/encoding/res.h"
#include "html/encoding/encode.h"
#include "html/encoding/decode.h"

#include "html/core/shs.h"


/*
 * Before searching will be removed any leading and trailing
 * ASCII whitespace in name.
 */
const pchtml_encoding_data_t *
pchtml_encoding_data_by_pre_name(const unsigned char *name, 
                size_t length) WTF_INTERNAL;


/*
 * Inline functions
 */

/*
 * Encode
 */
static inline unsigned int
pchtml_encoding_encode_init(pchtml_encoding_encode_t *encode,
                         const pchtml_encoding_data_t *encoding_data,
                         unsigned char *buffer_out, size_t buffer_length)
{
    if (encoding_data == NULL) {
        return PCHTML_STATUS_ERROR_WRONG_ARGS;
    }

    memset(encode, 0, sizeof(pchtml_encoding_encode_t));

    encode->buffer_out = buffer_out;
    encode->buffer_length = buffer_length;
    encode->encoding_data = encoding_data;

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_encoding_encode_finish(pchtml_encoding_encode_t *encode)
{
    if (encode->encoding_data->encoding == PCHTML_ENCODING_ISO_2022_JP) {
        return pchtml_encoding_encode_iso_2022_jp_eof(encode);
    }

    return PCHTML_STATUS_OK;
}

static inline unsigned char *
pchtml_encoding_encode_buf(pchtml_encoding_encode_t *encode)
{
    return encode->buffer_out;
}

static inline void
pchtml_encoding_encode_buf_set(pchtml_encoding_encode_t *encode,
                            unsigned char *buffer_out, size_t buffer_length)
{
    encode->buffer_out = buffer_out;
    encode->buffer_length = buffer_length;
    encode->buffer_used = 0;
}

static inline void
pchtml_encoding_encode_buf_used_set(pchtml_encoding_encode_t *encode,
                                 size_t buffer_used)
{
    encode->buffer_used = buffer_used;
}

static inline size_t
pchtml_encoding_encode_buf_used(pchtml_encoding_encode_t *encode)
{
    return encode->buffer_used;
}

static inline unsigned int
pchtml_encoding_encode_replace_set(pchtml_encoding_encode_t *encode,
                                const unsigned char *replace, size_t length)
{
    if (encode->buffer_out == NULL || encode->buffer_length < length) {
        return PCHTML_STATUS_SMALL_BUFFER;
    }

    encode->replace_to = replace;
    encode->replace_len = length;

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_encoding_encode_buf_add_to(pchtml_encoding_encode_t *encode,
                               unsigned char *data, size_t length)
{
    if ((encode->buffer_used + length) > encode->buffer_length) {
        return PCHTML_STATUS_SMALL_BUFFER;
    }

    memcpy(&encode->buffer_out[encode->buffer_used], data, length);

    encode->buffer_used += length;

    return PCHTML_STATUS_OK;
}

/*
 * Decode
 */
static inline unsigned int
pchtml_encoding_decode_buf_add_to(pchtml_encoding_decode_t *decode,
                               const uint32_t *data, size_t length)
{
    if ((decode->buffer_used + length) > decode->buffer_length) {
        return PCHTML_STATUS_SMALL_BUFFER;
    }

    memcpy(&decode->buffer_out[decode->buffer_used], data,
           sizeof(uint32_t) * length);

    decode->buffer_used += length;

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_encoding_decode_init(pchtml_encoding_decode_t *decode,
                         const pchtml_encoding_data_t *encoding_data,
                         uint32_t *buffer_out, size_t buffer_length)
{
    if (encoding_data == NULL) {
        return PCHTML_STATUS_ERROR_WRONG_ARGS;
    }

    memset(decode, 0, sizeof(pchtml_encoding_decode_t));

    decode->buffer_out = buffer_out;
    decode->buffer_length = buffer_length;
    decode->encoding_data = encoding_data;

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_encoding_decode_finish(pchtml_encoding_decode_t *decode)
{
    unsigned int status;

    if (decode->status != PCHTML_STATUS_OK) {

        if (decode->encoding_data->encoding == PCHTML_ENCODING_ISO_2022_JP
            && decode->u.iso_2022_jp.state == PCHTML_ENCODING_DECODE_2022_JP_ASCII)
        {
            return PCHTML_STATUS_OK;
        }

        if (decode->replace_to == NULL) {
            return PCHTML_STATUS_ERROR;
        }

        status = pchtml_encoding_decode_buf_add_to(decode, decode->replace_to,
                                                decode->replace_len);
        if (status == PCHTML_STATUS_SMALL_BUFFER) {
            return status;
        }
    }

    return PCHTML_STATUS_OK;
}

static inline uint32_t *
pchtml_encoding_decode_buf(pchtml_encoding_decode_t *decode)
{
    return decode->buffer_out;
}

static inline void
pchtml_encoding_decode_buf_set(pchtml_encoding_decode_t *decode,
                            uint32_t *buffer_out, size_t buffer_length)
{
    decode->buffer_out = buffer_out;
    decode->buffer_length = buffer_length;
    decode->buffer_used = 0;
}

static inline void
pchtml_encoding_decode_buf_used_set(pchtml_encoding_decode_t *decode,
                                 size_t buffer_used)
{
    decode->buffer_used = buffer_used;
}

static inline size_t
pchtml_encoding_decode_buf_used(pchtml_encoding_decode_t *decode)
{
    return decode->buffer_used;
}

static inline unsigned int
pchtml_encoding_decode_replace_set(pchtml_encoding_decode_t *decode,
                                const uint32_t *replace, size_t length)
{
    if (decode->buffer_out == NULL || decode->buffer_length < length) {
        return PCHTML_STATUS_SMALL_BUFFER;
    }

    decode->replace_to = replace;
    decode->replace_len = length;

    return PCHTML_STATUS_OK;
}

/*
 * Single encode.
 */
static inline unsigned int
pchtml_encoding_encode_init_single(pchtml_encoding_encode_t *encode,
                                const pchtml_encoding_data_t *encoding_data)
{
    if (encoding_data == NULL) {
        return PCHTML_STATUS_ERROR_WRONG_ARGS;
    }

    memset(encode, 0, sizeof(pchtml_encoding_encode_t));

    encode->encoding_data = encoding_data;

    return PCHTML_STATUS_OK;
}

static inline int8_t
pchtml_encoding_encode_finish_single(pchtml_encoding_encode_t *encode,
                                  unsigned char **data, const unsigned char *end)
{
    if (encode->encoding_data->encoding == PCHTML_ENCODING_ISO_2022_JP) {
        return pchtml_encoding_encode_iso_2022_jp_eof_single(encode, data, end);
    }

    return 0;
}

/*
 * Single decode.
 */
static inline unsigned int
pchtml_encoding_decode_init_single(pchtml_encoding_decode_t *decode,
                                const pchtml_encoding_data_t *encoding_data)
{
    if (encoding_data == NULL) {
        return PCHTML_STATUS_ERROR_WRONG_ARGS;
    }

    memset(decode, 0, sizeof(pchtml_encoding_decode_t));

    decode->encoding_data = encoding_data;

    return PCHTML_STATUS_OK;
}

static inline unsigned int
pchtml_encoding_decode_finish_single(pchtml_encoding_decode_t *decode)
{
    if (decode->status != PCHTML_STATUS_OK) {

        if (decode->encoding_data->encoding == PCHTML_ENCODING_ISO_2022_JP
            && decode->u.iso_2022_jp.state == PCHTML_ENCODING_DECODE_2022_JP_ASCII)
        {
            return PCHTML_STATUS_OK;
        }

        return PCHTML_STATUS_ERROR;
    }

    return PCHTML_STATUS_OK;
}

/*
 * Encoding data.
 */
static inline const pchtml_encoding_data_t *
pchtml_encoding_data_by_name(const unsigned char *name, size_t length)
{
    const pchtml_shs_entry_t *entry;

    if (length == 0) {
        return NULL;
    }

    entry = pchtml_shs_entry_get_lower_static(pchtml_encoding_res_shs_entities,
                                              name, length);
    if (entry == NULL) {
        return NULL;
    }

    return (const pchtml_encoding_data_t *) entry->value;
}

static inline const pchtml_encoding_data_t *
pchtml_encoding_data(pchtml_encoding_t encoding)
{
    if (encoding >= PCHTML_ENCODING_LAST_ENTRY) {
        return NULL;
    }

    return &pchtml_encoding_res_map[encoding];
}

static inline pchtml_encoding_encode_f
pchtml_encoding_encode_function(pchtml_encoding_t encoding)
{
    if (encoding >= PCHTML_ENCODING_LAST_ENTRY) {
        return NULL;
    }

    return pchtml_encoding_res_map[encoding].encode;
}

static inline pchtml_encoding_decode_f
pchtml_encoding_decode_function(pchtml_encoding_t encoding)
{
    if (encoding >= PCHTML_ENCODING_LAST_ENTRY) {
        return NULL;
    }

    return pchtml_encoding_res_map[encoding].decode;
}

static inline unsigned int
pchtml_encoding_data_call_encode(pchtml_encoding_data_t *encoding_data, pchtml_encoding_encode_t *ctx,
                              const uint32_t **cp, const uint32_t *end)
{
    return encoding_data->encode(ctx, cp, end);
}

static inline unsigned int
pchtml_encoding_data_call_decode(pchtml_encoding_data_t *encoding_data, pchtml_encoding_decode_t *ctx,
                              const unsigned char **data, const unsigned char *end)
{
    return encoding_data->decode(ctx, data, end);
}

static inline pchtml_encoding_t
pchtml_encoding_data_encoding(pchtml_encoding_data_t *data)
{
    return data->encoding;
}

/*
 * No inline functions for ABI.
 */
unsigned int
pchtml_encoding_encode_init_noi(pchtml_encoding_encode_t *encode,
            const pchtml_encoding_data_t *encoding_data,
            unsigned char *buffer_out, size_t buffer_length) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_finish_noi(
            pchtml_encoding_encode_t *encode) WTF_INTERNAL;

unsigned char *
pchtml_encoding_encode_buf_noi(pchtml_encoding_encode_t *encode) WTF_INTERNAL;

void
pchtml_encoding_encode_buf_set_noi(pchtml_encoding_encode_t *encode,
            unsigned char *buffer_out, size_t buffer_length) WTF_INTERNAL;

void
pchtml_encoding_encode_buf_used_set_noi(pchtml_encoding_encode_t *encode,
            size_t buffer_used) WTF_INTERNAL;

size_t
pchtml_encoding_encode_buf_used_noi(
            pchtml_encoding_encode_t *encode) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_replace_set_noi(pchtml_encoding_encode_t *encode,
            const unsigned char *replace, size_t buffer_length) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_buf_add_to_noi(pchtml_encoding_encode_t *encode,
            unsigned char *data, size_t length) WTF_INTERNAL;

unsigned int
pchtml_encoding_decode_init_noi(pchtml_encoding_decode_t *decode,
            const pchtml_encoding_data_t *encoding_data,
            uint32_t *buffer_out, size_t buffer_length) WTF_INTERNAL;

unsigned int
pchtml_encoding_decode_finish_noi(
            pchtml_encoding_decode_t *decode) WTF_INTERNAL;

uint32_t *
pchtml_encoding_decode_buf_noi(pchtml_encoding_decode_t *decode) WTF_INTERNAL;

void
pchtml_encoding_decode_buf_set_noi(pchtml_encoding_decode_t *decode,
            uint32_t *buffer_out, size_t buffer_length) WTF_INTERNAL;

void
pchtml_encoding_decode_buf_used_set_noi(pchtml_encoding_decode_t *decode,
            size_t buffer_used) WTF_INTERNAL;

size_t
pchtml_encoding_decode_buf_used_noi(
            pchtml_encoding_decode_t *decode) WTF_INTERNAL;

unsigned int
pchtml_encoding_decode_replace_set_noi(pchtml_encoding_decode_t *decode,
            const uint32_t *replace, size_t length) WTF_INTERNAL;

unsigned int
pchtml_encoding_decode_buf_add_to_noi(pchtml_encoding_decode_t *decode,
            const uint32_t *data, size_t length) WTF_INTERNAL;

unsigned int
pchtml_encoding_encode_init_single_noi(pchtml_encoding_encode_t *encode,
            const pchtml_encoding_data_t *encoding_data) WTF_INTERNAL;

int8_t
pchtml_encoding_encode_finish_single_noi(pchtml_encoding_encode_t *encode,
            unsigned char **data, const unsigned char *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_decode_init_single_noi(pchtml_encoding_decode_t *decode,
            const pchtml_encoding_data_t *encoding_data) WTF_INTERNAL;

unsigned int
pchtml_encoding_decode_finish_single_noi(
            pchtml_encoding_decode_t *decode) WTF_INTERNAL;

const pchtml_encoding_data_t *
pchtml_encoding_data_by_name_noi(const unsigned char *name, 
            size_t length) WTF_INTERNAL;

const pchtml_encoding_data_t *
pchtml_encoding_data_noi(pchtml_encoding_t encoding) WTF_INTERNAL;

pchtml_encoding_encode_f
pchtml_encoding_encode_function_noi(pchtml_encoding_t encoding) WTF_INTERNAL;

pchtml_encoding_decode_f
pchtml_encoding_decode_function_noi(pchtml_encoding_t encoding) WTF_INTERNAL;

unsigned int
pchtml_encoding_data_call_encode_noi(pchtml_encoding_data_t *encoding_data, 
            pchtml_encoding_encode_t *ctx,
            const uint32_t **cp, const uint32_t *end) WTF_INTERNAL;

unsigned int
pchtml_encoding_data_call_decode_noi(pchtml_encoding_data_t *encoding_data, 
            pchtml_encoding_decode_t *ctx,
            const unsigned char **data, const unsigned char *end) WTF_INTERNAL;

pchtml_encoding_t
pchtml_encoding_data_encoding_noi(pchtml_encoding_data_t *data) WTF_INTERNAL;

size_t
pchtml_encoding_encode_t_sizeof(void) WTF_INTERNAL;

size_t
pchtml_encoding_decode_t_sizeof(void) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_ENCODING_ENCODING_H */
