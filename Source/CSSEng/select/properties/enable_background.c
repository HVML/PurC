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

css_error css__cascade_enable_background(uint32_t opv, css_style *style,
        css_select_state *state)
{
    uint16_t value = ENABLE_BACKGROUND_ACCUMULATE;

    UNUSED(style);

    if (isInherit(opv) == false) {
        switch (getValue(opv)) {
        case ENABLE_BACKGROUND_ACCUMULATE:
            value = CSS_ENABLE_BACKGROUND_ACCUMULATE;
            break;
        case ENABLE_BACKGROUND_NEW:
            value = CSS_ENABLE_BACKGROUND_NEW;
            break;
        }
    }

    if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
                isInherit(opv))) {
        return set_enable_background(state->computed, value);
    }

    return CSS_OK;
}

css_error css__set_enable_background_from_hint(const css_hint *hint,
        css_computed_style *style)
{
    return set_enable_background(style, hint->status);
}

css_error css__initial_enable_background(css_select_state *state)
{
    return set_enable_background(state->computed, CSS_ENABLE_BACKGROUND_ACCUMULATE);
}

css_error css__compose_enable_background(const css_computed_style *parent,
        const css_computed_style *child,
        css_computed_style *result)
{
    (void)parent;
    uint8_t type = get_enable_background(child);
    return set_enable_background(result, type);
}
