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
 * Parse list-style
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

css_error css__parse_flex(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style *result)
{
	int orig_ctx = *ctx;
	int prev_ctx;
	const css_token *token;
	css_error error;
	bool grow = true;
	bool shrink = true;
	bool basis = true;
	css_style *grow_style;
	css_style *shrink_style;
	css_style *basis_style;
	bool short_auto = false;
	bool short_none = false;
	bool match;

	/* Firstly, handle inherit */
	token = parserutils_vector_peek(vector, *ctx);
	if (token == NULL) 
		return CSS_INVALID;
		
	if (is_css_inherit(c, token)) {
		error = css_stylesheet_style_inherit(result,
				CSS_PROP_FLEX_GROW);
		if (error != CSS_OK) 
			return error;

		error = css_stylesheet_style_inherit(result,
				CSS_PROP_FLEX_SHRINK);

		if (error != CSS_OK) 
			return error;

		error = css_stylesheet_style_inherit(result,
				CSS_PROP_FLEX_BASIS);

		if (error == CSS_OK) 
			parserutils_vector_iterate(vector, ctx);

		return error;
	}
	
	/* allocate styles */
	error = css__stylesheet_style_create(c->sheet, &grow_style);
	if (error != CSS_OK) 
		return error;

	error = css__stylesheet_style_create(c->sheet, &shrink_style);
	if (error != CSS_OK) {
		css__stylesheet_style_destroy(grow_style);
		return error;
	}

	error = css__stylesheet_style_create(c->sheet, &basis_style);
	if (error != CSS_OK) {
		css__stylesheet_style_destroy(grow_style);
		css__stylesheet_style_destroy(shrink_style);
		return error;
	}

	/* Handle shorthand none, equivalent of flex: 0 0 auto; */
	if ((token->type == CSS_TOKEN_IDENT) &&
		(lwc_string_caseless_isequal(
			token->idata, c->strings[NONE],
			&match) == lwc_error_ok && match)) {
		short_none = true;
		parserutils_vector_iterate(vector, ctx);

	} else if ((token->type == CSS_TOKEN_IDENT) &&
		(lwc_string_caseless_isequal(
			token->idata, c->strings[AUTO],
			&match) == lwc_error_ok && match)) {
		/* Handle shorthand auto, equivalent of flex: 1 1 auto; */
		short_auto = true;
		parserutils_vector_iterate(vector, ctx);

	} else do {
		/* Attempt to parse the various longhand properties */
		prev_ctx = *ctx;
		error = CSS_OK;

		/* Ensure that we're not about to parse another inherit */
		token = parserutils_vector_peek(vector, *ctx);
		if (token != NULL && is_css_inherit(c, token)) {
			error = CSS_INVALID;
			goto css__parse_flex_cleanup;
		}

		if ((grow) && 
			   (error = css__parse_flex_grow(c, vector,
				ctx, grow_style)) == CSS_OK) {
			grow = false;
		} else if ((basis) && 
			   (error = css__parse_flex_basis(c, vector, 
				ctx, basis_style)) == CSS_OK) {
			basis = false;
		} else if ((shrink) && 
			   (error = css__parse_flex_shrink(c, vector, 
				ctx, shrink_style)) == CSS_OK) {
			shrink = false;
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
	if (grow) {
		error = css__stylesheet_style_appendOPV(grow_style, 
				CSS_PROP_FLEX_GROW, 0, FLEX_GROW_SET);
		if (error != CSS_OK)
			goto css__parse_flex_cleanup;

		css_fixed grow_num = short_auto ? INTTOFIX(1) : 0;
		error = css__stylesheet_style_append(grow_style, grow_num);
		if (error != CSS_OK)
			goto css__parse_flex_cleanup;
	}

	if (shrink) {
		error = css__stylesheet_style_appendOPV(shrink_style, 
				CSS_PROP_FLEX_SHRINK, 0, FLEX_SHRINK_SET);
		if (error != CSS_OK)
			goto css__parse_flex_cleanup;

		css_fixed shrink_num = short_none ? 0 : INTTOFIX(1);
		error = css__stylesheet_style_append(shrink_style, shrink_num);
		if (error != CSS_OK)
			goto css__parse_flex_cleanup;
	}

	if (basis) {
		/* Default is auto, but zero if grow or shrink are set */
		if (!grow || !shrink) {
			error = css__stylesheet_style_appendOPV(basis_style, 
					CSS_PROP_FLEX_BASIS, 0,
					FLEX_BASIS_SET);
			if (error != CSS_OK)
				goto css__parse_flex_cleanup;

			error = css__stylesheet_style_vappend(
					basis_style, 2, 0, UNIT_PX);
			if (error != CSS_OK)
				goto css__parse_flex_cleanup;

		} else {
			error = css__stylesheet_style_appendOPV(basis_style, 
					CSS_PROP_FLEX_BASIS, 0,
					FLEX_BASIS_AUTO);
			if (error != CSS_OK)
				goto css__parse_flex_cleanup;
		}
	}

	error = css__stylesheet_merge_style(result, grow_style);
	if (error != CSS_OK)
		goto css__parse_flex_cleanup;

	error = css__stylesheet_merge_style(result, shrink_style);
	if (error != CSS_OK)
		goto css__parse_flex_cleanup;

	error = css__stylesheet_merge_style(result, basis_style);

css__parse_flex_cleanup:

	css__stylesheet_style_destroy(basis_style);
	css__stylesheet_style_destroy(shrink_style);
	css__stylesheet_style_destroy(grow_style);

	if (error != CSS_OK)
		*ctx = orig_ctx;

	return error;
}

