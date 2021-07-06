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

#ifndef MyCORE_CHAREF_H
#define MyCORE_CHAREF_H

#pragma once

#include <stddef.h>

struct charef_entry {
    unsigned char ch;
    size_t next;
    size_t cur_pos;
    size_t codepoints[2];
    size_t codepoints_len;
}
typedef charef_entry_t;

struct charef_entry_result {
    const charef_entry_t *curr_entry;
    const charef_entry_t *last_entry;
    size_t last_offset;
    int is_done;
}
typedef charef_entry_result_t;

#ifdef __cplusplus
extern "C" {
#endif
    
const charef_entry_t * mycore_charef_find(const char *begin, size_t *offset, size_t size, size_t *data_size);
const charef_entry_t * mycore_charef_find_by_pos(size_t pos, const char *begin, size_t *offset, size_t size, charef_entry_result_t *result);
const charef_entry_t * mycore_charef_get_first_position(const char begin);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MyCORE_CHAREF_H */
