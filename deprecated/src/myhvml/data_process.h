/*
** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
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
*/

#ifndef MyHVML_DATA_PROCESS_H
#define MyHVML_DATA_PROCESS_H

#pragma once

#include "myosi.h"
#include "mycore.h"
#include "mystring.h"

struct myhvml_data_process_entry {
    /* current state for process data */
    myhvml_data_process_state_f state;
    
    /* for encodings */
    myencoding_t encoding;
    myencoding_result_t res;
    
    /* temp */
    size_t tmp_str_pos_proc;
    size_t tmp_str_pos;
    size_t tmp_num;
    
    /* &lt; current result */
    charef_entry_result_t charef_res;
    
    /* settings */
    bool is_attributes;
    bool emit_null_char;
};

#ifdef __cplusplus
extern "C" {
#endif

void myhvml_data_process_entry_clean(myhvml_data_process_entry_t* proc_entry);

void myhvml_data_process(myhvml_data_process_entry_t* proc_entry, mycore_string_t* str, const char* data, size_t size);
void myhvml_data_process_end(myhvml_data_process_entry_t* proc_entry, mycore_string_t* str);

size_t myhvml_data_process_state_data(myhvml_data_process_entry_t* proc_entry, mycore_string_t* str, const char* data, size_t offset, size_t size);
size_t myhvml_data_process_state_ampersand(myhvml_data_process_entry_t* proc_entry, mycore_string_t* str, const char* data, size_t offset, size_t size);
size_t myhvml_data_process_state_ampersand_data(myhvml_data_process_entry_t* proc_entry, mycore_string_t* str, const char* data, size_t offset, size_t size);
size_t myhvml_data_process_state_ampersand_hash(myhvml_data_process_entry_t* proc_entry, mycore_string_t* str, const char* data, size_t offset, size_t size);
size_t myhvml_data_process_state_ampersand_hash_data(myhvml_data_process_entry_t* proc_entry, mycore_string_t* str, const char *data, size_t offset, size_t size);
size_t myhvml_data_process_state_ampersand_hash_x_data(myhvml_data_process_entry_t* proc_entry, mycore_string_t* str, const char* data, size_t offset, size_t size);
void   myhvml_data_process_state_end(myhvml_data_process_entry_t* proc_entry, mycore_string_t* str);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MyHVML_DATA_PROCESS_H */

