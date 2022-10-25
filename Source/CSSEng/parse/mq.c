/*
 * This file is part of CSSEng.
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 * Copyright 2016 John-Mark Bell <jmb@netsurf-browser.org>
 */

/* https://drafts.csswg.org/mediaqueries/ */

#include <string.h>

#include "csseng-fpmath.h"

#include "select/stylesheet.h"
#include "bytecode/bytecode.h"
#include "parse/mq.h"
#include "parse/properties/utils.h"
#include "utils/utils.h"

static void css__mq_value_destroy(css_mq_value *value)
{
	assert(value != NULL);

	if (value->type == CSS_MQ_VALUE_TYPE_IDENT) {
		lwc_string_unref(value->data.ident);
	}
}

static void css__mq_feature_destroy(css_mq_feature *feature)
{
	if (feature != NULL) {
		lwc_string_unref(feature->name);
		css__mq_value_destroy(&feature->value);
		css__mq_value_destroy(&feature->value2);
		free(feature);
	}
}

static void css__mq_cond_or_feature_destroy(
		css_mq_cond_or_feature *cond_or_feature);

static void css__mq_cond_destroy(css_mq_cond *cond)
{
	if (cond != NULL) {
		for (uint32_t i = 0; i < cond->nparts; i++) {
			css__mq_cond_or_feature_destroy(cond->parts[i]);
		}
		free(cond->parts);
		free(cond);
	}
}

static void css__mq_cond_or_feature_destroy(
		css_mq_cond_or_feature *cond_or_feature)
{
	if (cond_or_feature != NULL) {
		switch (cond_or_feature->type) {
		case CSS_MQ_FEATURE:
			css__mq_feature_destroy(cond_or_feature->data.feat);
			break;
		case CSS_MQ_COND:
			css__mq_cond_destroy(cond_or_feature->data.cond);
			break;
		}
		free(cond_or_feature);
	}
}

void css__mq_query_destroy(css_mq_query *media)
{
	while (media != NULL) {
		css_mq_query *next = media->next;

		css__mq_cond_destroy(media->cond);
		free(media);

		media = next;
	}
}

static css_error mq_parse_condition(lwc_string **strings,
		const parserutils_vector *vector, int *ctx,
		bool permit_or, css_mq_cond **cond);

static css_error mq_parse_ratio(
		const parserutils_vector *vector, int *ctx,
		const css_token *numerator, css_fixed *ratio)
{
	const css_token *token;
	css_fixed num, den;
	size_t num_len, den_len;

	/* NUMBER ws* '/' ws* NUMBER */

	/* numerator, ws* already consumed */

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || tokenIsChar(token, '/') == false) {
		return CSS_INVALID;
	}

	consumeWhitespace(vector, ctx);

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || token->type != CSS_TOKEN_NUMBER) {
		return CSS_INVALID;
	}

	num = css__number_from_lwc_string(numerator->idata, true, &num_len);
	den = css__number_from_lwc_string(token->idata, true, &den_len);

	if (num == 0 || den == 0) {
		return CSS_INVALID;
	}

	*ratio = css_divide_fixed(num, den);

	return CSS_OK;
}

static css_error mq_create_feature(
		lwc_string *name,
		css_mq_feature **feature)
{
	css_mq_feature *f;

	f = malloc(sizeof(*f));
	if (f == NULL) {
		return CSS_NOMEM;
	}

	memset(f, 0, sizeof(*f));

	if (lwc_string_tolower(name, &f->name) != lwc_error_ok) {
		free(f);
		return CSS_NOMEM;
	}

	*feature = f;

	return CSS_OK;
}

static css_error mq_populate_value(css_mq_value *value,
		const css_token *token)
{
	if (token->type == CSS_TOKEN_NUMBER) {
		size_t num_len;
		value->type = CSS_MQ_VALUE_TYPE_NUM;
		value->data.num_or_ratio = css__number_from_lwc_string(
				token->idata, false, &num_len);
	} else if (token->type == CSS_TOKEN_DIMENSION) {
		size_t len = lwc_string_length(token->idata);
		const char *data = lwc_string_data(token->idata);
		uint32_t unit = UNIT_PX;
		size_t consumed;
		css_error error;

		value->type = CSS_MQ_VALUE_TYPE_DIM;
		value->data.dim.len = css__number_from_lwc_string(
				token->idata, false, &consumed);
		error = css__parse_unit_keyword(data + consumed, len - consumed,
				&unit);
		if (error != CSS_OK) {
			return error;
		}
		value->data.dim.unit = unit;
	} else if (token->type == CSS_TOKEN_IDENT) {
		value->type = CSS_MQ_VALUE_TYPE_IDENT;
		value->data.ident = lwc_string_ref(token->idata);
	}

	return CSS_OK;
}

