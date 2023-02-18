/*
 * This file is part of CSSEng
 * Licensed under the MIT License,
 *          http://www.opensource.org/licenses/mit-license.php
 * Copyright (C) 2022 Beijing FMSoft Technologies Co., Ltd.
 */

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "select/propset.h"
#include "select/propget.h"
#include "utils/utils.h"

#include "select/properties/properties.h"
#include "select/properties/helpers.h"

css_error css__cascade__foil_color_info(uint32_t opv, css_style *style,
        css_select_state *state)
{
    bool inherit = isInherit(opv);
    uint16_t value = CSS_COLOR_INHERIT;
    css_color color = 0;

    if (inherit == false) {
        switch (getValue(opv)) {
        case COLOR_TRANSPARENT:
            value = CSS_COLOR_COLOR;
            break;
        case COLOR_CURRENT_COLOR:
            /* color: currentColor always computes to inherit */
            value = CSS_COLOR_INHERIT;
            inherit = true;
            break;
        case COLOR_DEFAULT:
            value = CSS_COLOR_DEFAULT;
            break;
        case COLOR_SET:
            value = CSS_COLOR_COLOR;
            color = *((css_color *) style->bytecode);
            advance_bytecode(style, sizeof(color));
            break;
        }
    }

    if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
            inherit)) {
        return set__foil_color_info(state->computed, value, color);
    }

    return CSS_OK;
}

css_error css__set__foil_color_info_from_hint(const css_hint *hint,
        css_computed_style *style)
{
    return set__foil_color_info(style, hint->status, hint->data.color);
}

css_error css__initial__foil_color_info(css_select_state *state)
{
    css_hint hint;
    css_error error;

    error = state->handler->ua_default_for_property(state->pw,
            CSS_PROP_FOIL_COLOR_INFO, &hint);
    if (error != CSS_OK)
        return error;

    return css__set__foil_color_info_from_hint(&hint, state->computed);
}

css_error css__compose__foil_color_info(const css_computed_style *parent,
        const css_computed_style *child,
        css_computed_style *result)
{
    css_color color;
    uint8_t type = get__foil_color_info(child, &color);

    if (type == CSS_COLOR_INHERIT) {
        type = get__foil_color_info(parent, &color);
    }

    return set__foil_color_info(result, type, color);
}

