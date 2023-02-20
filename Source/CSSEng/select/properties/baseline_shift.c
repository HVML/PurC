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

css_error css__cascade_baseline_shift(uint32_t opv, css_style *style,
        css_select_state *state)
{
    uint16_t value = CSS_BASELINE_SHIFT_INHERIT;

    UNUSED(style);

    if (isInherit(opv) == false) {
        switch (getValue(opv)) {
        case BASELINE_SHIFT_BASELINE:
            value = CSS_BASELINE_SHIFT_BASELINE;
            break;
        case BASELINE_SHIFT_SUB:
            value = CSS_BASELINE_SHIFT_SUB;
            break;
        case BASELINE_SHIFT_SUPER:
            value = CSS_BASELINE_SHIFT_SUPER;
            break;
        }
    }

    if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
                isInherit(opv))) {
        return set_baseline_shift(state->computed, value);
    }

    return CSS_OK;
}

css_error css__set_baseline_shift_from_hint(const css_hint *hint,
        css_computed_style *style)
{
    return set_baseline_shift(style, hint->status);
}

css_error css__initial_baseline_shift(css_select_state *state)
{
    return set_baseline_shift(state->computed, CSS_BASELINE_SHIFT_INHERIT);
}

css_error css__compose_baseline_shift(const css_computed_style *parent,
        const css_computed_style *child,
        css_computed_style *result)
{
    uint8_t type = get_baseline_shift(child);

    if (type == CSS_BASELINE_SHIFT_INHERIT) {
        type = get_baseline_shift(parent);
    }

    return set_baseline_shift(result, type);
}