static css_error mq_parse_op(const css_token *token,
		css_mq_feature_op *op)
{
	size_t len;
	const char *data;

	if (token == NULL || token->type != CSS_TOKEN_CHAR)
		return CSS_INVALID;

	len = lwc_string_length(token->idata);
	data = lwc_string_data(token->idata);

	if (len == 2) {
		if (strncasecmp(data, "<=", 2) == 0)
			*op = CSS_MQ_FEATURE_OP_LTE;
		else if (strncasecmp(data, ">=", 2) == 0)
			*op = CSS_MQ_FEATURE_OP_GTE;
		else
			return CSS_INVALID;
	} else if (len == 1) {
		if (*data == '<')
			*op = CSS_MQ_FEATURE_OP_LT;
		else if (*data == '=')
			*op = CSS_MQ_FEATURE_OP_EQ;
		else if (*data == '>')
			*op = CSS_MQ_FEATURE_OP_GT;
		else
			return CSS_INVALID;
	} else {
		return CSS_INVALID;
	}

	return CSS_OK;
}


/**
 * Convert level 3 ranged descriptors into level 4 style.
 *
 * Helper for \ref mq_parse_range().
 *
 * \param[in] feature  Range feature to convert.
 * \return CSS_OK, or error code.
 *
 * This detects the min- and max- prefixes, strips them, and converts
 * the operator.
 */
static css_error mq_parse_range__convert_to_level_4(
		css_mq_feature *feature)
{
	lwc_string *new_name;
	const char *name = lwc_string_data(feature->name);

	if (feature->op != CSS_MQ_FEATURE_OP_EQ ||
			lwc_string_length(feature->name) <= 4) {
		return CSS_OK;
	}

	if (name[0] == 'm' && name[3] == '-') {
		if (name[1] == 'i' && name[2] == 'n') {
			if (lwc_intern_substring(feature->name,
					4, lwc_string_length(feature->name) - 4,
					&new_name) != lwc_error_ok) {
				return CSS_NOMEM;
			}
			lwc_string_unref(feature->name);
			feature->name = new_name;

			feature->op = CSS_MQ_FEATURE_OP_LTE;

		} else if (name[1] == 'a' && name[2] == 'x') {
			if (lwc_intern_substring(feature->name,
					4, lwc_string_length(feature->name) - 4,
					&new_name) != lwc_error_ok) {
				return CSS_NOMEM;
			}
			lwc_string_unref(feature->name);
			feature->name = new_name;

			feature->op = CSS_MQ_FEATURE_OP_GTE;
		}
	}

	return CSS_OK;
}

