/*
 * This file is part of CSSEng.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2007 John-Mark Bell <jmb@netsurf-browser.org>
 */

#ifndef libcss_types_h_
#define libcss_types_h_

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "csseng-wapcaplet.h"
#include "csseng-fpmath.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Source of charset information, in order of importance.
 * A client-dictated charset will override all others.
 * A document-specified charset will override autodetection or the default.
 */
typedef enum css_charset_source {
	CSS_CHARSET_DEFAULT          = 0,	/**< Default setting */
	CSS_CHARSET_REFERRED         = 1,	/**< From referring document */
	CSS_CHARSET_METADATA         = 2,	/**< From linking metadata */
	CSS_CHARSET_DOCUMENT         = 3,	/**< Defined in document */
	CSS_CHARSET_DICTATED         = 4	/**< Dictated by client */
} css_charset_source;

/**
 * Stylesheet language level -- defines parsing rules and supported properties
 */
typedef enum css_language_level {
	CSS_LEVEL_1                 = 0,	/**< CSS 1 */
	CSS_LEVEL_2                 = 1,	/**< CSS 2 */
	CSS_LEVEL_21                = 2,	/**< CSS 2.1 */
	CSS_LEVEL_3                 = 3,	/**< CSS 3 */
	CSS_LEVEL_DEFAULT           = CSS_LEVEL_21	/**< Default level */
} css_language_level;

/**
 * Stylesheet media types
 */
typedef enum css_media_type {
	CSS_MEDIA_AURAL             = (1 << 0),
	CSS_MEDIA_BRAILLE           = (1 << 1),
	CSS_MEDIA_EMBOSSED          = (1 << 2),
	CSS_MEDIA_HANDHELD          = (1 << 3),
	CSS_MEDIA_PRINT             = (1 << 4),
	CSS_MEDIA_PROJECTION        = (1 << 5),
	CSS_MEDIA_SCREEN            = (1 << 6),
	CSS_MEDIA_SPEECH            = (1 << 7),
	CSS_MEDIA_TTY               = (1 << 8),
	CSS_MEDIA_TV                = (1 << 9),
	CSS_MEDIA_ALL               = CSS_MEDIA_AURAL | CSS_MEDIA_BRAILLE |
	                              CSS_MEDIA_EMBOSSED | CSS_MEDIA_HANDHELD |
	                              CSS_MEDIA_PRINT | CSS_MEDIA_PROJECTION |
	                              CSS_MEDIA_SCREEN | CSS_MEDIA_SPEECH |
	                              CSS_MEDIA_TTY | CSS_MEDIA_TV
} css_media_type;

/**
 * Stylesheet origin
 */
typedef enum css_origin {
	CSS_ORIGIN_UA               = 0,	/**< User agent stylesheet */
	CSS_ORIGIN_USER             = 1,	/**< User stylesheet */
	CSS_ORIGIN_AUTHOR           = 2		/**< Author stylesheet */
} css_origin;

/** CSS colour -- AARRGGBB */
typedef uint32_t css_color;

/* CSS unit */
typedef enum css_unit {
	CSS_UNIT_PX                 = 0x00,
	CSS_UNIT_EX                 = 0x01,
	CSS_UNIT_EM                 = 0x02,
	CSS_UNIT_IN                 = 0x03,
	CSS_UNIT_CM                 = 0x04,
	CSS_UNIT_MM                 = 0x05,
	CSS_UNIT_PT                 = 0x06,
	CSS_UNIT_PC                 = 0x07,
	CSS_UNIT_CAP                = 0x08,
	CSS_UNIT_CH                 = 0x09,
	CSS_UNIT_IC                 = 0x0a,
	CSS_UNIT_REM                = 0x0b,
	CSS_UNIT_LH                 = 0x0c,
	CSS_UNIT_RLH                = 0x0d,
	CSS_UNIT_VH                 = 0x0e,
	CSS_UNIT_VW                 = 0x0f,
	CSS_UNIT_VI                 = 0x10,
	CSS_UNIT_VB                 = 0x11,
	CSS_UNIT_VMIN               = 0x12,
	CSS_UNIT_VMAX               = 0x13,
	CSS_UNIT_Q                  = 0x14,

	CSS_UNIT_PCT                = 0x15,	/* Percentage */

	CSS_UNIT_DEG                = 0x16,
	CSS_UNIT_GRAD               = 0x17,
	CSS_UNIT_RAD                = 0x18,

	CSS_UNIT_MS                 = 0x19,
	CSS_UNIT_S                  = 0x1a,

	CSS_UNIT_HZ                 = 0x1b,
	CSS_UNIT_KHZ                = 0x1c
} css_unit;

/**
 * Media orienations
 */
