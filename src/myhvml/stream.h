/*
** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of Purring Cat 2, a HVML parser and interpreter.
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
**
** Author: Vincent Wei <https://github.com/VincentWei>
*/

#ifndef MyHVML_STREAM_H
#define MyHVML_STREAM_H

#pragma once

#include "myosi.h"
#include "myhvml_internals.h"

struct myhvml_stream_buffer_entry {
    char* data;
    size_t length;
    size_t size;
};

struct myhvml_stream_buffer {
    myhvml_stream_buffer_entry_t* entries;
    size_t length;
    size_t size;
    
    myencoding_result_t res;
};

#ifdef __cplusplus
extern "C" {
#endif
    
myhvml_stream_buffer_t * myhvml_stream_buffer_create(void);
mystatus_t myhvml_stream_buffer_init(myhvml_stream_buffer_t* stream_buffer, size_t entries_size);
void myhvml_stream_buffer_clean(myhvml_stream_buffer_t* stream_buffer);
myhvml_stream_buffer_t * myhvml_stream_buffer_destroy(myhvml_stream_buffer_t* stream_buffer, bool self_destroy);
myhvml_stream_buffer_entry_t * myhvml_stream_buffer_add_entry(myhvml_stream_buffer_t* stream_buffer, size_t entry_data_size);
myhvml_stream_buffer_entry_t * myhvml_stream_buffer_current_entry(myhvml_stream_buffer_t* stream_buffer);

mystatus_t myhvml_stream_buffer_entry_init(myhvml_stream_buffer_entry_t* stream_buffer_entry, size_t size);
void myhvml_stream_buffer_entry_clean(myhvml_stream_buffer_entry_t* stream_buffer_entry);
myhvml_stream_buffer_entry_t * myhvml_stream_buffer_entry_destroy(myhvml_stream_buffer_entry_t* stream_buffer_entry, bool self_destroy);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MyHVML_STREAM_H */