static css_error mq_parse_range(lwc_string **strings,
		const parserutils_vector *vector, int *ctx,
		const css_token *name_or_value,
		css_mq_feature **feature)
{
	const css_token *token, *value_or_name, *name = NULL, *value2 = NULL;
	css_mq_feature *result;
	css_mq_feature_op op, op2;
	css_fixed ratio, ratio2;
	bool name_first = false, value_is_ratio = false, value2_is_ratio = false, match;
	css_error error;

	/* <mf-range> = <mf-name> [ '<' | '>' ]? '='? <mf-value>
	 *            | <mf-value> [ '<' | '>' ]? '='? <mf-name>
	 *            | <mf-value> '<' '='? <mf-name> '<' '='? <mf-value>
	 *            | <mf-value> '>' '='? <mf-name> '>' '='? <mf-value>
	 */

	if (name_or_value == NULL || (name_or_value->type != CSS_TOKEN_NUMBER &&
			name_or_value->type != CSS_TOKEN_DIMENSION &&
			name_or_value->type != CSS_TOKEN_IDENT)) {
		return CSS_INVALID;
	}

	consumeWhitespace(vector, ctx);

	/* Name-or-value */
	if (name_or_value->type == CSS_TOKEN_NUMBER &&
			tokenIsChar(parserutils_vector_peek(vector, *ctx), '/')) {
		/* ratio */
		error = mq_parse_ratio(vector, ctx, name_or_value, &ratio);
		if (error != CSS_OK) {
			return error;
		}

		consumeWhitespace(vector, ctx);

		value_is_ratio = true;
	} else if (name_or_value->type == CSS_TOKEN_IDENT &&
			lwc_string_caseless_isequal(name_or_value->idata,
				strings[INFINITE], &match) == lwc_error_ok && 
			match == false) {
		/* The only ident permitted for mf-value is 'infinite', thus must have name */
		name = name_or_value;
		name_first = true;
	}

	/* Op */
	token = parserutils_vector_iterate(vector, ctx);
	error = mq_parse_op(token, &op);
	if (error != CSS_OK) {
		return error;
	}

	consumeWhitespace(vector, ctx);

	/* Value-or-name */
	value_or_name = parserutils_vector_iterate(vector, ctx);
	if (value_or_name == NULL || (value_or_name->type != CSS_TOKEN_NUMBER &&
			value_or_name->type != CSS_TOKEN_DIMENSION &&
			value_or_name->type != CSS_TOKEN_IDENT)) {
		return CSS_INVALID;
	}

	if (name == NULL) {
		if (value_or_name->type != CSS_TOKEN_IDENT) {
			return CSS_INVALID;
		} else {
			name = value_or_name;
		}
	}

	consumeWhitespace(vector, ctx);

	if (value_or_name->type == CSS_TOKEN_NUMBER &&
			tokenIsChar(parserutils_vector_peek(vector, *ctx), '/')) {
		/* ratio */
		error = mq_parse_ratio(vector, ctx, token, &ratio);
		if (error != CSS_OK) {
			return error;
		}

		consumeWhitespace(vector, ctx);

		value_is_ratio = true;
	}

	token = parserutils_vector_peek(vector, *ctx);
	if (name_first == false && token != NULL && tokenIsChar(token, ')') == false) {
		/* Op2 */
		token = parserutils_vector_iterate(vector, ctx);
		error = mq_parse_op(token, &op2);
		if (error != CSS_OK) {
			return error;
		}

		consumeWhitespace(vector, ctx);

		/* Validate operators: must both be LT(E) or GT(E) */
		if (op == CSS_MQ_FEATURE_OP_LT || op == CSS_MQ_FEATURE_OP_LTE) {
			if (op2 != CSS_MQ_FEATURE_OP_LT && op2 != CSS_MQ_FEATURE_OP_LTE) {
				return CSS_INVALID;
			}
		} else if (op == CSS_MQ_FEATURE_OP_GT || op == CSS_MQ_FEATURE_OP_GTE) {
		       if (op2 != CSS_MQ_FEATURE_OP_GT && op2 != CSS_MQ_FEATURE_OP_GTE) {
				return CSS_INVALID;
			}
		} else {
			return CSS_INVALID;
		}

		/* Value2 */
		value2 = parserutils_vector_iterate(vector, ctx);
		if (value2 == NULL || (value2->type != CSS_TOKEN_NUMBER &&
				value2->type != CSS_TOKEN_DIMENSION &&
				value2->type != CSS_TOKEN_IDENT)) {
			return CSS_INVALID;
		}

		consumeWhitespace(vector, ctx);

		if (value_or_name->type == CSS_TOKEN_NUMBER &&
				tokenIsChar(parserutils_vector_peek(vector, *ctx), '/')) {
			/* ratio */
			error = mq_parse_ratio(vector, ctx, token, &ratio2);
			if (error != CSS_OK) {
				return error;
			}

			consumeWhitespace(vector, ctx);

			value2_is_ratio = true;
		}
	}

	error = mq_create_feature(name->idata, &result);
	if (error != CSS_OK) {
		return error;
	}
	if (name_first) {
		/* Invert operator */
		if (op == CSS_MQ_FEATURE_OP_LT) {
			op = CSS_MQ_FEATURE_OP_GTE;
		} else if (op == CSS_MQ_FEATURE_OP_LTE) {
			op = CSS_MQ_FEATURE_OP_GT;
		} else if (op == CSS_MQ_FEATURE_OP_GT) {
			op = CSS_MQ_FEATURE_OP_LTE;
		} else if (op == CSS_MQ_FEATURE_OP_GTE) {
			op = CSS_MQ_FEATURE_OP_LT;
		}
	}
	result->op = op;
	if (value_is_ratio) {
		result->value.type = CSS_MQ_VALUE_TYPE_RATIO;
		result->value.data.num_or_ratio = ratio;
	} else {
		/* num/dim/ident */
		error = mq_populate_value(&result->value, name_or_value);
		if (error != CSS_OK) {
			css__mq_feature_destroy(result);
			return error;
		}
	}
	if (value2 != NULL) {
		result->op2 = op2;
		if (value2_is_ratio) {
			result->value2.type = CSS_MQ_VALUE_TYPE_RATIO;
			result->value2.data.num_or_ratio = ratio;
		} else {
			/* num/dim/ident */
			error = mq_populate_value(&result->value2, value2);
			if (error != CSS_OK) {
				css__mq_feature_destroy(result);
				return error;
			}
		}
	}

	*feature = result;

	return CSS_OK;
}

