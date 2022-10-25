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


#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "select/propset.h"
#include "select/propget.h"
#include "utils/utils.h"

#include "select/properties/properties.h"
#include "select/properties/helpers.h"

css_error css__cascade_stroke(uint32_t opv, css_style *style,
		css_select_state *state)
{
    uint16_t value = CSS_STROKE_INHERIT;
    lwc_string *uri = NULL;
    css_color color;

    if (isInherit(opv) == false) {
        switch (getValue(opv)) {
        case STROKE_NONE:
            value = CSS_STROKE_NONE;
            break;
        case STROKE_CURRENT_COLOR:
            value = CSS_STROKE_CURRENT_COLOR;
            break;
        case STROKE_URI:
            value = CSS_STROKE_URI;
            css__stylesheet_string_get(style->sheet, *((css_code_t *) style->bytecode), &uri);
            advance_bytecode(style, sizeof(css_code_t));
            break;
        case STROKE_SET_COLOR:
            value = CSS_STROKE_SET_COLOR;
			color = *((css_color *) style->bytecode);
			advance_bytecode(style, sizeof(color));
            break;
        }
    }

    if (css__outranks_existing(getOpcode(opv),
            isImportant(opv), state, isInherit(opv))) {
        return set_stroke(state->computed, value, uri, color);
    }

    return CSS_OK;
}

css_error css__set_stroke_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	css_error error;

	error = set_stroke(style, hint->status, hint->data.string, 0);

	if (hint->data.string != NULL)
		lwc_string_unref(hint->data.string);

	return error;
}

css_error css__initial_stroke(css_select_state *state)
{
	return set_stroke(state->computed, CSS_STROKE_NOT_SET, NULL, 0);
}

css_error css__compose_stroke(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	lwc_string *url;
    css_color color;
	uint8_t type = get_stroke(child, &url, &color);

	if (type == CSS_STROKE_INHERIT) {
		type = get_stroke(parent, &url, &color);
	}

	return set_stroke(result, type, url, color);
}
