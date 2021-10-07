/*
 * @file fs.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of file system dynamic variant object.
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

#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "private/utils.h"
#include "private/dvobjs.h"
#include "purc-variant.h"

#include <unistd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysmacros.h>

// for FILE
#define BUFFER_SIZE         1024
#define ENDIAN_PLATFORM     0
#define ENDIAN_LITTLE       1
#define ENDIAN_BIG          2

#define FS_DVOBJ_VERSION    0

typedef purc_variant_t (*pcdvobjs_create) (void);

// as FILE, FS, MATH
struct pcdvojbs_dvobjs_object {
    const char *name;
    const char *description;
    pcdvobjs_create create_func;
};

// dynamic variant in dynamic object
struct pcdvojbs_dvobjs {
    const char *name;
    purc_dvariant_method getter;
    purc_dvariant_method setter;
};

static const char * pcdvobjs_get_next_option (const char *data, 
        const char *delims, size_t *length)
{
    const char *head = data;
    char *temp = NULL;

    if ((delims == NULL) || (data == NULL) || (*delims == 0x00))
        return NULL;

    *length = 0;

    while (*data != 0x00) {
        temp = strchr (delims, *data);
        if (temp) {
            if (head == data) {
                head = data + 1;
            } else
                break;
        }
        data++;
    }

    *length = data - head;
    if (*length == 0)
        head = NULL;

    return head;
}

// for file to get '\n'
static const char * pcdvobjs_file_get_next_option (const char *data,
        const char *delims, size_t *length)
{
    const char *head = data;
    char *temp = NULL;

    if ((delims == NULL) || (data == NULL) || (*delims == 0x00))
        return NULL;

    *length = 0;

    while (*data != 0x00) {
        temp = strchr (delims, *data);
        if (temp)
            break;
        data++;
    }

    *length = data - head;

    return head;
}

static const char * pcdvobjs_file_get_prev_option (const char *data,
        size_t str_len, const char *delims, size_t *length)
{
    const char *head = NULL;
    size_t tail = str_len;
    char* temp = NULL;

    if ((delims == NULL) || (data == NULL) || (*delims == 0x00) ||
            (str_len == 0))
        return NULL;

    *length = 0;

    while (str_len) {
        temp = strchr (delims, *(data + str_len - 1));
        if (temp)
            break;
        str_len--;
    }

    *length = tail - str_len;
    head = data + str_len;

    return head;
}

static const char * pcdvobjs_remove_space (char *buffer)
{
    int i = 0;
    int j = 0;
    while (*(buffer + i) != 0x00) {
        if (*(buffer + i) != ' ') {
            *(buffer + j) = *(buffer + i);
            j++;
        }
        i++;
    }
    *(buffer + j) = 0x00;

    return buffer;
}

static bool wildcard_cmp (const char *str1, const char *pattern)
{
    if (str1 == NULL)
        return false;
    if (pattern == NULL)
        return false;

    int len1 = strlen (str1);
    int len2 = strlen (pattern);
    int mark = 0;
    int p1 = 0;
    int p2 = 0;

    while ((p1 < len1) && (p2<len2)) {
        if (pattern[p2] == '?') {
            p1++;
            p2++;
            continue;
        }
        if (pattern[p2] == '*') {
            p2++;
            mark = p2;
            continue;
        }
        if (str1[p1] != pattern[p2]) {
            if (p1 == 0 && p2 == 0)
                return false;
            p1 -= p2 - mark - 1;
            p2 = mark;
            continue;
        }
        p1++;
        p2++;
    }
    if (p2 == len2) {
        if (p1 == len1)
            return true;
        if (pattern[p2 - 1] == '*')
            return true;
    }
    while (p2 < len2) {
        if (pattern[p2] != '*')
            return false;
        p2++;
    }
    return true;
}

static purc_variant_t pcdvobjs_make_dvobjs (
        const struct pcdvojbs_dvobjs *method, size_t size)
{
    size_t i = 0;
    purc_variant_t val = PURC_VARIANT_INVALID;
    purc_variant_t ret_var= purc_variant_make_object (0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);

    if (ret_var == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    for (i = 0; i < size; i++) {
        val = purc_variant_make_dynamic (method[i].getter, method[i].setter);
        if (val == PURC_VARIANT_INVALID) {
            goto error;
        }

        if (!purc_variant_object_set_by_static_ckey (ret_var,
                    method[i].name, val)) {
            goto error;
        }

        purc_variant_unref (val);
    }

    return ret_var;

error:
    purc_variant_unref (ret_var);
    return PURC_VARIANT_INVALID;
}

static inline bool is_endian (void)
{
#if CPU(BIG_ENDIAN)
    return false;
#elif CPU(LITTLE_ENDIAN)
    return true;
#endif
}

static ssize_t find_line (FILE *fp, int line_num, ssize_t file_length)
{
    size_t pos = 0;
    int i = 0;
    unsigned char buffer[BUFFER_SIZE];
    ssize_t read_size = 0;
    size_t length = 0;
    const char *head = NULL;

    if (line_num > 0) {
        fseek (fp, 0L, SEEK_SET);

        while (line_num) {
            read_size = fread (buffer, 1, BUFFER_SIZE, fp);
            if (read_size < 0)
                break;

            head = pcdvobjs_file_get_next_option ((char *)buffer,
                    "\n", &length);
            while (head) {
                pos += length + 1;          // to be checked
                line_num --;

                if (line_num == 0)
                    break;

                head = pcdvobjs_file_get_next_option (head + length + 1, 
                        "\n", &length);
            }
            if (read_size < BUFFER_SIZE)           // to the end
                break;

            if (line_num == 0)
                break;
        }
    } else {
        line_num = -1 * line_num;
        file_length --;                     // the last is 0x0A
        pos = file_length;

        while (line_num) {
            if (file_length <= BUFFER_SIZE)
                fseek (fp, 0L, SEEK_SET);
            else
                fseek (fp, file_length - (i + 1) * BUFFER_SIZE, SEEK_SET);

            read_size = fread (buffer, 1, BUFFER_SIZE, fp);
            if (read_size < 0)
                break;

            head = pcdvobjs_file_get_prev_option ((char *)buffer,
                    read_size, "\n", &length);
            while (head) {
                pos -= length;
                pos--;
                line_num --;

                if (line_num == 0)
                    break;

                read_size -= length;
                read_size--;
                head = pcdvobjs_file_get_prev_option ((char *)buffer,
                        read_size, "\n", &length);
            }
            if (read_size < BUFFER_SIZE)           // to the end
                break;

            if (line_num == 0)
                break;

            i ++;
            file_length -= BUFFER_SIZE;
        }
    }

    return pos;
}

static ssize_t find_line_stream (purc_rwstream_t stream, int line_num)
{
    size_t pos = 0;
    unsigned char buffer[BUFFER_SIZE];
    ssize_t read_size = 0;
    size_t length = 0;
    const char *head = NULL;

    purc_rwstream_seek (stream, 0L, SEEK_SET);

    while (line_num) {
        read_size = purc_rwstream_read (stream, buffer, BUFFER_SIZE);
        if (read_size < 0)
            break;

        head = pcdvobjs_file_get_next_option ((char *)buffer,
                "\n", &length);
        while (head) {
            pos += length + 1;          // to be checked
            line_num --;

            if (line_num == 0)
                break;

            head = pcdvobjs_file_get_next_option (head + length + 1,
                    "\n", &length);
        }
        if (read_size < BUFFER_SIZE)           // to the end
            break;

        if (line_num == 0)
            break;
    }

    return pos;
}

static purc_variant_t
text_head_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    int64_t line_num = 0;
    const char *filename = NULL;
    FILE *fp = NULL;
    size_t pos = 0;
    struct stat filestat;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args != 2) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);

    // check whether the file exists
    if((access(filename, F_OK | R_OK)) != 0) {
        purc_set_error (PURC_ERROR_NOT_EXISTS);
        return PURC_VARIANT_INVALID;
    }

    // get the file length
    if(stat(filename, &filestat) < 0) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }
    if (filestat.st_size == 0) {
        return purc_variant_make_string ("", false);
    }

    if (argv[1] != PURC_VARIANT_INVALID)
        purc_variant_cast_to_longint (argv[1], &line_num, false);

    fp = fopen (filename, "r");
    if (fp == NULL) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    if (line_num == 0)
        pos = filestat.st_size;
    else
        pos = find_line (fp, line_num, filestat.st_size);

    char *content = malloc (pos + 1);
    if (content == NULL) {
        fclose (fp);
        return purc_variant_make_string ("", false);
    }

    fseek (fp, 0L, SEEK_SET);
    pos = fread (content, 1, pos, fp);
    *(content + pos) = 0x00;

    ret_var = purc_variant_make_string_reuse_buff (content, pos, false);
    fclose (fp);

    return ret_var;
}

static purc_variant_t
text_tail_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    int64_t line_num = 0;
    const char *filename = NULL;
    FILE *fp = NULL;
    size_t pos = 0;
    struct stat filestat;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args != 2) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);

    // check whether the file exists
    if((access(filename, F_OK | R_OK)) != 0) {
        purc_set_error (PURC_ERROR_NOT_EXISTS);
        return PURC_VARIANT_INVALID;
    }

    // get the file length
    if(stat(filename, &filestat) < 0) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
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
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
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

    char *content = malloc (pos + 1);
    if (content == NULL) {
        fclose (fp);
        return purc_variant_make_string ("", false);
    }

    pos = fread (content, 1, pos - 1, fp);
    *(content + pos) = 0x00;

    ret_var = purc_variant_make_string_reuse_buff (content, pos, false);

    fclose (fp);

    return ret_var;
}

static purc_variant_t
bin_head_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    int64_t byte_num = 0;
    const char *filename = NULL;
    FILE *fp = NULL;
    size_t pos = 0;
    struct stat filestat;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args != 2) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);

    // check whether the file exists
    if((access(filename, F_OK | R_OK)) != 0) {
        purc_set_error (PURC_ERROR_NOT_EXISTS);
        return PURC_VARIANT_INVALID;
    }

    // get the file length
    if(stat(filename, &filestat) < 0) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }
    if (filestat.st_size == 0) {
        return PURC_VARIANT_INVALID;
    }

    if (argv[1] != PURC_VARIANT_INVALID)
        purc_variant_cast_to_longint (argv[1], &byte_num, false);

    fp = fopen (filename, "r");
    if (fp == NULL) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    if (byte_num == 0)
        pos = filestat.st_size;
    else if (byte_num > 0)
        pos = byte_num;
    else {
        if ((-1 * byte_num) > filestat.st_size) {
            return PURC_VARIANT_INVALID;
        } else
            pos = filestat.st_size + byte_num;
    }

    char *content = malloc (pos);
    if (content == NULL) {
        fclose (fp);
        return PURC_VARIANT_INVALID;
    }

    fseek (fp, 0L, SEEK_SET);
    pos = fread (content, 1, pos, fp);

    ret_var = purc_variant_make_byte_sequence_reuse_buff (content, pos, pos);

    fclose (fp);

    return ret_var;
}


static purc_variant_t
bin_tail_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    int64_t byte_num = 0;
    const char *filename = NULL;
    FILE *fp = NULL;
    size_t pos = 0;
    struct stat filestat;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args != 2) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);

    // check whether the file exists
    if((access(filename, F_OK | R_OK)) != 0) {
        purc_set_error (PURC_ERROR_NOT_EXISTS);
        return PURC_VARIANT_INVALID;
    }


    // get the file length
    if(stat(filename, &filestat) < 0) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }
    if (filestat.st_size == 0) {
        return PURC_VARIANT_INVALID;
    }

    if (argv[1] != NULL)
        purc_variant_cast_to_longint (argv[1], &byte_num, false);

    fp = fopen (filename, "r");
    if (fp == NULL) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    if (byte_num == 0)
        pos = filestat.st_size;
    else if (byte_num > 0)
        pos = byte_num;
    else {
        if ((-1 * byte_num) > filestat.st_size) {
            return PURC_VARIANT_INVALID;
        } else
            pos = filestat.st_size + byte_num;
    }

    fseek (fp, filestat.st_size - pos, SEEK_SET);

    char *content = malloc (pos);
    if (content == NULL) {
        fclose (fp);
        return PURC_VARIANT_INVALID;
    }

    pos = fread (content, 1, pos, fp);

    ret_var = purc_variant_make_byte_sequence_reuse_buff (content, pos, pos);

    fclose (fp);

    return ret_var;
}


static purc_variant_t
stream_open_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    if (nr_args != 2) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    const char *filename = NULL;
    struct stat filestat;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = NULL;

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[1]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);
    // check whether the file exists
    if((access(filename, F_OK | R_OK)) != 0) {
        purc_set_error (PURC_ERROR_NOT_EXISTS);
        return PURC_VARIANT_INVALID;
    }

    // get the file length
    if(stat(filename, &filestat) < 0) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    rwstream = purc_rwstream_new_from_file (filename,
            purc_variant_get_string_const (argv[1]));

    if (rwstream == NULL) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        return PURC_VARIANT_INVALID;
    }

    static struct purc_native_ops ops;
    memset (&ops, 0, sizeof(ops));
    ret_var = purc_variant_make_native (rwstream, &ops);

    return ret_var;
}

static inline void change_order (unsigned char *buf, size_t size)
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
                    unsigned char *buf, int type, int bytes)
{
    purc_rwstream_read (rwstream, buf, bytes);
    switch (type) {
        case ENDIAN_PLATFORM:
            break;
        case ENDIAN_LITTLE:
            if (!is_endian ())
                change_order (buf, bytes);
            break;
        case ENDIAN_BIG:
            if (is_endian ())
                change_order (buf, bytes);
            break;
    }
}

/*
   According to IEEE 754
    sign    e      base   offset
16   1      5       10      15
32   1      8       23     127
64   1      11      52     1023
96   1      15      64     16383
128  1      15      64     16383
*/