static css_error mq_parse_media_feature(lwc_string **strings,
		const parserutils_vector *vector, int *ctx,
		css_mq_feature **feature)
{
	const css_token *name_or_value, *token;
	css_mq_feature *result;
	css_error error;

	/* <media-feature> = ( [ <mf-plain> | <mf-boolean> | <mf-range> ] )
	 * <mf-plain> = <mf-name> : <mf-value>
	 * <mf-boolean> = <mf-name>
	 * <mf-name> = <ident>
	 * <mf-value> = <number> | <dimension> | <ident> | <ratio>
	 */

	/* ( already consumed */

	consumeWhitespace(vector, ctx);

	name_or_value = parserutils_vector_iterate(vector, ctx);
	if (name_or_value == NULL)
		return CSS_INVALID;

	if (name_or_value->type == CSS_TOKEN_IDENT) {
		consumeWhitespace(vector, ctx);

		token = parserutils_vector_peek(vector, *ctx);
		if (tokenIsChar(token, ')')) {
			/* mf-boolean */
			error = mq_create_feature(name_or_value->idata, &result);
			if (error != CSS_OK) {
				return error;
			}

			result->op = CSS_MQ_FEATURE_OP_BOOL;
		} else if (tokenIsChar(token, ':')) {
			/* mf-plain */
			parserutils_vector_iterate(vector, ctx);

			consumeWhitespace(vector, ctx);

			token = parserutils_vector_iterate(vector, ctx);
			if (token == NULL || (token->type != CSS_TOKEN_NUMBER &&
					token->type != CSS_TOKEN_DIMENSION &&
					token->type != CSS_TOKEN_IDENT)) {
				return CSS_INVALID;
			}

			consumeWhitespace(vector, ctx);

			error = mq_create_feature(name_or_value->idata, &result);
			if (error != CSS_OK) {
				return error;
			}
			result->op = CSS_MQ_FEATURE_OP_EQ;

			if (token->type == CSS_TOKEN_NUMBER &&
					tokenIsChar(parserutils_vector_peek(vector, *ctx), '/')) {
				/* ratio */
				css_fixed ratio;

				error = mq_parse_ratio(vector, ctx, token, &ratio);
				if (error != CSS_OK) {
					css__mq_feature_destroy(result);
					return error;
				}

				result->value.type = CSS_MQ_VALUE_TYPE_RATIO;
				result->value.data.num_or_ratio = ratio;
			} else {
				/* num/dim/ident */
				error = mq_populate_value(&result->value, token);
				if (error != CSS_OK) {
					css__mq_feature_destroy(result);
					return error;
				}
			}

			consumeWhitespace(vector, ctx);

			error = mq_parse_range__convert_to_level_4(result);
			if (error != CSS_OK) {
				css__mq_feature_destroy(result);
				return error;
			}
		} else {
			/* mf-range */
			error = mq_parse_range(strings, vector, ctx,
					name_or_value, &result);
			if (error != CSS_OK) {
				return error;
			}

			consumeWhitespace(vector, ctx);
		}
	} else {
		/* mf-range */
		error = mq_parse_range(strings, vector, ctx,
				name_or_value, &result);
		if (error != CSS_OK) {
			return error;
		}

		consumeWhitespace(vector, ctx);
	}

	token = parserutils_vector_iterate(vector, ctx);
	if (tokenIsChar(token, ')') == false) {
		css__mq_feature_destroy(result);
		return CSS_INVALID;
	}

	*feature = result;

	return CSS_OK;
}

/*
 * Consume any value
 *
 * CSS Syntax Module Level 3: 8.2
 */
static css_error mq_parse_consume_any_value(lwc_string **strings,
		const parserutils_vector *vector, int *ctx,
		bool until, const char until_char)
{
	const css_token *token;
	css_error error;

	while (true) {
		consumeWhitespace(vector, ctx);

		token = parserutils_vector_iterate(vector, ctx);
		if (token == NULL) {
			return CSS_INVALID;
		}

		switch (token->type) {
		case CSS_TOKEN_INVALID_STRING:
			return CSS_INVALID;

		case CSS_TOKEN_CHAR:
			if (until && tokenIsChar(token, until_char)) {
				/* Found matching close bracket */
				return CSS_OK;

			} else if (tokenIsChar(token, ')') ||
			           tokenIsChar(token, ']') ||
			           tokenIsChar(token, '}')) {
				/* Non-matching close bracket */
				return CSS_INVALID;
			}
			if (tokenIsChar(token, '(')) {
				/* Need to consume until matching bracket. */
				error = mq_parse_consume_any_value(strings,
						vector, ctx, true, ')');
				if (error != CSS_OK) {
					return error;
				}
			} else if (tokenIsChar(token, '[')) {
				/* Need to consume until matching bracket. */
				error = mq_parse_consume_any_value(strings,
						vector, ctx, true, ']');
				if (error != CSS_OK) {
					return error;
				}
			} else if (tokenIsChar(token, '{')) {
				/* Need to consume until matching bracket. */
				error = mq_parse_consume_any_value(strings,
						vector, ctx, true, '}');
				if (error != CSS_OK) {
					return error;
				}
			}
			break;

		default:
			break;
		}
	}

	return CSS_OK;
}

