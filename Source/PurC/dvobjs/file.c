/*
 * @file dvobjs.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of public part for html parser.
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
 *
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/edom.h"
#include "private/html.h"

#include "purc-variant.h"
#include "tools.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static const char* get_work_dirctory (void)
{
    return "/home/gengyue";
}

static ssize_t find_line (purc_rwstream_t rw, int line_num, ssize_t length)
{
    size_t pos = 0;
    int i = 0;
    unsigned char buffer[1024];
    ssize_t read_size = 0;
    size_t length = 0;
    const char* head = NULL;

    if (line_num > 0) {
        purc_rwstream_seek (rw, 0, SEEK_SET);

        while (line_num) {
            read_size = purc_rwstream_read (rw, buffer, 1024);
            if (read_size < 0) 
                break;

            head = get_next_option (buffer, "\n", &length);
            while (head) {
                pos += length + 1;          // to be checked
                line_num --;

                if (line_num == 0)
                    break;

                head = get_next_option (head + length, "\n", &length);
            }
            if (read_size < 1024)           // to the end
                break;

            if (line_num == 0)
                break;
        }
    }
    else {
        line_num = -1 * line_num;

        if (length <= 1024)
            purc_rwstream_seek (rw, 0, SEEK_SET);
        else
            purc_rwstream_seek (rw, length - i * 1024, SEEK_SET);

        while (line_num) {
            read_size = purc_rwstream_read (rw, buffer, 1024);
            if (read_size < 0) 
                break;

            head = get_prev_option (buffer, read_size, "\n", &length);
            while (head) {
                pos += length + 1;          // to be checked
                line_num --;

                if (line_num == 0)
                    break;

                head = get_next_option (head + length, "\n", &length);
            }
            if (read_size < 1024)           // to the end
                break;

            if (line_num == 0)
                break;
        }
    }

    return pos;
}

static purc_variant_t
file_text_head (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    
    int64_t line_num = 0;
    char filename[MAX_PATH] = {0,};
    const char* string_filename = NULL;
    off_t pos = 0;

    purc_rwstream_t rw_file = NULL;
    purc_rwstream_t rw_buffer = NULL;
    purc_variant_t ret_var = NULL;

    if ((argv == NULL) || (nr_args == 0)) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != NULL) && (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (*string_filename == '/') {
        strcpy (filename, string_filename);
    }
    else {
        strcpy (filename, get_work_dirctory ());
        strcat (filename, '/');
        strcat (filename, string_filename);
    }

#if 0
    // check whether the file exists
    if((access(filename, F_OK | R_OK)) != 0) {
        pcinst_set_error (PURC_ERROR_NOT_EXISTS);
        return PURC_VARIANT_INVALID;
    }

    // get the file length
    if(stat(filename, &filestat) < 0) {
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }
    if (filestat.st_size == 0) {
        return purc_variant_make_string ("", false); 
    }
#endif

    if (argv[1] != NULL) 
        purc_variant_cast_to_longint (argv[1], &line_num, false);

    rw_file = purc_rwstream_new_from_file (filename, "r");
    if (rw_file == NULL) {
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    pos = purc_rwstream_seek (rw_file, 0, SEEK_END);

    pos = find_line (rw_file, line_num, pos);

    return ret_var;
}

static purc_variant_t
file_text_tail (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

static purc_variant_t
file_bin_head (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}


static purc_variant_t
file_bin_tail (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    return NULL;
}

// only for test now.
purc_variant_t pcdvojbs_get_file (void)
{
    purc_variant_t file = purc_variant_make_object_c (4,
            "file_text_head",  purc_variant_make_dynamic (file_text_head, NULL),
            "file_text_tail",  purc_variant_make_dynamic (file_text_tail, NULL),
            "file_bin_head",   purc_variant_make_dynamic (file_bin_head, NULL),
            "file_bin_tail",   purc_variant_make_dynamic (file_bin_tail, NULL)
       );
    return file;
}
