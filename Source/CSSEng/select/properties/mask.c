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

css_error css__cascade_mask(uint32_t opv, css_style *style,
		css_select_state *state)
{
	return css__cascade_uri_none(opv, style, state, set_mask);
}

css_error css__set_mask_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	css_error error;

	error = set_mask(style, hint->status, hint->data.string);

	if (hint->data.string != NULL)
		lwc_string_unref(hint->data.string);

	return error;
}

css_error css__initial_mask(css_select_state *state)
{
	return set_mask(state->computed,
			CSS_MASK_NONE, NULL);
}

css_error css__compose_mask(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	lwc_string *url;
	uint8_t type = get_mask(child, &url);

	if (type == CSS_MASK_INHERIT) {
		type = get_mask(parent, &url);
	}

	return set_mask(result, type, url);
}