static css_error mq_parse_general_enclosed(lwc_string **strings,
		const parserutils_vector *vector, int *ctx)
{
	const css_token *token;
	css_error error;

	/* <general-enclosed> = [ <function-token> <any-value> ) ]
	 *                    | ( <ident> <any-value> )
	 */

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL) {
		return CSS_INVALID;
	}

	switch (token->type) {
	case CSS_TOKEN_FUNCTION:
		error = mq_parse_consume_any_value(strings, vector, ctx,
				true, ')');
		if (error != CSS_OK) {
			return error;
		}

		token = parserutils_vector_peek(vector, *ctx);
		if (!tokenIsChar(token, ')')) {
			return CSS_INVALID;
		}
		break;

	case CSS_TOKEN_IDENT:
		error = mq_parse_consume_any_value(strings, vector, ctx,
				false, '\0');
		if (error != CSS_OK) {
			return error;
		}
		break;

	default:
		return CSS_INVALID;
	}

	return CSS_OK;
}

static css_error mq_parse_media_in_parens(lwc_string **strings,
		const parserutils_vector *vector, int *ctx,
		css_mq_cond_or_feature **cond_or_feature)
{
	const css_token *token;
	bool match;
	int old_ctx;
	css_mq_cond_or_feature *result = NULL;
	css_error error = CSS_OK;

	/* <media-in-parens> = ( <media-condition> ) | <media-feature> | <general-enclosed>
	 */

	//LPAREN -> condition-or-feature
	//	  "not" or LPAREN -> condition
	//	  IDENT | NUMBER | DIMENSION | RATIO -> feature

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || tokenIsChar(token, '(') == false) {
		return CSS_INVALID;
	}

	consumeWhitespace(vector, ctx);

	token = parserutils_vector_peek(vector, *ctx);
	if (token == NULL) {
		return CSS_INVALID;
	}

	old_ctx = *ctx;

	if (tokenIsChar(token, '(') || (token->type == CSS_TOKEN_IDENT &&
			lwc_string_caseless_isequal(token->idata,
				strings[NOT], &match) == lwc_error_ok &&
			match)) {
		css_mq_cond *cond;
		error = mq_parse_condition(strings, vector, ctx, true, &cond);
		if (error == CSS_OK) {
			token = parserutils_vector_iterate(vector, ctx);
			if (tokenIsChar(token, ')') == false) {
				return CSS_INVALID;
			}

			result = malloc(sizeof(*result));
			if (result == NULL) {
				css__mq_cond_destroy(cond);
				return CSS_NOMEM;
			}
			memset(result, 0, sizeof(*result));
			result->type = CSS_MQ_COND;
			result->data.cond = cond;
			*cond_or_feature = result;
			return CSS_OK;
		}
	} else if (token->type == CSS_TOKEN_IDENT ||
			token->type == CSS_TOKEN_NUMBER ||
			token->type == CSS_TOKEN_DIMENSION) {
		css_mq_feature *feature;
		error = mq_parse_media_feature(strings, vector, ctx, &feature);
		if (error == CSS_OK) {
			result = malloc(sizeof(*result));
			if (result == NULL) {
				css__mq_feature_destroy(feature);
				return CSS_NOMEM;
			}
			memset(result, 0, sizeof(*result));
			result->type = CSS_MQ_FEATURE;
			result->data.feat = feature;
			*cond_or_feature = result;
			return CSS_OK;
		}
	}

	*ctx = old_ctx;
	error = mq_parse_general_enclosed(strings, vector, ctx);
	if (error != CSS_OK) {
		return error;
	}

	*cond_or_feature = NULL;
	return CSS_OK;
}

