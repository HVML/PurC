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

#define LENGTH_SIZE  3
css_error css__parse_text_shadow_impl(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style *result, int np)
{
    (void)np;
    int orig_ctx = *ctx;
	css_error error = CSS_INVALID;
	const css_token *token;
	bool match;

    css_fixed lengths[LENGTH_SIZE];
    uint32_t  units[LENGTH_SIZE];
    uint8_t count = 0;

    bool has_color = false;
    uint16_t color_value = 0;
    uint32_t color_color = 0;

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if (lwc_string_caseless_isequal(token->idata, c->strings[INHERIT], &match) == lwc_error_ok && match) {
		error = css_stylesheet_style_inherit(result, CSS_PROP_TEXT_SHADOW);
	} else if (lwc_string_caseless_isequal(token->idata, c->strings[NONE], &match) == lwc_error_ok && match) {
		error = css__stylesheet_style_appendOPV(result, CSS_PROP_TEXT_SHADOW, 0, TEXT_SHADOW_NONE);
	} else {
        *ctx = orig_ctx;
        int last_ctx = *ctx;
        while ((token = parserutils_vector_iterate(vector, ctx)) != NULL)
        {
            if (token->idata == NULL)
            {
                last_ctx = *ctx;
                continue;
            }

            switch (token->type)
            {
                case CSS_TOKEN_IDENT:
                case CSS_TOKEN_HASH:
                case CSS_TOKEN_FUNCTION:
                    if (has_color)
                    {
                        *ctx = orig_ctx;
                        return CSS_INVALID;
                    }
                    else
                    {
                        error = css__parse_colour_specifier(c, vector, &last_ctx, &color_value, &color_color);
                        if (error == CSS_OK) 
                        {
                            has_color = true;
                            last_ctx = *ctx;
                        }
                        else
                        {
                            *ctx = orig_ctx;
                            return error;
                        }
                    }
                    break;

                case CSS_TOKEN_NUMBER:
                case CSS_TOKEN_PERCENTAGE:
                case CSS_TOKEN_DIMENSION:
                    if (count < LENGTH_SIZE)
                    {
                        error = css__parse_unit_specifier(c, vector, &last_ctx, UNIT_PX, &lengths[count], &units[count]);
                        if (error == CSS_OK)
                        {
                            count++;
                            last_ctx = *ctx;
                            continue;
                        }
                        else
                        {
                            *ctx = orig_ctx;
                            return error;
                        }
                    }
                    else
                    {
                        *ctx = orig_ctx;
                        return CSS_INVALID;
                    }
                    break;

                default:
                    *ctx = orig_ctx;
                    return CSS_INVALID;
            }

            last_ctx = *ctx;
        }

        if (count < 2)
        {
            *ctx = orig_ctx;
            return CSS_INVALID;
        }

        uint8_t type = TEXT_SHADOW_H | TEXT_SHADOW_V;
        if (count > 2)
        {
            type = type | TEXT_SHADOW_BLUR;
        }

        if (has_color)
        {
            type = type | TEXT_SHADOW_COLOR;
        }

        error = css__stylesheet_style_appendOPV(result, CSS_PROP_TEXT_SHADOW, 0, type);
        if (error != CSS_OK)
        {
            *ctx = orig_ctx;
            return error;
        }

        for (int i = 0; i < count; i++)
        {
            error = css__stylesheet_style_vappend(result, 2, lengths[i], units[i]);
            if (error != CSS_OK) {
                *ctx = orig_ctx;
                return error;
            }
        }

        if (has_color)
        {
            error = css__stylesheet_style_append(result, color_color);
            if (error != CSS_OK) {
                *ctx = orig_ctx;
                return error;
            }
        }
    }

	if (error != CSS_OK)
		*ctx = orig_ctx;

	return error;
}

