/*
 * @file serializer.c
 * @author Vincent Wei
 * @date 2021/07/12
 * @brief The serializer of variant.
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

#include "purc-variant.h"
#include "purc-rwstream.h"
#include "private/variant.h"
#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *hex_chars = "0123456789abcdefABCDEF";

#define MY_WRITE(rws, buff, count)                              \
    do {                                                        \
        ssize_t n;                                              \
        n = purc_rwstream_write((rws), (buff), (count));        \
        if (n < 0)                                              \
            goto failed;                                        \
        nr_written += n;                                        \
        if ((size_t)n < count)                                          \
            goto partial;                                       \
    } while (0)

static ssize_t
serialize_string(purc_rwstream_t rws, const char* str,
        size_t len, unsigned int flags)
{
    int nr_written = 0;
    size_t pos = 0, start_offset = 0;
    unsigned char c;
    char buff[3];

    while (len--) {
        c = str[pos];
        switch (c) {
        case '\b':
        case '\n':
        case '\r':
        case '\t':
        case '\f':
        case '"':
        case '\\':
        case '/':
            if ((flags & PCVARIANT_SERIALIZE_OPT_NOSLASHESCAPE) && c == '/') {
                pos++;
                break;
            }

            if (pos - start_offset > 0)
                MY_WRITE(rws, str + start_offset, pos - start_offset);

            buff[0] = 0;
            if (c == '\b') {
                buff[0] = '\\';
                buff[1] = 'b';
            }
            else if (c == '\n') {
                buff[0] = '\\';
                buff[1] = 'n';
            }
            else if (c == '\r') {
                buff[0] = '\\';
                buff[1] = 'r';
            }
            else if (c == '\t') {
                buff[0] = '\\';
                buff[1] = 't';
            }
            else if (c == '\f') {
                buff[0] = '\\';
                buff[1] = 'f';
            }
            else if (c == '"') {
                buff[0] = '\\';
                buff[1] = '\"';
            }
            else if (c == '\\') {
                buff[0] = '\\';
                buff[1] = '\\';
            }
            else if (c == '/') {
                buff[0] = '\\';
                buff[1] = '/';
            }

            if (buff[0])
                MY_WRITE(rws, buff, 2);

            start_offset = ++pos;
            break;
        default:
            if (c < ' ')
            {
                char sbuf[7];
                if (pos - start_offset > 0)
                    MY_WRITE(rws, str + start_offset, pos - start_offset);
                snprintf(sbuf, sizeof(sbuf), "\\u00%c%c", hex_chars[c >> 4],
                         hex_chars[c & 0xf]);
                MY_WRITE(rws, sbuf, sizeof(sbuf) - 1);
                start_offset = ++pos;
            }
            else
                pos++;
        }
    }
    if (pos - start_offset > 0)
        MY_WRITE(rws, str + start_offset, pos - start_offset);

    return nr_written;

failed:
    return -1;
partial:
    // TODO:set an error code?
    return nr_written;
}

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char base64_pad = '=';

/* (From RFC1521 and draft-ietf-dnssec-secext-03.txt)
   The following encoding technique is taken from RFC 1521 by Borenstein
   and Freed.  It is reproduced here in a slightly edited form for
   convenience.

   A 65-character subset of US-ASCII is used, enabling 6 bits to be
   represented per printable character. (The extra 65th character, "=",
   is used to signify a special processing function.)

   The encoding process represents 24-bit groups of input bits as output
   strings of 4 encoded characters. Proceeding from left to right, a
   24-bit input group is formed by concatenating 3 8-bit input groups.
   These 24 bits are then treated as 4 concatenated 6-bit groups, each
   of which is translated into a single digit in the base64 alphabet.

   Each 6-bit group is used as an index into an array of 64 printable
   characters. The character referenced by the index is placed in the
   output string.

                         Table 1: The base64_chars Alphabet

      Value Encoding  Value Encoding  Value Encoding  Value Encoding
          0 A            17 R            34 i            51 z
          1 B            18 S            35 j            52 0
          2 C            19 T            36 k            53 1
          3 D            20 U            37 l            54 2
          4 E            21 V            38 m            55 3
          5 F            22 W            39 n            56 4
          6 G            23 X            40 o            57 5
          7 H            24 Y            41 p            58 6
          8 I            25 Z            42 q            59 7
          9 J            26 a            43 r            60 8
         10 K            27 b            44 s            61 9
         11 L            28 c            45 t            62 +
         12 M            29 d            46 u            63 /
         13 N            30 e            47 v
         14 O            31 f            48 w         (pad) =
         15 P            32 g            49 x
         16 Q            33 h            50 y

   Special processing is performed if fewer than 24 bits are available
   at the end of the data being encoded.  A full encoding quantum is
   always completed at the end of a quantity.  When fewer than 24 input
   bits are available in an input group, zero bits are added (on the
   right) to form an integral number of 6-bit groups.  Padding at the
   end of the data is performed using the '=' character.

   Since all base64 input is an integral number of octets, only the
         -------------------------------------------------
   following cases can arise:

       (1) the final quantum of encoding input is an integral
           multiple of 24 bits; here, the final unit of encoded
       output will be an integral multiple of 4 characters
       with no "=" padding,
       (2) the final quantum of encoding input is exactly 8 bits;
           here, the final unit of encoded output will be two
       characters followed by two "=" padding characters, or
       (3) the final quantum of encoding input is exactly 16 bits;
           here, the final unit of encoded output will be three
       characters followed by one "=" padding character.
   */