static css_error mq_parse_condition(lwc_string **strings,
		const parserutils_vector *vector, int *ctx,
		bool permit_or, css_mq_cond **cond)
{
	const css_token *token;
	bool match = false;
	int op = 0; /* Will be AND | OR once we've had one */
	css_mq_cond_or_feature *cond_or_feature, **parts;
	css_mq_cond *result;
	css_error error;

	/* <media-condition> = <media-not> | <media-in-parens> [ <media-and>* | <media-or>* ]
	 * <media-condition-without-or> = <media-not> | <media-in-parens> <media-and>*
	 * <media-not> = not <media-in-parens>
	 * <media-and> = and <media-in-parens>
	 * <media-or> = or <media-in-parens>
	 */

	token = parserutils_vector_peek(vector, *ctx);
	if (token == NULL ||
			(tokenIsChar(token, '(') == false &&
			token->type != CSS_TOKEN_IDENT &&
			lwc_string_caseless_isequal(token->idata,
				strings[NOT], &match) != lwc_error_ok &&
			match == false)) {
		return CSS_INVALID;
	}

	result = malloc(sizeof(*result));
	if (result == NULL) {
		return CSS_NOMEM;
	}
	memset(result, 0, sizeof(*result));

	if (tokenIsChar(token, '(') == false) {
		/* Must be "not" */
		parserutils_vector_iterate(vector, ctx);
		consumeWhitespace(vector, ctx);

		error = mq_parse_media_in_parens(strings,
				vector, ctx, &cond_or_feature);
		if (error != CSS_OK) {
			css__mq_cond_destroy(result);
			return CSS_INVALID;
		}

		result->negate = 1;
		result->parts = malloc(sizeof(*result->parts));
		if (result->parts == NULL) {
			css__mq_cond_or_feature_destroy(cond_or_feature);
			css__mq_cond_destroy(result);
			return CSS_NOMEM;
		}
		result->nparts = 1;
		result->parts[0] = cond_or_feature;

		*cond = result;

		return CSS_OK;
	}

	/* FOLLOW(media-condition) := RPAREN | COMMA | EOF */
	while (token != NULL && tokenIsChar(token, ')') == false &&
			tokenIsChar(token, ',') == false) {
		error = mq_parse_media_in_parens(strings, vector, ctx,
				&cond_or_feature);
		if (error != CSS_OK) {
			css__mq_cond_destroy(result);
			return CSS_INVALID;
		}

		parts = realloc(result->parts,
				(result->nparts+1)*sizeof(*result->parts));
		if (parts == NULL) {
			css__mq_cond_or_feature_destroy(cond_or_feature);
			css__mq_cond_destroy(result);
			return CSS_NOMEM;
		}
		parts[result->nparts] = cond_or_feature;
		result->parts = parts;
		result->nparts++;

		consumeWhitespace(vector, ctx);

		token = parserutils_vector_peek(vector, *ctx);
		if (token != NULL && tokenIsChar(token, ')') == false &&
				tokenIsChar(token, ',') == false) {
			if (token->type != CSS_TOKEN_IDENT) {
				css__mq_cond_destroy(result);
				return CSS_INVALID;
			} else if (lwc_string_caseless_isequal(token->idata,
					strings[AND], &match) == lwc_error_ok &&
					match) {
				if (op != 0 && op != AND) {
					css__mq_cond_destroy(result);
					return CSS_INVALID;
				}
				op = AND;
			} else if (lwc_string_caseless_isequal(token->idata,
						strings[OR], &match) == lwc_error_ok &&
					match) {
				if (permit_or == false || (op != 0 && op != OR)) {
					css__mq_cond_destroy(result);
					return CSS_INVALID;
				}
				op = OR;
			} else {
				/* Neither AND nor OR */
				css__mq_cond_destroy(result);
				return CSS_INVALID;
			}

			parserutils_vector_iterate(vector, ctx);
			consumeWhitespace(vector, ctx);
		}
	}

	if (op == OR) {
		result->op = 1;
	}

	*cond = result;

	return CSS_OK;
}

/**
 * Parse a media query type.
 */
