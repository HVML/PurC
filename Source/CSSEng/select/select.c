/*
 * This file is part of CSSEng
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2009 John-Mark Bell <jmb@netsurf-browser.org>
 */

#include <assert.h>
#include <string.h>

#include "csseng-wapcaplet.h"
#include "csseng-select.h"

#include "bytecode/bytecode.h"
#include "bytecode/opcodes.h"
#include "select/stylesheet.h"
#include "select/arena.h"
#include "select/computed.h"
#include "select/dispatch.h"
#include "select/hash.h"
#include "select/mq.h"
#include "select/propset.h"
#include "select/font_face.h"
#include "select/select.h"
#include "utils/parserutilserror.h"
#include "utils/utils.h"

/* Define this to enable verbose messages when matching selector chains */
#undef DEBUG_CHAIN_MATCHING

/* Define this to enable verbose messages when attempting to share styles */
#undef DEBUG_STYLE_SHARING

/**
 * Container for stylesheet selection info
 */
typedef struct css_select_sheet {
	const css_stylesheet *sheet;	/**< Stylesheet */
	css_origin origin;		/**< Stylesheet origin */
	css_mq_query *media;		/**< Applicable media */
} css_select_sheet;

/**
 * CSS selection context
 */
struct css_select_ctx {
	uint32_t n_sheets;		/**< Number of sheets */

	css_select_sheet *sheets;	/**< Array of sheets */

	void *pw;	/**< Client's private selection context */

	/* Useful interned strings */
	lwc_string *universal;
	lwc_string *first_child;
	lwc_string *link;
	lwc_string *visited;
	lwc_string *hover;
	lwc_string *active;
	lwc_string *focus;
	lwc_string *nth_child;
	lwc_string *nth_last_child;
	lwc_string *nth_of_type;
	lwc_string *nth_last_of_type;
	lwc_string *last_child;
	lwc_string *first_of_type;
	lwc_string *last_of_type;
	lwc_string *only_child;
	lwc_string *only_of_type;
	lwc_string *root;
	lwc_string *empty;
	lwc_string *target;
	lwc_string *lang;
	lwc_string *enabled;
	lwc_string *disabled;
	lwc_string *checked;
	lwc_string *first_line;
	lwc_string *first_letter;
	lwc_string *before;
	lwc_string *after;

	/* Interned default style */
	css_computed_style *default_style;
};

/**
 * Container for selected font faces
 */
typedef struct css_select_font_faces_list {
	const css_font_face **font_faces;
	size_t count;
} css_select_font_faces_list;

/**
 * Font face selection state
 */
typedef struct css_select_font_faces_state {
	lwc_string *font_family;
	const css_media *media;

	css_select_font_faces_list ua_font_faces;
	css_select_font_faces_list user_font_faces;
	css_select_font_faces_list author_font_faces;
} css_select_font_faces_state;

/**
 * CSS rule source
 */
typedef struct css_select_rule_source {
	enum {
		CSS_SELECT_RULE_SRC_ELEMENT,
		CSS_SELECT_RULE_SRC_CLASS,
		CSS_SELECT_RULE_SRC_ID,
		CSS_SELECT_RULE_SRC_UNIVERSAL
	} source;
	uint32_t class;
} css_select_rule_source;


static css_error set_hint(css_select_state *state, css_hint *hint);
static css_error set_initial(css_select_state *state,
		uint32_t prop, css_pseudo_element pseudo,
		void *parent);

static css_error intern_strings(css_select_ctx *ctx);
static void destroy_strings(css_select_ctx *ctx);

static css_error select_from_sheet(css_select_ctx *ctx,
		const css_stylesheet *sheet, css_origin origin,
		css_select_state *state);
static css_error match_selectors_in_sheet(css_select_ctx *ctx,
		const css_stylesheet *sheet, css_select_state *state,
		size_t *nr_matched);
static css_error match_selector_chain(css_select_ctx *ctx,
		const css_selector *selector, css_select_state *state, bool *match);
static css_error match_named_combinator(css_select_ctx *ctx,
		css_combinator type, const css_selector *selector,
		css_select_state *state, void *node, void **next_node);
static css_error match_universal_combinator(css_select_ctx *ctx,
		css_combinator type, const css_selector *selector,
		css_select_state *state, void *node, bool may_optimise,
		bool *rejected_by_cache, void **next_node);
static css_error match_details(css_select_ctx *ctx, void *node,
		const css_selector_detail *detail, css_select_state *state,
		bool *match, css_pseudo_element *pseudo_element);
static css_error match_detail(css_select_ctx *ctx, void *node,
		const css_selector_detail *detail, css_select_state *state,
		bool *match, css_pseudo_element *pseudo_element);
static css_error cascade_style(const css_style *style, css_select_state *state);

static css_error select_font_faces_from_sheet(
		const css_stylesheet *sheet,
		css_origin origin,
		css_select_font_faces_state *state);

#ifdef DEBUG_CHAIN_MATCHING
static void dump_chain(const css_selector *selector);
#endif


static css_error css__create_node_data(struct css_node_data **node_data)
{
	struct css_node_data *nd;

	nd = calloc(sizeof(struct css_node_data), 1);
	if (nd == NULL) {
		return CSS_NOMEM;
	}

	*node_data = nd;

	return CSS_OK;
}

static void css__destroy_node_data(struct css_node_data *node_data)
{
	int i;

	assert(node_data != NULL);

	if (node_data->bloom != NULL) {
		free(node_data->bloom);
	}

	for (i = 0; i < CSS_PSEUDO_ELEMENT_COUNT; i++) {
		if (node_data->partial.styles[i] != NULL) {
			css_computed_style_destroy(
					node_data->partial.styles[i]);
		}
	}

	free(node_data);
}


/* Exported function documented in public select.h header. */
css_error css_node_data_handler(css_select_handler *handler,
		css_node_data_action action, void *pw, void *node,
		void *clone_node, void *_node_data)
{
	struct css_node_data *node_data = _node_data;
	css_error error;

	UNUSED(clone_node);

	if (handler == NULL || node_data == NULL ||
	    handler->handler_version != CSS_SELECT_HANDLER_VERSION_1) {
		return CSS_BADPARM;
	}

	switch (action) {
	case CSS_NODE_DELETED:
		css__destroy_node_data(node_data);
		break;

	case CSS_NODE_MODIFIED:
	case CSS_NODE_ANCESTORS_MODIFIED:
		if (node == NULL) {
			return CSS_BADPARM;
		}

		css__destroy_node_data(node_data);

		/* Don't bother rebuilding node_data, it can be done
		 * when the node is selected for.  Just ensure the
		 * client drops its reference to the node_data. */
		error = handler->set_node_data(pw, node, NULL);
		if (error != CSS_OK) {
			return error;
		}
		break;

	case CSS_NODE_CLONED:
		/* TODO: is it worth cloning libcss data?  We only store
		 *       data on the nodes as an optimisation, which is
		 *       unlikely to be valid for most cloning cases.
		 */
		break;

	default:
		return CSS_BADPARM;
	}

	return CSS_OK;
}

/**
 * Create a selection context
 *
 * \param result  Pointer to location to receive created context
 * \return CSS_OK on success, appropriate error otherwise.
 */
css_error css_select_ctx_create(css_select_ctx **result)
{
	css_select_ctx *c;
	css_error error;

	if (result == NULL)
		return CSS_BADPARM;

	c = calloc(sizeof(css_select_ctx), 1);
	if (c == NULL)
		return CSS_NOMEM;

	error = intern_strings(c);
	if (error != CSS_OK) {
		free(c);
		return error;
	}

	*result = c;

	return CSS_OK;
}

/**
 * Destroy a selection context
 *
 * \param ctx  The context to destroy
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_select_ctx_destroy(css_select_ctx *ctx)
{
	if (ctx == NULL)
		return CSS_BADPARM;

	destroy_strings(ctx);

	if (ctx->default_style != NULL)
		css_computed_style_destroy(ctx->default_style);

	if (ctx->sheets != NULL) {
		for (uint32_t index = 0; index < ctx->n_sheets; index++) {
			css__mq_query_destroy(ctx->sheets[index].media);
		}
		free(ctx->sheets);
	}

	free(ctx);

	return CSS_OK;
}

/**
 * Append a stylesheet to a selection context
 *
 * \param ctx     The context to append to
 * \param sheet   The sheet to append
 * \param origin  Origin of the sheet
 * \param media   Media string for the stylesheet
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_select_ctx_append_sheet(css_select_ctx *ctx,
		const css_stylesheet *sheet, css_origin origin,
		const char *media)
{
	if (ctx == NULL || sheet == NULL)
		return CSS_BADPARM;

	return css_select_ctx_insert_sheet(ctx, sheet, ctx->n_sheets,
			origin, media);
}

/**
 * Insert a stylesheet into a selection context
 *
 * \param ctx    The context to insert into
 * \param sheet  Sheet to insert
 * \param index  Index in context to insert sheet
 * \param origin  Origin of the sheet
 * \param media   Media string for the stylesheet
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_select_ctx_insert_sheet(css_select_ctx *ctx,
		const css_stylesheet *sheet, uint32_t index,
		css_origin origin, const char *media)
{
	css_select_sheet *temp;
	css_mq_query *mq;
	css_error error;

	if (ctx == NULL || sheet == NULL)
		return CSS_BADPARM;

	/* Inline styles cannot be inserted into a selection context */
	if (sheet->inline_style)
		return CSS_INVALID;

	/* Index must be in the range [0, n_sheets]
	 * The latter being equivalent to append */
	if (index > ctx->n_sheets)
		return CSS_INVALID;

	temp = realloc(ctx->sheets,
			(ctx->n_sheets + 1) * sizeof(css_select_sheet));
	if (temp == NULL)
		return CSS_NOMEM;

	ctx->sheets = temp;

	if (index < ctx->n_sheets) {
		memmove(&ctx->sheets[index + 1], &ctx->sheets[index],
			(ctx->n_sheets - index) * sizeof(css_select_sheet));
	}

	error = css_parse_media_query(sheet->propstrings,
			(const uint8_t *)media,
			(media == NULL) ? 0 : strlen(media), &mq);
	if (error == CSS_NOMEM) {
		return error;
	} else if (error != CSS_OK) {
		/* Fall back to default media: "all". */
		mq = calloc(1, sizeof(*mq));
		if (mq == NULL) {
			return CSS_NOMEM;
		}
		mq->type = CSS_MEDIA_ALL;
	}

	ctx->sheets[index].sheet = sheet;
	ctx->sheets[index].origin = origin;
	ctx->sheets[index].media = mq;

	ctx->n_sheets++;

	return CSS_OK;
}

