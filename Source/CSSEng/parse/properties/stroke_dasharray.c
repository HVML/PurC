/////////////////////////////////////////////////////////////////////////////// //
//                          IMPORTANT NOTICE
//
// The following open source license statement does not apply to any
// entity in the Exception List published by FMSoft.
//
// For more information, please visit:
//
// https://www.fmsoft.cn/exception-list
//
//////////////////////////////////////////////////////////////////////////////
/**
 \verbatim

    This file is part of DOM Ruler. DOM Ruler is a library to
    maintain a DOM tree, lay out and stylize the DOM nodes by
    using CSS (Cascaded Style Sheets).

    Copyright (C) 2021 Beijing FMSoft Technologies Co., Ltd.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General License for more details.

    You should have received a copy of the GNU Lesser General License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Or,

    As this program is a library, any link to this program must follow
    GNU Lesser General License version 3 (LGPLv3). If you cannot accept
    LGPLv3, you need to be licensed from FMSoft.

    If you have got a commercial license of this program, please use it
    under the terms and conditions of the commercial license.

    For more information about the commercial license, please refer to
    <http://www.minigui.com/blog/minigui-licensing-policy/>.

 \endverbatim
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

