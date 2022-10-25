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

css_error css__cascade_text_rendering(uint32_t opv, css_style *style,
        css_select_state *state)
{
    uint16_t value = CSS_TEXT_RENDERING_INHERIT;

    UNUSED(style);

    if (isInherit(opv) == false) {
        switch (getValue(opv)) {
            case TEXT_RENDERING_AUTO:
                value = CSS_TEXT_RENDERING_AUTO;
                break;
            case TEXT_RENDERING_OPTIMIZESPEED:
                value = CSS_TEXT_RENDERING_OPTIMIZESPEED;
                break;
            case TEXT_RENDERING_GEOMETRICPRECISION:
                value = CSS_TEXT_RENDERING_GEOMETRICPRECISION;
                break;
            case TEXT_RENDERING_OPTIMIZELEGIBILITY:
                value = CSS_TEXT_RENDERING_OPTIMIZELEGIBILITY;
                break;
            case TEXT_RENDERING_DEFAULT:
                value = CSS_TEXT_RENDERING_DEFAULT;
                break;
        }
    }

    if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
                isInherit(opv))) {
        return set_text_rendering(state->computed, value);
    }

    return CSS_OK;
}

css_error css__set_text_rendering_from_hint(const css_hint *hint,
        css_computed_style *style)
{
    return set_text_rendering(style, hint->status);
}

css_error css__initial_text_rendering(css_select_state *state)
{
    return set_text_rendering(state->computed, CSS_TEXT_RENDERING_AUTO);
}

css_error css__compose_text_rendering(const css_computed_style *parent,
        const css_computed_style *child,
        css_computed_style *result)
{
    uint8_t type = get_text_rendering(child);

    if (type == CSS_TEXT_RENDERING_INHERIT) {
        type = get_text_rendering(parent);
    }

    return set_text_rendering(result, type);
}