static uint64_t mq_parse_type(lwc_string **strings, lwc_string *type)
{
	bool match;

	if (type == NULL) {
		return CSS_MEDIA_ALL;
	} else if (lwc_string_caseless_isequal(
			type, strings[AURAL],
			&match) == lwc_error_ok && match) {
		return CSS_MEDIA_AURAL;
	} else if (lwc_string_caseless_isequal(
			type, strings[BRAILLE],
			&match) == lwc_error_ok && match) {
		return CSS_MEDIA_BRAILLE;
	} else if (lwc_string_caseless_isequal(
			type, strings[EMBOSSED],
			&match) == lwc_error_ok && match) {
		return CSS_MEDIA_EMBOSSED;
	} else if (lwc_string_caseless_isequal(
			type, strings[HANDHELD],
			&match) == lwc_error_ok && match) {
		return CSS_MEDIA_HANDHELD;
	} else if (lwc_string_caseless_isequal(
			type, strings[PRINT],
			&match) == lwc_error_ok && match) {
		return CSS_MEDIA_PRINT;
	} else if (lwc_string_caseless_isequal(
			type, strings[PROJECTION],
			&match) == lwc_error_ok && match) {
		return CSS_MEDIA_PROJECTION;
	} else if (lwc_string_caseless_isequal(
			type, strings[SCREEN],
			&match) == lwc_error_ok && match) {
		return CSS_MEDIA_SCREEN;
	} else if (lwc_string_caseless_isequal(
			type, strings[SPEECH],
			&match) == lwc_error_ok && match) {
		return CSS_MEDIA_SPEECH;
	} else if (lwc_string_caseless_isequal(
			type, strings[TTY],
			&match) == lwc_error_ok && match) {
		return CSS_MEDIA_TTY;
	} else if (lwc_string_caseless_isequal(
			type, strings[TV],
			&match) == lwc_error_ok && match) {
		return CSS_MEDIA_TV;
	} else if (lwc_string_caseless_isequal(
			type, strings[ALL],
			&match) == lwc_error_ok && match) {
		return CSS_MEDIA_ALL;
	}

	return 0;
}

static css_error mq_parse_media_query(lwc_string **strings,
		const parserutils_vector *vector, int *ctx,
		css_mq_query **query)
{
	const css_token *token;
	bool match, is_condition = false;
	css_mq_query *result;
	css_error error;

	/* <media-query> = <media-condition>
	 *               | [ not | only ]? <media-type> [ and <media-condition-without-or> ]?
	 * <media-type> = <ident> (except "not", "and", "or", "only")
	 */

	// LPAREN -> media-condition
	//    not LPAREN -> media-condition

	consumeWhitespace(vector, ctx);

	token = parserutils_vector_peek(vector, *ctx);
	if (tokenIsChar(token, '(')) {
		is_condition = true;
	} else if (token->type == CSS_TOKEN_IDENT &&
			lwc_string_caseless_isequal(token->idata,
				strings[NOT], &match) == lwc_error_ok &&
				match) {
		int old_ctx = *ctx;

		parserutils_vector_iterate(vector, ctx);
		consumeWhitespace(vector, ctx);

		token = parserutils_vector_peek(vector, *ctx);
		if (tokenIsChar(token, '(')) {
			is_condition = true;
		}

		*ctx = old_ctx;
	}

	result = malloc(sizeof(*result));
	if (result == NULL) {
		return CSS_NOMEM;
	}
	memset(result, 0, sizeof(*result));

	if (is_condition) {
		/* media-condition */
		error = mq_parse_condition(strings, vector, ctx, true,
				&result->cond);
		if (error != CSS_OK) {
			free(result);
			return error;
		}

		goto finished;
	}

	token = parserutils_vector_iterate(vector, ctx);
	if (token == NULL || token->type != CSS_TOKEN_IDENT) {
		free(result);
		return CSS_INVALID;
	}

	if (lwc_string_caseless_isequal(token->idata,
			strings[NOT], &match) == lwc_error_ok && match) {
		result->negate_type = 1;
		consumeWhitespace(vector, ctx);
		token = parserutils_vector_iterate(vector, ctx);
	} else if (lwc_string_caseless_isequal(token->idata,
			strings[ONLY], &match) == lwc_error_ok && match) {
		consumeWhitespace(vector, ctx);
		token = parserutils_vector_iterate(vector, ctx);
	}

	if (token == NULL || token->type != CSS_TOKEN_IDENT) {
		free(result);
		return CSS_INVALID;
	}

	result->type = mq_parse_type(strings, token->idata);

	consumeWhitespace(vector, ctx);

	token = parserutils_vector_iterate(vector, ctx);
	if (token != NULL) {
		if (token->type != CSS_TOKEN_IDENT ||
				lwc_string_caseless_isequal(token->idata,
					strings[AND], &match) != lwc_error_ok ||
				match == false) {
			free(result);
			return CSS_INVALID;
		}

		consumeWhitespace(vector, ctx);

		error = mq_parse_condition(strings, vector, ctx, false,
				&result->cond);
		if (error != CSS_OK) {
			free(result);
			return error;
		}
	}

finished:
	if (result->type == 0) {
		result->type = CSS_MEDIA_ALL;
	}

	*query = result;
	return CSS_OK;
}

/**
 * Create a `not all` media query.
 *
 * > 3.2: "A media query that does not match the grammar in the previous
 * > section must be replaced by not all during parsing."
 *
 * https://www.w3.org/TR/mediaqueries-4/#error-handling
 *
 * \param[out]  Returns the created mq on success.
 * \return CSS_OK on success,
 */