/**
 * Remove a sheet from a selection context
 *
 * \param ctx    The context to remove from
 * \param sheet  Sheet to remove
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_select_ctx_remove_sheet(css_select_ctx *ctx,
		const css_stylesheet *sheet)
{
	uint32_t index;

	if (ctx == NULL || sheet == NULL)
		return CSS_BADPARM;

	for (index = 0; index < ctx->n_sheets; index++) {
		if (ctx->sheets[index].sheet == sheet)
			break;
	}

	if (index == ctx->n_sheets)
		return CSS_INVALID;

	css__mq_query_destroy(ctx->sheets[index].media);

	ctx->n_sheets--;

	memmove(&ctx->sheets[index], &ctx->sheets[index + 1],
			(ctx->n_sheets - index) * sizeof(css_select_sheet));

	return CSS_OK;

}

/**
 * Count the number of top-level sheets in a selection context
 *
 * \param ctx    Context to consider
 * \param count  Pointer to location to receive count of sheets
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_select_ctx_count_sheets(css_select_ctx *ctx, uint32_t *count)
{
	if (ctx == NULL || count == NULL)
		return CSS_BADPARM;

	*count = ctx->n_sheets;

	return CSS_OK;
}

/**
 * Retrieve a sheet from a selection context
 *
 * \param ctx    Context to look in
 * \param index  Index in context to look
 * \param sheet  Pointer to location to receive sheet
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_select_ctx_get_sheet(css_select_ctx *ctx, uint32_t index,
		const css_stylesheet **sheet)
{
	if (ctx == NULL || sheet == NULL)
		return CSS_BADPARM;

	if (index > ctx->n_sheets)
		return CSS_INVALID;

	*sheet = ctx->sheets[index].sheet;

	return CSS_OK;
}


/**
 * Create a default style on the selection context
 *
 * \param ctx		Context to create default style in
 * \param handler	Dispatch table of handler functions
 * \param pw		Client-specific private data for handler functions
 * \return CSS_OK on success, appropriate error otherwise
 */
static css_error css__select_ctx_create_default_style(css_select_ctx *ctx,
		css_select_handler *handler, void *pw)
{
	css_computed_style *style;
	css_error error;

	/* Need to construct the default style */
	error = css__computed_style_create(&style);
	if (error != CSS_OK)
		return error;

	error = css__computed_style_initialise(style, handler, pw);
	if (error != CSS_OK) {
		css_computed_style_destroy(style);
		return error;
	}

	/* Neither create nor initialise intern the style, so intern it now */
	error = css__arena_intern_style(&style);
	if (error != CSS_OK)
		return error;

	/* Store it on the ctx */
	ctx->default_style = style;

	return CSS_OK;
}


/**
 * Get a default style, e.g. for an implied element's anonamous box
 *
 * \param ctx		Selection context (used to avoid recreating default)
 * \param handler	Dispatch table of handler functions
 * \param pw		Client-specific private data for handler functions
 * \param style		Pointer to location to receive default style
 * \return CSS_OK on success, appropriate error otherwise.
 */
css_error css_select_default_style(css_select_ctx *ctx,
		css_select_handler *handler, void *pw,
		css_computed_style **style)
{
	css_error error;

	if (ctx == NULL || style == NULL || handler == NULL ||
			handler->handler_version !=
					CSS_SELECT_HANDLER_VERSION_1)
		return CSS_BADPARM;

	/* Ensure the ctx has a default style */
	if (ctx->default_style == NULL) {
		error = css__select_ctx_create_default_style(ctx, handler, pw);
		if (error != CSS_OK) {
			return error;
		}
	}

	/* Pass a ref back to the client */
	*style = css__computed_style_ref(ctx->default_style);
	return CSS_OK;
}


/**
 * Get a bloom filter for the parent node
 *
 * \param parent	Parent node to get bloom filter for
 * \param handler	Dispatch table of handler functions
 * \param pw		Client-specific private data for handler functions
 * \param parent_bloom	Updated to parent bloom to use.
 *                    	Note: if there's no parent, the caller must free
 *                      the returned parent bloom, since it has no node to
 *                      own it.
 * \return CSS_OK on success, appropriate error otherwise.
 */
static css_error css__get_parent_bloom(void *parent,
		css_select_handler *handler, void *pw,
		css_bloom **parent_bloom)
{
	struct css_node_data *node_data = NULL;
	css_bloom *bloom = NULL;
	css_error error;

	/* Get parent node's bloom filter */
	if (parent != NULL) {
		/* Get parent bloom filter */
		struct css_node_data *node_data;

		/* Hideous casting to avoid warnings on all platforms
		 * we build for. */
		error = handler->get_node_data(pw, parent,
				(void **) (void *) &node_data);
		if (error != CSS_OK) {
			return error;
		}
		if (node_data != NULL) {
			bloom = node_data->bloom;
		}
	}

	if (bloom == NULL) {
		uint32_t i;
		/* Need to create parent bloom */

		if (parent != NULL) {
			/* TODO:
			 * Build & set the parent node's bloom properly.
			 * This will speed up the case where DOM change
			 * has caused bloom to get deleted.  For now we
			 * fall back to a fully satruated bloom filter,
			 * which is slower but perfectly valid.
			 */
			bloom = malloc(sizeof(css_bloom) * CSS_BLOOM_SIZE);
			if (bloom == NULL) {
				return CSS_NOMEM;
			}

			for (i = 0; i < CSS_BLOOM_SIZE; i++) {
				bloom[i] = ~0;
			}

			if (node_data == NULL) {
				error = css__create_node_data(&node_data);
				if (error != CSS_OK) {
					free(bloom);
					return error;
				}
				node_data->bloom = bloom;

				/* Set parent node bloom filter */
				error = handler->set_node_data(pw,
						parent, node_data);
				if (error != CSS_OK) {
					css__destroy_node_data(node_data);
					return error;
				}
			}
		} else {
			/* No ancestors; empty bloom filter */
			/* The parent bloom is owned by the parent node's
			 * node data.  However, for the root node, there is
			 * no parent node to own the bloom filter.
			 * As such, we just use a pointer to static storage
			 * so calling code doesn't need to worry about
			 * whether the returned parent bloom is owned
			 * by something or not.
			 * Note, parent bloom is only read from, and not
			 * written to. */
			static css_bloom empty_bloom[CSS_BLOOM_SIZE];
			bloom = empty_bloom;
		}
	}

	*parent_bloom = bloom;
	return CSS_OK;
}

static css_error css__create_node_bloom(
		css_bloom **node_bloom, css_select_state *state)
{
	css_error error;
	css_bloom *bloom;
	lwc_hash hash;

	*node_bloom = NULL;

	/* Create the node's bloom */
	bloom = calloc(sizeof(css_bloom), CSS_BLOOM_SIZE);
	if (bloom == NULL) {
		return CSS_NOMEM;
	}

	/* Add node name to bloom */
	if (lwc_string_caseless_hash_value(state->element.name,
			&hash) != lwc_error_ok) {
		error = CSS_NOMEM;
		goto cleanup;
	}
	css_bloom_add_hash(bloom, hash);

	/* Add id name to bloom */
	if (state->id != NULL) {
		if (lwc_string_caseless_hash_value(state->id,
				&hash) != lwc_error_ok) {
			error = CSS_NOMEM;
			goto cleanup;
		}
		css_bloom_add_hash(bloom, hash);
	}

	/* Add class names to bloom */
	if (state->classes != NULL) {
		for (uint32_t i = 0; i < state->n_classes; i++) {
			lwc_string *s = state->classes[i];
			if (lwc_string_caseless_hash_value(s,
					&hash) != lwc_error_ok) {
				error = CSS_NOMEM;
				goto cleanup;
			}
			css_bloom_add_hash(bloom, hash);
		}
	}

	/* Merge parent bloom into node bloom */
	css_bloom_merge(state->node_data->bloom, bloom);
	*node_bloom = bloom;

	return CSS_OK;

cleanup:
	free(bloom);

	return error;
}

/**
 * Set a node's data
 *
 * \param node     Node to set node data for
 * \param state    Selection state for node
 * \param handler  Dispatch table of handler functions
 * \param pw       Client-specific private data for handler functions
 * \return CSS_OK on success, appropriate error otherwise.
 */
static css_error css__set_node_data(void *node, css_select_state *state,
		css_select_handler *handler, void *pw)
{
	int i;
	css_error error;
	css_bloom *bloom;
	css_select_results *results;

	struct css_node_data *node_data = state->node_data;

	/* Set node bloom filter */
	error = css__create_node_bloom(&bloom, state);
	if (error != CSS_OK) {
		return error;
	}
	node_data->bloom = bloom;

	/* Set selection results */
	results = state->results;
	for (i = 0; i < CSS_PSEUDO_ELEMENT_COUNT; i++) {
		node_data->partial.styles[i] =
				css__computed_style_ref(results->styles[i]);
	}

	error = handler->set_node_data(pw, node, node_data);
	if (error != CSS_OK) {
		css__destroy_node_data(node_data);
		state->node_data = NULL;
		return error;
	}

	state->node_data = NULL;

	return CSS_OK;
}


/** The releationship of a share candidate node to the selection node. */
enum share_candidate_type {
	CANDIDATE_SIBLING,
	CANDIDATE_COUSIN,
};


/**
 * Get node_data for candidate node if we can reuse its style.
 *
 * \param[in]  state                 The selection state for current node.
 * \param[in]  share_candidate_node  The node to test id and classes of.
 * \param[in]  type                  The candidate's relation to selection node.
 * \param[out] sharable_node_data    Returns node_data or NULL.
 * \return CSS_OK on success, appropriate error otherwise.
 */
static css_error css_select_style__get_sharable_node_data_for_candidate(
		css_select_state *state,
		void *share_candidate_node,
		enum share_candidate_type type,
		struct css_node_data **sharable_node_data)
{
	css_error error;
	lwc_string *share_candidate_id;
	uint32_t share_candidate_n_classes;
	lwc_string **share_candidate_classes;
	struct css_node_data *node_data;

	UNUSED(type);

	*sharable_node_data = NULL;

	/* We get the candidate node data first, as if it has none, we can't
	 * share its data anyway.
	 * Hideous casting to avoid warnings on all platforms we build for. */
	error = state->handler->get_node_data(state->pw,
			share_candidate_node, (void **) (void *) &node_data);
	if (error != CSS_OK || node_data == NULL) {
#ifdef DEBUG_STYLE_SHARING
		printf("      \t%s\tno share: no candidate node data\n",
				lwc_string_data(state->element.name));
#endif
		return error;
	}

	/* If one node has hints and other doesn't then can't share */
	if ((node_data->flags & CSS_NODE_FLAGS_HAS_HINTS) !=
			(state->node_data->flags & CSS_NODE_FLAGS_HAS_HINTS)) {
#ifdef DEBUG_STYLE_SHARING
		printf("      \t%s\tno share: have hints mismatch\n",
				lwc_string_data(state->element.name));
#endif
		return CSS_OK;
	}

	/* If the node and candidate node had different pseudo classes, we
	 * can't share. */
	if ((node_data->flags & CSS_NODE_FLAGS__PSEUDO_CLASSES_MASK) !=
			(state->node_data->flags &
					CSS_NODE_FLAGS__PSEUDO_CLASSES_MASK)) {
#ifdef DEBUG_STYLE_SHARING
		printf("      \t%s\tno share: different pseudo classes\n",
				lwc_string_data(state->element.name));
#endif
		return CSS_OK;

	}

	/* If the node was affected by attribute or pseudo class rules,
	 * or had an inline style, it's not a candidate for sharing */
	if (node_data->flags & (
			CSS_NODE_FLAGS_TAINT_PSEUDO_CLASS |
			CSS_NODE_FLAGS_TAINT_ATTRIBUTE |
			CSS_NODE_FLAGS_TAINT_SIBLING |
			CSS_NODE_FLAGS_HAS_INLINE_STYLE)) {
#ifdef DEBUG_STYLE_SHARING
		printf("      \t%s\tno share: candidate flags: %s%s%s%s\n",
				lwc_string_data(state->element.name),
				(node_data->flags &
					CSS_NODE_FLAGS_TAINT_PSEUDO_CLASS) ?
						"PSEUDOCLASS" : "",
				(node_data->flags &
					CSS_NODE_FLAGS_TAINT_ATTRIBUTE) ?
						" ATTRIBUTE" : "",
				(node_data->flags &
					CSS_NODE_FLAGS_TAINT_SIBLING) ?
						" SIBLING" : "",
				(node_data->flags &
					CSS_NODE_FLAGS_HAS_INLINE_STYLE) ?
						" INLINE_STYLE" : "");
#endif
		return CSS_OK;
	}

