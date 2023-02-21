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

css_error css__cascade_fill(uint32_t opv, css_style *style,
		css_select_state *state)
{
    uint16_t value = CSS_FILL_INHERIT;
    lwc_string *uri = NULL;
    css_color color;

    if (isInherit(opv) == false) {
        switch (getValue(opv)) {
        case FILL_NONE:
            value = CSS_FILL_NONE;
            break;
        case FILL_CURRENT_COLOR:
            value = CSS_FILL_CURRENT_COLOR;
            break;
        case FILL_URI:
            value = CSS_FILL_URI;
            css__stylesheet_string_get(style->sheet, *((css_code_t *) style->bytecode), &uri);
            advance_bytecode(style, sizeof(css_code_t));
            break;
        case FILL_SET_COLOR:
            value = CSS_FILL_SET_COLOR;
			color = *((css_color *) style->bytecode);
			advance_bytecode(style, sizeof(color));
            break;
        }
    }

    if (css__outranks_existing(getOpcode(opv),
            isImportant(opv), state, isInherit(opv))) {
        return set_fill(state->computed, value, uri, color);
    }

    return CSS_OK;
}

css_error css__set_fill_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	css_error error;

	error = set_fill(style, hint->status, hint->data.string, 0);

	if (hint->data.string != NULL)
		lwc_string_unref(hint->data.string);

	return error;
}

css_error css__initial_fill(css_select_state *state)
{
	return set_fill(state->computed, CSS_FILL_NOT_SET, NULL, 0);
}

css_error css__compose_fill(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	lwc_string *url;
    css_color color;
	uint8_t type = get_fill(child, &url, &color);

	if (type == CSS_FILL_INHERIT) {
		type = get_fill(parent, &url, &color);
	}

	return set_fill(result, type, url, color);
}
