/*
 * @file tempbuffer.h
 * @author XueShuming
 * @date 2021/08/27
 * @brief The interfaces for temp buffer.
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

#ifndef PURC_HVML_TEMPBUFFER_H
#define PURC_HVML_TEMPBUFFER_H

#include <stddef.h>
#include <stdint.h>

struct temp_buffer {
    uint8_t* base;
    uint8_t* here;
    uint8_t* stop;
    wchar_t last_wc;
    size_t sz_char;
};

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

struct temp_buffer* temp_buffer_new (size_t sz_init);

bool temp_buffer_is_empty (struct temp_buffer* buffer);

/*
 * Get the memory size of the character buffer
 */
size_t temp_buffer_get_memory_size (struct temp_buffer* buffer);

/*
 * Get the character size of the character buffer
 */
size_t temp_buffer_get_character_size (struct temp_buffer* buffer);

/*
 * Append character to the character buffer
 * bytes : utf8
 */
void temp_buffer_append (struct temp_buffer* buffer,
        const char* bytes, size_t nr_bytes, wchar_t wc);

const char* temp_buffer_get_buff (
        struct temp_buffer* buffer);

bool temp_buffer_is_end_with (struct temp_buffer* buffer,
        const char* bytes, size_t nr_bytes);

bool temp_buffer_is_equal (struct temp_buffer* buffer,
        const char* bytes, size_t nr_bytes);

wchar_t temp_buffer_get_last_wchar_t (struct temp_buffer* buffer);

void temp_buffer_reset (struct temp_buffer* buffer);

void temp_buffer_destroy (struct temp_buffer* buffer);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_HVML_TEMPBUFFER_H */

