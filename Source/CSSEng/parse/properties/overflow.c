/*
 * This file is part of CSSEng.
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Michael Drake <tlsa@netsurf-browser.org>
 */

#include <assert.h>
#include <string.h>

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "parse/properties/properties.h"
#include "parse/properties/utils.h"

/**
 * Parse overflow shorthand
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
css_error css__parse_overflow(css_language *c,
		const parserutils_vector *vector, int *ctx,
		css_style *result)
{
	int orig_ctx = *ctx;
	css_error error1, error2 = CSS_OK;
	const css_token *token;
	bool match;

	token = parserutils_vector_iterate(vector, ctx);
	if ((token == NULL) || ((token->type != CSS_TOKEN_IDENT))) {
		*ctx = orig_ctx;
		return CSS_INVALID;
	}

	if ((lwc_string_caseless_isequal(token->idata,
			c->strings[INHERIT], &match) == lwc_error_ok &&
			match)) {
		error1 = css_stylesheet_style_inherit(result,
				CSS_PROP_OVERFLOW_X);
		error2 = css_stylesheet_style_inherit(result,
				CSS_PROP_OVERFLOW_Y);

	} else if ((lwc_string_caseless_isequal(token->idata,
			c->strings[VISIBLE], &match) == lwc_error_ok &&
			match)) {
		error1 = css__stylesheet_style_appendOPV(result,
				CSS_PROP_OVERFLOW_X, 0, OVERFLOW_VISIBLE);
		error2 = css__stylesheet_style_appendOPV(result,
				CSS_PROP_OVERFLOW_Y, 0, OVERFLOW_VISIBLE);

	} else if ((lwc_string_caseless_isequal(token->idata,
			c->strings[HIDDEN], &match) == lwc_error_ok &&
			match)) {
		error1 = css__stylesheet_style_appendOPV(result,
				CSS_PROP_OVERFLOW_X, 0, OVERFLOW_HIDDEN);
		error2 = css__stylesheet_style_appendOPV(result,
				CSS_PROP_OVERFLOW_Y, 0, OVERFLOW_HIDDEN);

	} else if ((lwc_string_caseless_isequal(token->idata,
			c->strings[SCROLL], &match) == lwc_error_ok &&
			match)) {
		error1 = css__stylesheet_style_appendOPV(result,
				CSS_PROP_OVERFLOW_X, 0, OVERFLOW_SCROLL);
		error2 = css__stylesheet_style_appendOPV(result,
				CSS_PROP_OVERFLOW_Y, 0, OVERFLOW_SCROLL);

	} else if ((lwc_string_caseless_isequal(token->idata,
			c->strings[AUTO], &match) == lwc_error_ok &&
			match)) {
		error1 = css__stylesheet_style_appendOPV(result,
				CSS_PROP_OVERFLOW_X, 0, OVERFLOW_AUTO);
		error2 = css__stylesheet_style_appendOPV(result,
				CSS_PROP_OVERFLOW_Y, 0, OVERFLOW_AUTO);

	} else {
		error1 = CSS_INVALID;
	}

	if (error2 != CSS_OK)
		error1 = error2;

	if (error1 != CSS_OK)
		*ctx = orig_ctx;

	return error1;
}