static css_error css__mq_parse__create_not_all(
		css_mq_query **not_all_out)
{
	css_mq_query *not_all;

	not_all = calloc(1, sizeof(*not_all));
	if (not_all == NULL) {
		return CSS_NOMEM;
	}

	not_all->negate_type = 1;
	not_all->type = CSS_MEDIA_ALL;

	*not_all_out = not_all;
	return CSS_OK;
}

css_error css__mq_parse_media_list(lwc_string **strings,
		const parserutils_vector *vector, int *ctx,
		css_mq_query **media)
{
	css_mq_query *result = NULL, *last = NULL;
	const css_token *token;
	css_error error;

	/* <media-query-list> = <media-query> [ COMMA <media-query> ]* */

	/* if {[(, push }]) to stack
	 * if func, push ) to stack
	 * on error, scan forward until stack is empty (or EOF), popping matching tokens off stack
	 * if stack is empty, the next input token must be comma or EOF
	 * if comma, consume, and start again from the next input token
	 */

	token = parserutils_vector_peek(vector, *ctx);
	while (token != NULL) {
		css_mq_query *query = NULL;

		error = mq_parse_media_query(strings, vector, ctx, &query);
		if (error == CSS_INVALID) {
			error = css__mq_parse__create_not_all(&query);
		}

		if (error != CSS_OK) {
			css__mq_query_destroy(result);
			return error;
		}

		if (result == NULL) {
			result = last = query;
		} else {
			assert(last != NULL);
			last->next = query;
			last = query;
		}

		consumeWhitespace(vector, ctx);

		token = parserutils_vector_iterate(vector, ctx);
		if (token != NULL && tokenIsChar(token, ',') == false) {
			/* Give up */
			break;
		}
	}

	*media = result;

	return CSS_OK;
}

typedef struct css_mq_parse_ctx {
	lwc_string **strings;
	css_mq_query *media;
} css_mq_parse_ctx;

static css_error css_parse_media_query_handle_event(
		css_parser_event type,
		const parserutils_vector *tokens,
		void *pw)
{
	int idx = 0;
	css_error err;
	css_mq_query *media;
	const css_token *tok;
	css_mq_parse_ctx *ctx = pw;
	lwc_string **strings = ctx->strings;

	UNUSED(type);

	/* Skip @media */
	tok = parserutils_vector_iterate(tokens, &idx);
	assert(tok->type == CSS_TOKEN_ATKEYWORD);
	UNUSED(tok);

	/* Skip whitespace */
	tok = parserutils_vector_iterate(tokens, &idx);
	assert(tok->type == CSS_TOKEN_S);
	UNUSED(tok);

	err = css__mq_parse_media_list(strings, tokens, &idx, &media);
	if (err != CSS_OK) {
		return CSS_OK;
	}

	ctx->media = media;
	return CSS_OK;
}

css_error css_parse_media_query(lwc_string **strings,
		const uint8_t *mq, size_t len,
		css_mq_query **media_out)
{
	css_error err;
	css_parser *parser;
	css_mq_parse_ctx ctx = {
		.strings = strings,
	};
	css_parser_optparams params_quirks = {
		.quirks = false,
	};
	css_parser_optparams params_handler = {
		.event_handler = {
			.handler = css_parse_media_query_handle_event,
			.pw = &ctx,
		},
	};

	if (mq == NULL || len == 0) {
		return CSS_BADPARM;
	}

	err = css__parser_create_for_media_query(NULL,
			CSS_CHARSET_DEFAULT, &parser);
	if (err != CSS_OK) {
		return err;
	}

	err = css__parser_setopt(parser, CSS_PARSER_QUIRKS,
			&params_quirks);
	if (err != CSS_OK) {
		css__parser_destroy(parser);
		return err;
	}

	err = css__parser_setopt(parser, CSS_PARSER_EVENT_HANDLER,
			&params_handler);
	if (err != CSS_OK) {
		css__parser_destroy(parser);
		return err;
	}

	err = css__parser_parse_chunk(parser,
			(const uint8_t *)"@media ",
			          strlen("@media "));
	if (err != CSS_OK && err != CSS_NEEDDATA) {
		css__parser_destroy(parser);
		return err;
	}

	err = css__parser_parse_chunk(parser, mq, len);
	if (err != CSS_OK && err != CSS_NEEDDATA) {
		css__parser_destroy(parser);
		return err;
	}

	err = css__parser_completed(parser);
	if (err != CSS_OK) {
		css__parser_destroy(parser);
		return err;
	}

	css__parser_destroy(parser);

	*media_out = ctx.media;
	return CSS_OK;
}