static purc_variant_t
read_rwstream_float (purc_rwstream_t rwstream, int type, int bytes)
{
    purc_variant_t val = PURC_VARIANT_INVALID;
    unsigned char buf[128];
    float f = 0.0;
    double d = 0.0d;
    long double ld = 0.0d;
    unsigned long long sign = 0;
    unsigned long long e = 0;
    unsigned long long base = 0;
    unsigned short number = 0;

    // change byte order to little endian
    purc_rwstream_read (rwstream, buf, bytes);
    switch (type) {
        case ENDIAN_PLATFORM:
            break;
        case ENDIAN_LITTLE:
            if (!is_endian ())
                change_order (buf, bytes);
            break;
        case ENDIAN_BIG:
            if (is_endian ())
                change_order (buf, bytes);
            break;
    }

    switch (bytes) {
        case 2:
            number = *((unsigned short *)buf);
            sign = number >> 15;
            e = (number >> 10) & 0x1F;
            base = number & 0x3FF;

            sign = sign << 63;
            e = 1023 + e - 15;
            e = e << 52;
            sign |= e;
            base = base << (52 - 10);
            sign |= base;
            memcpy (buf, &sign, 8);

            d = *((double *)buf);
            val = purc_variant_make_number (d);
            break;
        case 4:
            f = *((float *)buf);
            d = (double)f;
            val = purc_variant_make_number (d);
            break;
        case 8:
            d = *((double *)buf);
            val = purc_variant_make_number (d);
            break;
        case 12:
        case 16:
            ld = *((long double *)buf);
            val = purc_variant_make_longdouble (ld);
            break;
    }

    return val;
}


