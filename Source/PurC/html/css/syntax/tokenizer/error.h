/**
 * @file error.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for css error.
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

#ifndef PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_H
#define PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/base.h"
#include "html/core/array_obj.h"


typedef enum {
    /* unexpected-eof */
    PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_UNEOF = 0x0000,
    /* eof-in-comment */
    PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_EOINCO,
    /* eof-in-string */
    PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_EOINST,
    /* eof-in-url */
    PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_EOINUR,
    /* qo-in-url */
    PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_QOINUR,
    /* wrong-escape-in-url */
    PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_WRESINUR,
    /* newline-in-string */
    PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_NEINST,
    /* bad-char */
    PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_BACH,
    /* bad-code-point */
    PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_BACOPO,
}
pchtml_css_syntax_tokenizer_error_id_t;

typedef struct {
    const unsigned char                    *pos;
    pchtml_css_syntax_tokenizer_error_id_t id;
}
pchtml_css_syntax_tokenizer_error_t;


pchtml_css_syntax_tokenizer_error_t *
pchtml_css_syntax_tokenizer_error_add(pchtml_array_obj_t *parse_errors,
                const unsigned char *pos,
                pchtml_css_syntax_tokenizer_error_id_t id) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_CSS_SYNTAX_TOKENIZER_ERROR_H */

