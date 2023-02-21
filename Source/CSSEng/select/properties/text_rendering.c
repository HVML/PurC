/*
 * This file is part of CSSEng.
 * Licensed under the MIT License,
 *          http://www.opensource.org/licenses/mit-license.php
 * Copyright (C) 2021 ~ 2023 Beijing FMSoft Technologies Co., Ltd.
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
