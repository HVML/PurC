/*
 * This file is part of CSSEng.
 * Licensed under the MIT License,
 *          http://www.opensource.org/licenses/mit-license.php
 * Copyright (C) 2021 ~ 2023 Beijing FMSoft Technologies Co., Ltd.
 */

#include <assert.h>
#include <string.h>

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "parse/properties/properties.h"
#include "parse/properties/utils.h"

css_error css__parse_filter_impl(css_language *c,
        const parserutils_vector *vector, int *ctx,
        css_style *result, int np)
{
    (void)np;
    int orig_ctx = *ctx;
    css_error error;
    const css_token *token;
    bool match;

    token = parserutils_vector_iterate(vector, ctx);
    if ((token == NULL) || ((token->type != CSS_TOKEN_IDENT) &&
                (token->type != CSS_TOKEN_URI))) {
        *ctx = orig_ctx;
        return CSS_INVALID;
    }

    if ((token->type == CSS_TOKEN_IDENT) &&
            (lwc_string_caseless_isequal(token->idata,
                                         c->strings[INHERIT], &match) ==
             lwc_error_ok && match)) {
        error = css_stylesheet_style_inherit(result, CSS_PROP_FILTER);
    } else if ((token->type == CSS_TOKEN_IDENT) &&
            (lwc_string_caseless_isequal(token->idata,
                                         c->strings[NONE], &match) ==
             lwc_error_ok && match)) {
        error = css__stylesheet_style_appendOPV(result,
                CSS_PROP_FILTER, 0, FILTER_NONE);
    } else if (token->type == CSS_TOKEN_URI) {
        lwc_string *uri = NULL;
        uint32_t uri_snumber;

        error = c->sheet->resolve(c->sheet->resolve_pw,
                c->sheet->url,
                token->idata, &uri);
        if (error != CSS_OK) {
            *ctx = orig_ctx;
            return error;
        }

        error = css__stylesheet_string_add(c->sheet, uri, &uri_snumber);
        if (error != CSS_OK) {
            *ctx = orig_ctx;
            return error;
        }

        error = css__stylesheet_style_appendOPV(result,
                CSS_PROP_FILTER, 0, FILTER_URI);
        if (error != CSS_OK) {
            *ctx = orig_ctx;
            return error;
        }

        error = css__stylesheet_style_append(result, uri_snumber);
    } else {
        error = CSS_INVALID;
    }

    if (error != CSS_OK)
        *ctx = orig_ctx;

    return error;
}

