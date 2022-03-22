/*
 * @file hvml-sbst.c
 * @author XueShuming
 * @date 2021/09/06
 * @brief The impl for hvml sbst.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */



#include "hvml-sbst.h"
#include "config.h"
#include "purc-utils.h"

#include "private/sbst.h"
#define PCHTML_HTML_TOKENIZER_RES_ENTITIES_SBST
#include "html/tokenizer/res.h"

#include <stddef.h>
#include <stdint.h>

#if HAVE(GLIB)
#include <gmodule.h>
#else
#include <stdlib.h>
#endif

#if HAVE(GLIB)
#define    PCHVML_ALLOC(sz)   g_slice_alloc0(sz)
#define    PCHVML_FREE(p)     g_slice_free1(sizeof(*p), (gpointer)p)
#else
#define    PCHVML_ALLOC(sz)   calloc(1, sz)
#define    PCHVML_FREE(p)     free(p)
#endif

struct pchvml_sbst {
    const pcutils_sbst_entry_static_t* strt;
    const pcutils_sbst_entry_static_t* root;
    const pcutils_sbst_entry_static_t* match;
    struct pcutils_arrlist* ucs;
};

static
struct pchvml_sbst* pchvml_sbst_new(const pcutils_sbst_entry_static_t* strt)
{
    struct pchvml_sbst* sbst = (struct pchvml_sbst*)
        PCHVML_ALLOC(sizeof(struct pchvml_sbst));
    if (!sbst) {
        return NULL;
    }
    sbst->strt = strt;
    sbst->root = strt + 1;
    sbst->ucs = pcutils_arrlist_new(NULL);
    return sbst;
}

void pchvml_sbst_destroy(struct pchvml_sbst* sbst)
{
    if (sbst) {
        pcutils_arrlist_free(sbst->ucs);
        PCHVML_FREE(sbst);
    }
}

bool pchvml_sbst_advance_ex(struct pchvml_sbst* sbst,
        uint32_t uc, bool case_insensitive)
{
    pcutils_arrlist_append(sbst->ucs, (void*)(uintptr_t)uc);
    if (uc > 0x7F) {
        return false;
    }

    if (case_insensitive && uc >= 'A' && uc <= 'Z') {
        uc = uc | 0x20;
    }
    const pcutils_sbst_entry_static_t* ret =
                pcutils_sbst_entry_static_find(sbst->strt, sbst->root, uc);
    if (ret) {
        if (ret->value) {
            sbst->match = ret;
        }
        sbst->root = sbst->strt + ret->next;
        return true;
    }
    sbst->root = NULL;
    sbst->match = NULL;
    return false;
}

const char* pchvml_sbst_get_match(struct pchvml_sbst* sbst)
{
    if (sbst->match) {
        return (char*)sbst->match->value;
    }
    return NULL;
}

struct pcutils_arrlist* pchvml_sbst_get_buffered_ucs(
        struct pchvml_sbst* sbst)
{
    return sbst->ucs;
}

struct pchvml_sbst* pchvml_sbst_new_char_ref(void)
{
    return pchvml_sbst_new(pchtml_html_tokenizer_res_entities_sbst);
}

static const pcutils_sbst_entry_static_t markup_declaration_open_state_sbst[] =
{
    {0x00, NULL, 0, 0, 0, 0},
    {0x44, NULL, 0, 3, 2, 4},
    {0x5b, NULL, 0, 0, 0, 10},
    {0x2d, NULL, 0, 0, 0, 16},
    {0x4f, NULL, 0, 0, 0, 5},
    {0x43, NULL, 0, 0, 0, 6},
    {0x54, NULL, 0, 0, 0, 7},
    {0x59, NULL, 0, 0, 0, 8},
    {0x50, NULL, 0, 0, 0, 9},
    {0x45, (char*)"\x44\x4f\x43\x54\x59\x50\x45", 7, 0, 0, 0},
    {0x43, NULL, 0, 0, 0, 11},
    {0x44, NULL, 0, 0, 0, 12},
    {0x41, NULL, 0, 0, 0, 13},
    {0x54, NULL, 0, 0, 0, 14},
    {0x41, NULL, 0, 0, 0, 15},
    {0x5b, (char*)"\x5b\x43\x44\x41\x54\x41\x5b", 7, 0, 0, 0},
    {0x2d, (char*)"\x2d\x2d", 2, 0, 0, 0}
};

struct pchvml_sbst* pchvml_sbst_new_markup_declaration_open_state(void)
{
    return pchvml_sbst_new(markup_declaration_open_state_sbst);
}

static const pcutils_sbst_entry_static_t after_doctype_name_state_sbst[] =
{
    {0x00, NULL, 0, 0, 0, 0},
    {0x73, NULL, 0, 2, 0, 3},
    {0x70, NULL, 0, 0, 0, 8},
    {0x79, NULL, 0, 0, 0, 4},
    {0x73, NULL, 0, 0, 0, 5},
    {0x74, NULL, 0, 0, 0, 6},
    {0x65, NULL, 0, 0, 0, 7},
    {0x6d, (char*)"\x53\x59\x53\x54\x45\x4d", 6, 0, 0, 0},
    {0x75, NULL, 0, 0, 0, 9},
    {0x62, NULL, 0, 0, 0, 10},
    {0x6c, NULL, 0, 0, 0, 11},
    {0x69, NULL, 0, 0, 0, 12},
    {0x63, (char*)"\x50\x55\x42\x4c\x49\x43", 6, 0, 0, 0}
};

struct pchvml_sbst* pchvml_sbst_new_after_doctype_name_state(void)
{
    return pchvml_sbst_new(after_doctype_name_state_sbst);
}

/* true false null undefined */
static const pcutils_sbst_entry_static_t ejson_keywords_sbst[] =
{
    {0x00, NULL, 0, 0, 0, 0},
    {0x74, NULL, 0, 3, 2, 5},
    {0x75, NULL, 0, 0, 0, 8},
    {0x6e, NULL, 0, 4, 0, 16},
    {0x66, NULL, 0, 0, 0, 19},
    {0x72, NULL, 0, 0, 0, 6},
    {0x75, NULL, 0, 0, 0, 7},
    {0x65, (char*)"\x74\x72\x75\x65", 4, 0, 0, 0},
    {0x6e, NULL, 0, 0, 0, 9},
    {0x64, NULL, 0, 0, 0, 10},
    {0x65, NULL, 0, 0, 0, 11},
    {0x66, NULL, 0, 0, 0, 12},
    {0x69, NULL, 0, 0, 0, 13},
    {0x6e, NULL, 0, 0, 0, 14},
    {0x65, NULL, 0, 0, 0, 15},
    {0x64, (char*)"\x75\x6e\x64\x65\x66\x69\x6e\x65\x64", 9, 0, 0, 0},
    {0x75, NULL, 0, 0, 0, 17},
    {0x6c, NULL, 0, 0, 0, 18},
    {0x6c, (char*)"\x6e\x75\x6c\x6c", 4, 0, 0, 0},
    {0x61, NULL, 0, 0, 0, 20},
    {0x6c, NULL, 0, 0, 0, 21},
    {0x73, NULL, 0, 0, 0, 22},
    {0x65, (char*)"\x66\x61\x6c\x73\x65", 5, 0, 0, 0}
};

struct pchvml_sbst* pchvml_sbst_new_ejson_keywords(void)
{
    return pchvml_sbst_new(ejson_keywords_sbst);
}
