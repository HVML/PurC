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

css_error css__cascade_text_align_last(uint32_t opv, css_style *style,
		css_select_state *state)
{
	uint16_t value = CSS_TEXT_ALIGN_LAST_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
        switch (getValue(opv)) {
            case TEXT_ALIGN_LAST_AUTO:
                value = CSS_TEXT_ALIGN_LAST_AUTO;
                break;
            case TEXT_ALIGN_LAST_LEFT:
                value = CSS_TEXT_ALIGN_LAST_LEFT;
                break;
            case TEXT_ALIGN_LAST_RIGHT:
                value = CSS_TEXT_ALIGN_LAST_RIGHT;
                break;
            case TEXT_ALIGN_LAST_CENTER:
                value = CSS_TEXT_ALIGN_LAST_CENTER;
                break;
            case TEXT_ALIGN_LAST_JUSTIFY:
                value = CSS_TEXT_ALIGN_LAST_JUSTIFY;
                break;
            case TEXT_ALIGN_LAST_START:
                value = CSS_TEXT_ALIGN_LAST_START;
                break;
            case TEXT_ALIGN_LAST_END:
                value = CSS_TEXT_ALIGN_LAST_END;
                break;
		}
	}

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
			isInherit(opv))) {
		return set_text_align_last(state->computed, value);
	}

	return CSS_OK;
}

css_error css__set_text_align_last_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_text_align_last(style, hint->status);
}

css_error css__initial_text_align_last(css_select_state *state)
{
	return set_text_align_last(state->computed, CSS_TEXT_ALIGN_LAST_AUTO);
}

css_error css__compose_text_align_last(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_text_align_last(child);

	if (type == CSS_TEXT_ALIGN_LAST_INHERIT) {
		type = get_text_align_last(parent);
	}

	return set_text_align_last(result, type);
}

