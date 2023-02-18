/*
** @file foil.h
** @author Vincent Wei
** @date 2022/09/30
** @brief The global definitions for the renderer Foil.
**
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of purc, which is an HVML interpreter with
** a command line interface (CLI).
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef purc_foil_h
#define purc_foil_h

#include <time.h>

#include <purc/purc.h>

#include "purcmc-thread.h"

#define FOIL_APP_NAME           "cn.fmsoft.hvml.renderer"
#define FOIL_RUN_NAME           "foil"

#define FOIL_RDR_NAME           "Foil"

#define FOIL_DEF_CHARSET        "UTF-8"
#define FOIL_DEF_DPI            96.0

/* maximum size of the buffer for cells in a page: 512 * 256 * 8 (1 MiB) */
#define FOIL_MAX_COLS           512
#define FOIL_MAX_ROWS           256

#define FOIL_PX_GRID_CELL_H     8
#define FOIL_PX_GRID_CELL_W     4
#define FOIL_PX_REPLACED_W      (FOIL_PX_GRID_CELL_W * 20)
#define FOIL_PX_REPLACED_H      (FOIL_PX_GRID_CELL_H * 5)

#define FOIL_PX_PER_EM          FOIL_PX_GRID_CELL_H
#define FOIL_PX_PER_EX          FOIL_PX_GRID_CELL_W

#define FOIL_RDR_FEATURES \
    PCRDR_PURCMC_PROTOCOL_NAME ":" PCRDR_PURCMC_PROTOCOL_VERSION_STRING "\n" \
    FOIL_RDR_NAME ":" PURC_VERSION_STRING "\n" \
    "HTML:5.3\n" \
    "workspace:0/tabbedWindow:-1/plainWindow:-1/widgetInTabbedWindow:8\n" \
    "DOMElementSelectors:handle"

#ifdef NDEBUG
#   define LOG_DEBUG(x, ...)
#else
#   define LOG_DEBUG(x, ...)   \
    purc_log_debug("%s: " x, __func__, ##__VA_ARGS__)
#endif /* not defined NDEBUG */

#ifdef LOG_ERROR
#   undef LOG_ERROR
#endif

#define LOG_ERROR(x, ...)   \
    purc_log_error("%s: " x, __func__, ##__VA_ARGS__)

#define LOG_WARN(x, ...)    \
    purc_log_warn("%s: " x, __func__, ##__VA_ARGS__)

#define LOG_INFO(x, ...)    \
    purc_log_info("%s: " x, __func__, ##__VA_ARGS__)

#ifndef MIN
#   define MIN(x, y)   (((x) > (y)) ? (y) : (x))
#endif

#ifndef MAX
#   define MAX(x, y)   (((x) < (y)) ? (y) : (x))
#endif

/* round n to multiple of m */
#define ROUND_TO_MULTIPLE(n, m) (((n) + (((m) - 1))) & ~((m) - 1))

#if defined(_WIN64)
#   define SIZEOF_PTR   8
#   define SIZEOF_HPTR  4
#elif defined(__LP64__)
#   define SIZEOF_PTR   8
#   define SIZEOF_HPTR  4
#else
#   define SIZEOF_PTR   4
#   define SIZEOF_HPTR  2
#endif

enum {
    FOIL_TERM_MODE_LINE = 0,
    FOIL_TERM_MODE_FULL_SCREEN,
};

/* the standard 16 colors. */
enum {
    FOIL_STD_COLOR_BLACK = 0,
    FOIL_STD_COLOR_DARK_RED,
    FOIL_STD_COLOR_DARK_GREEN,
    FOIL_STD_COLOR_DARK_YELLOW,
    FOIL_STD_COLOR_DARK_BLUE,
    FOIL_STD_COLOR_DARK_MAGENTA,
    FOIL_STD_COLOR_DARK_CYAN,
    FOIL_STD_COLOR_GRAY,
    FOIL_STD_COLOR_DARK_GRAY,
    FOIL_STD_COLOR_RED,
    FOIL_STD_COLOR_GREEN,
    FOIL_STD_COLOR_YELLOW,
    FOIL_STD_COLOR_BLUE,
    FOIL_STD_COLOR_MAGENTA,
    FOIL_STD_COLOR_CYAN,
    FOIL_STD_COLOR_WHITE,
};

typedef struct foil_color {
    bool        specified;  /* false (zero) for default */
    uint32_t    argb;
} foil_color;

#define FOIL_DEF_FGC            0xFFA0A0A0
#define FOIL_DEF_BGC            0xFF000000

#define FOIL_COLOR_INFO         0xFF087990
#define FOIL_COLOR_WARNING      0xFF997404
#define FOIL_COLOR_DANGER       0xFFB02A37
#define FOIL_COLOR_SUCCESS      0xFF146C43
#define FOIL_COLOR_PRIMARY      0xFF0A58CA
#define FOIL_COLOR_SECONDARY    0xFF6C757D

enum {
    FOIL_CHAR_ATTR_NULL         = 0x00,
    FOIL_CHAR_ATTR_BOLD         = 0x01,
    FOIL_CHAR_ATTR_UNDERLINE    = 0x02,
    FOIL_CHAR_ATTR_STRIKEOUT    = 0x04,
    FOIL_CHAR_ATTR_BLINK        = 0x08,
    FOIL_CHAR_ATTR_REVERSE      = 0x10,
};

struct pcmcth_rdr_data {
    int term_mode;
    int rows, cols;
};

#ifdef __cplusplus
extern "C" {
#endif

/** Starts Foil renderer */
purc_atom_t foil_start(const char *rdr_uri);

/** Returns the pointer to the Foil renderer. */
pcmcth_renderer *foil_get_renderer(void);

/** Wait for Foil renderer to exit synchronously */
void foil_sync_exit(void);

int foil_doc_get_element_lang(purc_document_t doc, pcdoc_element_t ele,
        const char **lang, size_t *len);

int foil_ucs_calc_width_nowrap(const uint32_t *ucs, size_t nr_ucs);

uint8_t foil_map_xrgb_to_256c(uint32_t xrgb);
uint8_t foil_map_xrgb_to_16c(uint32_t xrgb);

#ifdef __cplusplus
}
#endif

#endif  /* purc_foil_h */