static ssize_t serialize_bsequence_base64(purc_rwstream_t rws,
        const void *_src, size_t srclength)
{
    const unsigned char *src = _src;
    ssize_t nr_written = 0;
    uint8_t input[3] = {0};
    uint8_t output[4];
    char buff[4];
    size_t i;

    while (2 < srclength) {
        input[0] = *src++;
        input[1] = *src++;
        input[2] = *src++;
        srclength -= 3;

        output[0] = input[0] >> 2;
        output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
        output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
        output[3] = input[2] & 0x3f;

        buff[0] = base64_chars[output[0]];
        buff[1] = base64_chars[output[1]];
        buff[2] = base64_chars[output[2]];
        buff[3] = base64_chars[output[3]];

        MY_WRITE(rws, buff, 4);
    }

    /* Now we worry about padding. */
    if (0 != srclength) {
        /* Get what's left. */
        input[0] = input[1] = input[2] = '\0';
        for (i = 0; i < srclength; i++)
            input[i] = *src++;

        output[0] = input[0] >> 2;
        output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
        output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);

        buff[0] = base64_chars[output[0]];
        buff[1] = base64_chars[output[1]];
        if (srclength == 1)
            buff[2] = base64_pad;
        else
            buff[2] = base64_chars[output[2]];
        buff[3] = base64_pad;

        MY_WRITE(rws, buff, 4);
    }

    return nr_written;

failed:
    return -1;
partial:
    // TODO:set an error code?
    return nr_written;
}

