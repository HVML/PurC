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

    This file is part of HybridOS, a developing operating system based on
    MiniGUI. HybridOs will support embedded systems and smart IoT devices.

    Copyright (C) 2020 Beijing FMSoft Technologies Co., Ltd.

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

css_error css__cascade_text_shadow(uint32_t opv, css_style *style,
        css_select_state *state)
{
    uint16_t v = CSS_TEXT_SHADOW_INHERIT;

    css_fixed text_shadow_h; 
    css_unit text_shadow_h_unit;

    css_fixed text_shadow_v;
    css_unit text_shadow_v_unit;

    css_fixed text_shadow_blur; 
    css_unit text_shadow_blur_unit;

    css_color text_shadow_color;
        
    uint16_t value = getValue(opv);
    if (value == TEXT_SHADOW_NONE)
        v = CSS_TEXT_SHADOW_NONE;
    if (isInherit(opv) == false && value != TEXT_SHADOW_NONE) {

        // text_shadow_h 
        text_shadow_h = *((css_fixed *) style->bytecode);
        advance_bytecode(style, sizeof(css_fixed));

        text_shadow_h_unit = *((uint32_t *) style->bytecode);
        advance_bytecode(style, sizeof(css_unit));
        text_shadow_h_unit = css__to_css_unit(text_shadow_h_unit);

        // text_shadow_v 
        text_shadow_v = *((css_fixed *) style->bytecode);
        advance_bytecode(style, sizeof(css_fixed));

        text_shadow_v_unit = *((uint32_t *) style->bytecode);
        advance_bytecode(style, sizeof(css_unit));
        text_shadow_v_unit = css__to_css_unit(text_shadow_v_unit);

        v = v | CSS_TEXT_SHADOW_H | CSS_TEXT_SHADOW_V;
        // text_shadow_blur
        if (value & TEXT_SHADOW_BLUR)
        {
            v = v | CSS_TEXT_SHADOW_BLUR;
            text_shadow_blur = *((css_fixed *) style->bytecode);
            advance_bytecode(style, sizeof(css_fixed));

            text_shadow_blur_unit = *((uint32_t *) style->bytecode);
            advance_bytecode(style, sizeof(css_unit));
            text_shadow_blur_unit = css__to_css_unit(text_shadow_blur_unit);

        }

        if (value & TEXT_SHADOW_COLOR)
        {
            v = v | CSS_TEXT_SHADOW_COLOR;
			text_shadow_color = *((css_color *) style->bytecode);
			advance_bytecode(style, sizeof(css_color));
        }
    }

    if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
            isInherit(opv))) {
		return set_text_shadow(state->computed, v,
                text_shadow_h, text_shadow_h_unit, 
                text_shadow_v, text_shadow_v_unit, 
                text_shadow_blur, text_shadow_blur_unit, 
                text_shadow_color);
    }

    return CSS_OK;
}

css_error css__set_text_shadow_from_hint(const css_hint *hint,
        css_computed_style *style)
{
    (void)hint;
    return set_text_shadow(style, CSS_TEXT_SHADOW_NONE,  0, 0, 0, 0, 0, 0, 0);
}

css_error css__initial_text_shadow(css_select_state *state)
{
    return set_text_shadow(state->computed, CSS_TEXT_SHADOW_NONE,  0, 0, 0, 0, 0, 0, 0);
}

css_error css__compose_text_shadow(const css_computed_style *parent,
        const css_computed_style *child,
        css_computed_style *result)
{
    css_fixed text_shadow_h; 
    css_unit text_shadow_h_unit;

    css_fixed text_shadow_v;
    css_unit text_shadow_v_unit;

    css_fixed text_shadow_blur; 
    css_unit text_shadow_blur_unit;

    css_color text_shadow_color;

    uint8_t type = get_text_shadow(child, 
            &text_shadow_h, &text_shadow_h_unit, 
            &text_shadow_v, &text_shadow_v_unit, 
            &text_shadow_blur, &text_shadow_blur_unit, 
            &text_shadow_color);

    if (type == CSS_WORD_WRAP_INHERIT) {
        type = get_text_shadow(parent, 
            &text_shadow_h, &text_shadow_h_unit, 
            &text_shadow_v, &text_shadow_v_unit, 
            &text_shadow_blur, &text_shadow_blur_unit, 
            &text_shadow_color);
    }

    return set_text_shadow(result, type, 
            text_shadow_h, text_shadow_h_unit, 
            text_shadow_v, text_shadow_v_unit, 
            text_shadow_blur, text_shadow_blur_unit, 
            text_shadow_color);
}
