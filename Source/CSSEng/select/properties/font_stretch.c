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

css_error css__cascade_font_stretch(uint32_t opv, css_style *style,
        css_select_state *state)
{
    uint16_t value = CSS_FONT_STRETCH_INHERIT;

    UNUSED(style);

    if (isInherit(opv) == false) {
        switch (getValue(opv)) {
            case FONT_STRETCH_NORMAL:
                value = CSS_FONT_STRETCH_NORMAL;
                break;
            case FONT_STRETCH_WIDER:
                value = CSS_FONT_STRETCH_WIDER;
                break;
            case FONT_STRETCH_NARROWER:
                value = CSS_FONT_STRETCH_NARROWER;
                break;
            case FONT_STRETCH_ULTRA_CONDENSED:
                value = CSS_FONT_STRETCH_ULTRA_CONDENSED;
                break;
            case FONT_STRETCH_EXTRA_CONDENSED:
                value = CSS_FONT_STRETCH_EXTRA_CONDENSED;
                break;
            case FONT_STRETCH_CONDENSED:
                value = CSS_FONT_STRETCH_CONDENSED;
                break;
            case FONT_STRETCH_SEMI_CONDENSED:
                value = CSS_FONT_STRETCH_SEMI_CONDENSED;
                break;
            case FONT_STRETCH_SEMI_EXPANDED:
                value = CSS_FONT_STRETCH_SEMI_EXPANDED;
                break;
            case FONT_STRETCH_EXPANDED:
                value = CSS_FONT_STRETCH_EXPANDED;
                break;
            case FONT_STRETCH_EXTRA_EXPANDED:
                value = CSS_FONT_STRETCH_EXTRA_EXPANDED;
                break;
            case FONT_STRETCH_ULTRA_EXPANDED:
                value = CSS_FONT_STRETCH_ULTRA_EXPANDED;
                break;
        }
    }

    if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
                isInherit(opv))) {
        return set_font_stretch(state->computed, value);
    }

    return CSS_OK;
}

css_error css__set_font_stretch_from_hint(const css_hint *hint,
        css_computed_style *style)
{
    return set_font_stretch(style, hint->status);
}

css_error css__initial_font_stretch(css_select_state *state)
{
    return set_font_stretch(state->computed, CSS_FONT_STRETCH_NORMAL);
}

css_error css__compose_font_stretch(const css_computed_style *parent,
        const css_computed_style *child,
        css_computed_style *result)
{
    uint8_t type = get_font_stretch(child);

    if (type == CSS_FONT_STRETCH_INHERIT) {
        type = get_font_stretch(parent);
    }

    return set_font_stretch(result, type);
}
