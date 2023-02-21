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

css_error css__cascade_marker_end(uint32_t opv, css_style *style,
		css_select_state *state)
{
	return css__cascade_uri_none(opv, style, state, set_marker_end);
}

css_error css__set_marker_end_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	css_error error;

	error = set_marker_end(style, hint->status, hint->data.string);

	if (hint->data.string != NULL)
		lwc_string_unref(hint->data.string);

	return error;
}

css_error css__initial_marker_end(css_select_state *state)
{
	return set_marker_end(state->computed,
			CSS_MARKER_END_NONE, NULL);
}

css_error css__compose_marker_end(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	lwc_string *url;
	uint8_t type = get_marker_end(child, &url);

	if (type == CSS_MARKER_END_INHERIT) {
		type = get_marker_end(parent, &url);
	}

	return set_marker_end(result, type, url);
}

