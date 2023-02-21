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

css_error css__cascade_clip_path(uint32_t opv, css_style *style,
		css_select_state *state)
{
	return css__cascade_uri_none(opv, style, state, set_clip_path);
}

css_error css__set_clip_path_from_hint(const css_hint *hint,
		css_computed_style *style)
{
	css_error error;

	error = set_clip_path(style, hint->status, hint->data.string);

	if (hint->data.string != NULL)
		lwc_string_unref(hint->data.string);

	return error;
}

css_error css__initial_clip_path(css_select_state *state)
{
	return set_clip_path(state->computed,
			CSS_CLIP_PATH_NONE, NULL);
}

css_error css__compose_clip_path(const css_computed_style *parent,
		const css_computed_style *child,
		css_computed_style *result)
{
	lwc_string *url;
	uint8_t type = get_clip_path(child, &url);

	if (type == CSS_CLIP_PATH_INHERIT) {
		type = get_clip_path(parent, &url);
	}

	return set_clip_path(result, type, url);
}

