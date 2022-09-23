/*
 * This file is part of CSSEng.
 * Licensed under the MIT License,
 *		  http://www.opensource.org/licenses/mit-license.php
 * Copyright 2016 John-Mark Bell <jmb@netsurf-browser.org>
 */

#ifndef css_parse_mq_h_
#define css_parse_mq_h_

#include "parser-utils/vector.h"
#include "parse/language.h"

typedef struct {
	enum {
		CSS_MQ_VALUE_TYPE_NUM,
		CSS_MQ_VALUE_TYPE_DIM,
		CSS_MQ_VALUE_TYPE_IDENT,
		CSS_MQ_VALUE_TYPE_RATIO
	} type;
	union {
		css_fixed num_or_ratio; /* Where ratio is the result of a/b */
		struct {
			css_fixed len;
			uint32_t unit;
		} dim;
		lwc_string *ident;
	} data;
} css_mq_value;

/*
 * "name : value" is encoded as "name = value"
 * "name" is encoded by setting the operator to "bool"
 * "value op name" is encoded verbatim (with op2 set to "unused")
 * "name op value" inverts the operator to encode (i.e < becomes >=) (and sets op2 to "unused")
 * "value op name op value" is encoded using op2 and value2
 */
typedef enum {
	CSS_MQ_FEATURE_OP_BOOL, /* op only */
	CSS_MQ_FEATURE_OP_UNUSED = CSS_MQ_FEATURE_OP_BOOL, /* op2 only */

	CSS_MQ_FEATURE_OP_LT,
	CSS_MQ_FEATURE_OP_LTE,
	CSS_MQ_FEATURE_OP_EQ, /* op only */
	CSS_MQ_FEATURE_OP_GTE,
	CSS_MQ_FEATURE_OP_GT
} css_mq_feature_op;

typedef struct {
	lwc_string *name;
	css_mq_feature_op op;
	css_mq_feature_op op2;
	css_mq_value value;
	css_mq_value value2;
} css_mq_feature;

typedef struct css_mq_cond_or_feature css_mq_cond_or_feature;

typedef struct {
	uint32_t negate : 1, /* set if "not" */
		 op     : 1; /* clear if "and", set if "or" */
	uint32_t nparts;
	css_mq_cond_or_feature **parts;
} css_mq_cond;

struct css_mq_cond_or_feature {
	enum {
		CSS_MQ_FEATURE,
		CSS_MQ_COND
	} type;
	union {
		css_mq_cond *cond;
		css_mq_feature *feat;
	} data;
};

typedef struct css_mq_query {
	struct css_mq_query *next;

	uint32_t negate_type : 1; /* set if "not type" */
	uint64_t type; /* or NULL */

	css_mq_cond *cond;
} css_mq_query;

css_error css_parse_media_query(lwc_string **strings,
		const uint8_t *mq, size_t len,
		css_mq_query **media_out);

css_error css__mq_parse_media_list(lwc_string **strings,
		const parserutils_vector *vector, int *ctx,
		css_mq_query **media);

void css__mq_query_destroy(css_mq_query *media);

#endif
