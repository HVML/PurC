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

css_error css__cascade_clip_rule(uint32_t opv, css_style *style,
        css_select_state *state)
{
    uint16_t value = CSS_CLIP_RULE_INHERIT;

    UNUSED(style);

    if (isInherit(opv) == false) {
        switch (getValue(opv)) {
        case CLIP_RULE_NONZERO:
            value = CSS_CLIP_RULE_NONZERO;
            break;
        case CLIP_RULE_EVENODD:
            value = CSS_CLIP_RULE_EVENODD;
            break;
        }
    }

    if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
                isInherit(opv))) {
        return set_clip_rule(state->computed, value);
    }

    return CSS_OK;
}

css_error css__set_clip_rule_from_hint(const css_hint *hint,
        css_computed_style *style)
{
    return set_clip_rule(style, hint->status);
}

css_error css__initial_clip_rule(css_select_state *state)
{
    return set_clip_rule(state->computed, CSS_CLIP_RULE_NONZERO);
}

css_error css__compose_clip_rule(const css_computed_style *parent,
        const css_computed_style *child,
        css_computed_style *result)
{
    uint8_t type = get_clip_rule(child);

    if (type == CSS_CLIP_RULE_INHERIT) {
        type = get_clip_rule(parent);
    }

    return set_clip_rule(result, type);
}
