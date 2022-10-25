/*
 * This file is part of CSSEng.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2007 John-Mark Bell <jmb@netsurf-browser.org>
 */

#ifndef css_utils_parserutilserror_h_
#define css_utils_parserutilserror_h_

#include "csseng-parserutils.h"
#include "csseng-errors.h"

/**
 * Convert a ParserUtils error into a CSSEng error
 *
 * \param error  The ParserUtils error to convert
 * \return The corresponding CSSEng error
 */
static inline css_error css_error_from_parserutils_error(
		parserutils_error error)
{
	/* Currently, there's a 1:1 mapping, so we've nothing to do */
	return (css_error) error;
}

#endif

