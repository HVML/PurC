/**
 * @file warc.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for warc.
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


#ifndef PCHTML_UTILS_WARC_H
#define PCHTML_UTILS_WARC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/utils/base.h"

#include "html/core/mraw.h"
#include "html/core/str.h"
#include "html/core/array_obj.h"


typedef struct pchtml_utils_warc pchtml_utils_warc_t;

typedef struct {
    pchtml_str_t type;
    double       number;
}
pchtml_utils_warc_version_t;

typedef struct {
    pchtml_str_t name;
    pchtml_str_t value;
}
pchtml_utils_warc_field_t;

typedef unsigned int
(*pchtml_utils_warc_header_cb_f)(pchtml_utils_warc_t *warc);

typedef unsigned int
(*pchtml_utils_warc_content_cb_f)(pchtml_utils_warc_t *warc, const unsigned char *data,
                               const unsigned char *end);
typedef unsigned int
(*pchtml_utils_warc_content_end_cb_f)(pchtml_utils_warc_t *warc);

struct pchtml_utils_warc {
    pchtml_mraw_t                   *mraw;
    pchtml_array_obj_t              *fields;

    pchtml_str_t                    tmp;
    pchtml_utils_warc_version_t        version;

    pchtml_utils_warc_header_cb_f      header_cb;
    pchtml_utils_warc_content_cb_f     content_cb;
    pchtml_utils_warc_content_end_cb_f content_end_cb;
    void                            *ctx;

    size_t                          content_length;
    size_t                          content_read;
    size_t                          count;

    const char                      *error;
    unsigned                        state;
    unsigned                        ends;
    bool                            skip;
};


pchtml_utils_warc_t *
pchtml_utils_warc_create(void);

unsigned int
pchtml_utils_warc_init(pchtml_utils_warc_t *warc, pchtml_utils_warc_header_cb_f h_cd,
                pchtml_utils_warc_content_cb_f c_cb,
                pchtml_utils_warc_content_end_cb_f c_end_cb, void *ctx) WTF_INTERNAL;

unsigned int
pchtml_utils_warc_clear(pchtml_utils_warc_t *warc) WTF_INTERNAL;

pchtml_utils_warc_t *
pchtml_utils_warc_destroy(pchtml_utils_warc_t *warc, bool self_destroy) WTF_INTERNAL;


unsigned int
pchtml_utils_warc_parse_file(pchtml_utils_warc_t *warc, FILE *fh) WTF_INTERNAL;

/*
 * We must call pchtml_warc_parse_eof after processing.
 * Before new processing we must call pchtml_warc_clear
 * if previously ends with error.
 */
unsigned int
pchtml_utils_warc_parse(pchtml_utils_warc_t *warc,
                const unsigned char **data, const unsigned char *end) WTF_INTERNAL;

unsigned int
pchtml_utils_warc_parse_eof(pchtml_utils_warc_t *warc) WTF_INTERNAL;


pchtml_utils_warc_field_t *
pchtml_utils_warc_header_field(pchtml_utils_warc_t *warc, const unsigned char *name,
                size_t len, size_t offset) WTF_INTERNAL;

unsigned int
pchtml_utils_warc_header_serialize(pchtml_utils_warc_t *warc, 
                pchtml_str_t *str) WTF_INTERNAL;

/*
 * Inline functions
 */
static inline size_t
pchtml_utils_warc_content_length(pchtml_utils_warc_t *warc)
{
    return warc->content_length;
}

/*
 * No inline functions for ABI.
 */
size_t
pchtml_utils_warc_content_length_noi(pchtml_utils_warc_t *warc) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_UTILS_WARC_H */

