/**
 * @file consume.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for css parse processing.
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


#ifndef PCHTML_CSS_SYNTAX_CONSUME_H
#define PCHTML_CSS_SYNTAX_CONSUME_H


#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/css/syntax/base.h"
#include "html/css/syntax/tokenizer.h"


const unsigned char *
pchtml_css_syntax_consume_string(pchtml_css_syntax_tokenizer_t *tkz,
                              const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_consume_before_numeric(pchtml_css_syntax_tokenizer_t *tkz,
                                      const unsigned char *data,
                                      const unsigned char *end);

const unsigned char *
pchtml_css_syntax_consume_numeric(pchtml_css_syntax_tokenizer_t *tkz,
                               const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_consume_numeric_decimal(pchtml_css_syntax_tokenizer_t *tkz,
                                       const unsigned char *data,
                                       const unsigned char *end);

const unsigned char *
pchtml_css_syntax_consume_ident_like(pchtml_css_syntax_tokenizer_t *tkz,
                                  const unsigned char *data, const unsigned char *end);

const unsigned char *
pchtml_css_syntax_consume_ident_like_not_url(pchtml_css_syntax_tokenizer_t *tkz,
                                          const unsigned char *data,
                                          const unsigned char *end);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_CSS_SYNTAX_CONSUME_H */
