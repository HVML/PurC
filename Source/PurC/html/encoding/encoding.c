/**
 * @file encoding.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of encoding.
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


#include "html/encoding/encoding.h"


const pchtml_encoding_data_t *
pchtml_encoding_data_by_pre_name(const unsigned char *name, size_t length)
{
    const unsigned char *end;
    const pchtml_shs_entry_t *entry;

    if (length == 0) {
        return NULL;
    }

    end = name + length;

    /* Remove any leading */
    do {
        switch (*name) {
            case 0x09: case 0x0A: case 0x0C: case 0x0D: case 0x20:
                name++;
                continue;
        }

        break;
    }
    while (name < end);

    /* Remove any trailing */
    while (name < end) {
        switch (*(end - 1)) {
            case 0x09: case 0x0A: case 0x0C: case 0x0D: case 0x20:
                end--;
                continue;
        }

        break;
    }

    if (name == end) {
        return NULL;
    }

    entry = pchtml_shs_entry_get_lower_static(pchtml_encoding_res_shs_entities,
                                              name, (end - name));
    if (entry == NULL) {
        return NULL;
    }

    return entry->value;
}

/*
 * No inline functions for ABI.
 */
unsigned int
pchtml_encoding_encode_init_noi(pchtml_encoding_encode_t *encode,
                             const pchtml_encoding_data_t *encoding_data,
                             unsigned char *buffer_out, size_t buffer_length)
{
    return pchtml_encoding_encode_init(encode, encoding_data,
                                    buffer_out, buffer_length);
}

unsigned int
pchtml_encoding_encode_finish_noi(pchtml_encoding_encode_t *encode)
{
    return pchtml_encoding_encode_finish(encode);
}

unsigned char *
pchtml_encoding_encode_buf_noi(pchtml_encoding_encode_t *encode)
{
    return pchtml_encoding_encode_buf(encode);
}

void
pchtml_encoding_encode_buf_set_noi(pchtml_encoding_encode_t *encode,
                                unsigned char *buffer_out, size_t buffer_length)
{
    pchtml_encoding_encode_buf_set(encode, buffer_out, buffer_length);
}

void
pchtml_encoding_encode_buf_used_set_noi(pchtml_encoding_encode_t *encode,
                                     size_t buffer_used)
{
    pchtml_encoding_encode_buf_used_set(encode, buffer_used);
}

size_t
pchtml_encoding_encode_buf_used_noi(pchtml_encoding_encode_t *encode)
{
    return pchtml_encoding_encode_buf_used(encode);
}

unsigned int
pchtml_encoding_encode_replace_set_noi(pchtml_encoding_encode_t *encode,
                                    const unsigned char *replace, size_t length)
{
    return pchtml_encoding_encode_replace_set(encode, replace, length);
}

unsigned int
pchtml_encoding_encode_buf_add_to_noi(pchtml_encoding_encode_t *encode,
                                   unsigned char *data, size_t length)
{
    return pchtml_encoding_encode_buf_add_to(encode, data, length);
}

unsigned int
pchtml_encoding_decode_init_noi(pchtml_encoding_decode_t *decode,
                             const pchtml_encoding_data_t *encoding_data,
                             uint32_t *buffer_out, size_t buffer_length)
{
    return pchtml_encoding_decode_init(decode, encoding_data,
                                    buffer_out, buffer_length);
}

unsigned int
pchtml_encoding_decode_finish_noi(pchtml_encoding_decode_t *decode)
{
    return pchtml_encoding_decode_finish(decode);
}

uint32_t *
pchtml_encoding_decode_buf_noi(pchtml_encoding_decode_t *decode)
{
    return pchtml_encoding_decode_buf(decode);
}

void
pchtml_encoding_decode_buf_set_noi(pchtml_encoding_decode_t *decode,
                              uint32_t *buffer_out, size_t buffer_length)
{
    pchtml_encoding_decode_buf_set(decode, buffer_out, buffer_length);
}

void
pchtml_encoding_decode_buf_used_set_noi(pchtml_encoding_decode_t *decode,
                                     size_t buffer_used)
{
    pchtml_encoding_decode_buf_used_set(decode, buffer_used);
}

size_t
pchtml_encoding_decode_buf_used_noi(pchtml_encoding_decode_t *decode)
{
    return pchtml_encoding_decode_buf_used(decode);
}

unsigned int
pchtml_encoding_decode_replace_set_noi(pchtml_encoding_decode_t *decode,
                                  const uint32_t *replace, size_t length)
{
    return pchtml_encoding_decode_replace_set(decode, replace, length);
}

unsigned int
pchtml_encoding_decode_buf_add_to_noi(pchtml_encoding_decode_t *decode,
                                   const uint32_t *data, size_t length)
{
    return pchtml_encoding_decode_buf_add_to(decode, data, length);
}

unsigned int
pchtml_encoding_encode_init_single_noi(pchtml_encoding_encode_t *encode,
                                    const pchtml_encoding_data_t *encoding_data)
{
    return pchtml_encoding_encode_init_single(encode, encoding_data);
}

int8_t
pchtml_encoding_encode_finish_single_noi(pchtml_encoding_encode_t *encode,
                                      unsigned char **data, const unsigned char *end)
{
    return pchtml_encoding_encode_finish_single(encode, data, end);
}

unsigned int
pchtml_encoding_decode_init_single_noi(pchtml_encoding_decode_t *decode,
                                    const pchtml_encoding_data_t *encoding_data)
{
    return pchtml_encoding_decode_init_single(decode, encoding_data);
}

unsigned int
pchtml_encoding_decode_finish_single_noi(pchtml_encoding_decode_t *decode)
{
    return pchtml_encoding_decode_finish_single(decode);
}

const pchtml_encoding_data_t *
pchtml_encoding_data_by_name_noi(const unsigned char *name, size_t length)
{
    return pchtml_encoding_data_by_name(name, length);
}

const pchtml_encoding_data_t *
pchtml_encoding_data_noi(pchtml_encoding_t encoding)
{
    return pchtml_encoding_data(encoding);
}

pchtml_encoding_encode_f
pchtml_encoding_encode_function_noi(pchtml_encoding_t encoding)
{
    return pchtml_encoding_encode_function(encoding);
}

pchtml_encoding_decode_f
pchtml_encoding_decode_function_noi(pchtml_encoding_t encoding)
{
    return pchtml_encoding_decode_function(encoding);
}

unsigned int
pchtml_encoding_data_call_encode_noi(pchtml_encoding_data_t *encoding_data, pchtml_encoding_encode_t *ctx,
                                  const uint32_t **cp, const uint32_t *end)
{
    return pchtml_encoding_data_call_encode(encoding_data, ctx, cp, end);
}

unsigned int
pchtml_encoding_data_call_decode_noi(pchtml_encoding_data_t *encoding_data, pchtml_encoding_decode_t *ctx,
                                  const unsigned char **data, const unsigned char *end)
{
    return pchtml_encoding_data_call_decode(encoding_data, ctx, data, end);
}

pchtml_encoding_t
pchtml_encoding_data_encoding_noi(pchtml_encoding_data_t *data)
{
    return pchtml_encoding_data_encoding(data);
}

size_t
pchtml_encoding_encode_t_sizeof(void)
{
    return sizeof(pchtml_encoding_encode_t);
}

size_t
pchtml_encoding_decode_t_sizeof(void)
{
    return sizeof(pchtml_encoding_decode_t);
}
