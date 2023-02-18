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

css_error css__cascade_line_break(uint32_t opv, css_style *style,
		css_select_state *state)
{
	uint16_t value = CSS_LINE_BREAK_INHERIT;

	UNUSED(style);

	if (isInherit(opv) == false) {
		switch (getValue(opv)) {
		case LINE_BREAK_AUTO:
			value = CSS_LINE_BREAK_AUTO;
			break;
		case LINE_BREAK_LOOSE:
			value = CSS_LINE_BREAK_LOOSE;
			break;
		case LINE_BREAK_NORMAL:
			value = CSS_LINE_BREAK_NORMAL;
			break;
		case LINE_BREAK_STRICT:
			value = CSS_LINE_BREAK_STRICT;
			break;
		case LINE_BREAK_ANYWHERE:
			value = CSS_LINE_BREAK_ANYWHERE;
			break;
		}
	}

	if (css__outranks_existing(getOpcode(opv), isImportant(opv), state,
                isInherit(opv))) {
		return set_line_break(state->computed, value);
	}

	return CSS_OK;
}

css_error css__set_line_break_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	return set_line_break(style, hint->status);
}

css_error css__initial_line_break(css_select_state *state)
{
	return set_line_break(state->computed, CSS_LINE_BREAK_NORMAL);
}

css_error css__compose_line_break(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	uint8_t type = get_line_break(child);

	if (type == CSS_LINE_BREAK_INHERIT) {
		type = get_line_break(parent);
	}

	return set_line_break(result, type);
}
