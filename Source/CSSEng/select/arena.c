/*
 * This file is part of CSSEng
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * Copyright 2015 Michael Drake <tlsa@netsurf-browser.org>
 */

#include <string.h>

#include "select/arena.h"
#include "select/arena_hash.h"
#include "select/computed.h"

#define TU_SIZE 3037
#define TS_SIZE 5101

struct css_computed_style *table_s[TS_SIZE];


static inline uint32_t css__arena_hash_style(struct css_computed_style *s)
{
	return css__arena_hash((const uint8_t *) &s->i, sizeof(s->i));
}


static inline bool arena__compare_computed_content_item(
		const struct css_computed_content_item *a,
		const struct css_computed_content_item *b)
{
	if (a == NULL && b == NULL) {
		return true;

	} else if (a == NULL || b == NULL) {
		return false;
	}

	if (a->type != b->type) {
		return false;
	}

	return memcmp(a, b, sizeof(struct css_computed_content_item)) == 0;
}


static inline bool arena__compare_css_computed_counter(
		const struct css_computed_counter *a,
		const struct css_computed_counter *b)
{
	bool match;

	if (a == NULL && b == NULL) {
		return true;

	} else if (a == NULL || b == NULL) {
		return false;
	}

	if (a->value == b->value &&
			lwc_string_isequal(a->name, b->name,
					&match) == lwc_error_ok &&
			match == true) {
		return true;
	}

	return false;
}

static inline bool arena__compare_string_list(
		lwc_string **a,
		lwc_string **b)
{
	if (a == NULL && b == NULL) {
		return true;

	} else if (a == NULL || b == NULL) {
		return false;
	}

	while (*a != NULL && *b != NULL) {
		bool match;

		if (lwc_string_isequal(*a, *b, &match) != lwc_error_ok ||
				match == false) {
			return false;
		}

		a++;
		b++;
	}

	if (*a != *b) {
		return false;
	}

	return true;
}


bool css__arena_style_is_equal(
		struct css_computed_style *a,
		struct css_computed_style *b)
{
	if (memcmp(&a->i, &b->i, sizeof(struct css_computed_style_i)) != 0) {
		return false;
	}

	if (!arena__compare_string_list(
			a->font_family,
			b->font_family)) {
		return false;
	}

	if (!arena__compare_css_computed_counter(
			a->counter_increment,
			b->counter_increment)) {
		return false;
	}

	if (!arena__compare_css_computed_counter(
			a->counter_reset,
			b->counter_reset)) {
		return false;
	}

	if (!arena__compare_computed_content_item(
			a->content,
			b->content)) {
		return false;
	}

	if (!arena__compare_string_list(
			a->cursor,
			b->cursor)) {
		return false;
	}

	if (!arena__compare_string_list(
			a->quotes,
			b->quotes)) {
		return false;
	}

	return true;
}


/* Internally exported function, documented in src/select/arena.h */
css_error css__arena_intern_style(struct css_computed_style **style)
{
	struct css_computed_style *s = *style;
	uint32_t hash, index;

	/* Don't try to intern an already-interned computed style */
	if (s->count != 0) {
		return CSS_BADPARM;
	}

	/* Need to intern the style block */
	hash = css__arena_hash_style(s);
	index = hash % TS_SIZE;
	s->bin = index;

	if (table_s[index] == NULL) {
		/* Can just insert */
		table_s[index] = s;
		s->count = 1;
	} else {
		/* Check for existing */
		struct css_computed_style *l = table_s[index];
		struct css_computed_style *existing = NULL;

		do {
			if (css__arena_style_is_equal(l, s)) {
				existing = l;
				break;
			}
			l = l->next;
		} while (l != NULL);

		if (existing != NULL) {
			css_computed_style_destroy(s);
			existing->count++;
			*style = existing;
		} else {
			/* Add to list */
			s->next = table_s[index];
			table_s[index] = s;
			s->count = 1;
		}
	}

	return CSS_OK;
}


/* Internally exported function, documented in src/select/arena.h */
enum css_error css__arena_remove_style(struct css_computed_style *style)
{
	uint32_t index = style->bin;

	if (table_s[index] == NULL) {
		return CSS_BADPARM;

	} else {
		/* Check for existing */
		struct css_computed_style *l = table_s[index];
		struct css_computed_style *existing = NULL;
		struct css_computed_style *prev = NULL;

		do {
			if (css__arena_style_is_equal(l, style)) {
				existing = l;
				break;
			}
			prev = l;
			l = l->next;
		} while (l != NULL);

		if (existing != NULL) {
			if (prev != NULL) {
				prev->next = existing->next;
			} else {
				table_s[index] = existing->next;
			}
		} else {
			return CSS_BADPARM;
		}
	}

	return CSS_OK;
}
