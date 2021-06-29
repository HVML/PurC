/*
** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
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
** Author: Vincent Wei <https://github.com/VincentWei>
** Author: lex.borisov@gmail.com (Alexander Borisov)
*/

#ifndef PCAT2_MyCORE_H
#define PCAT2_MyCORE_H

#pragma once

/**
 * @file myhtml.h
 *
 * Fast C/C++ HTML 5 Parser. Using threads.
 * With possibility of a Single Mode.
 * 
 * Complies with specification on <https://html.spec.whatwg.org/>.
 *
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "pcat2_version.h"
#include "pcat2_macros.h"
#include "mycore/myosi.h"
#include "mycore/incoming.h"
#include "mycore/mystring.h"
#include "mycore/charef.h"
#include "mycore/utils/mchar_async.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************
 *
 * MyCORE_STRING
 *
 ***********************************************************************************/

/**
 * Init mycore_string_t structure
 *
 * @param[in] mchar_async_t*. It can be obtained from myhtml_tree_t object
 *  (see myhtml_tree_get_mchar function) or create manualy
 *  For each Tree creates its object, I recommend to use it (myhtml_tree_get_mchar).
 *
 * @param[in] node_id. For all threads (and Main thread) identifier that is unique.
 *  if created mchar_async_t object manually you know it, if not then take from the Tree 
 *  (see myhtml_tree_get_mchar_node_id)
 *
 * @param[in] mycore_string_t*. It can be obtained from myhtml_tree_node_t object
 *  (see myhtml_node_string function) or create manualy
 *
 * @param[in] data size. Set the size you want for char*
 *
 * @return char* of the size if successful, otherwise a NULL value
 */
char*
mycore_string_init(mchar_async_t *mchar, size_t node_id,
                   mycore_string_t* str, size_t size);

/**
 * Increase the current size for mycore_string_t object
 *
 * @param[in] mycore_string_t*. See description for mycore_string_init function
 * @param[in] data size. Set the new size you want for mycore_string_t object
 *
 * @return char* of the size if successful, otherwise a NULL value
 */
char*
mycore_string_realloc(mycore_string_t *str, size_t new_size);

/**
 * Clean mycore_string_t object. In reality, data length set to 0
 * Equivalently: mycore_string_length_set(str, 0);
 *
 * @param[in] mycore_string_t*. See description for mycore_string_init function
 */
void
mycore_string_clean(mycore_string_t* str);

/**
 * Clean mycore_string_t object. Equivalently: memset(str, 0, sizeof(mycore_string_t))
 *
 * @param[in] mycore_string_t*. See description for mycore_string_init function
 */
void
mycore_string_clean_all(mycore_string_t* str);

/**
 * Release all resources for mycore_string_t object
 *
 * @param[in] mycore_string_t*. See description for mycore_string_init function
 * @param[in] call free function for current object or not
 *
 * @return NULL if destroy_obj set true, otherwise a current mycore_string_t object
 */
mycore_string_t*
mycore_string_destroy(mycore_string_t* str, bool destroy_obj);

/**
 * Get data (char*) from a mycore_string_t object
 *
 * @param[in] mycore_string_t*. See description for mycore_string_init function
 *
 * @return char* if exists, otherwise a NULL value
 */
char*
mycore_string_data(mycore_string_t *str);

/**
 * Get data length from a mycore_string_t object
 *
 * @param[in] mycore_string_t*. See description for mycore_string_init function
 *
 * @return data length
 */
size_t
mycore_string_length(mycore_string_t *str);

/**
 * Get data size from a mycore_string_t object
 *
 * @param[in] mycore_string_t*. See description for mycore_string_init function
 *
 * @return data size
 */
size_t
mycore_string_size(mycore_string_t *str);

/**
 * Set data (char *) for a mycore_string_t object.
 *
 * Attention!!! Attention!!! Attention!!!
 *
 * You can assign only that it has been allocated from functions:
 * mycore_string_data_alloc
 * mycore_string_data_realloc
 * or obtained manually created from mchar_async_t object
 *
 * Attention!!! Do not try set chat* from allocated by malloc or realloc!!!
 *
 * @param[in] mycore_string_t*. See description for mycore_string_init function
 * @param[in] you data to want assign
 *
 * @return assigned data if successful, otherwise a NULL value
 */
char*
mycore_string_data_set(mycore_string_t *str, char *data);

/**
 * Set data size for a mycore_string_t object.
 *
 * @param[in] mycore_string_t*. See description for mycore_string_init function
 * @param[in] you size to want assign
 *
 * @return assigned size
 */
size_t
mycore_string_size_set(mycore_string_t *str, size_t size);

/**
 * Set data length for a mycore_string_t object.
 *
 * @param[in] mycore_string_t*. See description for mycore_string_init function
 * @param[in] you length to want assign
 *
 * @return assigned length
 */
size_t
mycore_string_length_set(mycore_string_t *str, size_t length);

/**
 * Allocate data (char*) from a mchar_async_t object
 *
 * @param[in] mchar_async_t*. See description for mycore_string_init function
 * @param[in] node id. See description for mycore_string_init function
 * @param[in] you size to want assign
 *
 * @return data if successful, otherwise a NULL value
 */
char*
mycore_string_data_alloc(mchar_async_t *mchar, size_t node_id, size_t size);

/**
 * Allocate data (char*) from a mchar_async_t object
 *
 * @param[in] mchar_async_t*. See description for mycore_string_init function
 * @param[in] node id. See description for mycore_string_init function
 * @param[in] old data
 * @param[in] how much data is copied from the old data to new data
 * @param[in] new size
 *
 * @return data if successful, otherwise a NULL value
 */
char*
mycore_string_data_realloc(mchar_async_t *mchar, size_t node_id,
                           char *data,  size_t len_to_copy, size_t size);

