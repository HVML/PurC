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

/* for purc_atom_t */
#include <purc/purc.h>

#include "purcmc-thread.h"

#define FOIL_APP_NAME           "cn.fmsoft.hvml.renderer"
#define FOIL_RUN_NAME           "foil"

#define FOIL_RDR_NAME           "Foil"

#define FOIL_DEF_CHARSET        "UTF-8"
#define FOIL_PX_PER_EM          8
#define FOIL_PX_PER_EX          4

#define FOIL_PX_GRID_CELL_H     8
#define FOIL_PX_GRID_CELL_W     4
#define FOIL_PX_REPLACED_W      320
#define FOIL_PX_REPLACED_H      160

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

typedef struct foil_rect {
    int left, top;
    int right, bottom;
} foil_rect;

typedef struct foil_size{
    int cx;
    int cy;
} foil_size;

enum {
    FOIL_TERM_MODE_LINE = 0,
    FOIL_TERM_MODE_FULL_SCREEN,
};

struct pcmcth_rdr_data {
    int term_mode;
    int rows, cols;
};

static inline int foil_rect_width(const foil_rect *rc)
{
    return rc->right - rc->left;
}

static inline int foil_rect_height(const foil_rect *rc)
{
    return rc->bottom - rc->top;
}

#ifdef __cplusplus
extern "C" {
#endif

purc_atom_t foil_init(const char *rdr_uri);

#ifdef __cplusplus
}
#endif

#endif  /* purc_foil_h */