static ssize_t
serialize_bsequence(purc_rwstream_t rws, const char* content,
        size_t sz_content, unsigned int flags)
{
    ssize_t nr_written = 0;
    size_t i;

    switch (flags & PCVARIANT_SERIALIZE_OPT_BSEQUECE_MASK) {
        case PCVARIANT_SERIALIZE_OPT_BSEQUECE_HEX:
            MY_WRITE(rws, "bh", 2);
            for (i = 0; i < sz_content; i++) {
                unsigned char byte = (unsigned char)content[i];
                char buff[2];
                buff [0] = hex_chars[(byte >> 4) & 0x0f];
                buff [1] = hex_chars[byte & 0x0f];
                MY_WRITE(rws, buff, 2);
            }
            break;

        case PCVARIANT_SERIALIZE_OPT_BSEQUECE_BIN:
            MY_WRITE(rws, "bb", 2);
            for (i = 0; i < sz_content; i++) {
                unsigned char byte = (unsigned char)content[i];
                char buff[10];
                size_t k = 0;
                for (int j = 0; j < 8; j++) {
                    if (byte & 0x80 >> j)
                        buff[k] = '1';
                    else
                        buff[k] = '0';
                    k++;

                    if (flags & PCVARIANT_SERIALIZE_OPT_BSEQUENCE_BIN_DOT &&
                            j == 3) {
                        buff[k] = '.';
                        k++;
                    }
                }

                MY_WRITE(rws, buff, k);
            }
            break;

        case PCVARIANT_SERIALIZE_OPT_BSEQUECE_BASE64:
            MY_WRITE(rws, "b64", 3);
            ssize_t n = serialize_bsequence_base64(rws, content, sz_content);
            if (n < 0)
                goto failed;
            nr_written += n;
            break;

        default:
            PC_ASSERT(0);
            break;
    }

    return nr_written;

failed:
    return -1;

partial:
    // TODO:set an error code?
    return nr_written;
}

static ssize_t
print_newline(purc_rwstream_t rws, unsigned int flags)
{
    ssize_t nr_written = 0;

    if (flags & PCVARIANT_SERIALIZE_OPT_PRETTY) {
        nr_written = purc_rwstream_write(rws, "\n", 1);
    }

    return nr_written;
}

static ssize_t
print_indent(purc_rwstream_t rws, int level, unsigned int flags)
{
    size_t n;
    char buff[MAX_EMBEDDED_LEVELS * 2 + 2];

    if (level < 0 || level > MAX_EMBEDDED_LEVELS)
        return 0;

    if (flags & PCVARIANT_SERIALIZE_OPT_PRETTY_TAB) {
        n = level + 1;
        memset(buff, '\t', n);
    }
    else {
        n = level * 2 + 2;
        memset(buff, ' ', n);
    }

    return purc_rwstream_write(rws, buff, n);
}

static inline ssize_t
print_space(purc_rwstream_t rws, unsigned int flags)
{
    ssize_t nr_written = 0;

    if (flags & PCVARIANT_SERIALIZE_OPT_SPACED)
        nr_written = purc_rwstream_write(rws, " ", 1);

    return nr_written;
}

static inline ssize_t print_space_no_pretty(purc_rwstream_t rws,
        unsigned int flags)
{
    ssize_t nr_written = 0;
    if (flags & PCVARIANT_SERIALIZE_OPT_SPACED &&
            !(flags & PCVARIANT_SERIALIZE_OPT_PRETTY))
        nr_written = purc_rwstream_write(rws, " ", 1);

    return nr_written;
}

