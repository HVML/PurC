/*
 * This file is part of CSSEng
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * Copyright 2018 Michael Drake <tlsa@netsurf-browser.org>
 */

#ifndef css_select_mq_h_
#define css_select_mq_h_

static inline css_fixed css_len2px(
		css_fixed length,
		css_unit unit,
		const css_media *media)
{
	css_fixed px_per_unit;

	switch (unit) {
	case CSS_UNIT_VI:
		/* TODO: Assumes writing mode. */
		unit = CSS_UNIT_VW;
		break;
	case CSS_UNIT_VB:
		/* TODO: Assumes writing mode. */
		unit = CSS_UNIT_VH;
		break;
	case CSS_UNIT_VMIN:
		unit = (media->height < media->width) ?
				CSS_UNIT_VH : CSS_UNIT_VW;
		break;
	case CSS_UNIT_VMAX:
		unit = (media->width > media->height) ?
				CSS_UNIT_VH : CSS_UNIT_VW;
		break;
	default:
		break;
	}

	switch (unit) {
	case CSS_UNIT_EM:
	case CSS_UNIT_EX:
	case CSS_UNIT_CAP:
	case CSS_UNIT_CH:
	case CSS_UNIT_IC:
	{
		px_per_unit = FDIV(FMUL(media->client_font_size, F_96), F_72);

		/* TODO: Handling these as fixed ratios of CSS_UNIT_EM. */
		switch (unit) {
		case CSS_UNIT_EX:
			px_per_unit = FMUL(px_per_unit, FLTTOFIX(0.6));
			break;
		case CSS_UNIT_CAP:
			px_per_unit = FMUL(px_per_unit, FLTTOFIX(0.9));
			break;
		case CSS_UNIT_CH:
			px_per_unit = FMUL(px_per_unit, FLTTOFIX(0.4));
			break;
		case CSS_UNIT_IC:
			px_per_unit = FMUL(px_per_unit, FLTTOFIX(1.1));
			break;
		default:
			break;
		}
	}
		break;
	case CSS_UNIT_PX:
		return length;
	case CSS_UNIT_IN:
		px_per_unit = F_96;
		break;
	case CSS_UNIT_CM:
		px_per_unit = FDIV(F_96, FLTTOFIX(2.54));
		break;
	case CSS_UNIT_MM:
		px_per_unit = FDIV(F_96, FLTTOFIX(25.4));
		break;
	case CSS_UNIT_Q:
		px_per_unit = FDIV(F_96, FLTTOFIX(101.6));
		break;
	case CSS_UNIT_PT:
		px_per_unit = FDIV(F_96, F_72);
		break;
	case CSS_UNIT_PC:
		px_per_unit = FDIV(F_96, INTTOFIX(6));
		break;
	case CSS_UNIT_REM:
		px_per_unit = FDIV(FMUL(media->client_font_size, F_96), F_72);
		break;
	case CSS_UNIT_RLH:
		px_per_unit = media->client_line_height;
		break;
	case CSS_UNIT_VH:
		px_per_unit = FDIV(media->height, F_100);
		break;
	case CSS_UNIT_VW:
		px_per_unit = FDIV(media->width, F_100);
		break;
	default:
		px_per_unit = 0;
		break;
	}

	/* Ensure we round px_per_unit to the nearest whole number of pixels:
	 * the use of FIXTOINT() below will truncate. */
	px_per_unit += F_0_5;

	/* Calculate total number of pixels */
	return FMUL(length, TRUNCATEFIX(px_per_unit));
}

static inline bool mq_match_feature_range_length_op1(
		css_mq_feature_op op,
		const css_mq_value *value,
		const css_fixed client_len,
		const css_media *media)
{
	css_fixed v;

	if (value->type != CSS_MQ_VALUE_TYPE_DIM) {
		return false;
	}

	if (value->data.dim.unit != CSS_UNIT_PX) {
		v = css_len2px(value->data.dim.len,
				value->data.dim.unit, media);
	} else {
		v = value->data.dim.len;
	}

	switch (op) {
	case CSS_MQ_FEATURE_OP_BOOL: return false;
	case CSS_MQ_FEATURE_OP_LT:   return v <  client_len;
	case CSS_MQ_FEATURE_OP_LTE:  return v <= client_len;
	case CSS_MQ_FEATURE_OP_EQ:   return v == client_len;
	case CSS_MQ_FEATURE_OP_GTE:  return v >= client_len;
	case CSS_MQ_FEATURE_OP_GT:   return v >  client_len;
	default:
		return false;
	}
}

