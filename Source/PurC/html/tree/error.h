/**
 * @file error.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for error type.
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
 *
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache 
 * License Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */


#ifndef PCHTML_HTML_TREE_ERROR_H
#define PCHTML_HTML_TREE_ERROR_H

#include "config.h"
#include "html/core_base.h"
#include "private/array_obj.h"

#include "html/token.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    /* unexpected-token */
    PCHTML_HTML_RULES_ERROR_UNTO = 0x0000,
    /* unexpected-closed-token */
    PCHTML_HTML_RULES_ERROR_UNCLTO,
    /* null-character */
    PCHTML_HTML_RULES_ERROR_NUCH,
    /* unexpected-character-token */
    PCHTML_HTML_RULES_ERROR_UNCHTO,
    /* unexpected-token-in-initial-mode */
    PCHTML_HTML_RULES_ERROR_UNTOININMO,
    /* bad-doctype-token-in-initial-mode */
    PCHTML_HTML_RULES_ERROR_BADOTOININMO,
    /* doctype-token-in-before-html-mode */
    PCHTML_HTML_RULES_ERROR_DOTOINBEHTMO,
    /* unexpected-closed-token-in-before-html-mode */
    PCHTML_HTML_RULES_ERROR_UNCLTOINBEHTMO,
    /* doctype-token-in-before-head-mode */
    PCHTML_HTML_RULES_ERROR_DOTOINBEHEMO,
    /* unexpected-closed_token-in-before-head-mode */
    PCHTML_HTML_RULES_ERROR_UNCLTOINBEHEMO,
    /* doctype-token-in-head-mode */
    PCHTML_HTML_RULES_ERROR_DOTOINHEMO,
    /* non-void-html-element-start-tag-with-trailing-solidus */
    PCHTML_HTML_RULES_ERROR_NOVOHTELSTTAWITRSO,
    /* head-token-in-head-mode */
    PCHTML_HTML_RULES_ERROR_HETOINHEMO,
    /* unexpected-closed-token-in-head-mode */
    PCHTML_HTML_RULES_ERROR_UNCLTOINHEMO,
    /* template-closed-token-without-opening-in-head-mode */
    PCHTML_HTML_RULES_ERROR_TECLTOWIOPINHEMO,
    /* template-element-is-not-current-in-head-mode */
    PCHTML_HTML_RULES_ERROR_TEELISNOCUINHEMO,
    /* doctype-token-in-head-noscript-mode */
    PCHTML_HTML_RULES_ERROR_DOTOINHENOMO,
    /* doctype-token-after-head-mode */
    PCHTML_HTML_RULES_ERROR_DOTOAFHEMO,
    /* head-token-after-head-mode */
    PCHTML_HTML_RULES_ERROR_HETOAFHEMO,
    /* doctype-token-in-body-mode */
    PCHTML_HTML_RULES_ERROR_DOTOINBOMO,
    /* bad-ending-open-elements-is-wrong */
    PCHTML_HTML_RULES_ERROR_BAENOPELISWR,
    /* open-elements-is-wrong */
    PCHTML_HTML_RULES_ERROR_OPELISWR,
    /* unexpected-element-in-open-elements-stack */
    PCHTML_HTML_RULES_ERROR_UNELINOPELST,
    /* missing-element-in-open-elements-stack */
    PCHTML_HTML_RULES_ERROR_MIELINOPELST,
    /* no-body-element-in-scope */
    PCHTML_HTML_RULES_ERROR_NOBOELINSC,
    /* missing-element-in-scope */
    PCHTML_HTML_RULES_ERROR_MIELINSC,
    /* unexpected-element-in-scope */
    PCHTML_HTML_RULES_ERROR_UNELINSC,
    /* unexpected-element-in-active-formatting-stack */
    PCHTML_HTML_RULES_ERROR_UNELINACFOST,
    /* unexpected-end-of-file */
    PCHTML_HTML_RULES_ERROR_UNENOFFI,
    /* characters-in-table-text */
    PCHTML_HTML_RULES_ERROR_CHINTATE,
    /* doctype-token-in-table-mode */
    PCHTML_HTML_RULES_ERROR_DOTOINTAMO,
    /* doctype-token-in-select-mode */
    PCHTML_HTML_RULES_ERROR_DOTOINSEMO,
    /* doctype-token-after-body-mode */
    PCHTML_HTML_RULES_ERROR_DOTOAFBOMO,
    /* doctype-token-in-frameset-mode */
    PCHTML_HTML_RULES_ERROR_DOTOINFRMO,
    /* doctype-token-after-frameset-mode */
    PCHTML_HTML_RULES_ERROR_DOTOAFFRMO,
    /* doctype-token-foreign-content-mode */
    PCHTML_HTML_RULES_ERROR_DOTOFOCOMO,

    PCHTML_HTML_RULES_ERROR_LAST_ENTRY
}
pchtml_html_tree_error_id_t;

typedef struct {
    pchtml_html_tree_error_id_t id;
}
pchtml_html_tree_error_t;


pchtml_html_tree_error_t *
pchtml_html_tree_error_add(pcutils_array_obj_t *parse_errors,
        pchtml_html_token_t *token, pchtml_html_tree_error_id_t id) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_TREE_ERROR_H */

