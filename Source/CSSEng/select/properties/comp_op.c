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

css_error css__cascade_comp_op(uint32_t opv, css_style *style,
        css_select_state *state)
{
    uint16_t value = CSS_COMP_OP_SRC_OVER;

    UNUSED(style);

    if (isInherit(opv) == false) {
        switch (getValue(opv)) {
            case COMP_OP_CLEAR:
                value = CSS_COMP_OP_CLEAR;
                break;
            case COMP_OP_SRC:
                value = CSS_COMP_OP_SRC;
                break;
            case COMP_OP_DST:
                value = CSS_COMP_OP_DST;
                break;
            case COMP_OP_SRC_OVER:
                value = CSS_COMP_OP_SRC_OVER;
                break;
            case COMP_OP_DST_OVER:
                value = CSS_COMP_OP_DST_OVER;
                break;
            case COMP_OP_SRC_IN:
                value = CSS_COMP_OP_SRC_IN;
                break;
            case COMP_OP_DST_IN:
                value = CSS_COMP_OP_DST_IN;
                break;
            case COMP_OP_SRC_OUT:
                value = CSS_COMP_OP_SRC_OUT;
                break;
            case COMP_OP_DST_OUT:
                value = CSS_COMP_OP_DST_OUT;
                break;
            case COMP_OP_SRC_ATOP:
                value = CSS_COMP_OP_SRC_ATOP;
                break;
            case COMP_OP_DST_ATOP:
                value = CSS_COMP_OP_DST_ATOP;
                break;
            case COMP_OP_XOR:
                value = CSS_COMP_OP_XOR;
                break;
            case COMP_OP_PLUS:
                value = CSS_COMP_OP_PLUS;
                break;
            case COMP_OP_MULTIPLY:
                value = CSS_COMP_OP_MULTIPLY;
                break;
            case COMP_OP_SCREEN:
                value = CSS_COMP_OP_SCREEN;
                break;
            case COMP_OP_OVERLAY:
                value = CSS_COMP_OP_OVERLAY;
                break;
            case COMP_OP_DARKEN:
                value = CSS_COMP_OP_DARKEN;
                break;
            case COMP_OP_LIGHTEN:
                value = CSS_COMP_OP_LIGHTEN;
                break;
            case COMP_OP_COLOR_DODGE:
                value = CSS_COMP_OP_COLOR_DODGE;
                break;
            case COMP_OP_COLOR_BURN:
                value = CSS_COMP_OP_COLOR_BURN;
                break;
            case COMP_OP_HARD_LIGHT:
                value = CSS_COMP_OP_HARD_LIGHT;
                break;
            case COMP_OP_SOFT_LIGHT:
                value = CSS_COMP_OP_SOFT_LIGHT;
                break;
            case COMP_OP_DIFFERENCE:
                value = CSS_COMP_OP_DIFFERENCE;
                break;
            case COMP_OP_EXCLUSION:
                value = CSS_COMP_OP_EXCLUSION;
                break;
        }
    }

    if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
                isInherit(opv))) {
        return set_comp_op(state->computed, value);
    }

    return CSS_OK;
}

css_error css__set_comp_op_from_hint(const css_hint *hint,
        css_computed_style *style)
{
    return set_comp_op(style, hint->status);
}

css_error css__initial_comp_op(css_select_state *state)
{
    return set_comp_op(state->computed, CSS_COMP_OP_SRC_OVER);
}

css_error css__compose_comp_op(const css_computed_style *parent,
        const css_computed_style *child,
        css_computed_style *result)
{
    (void)parent;
    uint8_t type = get_comp_op(child);
    return set_comp_op(result, type);
}
