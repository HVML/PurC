/*
 * @file file.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of file dynamic variant object.
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

#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/edom.h"
#include "private/html.h"

#include "purc-variant.h"
#include "helper.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <linux/limits.h>
#include <stdio.h>

#define PURC_VARIANT_KEY    0xE2
#define PURC_VARIANT_VALUE  0xE3

static const char* get_work_dirctory (void)
{
    // todo: getcwd
    return "/home/gengyue";
}

static ssize_t find_line (FILE *fp, int line_num, ssize_t file_length)
{
    size_t pos = 0;
    int i = 0;
    unsigned char buffer[1024];
    ssize_t read_size = 0;
    size_t length = 0;
    const char* head = NULL;

    if (line_num > 0) {
        fseek (fp, 0L, SEEK_SET);

        while (line_num) {
            read_size = fread (buffer, 1, 1024, fp);
            if (read_size < 0) 
                break;

            head = pcdvobjs_file_get_next_option ((char *)buffer, "\n", &length);
            while (head) {
                pos += length + 1;          // to be checked
                line_num --;

                if (line_num == 0)
                    break;

                head = pcdvobjs_file_get_next_option (head + length + 1, "\n", &length);
            }
            if (read_size < 1024)           // to the end
                break;

            if (line_num == 0)
                break;
        }
    }
    else {
        line_num = -1 * line_num;
        file_length --;                     // the last is 0x0A
        pos = file_length;

        while (line_num) {
            if (file_length <= 1024)
                fseek (fp, 0L, SEEK_SET);
            else
                fseek (fp, file_length - (i + 1) * 1024, SEEK_SET);

            read_size = fread (buffer, 1, 1024, fp);
            if (read_size < 0) 
                break;

            head = pcdvobjs_file_get_prev_option ((char *)buffer, read_size, 
                                                                "\n", &length);
            while (head) {
                pos -= length;
                pos--;
                line_num --;

                if (line_num == 0)
                    break;

                read_size -= length;
                read_size--;
                head = pcdvobjs_file_get_prev_option ((char *)buffer, read_size, "\n",
                                                                    &length);
            }
            if (read_size < 1024)           // to the end
                break;

            if (line_num == 0)
                break;

            i ++;
            file_length -= 1024;
        }
    }

    return pos;
}

static char * find_line_in_stream (purc_rwstream_t stream, int line_num, 
                                                                size_t *position)
{
    size_t pos = 0;
    unsigned char buffer[1024];
    ssize_t read_size = 0;
    size_t length = 0;
    const char* head = NULL;
    char *content = NULL;
    char *temp = NULL;
    size_t buf_size = 4 * 1024;
    size_t buf_length = 0;

    content = malloc (buf_size);
    if (content == NULL)
        return content;

    while (line_num) {
        read_size = purc_rwstream_read (stream, buffer, 1024);
        if (read_size < 0) 
            break;
        
        if ((buf_length + read_size) > buf_size) {
            temp = content;
            buf_size += 4 * 1024;
            content = malloc (buf_size);
            if (content == NULL) {
                free (temp);
                return content;
            }
            else {
                memcpy (content, temp, buf_length);
                memcpy (content + buf_length, buffer, read_size);
                buf_length += read_size;
                free (temp);
            }
        }
        else {
            memcpy (content + buf_length, buffer, read_size);
            buf_length += read_size;
        }

        head = pcdvobjs_file_get_next_option ((char *)buffer, "\n", &length);
        while (head) {
            pos += length + 1;          // to be checked
            line_num --;

            if (line_num == 0)
                break;

            head = pcdvobjs_file_get_next_option (head + length + 1, "\n", &length);
        }
        if (read_size < 1024)           // to the end
            break;

        if (line_num == 0)
            break;
    }

    if (pos == 0) {
        if (content) {
            free (content);
            content = NULL;
        }
    }
    else
    {
        temp = content;
        content = malloc (pos + 1);
        if (content == NULL) {
            free (temp);
        }
        else {
            memcpy (content, temp, buf_length);
            free (temp);
            *(content + pos) = 0x00;
            *position = pos + 1;
        }
    }

    return content;
}

static purc_variant_t
text_head_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);
    
    int64_t line_num = 0;
    char filename[PATH_MAX] = {0,};
    const char* string_filename = NULL;
    FILE *fp = NULL;
    size_t pos = 0;
    struct stat filestat;
    purc_variant_t ret_var = NULL;

    if ((argv[0] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (*string_filename == '/') {
        strcpy (filename, string_filename);
    }
    else {
        strcpy (filename, get_work_dirctory ());
        strcat (filename, "/");
        strcat (filename, string_filename);
    }

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

    if (argv[1] != PURC_VARIANT_INVALID) 
        purc_variant_cast_to_longint (argv[1], &line_num, false);

    fp = fopen (filename, "r");
    if (fp == NULL) {
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    if (line_num == 0)
        pos = filestat.st_size;
    else
        pos = find_line (fp, line_num, filestat.st_size);

    char * content = malloc (pos + 1);
    if (content == NULL) {
        fclose (fp);
        return purc_variant_make_string ("", false);
    }

    fseek (fp, 0L, SEEK_SET);
    *(content + pos) = 0x00;
    pos = fread (content, 1, pos, fp);

    ret_var = purc_variant_make_string_reuse_buff (content, pos, false); 

    fclose (fp);

    return ret_var;
}

static purc_variant_t
text_tail_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    int64_t line_num = 0;
    char filename[PATH_MAX] = {0,};
    const char* string_filename = NULL;
    FILE *fp = NULL;
    size_t pos = 0;
    struct stat filestat;
    purc_variant_t ret_var = NULL;


    if ((argv[0] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (*string_filename == '/') {
        strcpy (filename, string_filename);
    }
    else {
        strcpy (filename, get_work_dirctory ());
        strcat (filename, "/");
        strcat (filename, string_filename);
    }

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

    if (argv[1] != PURC_VARIANT_INVALID) 
        purc_variant_cast_to_longint (argv[1], &line_num, false);

    line_num = -1 * line_num;

    fp = fopen (filename, "r");
    if (fp == NULL) {
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    if (line_num == 0)
        pos = filestat.st_size;
    else
        pos = find_line (fp, line_num, filestat.st_size);

    // pos is \n
    if (line_num < 0)
        pos++;                          

    fseek (fp, pos, SEEK_SET);

    pos = filestat.st_size - pos;

    char * content = malloc (pos + 1);
    if (content == NULL) {
        fclose (fp);
        return purc_variant_make_string ("", false);
    }

    *(content + pos) = 0x00;
    pos = fread (content, 1, pos - 1, fp);

    ret_var = purc_variant_make_string_reuse_buff (content, pos, false); 

    fclose (fp);

    return ret_var;
}

static purc_variant_t
bin_head_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    int64_t byte_num = 0;
    char filename[PATH_MAX] = {0,};
    const char* string_filename = NULL;
    FILE *fp = NULL;
    size_t pos = 0;
    struct stat filestat;
    purc_variant_t ret_var = NULL;

    if ((argv[0] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (*string_filename == '/') {
        strcpy (filename, string_filename);
    }
    else {
        strcpy (filename, get_work_dirctory ());
        strcat (filename, "/");
        strcat (filename, string_filename);
    }

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
        return purc_variant_make_byte_sequence (NULL, 0);
    }

    if (argv[1] != PURC_VARIANT_INVALID) 
        purc_variant_cast_to_longint (argv[1], &byte_num, false);

    fp = fopen (filename, "r");
    if (fp == NULL) { 
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    if (byte_num == 0)
        pos = filestat.st_size;
    else if (byte_num > 0)
        pos = byte_num;
    else {
        if ((-1 * byte_num) > filestat.st_size) {
            return purc_variant_make_byte_sequence (NULL, 0);
        }
        else
            pos = filestat.st_size + byte_num;
    }

    char * content = malloc (pos);
    if (content == NULL) {
        fclose (fp);
        return purc_variant_make_byte_sequence (NULL, 0);
    }

    fseek (fp, 0L, SEEK_SET);
    pos = fread (content, 1, pos, fp);

    ret_var = purc_variant_make_byte_sequence_reuse_buff (content, pos, pos); 

    fclose (fp);

    return ret_var;
}


static purc_variant_t
bin_tail_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    int64_t byte_num = 0;
    char filename[PATH_MAX] = {0,};
    const char* string_filename = NULL;
    FILE *fp = NULL;
    size_t pos = 0;
    struct stat filestat;
    purc_variant_t ret_var = NULL;

    if ((argv[0] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (*string_filename == '/') {
        strcpy (filename, string_filename);
    }
    else {
        strcpy (filename, get_work_dirctory ());
        strcat (filename, "/");
        strcat (filename, string_filename);
    }

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
        return purc_variant_make_byte_sequence (NULL, 0);
    }

    if (argv[1] != NULL) 
        purc_variant_cast_to_longint (argv[1], &byte_num, false);

    fp = fopen (filename, "r");
    if (fp == NULL) {
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    if (byte_num == 0)
        pos = filestat.st_size;
    else if (byte_num > 0)
        pos = byte_num;
    else {
        if ((-1 * byte_num) > filestat.st_size) {
            return purc_variant_make_byte_sequence (NULL, 0);
        }
        else
            pos = filestat.st_size + byte_num;
    }

    fseek (fp, filestat.st_size - pos, SEEK_SET);

    char * content = malloc (pos);
    if (content == NULL) {
        fclose (fp);
        return purc_variant_make_byte_sequence (NULL, 0);
    }

    pos = fread (content, 1, pos, fp);

    ret_var = purc_variant_make_byte_sequence_reuse_buff (content, pos, pos); 

    fclose (fp);

    return ret_var;
}


static purc_variant_t
stream_open_getter (purc_variant_t root, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    char filename[PATH_MAX] = {0,};
    const char *string_filename = NULL;
    struct stat filestat;
    purc_variant_t ret_var = NULL;
    purc_rwstream_t rwstream = NULL;

    if ((argv[0] != PURC_VARIANT_INVALID) && 
                            (!purc_variant_is_string (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    if (*string_filename == '/') {
        strcpy (filename, string_filename);
    }
    else {
        strcpy (filename, get_work_dirctory ());
        strcat (filename, "/");
        strcat (filename, string_filename);
    }

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

    rwstream = purc_rwstream_new_from_file (filename, "r");

    if (rwstream == NULL) {
        pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    struct purc_native_ops ops;
    ret_var = purc_variant_make_native (rwstream, &ops);

    return ret_var;
}


// check CPU type, true for little endian, otherwise big endian
static bool is_little_endian (void)
{
    union w
    {
        int a;
        char b;
    } c;

    c.a = 1;

    return (c.b == 1);
}


static inline void change_order (unsigned char * buf, size_t size)
{
    size_t time = 0;
    unsigned char temp = 0;

    for (time = 0; time < size / 2; size ++) {
        temp = *(buf + time);
        *(buf + time) = *(buf + size - 1 - time);
        *(buf + size - 1 - time) = temp;
    }
    return;
}

static inline void read_rwstream (purc_rwstream_t rwstream, 
                    unsigned char * buf, bool little, int bytes)
{
    purc_rwstream_read (rwstream, buf, bytes);
    if (!little)
        change_order (buf, bytes);
}

static inline purc_variant_t read_rwstream_float (purc_rwstream_t rwstream,
                                                    bool little, int bytes)
{
    purc_variant_t val = NULL;
    unsigned char buf[128];
    int compiler = sizeof (void *);
    float f = 0.0;
    double d = 0.0d;
    long double ld = 0.0d;

    purc_rwstream_read (rwstream, buf, bytes);
    if (!little)
        change_order (buf, bytes);

    switch (bytes) {
        case 2:
            switch (compiler) {
                case 16:
                    f = *((float *)buf);
                    d = (double)f;
                    val = purc_variant_make_number (d);
                    break;
                case 32:
                    break;
                case 64:
                    break;
            }
            break;
        case 4:
            switch (compiler) {
                case 16:
                    break;
                case 32:
                case 64:
                    f = *((float *)buf);
                    d = (double)f;
                    val = purc_variant_make_number (d);
                    break;
            }
            break;
        case 8:
            switch (compiler) {
                case 16:
                    val = purc_variant_make_number (0.0d);
                    break;
                case 32:
                case 64:
                    d = *((double *)buf);
                    val = purc_variant_make_number (d);
                    break;
            }
            break;
        case 12:
            switch (compiler) {
                case 16:
                    val = purc_variant_make_number (0.0d);
                    break;
                case 32:
                    val = purc_variant_make_number (0.0d);
                    break;
                case 64:
                    break;
            }
            break;
        case 16:
            switch (compiler) {
                case 16:
                    val = purc_variant_make_number (0.0d);
                    break;
                case 32:
                    break;
                case 64:
                    ld = *((long double *)buf);
                    val = purc_variant_make_longdouble (ld);
                    break;
            }
            break;
    }

    return val;
}

static purc_variant_t
stream_readstruct_getter (purc_variant_t root, size_t nr_args, 
                                                        purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    purc_variant_t ret_var = NULL;
    purc_variant_t val = NULL;
    purc_rwstream_t rwstream = NULL; 
    const char *format = NULL;
    const char *head = NULL;
    size_t length = 0;
    unsigned char buf[64];
    unsigned char * buffer = NULL;  // for string and bytes sequence
    int64_t i64 = 0;
    uint64_t u64 = 0;
    bool little = is_little_endian ();
    int read_number = 0;

    if ((argv[0] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_native (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    rwstream = purc_variant_native_get_entity (argv[0]); 
    if (rwstream == NULL) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_string (argv[1]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    format = purc_variant_get_string_const (argv[1]);
    head = pcdvobjs_get_next_option (format, " \t\n", &length);

    ret_var = purc_variant_make_array (0, PURC_VARIANT_INVALID);

    while (head) {
        switch (* head)
        {
            case 'i':
            case 'I':
                *((int64_t *)buf) = 0;
                if (strncasecmp (head, "i8", length) == 0)  
                    read_rwstream (rwstream, buf, little, 1);
                else if ((strncasecmp (head, "i16", length) == 0) ||
                        (strncasecmp (head, "i16le", length) == 0))
                    read_rwstream (rwstream, buf, little, 2);
                else if ((strncasecmp (head, "i32", length) == 0) ||
                        (strncasecmp (head, "i32le", length) == 0))
                    read_rwstream (rwstream, buf, little, 4);
                else if ((strncasecmp (head, "i64", length) == 0) ||
                        (strncasecmp (head, "i64le", length) == 0))
                    read_rwstream (rwstream, buf, little, 8);
                else if (strncasecmp (head, "i16be", length) == 0) 
                    read_rwstream (rwstream, buf, !little, 2);
                else if (strncasecmp (head, "i32be", length) == 0) 
                    read_rwstream (rwstream, buf, !little, 4);
                else if (strncasecmp (head, "i64be", length) == 0) 
                    read_rwstream (rwstream, buf, !little, 8);

                i64 = *((int64_t *)buf);
                val = purc_variant_make_longint (i64);
                break;
            case 'f':
            case 'F':
                *((float *)buf) = 0;
                if ( (strncasecmp (head, "f16", length) == 0) || 
                    (strncasecmp (head, "f16le", length) == 0)) 
                    val = read_rwstream_float (rwstream, little, 2);
                else if ((strncasecmp (head, "f32", length) == 0) ||
                    (strncasecmp (head, "f32le", length) == 0))  
                    val = read_rwstream_float (rwstream, little, 4);
                else if ((strncasecmp (head, "f64", length) == 0) ||
                    (strncasecmp (head, "f64le", length) == 0))  
                    val = read_rwstream_float (rwstream, little, 8);
                else if ((strncasecmp (head, "f96", length) == 0) ||
                    (strncasecmp (head, "f96le", length) == 0))  
                    val = read_rwstream_float (rwstream, little, 12);
                else if ((strncasecmp (head, "f128", length) == 0) ||
                    (strncasecmp (head, "f128le", length) == 0))  
                    val = read_rwstream_float (rwstream, little, 16);
                else if (strncasecmp (head, "f16be", length) == 0) 
                    val = read_rwstream_float (rwstream, !little, 2);
                else if (strncasecmp (head, "f32be", length) == 0) 
                    val = read_rwstream_float (rwstream, !little, 4);
                else if (strncasecmp (head, "f64be", length) == 0) 
                    val = read_rwstream_float (rwstream, !little, 8);
                else if (strncasecmp (head, "f96be", length) == 0) 
                    val = read_rwstream_float (rwstream, !little, 12);
                else if (strncasecmp (head, "f128be", length) == 0) 
                    val = read_rwstream_float (rwstream, !little, 16);
                break;
            case 'u':
            case 'U':
                *((uint64_t *)buf) = 0;
                if (strncasecmp (head, "u8", length) == 0)  
                    read_rwstream (rwstream, buf, little, 1);
                else if ((strncasecmp (head, "u16", length) == 0) ||
                        (strncasecmp (head, "u16le", length) == 0))
                    read_rwstream (rwstream, buf, little, 2);
                else if ((strncasecmp (head, "u32", length) == 0) ||
                        (strncasecmp (head, "u32le", length) == 0))
                    read_rwstream (rwstream, buf, little, 4);
                else if ((strncasecmp (head, "u64", length) == 0) ||
                        (strncasecmp (head, "u64le", length) == 0))
                    read_rwstream (rwstream, buf, little, 8);
                else if (strncasecmp (head, "u16be", length) == 0) 
                    read_rwstream (rwstream, buf, !little, 2);
                else if (strncasecmp (head, "u32be", length) == 0) 
                    read_rwstream (rwstream, buf, !little, 4);
                else if (strncasecmp (head, "u64be", length) == 0) 
                    read_rwstream (rwstream, buf, !little, 8);

                u64 = (uint64_t)*buf;
                val = purc_variant_make_ulongint (u64);
                break;
            case 'b':
            case 'B':
                if (length > 1)  { // get length
                    strncpy ((char *)buf, head + 1, length - 1);
                    *(buf + length - 1)= 0x00;
                    read_number = atoi((char *)buf);

                    if(read_number) {
                        buffer = malloc (read_number);
                        if (buffer == NULL)
                            val = purc_variant_make_null();
                        else {
                            purc_rwstream_read (rwstream, buffer, read_number);
                            ret_var = 
                                purc_variant_make_byte_sequence_reuse_buff(
                                        buffer, read_number, read_number);
                        }
                    }
                    else
                        val = purc_variant_make_null();
                }
                else
                    val = purc_variant_make_null();

                break;
            case 's':
            case 'S':
                if (length > 1)  { // get length
                    strncpy ((char *)buf, head + 1, length - 1);
                    *(buf + length - 1)= 0x00;
                    read_number = atoi((char *)buf);

                    if(read_number) {
                        buffer = malloc (read_number + 1);
                        if (buffer == NULL)
                            val = purc_variant_make_string ("", false);
                        else {
                            purc_rwstream_read (rwstream, buffer, read_number);
                            *(buffer + read_number) = 0x00;
                            ret_var = purc_variant_make_string_reuse_buff (
                                    (char *)buffer, read_number + 1, false);
                        }
                    }
                    else
                        val = purc_variant_make_string ("", false);
                }
                else {
                    int i = 0;
                    int j = 0;
                    size_t mem_size = 1024;

                    buffer = malloc (mem_size);
                    for (i = 0, j = 0; ; i++, j++)  {
                        purc_rwstream_read (rwstream, buffer + i, 1);
                        if (*(buffer + i) == 0x00)
                            break;

                        if (j == 1023) {
                            j = 0;
                            mem_size += 1024;
                            buffer = realloc (buffer, mem_size);
                        }
                    }
                    ret_var = purc_variant_make_string_reuse_buff (
                            (char *)buffer, i, false);
                }
                break;
        }

        purc_variant_array_append (ret_var, val);
        purc_variant_unref (val);
        head = pcdvobjs_get_next_option (head + length + 1, " \t\n", &length);
    }
    return ret_var;
}

static inline void write_rwstream_int (purc_rwstream_t rwstream, purc_variant_t arg,
                                        int *index, bool little, int bytes)
{
    purc_variant_t val = NULL;
    int64_t i64 = 0;

    val = purc_variant_array_get (arg, *index);
    (*index)++;
    purc_variant_cast_to_longint (val, &i64, false);
    if (!little)
        change_order ((unsigned char *)&i64, sizeof (int64_t));
    purc_rwstream_write (rwstream, (char *)&i64, bytes);
}

static inline void write_rwstream_uint (purc_rwstream_t rwstream, purc_variant_t arg,
                                        int *index, bool little, int bytes)
{
    purc_variant_t val = NULL;
    uint64_t u64 = 0;

    val = purc_variant_array_get (arg, *index);
    (*index)++;
    purc_variant_cast_to_ulongint (val, &u64, false);
    if (!little)
        change_order ((unsigned char *)&u64, sizeof (int64_t));
    purc_rwstream_write (rwstream, (char *)&u64, bytes);
}

static purc_variant_t
stream_writestruct_getter (purc_variant_t root, size_t nr_args, 
                                                        purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    purc_variant_t ret_var = NULL;
    purc_variant_t val = NULL;
    purc_rwstream_t rwstream = NULL; 
    const char *format = NULL;
    const char *head = NULL;
    size_t length = 0;
    unsigned char buf[64];
    const unsigned char * buffer = NULL;  // for string and bytes sequence
    float f = 0.0f;
    double d = 0.0d;
    //long double ld = 0.0d;
    bool little = is_little_endian ();
    int read_number = 0;
    int i = 0;
    size_t bsize = 0;

    if ((argv[0] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_native (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    rwstream = purc_variant_native_get_entity (argv[0]); 
    if (rwstream == NULL) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_string (argv[1]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[2] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_array (argv[2]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    format = purc_variant_get_string_const (argv[1]);
    head = pcdvobjs_get_next_option (format, " \t\n", &length);

    while (head) {
        switch (* head)
        {
            case 'i':
            case 'I':
                if (strncasecmp (head, "i8", length) == 0) 
                    write_rwstream_int (rwstream, argv[2], &i, little, 1);
                else if ((strncasecmp (head, "i16", length) == 0) || 
                         (strncasecmp (head, "i16le", length) == 0)) 
                    write_rwstream_int (rwstream, argv[2], &i, little, 2);
                else if ((strncasecmp (head, "i32", length) == 0) ||
                        (strncasecmp (head, "i32le", length) == 0))
                    write_rwstream_int (rwstream, argv[2], &i, little, 4);
                else if ((strncasecmp (head, "i64", length) == 0) ||
                        (strncasecmp (head, "i64le", length) == 0))
                    write_rwstream_int (rwstream, argv[2], &i, little, 8);
                else if (strncasecmp (head, "i16be", length) == 0) 
                    write_rwstream_int (rwstream, argv[2], &i, !little, 2);
                else if (strncasecmp (head, "i32be", length) == 0) 
                    write_rwstream_int (rwstream, argv[2], &i, !little, 4);
                else if (strncasecmp (head, "i64be", length) == 0) 
                    write_rwstream_int (rwstream, argv[2], &i, !little, 8);
                break;
            case 'f':
            case 'F':
                if (strncasecmp (head, "f16", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    if (!little)
                        change_order ((unsigned char *)&d, sizeof (double));
                    f = (float)d;
                    purc_rwstream_write (rwstream, (char *)&f, 2);
                }
                else if (strncasecmp (head, "f32", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    if (!little)
                        change_order ((unsigned char *)&d, sizeof (double));
                    f = (float)d;
                    purc_rwstream_write (rwstream, (char *)&f, 4);
                }
                else if (strncasecmp (head, "f64", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    if (!little)
                        change_order ((unsigned char *)&d, sizeof (double));
                    f = (float)d;
                    purc_rwstream_write (rwstream, (char *)&f, 8);
                }
                else if (strncasecmp (head, "f16be", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    if (little)
                        change_order ((unsigned char *)&d, sizeof (double));
                    f = (float)d;
                    purc_rwstream_write (rwstream, (char *)&f, 2);
                }
                else if (strncasecmp (head, "f32be", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    if (little)
                        change_order ((unsigned char *)&d, sizeof (double));
                    f = (float)d;
                    purc_rwstream_write (rwstream, (char *)&f, 4);
                }
                else if (strncasecmp (head, "f64be", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    if (little)
                        change_order ((unsigned char *)&d, sizeof (double));
                    f = (float)d;
                    purc_rwstream_write (rwstream, (char *)&f, 8);
                }
                else if (strncasecmp (head, "f16le", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    if (!little)
                        change_order ((unsigned char *)&d, sizeof (double));
                    f = (float)d;
                    purc_rwstream_write (rwstream, (char *)&f, 2);
                }
                else if (strncasecmp (head, "f32le", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    if (!little)
                        change_order ((unsigned char *)&d, sizeof (double));
                    f = (float)d;
                    purc_rwstream_write (rwstream, (char *)&f, 4);
                }
                else if (strncasecmp (head, "f64le", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    if (!little)
                        change_order ((unsigned char *)&d, sizeof (double));
                    f = (float)d;
                    purc_rwstream_write (rwstream, (char *)&f, 8);
                }
                break;
            case 'u':
            case 'U':
                if (strncasecmp (head, "u8", length) == 0) 
                    write_rwstream_uint (rwstream, argv[2], &i, little, 1);
                else if ((strncasecmp (head, "u16", length) == 0) || 
                         (strncasecmp (head, "u16le", length) == 0)) 
                    write_rwstream_uint (rwstream, argv[2], &i, little, 2);
                else if ((strncasecmp (head, "u32", length) == 0) ||
                        (strncasecmp (head, "u32le", length) == 0))
                    write_rwstream_uint (rwstream, argv[2], &i, little, 4);
                else if ((strncasecmp (head, "u64", length) == 0) ||
                        (strncasecmp (head, "u64le", length) == 0))
                    write_rwstream_uint (rwstream, argv[2], &i, little, 8);
                else if (strncasecmp (head, "u16be", length) == 0) 
                    write_rwstream_uint (rwstream, argv[2], &i, !little, 2);
                else if (strncasecmp (head, "u32be", length) == 0) 
                    write_rwstream_uint (rwstream, argv[2], &i, !little, 4);
                else if (strncasecmp (head, "u64be", length) == 0) 
                    write_rwstream_uint (rwstream, argv[2], &i, !little, 8);

                break;
            case 'b':
            case 'B':
                val = purc_variant_array_get (argv[2], i);
                i++;

                // get sequence length
                if (length > 1)  {
                    strncpy ((char *)buf, head + 1, length - 1);
                    *(buf + length - 1)= 0x00;
                    read_number = atoi((char *)buf);

                    if(read_number) {
                        bsize = read_number;
                        buffer = purc_variant_get_bytes_const (val, &bsize);
                        purc_rwstream_write (rwstream, buffer, read_number);
                    }
                }
                break;

            case 's':
            case 'S':
                val = purc_variant_array_get (argv[2], i);
                i++;

                // get string length
                if (length > 1)  { 
                    strncpy ((char *)buf, head + 1, length - 1);
                    *(buf + length - 1)= 0x00;
                    read_number = atoi((char *)buf);
                }
                else {
                    read_number = purc_variant_string_length (val);
                }

                if(read_number) {
                    buffer = (unsigned char *)purc_variant_get_string_const 
                                                                        (val);
                    purc_rwstream_write (rwstream, buffer, read_number);
                }
                break;
        }
        head = pcdvobjs_get_next_option (head + length + 1, " \t\n", &length);
    }
    return ret_var;
}

static purc_variant_t
stream_readlines_getter (purc_variant_t root, size_t nr_args, 
                                                        purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    purc_variant_t ret_var = NULL;
    purc_rwstream_t rwstream = NULL; 
    int64_t line_num = 0;


    if ((argv[0] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_native (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    rwstream = purc_variant_native_get_entity (argv[0]); 
    if (rwstream == NULL) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if (argv[1] != PURC_VARIANT_INVALID) {
        purc_variant_cast_to_longint (argv[1], &line_num, false);
        if (line_num < 0)
            line_num = 0;
    }

    if (line_num == 0)
        ret_var = purc_variant_make_string ("", false);
    else {
        size_t pos = 0;
        char * content = find_line_in_stream (rwstream, line_num, &pos);
        if (content == NULL) {
            return purc_variant_make_string ("", false);
        }

        ret_var = purc_variant_make_string_reuse_buff (content, pos, false); 
    }
    return ret_var;
}

static purc_variant_t
stream_readbytes_getter (purc_variant_t root, size_t nr_args, 
                                                        purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    purc_variant_t ret_var = NULL;
    purc_rwstream_t rwstream = NULL; 
    uint64_t byte_num = 0;

    if ((argv[0] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_native (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    rwstream = purc_variant_native_get_entity (argv[0]); 
    if (rwstream == NULL) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if (argv[1] != PURC_VARIANT_INVALID) {
        purc_variant_cast_to_ulongint (argv[1], &byte_num, false);
    }

    if (byte_num == 0)
        ret_var = purc_variant_make_byte_sequence (NULL, 0);
    else {
        char * content = malloc (byte_num);
        size_t size = 0;

        if (content == NULL) {
            pcinst_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PURC_VARIANT_INVALID;
        }

        size = purc_rwstream_read (rwstream, content, byte_num);
        if (size > 0)
            ret_var = purc_variant_make_byte_sequence_reuse_buff (content, 
                                                            byte_num, byte_num);
        else {
            free (content);
            pcinst_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
            return PURC_VARIANT_INVALID;
        }
    }

    return ret_var;
}

static purc_variant_t
stream_seek_getter (purc_variant_t root, size_t nr_args, 
                                                        purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    purc_variant_t ret_var = NULL;
    purc_rwstream_t rwstream = NULL; 
    int64_t byte_num = 0;
    off_t off = 0;

    if ((argv[0] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_native (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    rwstream = purc_variant_native_get_entity (argv[0]); 
    if (rwstream == NULL) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if (argv[1] != PURC_VARIANT_INVALID) {
        purc_variant_cast_to_longint (argv[1], &byte_num, false);
    }

    off = purc_rwstream_seek (rwstream, byte_num, SEEK_CUR);
    ret_var = purc_variant_make_ulongint (off);

    return ret_var;
}

static purc_variant_t
stream_close_getter (purc_variant_t root, size_t nr_args, 
                                                        purc_variant_t* argv)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(nr_args);

    purc_variant_t ret_var = NULL;
    purc_rwstream_t rwstream = NULL; 
    int close = 0;

    if ((argv[0] != PURC_VARIANT_INVALID) && 
                        (!purc_variant_is_native (argv[0]))) {
        pcinst_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    rwstream = purc_variant_native_get_entity (argv[0]); 
    if (rwstream == NULL) {
        pcinst_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    close = purc_rwstream_destroy (rwstream);

    if (close == 0)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

// only for test now.
purc_variant_t pcdvojbs_get_file (void)
{
    size_t size = 0;

    static struct pcdvojbs_dvobjs text [] = {
        {"head",     text_head_getter, NULL},
        {"tail",     text_tail_getter, NULL} };

    static struct pcdvojbs_dvobjs  bin[] = {
        {"head",     bin_head_getter, NULL},
        {"tail",     bin_tail_getter, NULL} };

    static struct pcdvojbs_dvobjs  stream[] = {
        {"open",        stream_open_getter,        NULL},
        {"readstruct",  stream_readstruct_getter,  NULL},
        {"writestruct", stream_writestruct_getter, NULL},
        {"readlines",   stream_readlines_getter,   NULL},
        {"readbytes",   stream_readbytes_getter,   NULL},
        {"seek",        stream_seek_getter,        NULL},
        {"close",       stream_close_getter,       NULL} };

    size = sizeof (text) / sizeof (struct pcdvojbs_dvobjs);
    purc_variant_t file_text = pcdvobjs_make_dvobjs (text, size);
    if (file_text == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;


    size = sizeof (bin) / sizeof (struct pcdvojbs_dvobjs);
    purc_variant_t file_bin = pcdvobjs_make_dvobjs (bin, size);
    if (file_bin == PURC_VARIANT_INVALID) {
        purc_variant_unref (file_text);
        return PURC_VARIANT_INVALID;
    }

    size = sizeof (stream) / sizeof (struct pcdvojbs_dvobjs);
    purc_variant_t file_stream = pcdvobjs_make_dvobjs (stream, size);
    if (file_stream == PURC_VARIANT_INVALID) {
        purc_variant_unref (file_text);
        purc_variant_unref (file_bin);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t file = purc_variant_make_object_by_static_ckey (3,
                                "text",   file_text,
                                "bin",    file_bin,
                                "stream", file_stream);
    purc_variant_unref (file_text);
    purc_variant_unref (file_bin);
    purc_variant_unref (file_stream);

    return file;
}
