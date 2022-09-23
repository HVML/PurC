/*
 * This file is part of CSSEng
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * Copyright 2015 Michael Drake <tlsa@netsurf-browser.org>
 */

#ifndef css_select_arena_h_
#define css_select_arena_h_

struct css_computed_style;

/*
 * Add computed style to the style sharing arena, or exchange for existing
 *
 * This takes a computed style.  Note that the original computed style
 * may be freed by this call and all future usage should be via the
 * updated computed style parameter.
 *
 * \params style  The style to intern; possibly freed and updated
 * \return CSS_OK on success or appropriate error otherwise.
 */
enum css_error css__arena_intern_style(struct css_computed_style **style);

/*
 * Remove a computed style from the style sharing arena
 *
 * \params style  The style to remove from the style sharing arena
 * \return CSS_OK on success or appropriate error otherwise.
 */
enum css_error css__arena_remove_style(struct css_computed_style *style);

#endif

