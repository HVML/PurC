/**
 * @file base.h
 * @author 
 * @date 2021/07/02
 * @brief The basic hearder file for encoding.
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


#ifndef PCHTML_ENCODING_BASE_H
#define PCHTML_ENCODING_BASE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/base.h"
#include "html/encoding/const.h"


#define PCHTML_ENCODING_VERSION_MAJOR 2
#define PCHTML_ENCODING_VERSION_MINOR 0
#define PCHTML_ENCODING_VERSION_PATCH 1

#define PCHTML_ENCODING_VERSION_STRING                                            \
        PCHTML_STRINGIZE(PCHTML_ENCODING_VERSION_MAJOR) "."                       \
        PCHTML_STRINGIZE(PCHTML_ENCODING_VERSION_MINOR) "."                       \
        PCHTML_STRINGIZE(PCHTML_ENCODING_VERSION_PATCH)


#define PCHTML_ENCODING_REPLACEMENT_BYTES ((unsigned char *) "\xEF\xBF\xBD")

#define PCHTML_ENCODING_REPLACEMENT_BUFFER_LEN 1
#define PCHTML_ENCODING_REPLACEMENT_BUFFER                                        \
    (&(const uint32_t) {PCHTML_ENCODING_REPLACEMENT_CODEPOINT})


/*
 * In UTF-8 0x10FFFF value is maximum (inclusive)
 */
enum {
    PCHTML_ENCODING_REPLACEMENT_SIZE      = 0x03,
    PCHTML_ENCODING_REPLACEMENT_CODEPOINT = 0xFFFD,
    PCHTML_ENCODING_MAX_CODEPOINT         = 0x10FFFF,
    PCHTML_ENCODING_ERROR_CODEPOINT       = 0x1FFFFF
};

enum {
    PCHTML_ENCODING_ENCODE_OK           =  0x00,
    PCHTML_ENCODING_ENCODE_ERROR        = -0x01,
    PCHTML_ENCODING_ENCODE_SMALL_BUFFER = -0x02
};

enum {
    PCHTML_ENCODING_DECODE_MAX_CODEPOINT = PCHTML_ENCODING_MAX_CODEPOINT,
    PCHTML_ENCODING_DECODE_ERROR         = PCHTML_ENCODING_ERROR_CODEPOINT,
    PCHTML_ENCODING_DECODE_CONTINUE      = 0x2FFFFF
};

enum {
    PCHTML_ENCODING_DECODE_2022_JP_ASCII = 0x00,
    PCHTML_ENCODING_DECODE_2022_JP_ROMAN,
    PCHTML_ENCODING_DECODE_2022_JP_KATAKANA,
    PCHTML_ENCODING_DECODE_2022_JP_LEAD,
    PCHTML_ENCODING_DECODE_2022_JP_TRAIL,
    PCHTML_ENCODING_DECODE_2022_JP_ESCAPE_START,
    PCHTML_ENCODING_DECODE_2022_JP_ESCAPE,
    PCHTML_ENCODING_DECODE_2022_JP_UNSET
};

enum {
    PCHTML_ENCODING_ENCODE_2022_JP_ASCII = 0x00,
    PCHTML_ENCODING_ENCODE_2022_JP_ROMAN,
    PCHTML_ENCODING_ENCODE_2022_JP_JIS0208
};

typedef struct {
    unsigned   need;
    unsigned char lower;
    unsigned char upper;
}
pchtml_encoding_ctx_utf_8_t;

typedef struct {
    unsigned char first;
    unsigned char second;
    unsigned char third;
}
pchtml_encoding_ctx_gb18030_t;

typedef struct {
    unsigned char lead;
    bool       is_jis0212;
}
pchtml_encoding_ctx_euc_jp_t;

typedef struct {
    unsigned char lead;
    unsigned char prepand;
    unsigned   state;
    unsigned   out_state;
    bool       out_flag;
}
pchtml_encoding_ctx_2022_jp_t;

typedef struct pchtml_encoding_data pchtml_encoding_data_t;

typedef struct {
    const pchtml_encoding_data_t *encoding_data;

    /* Out buffer */
    uint32_t           *buffer_out;
    size_t                    buffer_length;
    size_t                    buffer_used;

    /*
     * Bad code points will be replaced to user code point.
     * If replace_to == 0 stop parsing and return error ot user.
     */
    const uint32_t     *replace_to;
    size_t                    replace_len;

    /* Not for users */
    uint32_t           codepoint;
    uint32_t           second_codepoint;
    bool                      prepend;
    bool                      have_error;

    unsigned int              status;

    union {
        pchtml_encoding_ctx_utf_8_t   utf_8;
        pchtml_encoding_ctx_gb18030_t gb18030;
        unsigned                   lead;
        pchtml_encoding_ctx_euc_jp_t  euc_jp;
        pchtml_encoding_ctx_2022_jp_t iso_2022_jp;
    } u;
}
pchtml_encoding_decode_t;

typedef struct {
    const pchtml_encoding_data_t *encoding_data;

    /* Out buffer */
    unsigned char                *buffer_out;
    size_t                    buffer_length;
    size_t                    buffer_used;

    /*
     * Bad code points will be replaced to user bytes.
     * If replace_to == NULL stop parsing and return error ot user.
     */
    const unsigned char          *replace_to;
    size_t                    replace_len;

    unsigned                  state;
}
pchtml_encoding_encode_t;

/*
* Why can't I pass a char ** to a function which expects a const char **?
* http://c-faq.com/ansi/constmismatch.html
*
* Short answer: use cast (const char **).
*
* For example:
*     pchtml_encoding_ctx_t ctx = {0};
*     const pchtml_encoding_data_t *enc;
*
*     unsigned char *data = (unsigned char *) "\x81\x30\x84\x36";
*
*     enc = pchtml_encoding_data(PCHTML_ENCODING_GB18030);
*
*     enc->decode(&ctx, (const unsigned char **) &data, data + 4);
*/
typedef unsigned int
(*pchtml_encoding_encode_f)(pchtml_encoding_encode_t *ctx, const uint32_t **cp,
                         const uint32_t *end);

typedef unsigned int
(*pchtml_encoding_decode_f)(pchtml_encoding_decode_t *ctx,
                         const unsigned char **data, const unsigned char *end);

typedef int8_t
(*pchtml_encoding_encode_single_f)(pchtml_encoding_encode_t *ctx, unsigned char **data,
                                const unsigned char *end, uint32_t cp);

typedef uint32_t
(*pchtml_encoding_decode_single_f)(pchtml_encoding_decode_t *ctx,
                                const unsigned char **data, const unsigned char *end);

struct pchtml_encoding_data {
    pchtml_encoding_t               encoding;
    pchtml_encoding_encode_f        encode;
    pchtml_encoding_decode_f        decode;
    pchtml_encoding_encode_single_f encode_single;
    pchtml_encoding_decode_single_f decode_single;
    unsigned char                   *name;
};

typedef struct {
    unsigned char      *name;
    unsigned        size;
    uint32_t codepoint;
}
pchtml_encoding_single_index_t;

typedef pchtml_encoding_single_index_t pchtml_encoding_multi_index_t;

typedef struct {
    unsigned        index;
    uint32_t codepoint;
}
pchtml_encoding_range_index_t;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_ENCODING_BASE_H */
