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

css_error css__parse_stroke_dasharray_impl(css_language *c,
        const parserutils_vector *vector, int *ctx,
        css_style *result, int np)
{
    (void)np;
    int orig_ctx = *ctx;
    css_error error = CSS_OK;
    const css_token *token;
    bool match;

    css_fixed length = 0;
    uint32_t unit = 0;

    token = parserutils_vector_iterate(vector, ctx);
    if (token == NULL) {
        *ctx = orig_ctx;
        return CSS_INVALID;
    }

    if ((lwc_string_caseless_isequal(token->idata, c->strings[INHERIT], &match) == lwc_error_ok && match)) {
        error = css_stylesheet_style_inherit(result, CSS_PROP_STROKE_DASHARRAY);
    } else if ((lwc_string_caseless_isequal(token->idata, c->strings[NONE], &match) == lwc_error_ok && match)) {
        error = css__stylesheet_style_appendOPV(result, CSS_PROP_STROKE_DASHARRAY, 0,STROKE_DASHARRAY_NONE);
    }
    else {
        *ctx = orig_ctx;
        int last_ctx = *ctx;
        while ((token = parserutils_vector_iterate(vector, ctx)) != NULL)
        {
            if (token->idata == NULL)
            {
                last_ctx = *ctx;
                continue;
            }

            if (token->type == CSS_TOKEN_CHAR)
            {
                if (tokenIsChar(token, ','))
                {
                    last_ctx = *ctx;
                    continue;
                }
                else
                {
                    *ctx = orig_ctx;
                    return CSS_INVALID;
                }
            }

            if (token->type != CSS_TOKEN_NUMBER && token->type != CSS_TOKEN_PERCENTAGE) {
                *ctx = orig_ctx;
                return CSS_INVALID;
            }

            error = css__parse_unit_specifier(c, vector, &last_ctx, UNIT_PX, &length, &unit);
            if (error != CSS_OK)
            {
                *ctx = orig_ctx;
                return error;
            }
            last_ctx = *ctx;

            error = css__stylesheet_style_appendOPV(result, CSS_PROP_STROKE_DASHARRAY, 0, STROKE_DASHARRAY_SET);
            if (error != CSS_OK) {
                *ctx = orig_ctx;
                return error;
            }

            error = css__stylesheet_style_vappend(result, 2, length, unit);
            if (error != CSS_OK) {
                *ctx = orig_ctx;
                return error;
            }

        }
        error = css__stylesheet_style_appendOPV(result, CSS_PROP_STROKE_DASHARRAY, 0, STROKE_DASHARRAY_END);
    }

    if (error != CSS_OK)
        *ctx = orig_ctx;

    return error;
}

