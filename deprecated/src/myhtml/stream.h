/*
** Copyright (C) 2015-2017 Alexander Borisov
**
** This file is a part of PurC (short for Purring Cat), an HVML parser
** and interpreter.
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
** Author: lex.borisov@gmail.com (Alexander Borisov)
*/

#ifndef MyHTML_STREAM_H
#define MyHTML_STREAM_H
#pragma once

#ifdef __cplusplus
extern "C" {
#endif
    
#include "myosi.h"
#include "myhtml_internals.h"


struct myhtml_stream_buffer_entry {
    char* data;
    size_t length;
    size_t size;
};

struct myhtml_stream_buffer {
    myhtml_stream_buffer_entry_t* entries;
    size_t length;
    size_t size;
    
    myencoding_result_t res;
};

myhtml_stream_buffer_t * myhtml_stream_buffer_create(void);
mystatus_t myhtml_stream_buffer_init(myhtml_stream_buffer_t* stream_buffer, size_t entries_size);
void myhtml_stream_buffer_clean(myhtml_stream_buffer_t* stream_buffer);
myhtml_stream_buffer_t * myhtml_stream_buffer_destroy(myhtml_stream_buffer_t* stream_buffer, bool self_destroy);
myhtml_stream_buffer_entry_t * myhtml_stream_buffer_add_entry(myhtml_stream_buffer_t* stream_buffer, size_t entry_data_size);
myhtml_stream_buffer_entry_t * myhtml_stream_buffer_current_entry(myhtml_stream_buffer_t* stream_buffer);

mystatus_t myhtml_stream_buffer_entry_init(myhtml_stream_buffer_entry_t* stream_buffer_entry, size_t size);
void myhtml_stream_buffer_entry_clean(myhtml_stream_buffer_entry_t* stream_buffer_entry);
myhtml_stream_buffer_entry_t * myhtml_stream_buffer_entry_destroy(myhtml_stream_buffer_entry_t* stream_buffer_entry, bool self_destroy);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MyHTML_STREAM_H */
