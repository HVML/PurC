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

css_error css__cascade_stroke(uint32_t opv, css_style *style,
		css_select_state *state)
{
    uint16_t value = CSS_STROKE_INHERIT;
    lwc_string *uri = NULL;
    css_color color;

    if (isInherit(opv) == false) {
        switch (getValue(opv)) {
        case STROKE_NONE:
            value = CSS_STROKE_NONE;
            break;
        case STROKE_CURRENT_COLOR:
            value = CSS_STROKE_CURRENT_COLOR;
            break;
        case STROKE_URI:
            value = CSS_STROKE_URI;
            css__stylesheet_string_get(style->sheet, *((css_code_t *) style->bytecode), &uri);
            advance_bytecode(style, sizeof(css_code_t));
            break;
        case STROKE_SET_COLOR:
            value = CSS_STROKE_SET_COLOR;
			color = *((css_color *) style->bytecode);
			advance_bytecode(style, sizeof(color));
            break;
        }
    }

    if (css__outranks_existing(getOpcode(opv),
            isImportant(opv), state, isInherit(opv))) {
        return set_stroke(state->computed, value, uri, color);
    }

    return CSS_OK;
}

css_error css__set_stroke_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	css_error error;

	error = set_stroke(style, hint->status, hint->data.string, 0);

	if (hint->data.string != NULL)
		lwc_string_unref(hint->data.string);

	return error;
}

css_error css__initial_stroke(css_select_state *state)
{
	return set_stroke(state->computed, CSS_STROKE_NOT_SET, NULL, 0);
}

css_error css__compose_stroke(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	lwc_string *url;
    css_color color;
	uint8_t type = get_stroke(child, &url, &color);

	if (type == CSS_STROKE_INHERIT) {
		type = get_stroke(parent, &url, &color);
	}

	return set_stroke(result, type, url, color);
}
