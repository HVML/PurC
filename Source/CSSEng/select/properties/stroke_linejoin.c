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

css_error css__cascade_stroke_linejoin(uint32_t opv, css_style *style,
        css_select_state *state)
{
    uint16_t value = CSS_STROKE_LINEJOIN_INHERIT;

    UNUSED(style);

    if (isInherit(opv) == false) {
        switch (getValue(opv)) {
        case STROKE_LINEJOIN_MITER:
            value = CSS_STROKE_LINEJOIN_MITER;
            break;
        case STROKE_LINEJOIN_ROUND:
            value = CSS_STROKE_LINEJOIN_ROUND;
            break;
        case STROKE_LINEJOIN_BEVEL:
            value = CSS_STROKE_LINEJOIN_BEVEL;
            break;
        }
    }

    if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
                isInherit(opv))) {
        return set_stroke_linejoin(state->computed, value);
    }

    return CSS_OK;
}

css_error css__set_stroke_linejoin_from_hint(const css_hint *hint,
        css_computed_style *style)
{
    return set_stroke_linejoin(style, hint->status);
}

css_error css__initial_stroke_linejoin(css_select_state *state)
{
    return set_stroke_linejoin(state->computed, CSS_STROKE_LINEJOIN_MITER);
}

css_error css__compose_stroke_linejoin(const css_computed_style *parent,
        const css_computed_style *child,
        css_computed_style *result)
{
    uint8_t type = get_stroke_linejoin(child);

    if (type == CSS_STROKE_LINEJOIN_INHERIT) {
        type = get_stroke_linejoin(parent);
    }

    return set_stroke_linejoin(result, type);
}