	/* Check candidate ID doesn't prevent sharing */
	error = state->handler->node_id(state->pw,
			share_candidate_node,
			&share_candidate_id);
	if (error != CSS_OK) {
		return error;

	} else if (share_candidate_id != NULL) {
		lwc_string_unref(share_candidate_id);
#ifdef DEBUG_STYLE_SHARING
		printf("      \t%s\tno share: candidate id\n",
				lwc_string_data(state->element.name));
#endif
		return CSS_OK;
	}

	/* Check candidate classes don't prevent sharing */
	error = state->handler->node_classes(state->pw,
			share_candidate_node,
			&share_candidate_classes,
			&share_candidate_n_classes);
	if (error != CSS_OK) {
		return error;
	}

	if (state->n_classes != share_candidate_n_classes) {
#ifdef DEBUG_STYLE_SHARING
		printf("      \t%s\tno share: class count mismatch\n",
				lwc_string_data(state->element.name));
#endif
		goto cleanup;
	}

	/* TODO: no need to care about the order, but it's simpler to
	 *       have an ordered match, and authors are more likely to be
	 *       consistent than  not. */
	for (uint32_t i = 0; i < share_candidate_n_classes; i++) {
		bool match;
		if (lwc_string_caseless_isequal(
				state->classes[i],
				share_candidate_classes[i],
				&match) == lwc_error_ok &&
				match == false) {
#ifdef DEBUG_STYLE_SHARING
			printf("      \t%s\tno share: class mismatch\n",
					lwc_string_data(state->element.name));
#endif
			goto cleanup;
		}
	}

	if (node_data->flags & CSS_NODE_FLAGS_HAS_HINTS) {
		/* TODO: check hints match.  For now, just prevent sharing */
#ifdef DEBUG_STYLE_SHARING
		printf("      \t%s\tno share: hints\n",
				lwc_string_data(state->element.name));
#endif
		goto cleanup;
	}

	*sharable_node_data = node_data;

cleanup:
	if (share_candidate_classes != NULL) {
		for (uint32_t i = 0; i < share_candidate_n_classes; i++) {
			lwc_string_unref(share_candidate_classes[i]);
		}
		free(share_candidate_classes);
	}

	return CSS_OK;
}


/**
 * Get previous named cousin node.
 *
 * \param[in]  state       The selection state for current node.
 * \param[in]  node        The node to get the cousin of.
 * \param[out] cousin_out  Returns a cousin node or NULL.
 * \return CSS_OK on success or appropriate error otherwise.
 */
static css_error css_select_style__get_named_cousin(
		css_select_state *state, void *node,
		void **cousin_out)
{
	/* TODO:
	 *
	 * Consider cousin nodes; Go to parent's previous sibling's last child.
	 * The parent and the parent's sibling must be "similar".
	 */
	UNUSED(state);
	UNUSED(node);

	*cousin_out = NULL;

	return CSS_OK;
}


/**
 * Get node_data for any node that we can reuse the style for.
 *
 * This is an optimisation to needing to perform selection for a node,
 * by sharing the style for a previous node.
 *
 * \param[in]  node                Node we're selecting for.
 * \param[in]  state               The current selection state.
 * \param[out] sharable_node_data  Returns node_data or NULL.
 * \return CSS_OK on success or appropriate error otherwise.
 */
static css_error css_select_style__get_sharable_node_data(
		void *node, css_select_state *state,
		struct css_node_data **sharable_node_data)
{
	css_error error;
	enum share_candidate_type type = CANDIDATE_SIBLING;

	*sharable_node_data = NULL;

	/* TODO: move this test to caller? */
	if (state->id != NULL) {
		/* If the node has an ID can't share another node's style. */
		/* TODO: Consider whether this ID exists in the ID hash tables.
		 *       (If not, the ID cannot affect the node's style.)
		 *
		 *       Call css__selector_hash_find_by_id, for each sheet,
		 *       and if we get a non-NULL "matched" then return.
		 *
		 *       Check overhead is worth cost. */
#ifdef DEBUG_STYLE_SHARING
printf("      \t%s\tno share: node id (%s)\n", lwc_string_data(state->element.name), lwc_string_data(state->id));
#endif
		return CSS_OK;
	}
	if (state->node_data->flags & CSS_NODE_FLAGS_HAS_INLINE_STYLE) {
#ifdef DEBUG_STYLE_SHARING
printf("      \t%s\tno share: inline style\n");
#endif
		return CSS_OK;
	}

	while (true) {
		void *share_candidate_node;

		/* Get previous sibling with same element name */
		error = state->handler->named_generic_sibling_node(state->pw,
				node, &state->element, &share_candidate_node);
		if (error != CSS_OK) {
			return error;
		} else {
			if (share_candidate_node == NULL) {
				error = css_select_style__get_named_cousin(
						state, node,
						&share_candidate_node);
				if (error != CSS_OK) {
					return error;
				} else {
					if (share_candidate_node == NULL) {
						break;
					}
				}
				type = CANDIDATE_COUSIN;
			}
		}

		/* Check whether we can share the candidate node's
		 * style.  We already know the element names match,
		 * check that candidate node's ID and class won't
		 * prevent sharing. */
		error = css_select_style__get_sharable_node_data_for_candidate(
				state, share_candidate_node,
				type, sharable_node_data);
		if (error != CSS_OK) {
			return error;
		}

		if (*sharable_node_data != NULL) {
			/* Found style date we can share */
			break;
		}

		/* Can't share with this; look for another */
		node = share_candidate_node;
	}

	return CSS_OK;
}


/**
 * Finalise a selection state, releasing any resources it owns
 *
 * \param[in] state  The selection state to finalise.
 */
static void css_select__finalise_selection_state(
		css_select_state *state)
{
	if (state->results != NULL) {
		css_select_results_destroy(state->results);
	}

	if (state->node_data != NULL) {
		css__destroy_node_data(state->node_data);
	}

	if (state->classes != NULL) {
		for (uint32_t i = 0; i < state->n_classes; i++) {
			lwc_string_unref(state->classes[i]);
		}
		free(state->classes);
	}

	if (state->id != NULL) {
		lwc_string_unref(state->id);
	}

	if (state->element.ns != NULL) {
		lwc_string_unref(state->element.ns);
	}

	if (state->element.name != NULL){
		lwc_string_unref(state->element.name);
	}
}


/**
 * Initialise a selection state.
 *
 * \param[in]  state    The selection state to initialise
 * \param[in]  node     The node we are selecting for.
 * \param[in]  parent   The node's parent node, or NULL.
 * \param[in]  media    The media specification we're selecting for.
 * \param[in]  handler  The client selection callback table.
 * \param[in]  pw       The client private data, passsed out to callbacks.
 * \return CSS_OK or appropriate error otherwise.
 */
static css_error css_select__initialise_selection_state(
		css_select_state *state,
		void *node,
		void *parent,
		const css_media *media,
		css_select_handler *handler,
		void *pw)
{
	css_error error;
	bool match;

	/* Set up the selection state */
	memset(state, 0, sizeof(*state));
	state->node = node;
	state->media = media;
	state->handler = handler;
	state->pw = pw;
	state->next_reject = state->reject_cache +
			(N_ELEMENTS(state->reject_cache) - 1);

	/* Allocate the result set */
	state->results = calloc(1, sizeof(css_select_results));
	if (state->results == NULL) {
		return CSS_NOMEM;
	}

	error = css__create_node_data(&state->node_data);
	if (error != CSS_OK) {
		goto failed;
	}

	error = css__get_parent_bloom(parent, handler, pw,
			&state->node_data->bloom);
	if (error != CSS_OK) {
		goto failed;
	}

	/* Get node's name */
	error = handler->node_name(pw, node, &state->element);
	if (error != CSS_OK){
		goto failed;
	}

	/* Get node's ID, if any */
	error = handler->node_id(pw, node, &state->id);
	if (error != CSS_OK){
		goto failed;
	}

	/* Get node's classes, if any */
	error = handler->node_classes(pw, node,
			&state->classes, &state->n_classes);
	if (error != CSS_OK){
		goto failed;
	}

	/* Node pseudo classes */
	error = handler->node_is_link(pw, node, &match);
	if (error != CSS_OK){
		goto failed;
	} else if (match) {
		state->node_data->flags |= CSS_NODE_FLAGS_PSEUDO_CLASS_LINK;
	}

	error = handler->node_is_visited(pw, node, &match);
	if (error != CSS_OK){
		goto failed;
	} else if (match) {
		state->node_data->flags |= CSS_NODE_FLAGS_PSEUDO_CLASS_VISITED;
	}

	error = handler->node_is_hover(pw, node, &match);
	if (error != CSS_OK){
		goto failed;
	} else if (match) {
		state->node_data->flags |= CSS_NODE_FLAGS_PSEUDO_CLASS_HOVER;
	}

	error = handler->node_is_active(pw, node, &match);
	if (error != CSS_OK){
		goto failed;
	} else if (match) {
		state->node_data->flags |= CSS_NODE_FLAGS_PSEUDO_CLASS_ACTIVE;
	}

	error = handler->node_is_focus(pw, node, &match);
	if (error != CSS_OK){
		goto failed;
	} else if (match) {
		state->node_data->flags |= CSS_NODE_FLAGS_PSEUDO_CLASS_FOCUS;
	}

	return CSS_OK;

failed:
    /* FIXME: xue node : bloom belong to its parent */
    if (state->node_data->bloom) {
        state->node_data->bloom = NULL;
    }
	css_select__finalise_selection_state(state);
	return error;
}


/**
 * Select a style for the given node
 *
 * \param ctx             Selection context to use
 * \param node            Node to select style for
 * \param media           Currently active media specification
 * \param inline_style    Corresponding inline style for node, or NULL
 * \param handler         Dispatch table of handler functions
 * \param pw              Client-specific private data for handler functions
 * \param result          Pointer to location to receive result set
 * \return CSS_OK on success, appropriate error otherwise.
 *
 * In computing the style, no reference is made to the parent node's
 * style. Therefore, the resultant computed style is not ready for
 * immediate use, as some properties may be marked as inherited.
 * Use css_computed_style_compose() to obtain a fully computed style.
 *
 * This two-step approach to style computation is designed to allow
 * the client to store the partially computed style and efficiently
 * update the fully computed style for a node when layout changes.
 */