/**
 * Release allocated data
 *
 * @param[in] mchar_async_t*. See description for mycore_string_init function
 * @param[in] node id. See description for mycore_string_init function
 * @param[in] data to release
 *
 * @return data if successful, otherwise a NULL value
 */
void
mycore_string_data_free(mchar_async_t *mchar, size_t node_id, char *data);

/***********************************************************************************
 *
 * MyCORE_STRING_RAW
 *
 * All work with mycore_string_raw_t object occurs through 
 *    mycore_malloc (standart malloc), mycore_realloc (standart realloc),
 *    mycore_free (standart free).
 * 
 * You are free to change them on without fear that something will happen
 * You can call free for str_raw.data, or change str_raw.length = 0
 *
 ***********************************************************************************/

/**
 * Clean mycore_string_raw_t object. In reality, data length set to 0
 *
 * @param[in] mycore_string_raw_t*
 */
void
mycore_string_raw_clean(mycore_string_raw_t* str_raw);

/**
 * Full clean mycore_string_raw_t object.
 * Equivalently: memset(str_raw, 0, sizeof(mycore_string_raw_t))
 *
 * @param[in] mycore_string_raw_t*
 */
void
mycore_string_raw_clean_all(mycore_string_raw_t* str_raw);

/**
 * Free resources for mycore_string_raw_t object
 *
 * @param[in] mycore_string_raw_t*
 * @param[in] call free function for current object or not
 *
 * @return NULL if destroy_obj set true, otherwise a current mycore_string_raw_t object
 */
mycore_string_raw_t*
mycore_string_raw_destroy(mycore_string_raw_t* str_raw, bool destroy_obj);


/***********************************************************************************
 *
 * MyCORE_INCOMING
 *
 * @description
 * For example, three buffer:
 *   1) Data: "bebebe";        Prev: 0; Next: 2; Size: 6;  Length: 6;  Offset: 0
 *   2) Data: "lalala-lululu"; Prev: 1; Next: 3; Size: 13; Length: 13; Offset: 6
 *   3) Data: "huy";           Prev: 2; Next: 0; Size: 3;  Length: 1;  Offset: 19
 *
 ***********************************************************************************/

/**
 * Get Incoming Buffer by position
 *
 * @param[in] current mycore_incoming_buffer_t*
 * @param[in] begin position
 *
 * @return mycore_incoming_buffer_t if successful, otherwise a NULL value
 */
mycore_incoming_buffer_t*
mycore_incoming_buffer_find_by_position(mycore_incoming_buffer_t *inc_buf, size_t begin);

/**
 * Get data of Incoming Buffer
 *
 * @param[in] mycore_incoming_buffer_t*
 *
 * @return const char* if successful, otherwise a NULL value
 */
const char*
mycore_incoming_buffer_data(mycore_incoming_buffer_t *inc_buf);

/**
 * Get data length of Incoming Buffer
 *
 * @param[in] mycore_incoming_buffer_t*
 *
 * @return size_t
 */
size_t
mycore_incoming_buffer_length(mycore_incoming_buffer_t *inc_buf);

/**
 * Get data size of Incoming Buffer
 *
 * @param[in] mycore_incoming_buffer_t*
 *
 * @return size_t
 */
size_t
mycore_incoming_buffer_size(mycore_incoming_buffer_t *inc_buf);

/**
 * Get data offset of Incoming Buffer. Global position of begin Incoming Buffer.
 * See description for MyCORE_INCOMING title
 *
 * @param[in] mycore_incoming_buffer_t*
 *
 * @return size_t
 */
size_t
mycore_incoming_buffer_offset(mycore_incoming_buffer_t *inc_buf);

/**
 * Get Relative Position for Incoming Buffer.
 * Incoming Buffer should be prepared by mycore_incoming_buffer_find_by_position
 *
 * @param[in] mycore_incoming_buffer_t*
 * @param[in] global begin
 *
 * @return size_t
 */
size_t
mycore_incoming_buffer_relative_begin(mycore_incoming_buffer_t *inc_buf, size_t begin);

/**
 * This function returns number of available data by Incoming Buffer
 * Incoming buffer may be incomplete. See mycore_incoming_buffer_next
 *
 * @param[in] mycore_incoming_buffer_t*
 * @param[in] global begin
 *
 * @return size_t
 */
size_t
mycore_incoming_buffer_available_length(mycore_incoming_buffer_t *inc_buf,
                                        size_t relative_begin, size_t length);

/**
 * Get next buffer
 *
 * @param[in] mycore_incoming_buffer_t*
 *
 * @return mycore_incoming_buffer_t*
 */
mycore_incoming_buffer_t*
mycore_incoming_buffer_next(mycore_incoming_buffer_t *inc_buf);

/**
 * Get prev buffer
 *
 * @param[in] mycore_incoming_buffer_t*
 *
 * @return mycore_incoming_buffer_t*
 */
mycore_incoming_buffer_t*
mycore_incoming_buffer_prev(mycore_incoming_buffer_t *inc_buf);

/***********************************************************************************
 *
 * MyCORE_UTILS
 *
 ***********************************************************************************/

/**
 * Compare two strings ignoring case
 *
 * @param[in] myhtml_collection_t*
 * @param[in] count of add nodes
 *
 * @return 0 if match, otherwise index of break position
 */
size_t
mycore_strcasecmp(const char* str1, const char* str2);

/**
 * Compare two strings ignoring case of the first n characters
 *
 * @param[in] myhtml_collection_t*
 * @param[in] count of add nodes
 *
 * @return 0 if match, otherwise index of break position
 */
size_t
mycore_strncasecmp(const char* str1, const char* str2, size_t size);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PCAT2_MyCORE_H */

