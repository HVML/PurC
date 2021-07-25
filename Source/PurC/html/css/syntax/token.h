/**
 * @file token.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for css token.
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


#ifndef PCHTML_CSS_SYNTAX_TOKEN_H
#define PCHTML_CSS_SYNTAX_TOKEN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/in.h"
#include "html/core/str.h"

#include "html/css/syntax/base.h"


#define pchtml_css_syntax_token_data_type_set(tkz, dtype)                         \
    do {                                                                       \
        if ((pchtml_css_syntax_token(tkz)->data_type & dtype) == 0) {             \
                pchtml_css_syntax_token(tkz)->data_type |= dtype;                 \
        }                                                                      \
    }                                                                          \
    while (0)

#define pchtml_css_syntax_token_escaped_set(tkz)                                  \
    pchtml_css_syntax_token_data_type_set(tkz, PCHTML_CSS_SYNTAX_TOKEN_DATA_ESCAPED)

#define pchtml_css_syntax_token_cr_set(tkz)                                       \
    pchtml_css_syntax_token_data_type_set(tkz, PCHTML_CSS_SYNTAX_TOKEN_DATA_CR)

#define pchtml_css_syntax_token_ff_set(tkz)                                       \
    pchtml_css_syntax_token_data_type_set(tkz, PCHTML_CSS_SYNTAX_TOKEN_DATA_FF)

#define pchtml_css_syntax_token_have_null_set(tkz)                                \
    pchtml_css_syntax_token_data_type_set(tkz, PCHTML_CSS_SYNTAX_TOKEN_DATA_HAVE_NULL)


#define pchtml_css_syntax_token(tkz) ((pchtml_css_syntax_token_base_t *) tkz->token)

#define pchtml_css_syntax_token_base(token) ((pchtml_css_syntax_token_base_t *) token)
#define pchtml_css_syntax_token_ident(token) ((pchtml_css_syntax_token_ident_t *) token)
#define pchtml_css_syntax_token_function(token) ((pchtml_css_syntax_token_function_t *) token)
#define pchtml_css_syntax_token_at_keyword(token) ((pchtml_css_syntax_token_at_keyword_t *) token)
#define pchtml_css_syntax_token_hash(token) ((pchtml_css_syntax_token_hash_t *) token)
#define pchtml_css_syntax_token_string(token) ((pchtml_css_syntax_token_string_t *) token)
#define pchtml_css_syntax_token_bad_string(token) ((pchtml_css_syntax_token_bad_string_t *) token)
#define pchtml_css_syntax_token_url(token) ((pchtml_css_syntax_token_url_t *) token)
#define pchtml_css_syntax_token_bad_url(token) ((pchtml_css_syntax_token_bad_url_t *) token)
#define pchtml_css_syntax_token_delim(token) ((pchtml_css_syntax_token_delim_t *) token)
#define pchtml_css_syntax_token_number(token) ((pchtml_css_syntax_token_number_t *) token)
#define pchtml_css_syntax_token_percentage(token) ((pchtml_css_syntax_token_percentage_t *) token)
#define pchtml_css_syntax_token_dimension(token) ((pchtml_css_syntax_token_dimension_t *) token)
#define pchtml_css_syntax_token_whitespace(token) ((pchtml_css_syntax_token_whitespace_t *) token)
#define pchtml_css_syntax_token_cdo(token) ((pchtml_css_syntax_token_cdo_t *) token)
#define pchtml_css_syntax_token_cdc(token) ((pchtml_css_syntax_token_cdc_t *) token)
#define pchtml_css_syntax_token_colon(token) ((pchtml_css_syntax_token_colon_t *) token)
#define pchtml_css_syntax_token_semicolon(token) ((pchtml_css_syntax_token_semicolon_t *) token)
#define pchtml_css_syntax_token_comma(token) ((pchtml_css_syntax_token_comma_t *) token)
#define pchtml_css_syntax_token_ls_bracket(token) ((pchtml_css_syntax_token_ls_bracket_t *) token)
#define pchtml_css_syntax_token_rs_bracket(token) ((pchtml_css_syntax_token_rs_bracket_t *) token)
#define pchtml_css_syntax_token_l_parenthesis(token) ((pchtml_css_syntax_token_l_parenthesis_t *) token)
#define pchtml_css_syntax_token_r_parenthesis(token) ((pchtml_css_syntax_token_r_parenthesis_t *) token)
#define pchtml_css_syntax_token_lc_bracket(token) ((pchtml_css_syntax_token_lc_bracket_t *) token)
#define pchtml_css_syntax_token_rc_bracket(token) ((pchtml_css_syntax_token_rc_bracket_t *) token)
#define pchtml_css_syntax_token_comment(token) ((pchtml_css_syntax_token_comment_t *) token)


typedef struct pchtml_css_syntax_token_data pchtml_css_syntax_token_data_t;

typedef const unsigned char *
(*pchtml_css_syntax_token_data_cb_f)(const unsigned char *begin, const unsigned char *end,
                                  pchtml_str_t *str, pchtml_mraw_t *mraw,
                                  pchtml_css_syntax_token_data_t *td);

typedef unsigned int
(*pchtml_css_syntax_token_cb_f)(const unsigned char *data, size_t len, void *ctx);

struct pchtml_css_syntax_token_data {
    pchtml_css_syntax_token_data_cb_f cb;
    pchtml_in_node_t               *node_done;
    unsigned int                   status;
    int                            count;
    uint32_t                       num;
    bool                           is_last;
};


typedef unsigned int pchtml_css_syntax_token_type_t;
typedef unsigned int pchtml_css_syntax_token_data_type_t;

enum pchtml_css_syntax_token_type {
    PCHTML_CSS_SYNTAX_TOKEN_UNDEF = 0x00,
    PCHTML_CSS_SYNTAX_TOKEN_IDENT,
    PCHTML_CSS_SYNTAX_TOKEN_FUNCTION,
    PCHTML_CSS_SYNTAX_TOKEN_AT_KEYWORD,
    PCHTML_CSS_SYNTAX_TOKEN_HASH,
    PCHTML_CSS_SYNTAX_TOKEN_STRING,
    PCHTML_CSS_SYNTAX_TOKEN_BAD_STRING,
    PCHTML_CSS_SYNTAX_TOKEN_URL,
    PCHTML_CSS_SYNTAX_TOKEN_BAD_URL,
    PCHTML_CSS_SYNTAX_TOKEN_DELIM,
    PCHTML_CSS_SYNTAX_TOKEN_NUMBER,
    PCHTML_CSS_SYNTAX_TOKEN_PERCENTAGE,
    PCHTML_CSS_SYNTAX_TOKEN_DIMENSION,
    PCHTML_CSS_SYNTAX_TOKEN_WHITESPACE,
    PCHTML_CSS_SYNTAX_TOKEN_CDO,
    PCHTML_CSS_SYNTAX_TOKEN_CDC,
    PCHTML_CSS_SYNTAX_TOKEN_COLON,
    PCHTML_CSS_SYNTAX_TOKEN_SEMICOLON,
    PCHTML_CSS_SYNTAX_TOKEN_COMMA,
    PCHTML_CSS_SYNTAX_TOKEN_LS_BRACKET,   /* U+005B LEFT SQUARE BRACKET ([) */
    PCHTML_CSS_SYNTAX_TOKEN_RS_BRACKET,  /* U+005D RIGHT SQUARE BRACKET (]) */
    PCHTML_CSS_SYNTAX_TOKEN_L_PARENTHESIS,   /* U+0028 LEFT PARENTHESIS (() */
    PCHTML_CSS_SYNTAX_TOKEN_R_PARENTHESIS,  /* U+0029 RIGHT PARENTHESIS ()) */
    PCHTML_CSS_SYNTAX_TOKEN_LC_BRACKET,    /* U+007B LEFT CURLY BRACKET ({) */
    PCHTML_CSS_SYNTAX_TOKEN_RC_BRACKET,   /* U+007D RIGHT CURLY BRACKET (}) */
    PCHTML_CSS_SYNTAX_TOKEN_COMMENT,                /* not in specification */
    PCHTML_CSS_SYNTAX_TOKEN__EOF,
    PCHTML_CSS_SYNTAX_TOKEN__LAST_ENTRY
};