css_error css_select_style(css_select_ctx *ctx, void *node,
		const css_media *media, const css_stylesheet *inline_style,
		css_select_handler *handler, void *pw,
		css_select_results **result)
{
	uint32_t i, j, nhints;
	css_error error;
	css_select_state state;
	css_hint *hints = NULL;
	void *parent = NULL;
	struct css_node_data *share;

	if (ctx == NULL || node == NULL || result == NULL || handler == NULL ||
	    handler->handler_version != CSS_SELECT_HANDLER_VERSION_1)
		return CSS_BADPARM;

	error = handler->parent_node(pw, node, &parent);
	if (error != CSS_OK)
		return error;

	error = css_select__initialise_selection_state(
			&state, node, parent, media, handler, pw);
	if (error != CSS_OK)
		return error;

	/* Fetch presentational hints */
	error = handler->node_presentational_hint(pw, node, &nhints, &hints);
	if (error != CSS_OK)
		goto cleanup;
	if (nhints > 0) {
		state.node_data->flags |= CSS_NODE_FLAGS_HAS_HINTS;
	}

	if (inline_style != NULL) {
		state.node_data->flags |= CSS_NODE_FLAGS_HAS_INLINE_STYLE;
	}

	/* Check if we can share another node's style */
	error = css_select_style__get_sharable_node_data(node, &state, &share);
	if (error != CSS_OK) {
		goto cleanup;
	} else if (share != NULL) {
		css_computed_style **styles = share->partial.styles;
		for (i = 0; i < CSS_PSEUDO_ELEMENT_COUNT; i++) {
			state.results->styles[i] =
					css__computed_style_ref(styles[i]);
		}
#ifdef DEBUG_STYLE_SHARING
		printf("style:\t%s\tSHARED!\n",
				lwc_string_data(state.element.name));
#endif
		goto complete;
	}
#ifdef DEBUG_STYLE_SHARING
	printf("style:\t%s\tSELECTED\n", lwc_string_data(state.element.name));
#endif

	/* Not sharing; need to select.
	 * Base element style is guaranteed to exist
	 */
	error = css__computed_style_create(
			&state.results->styles[CSS_PSEUDO_ELEMENT_NONE]);
	if (error != CSS_OK) {
		goto cleanup;
	}

	/* Apply any hints */
	if (nhints > 0) {
		/* Ensure that the appropriate computed style exists */
		struct css_computed_style *computed_style =
				state.results->styles[CSS_PSEUDO_ELEMENT_NONE];
		state.computed = computed_style;

		for (i = 0; i < nhints; i++) {
			error = set_hint(&state, &hints[i]);
			if (error != CSS_OK)
				goto cleanup;
		}
	}

	/* Iterate through the top-level stylesheets, selecting styles
	 * from those which apply to our current media requirements and
	 * are not disabled */
	for (i = 0; i < ctx->n_sheets; i++) {
		const css_select_sheet s = ctx->sheets[i];

		if (mq__list_match(s.media, media) &&
				s.sheet->disabled == false) {
			error = select_from_sheet(ctx, s.sheet,
					s.origin, &state);
			if (error != CSS_OK)
				goto cleanup;
		}
	}

	/* Consider any inline style for the node */
	if (inline_style != NULL) {
		css_rule_selector *sel =
				(css_rule_selector *) inline_style->rule_list;

		/* Sanity check style */
		if (inline_style->rule_count != 1 ||
			inline_style->rule_list->type != CSS_RULE_SELECTOR ||
				inline_style->rule_list->items != 0) {
			error = CSS_INVALID;
			goto cleanup;
		}

		/* No bytecode if input was empty or wholly invalid */
		if (sel->style != NULL) {
			/* Inline style applies to base element only */
			state.current_pseudo = CSS_PSEUDO_ELEMENT_NONE;
			state.computed = state.results->styles[
					CSS_PSEUDO_ELEMENT_NONE];

			error = cascade_style(sel->style, &state);
			if (error != CSS_OK)
				goto cleanup;
		}
	}

	/* Fix up any remaining unset properties. */

	/* Base element */
	state.current_pseudo = CSS_PSEUDO_ELEMENT_NONE;
	state.computed = state.results->styles[CSS_PSEUDO_ELEMENT_NONE];
	for (i = 0; i < CSS_N_PROPERTIES; i++) {
		const prop_state *prop =
				&state.props[i][CSS_PSEUDO_ELEMENT_NONE];

		/* If the property is still unset or it's set to inherit
		 * and we're the root element, then set it to its initial
		 * value. */
		if (prop->set == false ||
				(parent == NULL &&
				prop->inherit == true)) {
			error = set_initial(&state, i,
					CSS_PSEUDO_ELEMENT_NONE, parent);
			if (error != CSS_OK)
				goto cleanup;
		}
	}

	/* Pseudo elements, if any */
	for (j = CSS_PSEUDO_ELEMENT_NONE + 1; j < CSS_PSEUDO_ELEMENT_COUNT; j++) {
		state.current_pseudo = j;
		state.computed = state.results->styles[j];

		/* Skip non-existent pseudo elements */
		if (state.computed == NULL)
			continue;

		for (i = 0; i < CSS_N_PROPERTIES; i++) {
			const prop_state *prop = &state.props[i][j];

			/* If the property is still unset then set it
			 * to its initial value. */
			if (prop->set == false) {
				error = set_initial(&state, i, j, parent);
				if (error != CSS_OK)
					goto cleanup;
			}
		}
	}

	/* If this is the root element, then we must ensure that all
	 * length values are absolute, display and float are correctly
	 * computed, and the default border-{top,right,bottom,left}-color
	 * is set to the computed value of color. */
	if (parent == NULL) {
		/* Only compute absolute values for the base element */
		error = css__compute_absolute_values(NULL,
				state.results->styles[CSS_PSEUDO_ELEMENT_NONE],
				handler->compute_font_size, pw);
		if (error != CSS_OK)
			goto cleanup;
	}

	/* Intern the partial computed styles */
	for (j = CSS_PSEUDO_ELEMENT_NONE; j < CSS_PSEUDO_ELEMENT_COUNT; j++) {
		/* Skip non-existent pseudo elements */
		if (state.results->styles[j] == NULL)
			continue;

		error = css__arena_intern_style(&state.results->styles[j]);
		if (error != CSS_OK) {
			goto cleanup;
		}
	}

complete:
	error = css__set_node_data(node, &state, handler, pw);
	if (error != CSS_OK) {
		goto cleanup;
	}

	/* Steal the results from the selection state, so they don't get
	 * freed when the selection state is finalised */
	*result = state.results;
	state.results = NULL;

	error = CSS_OK;

cleanup:
	css_select__finalise_selection_state(&state);

	return error;
}

/**
 * Destroy a selection result set
 *
 * \param results  Result set to destroy
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_select_results_destroy(css_select_results *results)
{
	uint32_t i;

	if (results == NULL)
		return CSS_BADPARM;

	for (i = 0; i < CSS_PSEUDO_ELEMENT_COUNT; i++) {
		if (results->styles[i] != NULL)
			css_computed_style_destroy(results->styles[i]);
	}

	free(results);

	return CSS_OK;
}

/**
 * Search a selection context for defined font faces
 *
 * \param ctx          Selection context
 * \param media        Currently active media spec
 * \param font_family  Font family to search for
 * \param result       Pointer to location to receive result
 * \return CSS_OK on success, appropriate error otherwise.
 */
css_error css_select_font_faces(css_select_ctx *ctx,
		const css_media *media, lwc_string *font_family,
		css_select_font_faces_results **result)
{
	uint32_t i;
	css_error error;
	css_select_font_faces_state state;
	uint32_t n_font_faces;

	if (ctx == NULL || font_family == NULL || result == NULL)
		return CSS_BADPARM;

	memset(&state, 0, sizeof(css_select_font_faces_state));
	state.font_family = font_family;
	state.media = media;

	/* Iterate through the top-level stylesheets, selecting font-faces
	 * from those which apply to our current media requirements and
	 * are not disabled */
	for (i = 0; i < ctx->n_sheets; i++) {
		const css_select_sheet s = ctx->sheets[i];

		if (mq__list_match(s.media, media) &&
				s.sheet->disabled == false) {
			error = select_font_faces_from_sheet(s.sheet,
					s.origin, &state);
			if (error != CSS_OK)
				goto cleanup;
		}
	}

	n_font_faces = state.ua_font_faces.count +
			state.user_font_faces.count +
			state.author_font_faces.count;


	if (n_font_faces > 0) {
		/* We found some matching faces.  Make a results structure with
		 * the font faces in priority order. */
		css_select_font_faces_results *results;

		results = malloc(sizeof(css_select_font_faces_results));
		if (results == NULL) {
			error = CSS_NOMEM;
			goto cleanup;
		}

		results->font_faces = malloc(
				n_font_faces * sizeof(css_font_face *));
		if (results->font_faces == NULL) {
			free(results);
			error = CSS_NOMEM;
			goto cleanup;
		}

		results->n_font_faces = n_font_faces;

		i = 0;
		if (state.ua_font_faces.count != 0) {
			memcpy(results->font_faces,
					state.ua_font_faces.font_faces,
					sizeof(css_font_face *) *
						state.ua_font_faces.count);

			i += state.ua_font_faces.count;
		}

		if (state.user_font_faces.count != 0) {
			memcpy(results->font_faces + i,
					state.user_font_faces.font_faces,
					sizeof(css_font_face *) *
						state.user_font_faces.count);
			i += state.user_font_faces.count;
		}

		if (state.author_font_faces.count != 0) {
			memcpy(results->font_faces + i,
					state.author_font_faces.font_faces,
					sizeof(css_font_face *) *
						state.author_font_faces.count);
		}

		*result = results;
	}

	error = CSS_OK;

cleanup:
	if (state.ua_font_faces.count != 0)
		free(state.ua_font_faces.font_faces);

	if (state.user_font_faces.count != 0)
		free(state.user_font_faces.font_faces);

	if (state.author_font_faces.count != 0)
		free(state.author_font_faces.font_faces);

	return error;
}

/**
 * Destroy a font-face result set
 *
 * \param results  Result set to destroy
 * \return CSS_OK on success, appropriate error otherwise
 */
css_error css_select_font_faces_results_destroy(
		css_select_font_faces_results *results)
{
	if (results == NULL)
		return CSS_BADPARM;

	if (results->font_faces != NULL) {
		/* Don't destroy the individual css_font_faces, they're owned
		   by their respective sheets */
		free(results->font_faces);
	}

	free(results);

	return CSS_OK;
}

/******************************************************************************
 * Selection engine internals below here                                      *
 ******************************************************************************/

css_error intern_strings(css_select_ctx *ctx)
{
	lwc_error error;

	/* Universal selector */
	error = lwc_intern_string("*", SLEN("*"), &ctx->universal);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	/* Pseudo classes */
	error = lwc_intern_string(
			"first-child", SLEN("first-child"),
			&ctx->first_child);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"link", SLEN("link"),
			&ctx->link);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"visited", SLEN("visited"),
			&ctx->visited);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"hover", SLEN("hover"),
			&ctx->hover);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"active", SLEN("active"),
			&ctx->active);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"focus", SLEN("focus"),
			&ctx->focus);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"nth-child", SLEN("nth-child"),
			&ctx->nth_child);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"nth-last-child", SLEN("nth-last-child"),
			&ctx->nth_last_child);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"nth-of-type", SLEN("nth-of-type"),
			&ctx->nth_of_type);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"nth-last-of-type", SLEN("nth-last-of-type"),
			&ctx->nth_last_of_type);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"last-child", SLEN("last-child"),
			&ctx->last_child);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"first-of-type", SLEN("first-of-type"),
			&ctx->first_of_type);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"last-of-type", SLEN("last-of-type"),
			&ctx->last_of_type);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"only-child", SLEN("only-child"),
			&ctx->only_child);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"only-of-type", SLEN("only-of-type"),
			&ctx->only_of_type);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"root", SLEN("root"),
			&ctx->root);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"empty", SLEN("empty"),
			&ctx->empty);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"target", SLEN("target"),
			&ctx->target);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"lang", SLEN("lang"),
			&ctx->lang);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"enabled", SLEN("enabled"),
			&ctx->enabled);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"disabled", SLEN("disabled"),
			&ctx->disabled);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"checked", SLEN("checked"),
			&ctx->checked);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	/* Pseudo elements */
	error = lwc_intern_string(
			"first-line", SLEN("first-line"),
			&ctx->first_line);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"first_letter", SLEN("first-letter"),
			&ctx->first_letter);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"before", SLEN("before"),
			&ctx->before);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	error = lwc_intern_string(
			"after", SLEN("after"),
			&ctx->after);
	if (error != lwc_error_ok)
		return css_error_from_lwc_error(error);

	return CSS_OK;
}

