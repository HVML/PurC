/**
 * @file encoding.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for encoding text.
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

#ifndef PCHTML_HTML_ENCODING_H
#define PCHTML_HTML_ENCODING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/html/base.h"

#include "html/core/array_obj.h"


typedef struct {
    const unsigned char *name;
    const unsigned char *end;
}
pchtml_html_encoding_entry_t;

typedef struct {
    pchtml_array_obj_t cache;
    pchtml_array_obj_t result;
}
pchtml_html_encoding_t;


unsigned int
pchtml_html_encoding_init(pchtml_html_encoding_t *em) WTF_INTERNAL;

pchtml_html_encoding_t *
pchtml_html_encoding_destroy(pchtml_html_encoding_t *em, 
                                    bool self_destroy) WTF_INTERNAL;


unsigned int
pchtml_html_encoding_determine(pchtml_html_encoding_t *em,
                            const unsigned char *data, 
                            const unsigned char *end) WTF_INTERNAL;

const unsigned char *
pchtml_html_encoding_content(const unsigned char *data, const unsigned char *end,
                          const unsigned char **name_end) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline pchtml_html_encoding_t *
pchtml_html_encoding_create(void)
{
    return (pchtml_html_encoding_t *) pchtml_calloc(1,
                                                 sizeof(pchtml_html_encoding_t));
}

static inline void
pchtml_html_encoding_clean(pchtml_html_encoding_t *em)
{
    pchtml_array_obj_clean(&em->cache);
    pchtml_array_obj_clean(&em->result);
}

static inline pchtml_html_encoding_entry_t *
pchtml_html_encoding_meta_entry(pchtml_html_encoding_t *em, size_t idx)
{
    return (pchtml_html_encoding_entry_t *) pchtml_array_obj_get(&em->result, idx);
}

static inline size_t
pchtml_html_encoding_meta_length(pchtml_html_encoding_t *em)
{
    return pchtml_array_obj_length(&em->result);
}

static inline pchtml_array_obj_t *
pchtml_html_encoding_meta_result(pchtml_html_encoding_t *em)
{
    return &em->result;
}

/*
 * No inline functions for ABI.
 */
pchtml_html_encoding_t *
pchtml_html_encoding_create_noi(void);

void
pchtml_html_encoding_clean_noi(pchtml_html_encoding_t *em);

pchtml_html_encoding_entry_t *
pchtml_html_encoding_meta_entry_noi(pchtml_html_encoding_t *em, size_t idx);

size_t
pchtml_html_encoding_meta_length_noi(pchtml_html_encoding_t *em);

pchtml_array_obj_t *
pchtml_html_encoding_meta_result_noi(pchtml_html_encoding_t *em);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_ENCODING_H */
