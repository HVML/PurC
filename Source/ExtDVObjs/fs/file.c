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

#include "config.h"
#include "private/instance.h"
#include "private/errors.h"
#include "private/dvobjs.h"
#include "purc-variant.h"

#include <unistd.h>
#include <sys/stat.h>

#define BUFFER_SIZE         4096
#define ENDIAN_PLATFORM     0
#define ENDIAN_LITTLE       1
#define ENDIAN_BIG          2

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

static inline bool is_little_endian (void)
{
#if CPU(BIG_ENDIAN)
    return false;
#elif CPU(LITTLE_ENDIAN)
    return true;
#endif
}

#if 0
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
    }
    else {
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
#endif

// Scan the file and tell me how many lines there will be.
static size_t scan_lines (FILE *fp)
{
    char    buffer[BUFFER_SIZE];
    size_t  read_size = 0;
    size_t  total_line = 0;
    bool    new_line_flag = false;

    fseek (fp, 0, SEEK_SET);

    while (1) {
        read_size = fread (buffer, 1, BUFFER_SIZE, fp);
        if (read_size == 0)
            break;

        size_t i;
        for (i = 0; i < read_size; i++) {
            if (buffer[i] == '\n') {
                total_line ++;
                new_line_flag = false;
            }
            else {
                new_line_flag = true;
            }
        }

        if (read_size < BUFFER_SIZE) // No more content.
            break;
    }

    if (new_line_flag)
        total_line ++;

    fseek (fp, 0, SEEK_SET);
    return total_line;
}

// line_num == 0: Read all lines.
// line_num  > 0: Read the first line_num lines.
// line_num  < 0: Skip the first line_num lines and read the remaining lines.
static purc_variant_t read_lines (FILE *fp, ssize_t line_num)
{
    char    buffer[BUFFER_SIZE];
    size_t  read_size = 0;
    char   *content = NULL;
    size_t  content_len = 0;
    purc_variant_t val = PURC_VARIANT_INVALID;
    purc_variant_t ret_var = purc_variant_make_array (0, PURC_VARIANT_INVALID);

    while (1) {
        read_size = fread (buffer, 1, BUFFER_SIZE, fp);
        if (read_size == 0)
            break;

        size_t i;
        size_t buffer_line_start = 0;
        size_t buffer_line_end = 0;
        for (i = 0; i < read_size; i++) {
            if (buffer[i] == '\n')
            {
                if (line_num < 0) {
                    // Skip the first line_num lines.
                    line_num ++;
                }
                else {
                    buffer_line_end = i - 1;
                    
                    if (content_len > 0) {
                        content = realloc (content,
                                content_len + buffer_line_end - buffer_line_start + 1);
                        memcpy (content + content_len,
                                buffer + buffer_line_start,
                                buffer_line_end - buffer_line_start);
                        content_len += (buffer_line_end - buffer_line_start);
                        content[content_len] = 0x0;
                        if (content[content_len - 1] == '\r')
                            content[content_len - 1] = 0x0;

                        val = purc_variant_make_string_ex (content, content_len, false);
                        purc_variant_array_append (ret_var, val);
                        purc_variant_unref (val);

                        free (content);
                        content = NULL;
                        content_len = 0;
                    }
                    else {
                        val = purc_variant_make_string_ex (buffer + buffer_line_start,
                                buffer_line_end - buffer_line_start, false);
                        purc_variant_array_append (ret_var, val);
                        purc_variant_unref (val);
                    }

                    if (line_num > 0) {
                        // Read the first line_num lines.
                        line_num --;
                        if (line_num == 0)
                            goto out;
                    }
                }

                buffer_line_start = i + 1;
            }
        }

        if (i > buffer_line_start) {
            content = realloc (content,
                    content_len + read_size - buffer_line_start + 1);
            memcpy (content + content_len,
                    buffer + buffer_line_start,
                    read_size - buffer_line_start);
            content_len += (read_size - buffer_line_start);
            content[content_len] = 0x0;
        }

        if (read_size < BUFFER_SIZE) // No more content.
        {
            if (content && line_num >= 0) {
                val = purc_variant_make_string_ex (content, content_len, false);
                purc_variant_array_append (ret_var, val);
                purc_variant_unref (val);

                free (content);
                content = NULL;
                content_len = 0;
            }

            break;
        }
    }

out:
    if (content)
        free (content);

    return ret_var;
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
text_head_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    int64_t     line_num = 0;
    const char *filename = NULL;
    FILE       *fp = NULL;
    //purc_variant_t val;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }
    // get the file name
    filename = purc_variant_get_string_const (argv[0]);
    if (filename == NULL) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (nr_args > 1) {
        if (! purc_variant_cast_to_longint (argv[1], &line_num, false)) {
            purc_set_error (PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    fp = fopen (filename, "r");
    if (fp == NULL) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        goto failed;
    }

    if (line_num >= 0) {
        // ==0: Read all lines.
        // > 0: Read the first line_num lines.
        ret_var = read_lines (fp, line_num);
    }
    else {
        // line_num < 0: Read all but the last (-line_num) lines.

        // Scan the file and tell me how many lines there will be.
        size_t total_line = scan_lines (fp);

        line_num = total_line + line_num;// line_num is NEGATIVE !

        if (line_num <= 0)
            ret_var = purc_variant_make_array (0, PURC_VARIANT_INVALID);
        else
            ret_var = read_lines (fp, line_num);
    }

    fclose (fp);
    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
text_tail_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    int64_t     line_num = 0;
    const char *filename = NULL;
    FILE       *fp = NULL;
    //purc_variant_t val;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args < 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }
    // get the file name
    filename = purc_variant_get_string_const (argv[0]);
    if (filename == NULL) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (nr_args > 1) {
        if (! purc_variant_cast_to_longint (argv[1], &line_num, false)) {
            purc_set_error (PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    fp = fopen (filename, "r");
    if (fp == NULL) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        goto failed;
    }

    if (line_num <= 0) {
        // ==0: Read all lines.
        // < 0: Skip the first line_num lines and read the remaining lines.
        ret_var = read_lines (fp, line_num);
    }
    else {
        // line_num > 0: Read the last line_num lines.

        // Scan the file and tell me how many lines there will be.
        size_t total_line = scan_lines (fp);

        line_num = total_line - line_num;

        if (line_num <= 0)
            ret_var = purc_variant_make_array (0, PURC_VARIANT_INVALID);
        else
            // Skip line_num lines.
            ret_var = read_lines (fp, -line_num);
    }

    fclose (fp);
    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
bin_head_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    int64_t byte_num = 0;
    const char *filename = NULL;
    FILE *fp = NULL;
    size_t pos = 0;
    struct stat filestat;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args != 2) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);

    // check whether the file exists
    if((access(filename, F_OK | R_OK)) != 0) {
        purc_set_error (PURC_ERROR_NOT_EXISTS);
        goto failed;
    }

    // get the file length
    if(stat(filename, &filestat) < 0) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        goto failed;
    }
    if (filestat.st_size == 0) {
        goto empty;
    }

    if (argv[1] != PURC_VARIANT_INVALID)
        purc_variant_cast_to_longint (argv[1], &byte_num, false);

    fp = fopen (filename, "r");
    if (fp == NULL) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        goto failed;
    }

    if (byte_num == 0)
        pos = filestat.st_size;
    else if (byte_num > 0)
        pos = byte_num;
    else {
        if ((-1 * byte_num) > filestat.st_size) {
            purc_set_error (PURC_ERROR_INTERNAL_FAILURE);
            goto failed;
        }
        else
            pos = filestat.st_size + byte_num;
    }

    char *content = malloc (pos);
    if (content == NULL) {
        fclose (fp);
        goto failed;
    }

    fseek (fp, 0L, SEEK_SET);
    pos = fread (content, 1, pos, fp);

    ret_var = purc_variant_make_byte_sequence_reuse_buff (content, pos, pos);

    fclose (fp);
    return ret_var;

empty:
    return purc_variant_make_byte_sequence_empty();

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
bin_tail_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    int64_t byte_num = 0;
    const char *filename = NULL;
    FILE *fp = NULL;
    size_t pos = 0;
    struct stat filestat;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;

    if (nr_args != 2) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);

    // check whether the file exists
    if((access(filename, F_OK | R_OK)) != 0) {
        purc_set_error (PURC_ERROR_NOT_EXISTS);
        goto failed;
    }

    // get the file length
    if(stat(filename, &filestat) < 0) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        goto failed;
    }
    if (filestat.st_size == 0) {
        goto empty;
    }

    if (argv[1] != NULL)
        purc_variant_cast_to_longint (argv[1], &byte_num, false);

    fp = fopen (filename, "r");
    if (fp == NULL) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        goto failed;
    }

    if (byte_num == 0)
        pos = filestat.st_size;
    else if (byte_num > 0)
        pos = byte_num;
    else {
        if ((-1 * byte_num) > filestat.st_size) {
            purc_set_error (PURC_ERROR_INTERNAL_FAILURE);
            goto failed;
        }
        else
            pos = filestat.st_size + byte_num;
    }

    fseek (fp, filestat.st_size - pos, SEEK_SET);

    char *content = malloc (pos);
    if (content == NULL) {
        fclose (fp);
        purc_set_error (PURC_ERROR_OUT_OF_MEMORY);
        goto failed;
    }

    pos = fread (content, 1, pos, fp);

    ret_var = purc_variant_make_byte_sequence_reuse_buff (content, pos, pos);

    fclose (fp);
    return ret_var;