void destroy_strings(css_select_ctx *ctx)
{
	if (ctx->universal != NULL)
		lwc_string_unref(ctx->universal);
	if (ctx->first_child != NULL)
		lwc_string_unref(ctx->first_child);
	if (ctx->link != NULL)
		lwc_string_unref(ctx->link);
	if (ctx->visited != NULL)
		lwc_string_unref(ctx->visited);
	if (ctx->hover != NULL)
		lwc_string_unref(ctx->hover);
	if (ctx->active != NULL)
		lwc_string_unref(ctx->active);
	if (ctx->focus != NULL)
		lwc_string_unref(ctx->focus);
	if (ctx->nth_child != NULL)
		lwc_string_unref(ctx->nth_child);
	if (ctx->nth_last_child != NULL)
		lwc_string_unref(ctx->nth_last_child);
	if (ctx->nth_of_type != NULL)
		lwc_string_unref(ctx->nth_of_type);
	if (ctx->nth_last_of_type != NULL)
		lwc_string_unref(ctx->nth_last_of_type);
	if (ctx->last_child != NULL)
		lwc_string_unref(ctx->last_child);
	if (ctx->first_of_type != NULL)
		lwc_string_unref(ctx->first_of_type);
	if (ctx->last_of_type != NULL)
		lwc_string_unref(ctx->last_of_type);
	if (ctx->only_child != NULL)
		lwc_string_unref(ctx->only_child);
	if (ctx->only_of_type != NULL)
		lwc_string_unref(ctx->only_of_type);
	if (ctx->root != NULL)
		lwc_string_unref(ctx->root);
	if (ctx->empty != NULL)
		lwc_string_unref(ctx->empty);
	if (ctx->target != NULL)
		lwc_string_unref(ctx->target);
	if (ctx->lang != NULL)
		lwc_string_unref(ctx->lang);
	if (ctx->enabled != NULL)
		lwc_string_unref(ctx->enabled);
	if (ctx->disabled != NULL)
		lwc_string_unref(ctx->disabled);
	if (ctx->checked != NULL)
		lwc_string_unref(ctx->checked);
	if (ctx->first_line != NULL)
		lwc_string_unref(ctx->first_line);
	if (ctx->first_letter != NULL)
		lwc_string_unref(ctx->first_letter);
	if (ctx->before != NULL)
		lwc_string_unref(ctx->before);
	if (ctx->after != NULL)
		lwc_string_unref(ctx->after);
}

css_error set_hint(css_select_state *state, css_hint *hint)
{
	uint32_t prop = hint->prop;
	prop_state *existing = &state->props[prop][CSS_PSEUDO_ELEMENT_NONE];
	css_error error;

	/* Hint defined -- set it in the result */
	error = prop_dispatch[prop].set_from_hint(hint, state->computed);
	if (error != CSS_OK)
		return error;

	/* Keep selection state in sync with reality */
	existing->set = 1;
	existing->specificity = 0;
	existing->origin = CSS_ORIGIN_AUTHOR;
	existing->important = 0;
	existing->inherit = (hint->status == 0);

	return CSS_OK;
}

css_error set_initial(css_select_state *state,
		uint32_t prop, css_pseudo_element pseudo,
		void *parent)
{
	css_error error;

	/* Do nothing if this property is inherited (the default state
	 * of a clean computed style is for everything to be set to inherit)
	 *
	 * If the node is tree root and we're dealing with the base element,
	 * everything should be defaulted.
	 */
	if (prop_dispatch[prop].inherited == false ||
			(pseudo == CSS_PSEUDO_ELEMENT_NONE && parent == NULL)) {
		error = prop_dispatch[prop].initial(state);
		if (error != CSS_OK)
			return error;
	}

	return CSS_OK;
}

#define IMPORT_STACK_SIZE 256

css_error select_from_sheet(css_select_ctx *ctx, const css_stylesheet *sheet,
		css_origin origin, css_select_state *state)
{
	const css_stylesheet *s = sheet;
	const css_rule *rule = s->rule_list;
	uint32_t sp = 0;
	const css_rule *import_stack[IMPORT_STACK_SIZE];
	size_t nr_matched = 0;

	do {
		/* Find first non-charset rule, if we're at the list head */
		if (rule == s->rule_list) {
			while (rule != NULL && rule->type == CSS_RULE_CHARSET)
				rule = rule->next;
		}

		if (rule != NULL && rule->type == CSS_RULE_IMPORT) {
			/* Current rule is an import */
			const css_rule_import *import =
					(const css_rule_import *) rule;

			if (import->sheet != NULL &&
					mq__list_match(import->media,
							state->media)) {
				/* It's applicable, so process it */
				if (sp >= IMPORT_STACK_SIZE)
					return CSS_NOMEM;

				import_stack[sp++] = rule;

				s = import->sheet;
				rule = s->rule_list;
			} else {
				/* Not applicable; skip over it */
				rule = rule->next;
			}
		} else {
			/* Gone past import rules in this sheet */
			css_error error;

			/* Process this sheet */
			state->sheet = s;
			state->current_origin = origin;

			error = match_selectors_in_sheet(ctx, s, state, &nr_matched);
			if (error != CSS_OK)
				return error;

			/* Find next sheet to process */
			if (sp > 0) {
				sp--;
				rule = import_stack[sp]->next;
				s = import_stack[sp]->parent;
			} else {
				s = NULL;
			}
		}
	} while (s != NULL);

	return CSS_OK;
}

static css_error _select_font_face_from_rule(
		const css_rule_font_face *rule, css_origin origin,
		css_select_font_faces_state *state)
{
	if (mq_rule_good_for_media((const css_rule *) rule, state->media)) {
		bool correct_family = false;

		if (lwc_string_isequal(
				rule->font_face->font_family,
				state->font_family,
				&correct_family) == lwc_error_ok &&
				correct_family) {
			css_select_font_faces_list *faces = NULL;
			const css_font_face **new_faces;
			uint32_t index;
			size_t new_size;

			switch (origin) {
				case CSS_ORIGIN_UA:
					faces = &state->ua_font_faces;
					break;
				case CSS_ORIGIN_USER:
					faces = &state->user_font_faces;
					break;
				case CSS_ORIGIN_AUTHOR:
					faces = &state->author_font_faces;
					break;
			}

			index = faces->count++;
			new_size = faces->count * sizeof(css_font_face *);

			new_faces = realloc(faces->font_faces, new_size);
			if (new_faces == NULL) {
				return CSS_NOMEM;
			}
			faces->font_faces = new_faces;

			faces->font_faces[index] = rule->font_face;
		}
	}

	return CSS_OK;
}

static css_error select_font_faces_from_sheet(
		const css_stylesheet *sheet,
		css_origin origin,
		css_select_font_faces_state *state)
{
	const css_stylesheet *s = sheet;
	const css_rule *rule = s->rule_list;
	uint32_t sp = 0;
	const css_rule *import_stack[IMPORT_STACK_SIZE];

	do {
		/* Find first non-charset rule, if we're at the list head */
		if (rule == s->rule_list) {
			while (rule != NULL && rule->type == CSS_RULE_CHARSET)
				rule = rule->next;
		}

		if (rule != NULL && rule->type == CSS_RULE_IMPORT) {
			/* Current rule is an import */
			const css_rule_import *import =
					(const css_rule_import *) rule;

			if (import->sheet != NULL &&
					mq__list_match(import->media,
							state->media)) {
				/* It's applicable, so process it */
				if (sp >= IMPORT_STACK_SIZE)
					return CSS_NOMEM;

				import_stack[sp++] = rule;

				s = import->sheet;
				rule = s->rule_list;
			} else {
				/* Not applicable; skip over it */
				rule = rule->next;
			}
		} else if (rule != NULL && rule->type == CSS_RULE_FONT_FACE) {
			css_error error;

			error = _select_font_face_from_rule(
					(const css_rule_font_face *) rule,
					origin,
					state);

			if (error != CSS_OK)
				return error;

			rule = rule->next;
		} else if (rule == NULL) {
			/* Find next sheet to process */
			if (sp > 0) {
				sp--;
				rule = import_stack[sp]->next;
				s = import_stack[sp]->parent;
			} else {
				s = NULL;
			}
		} else {
			rule = rule->next;
		}
	} while (s != NULL);

	return CSS_OK;
}

#undef IMPORT_STACK_SIZE

static inline bool _selectors_pending(const css_selector **node,
		const css_selector **id, const css_selector ***classes,
		uint32_t n_classes, const css_selector **univ)
{
	bool pending = false;
	uint32_t i;

	pending |= *node != NULL;
	pending |= *id != NULL;
	pending |= *univ != NULL;

	if (classes != NULL && n_classes > 0) {
		for (i = 0; i < n_classes; i++)
			pending |= *(classes[i]) != NULL;
	}

	return pending;
}

static inline bool _selector_less_specific(const css_selector *ref,
		const css_selector *cand)
{
	bool result = true;

	if (cand == NULL)
		return false;

	if (ref == NULL)
		return true;

	/* Sort by specificity */
	if (cand->specificity < ref->specificity) {
		result = true;
	} else if (ref->specificity < cand->specificity) {
		result = false;
	} else {
		/* Then by rule index -- earliest wins */
		if (cand->rule->index < ref->rule->index)
			result = true;
		else
			result = false;
	}

	return result;
}

static const css_selector *_selector_next(const css_selector **node,
		const css_selector **id, const css_selector ***classes,
		uint32_t n_classes, const css_selector **univ,
		css_select_rule_source *src)
{
	const css_selector *ret = NULL;

	if (_selector_less_specific(ret, *node)) {
		ret = *node;
		src->source = CSS_SELECT_RULE_SRC_ELEMENT;
	}

	if (_selector_less_specific(ret, *id)) {
		ret = *id;
		src->source = CSS_SELECT_RULE_SRC_ID;
	}

	if (_selector_less_specific(ret, *univ)) {
		ret = *univ;
		src->source = CSS_SELECT_RULE_SRC_UNIVERSAL;
	}

	if (classes != NULL && n_classes > 0) {
		uint32_t i;

		for (i = 0; i < n_classes; i++) {
			if (_selector_less_specific(ret, *(classes[i]))) {
				ret = *(classes[i]);
				src->source = CSS_SELECT_RULE_SRC_CLASS;
				src->class = i;
			}
		}
	}

	return ret;
}