enum pchtml_css_syntax_token_data_type {
    PCHTML_CSS_SYNTAX_TOKEN_DATA_SIMPLE    = 0x00,
    PCHTML_CSS_SYNTAX_TOKEN_DATA_CR        = 0x01,
    PCHTML_CSS_SYNTAX_TOKEN_DATA_FF        = 0x02,
    PCHTML_CSS_SYNTAX_TOKEN_DATA_ESCAPED   = 0x04,
    PCHTML_CSS_SYNTAX_TOKEN_DATA_HAVE_NULL = 0x08
};

typedef struct pchtml_css_syntax_token_base {
    pchtml_css_syntax_token_type_t      type;
    pchtml_css_syntax_token_data_type_t data_type;
}
pchtml_css_syntax_token_base_t;

typedef struct pchtml_css_syntax_token_number {
    pchtml_css_syntax_token_base_t base;

    double                      num;
    bool                        is_float;
}
pchtml_css_syntax_token_number_t;

typedef struct pchtml_css_syntax_token_dimension {
    /* Do not change it. */
    pchtml_css_syntax_token_number_t num;

    pchtml_str_t                  data;

    /* Ident */
    const unsigned char              *begin;
    const unsigned char              *end;
}
pchtml_css_syntax_token_dimension_t;

typedef struct pchtml_css_syntax_token_string {
    pchtml_css_syntax_token_base_t base;

    pchtml_str_t                data;

    const unsigned char            *begin;
    const unsigned char            *end;
}
pchtml_css_syntax_token_string_t;

