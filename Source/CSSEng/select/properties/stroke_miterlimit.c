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

css_error css__cascade_stroke_miterlimit(uint32_t opv, css_style *style, 
		css_select_state *state)
{
	uint16_t value = CSS_STROKE_MITERLIMIT_INHERIT;
	css_fixed stroke_miterlimit = 0;

	if (isInherit(opv) == false) {
		value = CSS_STROKE_MITERLIMIT_SET;

		stroke_miterlimit = *((css_fixed *) style->bytecode);
		advance_bytecode(style, sizeof(stroke_miterlimit));
	}

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_stroke_miterlimit(state->computed, value, stroke_miterlimit);
	}

	return CSS_OK;
}

css_error css__set_stroke_miterlimit_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_stroke_miterlimit(style, hint->status, hint->data.fixed);
}

css_error css__initial_stroke_miterlimit(css_select_state *state)
{
	return set_stroke_miterlimit(state->computed, CSS_STROKE_MITERLIMIT_SET, INTTOFIX(4));
}

css_error css__compose_stroke_miterlimit(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	css_fixed stroke_miterlimit = 0;
	uint8_t type = get_stroke_miterlimit(child, &stroke_miterlimit);

	if (type == CSS_STROKE_MITERLIMIT_INHERIT) {
		type = get_stroke_miterlimit(parent, &stroke_miterlimit);
	}

	return set_stroke_miterlimit(result, type, stroke_miterlimit);
}