css_error match_selectors_in_sheet(css_select_ctx *ctx,
		const css_stylesheet *sheet, css_select_state *state,
		size_t *nr_matched)
{
	static const css_selector *empty_selector = NULL;
	const uint32_t n_classes = state->n_classes;
	uint32_t i = 0;
	const css_selector **node_selectors = &empty_selector;
	css_selector_hash_iterator node_iterator;
	const css_selector **id_selectors = &empty_selector;
	css_selector_hash_iterator id_iterator;
	const css_selector ***class_selectors = NULL;
	css_selector_hash_iterator class_iterator;
	const css_selector **univ_selectors = &empty_selector;
	css_selector_hash_iterator univ_iterator;
	css_select_rule_source src = { CSS_SELECT_RULE_SRC_ELEMENT, 0 };
	struct css_hash_selection_requirments req;
	css_error error;
	bool match = false;
	if (nr_matched) {
		*nr_matched = 0;
	}

	/* Set up general selector chain requirments */
	req.media = state->media;
	req.node_bloom = state->node_data->bloom;
	req.uni = ctx->universal;

	/* Find hash chain that applies to current node */
	req.qname = state->element;
	error = css__selector_hash_find(sheet->selectors,
			&req, &node_iterator,
			&node_selectors);
	if (error != CSS_OK)
		goto cleanup;

	if (state->classes != NULL && n_classes > 0) {
		/* Find hash chains for node classes */
		class_selectors = malloc(n_classes * sizeof(css_selector **));
		if (class_selectors == NULL) {
			error = CSS_NOMEM;
			goto cleanup;
		}

		for (i = 0; i < n_classes; i++) {
			req.class = state->classes[i];
			error = css__selector_hash_find_by_class(
					sheet->selectors, &req,
					&class_iterator, &class_selectors[i]);
			if (error != CSS_OK)
				goto cleanup;
		}
	}

	if (state->id != NULL) {
		/* Find hash chain for node ID */
		req.id = state->id;
		error = css__selector_hash_find_by_id(sheet->selectors,
				&req, &id_iterator, &id_selectors);
		if (error != CSS_OK)
			goto cleanup;
	}

	/* Find hash chain for universal selector */
	error = css__selector_hash_find_universal(sheet->selectors, &req,
			&univ_iterator, &univ_selectors);
	if (error != CSS_OK)
		goto cleanup;

	/* Process matching selectors, if any */
	while (_selectors_pending(node_selectors, id_selectors,
			class_selectors, n_classes, univ_selectors)) {
		const css_selector *selector;

		/* Selectors must be matched in ascending order of specificity
		 * and rule index. (c.f. css__outranks_existing())
		 *
		 * Pick the least specific/earliest occurring selector.
		 */
		selector = _selector_next(node_selectors, id_selectors,
				class_selectors, n_classes, univ_selectors,
				&src);

		/* We know there are selectors pending, so should have a
		 * selector here */
		assert(selector != NULL);

		/* Match and handle the selector chain */
		error = match_selector_chain(ctx, selector, state, &match);
		if (error != CSS_OK)
			goto cleanup;
		if (nr_matched && match) {
			*nr_matched = *nr_matched + 1;
		}

		/* Advance to next selector in whichever chain we extracted
		 * the processed selector from. */
		switch (src.source) {
		case CSS_SELECT_RULE_SRC_ELEMENT:
			error = node_iterator(&req, node_selectors,
					&node_selectors);
			break;

		case CSS_SELECT_RULE_SRC_ID:
			error = id_iterator(&req, id_selectors,
					&id_selectors);
			break;

		case CSS_SELECT_RULE_SRC_UNIVERSAL:
			error = univ_iterator(&req, univ_selectors,
					&univ_selectors);
			break;

		case CSS_SELECT_RULE_SRC_CLASS:
			req.class = state->classes[src.class];
			error = class_iterator(&req, class_selectors[src.class],
					&class_selectors[src.class]);
			break;
		}

		if (error != CSS_OK)
			goto cleanup;
	}

	error = CSS_OK;
cleanup:
	if (class_selectors != NULL)
		free(class_selectors);

	return error;
}

static void update_reject_cache(css_select_state *state,
		css_combinator comb, const css_selector *s)
{
	const css_selector_detail *detail = &s->data;
	const css_selector_detail *next_detail = NULL;

	if (detail->next)
		next_detail = detail + 1;

	if (state->next_reject < state->reject_cache ||
			comb != CSS_COMBINATOR_ANCESTOR ||
			next_detail == NULL ||
			next_detail->next != 0 ||
			(next_detail->type != CSS_SELECTOR_CLASS &&
			 next_detail->type != CSS_SELECTOR_ID))
		return;

	/* Insert */
	state->next_reject->type = next_detail->type;
	state->next_reject->value = next_detail->qname.name;
	state->next_reject--;
}

css_error match_selector_chain(css_select_ctx *ctx,
		const css_selector *selector, css_select_state *state, bool *match)
{
	const css_selector *s = selector;
	void *node = state->node;
	const css_selector_detail *detail = &s->data;
	bool may_optimise = true;
	bool rejected_by_cache;
	*match = false;
	css_pseudo_element pseudo;
	css_error error;

#ifdef DEBUG_CHAIN_MATCHING
	fprintf(stderr, "matching: ");
	dump_chain(selector);
	fprintf(stderr, "\n");
#endif

	/* Match the details of the first selector in the chain.
	 *
	 * Note that pseudo elements will only appear as details of
	 * the first selector in the chain, as the parser will reject
	 * any selector chains containing pseudo elements anywhere
	 * else.
	 */
	error = match_details(ctx, node, detail, state, match, &pseudo);

	if (error != CSS_OK)
		return error;

	/* Details don't match, so reject selector chain */
	if (*match == false)
		return CSS_OK;

	/* Iterate up the selector chain, matching combinators */
	do {
		void *next_node = NULL;

		/* Consider any combinator on this selector */
		if (s->data.comb != CSS_COMBINATOR_NONE &&
				s->combinator->data.qname.name !=
					ctx->universal) {
			/* Named combinator */
			may_optimise &=
				(s->data.comb == CSS_COMBINATOR_ANCESTOR ||
				 s->data.comb == CSS_COMBINATOR_PARENT);

			error = match_named_combinator(ctx, s->data.comb,
					s->combinator, state, node, &next_node);
			if (error != CSS_OK)
				return error;

			/* No match for combinator, so reject selector chain */
			if (next_node == NULL)
				return CSS_OK;
		} else if (s->data.comb != CSS_COMBINATOR_NONE) {
			/* Universal combinator */
			may_optimise &=
				(s->data.comb == CSS_COMBINATOR_ANCESTOR ||
				 s->data.comb == CSS_COMBINATOR_PARENT);

			error = match_universal_combinator(ctx, s->data.comb,
					s->combinator, state, node,
					may_optimise, &rejected_by_cache,
					&next_node);
			if (error != CSS_OK)
				return error;

			/* No match for combinator, so reject selector chain */
			if (next_node == NULL) {
				if (may_optimise && s == selector &&
						rejected_by_cache == false) {
					update_reject_cache(state, s->data.comb,
							s->combinator);
				}

				return CSS_OK;
			}
		}

		/* Details matched, so progress to combining selector */
		s = s->combinator;
		node = next_node;
	} while (s != NULL);

	/* If we got here, then the entire selector chain matched, so cascade */
	state->current_specificity = selector->specificity;

	/* Ensure that the appropriate computed style exists */
	if (state->results->styles[pseudo] == NULL) {
		error = css__computed_style_create(
				&state->results->styles[pseudo]);
		if (error != CSS_OK)
			return error;
	}

	state->current_pseudo = pseudo;
	state->computed = state->results->styles[pseudo];

	return cascade_style(((css_rule_selector *) selector->rule)->style,
			state);
}

css_error match_named_combinator(css_select_ctx *ctx, css_combinator type,
		const css_selector *selector, css_select_state *state,
		void *node, void **next_node)
{
	const css_selector_detail *detail = &selector->data;
	void *n = node;
	css_error error;

	do {
		bool match = false;

		/* Find candidate node */
		switch (type) {
		case CSS_COMBINATOR_ANCESTOR:
			error = state->handler->named_ancestor_node(state->pw,
					n, &selector->data.qname, &n);
			if (error != CSS_OK)
				return error;
			break;
		case CSS_COMBINATOR_PARENT:
			error = state->handler->named_parent_node(state->pw,
					n, &selector->data.qname, &n);
			if (error != CSS_OK)
				return error;
			break;
		case CSS_COMBINATOR_SIBLING:
			error = state->handler->named_sibling_node(state->pw,
					n, &selector->data.qname, &n);
			if (error != CSS_OK)
				return error;
			if (node == state->node) {
				state->node_data->flags |=
						CSS_NODE_FLAGS_TAINT_SIBLING;
			}
			break;
		case CSS_COMBINATOR_GENERIC_SIBLING:
			error = state->handler->named_generic_sibling_node(
					state->pw, n, &selector->data.qname,
					&n);
			if (error != CSS_OK)
				return error;
			if (node == state->node) {
				state->node_data->flags |=
						CSS_NODE_FLAGS_TAINT_SIBLING;
			}
		case CSS_COMBINATOR_NONE:
			break;
		}

		if (n != NULL) {
			/* Match its details */
			error = match_details(ctx, n, detail, state,
					&match, NULL);
			if (error != CSS_OK)
				return error;

			/* If we found a match, use it */
			if (match == true)
				break;

			/* For parent and sibling selectors, only adjacent
			 * nodes are valid. Thus, if we failed to match,
			 * give up. */
			if (type == CSS_COMBINATOR_PARENT ||
					type == CSS_COMBINATOR_SIBLING)
				n = NULL;
		}
	} while (n != NULL);

	*next_node = n;

	return CSS_OK;
}

static inline void add_node_flags(const void *node,
		const css_select_state *state, css_node_flags flags)
{
	/* If the node in question is the node we're selecting for then its
	 * style has been tainted by particular rules that affect whether the
	 * node's style can be shared.  We don't care whether the rule matched
	 * or not, just that such a rule has been considered. */
	if (node == state->node) {
		state->node_data->flags |= flags;
	}
}

css_error match_universal_combinator(css_select_ctx *ctx, css_combinator type,
		const css_selector *selector, css_select_state *state,
		void *node, bool may_optimise, bool *rejected_by_cache,
		void **next_node)
{
	const css_selector_detail *detail = &selector->data;
	const css_selector_detail *next_detail = NULL;
	void *n = node;
	css_error error;

	if (detail->next)
		next_detail = detail + 1;

	*rejected_by_cache = false;

	/* Consult reject cache first */
	if (may_optimise && (type == CSS_COMBINATOR_ANCESTOR ||
			     type == CSS_COMBINATOR_PARENT) &&
			next_detail != NULL &&
			(next_detail->type == CSS_SELECTOR_CLASS ||
			 next_detail->type == CSS_SELECTOR_ID)) {
		reject_item *reject = state->next_reject + 1;
		reject_item *last = state->reject_cache +
				N_ELEMENTS(state->reject_cache) - 1;
		bool match = false;

		while (reject <= last) {
			/* Perform pessimistic matching (may hurt quirks) */
			if (reject->type == next_detail->type &&
					lwc_string_isequal(reject->value,
						next_detail->qname.name,
						&match) == lwc_error_ok &&
				 	match) {
				/* Found it: can't match */
				*next_node = NULL;
				*rejected_by_cache = true;
				return CSS_OK;
			}

			reject++;
		}
	}

	do {
		bool match = false;

		/* Find candidate node */
		switch (type) {
		case CSS_COMBINATOR_ANCESTOR:
		case CSS_COMBINATOR_PARENT:
			error = state->handler->parent_node(state->pw, n, &n);
			if (error != CSS_OK)
				return error;
			break;
		case CSS_COMBINATOR_SIBLING:
		case CSS_COMBINATOR_GENERIC_SIBLING:
			error = state->handler->sibling_node(state->pw, n, &n);
			if (error != CSS_OK)
				return error;
			add_node_flags(node, state,
					CSS_NODE_FLAGS_TAINT_SIBLING);
			break;
		case CSS_COMBINATOR_NONE:
			break;
		}

		if (n != NULL) {
			/* Match its details */
			error = match_details(ctx, n, detail, state,
					&match, NULL);
			if (error != CSS_OK)
				return error;

			/* If we found a match, use it */
			if (match == true)
				break;

			/* For parent and sibling selectors, only adjacent
			 * nodes are valid. Thus, if we failed to match,
			 * give up. */
			if (type == CSS_COMBINATOR_PARENT ||
					type == CSS_COMBINATOR_SIBLING)
				n = NULL;
		}
	} while (n != NULL);

	*next_node = n;

	return CSS_OK;
}