typedef struct pchtml_css_syntax_token_delim {
    pchtml_css_syntax_token_base_t base;

    unsigned char                  character;

    const unsigned char            *begin;
    const unsigned char            *end;
}
pchtml_css_syntax_token_delim_t;

typedef pchtml_css_syntax_token_string_t pchtml_css_syntax_token_ident_t;
typedef pchtml_css_syntax_token_string_t pchtml_css_syntax_token_function_t;
typedef pchtml_css_syntax_token_string_t pchtml_css_syntax_token_at_keyword_t;
typedef pchtml_css_syntax_token_string_t pchtml_css_syntax_token_hash_t;
typedef pchtml_css_syntax_token_string_t pchtml_css_syntax_token_bad_string_t;
typedef pchtml_css_syntax_token_string_t pchtml_css_syntax_token_url_t;
typedef pchtml_css_syntax_token_string_t pchtml_css_syntax_token_bad_url_t;
typedef pchtml_css_syntax_token_number_t pchtml_css_syntax_token_percentage_t;
typedef pchtml_css_syntax_token_string_t pchtml_css_syntax_token_whitespace_t;
typedef pchtml_css_syntax_token_base_t   pchtml_css_syntax_token_cdo_t;
typedef pchtml_css_syntax_token_base_t   pchtml_css_syntax_token_cdc_t;
typedef pchtml_css_syntax_token_base_t   pchtml_css_syntax_token_colon_t;
typedef pchtml_css_syntax_token_base_t   pchtml_css_syntax_token_semicolon_t;
typedef pchtml_css_syntax_token_base_t   pchtml_css_syntax_token_comma_t;
typedef pchtml_css_syntax_token_base_t   pchtml_css_syntax_token_ls_bracket_t;
typedef pchtml_css_syntax_token_base_t   pchtml_css_syntax_token_rs_bracket_t;
typedef pchtml_css_syntax_token_base_t   pchtml_css_syntax_token_l_parenthesis_t;
typedef pchtml_css_syntax_token_base_t   pchtml_css_syntax_token_r_parenthesis_t;
typedef pchtml_css_syntax_token_base_t   pchtml_css_syntax_token_lc_bracket_t;
typedef pchtml_css_syntax_token_base_t   pchtml_css_syntax_token_rc_bracket_t;
typedef pchtml_css_syntax_token_string_t pchtml_css_syntax_token_comment_t;

