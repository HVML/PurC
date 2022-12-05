/*
 * @file udom.h
 * @author Vincent Wei
 * @date 2022/10/06
 * @brief The global definitions for the ultimate DOM.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of purc, which is an HVML interpreter with
 * a command line interface (CLI).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef purc_foil_udom_h
#define purc_foil_udom_h

#include "foil.h"
#include "rdrbox.h"
#include "util/sorted-array.h"
#include "util/list.h"
#include "region/region.h"

#include <purc/purc-document.h>
#include <glib.h>

#define FOIL_DEF_FGC            0xFFFFFFFF
#define FOIL_DEF_BGC            0xFF000000
#define FOIL_DEF_RGNRCHEAP_SZ   16

struct pcmcth_udom {
    /* the sorted array of eDOM element and the corresponding CSS node data. */
    struct sorted_array *elem2nodedata;

    /* the sorted array of eDOM element and the corresponding rendering box. */
    struct sorted_array *elem2rdrbox;

    /* purc_document */
    purc_document_t doc;

    struct purc_broken_down_url *base;

    /* author-defined style sheet */
    css_stylesheet *author_sheet;

    /* CSS selection context */
    css_select_ctx *select_ctx;

    /* the initial containing block,
       it's also the root node of the rendering tree. */
    struct foil_rdrbox *initial_cblock;

    /* the CSS media */
    css_media media;

    /* size of whole page in pixels */
    unsigned width, height;

    /* size of page in rows and columns */
    unsigned cols, rows;

    /* title */
    uint32_t *title_ucs;
    size_t    title_len;

    /* quoting depth */
    int nr_open_quotes;
    int nr_close_quotes;

    /* the block heap for region rectangles */
    foil_block_heap rgnrc_heap;

    /* the pointer to the stacking context created by the root element */
    struct foil_stacking_context *root_stk_ctxt;
};

typedef struct foil_stacking_context {
    /* the parent stacking context; NULL for root stacking context. */
    struct foil_stacking_context   *parent;

    /* the creator of this stacking context */
    foil_rdrbox                    *creator;

    /* the z-index of this stacking context */
    int                             zidx;

    /* the array of child stacking contexts sorted by z-index. */
    struct sorted_array            *zidx2child;

    /* the list node used to link sibling stacking contexts
       which have the same z-index. */
    struct list_head                list;
} foil_stacking_context;

#ifdef __cplusplus
extern "C" {
#endif

int foil_udom_module_init(pcmcth_renderer *rdr);
void foil_udom_module_cleanup(pcmcth_renderer *rdr);

pcmcth_udom *foil_udom_new(pcmcth_page *page);
void foil_udom_delete(pcmcth_udom *udom);

foil_rdrbox *foil_udom_find_rdrbox(pcmcth_udom *udom,
        uint64_t element_handle);

pcmcth_udom *foil_udom_load_edom(pcmcth_page *page,
        purc_variant_t edom, int *retv);

foil_stacking_context *foil_stacking_context_new(
        foil_stacking_context *parent, int zidx, foil_rdrbox *creator);

int foil_stacking_context_detach(foil_stacking_context *parent,
        foil_stacking_context *ctxt);
int foil_stacking_context_delete(foil_stacking_context *ctxt);

uint8_t foil_udom_get_langcode(purc_document_t doc, pcdoc_element_t elem);

int foil_udom_update_rdrbox(pcmcth_udom *udom, foil_rdrbox *rdrbox,
        int op, const char *property, purc_variant_t ref_info);

purc_variant_t foil_udom_call_method(pcmcth_udom *udom, foil_rdrbox *rdrbox,
        const char *method, purc_variant_t arg);

purc_variant_t foil_udom_get_property(pcmcth_udom *udom, foil_rdrbox *rdrbox,
        const char *property);

purc_variant_t foil_udom_set_property(pcmcth_udom *udom, foil_rdrbox *rdrbox,
        const char *property, purc_variant_t value);

#ifdef __cplusplus
}
#endif

#endif  /* purc_foil_udom_h */

