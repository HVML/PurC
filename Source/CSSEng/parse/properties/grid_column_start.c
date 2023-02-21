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

css_error css__parse_grid_column_start(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style *result)
{
	int orig_ctx = *ctx;
	css_error error;
	const css_token *token;
    css_fixed length = 0;
    uint32_t unit = 0;

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

    *ctx = orig_ctx;
    error = css__parse_unit_specifier(c, vector, ctx, UNIT_PX, &length, &unit);
    if (error != CSS_OK) {
        *ctx = orig_ctx;
        return error;
    }

    error = css__stylesheet_style_appendOPV(result, CSS_PROP_GRID_COLUMN_START, 0, GRID_COLUMN_START_SET);
    if (error != CSS_OK)
        *ctx = orig_ctx;

    error = css__stylesheet_style_vappend(result, 2, length, unit);
    if (error != CSS_OK) {
        *ctx = orig_ctx;
        return error;
    }
	return error;
}