static inline bool mq_match_feature_range_length_op2(
		css_mq_feature_op op,
		const css_mq_value *value,
		const css_fixed client_len,
		const css_media *media)
{
	css_fixed v;

	if (op == CSS_MQ_FEATURE_OP_UNUSED) {
		return true;
	}
	if (value->type != CSS_MQ_VALUE_TYPE_DIM) {
		return false;
	}

	if (value->data.dim.unit != CSS_UNIT_PX) {
		v = css_len2px(value->data.dim.len,
				value->data.dim.unit, media);
	} else {
		v = value->data.dim.len;
	}

	switch (op) {
	case CSS_MQ_FEATURE_OP_LT:  return client_len <  v;
	case CSS_MQ_FEATURE_OP_LTE: return client_len <= v;
	case CSS_MQ_FEATURE_OP_EQ:  return client_len == v;
	case CSS_MQ_FEATURE_OP_GTE: return client_len >= v;
	case CSS_MQ_FEATURE_OP_GT:  return client_len >  v;
	default:
		return false;
	}
}

/**
 * Match media query features.
 *
 * \param[in] feat   Condition to match.
 * \param[in] media  Current media spec, to check against feat.
 * \return true if condition matches, otherwise false.
 */
static inline bool mq_match_feature(
		const css_mq_feature *feat,
		const css_media *media)
{
	/* TODO: Use interned string for comparison. */
	if (strcmp(lwc_string_data(feat->name), "width") == 0) {
		if (!mq_match_feature_range_length_op1(feat->op, &feat->value,
					media->width, media)) {
			return false;
		}
		return mq_match_feature_range_length_op2(feat->op2,
					&feat->value2, media->width, media);

	} else if (strcmp(lwc_string_data(feat->name), "height") == 0) {
		if (!mq_match_feature_range_length_op1(feat->op, &feat->value,
				media->height, media)) {
			return false;
		}

		return mq_match_feature_range_length_op2(feat->op2,
				&feat->value2, media->height, media);
	}

	/* TODO: Look at other feature names. */

	return false;
}

/**
 * Match media query conditions.
 *
 * \param[in] cond   Condition to match.
 * \param[in] media  Current media spec, to check against cond.
 * \return true if condition matches, otherwise false.
 */
static inline bool mq_match_condition(
		const css_mq_cond *cond,
		const css_media *media)
{
	bool matched = !cond->op;

	for (uint32_t i = 0; i < cond->nparts; i++) {
		bool part_matched;
		if (cond->parts[i]->type == CSS_MQ_FEATURE) {
			part_matched = mq_match_feature(
					cond->parts[i]->data.feat, media);
		} else {
			assert(cond->parts[i]->type == CSS_MQ_COND);
			part_matched = mq_match_condition(
					cond->parts[i]->data.cond, media);
		}

		if (cond->op) {
			/* OR */
			matched |= part_matched;
			if (matched) {
				break; /* Short-circuit */
			}
		} else {
			/* AND */
			matched &= part_matched;
			if (!matched) {
				break; /* Short-circuit */
			}
		}
	}

	return matched != cond->negate;
}

/**
 * Test whether media query list matches current media.
 *
 * If anything in the list matches, the list matches.  If none match
 * it doesn't match.
 *
 * \param[in] m      Media query list.
 * \param[in] media  Current media spec, to check against m.
 * \return true if media query list matches media
 */
static inline bool mq__list_match(
		const css_mq_query *m,
		const css_media *media)
{
	for (; m != NULL; m = m->next) {
		/* Check type */
		if (!!(m->type & media->type) != m->negate_type) {
			if (m->cond == NULL ||
					mq_match_condition(m->cond, media)) {
				/* We have a match, no need to look further. */
				return true;
			}
		}
	}

	return false;
}

/**
 * Test whether the rule applies for current media.
 *
 * \param rule   Rule to test
 * \param media  Current media spec
 * \return true iff chain's rule applies for media
 */
static inline bool mq_rule_good_for_media(const css_rule *rule, const css_media *media)
{
	bool applies = true;
	const css_rule *ancestor = rule;

	while (ancestor != NULL) {
		const css_rule_media *m = (const css_rule_media *) ancestor;

		if (ancestor->type == CSS_RULE_MEDIA) {
			applies = mq__list_match(m->media, media);
			if (applies == false) {
				break;
			}
		}

		if (ancestor->ptype != CSS_RULE_PARENT_STYLESHEET) {
			ancestor = ancestor->parent;
		} else {
			ancestor = NULL;
		}
	}

	return applies;
}

#endif
