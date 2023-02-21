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

css_error css__cascade_text_overflow(uint32_t opv, css_style *style,
		css_select_state *state)
{
	uint16_t value = CSS_TEXT_OVERFLOW_INHERIT;
	lwc_string *strings = NULL;

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case TEXT_OVERFLOW_CLIP:
			value = CSS_TEXT_OVERFLOW_CLIP;
			break;
        case TEXT_OVERFLOW_ELLIPSIS:
			value = CSS_TEXT_OVERFLOW_ELLIPSIS;
            break;
		case TEXT_OVERFLOW_STRING:
			value = CSS_TEXT_OVERFLOW_STRING;
			css__stylesheet_string_get(style->sheet, *((css_code_t *) style->bytecode), &strings);
			advance_bytecode(style, sizeof(css_code_t));
			break;
		}
	}

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state, 
                isInherit(opv))) {
		return set_text_overflow(state->computed, value, strings);
	}

	return CSS_OK;
}

css_error css__set_text_overflow_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	css_error error;

	error = set_text_overflow(style, hint->status, hint->data.string);

	if (hint->data.string != NULL)
		lwc_string_unref(hint->data.string);

	return error;
}

css_error css__initial_text_overflow(css_select_state *state)
{
	return set_text_overflow(state->computed, CSS_TEXT_OVERFLOW_CLIP, NULL);
}

css_error css__compose_text_overflow(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	(void)parent;
	lwc_string *value;
	uint8_t type = get_text_overflow(child, &value);
	return set_text_overflow(result, type, value);
}