typedef struct pchtml_css_syntax_token {
    union pchtml_css_syntax_token_u {
        pchtml_css_syntax_token_base_t          base;
        pchtml_css_syntax_token_comment_t       comment;
        pchtml_css_syntax_token_number_t        number;
        pchtml_css_syntax_token_dimension_t     dimension;
        pchtml_css_syntax_token_percentage_t    percentage;
        pchtml_css_syntax_token_hash_t          hash;
        pchtml_css_syntax_token_string_t        string;
        pchtml_css_syntax_token_bad_string_t    bad_string;
        pchtml_css_syntax_token_delim_t         delim;
        pchtml_css_syntax_token_l_parenthesis_t lparenthesis;
        pchtml_css_syntax_token_r_parenthesis_t rparenthesis;
        pchtml_css_syntax_token_cdc_t           cdc;
        pchtml_css_syntax_token_function_t      function;
        pchtml_css_syntax_token_ident_t         ident;
        pchtml_css_syntax_token_url_t           url;
        pchtml_css_syntax_token_bad_url_t       bad_url;
        pchtml_css_syntax_token_at_keyword_t    at_keyword;
        pchtml_css_syntax_token_whitespace_t    whitespace;
    }
    types;
}
pchtml_css_syntax_token_t;


const unsigned char *
pchtml_css_syntax_token_type_name_by_id(
                pchtml_css_syntax_token_type_t type) WTF_INTERNAL;

pchtml_css_syntax_token_type_t
pchtml_css_syntax_token_type_id_by_name(const unsigned char *type_name, 
                size_t len) WTF_INTERNAL;

unsigned int
pchtml_css_syntax_token_make_data(pchtml_css_syntax_token_t *token, 
                pchtml_in_node_t *in, pchtml_mraw_t *mraw, 
                pchtml_css_syntax_token_data_t *td) WTF_INTERNAL;

unsigned int
pchtml_css_syntax_token_serialize_cb(pchtml_css_syntax_token_t *token,
                pchtml_css_syntax_token_cb_f cb, void *ctx) WTF_INTERNAL;

unsigned int
pchtml_css_syntax_token_serialize_str(pchtml_css_syntax_token_t *token,
                pchtml_str_t *str, pchtml_mraw_t *mraw) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline pchtml_css_syntax_token_t *
pchtml_css_syntax_token_create(pchtml_dobject_t *dobj)
{
    return (pchtml_css_syntax_token_t *) pchtml_dobject_calloc(dobj);
}

static inline void
pchtml_css_syntax_token_clean(pchtml_css_syntax_token_t *token)
{
    memset(token, 0, sizeof(pchtml_css_syntax_token_t));
}

static inline pchtml_css_syntax_token_t *
pchtml_css_syntax_token_destroy(pchtml_css_syntax_token_t *token,
                             pchtml_dobject_t *dobj)
{
    return (pchtml_css_syntax_token_t *) pchtml_dobject_free(dobj, token);
}

static inline const unsigned char *
pchtml_css_syntax_token_type_name(pchtml_css_syntax_token_t *token)
{
    return pchtml_css_syntax_token_type_name_by_id(token->types.base.type);
}

static inline pchtml_css_syntax_token_type_t
pchtml_css_syntax_token_type(pchtml_css_syntax_token_t *token)
{
    return token->types.base.type;
}

/*
 * No inline functions for ABI.
 */
pchtml_css_syntax_token_t *
pchtml_css_syntax_token_create_noi(pchtml_dobject_t *dobj);

void
pchtml_css_syntax_token_clean_noi(pchtml_css_syntax_token_t *token);

pchtml_css_syntax_token_t *
pchtml_css_syntax_token_destroy_noi(pchtml_css_syntax_token_t *token,
                                 pchtml_dobject_t *dobj);

const unsigned char *
pchtml_css_syntax_token_type_name_noi(pchtml_css_syntax_token_t *token);

pchtml_css_syntax_token_type_t
pchtml_css_syntax_token_type_noi(pchtml_css_syntax_token_t *token);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_CSS_SYNTAX_TOKEN_H */