empty:
    return purc_variant_make_byte_sequence_empty();

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    
    return PURC_VARIANT_INVALID;
}

static void
release_rwstream(void *native_entity)
{
    purc_rwstream_destroy((purc_rwstream_t)native_entity);
}

static purc_variant_t
stream_open_getter (purc_variant_t root, size_t nr_args, purc_variant_t *argv,
        unsigned call_flags)
{
    UNUSED_PARAM(root);

    const char *filename = NULL;
    struct stat filestat;
    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = NULL;

    if (nr_args != 2) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (argv[1] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string (argv[1]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    // get the file name
    filename = purc_variant_get_string_const (argv[0]);
    // check whether the file exists
    if((access(filename, F_OK | R_OK)) != 0) {
        purc_set_error (PURC_ERROR_NOT_EXISTS);
        goto failed;
    }

    // get the file length
    if(stat(filename, &filestat) < 0) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        goto failed;
    }

    rwstream = purc_rwstream_new_from_file (filename,
            purc_variant_get_string_const (argv[1]));

    if (rwstream == NULL) {
        purc_set_error (PURC_ERROR_BAD_SYSTEM_CALL);
        goto failed;
    }

    // setup a callback for `on_release` to destroy the stream automatically
    static const struct purc_native_ops ops = {
        .on_release = release_rwstream,
    };
    ret_var = purc_variant_make_native (rwstream, &ops);

    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    
    return PURC_VARIANT_INVALID;
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
            if (!is_little_endian ())
                change_order (buf, bytes);
            break;
        case ENDIAN_BIG:
            if (is_little_endian ())
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
    float f = 0.0F;
    double d = 0.0;
    long double ld = 0.0L;
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
            if (!is_little_endian ())
                change_order (buf, bytes);
            break;
        case ENDIAN_BIG:
            if (is_little_endian ())
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
        purc_variant_t *argv, unsigned call_flags)
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
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_native (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }
    rwstream = purc_variant_native_get_entity (argv[0]);
    if (rwstream == NULL) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (argv[1] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string (argv[1]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }
    format = purc_variant_get_string_const (argv[1]);
    head = pcutils_get_next_token (format, " \t\n", &length);

    ret_var = purc_variant_make_array (0, PURC_VARIANT_INVALID);

    while (head) {
        switch (* head)
        {
            case 'i':
            case 'I':
                *((int64_t *)buf) = 0;
                if (pcutils_strncasecmp (head, "i8", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_PLATFORM, 1);
                else if (pcutils_strncasecmp (head, "i16", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_PLATFORM, 2);
                else if (pcutils_strncasecmp (head, "i32", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_PLATFORM, 4);
                else if (pcutils_strncasecmp (head, "i64", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_PLATFORM, 8);
                else if (pcutils_strncasecmp (head, "i16le", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_LITTLE, 2);
                else if (pcutils_strncasecmp (head, "i32le", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_LITTLE, 4);
                else if (pcutils_strncasecmp (head, "i64le", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_LITTLE, 8);
                else if (pcutils_strncasecmp (head, "i16be", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_BIG, 2);
                else if (pcutils_strncasecmp (head, "i32be", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_BIG, 4);
                else if (pcutils_strncasecmp (head, "i64be", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_BIG, 8);

                i64 = (int64_t)(*((int64_t *)buf));
                val = purc_variant_make_longint (i64);
                break;
            case 'f':
            case 'F':
                *((float *)buf) = 0;
                if (pcutils_strncasecmp (head, "f16", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_PLATFORM, 2);
                else if (pcutils_strncasecmp (head, "f32", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_PLATFORM, 4);
                else if (pcutils_strncasecmp (head, "f64", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_PLATFORM, 8);
                else if (pcutils_strncasecmp (head, "f96", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_PLATFORM, 12);
                else if (pcutils_strncasecmp (head, "f128", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_PLATFORM, 16);

                else if (pcutils_strncasecmp (head, "f16le", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_LITTLE, 2);
                else if (pcutils_strncasecmp (head, "f32le", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_LITTLE, 4);
                else if (pcutils_strncasecmp (head, "f64le", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_LITTLE, 8);
                else if (pcutils_strncasecmp (head, "f96le", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_LITTLE, 12);
                else if (pcutils_strncasecmp (head, "f128le", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_LITTLE, 16);

                else if (pcutils_strncasecmp (head, "f16be", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_BIG, 2);
                else if (pcutils_strncasecmp (head, "f32be", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_BIG, 4);
                else if (pcutils_strncasecmp (head, "f64be", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_BIG, 8);
                else if (pcutils_strncasecmp (head, "f96be", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_BIG, 12);
                else if (pcutils_strncasecmp (head, "f128be", length) == 0)
                    val = read_rwstream_float (rwstream, ENDIAN_BIG, 16);
                break;
            case 'u':
            case 'U':
                *((uint64_t *)buf) = 0;
                if (pcutils_strncasecmp (head, "u8", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_PLATFORM, 1);
                else if (pcutils_strncasecmp (head, "u16", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_PLATFORM, 2);
                else if (pcutils_strncasecmp (head, "u32", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_PLATFORM, 4);
                else if (pcutils_strncasecmp (head, "u64", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_PLATFORM, 8);
                else if (pcutils_strncasecmp (head, "u16le", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_LITTLE, 2);
                else if (pcutils_strncasecmp (head, "u32le", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_LITTLE, 4);
                else if (pcutils_strncasecmp (head, "u64le", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_LITTLE, 8);
                else if (pcutils_strncasecmp (head, "u16be", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_BIG, 2);
                else if (pcutils_strncasecmp (head, "u32be", length) == 0)
                    read_rwstream (rwstream, buf, ENDIAN_BIG, 4);
                else if (pcutils_strncasecmp (head, "u64be", length) == 0)
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
                    }
                    else
                        val = purc_variant_make_null();
                }
                else
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
                                    (char *)buffer, read_number, false);
                        }
                    }
                    else
                        val = purc_variant_make_string ("", false);
                }
                else {
                    int i = 0;
                    int j = 0;
                    size_t mem_size = BUFFER_SIZE;

                    buffer = malloc (mem_size);
                    for (i = 0, j = 0; ; i++, j++) {
                        ssize_t r = purc_rwstream_read (rwstream, buffer + i, 1);
                        if (r <= 0)
                            break;
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
        head = pcutils_get_next_token (head + length, " \t\n", &length);
    }
    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    
    return PURC_VARIANT_INVALID;
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
            if (!is_little_endian ())
                change_order ((unsigned char *)&i64, sizeof (int64_t));
            break;
        case ENDIAN_BIG:
            if (is_little_endian ())
                change_order ((unsigned char *)&i64, sizeof (int64_t));
            break;
    }

    if (is_little_endian ())
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
            if (!is_little_endian ())
                change_order ((unsigned char *)&ret, 2);
            break;
        case ENDIAN_BIG:
            if (is_little_endian ())
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
            if (!is_little_endian ())
                change_order ((unsigned char *)&u64, sizeof (uint64_t));
            break;
        case ENDIAN_BIG:
            if (is_little_endian ())
                change_order ((unsigned char *)&u64, sizeof (uint64_t));
            break;
    }

    if (is_little_endian ())
        purc_rwstream_write (rwstream, (char *)&u64, bytes);
    else
        purc_rwstream_write (rwstream,
                (char *)&u64 + sizeof(uint64_t) - bytes, bytes);
    *length += bytes;
}

static purc_variant_t
stream_writestruct_getter (purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, unsigned call_flags)
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
    float f = 0.0F;
    double d = 0.0;
    long double ld = 0.0L;
    int write_number = 0;
    int i = 0;
    size_t bsize = 0;
    unsigned short ui16 = 0;
    size_t write_length = 0;

    if (nr_args != 3) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_native (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }
    rwstream = purc_variant_native_get_entity (argv[0]);
    if (rwstream == NULL) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (argv[1] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_string (argv[1]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    if (argv[2] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_array (argv[2]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }

    format = purc_variant_get_string_const (argv[1]);
    head = pcutils_get_next_token (format, " \t\n", &length);

    while (head) {
        switch (* head)
        {
            case 'i':
            case 'I':
                if (pcutils_strncasecmp (head, "i8", length) == 0)
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 1, &write_length);
                else if (pcutils_strncasecmp (head, "i16", length) == 0)
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 2, &write_length);
                else if (pcutils_strncasecmp (head, "i32", length) == 0)
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 4, &write_length);
                else if (pcutils_strncasecmp (head, "i64", length) == 0)
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 8, &write_length);
                else if (pcutils_strncasecmp (head, "i16le", length) == 0)
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_LITTLE, 2, &write_length);
                else if (pcutils_strncasecmp (head, "i32le", length) == 0)
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_LITTLE, 4, &write_length);
                else if (pcutils_strncasecmp (head, "i64le", length) == 0)
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_LITTLE, 8, &write_length);
                else if (pcutils_strncasecmp (head, "i16be", length) == 0)
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_BIG, 2, &write_length);
                else if (pcutils_strncasecmp (head, "i32be", length) == 0)
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_BIG, 4, &write_length);
                else if (pcutils_strncasecmp (head, "i64be", length) == 0)
                    write_rwstream_int (rwstream,
                            argv[2], &i, ENDIAN_BIG, 8, &write_length);
                break;
            case 'f':
            case 'F':
                if (pcutils_strncasecmp (head, "f16", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    ui16 = write_double_to_16 (d, ENDIAN_PLATFORM);
                    purc_rwstream_write (rwstream, &ui16, 2);
                    write_length += 2;
                }
                else if (pcutils_strncasecmp (head, "f32", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    f = (float)d;
                    purc_rwstream_write (rwstream, (char *)&f, 4);
                    write_length += 4;
                }
                else if (pcutils_strncasecmp (head, "f64", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    purc_rwstream_write (rwstream, (char *)&d, 8);
                    write_length += 8;
                }
                else if (pcutils_strncasecmp (head, "f96", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_longdouble (val, &ld, false);
                    purc_rwstream_write (rwstream, (char *)&ld, 12);
                    write_length += 12;
                }
                else if (pcutils_strncasecmp (head, "f128", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_longdouble (val, &ld, false);
                    purc_rwstream_write (rwstream, (char *)&ld, 16);
                    write_length += 16;
                }
                else if (pcutils_strncasecmp (head, "f16be", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    ui16 = write_double_to_16 (d, ENDIAN_BIG);
                    purc_rwstream_write (rwstream, &ui16, 2);
                    write_length += 2;
                }
                else if (pcutils_strncasecmp (head, "f32be", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    f = (float)d;
                    if (is_little_endian ())
                        change_order ((unsigned char *)&f, sizeof (float));
                    purc_rwstream_write (rwstream, (char *)&f, 4);
                    write_length += 4;
                }
                else if (pcutils_strncasecmp (head, "f64be", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    if (is_little_endian ())
                        change_order ((unsigned char *)&d, sizeof (double));
                    purc_rwstream_write (rwstream, (char *)&d, 8);
                    write_length += 8;
                }
                else if (pcutils_strncasecmp (head, "f96be", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_longdouble (val, &ld, false);
                    if (is_little_endian ())
                        change_order ((unsigned char *)&ld,
                                sizeof (long double));
                    purc_rwstream_write (rwstream, (char *)&ld, 12);
                    write_length += 12;
                }
                else if (pcutils_strncasecmp (head, "f128be", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_longdouble (val, &ld, false);
                    if (is_little_endian ())
                        change_order ((unsigned char *)&ld,
                                sizeof (long double));
                    purc_rwstream_write (rwstream, (char *)&ld, 16);
                    write_length += 16;
                }
                else if (pcutils_strncasecmp (head, "f16le", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    ui16 = write_double_to_16 (d, ENDIAN_LITTLE);
                    purc_rwstream_write (rwstream, &ui16, 2);
                    write_length += 2;
                }
                else if (pcutils_strncasecmp (head, "f32le", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    f = (float)d;
                    if (!is_little_endian ())
                        change_order ((unsigned char *)&f, sizeof (float));
                    purc_rwstream_write (rwstream, (char *)&f, 4);
                    write_length += 4;
                }
                else if (pcutils_strncasecmp (head, "f64le", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_number (val, &d, false);
                    if (!is_little_endian ())
                        change_order ((unsigned char *)&d, sizeof (double));
                    purc_rwstream_write (rwstream, (char *)&d, 8);
                    write_length += 8;
                }
                else if (pcutils_strncasecmp (head, "f96le", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_longdouble (val, &ld, false);
                    if (!is_little_endian ())
                        change_order ((unsigned char *)&ld,
                                sizeof (long double));
                    purc_rwstream_write (rwstream, (char *)&ld, 12);
                    write_length += 12;
                }
                else if (pcutils_strncasecmp (head, "f128le", length) == 0) {
                    val = purc_variant_array_get (argv[2], i);
                    i++;
                    purc_variant_cast_to_longdouble (val, &ld, false);
                    if (!is_little_endian ())
                        change_order ((unsigned char *)&ld,
                                sizeof (long double));
                    purc_rwstream_write (rwstream, (char *)&ld, 16);
                    write_length += 16;
                }
                break;
            case 'u':
            case 'U':
                if (pcutils_strncasecmp (head, "u8", length) == 0)
                    write_rwstream_uint (rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 1, &write_length);
                else if (pcutils_strncasecmp (head, "u16", length) == 0)
                    write_rwstream_uint (rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 2, &write_length);
                else if (pcutils_strncasecmp (head, "u32", length) == 0)
                    write_rwstream_uint (rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 4, &write_length);
                else if (pcutils_strncasecmp (head, "u64", length) == 0) {
                    write_rwstream_uint (rwstream,
                            argv[2], &i, ENDIAN_PLATFORM, 8, &write_length);
                }
                else if (pcutils_strncasecmp (head, "u16le", length) == 0)
                    write_rwstream_uint (rwstream,
                            argv[2], &i, ENDIAN_LITTLE, 2, &write_length);
                else if (pcutils_strncasecmp (head, "u32le", length) == 0)
                    write_rwstream_uint (rwstream,
                            argv[2], &i, ENDIAN_LITTLE, 4, &write_length);
                else if (pcutils_strncasecmp (head, "u64le", length) == 0)
                    write_rwstream_uint (rwstream,
                            argv[2], &i, ENDIAN_LITTLE, 8, &write_length);
                else if (pcutils_strncasecmp (head, "u16be", length) == 0)
                    write_rwstream_uint (rwstream,
                            argv[2], &i, ENDIAN_BIG, 2, &write_length);
                else if (pcutils_strncasecmp (head, "u32be", length) == 0)
                    write_rwstream_uint (rwstream,
                            argv[2], &i, ENDIAN_BIG, 4, &write_length);
                else if (pcutils_strncasecmp (head, "u64be", length) == 0)
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
                    write_number = purc_variant_string_size (val);
                }

                if(write_number) {
                    buffer = (unsigned char *)purc_variant_get_string_const
                        (val);
                    purc_rwstream_write (rwstream, buffer, write_number);
                    write_length += write_number;
                }
                break;
        }
        head = pcutils_get_next_token (head + length, " \t\n", &length);
    }

    ret_var = purc_variant_make_ulongint (write_length);
    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
stream_readlines_getter (purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = NULL;
    int64_t line_num = 0;

    if (nr_args != 2) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_native (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }
    rwstream = purc_variant_native_get_entity (argv[0]);
    if (rwstream == NULL) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        goto failed;
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
            purc_set_error (PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }

        purc_rwstream_seek (rwstream, 0L, SEEK_SET);
        pos = purc_rwstream_read (rwstream, content, pos);
        *(content + pos - 1) = 0x00;

        ret_var = purc_variant_make_string_reuse_buff (content, pos, false);
    }
    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
stream_readbytes_getter (purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = NULL;
    uint64_t byte_num = 0;

    if (nr_args != 2) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }
    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_native (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }
    rwstream = purc_variant_native_get_entity (argv[0]);
    if (rwstream == NULL) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        goto failed;
    }

    if (argv[1] != PURC_VARIANT_INVALID) {
        purc_variant_cast_to_ulongint (argv[1], &byte_num, false);
    }

    if (byte_num == 0) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }
    else {
        char * content = malloc (byte_num);
        size_t size = 0;

        if (content == NULL) {
            purc_set_error (PURC_ERROR_OUT_OF_MEMORY);
            goto failed;
        }

        size = purc_rwstream_read (rwstream, content, byte_num);
        if (size > 0)
            ret_var =
                purc_variant_make_byte_sequence_reuse_buff(content, size, size);
        else {
            free (content);
            purc_set_error (PURC_ERROR_INVALID_VALUE);
            goto failed;
        }
    }

    return ret_var;

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    
    return PURC_VARIANT_INVALID;
}

static purc_variant_t
stream_seek_getter (purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = NULL;
    int64_t byte_num = 0;
    off_t off = 0;
    int64_t whence = 0;

    if (nr_args != 3) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        goto failed;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_native (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
        goto failed;
    }
    rwstream = purc_variant_native_get_entity (argv[0]);
    if (rwstream == NULL) {
        purc_set_error (PURC_ERROR_INVALID_VALUE);
        goto failed;
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

failed:
    if (call_flags & PCVRT_CALL_FLAG_SILENTLY)
        return purc_variant_make_boolean(false);
    
    return PURC_VARIANT_INVALID;
}

#if 0   // we do not need close method, the rwstream will be destroyed
        // when the variant is released.
static purc_variant_t
stream_close_getter (purc_variant_t root, size_t nr_args,
        purc_variant_t *argv, unsigned call_flags)
{
    UNUSED_PARAM(root);
    UNUSED_PARAM(call_flags);

    purc_variant_t ret_var = PURC_VARIANT_INVALID;
    purc_rwstream_t rwstream = NULL;
    int close = 0;

    if (nr_args != 1) {
        purc_set_error (PURC_ERROR_ARGUMENT_MISSED);
        return PURC_VARIANT_INVALID;
    }

    if (argv[0] == PURC_VARIANT_INVALID ||
            (!purc_variant_is_native (argv[0]))) {
        purc_set_error (PURC_ERROR_WRONG_DATA_TYPE);
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
#endif

purc_variant_t pcdvobjs_create_file (void)
{
    purc_variant_t file_text = PURC_VARIANT_INVALID;
    purc_variant_t file_bin = PURC_VARIANT_INVALID;
    purc_variant_t file_stream = PURC_VARIANT_INVALID;
    purc_variant_t file = PURC_VARIANT_INVALID;

    static struct purc_dvobj_method text [] = {
        {"head",     text_head_getter, NULL},
        {"tail",     text_tail_getter, NULL} };

    static struct purc_dvobj_method  bin[] = {
        {"head",     bin_head_getter, NULL},
        {"tail",     bin_tail_getter, NULL} };

    static struct purc_dvobj_method  stream[] = {
        {"open",        stream_open_getter,        NULL},
        {"readstruct",  stream_readstruct_getter,  NULL},
        {"writestruct", stream_writestruct_getter, NULL},
        {"readlines",   stream_readlines_getter,   NULL},
        {"readbytes",   stream_readbytes_getter,   NULL},
        {"seek",        stream_seek_getter,        NULL},
        // {"close",       stream_close_getter,       NULL},
    };


    file_text = purc_dvobj_make_from_methods (text, PCA_TABLESIZE(text));
    if (file_text == PURC_VARIANT_INVALID)
        goto error_text;


    file_bin = purc_dvobj_make_from_methods (bin, PCA_TABLESIZE(bin));
    if (file_bin == PURC_VARIANT_INVALID)
        goto error_bin;

    file_stream = purc_dvobj_make_from_methods (stream, PCA_TABLESIZE(stream));
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
