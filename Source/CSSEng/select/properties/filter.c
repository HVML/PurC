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

css_error css__cascade_filter(uint32_t opv, css_style *style,
		css_select_state *state)
{
	return css__cascade_uri_none(opv, style, state, set_filter);
}

css_error css__set_filter_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	css_error error;

	error = set_filter(style, hint->status, hint->data.string);

	if (hint->data.string != NULL)
		lwc_string_unref(hint->data.string);

	return error;
}

css_error css__initial_filter(css_select_state *state)
{
	return set_filter(state->computed, CSS_FILTER_NONE, NULL);
}

css_error css__compose_filter(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	lwc_string *url;
	uint8_t type = get_filter(child, &url);

	if (type == CSS_FILTER_INHERIT) {
		type = get_filter(parent, &url);
	}

	return set_filter(result, type, url);
}

