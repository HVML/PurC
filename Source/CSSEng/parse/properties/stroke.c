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

css_error css__parse_stroke_impl(css_language *c,
        const parserutils_vector *vector, int *ctx,
        css_style *result, int np)
{
    (void)np;
    int orig_ctx = *ctx;
    css_error error;
    const css_token *token;
    bool match;

    token = parserutils_vector_iterate(vector, ctx);
    if (token == NULL) {
        *ctx = orig_ctx;
        return CSS_INVALID;
    }

    if ((token->type == CSS_TOKEN_IDENT)
            && (lwc_string_caseless_isequal(token->idata,
                    c->strings[INHERIT], &match) == lwc_error_ok && match)) {
        error = css_stylesheet_style_inherit(result, CSS_PROP_STROKE);
    } else if ((token->type == CSS_TOKEN_IDENT) &&
            (lwc_string_caseless_isequal(token->idata, c->strings[NONE],
                                         &match) == lwc_error_ok && match)) {
        error = css__stylesheet_style_appendOPV(result,
                CSS_PROP_STROKE, 0,STROKE_NONE);
    } else if ((token->type == CSS_TOKEN_IDENT) &&
            (lwc_string_caseless_isequal(token->idata,
                                         c->strings[CURRENTCOLOR],
                                         &match) == lwc_error_ok && match)) {
        error = css__stylesheet_style_appendOPV(result,
                CSS_PROP_STROKE, 0,STROKE_CURRENT_COLOR);
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

        error = css__stylesheet_style_appendOPV(result, CSS_PROP_STROKE,
                0, STROKE_URI);
        if (error != CSS_OK) {
            *ctx = orig_ctx;
            return error;
        }

        error = css__stylesheet_style_append(result, uri_snumber);
    } else {
        uint16_t value = 0;
        uint32_t color = 0;
        *ctx = orig_ctx;

        error = css__parse_colour_specifier(c, vector, ctx, &value, &color);
        if (error != CSS_OK) {
            *ctx = orig_ctx;
            return error;
        }

        error = css__stylesheet_style_appendOPV(result, CSS_PROP_STROKE,
                0, STROKE_SET_COLOR);
        if (error != CSS_OK) {
            *ctx = orig_ctx;
            return error;
        }

        error = css__stylesheet_style_append(result, color);
    }

    if (error != CSS_OK)
        *ctx = orig_ctx;

    return error;
}
