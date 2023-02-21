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

css_error css__cascade_text_justify(uint32_t opv, css_style *style,
		css_select_state *state)
{
	uint16_t value = CSS_TEXT_JUSTIFY_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
        switch (getValue(opv)) {
            case TEXT_JUSTIFY_AUTO:
                value = CSS_TEXT_JUSTIFY_AUTO;
                break;
            case TEXT_JUSTIFY_NONE:
                value = CSS_TEXT_JUSTIFY_NONE;
                break;
            case TEXT_JUSTIFY_INTER_WORD:
                value = CSS_TEXT_JUSTIFY_INTER_WORD;
                break;
            case TEXT_JUSTIFY_INTER_IDEOGRAPH:
                value = CSS_TEXT_JUSTIFY_INTER_IDEOGRAPH;
                break;
            case TEXT_JUSTIFY_INTER_CLUSTER:
                value = CSS_TEXT_JUSTIFY_INTER_CLUSTER;
                break;
            case TEXT_JUSTIFY_DISTRIBUTE:
                value = CSS_TEXT_JUSTIFY_DISTRIBUTE;
                break;
            case TEXT_JUSTIFY_KASHIDA:
                value = CSS_TEXT_JUSTIFY_KASHIDA;
                break;
        }
	}

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_text_justify(state->computed, value);
	}

	return CSS_OK;
}

css_error css__set_text_justify_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_text_justify(style, hint->status);
}

css_error css__initial_text_justify(css_select_state *state)
{
	return set_text_justify(state->computed, CSS_TEXT_JUSTIFY_AUTO);
}

css_error css__compose_text_justify(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_text_justify(child);

	if (type == CSS_TEXT_JUSTIFY_INHERIT) {
		type = get_text_justify(parent);
	}

	return set_text_justify(result, type);
}