ssize_t purc_variant_serialize(purc_variant_t value,
        purc_rwstream_t rws, int level, unsigned int flags)
{
    ssize_t nr_written = 0;
    const char* content = NULL;
    size_t sz_content = 0;
    int i;
    char buff [128];
    purc_variant_t member = NULL;
    const char* key;

    PC_ASSERT(value);

    switch (value->type) {
        case PURC_VARIANT_TYPE_NULL:
            content = "null";
            sz_content = 4;
            break;

        case PURC_VARIANT_TYPE_UNDEFINED:
            content = "undefined";
            sz_content = 9;
            break;

        case PURC_VARIANT_TYPE_BOOLEAN:
            if (value->b) {
                content = "true";
                sz_content = 4;
            }
            else {
                content = "false";
                sz_content = 5;
            }
            break;

        case PURC_VARIANT_TYPE_NUMBER:
            sprintf(buff, "%f", value->d);
            content = buff;
            sz_content = strlen(buff);
            break;

        case PURC_VARIANT_TYPE_LONGINT:
            sprintf(buff, "%ld", value->i64);
            content = buff;
            sz_content = strlen(buff);
            break;

        case PURC_VARIANT_TYPE_LONGUINT:
            sprintf(buff, "%lu", value->u64);
            content = buff;
            sz_content = strlen(buff);
            break;

        case PURC_VARIANT_TYPE_LONGDOUBLE:
            sprintf(buff, "%Lf", value->ld);
            content = buff;
            sz_content = strlen(buff);
            break;

        case PURC_VARIANT_TYPE_ATOMSTRING:
            content = purc_atom_to_string(value->sz_ptr[1]);
            sz_content = strlen(content);
            nr_written +=
                serialize_string(rws, content, sz_content, flags);
            content = NULL;
            break;

        case PURC_VARIANT_TYPE_STRING:
        case PURC_VARIANT_TYPE_BSEQUENCE:
            if (value->flags & PCVARIANT_FLAG_EXTRA_SIZE) {
                content = (void*)value->sz_ptr[1];
                sz_content = (size_t)value->sz_ptr[0];
            }
            else {
                content = (char*)value->bytes;
                sz_content = value->size;
            }
            if (value->type == PURC_VARIANT_TYPE_STRING)
                nr_written +=
                    serialize_string(rws, content, sz_content, flags);
            else
                nr_written +=
                    serialize_bsequence(rws, content, sz_content, flags);
            content = NULL;
            break;

        case PURC_VARIANT_TYPE_DYNAMIC:
            content = "<dynamic>";
            sz_content = 9;
            break;

        case PURC_VARIANT_TYPE_NATIVE:
            content = "<native>";
            sz_content = 8;
            break;

        case PURC_VARIANT_TYPE_OBJECT:
            content = NULL;
            i = 0;
            MY_WRITE(rws, "{", 1);
            nr_written += print_newline(rws, flags);
            foreach_key_value_in_variant_object(value, key, member)
                if (i > 0) {
                    MY_WRITE(rws, ",", 1);
                    nr_written += print_space(rws, flags);
                }

                nr_written += print_indent(rws, level, flags);

                // key
                nr_written +=
                    serialize_string(rws, key, strlen(key), flags);
                MY_WRITE(rws, ":", 1);
                nr_written += print_space(rws, flags);

                // value
                nr_written += purc_variant_serialize(member,
                        rws, level + 1, flags);
                i++;
            end_foreach;
            nr_written += print_newline(rws, flags);
            MY_WRITE(rws, "}", 1);
            break;

        case PURC_VARIANT_TYPE_ARRAY:
            content = NULL;
            i = 0;
            MY_WRITE(rws, "[", 1);
            nr_written += print_newline(rws, flags);
            foreach_value_in_variant_array(value, member)
                if (i > 0) {
                    MY_WRITE(rws, ",", 1);
                    nr_written += print_space(rws, flags);
                }

                nr_written += print_indent(rws, level, flags);

                // member
                nr_written += purc_variant_serialize(member,
                        rws, level + 1, flags);
                i++;
            end_foreach;
            nr_written += print_newline(rws, flags);
            MY_WRITE(rws, "]", 1);
            break;

        case PURC_VARIANT_TYPE_SET:
            content = NULL;
            i = 0;
            MY_WRITE(rws, "[", 1);
            foreach_value_in_variant_set(value, member)
                if (i > 0) {
                    MY_WRITE(rws, ",", 1);
                    nr_written += print_space(rws, flags);
                }

                nr_written += print_indent(rws, level, flags);

                // member
                nr_written += purc_variant_serialize(member,
                        rws, level + 1, flags);
                i++;
            end_foreach;
            MY_WRITE(rws, "]", 1);
            break;

        default:
            break;
    }

    if (content) {
        // for simple types
        MY_WRITE(rws, content, strlen (content));
    }

    return nr_written;

failed:
    return -1;

partial:
    // TODO:set an error code?
    return nr_written;
}

