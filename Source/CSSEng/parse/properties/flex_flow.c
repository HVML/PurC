/*
 * This file is part of CSSEng.
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 * Copyright 2017 Lucas Neves <lcneves@gmail.com>
 */

#include <assert.h>
#include <string.h>

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "parse/properties/properties.h"
#include "parse/properties/utils.h"

/**
 * Parse flex-flow
 *
 * \param c	  Parsing context
 * \param vector  Vector of tokens to process
 * \param ctx	  Pointer to vector iteration context
 * \param result  Pointer to location to receive resulting style
 * \return CSS_OK on success,
 *	   CSS_NOMEM on memory exhaustion,
 *	   CSS_INVALID if the input is not valid
 *
 * Post condition: \a *ctx is updated with the next token to process
 *		   If the input is invalid, then \a *ctx remains unchanged.
 */

css_error css__parse_flex_flow(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style *result)
{
	int orig_ctx = *ctx;
	int prev_ctx;
	const css_token *token;
	css_error error;
	bool direction = true;
	bool wrap = true;
	css_style *direction_style;
	css_style *wrap_style;

	/* Firstly, handle inherit */
	token = parserutils_vector_peek(vector, *ctx);
	if (token == NULL) 
		return CSS_INVALID;
		
	if (is_css_inherit(c, token)) {
		error = css_stylesheet_style_inherit(result,
				CSS_PROP_FLEX_DIRECTION);
		if (error != CSS_OK) 
			return error;

		error = css_stylesheet_style_inherit(result,
				CSS_PROP_FLEX_WRAP);

		if (error == CSS_OK) 
			parserutils_vector_iterate(vector, ctx);

		return error;
	} 

	/* allocate styles */
	error = css__stylesheet_style_create(c->sheet, &direction_style);
	if (error != CSS_OK) 
		return error;

	error = css__stylesheet_style_create(c->sheet, &wrap_style);
	if (error != CSS_OK) {
		css__stylesheet_style_destroy(direction_style);
		return error;
	}

	/* Attempt to parse the various longhand properties */
	do {
		prev_ctx = *ctx;
		error = CSS_OK;

		/* Ensure that we're not about to parse another inherit */
		token = parserutils_vector_peek(vector, *ctx);
		if (token != NULL && is_css_inherit(c, token)) {
			error = CSS_INVALID;
			goto css__parse_flex_flow_cleanup;
		}

		if ((wrap) && 
			   (error = css__parse_flex_wrap(c, vector, 
				ctx, wrap_style)) == CSS_OK) {
			wrap = false;
		} else if ((direction) && 
			   (error = css__parse_flex_direction(c, vector,
				ctx, direction_style)) == CSS_OK) {
			direction = false;
		}

		if (error == CSS_OK) {
			consumeWhitespace(vector, ctx);

			token = parserutils_vector_peek(vector, *ctx);
		} else {
			/* Forcibly cause loop to exit */
			token = NULL;
		}
	} while (*ctx != prev_ctx && token != NULL);


	/* defaults */
	if (direction) {
		error = css__stylesheet_style_appendOPV(direction_style,
				CSS_PROP_FLEX_DIRECTION,
				0, FLEX_DIRECTION_ROW);
		if (error != CSS_OK) {
			goto css__parse_flex_flow_cleanup;
		}
	}

	if (wrap) {
		error = css__stylesheet_style_appendOPV(wrap_style,
				CSS_PROP_FLEX_WRAP,
				0, FLEX_WRAP_NOWRAP);
		if (error != CSS_OK) {
			goto css__parse_flex_flow_cleanup;
		}
	}

	error = css__stylesheet_merge_style(result, direction_style);
	if (error != CSS_OK)
		goto css__parse_flex_flow_cleanup;

	error = css__stylesheet_merge_style(result, wrap_style);

css__parse_flex_flow_cleanup:

	css__stylesheet_style_destroy(wrap_style);
	css__stylesheet_style_destroy(direction_style);

	if (error != CSS_OK)
		*ctx = orig_ctx;

	return error;
}

