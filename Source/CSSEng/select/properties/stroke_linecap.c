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

css_error css__cascade_stroke_linecap(uint32_t opv, css_style *style,
        css_select_state *state)
{
    uint16_t value = CSS_STROKE_LINECAP_INHERIT;

    UNUSED(style);

    if (isInherit(opv) == false) {
        switch (getValue(opv)) {
        case STROKE_LINECAP_BUTT:
            value = CSS_STROKE_LINECAP_BUTT;
            break;
        case STROKE_LINECAP_ROUND:
            value = CSS_STROKE_LINECAP_ROUND;
            break;
        case STROKE_LINECAP_SQUARE:
            value = CSS_STROKE_LINECAP_SQUARE;
            break;
        }
    }

    if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
                isInherit(opv))) {
        return set_stroke_linecap(state->computed, value);
    }

    return CSS_OK;
}

css_error css__set_stroke_linecap_from_hint(const css_hint *hint,
        css_computed_style *style)
{
    return set_stroke_linecap(style, hint->status);
}

css_error css__initial_stroke_linecap(css_select_state *state)
{
    return set_stroke_linecap(state->computed, CSS_STROKE_LINECAP_BUTT);
}

css_error css__compose_stroke_linecap(const css_computed_style *parent,
        const css_computed_style *child,
        css_computed_style *result)
{
    uint8_t type = get_stroke_linecap(child);

    if (type == CSS_STROKE_LINECAP_INHERIT) {
        type = get_stroke_linecap(parent);
    }

    return set_stroke_linecap(result, type);
}