static purc_variant_t
stream_readstruct_getter (purc_variant_t root, size_t nr_args,
        purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = NULL;
    const char *format = NULL;
    const char *head = NULL;
    size_t length = 0;
    unsigned char buf[64];
    unsigned char * buffer = NULL;  // for string and bytes sequence
    int64_t i64 = 0;
    uint64_t u64 = 0;
    int read_number = 0;

    if (nr_args != 2) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_native (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    rwstream = purc_variant_native_get_entity (argv[0]);
    if (rwstream == NULL) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] != PURC_VARIANT_INVALID) &&
                        (!purc_variant_is_string (argv[1]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
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
                    read_rwstream (rwstream, buf, ENDIAN_PLATFORM, 1);
                else if (strncasecmp (head, "i16", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_PLATFORM, 2);
                else if (strncasecmp (head, "i32", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_PLATFORM, 4);
                else if (strncasecmp (head, "i64", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_PLATFORM, 8);
                else if (strncasecmp (head, "i16le", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_LITTLE, 2);
                else if (strncasecmp (head, "i32le", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_LITTLE, 4);
                else if (strncasecmp (head, "i64le", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_LITTLE, 8);
                else if (strncasecmp (head, "i16be", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_BIG, 2);
                else if (strncasecmp (head, "i32be", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_BIG, 4);
                else if (strncasecmp (head, "i64be", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_BIG, 8);

                i64 = (int64_t)(*((int64_t *)buf));
                val = purc_variant_make_longint (i64);
                break;
            case 'f':
            case 'F':
                *((float *)buf) = 0;
                if (strncasecmp (head, "f16", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_PLATFORM, 2);
                else if (strncasecmp (head, "f32", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_PLATFORM, 4);
                else if (strncasecmp (head, "f64", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_PLATFORM, 8);
                else if (strncasecmp (head, "f96", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_PLATFORM, 12);
                else if (strncasecmp (head, "f128", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_PLATFORM, 16);

                else if (strncasecmp (head, "f16le", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_LITTLE, 2);
                else if (strncasecmp (head, "f32le", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_LITTLE, 4);
                else if (strncasecmp (head, "f64le", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_LITTLE, 8);
                else if (strncasecmp (head, "f96le", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_LITTLE, 12);
                else if (strncasecmp (head, "f128le", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_LITTLE, 16);

                else if (strncasecmp (head, "f16be", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_BIG, 2);
                else if (strncasecmp (head, "f32be", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_BIG, 4);
                else if (strncasecmp (head, "f64be", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_BIG, 8);
                else if (strncasecmp (head, "f96be", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_BIG, 12);
                else if (strncasecmp (head, "f128be", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_BIG, 16);
                break;
            case 'u':
            case 'U':
                *((uint64_t *)buf) = 0;
                if (strncasecmp (head, "u8", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_PLATFORM, 1);
                else if (strncasecmp (head, "u16", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_PLATFORM, 2);
                else if (strncasecmp (head, "u32", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_PLATFORM, 4);
                else if (strncasecmp (head, "u64", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_PLATFORM, 8);
                else if (strncasecmp (head, "u16le", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_LITTLE, 2);
                else if (strncasecmp (head, "u32le", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_LITTLE, 4);
                else if (strncasecmp (head, "u64le", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_LITTLE, 8);
                else if (strncasecmp (head, "u16be", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_BIG, 2);
                else if (strncasecmp (head, "u32be", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_BIG, 4);
                else if (strncasecmp (head, "u64be", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_BIG, 8);

                u64 = (uint64_t)(*((uint64_t *)buf));
                val = purc_variant_make_ulongint (u64);
                break;
            case 'b':
            case 'B':
                if (length > 1) { // get length
                    strncpy ((char *)buf, head + 1, length - 1);
                    *(buf + length - 1)= 0x00;
                    read_number = atoi((char *)buf);

                    if(read_number) {
                        buffer = malloc (read_number);
                        if (buffer == NULL)
                            val = purc_variant_make_null();
                        else {
                            purc_rwstream_read (rwstream, buffer, read_number);
                            val = purc_variant_make_byte_sequence_reuse_buff(
                                        buffer, read_number, read_number);
                        }
                    } else
                        val = purc_variant_make_null();
                } else
                    val = purc_variant_make_null();

                break;
            case 'p':
            case 'P':
                if (length > 1) { // get length
                    strncpy ((char *)buf, head + 1, length - 1);
                    *(buf + length - 1)= 0x00;
                    read_number = atoi((char *)buf);

                    if(read_number) {
                        int i = 0;
                        int times = read_number / sizeof(long double);
                        int rest = read_number % sizeof(long double);
                        long double ld = 0;
                        for (i = 0; i < times; i++)
                            purc_rwstream_read (rwstream,
                                    &ld, sizeof(long double));
                        purc_rwstream_read (rwstream, &ld, rest);
                    }
                }
                break;
            case 's':
            case 'S':
                if (length > 1) {          // get length
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
                            val = purc_variant_make_string_reuse_buff (
                                    (char *)buffer, read_number + 1, false);
                        }
                    } else
                        val = purc_variant_make_string ("", false);
                } else {
                    int i = 0;
                    int j = 0;
                    size_t mem_size = BUFFER_SIZE;

                    buffer = malloc (mem_size);
                    for (i = 0, j = 0; ; i++, j++) {
                        purc_rwstream_read (rwstream, buffer + i, 1);
                        if (*(buffer + i) == 0x00)
                            break;

                        if (j == 1023) {
                            j = 0;
                            mem_size += BUFFER_SIZE;
                            buffer = realloc (buffer, mem_size);
                        }
                    }
                    val = purc_variant_make_string_reuse_buff (
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

static inline void write_rwstream_int (purc_rwstream_t rwstream,
        purc_variant_t arg, int *index, int type, int bytes, size_t *length)
{
    purc_variant_t val = PURC_VARIANT_INVALID;
    int64_t i64 = 0;

    val = purc_variant_array_get (arg, *index);
    (*index)++;
    purc_variant_cast_to_longint (val, &i64, false);
    switch (type) {
        case ENDIAN_PLATFORM:
            break;
        case ENDIAN_LITTLE:
            if (!is_endian ())
                change_order ((unsigned char *)&i64, sizeof (int64_t));
            break;
        case ENDIAN_BIG:
            if (is_endian ())
                change_order ((unsigned char *)&i64, sizeof (int64_t));
            break;
    }

    if (is_endian ())
        purc_rwstream_write (rwstream, (char *)&i64, bytes);
    else
        purc_rwstream_write (rwstream,
                (char *)&i64 + sizeof(int64_t) - bytes, bytes);
    *length += bytes;
}

static inline unsigned short  write_double_to_16 (double d, int type)
{
    unsigned long long number = 0;
    unsigned long long sign = 0;
    unsigned long long e = 0;
    unsigned long long base = 0;
    unsigned short ret = 0;

    memcpy (&number, &d, sizeof(double));

    sign = number >> 63;
    e = (number >> 52) & 0x7FFFF;
    base = (number & 0xFFFFFFFFFFFFF) >> (52 - 10);

    e = 15 + e - 1023;
    e = e << 10;
    base |= e;
    base |= (sign << 15);
    ret = base;

    switch (type) {
        case ENDIAN_PLATFORM:
            break;
        case ENDIAN_LITTLE:
            if (!is_endian ())
                change_order ((unsigned char *)&ret, 2);
            break;
        case ENDIAN_BIG:
            if (is_endian ())
                change_order ((unsigned char *)&ret, 2);
            break;
    }
    return ret;
}

static inline void write_rwstream_uint (purc_rwstream_t rwstream,
        purc_variant_t arg, int *index, int type, int bytes, size_t *length)
{
    purc_variant_t val = PURC_VARIANT_INVALID;
    uint64_t u64 = 0;

    val = purc_variant_array_get (arg, *index);
    (*index)++;
    purc_variant_cast_to_ulongint (val, &u64, false);
    switch (type) {
        case ENDIAN_PLATFORM:
            break;
        case ENDIAN_LITTLE:
            if (!is_endian ())
                change_order ((unsigned char *)&u64, sizeof (uint64_t));
            break;
        case ENDIAN_BIG:
            if (is_endian ())
                change_order ((unsigned char *)&u64, sizeof (uint64_t));
            break;
    }

    if (is_endian ())
        purc_rwstream_write (rwstream, (char *)&u64, bytes);
    else
        purc_rwstream_write (rwstream,
                (char *)&u64 + sizeof(uint64_t) - bytes, bytes);
    *length += bytes;
}

static purc_variant_t
stream_writestruct_getter (purc_variant_t root, size_t nr_args,
        purc_variant_t* argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = NULL;
    const char *format = NULL;
    const char *head = NULL;
    size_t length = 0;
    unsigned char buf[64];
    const unsigned char * buffer = NULL;  // for string and bytes sequence
    float f = 0.0d;
    double d = 0.0d;
    long double ld = 0.0d;
    int write_number = 0;
    int i = 0;
    size_t bsize = 0;
    unsigned short ui16 = 0;
    size_t write_length = 0;

    if (nr_args != 3) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_native (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    rwstream = purc_variant_native_get_entity (argv[0]);
    if (rwstream == NULL) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[1] != PURC_VARIANT_INVALID) &&
                        (!purc_variant_is_string (argv[1]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[2] != PURC_VARIANT_INVALID) &&
                        (!purc_variant_is_array (argv[2]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
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
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 1, &write_length);
                else if (strncasecmp (head, "i16", length) == 0)
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 2, &write_length);
                else if (strncasecmp (head, "i32", length) == 0)
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 4, &write_length);
                else if (strncasecmp (head, "i64", length) == 0)
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 8, &write_length);
                else if (strncasecmp (head, "i16le", length) == 0)
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_LITTLE, 2, &write_length);
                else if (strncasecmp (head, "i32le", length) == 0)
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_LITTLE, 4, &write_length);
                else if (strncasecmp (head, "i64le", length) == 0)
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_LITTLE, 8, &write_length);
                else if (strncasecmp (head, "i16be", length) == 0)
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_BIG, 2, &write_length);
                else if (strncasecmp (head, "i32be", length) == 0)
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_BIG, 4, &write_length);
                else if (strncasecmp (head, "i64be", length) == 0)
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_BIG, 8, &write_length);
                break;
            case 'f':
            case 'F':
                if (strncasecmp (head, "f16", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    ui16 = write_double_to_16 (d, ENDIAN_PLATFORM);
                    purc_rwstream_write (rwstream, &ui16, 2);
                    write_length += 2;
                }
                else if (strncasecmp (head, "f32", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    f = (float)d;
                    purc_rwstream_write (rwstream, (char *)&f, 4);
                    write_length += 4;
                }
                else if (strncasecmp (head, "f64", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    purc_rwstream_write (rwstream, (char *)&d, 8);
                    write_length += 8;
                }
                else if (strncasecmp (head, "f96", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_long_double (val, &ld, false);
                    purc_rwstream_write (rwstream, (char *)&ld, 12);
                    write_length += 12;
                }
                else if (strncasecmp (head, "f128", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_long_double (val, &ld, false);
                    purc_rwstream_write (rwstream, (char *)&ld, 16);
                    write_length += 16;
                }
                else if (strncasecmp (head, "f16be", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    ui16 = write_double_to_16 (d, ENDIAN_BIG);
                    purc_rwstream_write (rwstream, &ui16, 2);
                    write_length += 2;
                }
                else if (strncasecmp (head, "f32be", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    f = (float)d;
                    if (is_endian ())
                        change_order ((unsigned char *)&f, sizeof (float));
                    purc_rwstream_write (rwstream, (char *)&f, 4);
                    write_length += 4;
                }
                else if (strncasecmp (head, "f64be", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    if (is_endian ())
                        change_order ((unsigned char *)&d, sizeof (double));
                    purc_rwstream_write (rwstream, (char *)&d, 8);
                    write_length += 8;
                }
                else if (strncasecmp (head, "f96be", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_long_double (val, &ld, false);
                    if (is_endian ())
                        change_order ((unsigned char *)&ld,
                                sizeof (long double));
                    purc_rwstream_write (rwstream, (char *)&ld, 12);
                    write_length += 12;
                }
                else if (strncasecmp (head, "f128be", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_long_double (val, &ld, false);
                    if (is_endian ())
                        change_order ((unsigned char *)&ld,
                                sizeof (long double));
                    purc_rwstream_write (rwstream, (char *)&ld, 16);
                    write_length += 16;
                }
                else if (strncasecmp (head, "f16le", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    ui16 = write_double_to_16 (d, ENDIAN_LITTLE);
                    purc_rwstream_write (rwstream, &ui16, 2);
                    write_length += 2;
                }
                else if (strncasecmp (head, "f32le", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    f = (float)d;
                    if (!is_endian ())
                        change_order ((unsigned char *)&f, sizeof (float));
                    purc_rwstream_write (rwstream, (char *)&f, 4);
                    write_length += 4;
                }
                else if (strncasecmp (head, "f64le", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    if (!is_endian ())
                        change_order ((unsigned char *)&d, sizeof (double));
                    purc_rwstream_write (rwstream, (char *)&d, 8);
                    write_length += 8;
                }
                else if (strncasecmp (head, "f96le", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_long_double (val, &ld, false);
                    if (!is_endian ())
                        change_order ((unsigned char *)&ld,
                                sizeof (long double));
                    purc_rwstream_write (rwstream, (char *)&ld, 12);
                    write_length += 12;
                }
                else if (strncasecmp (head, "f128le", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_long_double (val, &ld, false);
                    if (!is_endian ())
                        change_order ((unsigned char *)&ld,
                                sizeof (long double));
                    purc_rwstream_write (rwstream, (char *)&ld, 16);
                    write_length += 16;
                }
                break;
            case 'u':
            case 'U':
                if (strncasecmp (head, "u8", length) == 0)
                    write_rwstream_uint (rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 1, &write_length);
                else if (strncasecmp (head, "u16", length) == 0)
                    write_rwstream_uint (rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 2, &write_length);
                else if (strncasecmp (head, "u32", length) == 0)
                    write_rwstream_uint (rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 4, &write_length);
                else if (strncasecmp (head, "u64", length) == 0) {
                    write_rwstream_uint (rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 8, &write_length);
                }
                else if (strncasecmp (head, "u16le", length) == 0)
                    write_rwstream_uint (rwstream,
                            argv[2], &i, ENDIAN_LITTLE, 2, &write_length);
                else if (strncasecmp (head, "u32le", length) == 0)
                    write_rwstream_uint (rwstream,
                            argv[2], &i, ENDIAN_LITTLE, 4, &write_length);
                else if (strncasecmp (head, "u64le", length) == 0)
                    write_rwstream_uint (rwstream,
                            argv[2], &i, ENDIAN_LITTLE, 8, &write_length);
                else if (strncasecmp (head, "u16be", length) == 0)
                    write_rwstream_uint (rwstream,
                            argv[2], &i, ENDIAN_BIG, 2, &write_length);
                else if (strncasecmp (head, "u32be", length) == 0)
                    write_rwstream_uint (rwstream,
                            argv[2], &i, ENDIAN_BIG, 4, &write_length);
                else if (strncasecmp (head, "u64be", length) == 0)
                    write_rwstream_uint (rwstream,
                            argv[2], &i, ENDIAN_BIG, 8, &write_length);

                break;
            case 'b':
            case 'B':
                val = purc_variant_array_get (argv[2], i);
                i++;

                // get sequence length
                if (length > 1) {
                    strncpy ((char *)buf, head + 1, length - 1);
                    *(buf + length - 1)= 0x00;
                    write_number = atoi((char *)buf);

                    if(write_number) {
                        bsize = write_number;
                        buffer = purc_variant_get_bytes_const (val, &bsize);
                        purc_rwstream_write (rwstream, buffer, write_number);
                        write_length += write_number;
                    }
                }
                break;

            case 'p':
            case 'P':
                val = purc_variant_array_get (argv[2], i);
                i++;

                // get white space length
                if (length > 1) {
                    strncpy ((char *)buf, head + 1, length - 1);
                    *(buf + length - 1)= 0x00;
                    write_number = atoi((char *)buf);

                    if(write_number) {
                        int i = 0;
                        int times = write_number / sizeof(long double);
                        int rest = write_number % sizeof(long double);
                        ld = 0;
                        for (i = 0; i < times; i++)
                            purc_rwstream_write (rwstream,
                                    &ld, sizeof(long double));
                        purc_rwstream_write (rwstream, &ld, rest);
                        write_length += write_number;
                    }
                }
                break;

            case 's':
            case 'S':
                val = purc_variant_array_get (argv[2], i);
                i++;

                // get string length
                if (length > 1) {
                    strncpy ((char *)buf, head + 1, length - 1);
                    *(buf + length - 1)= 0x00;
                    write_number = atoi((char *)buf);
                } else {
                    write_number = purc_variant_string_length (val);
                }

                if(write_number) {
                    buffer = (unsigned char *)purc_variant_get_string_const
                        (val);
                    purc_rwstream_write (rwstream, buffer, write_number);
                    write_length += write_number;
                }
                break;
        }
        head = pcdvobjs_get_next_option (head + length + 1, " \t\n", &length);
    }

    ret_var = purc_variant_make_ulongint (write_length);
    return ret_var;
}

static purc_variant_t
stream_readlines_getter (purc_variant_t root, size_t nr_args,
        purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = NULL;
    int64_t line_num = 0;

    if (nr_args != 2) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_native (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    rwstream = purc_variant_native_get_entity (argv[0]);
    if (rwstream == NULL) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
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
        size_t pos = find_line_stream (rwstream, line_num);

        char * content = malloc (pos + 1);
        if (content == NULL) {
            return purc_variant_make_string ("", false);
        }

        purc_rwstream_seek (rwstream, 0L, SEEK_SET);
        pos = purc_rwstream_read (rwstream, content, pos);
        *(content + pos - 1) = 0x00;

        ret_var = purc_variant_make_string_reuse_buff (content, pos, false);
    }
    return ret_var;
}

static purc_variant_t
stream_readbytes_getter (purc_variant_t root, size_t nr_args,
        purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = NULL;
    uint64_t byte_num = 0;

    if (nr_args != 2) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_native (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    rwstream = purc_variant_native_get_entity (argv[0]);
    if (rwstream == NULL) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if (argv[1] != PURC_VARIANT_INVALID) {
        purc_variant_cast_to_ulongint (argv[1], &byte_num, false);
    }

    if (byte_num == 0) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        ret_var = PURC_VARIANT_INVALID;
    } else {
        char * content = malloc (byte_num);
        size_t size = 0;

        if (content == NULL) {
            purc_set_error (PURC_ERROR_OUT_OF_MEMORY);
            return PURC_VARIANT_INVALID;
        }

        size = purc_rwstream_read (rwstream, content, byte_num);
        if (size > 0)
            ret_var =
                purc_variant_make_byte_sequence_reuse_buff(content, size, size);
        else {
            free (content);
            purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
            ret_var = PURC_VARIANT_INVALID;
        }
    }

    return ret_var;
}

static purc_variant_t
stream_seek_getter (purc_variant_t root, size_t nr_args,
        purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = NULL;
    int64_t byte_num = 0;
    off_t off = 0;
    int64_t whence = 0;

    if (nr_args != 3) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_native (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    rwstream = purc_variant_native_get_entity (argv[0]);
    if (rwstream == NULL) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if (argv[1] != PURC_VARIANT_INVALID) {
        purc_variant_cast_to_longint (argv[1], &byte_num, false);
    }

    if (argv[2] != PURC_VARIANT_INVALID) {
        purc_variant_cast_to_longint (argv[2], &whence, false);
    }

    off = purc_rwstream_seek (rwstream, byte_num, (int)whence);
    ret_var = purc_variant_make_longint (off);

    return ret_var;
}

static purc_variant_t
stream_close_getter (purc_variant_t root, size_t nr_args,
        purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = NULL;
    int close = 0;

    if (nr_args != 1) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_native (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }
    rwstream = purc_variant_native_get_entity (argv[0]);
    if (rwstream == NULL) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    close = purc_rwstream_destroy (rwstream);

    if (close == 0)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t pcdvobjs_create_file (void)
{
    purc_variant_t file_text = PURC_VARIANT_INVALID;
    purc_variant_t file_bin = PURC_VARIANT_INVALID;
    purc_variant_t file_stream = PURC_VARIANT_INVALID;
    purc_variant_t file = PURC_VARIANT_INVALID;

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


    file_text = pcdvobjs_make_dvobjs (text, PCA_TABLESIZE(text));
    if (file_text == PURC_VARIANT_INVALID)
        goto error_text;


    file_bin = pcdvobjs_make_dvobjs (bin, PCA_TABLESIZE(bin));
    if (file_bin == PURC_VARIANT_INVALID)
        goto error_bin;

    file_stream = pcdvobjs_make_dvobjs (stream, PCA_TABLESIZE(stream));
    if (file_stream == PURC_VARIANT_INVALID)
        goto error_stream;

    file = purc_variant_make_object_by_static_ckey (3,
                                "text",   file_text,
                                "bin",    file_bin,
                                "stream", file_stream);

    purc_variant_unref (file_stream);
error_stream:
    purc_variant_unref (file_bin);
error_bin:
    purc_variant_unref (file_text);
error_text:


    return file;
}

// for FS
static bool remove_dir (char *dir)
{
    char dir_name[PATH_MAX];
    DIR *dirp;
    struct dirent *dp;
    struct stat dir_stat;
    bool ret = true;

    if (access(dir, F_OK | R_OK) != 0)
        return false;

    if (stat(dir, &dir_stat) < 0)
        return false;

    if (S_ISREG(dir_stat.st_mode))
        remove(dir);
    else if (S_ISDIR(dir_stat.st_mode)) {
        dirp = opendir(dir);

        while ((dp=readdir(dirp)) != NULL) {
            if ((strcmp(dp->d_name, ".") == 0)
                    || (strcmp(dp->d_name, "..") == 0))
                continue;
            sprintf(dir_name, "%s/%s", dir, dp->d_name);
            remove_dir(dir_name);
        }
        closedir(dirp);

        rmdir(dir);
    } else
        ret = false;

    return ret;
}

static purc_variant_t
list_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    char dir_name[PATH_MAX];
    char filename[PATH_MAX];
    const char *string_filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;
    const char *filter = NULL;
    struct wildcard_list *wildcard = NULL;
    struct wildcard_list *temp_wildcard = NULL;
    char au[10] = {0};
    int i = 0;

    if ((argv == NULL) || (nr_args < 1)) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strcpy (dir_name, string_filename);

    if (access(dir_name, F_OK | R_OK) != 0) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the filter
    if ((nr_args > 1) && (argv[1] != NULL) && 
            (!purc_variant_is_string (argv[1]))) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }
    if ((nr_args > 1) && (argv[1] != NULL))
        filter = purc_variant_get_string_const (argv[1]);

    // get filter array
    if (filter) {
        size_t length = 0;
        const char *head = pcdvobjs_get_next_option (filter, ";", &length);
        while (head) {
            if (wildcard == NULL) {
                wildcard = malloc (sizeof(struct wildcard_list));
                if (wildcard == NULL)
                    goto error;
                temp_wildcard = wildcard;
            } else {
                temp_wildcard->next = malloc (sizeof(struct wildcard_list));
                if (temp_wildcard->next == NULL)
                    goto error;
                temp_wildcard = temp_wildcard->next;
            }
            temp_wildcard->next = NULL;
            temp_wildcard->wildcard = malloc (length + 1);
            if (temp_wildcard->wildcard == NULL)
                goto error;
            strncpy(temp_wildcard->wildcard, head, length);
            *(temp_wildcard->wildcard + length) = 0x00;
            pcdvobjs_remove_space (temp_wildcard->wildcard);
            head = pcdvobjs_get_next_option (head + length + 1, ";", &length);
        }
    }

    // get the dirctory content
    DIR *dir = NULL;
    struct dirent *ptr = NULL;
    purc_variant_t obj_var = PURC_VARIANT_INVALID;
    struct stat file_stat;

    if ((dir = opendir (dir_name)) == NULL) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    ret_var = purc_variant_make_array (0, PURC_VARIANT_INVALID);
    while ((ptr = readdir(dir)) != NULL)
    {
        if (strcmp (ptr->d_name,".") == 0 || strcmp(ptr->d_name, "..") == 0)
            continue;

        // use filter
        temp_wildcard = wildcard;
        while (temp_wildcard) {
            if (wildcard_cmp (ptr->d_name, temp_wildcard->wildcard))
                break;
            temp_wildcard = temp_wildcard->next;
        }
        if (wildcard && (temp_wildcard == NULL))
            continue;

        obj_var = purc_variant_make_object (0, PURC_VARIANT_INVALID,
                PURC_VARIANT_INVALID);

        strcpy (filename, dir_name);
        strcat (filename, "/");
        strcat (filename, ptr->d_name);

        if (stat(filename, &file_stat) < 0)
            continue;

        // name
        val = purc_variant_make_string (ptr->d_name, false);
        purc_variant_object_set_by_static_ckey (obj_var, "name", val);
        purc_variant_unref (val);

        // dev
        val = purc_variant_make_number (file_stat.st_dev);
        purc_variant_object_set_by_static_ckey (obj_var, "dev", val);
        purc_variant_unref (val);

        // inode
        val = purc_variant_make_number (ptr->d_ino);
        purc_variant_object_set_by_static_ckey (obj_var, "inode", val);
        purc_variant_unref (val);

        // type
        if (ptr->d_type == DT_BLK) {
            val = purc_variant_make_string ("b", false);
            purc_variant_object_set_by_static_ckey (obj_var, "type", val);
            purc_variant_unref (val);
        } else if(ptr->d_type == DT_CHR) {
            val = purc_variant_make_string ("c", false);
            purc_variant_object_set_by_static_ckey (obj_var, "type", val);
            purc_variant_unref (val);
        } else if(ptr->d_type == DT_DIR) {
            val = purc_variant_make_string ("d", false);
            purc_variant_object_set_by_static_ckey (obj_var, "type", val);
            purc_variant_unref (val);
        } else if(ptr->d_type == DT_FIFO) {
            val = purc_variant_make_string ("f", false);
            purc_variant_object_set_by_static_ckey (obj_var, "type", val);
            purc_variant_unref (val);
        } else if(ptr->d_type == DT_LNK) {
            val = purc_variant_make_string ("l", false);
            purc_variant_object_set_by_static_ckey (obj_var, "type", val);
            purc_variant_unref (val);
        } else if(ptr->d_type == DT_REG) {
            val = purc_variant_make_string ("r", false);
            purc_variant_object_set_by_static_ckey (obj_var, "type", val);
            purc_variant_unref (val);
        } else if(ptr->d_type == DT_SOCK) {
            val = purc_variant_make_string ("s", false);
            purc_variant_object_set_by_static_ckey (obj_var, "type", val);
            purc_variant_unref (val);
        } else if(ptr->d_type == DT_UNKNOWN) {
            val = purc_variant_make_string ("u", false);
            purc_variant_object_set_by_static_ckey (obj_var, "type", val);
            purc_variant_unref (val);
        }

        // mode
        val = purc_variant_make_byte_sequence (&(file_stat.st_mode),
                                                    sizeof(unsigned long));
        purc_variant_object_set_by_static_ckey (obj_var, "mode", val);
        purc_variant_unref (val);

        // mode_str
        for (i = 0; i < 3; i++) {
            if ((0x01 << (8 - 3 * i)) & file_stat.st_mode)
                au[i * 3 + 0] = 'r';
            else
                au[i * 3 + 0] = '-';
            if ((0x01 << (7 - 3 * i)) & file_stat.st_mode)
                au[i * 3 + 1] = 'w';
            else
                au[i * 3 + 1] = '-';
            if ((0x01 << (6 - 3 * i)) & file_stat.st_mode)
                au[i * 3 + 2] = 'x';
            else
                au[i * 3 + 2] = '-';
        }
        val = purc_variant_make_string (au, false);
        purc_variant_object_set_by_static_ckey (obj_var, "mode_str", val);
        purc_variant_unref (val);

        // nlink
        val = purc_variant_make_number (file_stat.st_nlink);
        purc_variant_object_set_by_static_ckey (obj_var, "nlink", val);
        purc_variant_unref (val);

        // uid
        val = purc_variant_make_number (file_stat.st_uid);
        purc_variant_object_set_by_static_ckey (obj_var, "uid", val);
        purc_variant_unref (val);

        // gid
        val = purc_variant_make_number (file_stat.st_gid);
        purc_variant_object_set_by_static_ckey (obj_var, "gid", val);
        purc_variant_unref (val);

        // rdev_major 
        val = purc_variant_make_number (major(file_stat.st_dev));
        purc_variant_object_set_by_static_ckey (obj_var, "rdev_major", val);
        purc_variant_unref (val);

        // rdev_minor
        val = purc_variant_make_number (minor(file_stat.st_dev));
        purc_variant_object_set_by_static_ckey (obj_var, "rdev_minor", val);
        purc_variant_unref (val);

        // size
        val = purc_variant_make_number (file_stat.st_size);
        purc_variant_object_set_by_static_ckey (obj_var, "size", val);
        purc_variant_unref (val);

        // blksize
        val = purc_variant_make_number (file_stat.st_blksize);
        purc_variant_object_set_by_static_ckey (obj_var, "blksize", val);
        purc_variant_unref (val);

        // blocks
        val = purc_variant_make_number (file_stat.st_blocks);
        purc_variant_object_set_by_static_ckey (obj_var, "blocks", val);
        purc_variant_unref (val);

        // atime
        val = purc_variant_make_string (ctime(&file_stat.st_atime), false);
        purc_variant_object_set_by_static_ckey (obj_var, "atime", val);
        purc_variant_unref (val);

        // mtime
        val = purc_variant_make_string (ctime(&file_stat.st_mtime), false);
        purc_variant_object_set_by_static_ckey (obj_var, "mtime", val);
        purc_variant_unref (val);

        // ctime
        val = purc_variant_make_string (ctime(&file_stat.st_ctime), false);
        purc_variant_object_set_by_static_ckey (obj_var, "ctime", val);
        purc_variant_unref (val);

        purc_variant_array_append (ret_var, obj_var);
        purc_variant_unref (obj_var);
    }

    closedir(dir);

error:
    while (wildcard) {
        if (wildcard->wildcard)
            free (wildcard->wildcard);
        temp_wildcard = wildcard;
        wildcard = wildcard->next;
        free (temp_wildcard);
    }
    return ret_var;
}

static purc_variant_t
list_prt_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    enum display_order {
        DISPLAY_MODE = 1,
        DISPLAY_NLINK,
        DISPLAY_UID,
        DISPLAY_GID,
        DISPLAY_SIZE,
        DISPLAY_BLKSIZE,
        DISPLAY_ATIME,
        DISPLAY_CTIME,
        DISPLAY_MTIME,
        DISPLAY_NAME
    };
    char dir_name[PATH_MAX];
    char filename[PATH_MAX];
    const char *string_filename = NULL;
    const char *filter = NULL;
    struct wildcard_list *wildcard = NULL;
    struct wildcard_list *temp_wildcard = NULL;
    const char *mode = NULL;
    char display[10] = {0};
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_variant_t val = PURC_VARIANT_INVALID;
    char au[10] = {0};
    int i = 0;

    if ((argv == NULL) || (nr_args < 1)) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    string_filename = purc_variant_get_string_const (argv[0]);
    strcpy (dir_name, string_filename);

    if (access(dir_name, F_OK | R_OK) != 0) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the filter
    if ((nr_args > 1) && (argv[1] != NULL) &&
            (!purc_variant_is_string (argv[1]))) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        return PURC_VARIANT_INVALID;
    }
    if ((nr_args > 1) && (argv[1] != NULL))
        filter = purc_variant_get_string_const (argv[1]);

    // get filter array
    if (filter) {
        size_t length = 0;
        const char *head = pcdvobjs_get_next_option (filter, ";", &length);
        while (head) {
            if (wildcard == NULL) {
                wildcard = malloc (sizeof(struct wildcard_list));
                if (wildcard == NULL)
                    goto error;
                temp_wildcard = wildcard;
            } else {
                temp_wildcard->next = malloc (sizeof(struct wildcard_list));
                if (temp_wildcard->next == NULL)
                    goto error;
                temp_wildcard = temp_wildcard->next;
            }
            temp_wildcard->next = NULL;
            temp_wildcard->wildcard = malloc (length + 1);
            if (temp_wildcard->wildcard == NULL)
                goto error;
            strncpy(temp_wildcard->wildcard, head, length);
            *(temp_wildcard->wildcard + length) = 0x00;
            pcdvobjs_remove_space (temp_wildcard->wildcard);
            head = pcdvobjs_get_next_option (head + length + 1, ";", &length);
        }
    }

    // get the mode
    if ((nr_args > 2) && (argv[2] != NULL) && (!purc_variant_is_string (argv[2]))) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        goto error;
    }
    if ((nr_args > 2) && (argv[2] != NULL)) {
        mode = purc_variant_get_string_const (argv[2]);

        // get mode array
        i = 0;
        bool quit = false;
        size_t length = 0;
        const char * head = pcdvobjs_get_next_option (mode, " ", &length);
        while (head) {
            switch (* head)
            {
                case 'm':
                case 'M':
                    if (strncasecmp (head, "mode", length) == 0) {
                        display[i] = DISPLAY_MODE;
                        i++;
                    } else if (strncasecmp (head, "mtime", length) == 0) {
                        display[i] = DISPLAY_MTIME;
                        i++;
                    }
                    break;
                case 'n':
                case 'N':
                    if (strncasecmp (head, "nlink", length) == 0) {
                        display[i] = DISPLAY_NLINK;
                        i++;
                    } else if (strncasecmp (head, "name", length) == 0) {
                        display[i] = DISPLAY_NAME;
                        i++;
                    }
                    break;
                case 'u':
                case 'U':
                    if (strncasecmp (head, "uid", length) == 0) {
                        display[i] = DISPLAY_UID;
                        i++;
                    }
                    break;
                case 'g':
                case 'G':
                    if (strncasecmp (head, "gid", length) == 0) {
                        display[i] = DISPLAY_GID;
                        i++;
                    }
                    break;
                case 's':
                case 'S':
                    if (strncasecmp (head, "size", length) == 0) {
                        display[i] = DISPLAY_SIZE;
                        i++;
                    }
                    break;
                case 'b':
                case 'B':
                    if (strncasecmp (head, "blksize", length) == 0) {
                        display[i] = DISPLAY_BLKSIZE;
                        i++;
                    }
                    break;
                case 'a':
                case 'A':
                    if (strncasecmp (head, "atime", length) == 0) {
                        display[i] = DISPLAY_ATIME;
                        i++;
                    } else if (strncasecmp (head, "all", length) == 0) {
                        for (i = 0; i < 10; i++)
                            display[i] = i + 1;
                        quit = true;
                    }
                    break;
                case 'c':
                case 'C':
                    if (strncasecmp (head, "ctime", length) == 0) {
                        display[i] = DISPLAY_CTIME;
                        i++;
                    }
                    break;
                case 'd':
                case 'D':
                    if (strncasecmp (head, "default", length) == 0) {
                        for (i = 0; i < 10; i++)
                            display[i] = i + 1;
                        quit = true;
                    }
                    break;
            }

            if (quit)
                break;
            head = pcdvobjs_get_next_option (head + length + 1, " ", &length);
        }
    } else {
        for (i = 0; i < 10; i++)
            display[i] = i + 1;
    }

    // get the dirctory content
    DIR *dir = NULL;
    struct dirent *ptr = NULL;
    struct stat file_stat;
    char info[PATH_MAX] = {0};

    if ((dir = opendir (dir_name)) == NULL) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        goto error;
    }

    ret_var = purc_variant_make_array (0, PURC_VARIANT_INVALID);
    while ((ptr = readdir(dir)) != NULL)
    {
        if (strcmp (ptr->d_name,".") == 0 || strcmp(ptr->d_name, "..") == 0)
            continue;

        // use filter
        temp_wildcard = wildcard;
        while (temp_wildcard) {
            if (wildcard_cmp (ptr->d_name, temp_wildcard->wildcard))
                break;
            temp_wildcard = temp_wildcard->next;
        }
        if (wildcard && (temp_wildcard == NULL))
            continue;

        strcpy (filename, dir_name);
        strcat (filename, "/");
        strcat (filename, ptr->d_name);

        if (stat(filename, &file_stat) < 0)
            continue;

        for (i = 0; i < 10; i++) {
            switch (display[i]) {
                case DISPLAY_MODE:
                    // type
                    if (ptr->d_type == DT_BLK) {
                        sprintf (info + strlen (info), "b");
                    } else if(ptr->d_type == DT_CHR) {
                        sprintf (info + strlen (info), "c");
                    } else if(ptr->d_type == DT_DIR) {
                        sprintf (info + strlen (info), "d");
                    } else if(ptr->d_type == DT_FIFO) {
                        sprintf (info + strlen (info), "f");
                    } else if(ptr->d_type == DT_LNK) {
                        sprintf (info + strlen (info), "l");
                    } else if(ptr->d_type == DT_REG) {
                        sprintf (info + strlen (info), "-");
                    } else if(ptr->d_type == DT_SOCK) {
                        sprintf (info + strlen (info), "s");
                    }

                    // mode_str
                    for (i = 0; i < 3; i++) {
                        if ((0x01 << (8 - 3 * i)) & file_stat.st_mode)
                            au[i * 3 + 0] = 'r';
                        else
                            au[i * 3 + 0] = '-';
                        if ((0x01 << (7 - 3 * i)) & file_stat.st_mode)
                            au[i * 3 + 1] = 'w';
                        else
                            au[i * 3 + 1] = '-';
                        if ((0x01 << (6 - 3 * i)) & file_stat.st_mode)
                            au[i * 3 + 2] = 'x';
                        else
                            au[i * 3 + 2] = '-';
                    }
                    sprintf (info + strlen (info), "%s\t", au);
                    break;

                case DISPLAY_NLINK:
                    sprintf (info + strlen (info), "%ld\t",
                            (long)file_stat.st_nlink);
                    break;

                case DISPLAY_UID:
                    sprintf (info + strlen (info), "%ld\t",
                            (long)file_stat.st_uid);
                    break;

                case DISPLAY_GID:
                    sprintf (info + strlen (info), "%ld\t",
                            (long)file_stat.st_gid);
                    break;

                case DISPLAY_SIZE:
                    sprintf (info + strlen (info), "%lld\t",
                            (long long)file_stat.st_size);
                    break;

                case DISPLAY_BLKSIZE:
                    sprintf (info + strlen (info), "%ld\t",
                            file_stat.st_blksize);
                    break;

                case DISPLAY_ATIME:
                    sprintf (info + strlen (info), "%s\t",
                            ctime(&file_stat.st_atime));
                    break;

                case DISPLAY_CTIME:
                    sprintf (info + strlen (info), "%s\t",
                            ctime(&file_stat.st_ctime));
                    break;

                case DISPLAY_MTIME:
                    sprintf (info + strlen (info), "%s\t",
                            ctime(&file_stat.st_mtime));
                    break;

                case DISPLAY_NAME:
                    strcat (info, ptr->d_name);
                    strcat (info, "\t");
                    break;
            }
        }
        info[strlen (info) - 1] = 0x00;

        val = purc_variant_make_string (info, false);
        purc_variant_array_append (ret_var, val);
        purc_variant_unref (val);
    }

    closedir(dir);

error:
    while (wildcard) {
        if (wildcard->wildcard)
            free (wildcard->wildcard);
        temp_wildcard = wildcard;
        wildcard = wildcard->next;
        free (temp_wildcard);
    }
    return ret_var;
}

static purc_variant_t
mkdir_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args != 1)) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);

    if (mkdir (filename, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
        ret_var = purc_variant_make_boolean (false);
    else
        ret_var = purc_variant_make_boolean (true);

    return ret_var;
}


static purc_variant_t
rmdir_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;
    DIR *dirp;
    struct dirent *dp;
    struct stat dir_stat;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    bool empty = true;

    if ((argv == NULL) || (nr_args != 1)) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);

    if (access(filename, F_OK | R_OK) != 0) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return purc_variant_make_boolean (false);
    }

    if (stat(filename, &dir_stat) < 0)
        return purc_variant_make_boolean (false);

    if (S_ISDIR(dir_stat.st_mode)) {
        dirp = opendir(filename);

        while ((dp=readdir(dirp)) != NULL) {
            if ((strcmp(dp->d_name, ".") == 0) ||
                    (strcmp(dp->d_name, "..") == 0))
                continue;
            else {
                empty = false;
                break;
            }
        }
        closedir(dirp);

        if (empty) {
            if (rmdir(filename) == 0)
                empty = true;
            else
                empty = false;
        }
    }

    if (empty)
        ret_var = purc_variant_make_boolean (true);
    else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
touch_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args != 1)) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);

    // file not exist, create it
    if (access(filename, F_OK | R_OK) != 0) {
        int fd = -1;
        fd = open(filename, O_CREAT | O_WRONLY,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH |S_IWOTH);

        if (fd != -1) {
            ret_var = purc_variant_make_boolean (true);
            close (fd);
        }
        else
            ret_var = purc_variant_make_boolean (false);
    } else {      // change time
        struct timespec newtime[2];
        newtime[0].tv_nsec = UTIME_NOW;
        newtime[1].tv_nsec = UTIME_NOW;
        if (utimensat(AT_FDCWD, filename, newtime, 0) == 0) {
            ret_var = purc_variant_make_boolean (true);
        } else {
            ret_var = purc_variant_make_boolean (false);
        }
    }

    return ret_var;
}


static purc_variant_t
unlink_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;
    struct stat filestat;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args != 1)) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);

    if (access(filename, F_OK | R_OK) != 0) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return purc_variant_make_boolean (false);
    }

    if (stat(filename, &filestat) < 0)
        return purc_variant_make_boolean (false);

    if (S_ISREG(filestat.st_mode)) {
        if (unlink (filename) == 0)
            ret_var = purc_variant_make_boolean (false);
        else
            ret_var = purc_variant_make_boolean (true);
    } else
        ret_var = purc_variant_make_boolean (false);

    return ret_var;
}

static purc_variant_t
rm_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if ((argv == NULL) || (nr_args != 1)) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    if ((argv[0] != PURC_VARIANT_INVALID) &&
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_ARGS);
        return PURC_VARIANT_INVALID;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);

    if (remove_dir ((char *)filename))
        ret_var = purc_variant_make_boolean (false);
    else
        ret_var = purc_variant_make_boolean (true);

    return ret_var;

}

static purc_variant_t pcdvobjs_create_fs(void)
{
    static struct pcdvojbs_dvobjs method [] = {
        {"list",     list_getter, NULL},
        {"list_prt", list_prt_getter, NULL},
        {"mkdir",    mkdir_getter, NULL},
        {"rmdir",    rmdir_getter, NULL},
        {"touch",    touch_getter, NULL},
        {"unlink",   unlink_getter, NULL},
        {"rm",       rm_getter, NULL} };

    return pcdvobjs_make_dvobjs (method, PCA_TABLESIZE(method));
}

static struct pcdvojbs_dvobjs_object dynamic_objects [] = {
    {
        "FS",                                   // name
        "For File System Operations in PURC",   // description
        pcdvobjs_create_fs                      // create function
    },
    {
        "FILE",                                 // name
        "For File Operations in PURC",          // description
        pcdvobjs_create_file                    // create function
    }
};

purc_variant_t __purcex_load_dynamic_variant (const char *name, int *ver_code)
{
    size_t i = 0;
    for (i = 0; i < PCA_TABLESIZE(dynamic_objects); i++) {
        if (strncasecmp (name, dynamic_objects[i].name, strlen (name)) == 0)
            break;
    }

    if (i == PCA_TABLESIZE(dynamic_objects))
        return PURC_VARIANT_INVALID;
    else {
        *ver_code = FS_DVOBJ_VERSION;
        return dynamic_objects[i].create_func();
    }
}

size_t __purcex_get_number_of_dynamic_variants (void)
{
    return PCA_TABLESIZE(dynamic_objects);
}

const char * __purcex_get_dynamic_variant_name (size_t idx)
{
    if (idx >= PCA_TABLESIZE(dynamic_objects))
        return NULL;
    else
        return dynamic_objects[idx].name;
}

const char * __purcex_get_dynamic_variant_desc (size_t idx)
{
    if (idx >= PCA_TABLESIZE(dynamic_objects))
        return NULL;
    else
        return dynamic_objects[idx].description;
}