typedef enum css_media_orientation {
	CSS_MEDIA_ORIENTATION_PORTRAIT  = 0,
	CSS_MEDIA_ORIENTATION_LANDSCAPE = 1
} css_media_orientation;

/**
 * Media scans
 */
typedef enum css_media_scan {
	CSS_MEDIA_SCAN_PROGRESSIVE = 0,
	CSS_MEDIA_SCAN_INTERLACE   = 1
} css_media_scan;

/**
 * Media update-frequencies
 */
typedef enum css_media_update_frequency {
	CSS_MEDIA_UPDATE_FREQUENCY_NORMAL = 0,
	CSS_MEDIA_UPDATE_FREQUENCY_SLOW   = 1,
	CSS_MEDIA_UPDATE_FREQUENCY_NONE   = 2
} css_media_update_frequency;

/**
 * Media block overflows
 */
typedef enum css_media_overflow_block {
	CSS_MEDIA_OVERFLOW_BLOCK_NONE           = 0,
	CSS_MEDIA_OVERFLOW_BLOCK_SCROLL         = 1,
	CSS_MEDIA_OVERFLOW_BLOCK_OPTIONAL_PAGED = 2,
	CSS_MEDIA_OVERFLOW_BLOCK_PAGED          = 3
} css_media_overflow_block;

/**
 * Media inline overflows
 */
typedef enum css_media_overflow_inline {
	CSS_MEDIA_OVERFLOW_INLINE_NONE   = 0,
	CSS_MEDIA_OVERFLOW_INLINE_SCROLL = 1
} css_media_overflow_inline;

/**
 * Media pointers
 */
typedef enum css_media_pointer {
	CSS_MEDIA_POINTER_NONE   = 0,
	CSS_MEDIA_POINTER_COARSE = 1,
	CSS_MEDIA_POINTER_FINE   = 2
} css_media_pointer;

/**
 * Media hovers
 */
typedef enum css_media_hover {
	CSS_MEDIA_HOVER_NONE      = 0,
	CSS_MEDIA_HOVER_ON_DEMAND = 1,
	CSS_MEDIA_HOVER_HOVER     = 2
} css_media_hover;

/**
 * Media light-levels
 */
typedef enum css_media_light_level {
	CSS_MEDIA_LIGHT_LEVEL_NORMAL = 0,
	CSS_MEDIA_LIGHT_LEVEL_DIM    = 1,
	CSS_MEDIA_LIGHT_LEVEL_WASHED = 2
} css_media_light_level;

/**
 * Media scriptings
 */
typedef enum css_media_scripting {
	CSS_MEDIA_SCRIPTING_NONE         = 0,
	CSS_MEDIA_SCRIPTING_INITIAL_ONLY = 1,
	CSS_MEDIA_SCRIPTING_ENABLED      = 2
} css_media_scripting;

typedef struct css_media_resolution {
	css_fixed value;
	css_unit unit;
} css_media_resolution;

/**
 * Media specification
 */
typedef struct css_media {
	/* Media type */
	css_media_type        type;

	/* Viewport / page media features */
	css_fixed             width;  /* In css pixels */
	css_fixed             height; /* In css pixels */
	css_fixed             aspect_ratio;
	css_media_orientation orientation;

	/* Display quality media features */
	css_media_resolution       resolution;
	css_media_scan             scan;
	css_fixed                  grid; /** boolean: {0|1} */
	css_media_update_frequency update;
	css_media_overflow_block   overflow_block;
	css_media_overflow_inline  overflow_inline;

	/* Color media features */
	css_fixed color;      /* colour bpp (0 for monochrome) */
	css_fixed color_index;
	css_fixed monochrome; /* monochrome bpp (0 for colour) */
	css_fixed inverted_colors; /** boolean: {0|1} */

	/* Interaction media features */
	css_media_pointer pointer;
	css_media_pointer any_pointer;
	css_media_hover   hover;
	css_media_hover   any_hover;

	/* Environmental media features */
	css_media_light_level light_level;

	/* Scripting media features */
	css_media_scripting scripting;

	/* Client details for length conversion */
	css_fixed client_font_size;   /* In pt */
	css_fixed client_line_height; /* In css pixels */
} css_media;

/**
 * Type of a qualified name
 */
typedef struct css_qname {
	/**
	 * Namespace URI:
	 *
	 * NULL for no namespace
	 * '*' for any namespace (including none)
	 * URI for a specific namespace
	 */
	lwc_string *ns;

	/**
	 * Local part of qualified name
	 */
	lwc_string *name;
} css_qname;

typedef struct css_stylesheet css_stylesheet;

typedef struct css_select_ctx css_select_ctx;

typedef struct css_computed_style css_computed_style;

typedef struct css_font_face css_font_face;

typedef struct css_font_face_src css_font_face_src;

#ifdef __cplusplus
}
#endif

#endif
