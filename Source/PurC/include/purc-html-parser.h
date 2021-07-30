/**
 * @file purc-html-parser.h
 * @author Xu Xiaohong
 * @date 2021/07/30
 * @brief The API for html-parser.
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

#ifndef PURC_PURC_HTML_PARSER_H
#define PURC_PURC_HTML_PARSER_H

#include "purc-rwstream.h"

struct purc_html_document;
typedef struct purc_html_document purc_html_document;
typedef struct purc_html_document* purc_html_document_t;


PCA_EXTERN_C_BEGIN

/**
 */
PCA_EXPORT purc_html_document_t
purc_html_load_from_stream(purc_rwstream_t in);

/**
 */
PCA_EXPORT int
purc_html_write_to_stream(purc_html_document_t doc, purc_rwstream_t out);

/**
 */
PCA_EXPORT int
purc_html_destroy_doc(purc_html_document_t doc);



PCA_EXTERN_C_END

#endif // PURC_PURC_HTML_PARSER_H

