/**
 * @file http.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for http protocol.
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


#ifndef PCHTML_UTILS_HTTP_H
#define PCHTML_UTILS_HTTP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/utils/base.h"

#include "html/core/mraw.h"
#include "html/core/str.h"
#include "html/core/array_obj.h"


typedef struct {
    pchtml_str_t name;
    double       number;
    unsigned     status;
}
pchtml_utils_http_version_t;

typedef struct {
    pchtml_str_t name;
    pchtml_str_t value;
}
pchtml_utils_http_field_t;

typedef struct {
    pchtml_mraw_t            *mraw;
    pchtml_array_obj_t       *fields;

    pchtml_str_t             tmp;
    pchtml_utils_http_version_t version;

    const char               *error;
    unsigned                 state;
}
pchtml_utils_http_t;


pchtml_utils_http_t *
pchtml_utils_http_create(void) WTF_INTERNAL;

unsigned int
pchtml_utils_http_init(pchtml_utils_http_t *http, 
            pchtml_mraw_t *mraw) WTF_INTERNAL;

unsigned int
pchtml_utils_http_clear(pchtml_utils_http_t *http) WTF_INTERNAL;

pchtml_utils_http_t *
pchtml_utils_http_destroy(pchtml_utils_http_t *http, 
                bool self_destroy) WTF_INTERNAL;

/*
 * Before new processing we must call pchtml_utils_http_clear function.
 */
unsigned int
pchtml_utils_http_parse(pchtml_utils_http_t *http,
                const unsigned char **data, 
                const unsigned char *end) WTF_INTERNAL;

unsigned int
pchtml_utils_http_header_parse_eof(pchtml_utils_http_t *http) WTF_INTERNAL;


pchtml_utils_http_field_t *
pchtml_utils_http_header_field(pchtml_utils_http_t *http, const unsigned char *name,
                size_t len, size_t offset) WTF_INTERNAL;

unsigned int
pchtml_utils_http_header_serialize(pchtml_utils_http_t *http, 
                pchtml_str_t *str) WTF_INTERNAL;

unsigned int
pchtml_utils_http_field_serialize(pchtml_utils_http_t *http, pchtml_str_t *str,
                const pchtml_utils_http_field_t *field) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* UTILS_PCHTML_HTTP_H */

