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

css_error css__cascade_flood_color(uint32_t opv, css_style *style,
		css_select_state *state)
{
	bool inherit = isInherit(opv);
	uint16_t value = CSS_FLOOD_COLOR_INHERIT;
	css_color color = 0;

	if (inherit == false) {
		switch (getValue(opv)) {
		case COLOR_TRANSPARENT:
			value = CSS_FLOOD_COLOR_COLOR;
			break;
		case COLOR_CURRENT_COLOR:
			/* color: currentColor always computes to inherit */
			value = CSS_FLOOD_COLOR_INHERIT;
			inherit = true;
			break;
		case COLOR_SET:
			value = CSS_FLOOD_COLOR_COLOR;
			color = *((css_color *) style->bytecode);
			advance_bytecode(style, sizeof(color));
			break;
		}
	}

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
			inherit)) {
		return set_flood_color(state->computed, value, color);
	}

	return CSS_OK;
}

css_error css__set_flood_color_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_flood_color(style, hint->status, hint->data.color);
}

css_error css__initial_flood_color(css_select_state *state)
{
	css_hint hint;
	css_error error;

	error = state->handler->ua_default_for_property(state->pw,
			CSS_PROP_COLOR, &hint);
	if (error != CSS_OK)
		return error;

	return css__set_flood_color_from_hint(&hint, state->computed);
}

css_error css__compose_flood_color(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_color color;
	uint8_t type = get_flood_color(child, &color);

	if (type == CSS_FLOOD_COLOR_INHERIT) {
		type = get_flood_color(parent, &color);
	}

	return set_flood_color(result, type, color);
}