css_error match_details(css_select_ctx *ctx, void *node,
		const css_selector_detail *detail, css_select_state *state,
		bool *match, css_pseudo_element *pseudo_element)
{
	css_error error;
	css_pseudo_element pseudo = CSS_PSEUDO_ELEMENT_NONE;

	/* Skip the element selector detail, which is always first.
	 * (Named elements are handled by match_named_combinator, so the
	 * element selector detail always matches here.) */
	if (detail->next)
		detail++;
	else
		detail = NULL;

	/* We match by default (if there are no details other than the element
	 * selector, then we must match) */
	*match = true;

	/** \todo Some details are easier to test than others (e.g. dashmatch
	 * actually requires looking at data rather than simply comparing
	 * pointers). Should we consider sorting the detail list such that the
	 * simpler details come first (and thus the expensive match routines
	 * can be avoided unless absolutely necessary)? */

	while (detail != NULL) {
		error = match_detail(ctx, node, detail, state, match, &pseudo);
		if (error != CSS_OK)
			return error;

		/* Detail doesn't match, so reject selector chain */
		if (*match == false)
			return CSS_OK;

		if (detail->next)
			detail++;
		else
			detail = NULL;
	}

	/* Return the applicable pseudo element, if required */
	if (pseudo_element != NULL)
		*pseudo_element = pseudo;

	return CSS_OK;
}

static inline bool match_nth(int32_t a, int32_t b, int32_t count)
{
	if (a == 0) {
		return count == b;
	} else {
		const int32_t delta = count - b;

		/* (count - b) / a is positive or (count - b) is 0 */
		if (((delta > 0) == (a > 0)) || delta == 0) {
			/* (count - b) / a is integer */
			return (delta % a == 0);
		}

		return false;
	}
}

css_error match_detail(css_select_ctx *ctx, void *node,
		const css_selector_detail *detail, css_select_state *state,
		bool *match, css_pseudo_element *pseudo_element)
{
	bool is_root = false;
	css_error error = CSS_OK;
	css_node_flags flags = CSS_NODE_FLAGS_TAINT_PSEUDO_CLASS;

	switch (detail->type) {
	case CSS_SELECTOR_ELEMENT:
		if (detail->negate != 0) {
			/* Only need to test this inside not(), since
			 * it will have been considered as a named node
			 * otherwise. */
			error = state->handler->node_has_name(state->pw, node,
					&detail->qname, match);
		}
		break;
	case CSS_SELECTOR_CLASS:
		error = state->handler->node_has_class(state->pw, node,
				detail->qname.name, match);
		break;
	case CSS_SELECTOR_ID:
		error = state->handler->node_has_id(state->pw, node,
				detail->qname.name, match);
		break;
	case CSS_SELECTOR_PSEUDO_CLASS:
		error = state->handler->node_is_root(state->pw, node, &is_root);
		if (error != CSS_OK)
			return error;

		if (is_root == false &&
				detail->qname.name == ctx->first_child) {
			int32_t num_before = 0;

			error = state->handler->node_count_siblings(state->pw,
					node, false, false, &num_before);
			if (error == CSS_OK)
				*match = (num_before == 0);
		} else if (is_root == false &&
				detail->qname.name == ctx->nth_child) {
			int32_t num_before = 0;

			error = state->handler->node_count_siblings(state->pw,
					node, false, false, &num_before);
			if (error == CSS_OK) {
				int32_t a = detail->value.nth.a;
				int32_t b = detail->value.nth.b;

				*match = match_nth(a, b, num_before + 1);
			}
		} else if (is_root == false &&
				detail->qname.name == ctx->nth_last_child) {
			int32_t num_after = 0;

			error = state->handler->node_count_siblings(state->pw,
					node, false, true, &num_after);
			if (error == CSS_OK) {
				int32_t a = detail->value.nth.a;
				int32_t b = detail->value.nth.b;

				*match = match_nth(a, b, num_after + 1);
			}
		} else if (is_root == false &&
				detail->qname.name == ctx->nth_of_type) {
			int32_t num_before = 0;

			error = state->handler->node_count_siblings(state->pw,
					node, true, false, &num_before);
			if (error == CSS_OK) {
				int32_t a = detail->value.nth.a;
				int32_t b = detail->value.nth.b;

				*match = match_nth(a, b, num_before + 1);
			}
		} else if (is_root == false &&
				detail->qname.name == ctx->nth_last_of_type) {
			int32_t num_after = 0;

			error = state->handler->node_count_siblings(state->pw,
					node, true, true, &num_after);
			if (error == CSS_OK) {
				int32_t a = detail->value.nth.a;
				int32_t b = detail->value.nth.b;

				*match = match_nth(a, b, num_after + 1);
			}
		} else if (is_root == false &&
				detail->qname.name == ctx->last_child) {
			int32_t num_after = 0;

			error = state->handler->node_count_siblings(state->pw,
					node, false, true, &num_after);
			if (error == CSS_OK)
				*match = (num_after == 0);
		} else if (is_root == false &&
				detail->qname.name == ctx->first_of_type) {
			int32_t num_before = 0;

			error = state->handler->node_count_siblings(state->pw,
					node, true, false, &num_before);
			if (error == CSS_OK)
				*match = (num_before == 0);
		} else if (is_root == false &&
				detail->qname.name == ctx->last_of_type) {
			int32_t num_after = 0;

			error = state->handler->node_count_siblings(state->pw,
					node, true, true, &num_after);
			if (error == CSS_OK)
				*match = (num_after == 0);
		} else if (is_root == false &&
				detail->qname.name == ctx->only_child) {
			int32_t num_before = 0, num_after = 0;

			error = state->handler->node_count_siblings(state->pw,
					node, false, false, &num_before);
			if (error == CSS_OK) {
				error = state->handler->node_count_siblings(
						state->pw, node, false, true,
						&num_after);
				if (error == CSS_OK)
					*match = (num_before == 0) &&
							(num_after == 0);
			}
		} else if (is_root == false &&
				detail->qname.name == ctx->only_of_type) {
			int32_t num_before = 0, num_after = 0;

			error = state->handler->node_count_siblings(state->pw,
					node, true, false, &num_before);
			if (error == CSS_OK) {
				error = state->handler->node_count_siblings(
						state->pw, node, true, true,
						&num_after);
				if (error == CSS_OK)
					*match = (num_before == 0) &&
							(num_after == 0);
			}
		} else if (detail->qname.name == ctx->root) {
			*match = is_root;
		} else if (detail->qname.name == ctx->empty) {
			error = state->handler->node_is_empty(state->pw,
					node, match);
		} else if (detail->qname.name == ctx->link) {
			error = state->handler->node_is_link(state->pw,
					node, match);
			flags = CSS_NODE_FLAGS_NONE;
		} else if (detail->qname.name == ctx->visited) {
			error = state->handler->node_is_visited(state->pw,
					node, match);
			flags = CSS_NODE_FLAGS_NONE;
		} else if (detail->qname.name == ctx->hover) {
			error = state->handler->node_is_hover(state->pw,
					node, match);
			flags = CSS_NODE_FLAGS_NONE;
		} else if (detail->qname.name == ctx->active) {
			error = state->handler->node_is_active(state->pw,
					node, match);
			flags = CSS_NODE_FLAGS_NONE;
		} else if (detail->qname.name == ctx->focus) {
			error = state->handler->node_is_focus(state->pw,
					node, match);
			flags = CSS_NODE_FLAGS_NONE;
		} else if (detail->qname.name == ctx->target) {
			error = state->handler->node_is_target(state->pw,
					node, match);
		} else if (detail->qname.name == ctx->lang) {
			error = state->handler->node_is_lang(state->pw,
					node, detail->value.string, match);
		} else if (detail->qname.name == ctx->enabled) {
			error = state->handler->node_is_enabled(state->pw,
					node, match);
		} else if (detail->qname.name == ctx->disabled) {
			error = state->handler->node_is_disabled(state->pw,
					node, match);
		} else if (detail->qname.name == ctx->checked) {
			error = state->handler->node_is_checked(state->pw,
					node, match);
		} else {
			*match = false;
		}
		add_node_flags(node, state, flags);
		break;
	case CSS_SELECTOR_PSEUDO_ELEMENT:
		*match = true;

		if (detail->qname.name == ctx->first_line) {
			*pseudo_element = CSS_PSEUDO_ELEMENT_FIRST_LINE;
		} else if (detail->qname.name == ctx->first_letter) {
			*pseudo_element = CSS_PSEUDO_ELEMENT_FIRST_LETTER;
		} else if (detail->qname.name == ctx->before) {
			*pseudo_element = CSS_PSEUDO_ELEMENT_BEFORE;
		} else if (detail->qname.name == ctx->after) {
			*pseudo_element = CSS_PSEUDO_ELEMENT_AFTER;
		} else
			*match = false;
		break;
	case CSS_SELECTOR_ATTRIBUTE:
		error = state->handler->node_has_attribute(state->pw, node,
				&detail->qname, match);
		add_node_flags(node, state, CSS_NODE_FLAGS_TAINT_ATTRIBUTE);
		break;
	case CSS_SELECTOR_ATTRIBUTE_EQUAL:
		error = state->handler->node_has_attribute_equal(state->pw,
				node, &detail->qname, detail->value.string,
				match);
		add_node_flags(node, state, CSS_NODE_FLAGS_TAINT_ATTRIBUTE);
		break;
	case CSS_SELECTOR_ATTRIBUTE_DASHMATCH:
		error = state->handler->node_has_attribute_dashmatch(state->pw,
				node, &detail->qname, detail->value.string,
				match);
		add_node_flags(node, state, CSS_NODE_FLAGS_TAINT_ATTRIBUTE);
		break;
	case CSS_SELECTOR_ATTRIBUTE_INCLUDES:
		error = state->handler->node_has_attribute_includes(state->pw,
				node, &detail->qname, detail->value.string,
				match);
		add_node_flags(node, state, CSS_NODE_FLAGS_TAINT_ATTRIBUTE);
		break;
	case CSS_SELECTOR_ATTRIBUTE_PREFIX:
		error = state->handler->node_has_attribute_prefix(state->pw,
				node, &detail->qname, detail->value.string,
				match);
		add_node_flags(node, state, CSS_NODE_FLAGS_TAINT_ATTRIBUTE);
		break;
	case CSS_SELECTOR_ATTRIBUTE_SUFFIX:
		error = state->handler->node_has_attribute_suffix(state->pw,
				node, &detail->qname, detail->value.string,
				match);
		add_node_flags(node, state, CSS_NODE_FLAGS_TAINT_ATTRIBUTE);
		break;
	case CSS_SELECTOR_ATTRIBUTE_SUBSTRING:
		error = state->handler->node_has_attribute_substring(state->pw,
				node, &detail->qname, detail->value.string,
				match);
		add_node_flags(node, state, CSS_NODE_FLAGS_TAINT_ATTRIBUTE);
		break;
	}

	/* Invert match, if the detail requests it */
	if (error == CSS_OK && detail->negate != 0)
		*match = !*match;

	return error;
}

