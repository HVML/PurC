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

css_error css__parse_grid_template_columns_impl(css_language *c,
        const parserutils_vector *vector, int *ctx,
        css_style *result, int np)
{
    (void)np;
    int orig_ctx = *ctx;
    int last_ctx = *ctx;
    css_error error;
    const css_token *token;
    css_fixed length = 0;
    uint32_t unit = 0;

    while ((token = parserutils_vector_iterate(vector, ctx)) != NULL) {
        if (token->idata != NULL) {
            error = css__parse_unit_specifier(c, vector, &last_ctx, UNIT_PX, &length, &unit);
            if (error != CSS_OK) 
            {
                *ctx = orig_ctx;
                return error;
            }
            error = css__stylesheet_style_appendOPV(result, CSS_PROP_GRID_TEMPLATE_COLUMNS, 0, GRID_TEMPLATE_COLUMNS_SET);
            if (error != CSS_OK)
            {
                *ctx = orig_ctx;
                return error;
            }

            error = css__stylesheet_style_vappend(result, 2, length, unit);
            if (error != CSS_OK) {
                *ctx = orig_ctx;
                return error;
            }
            last_ctx = *ctx;
        }
    }
    error = css__stylesheet_style_append(result, GRID_TEMPLATE_COLUMNS_END);
    return error;
}