css_error cascade_style(const css_style *style, css_select_state *state)
{
	css_style s = *style;

	while (s.used > 0) {
		opcode_t op;
		css_error error;
		css_code_t opv = *s.bytecode;

		advance_bytecode(&s, sizeof(opv));

		op = getOpcode(opv);

		error = prop_dispatch[op].cascade(opv, &s, state);
		if (error != CSS_OK)
			return error;
	}

	return CSS_OK;
}

bool css__outranks_existing(uint16_t op, bool important, css_select_state *state,
		bool inherit)
{
	prop_state *existing = &state->props[op][state->current_pseudo];
	bool outranks = false;

	/* Sorting on origin & importance gives the following:
	 *
	 *           | UA, - | UA, i | USER, - | USER, i | AUTHOR, - | AUTHOR, i
	 *           |----------------------------------------------------------
	 * UA    , - |   S       S       Y          Y         Y           Y
	 * UA    , i |   S       S       Y          Y         Y           Y
	 * USER  , - |   -       -       S          Y         Y           Y
	 * USER  , i |   -       -       -          S         -           -
	 * AUTHOR, - |   -       -       -          Y         S           Y
	 * AUTHOR, i |   -       -       -          Y         -           S
	 *
	 * Where the columns represent the origin/importance of the property
	 * being considered and the rows represent the origin/importance of
	 * the existing property.
	 *
	 * - means that the existing property must be preserved
	 * Y means that the new property must be applied
	 * S means that the specificities of the rules must be considered.
	 *
	 * If specificities are considered, the highest specificity wins.
	 * If specificities are equal, then the rule defined last wins.
	 *
	 * We have no need to explicitly consider the ordering of rules if
	 * the specificities are the same because:
	 *
	 * a) We process stylesheets in order
	 * b) The selector hash chains within a sheet are ordered such that
	 *    more specific rules come after less specific ones and, when
	 *    specificities are identical, rules defined later occur after
	 *    those defined earlier.
	 *
	 * Therefore, where we consider specificity, below, the property
	 * currently being considered will always be applied if its specificity
	 * is greater than or equal to that of the existing property.
	 */

	if (existing->set == 0) {
		/* Property hasn't been set before, new one wins */
		outranks = true;
	} else {
		assert(CSS_ORIGIN_UA < CSS_ORIGIN_USER);
		assert(CSS_ORIGIN_USER < CSS_ORIGIN_AUTHOR);

		if (existing->origin < state->current_origin) {
			/* New origin has more weight than existing one.
			 * Thus, new property wins, except when the existing
			 * one is USER, i. */
			if (existing->important == 0 ||
					existing->origin != CSS_ORIGIN_USER) {
				outranks = true;
			}
		} else if (existing->origin == state->current_origin) {
			/* Origins are identical, consider importance, except
			 * for UA stylesheets, when specificity is always
			 * considered (as importance is meaningless) */
			if (existing->origin == CSS_ORIGIN_UA) {
				if (state->current_specificity >=
						existing->specificity) {
					outranks = true;
				}
			} else if (existing->important == 0 && important) {
				/* New is more important than old. */
				outranks = true;
			} else if (existing->important && important == false) {
				/* Old is more important than new */
			} else {
				/* Same importance, consider specificity */
				if (state->current_specificity >=
						existing->specificity) {
					outranks = true;
				}
			}
		} else {
			/* Existing origin has more weight than new one.
			 * Thus, existing property wins, except when the new
			 * one is USER, i. */
			if (state->current_origin == CSS_ORIGIN_USER &&
					important) {
				outranks = true;
			}
		}
	}

	if (outranks) {
		/* The new property is about to replace the old one.
		 * Update our state to reflect this. */
		existing->set = 1;
		existing->specificity = state->current_specificity;
		existing->origin = state->current_origin;
		existing->important = important;
		existing->inherit = inherit;
	}

	return outranks;
}

/******************************************************************************
 * Debug helpers                                                              *
 ******************************************************************************/
#ifdef DEBUG_CHAIN_MATCHING
void dump_chain(const css_selector *selector)
{
	const css_selector_detail *detail = &selector->data;

	if (selector->data.comb != CSS_COMBINATOR_NONE)
		dump_chain(selector->combinator);

	if (selector->data.comb == CSS_COMBINATOR_ANCESTOR)
		fprintf(stderr, " ");
	else if (selector->data.comb == CSS_COMBINATOR_SIBLING)
		fprintf(stderr, " + ");
	else if (selector->data.comb == CSS_COMBINATOR_PARENT)
		fprintf(stderr, " > ");

	do {
		switch (detail->type) {
		case CSS_SELECTOR_ELEMENT:
			if (lwc_string_length(detail->qname.name) == 1 &&
				lwc_string_data(detail->qname.name)[0] == '*' &&
					detail->next == 1) {
				break;
			}
			fprintf(stderr, "%.*s",
					(int) lwc_string_length(detail->qname.name),
					lwc_string_data(detail->qname.name));
			break;
		case CSS_SELECTOR_CLASS:
			fprintf(stderr, ".%.*s",
					(int) lwc_string_length(detail->qname.name),
					lwc_string_data(detail->qname.name));
			break;
		case CSS_SELECTOR_ID:
			fprintf(stderr, "#%.*s",
					(int) lwc_string_length(detail->qname.name),
					lwc_string_data(detail->qname.name));
			break;
		case CSS_SELECTOR_PSEUDO_CLASS:
		case CSS_SELECTOR_PSEUDO_ELEMENT:
			fprintf(stderr, ":%.*s",
					(int) lwc_string_length(detail->qname.name),
					lwc_string_data(detail->qname.name));

			if (detail->value.string != NULL) {
				fprintf(stderr, "(%.*s)",
					(int) lwc_string_length(detail->value.string),
					lwc_string_data(detail->value.string));
			}
			break;
		case CSS_SELECTOR_ATTRIBUTE:
			fprintf(stderr, "[%.*s]",
					(int) lwc_string_length(detail->qname.name),
					lwc_string_data(detail->qname.name));
			break;
		case CSS_SELECTOR_ATTRIBUTE_EQUAL:
			fprintf(stderr, "[%.*s=\"%.*s\"]",
					(int) lwc_string_length(detail->qname.name),
					lwc_string_data(detail->qname.name),
					(int) lwc_string_length(detail->value.string),
					lwc_string_data(detail->value.string));
			break;
		case CSS_SELECTOR_ATTRIBUTE_DASHMATCH:
			fprintf(stderr, "[%.*s|=\"%.*s\"]",
					(int) lwc_string_length(detail->qname.name),
					lwc_string_data(detail->qname.name),
					(int) lwc_string_length(detail->value.string),
					lwc_string_data(detail->value.string));
			break;
		case CSS_SELECTOR_ATTRIBUTE_INCLUDES:
			fprintf(stderr, "[%.*s~=\"%.*s\"]",
					(int) lwc_string_length(detail->qname.name),
					lwc_string_data(detail->qname.name),
					(int) lwc_string_length(detail->value.string),
					lwc_string_data(detail->value.string));
			break;
		case CSS_SELECTOR_ATTRIBUTE_PREFIX:
			fprintf(stderr, "[%.*s^=\"%.*s\"]",
					(int) lwc_string_length(detail->qname.name),
					lwc_string_data(detail->qname.name),
					(int) lwc_string_length(detail->value.string),
					lwc_string_data(detail->value.string));
			break;
		case CSS_SELECTOR_ATTRIBUTE_SUFFIX:
			fprintf(stderr, "[%.*s$=\"%.*s\"]",
					(int) lwc_string_length(detail->qname.name),
					lwc_string_data(detail->qname.name),
					(int) lwc_string_length(detail->value.string),
					lwc_string_data(detail->value.string));
			break;
		case CSS_SELECTOR_ATTRIBUTE_SUBSTRING:
			fprintf(stderr, "[%.*s*=\"%.*s\"]",
					(int) lwc_string_length(detail->qname.name),
					lwc_string_data(detail->qname.name),
					(int) lwc_string_length(detail->value.string),
					lwc_string_data(detail->value.string));
			break;
		}

		if (detail->next)
			detail++;
		else
			detail = NULL;
	} while (detail);
}
#endif

static css_error resolve_url(void *pw, const char *base,
        lwc_string *rel, lwc_string **abs)
{
    (void)pw;
    (void)base;

    /* About as useless as possible */
    *abs = lwc_string_ref(rel);

    return CSS_OK;
}


css_error css_element_selector_create(const char *selector,
        css_element_selector **result)
{
    css_error err = CSS_OK;
    if (!selector && !result) {
        err = CSS_BADPARM;
        goto out;
    }
    css_element_selector *sel = (css_element_selector*)calloc(1, sizeof(*sel));
    if (!sel) {
        err = CSS_NOMEM;
        goto out;
    }
    err = css_select_ctx_create(&sel->ctx);
    if (err != CSS_OK) {
        goto out_clear_sel;
    }

    css_stylesheet_params params;
    memset(&params, 0, sizeof(params));
    params.params_version = CSS_STYLESHEET_PARAMS_VERSION_1;
    params.level = CSS_LEVEL_DEFAULT;
    params.charset = "UTF-8";
    params.url = "css_element_selector";
    params.title = "css_element_selector";
    params.resolve = resolve_url;

    err = css_stylesheet_create(&params, &sel->sheet);
    if (err != CSS_OK) {
        goto out_clear_ctx;
    }

    err = css_stylesheet_append_data(sel->sheet, (const uint8_t *)selector,
            strlen(selector));
    if (err != CSS_OK) {
        goto out_clear_sheet;
    }

    err = css_stylesheet_append_data(sel->sheet, (const uint8_t *)"{}", 2);
    if (err != CSS_OK) {
        goto out_clear_sheet;
    }

    err = css_stylesheet_data_done(sel->sheet);
    if (err != CSS_OK) {
        goto out_clear_sheet;
    }

    err = css_select_ctx_append_sheet(sel->ctx,
            sel->sheet, CSS_ORIGIN_AUTHOR, NULL);
    if (err != CSS_OK) {
        goto out_clear_sheet;
    }


    *result = sel;
    goto out;

out_clear_sheet:
    css_stylesheet_destroy(sel->sheet);

out_clear_ctx:
    css_select_ctx_destroy(sel->ctx);

out_clear_sel:
    free(sel);

out:
    return err;
}

css_error css_element_selector_destroy(css_element_selector *selector)
{
    css_error err = CSS_OK;
    if (!selector) {
        err = CSS_BADPARM;
        goto out;
    }

    if (selector->sheet) {
        css_stylesheet_destroy(selector->sheet);
    }

    if (selector->ctx) {
        css_select_ctx_destroy(selector->ctx);
    }

    free(selector);

out:
    return err;
}

css_error css_element_selector_match(css_element_selector *selector,
		void *node, css_select_handler *handler, void *pw, bool *match)
{
    css_error err = CSS_OK;
    if (!selector || !node || !handler || !pw || !match) {
        err = CSS_BADPARM;
        goto out;
    }

out:
    return err;
}


