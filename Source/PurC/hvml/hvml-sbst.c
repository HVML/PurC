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
#include "private/arraylist.h"

#include "html/sbst.h"
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
    const pchtml_sbst_entry_static_t* strt;
    const pchtml_sbst_entry_static_t* root;
    struct pcutils_arrlist* ucs;
};

static
struct pchvml_sbst* pchvml_sbst_new(const pchtml_sbst_entry_static_t* strt)
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

void pchvml_sbst_destroy (struct pchvml_sbst* sbst)
{
    if (sbst) {
        pcutils_arrlist_free(sbst->ucs);
        PCHVML_FREE(sbst);
    }
}

bool pchvml_sbst_advance (struct pchvml_sbst* sbst,
        wchar_t uc)
{
    pcutils_arrlist_add (sbst->ucs, (void*)(uintptr_t)uc);
    if (uc > 0x7F) {
        return false;
    }

    const pchtml_sbst_entry_static_t* ret =
                pchtml_sbst_entry_static_find(sbst->strt, sbst->root, uc);
    if (ret) {
        sbst->root = sbst->strt + ret->next;
        return true;
    }
    sbst->root = NULL;
    return false;
}

bool pchvml_sbst_advance_case_insensitive (struct pchvml_sbst* sbst,
        wchar_t uc)
{
    pcutils_arrlist_add (sbst->ucs, (void*)(uintptr_t)uc);
    if (uc > 0x7F) {
        return false;
    }

    if (uc >= 'A' && uc <= 'Z') {
        uc = uc | 0x20;
    }
    const pchtml_sbst_entry_static_t* ret =
                pchtml_sbst_entry_static_find(sbst->strt, sbst->root, uc);
    if (ret) {
        sbst->root = sbst->strt + ret->next;
        return true;
    }
    sbst->root = NULL;
    return false;
}

const char* pchvml_sbst_get_match (struct pchvml_sbst* sbst)
{
    if (sbst->root) {
        return (char*)sbst->root->value;
    }
    return NULL;
}

struct pcutils_arrlist* pchvml_sbst_get_buffered_ucs (
        struct pchvml_sbst* sbst)
{
    return sbst->ucs;
}

struct pchvml_sbst* pchvml_sbst_new_char_ref(void)
{
    return pchvml_sbst_new(pchtml_html_tokenizer_res_entities_sbst);
}

static const pchtml_sbst_entry_static_t markup_declaration_open_state_sbst[] =
{
    {0x00, NULL, 0, 0, 0, 0},
    {0x5b, NULL, 0, 3, 2, 4},
    {0x64, NULL, 0, 0, 0, 10},
    {0x2d, NULL, 0, 0, 0, 16},
    {0x43, NULL, 0, 0, 0, 5},
    {0x44, NULL, 0, 0, 0, 6},
    {0x41, NULL, 0, 0, 0, 7},
    {0x54, NULL, 0, 0, 0, 8},
    {0x41, NULL, 0, 0, 0, 9},
    {0x5b, (char*)"\x5b\x43\x44\x41\x54\x41\x5b", 7, 0, 0, 0},
    {0x6f, NULL, 0, 0, 0, 11},
    {0x63, NULL, 0, 0, 0, 12},
    {0x74, NULL, 0, 0, 0, 13},
    {0x79, NULL, 0, 0, 0, 14},
    {0x70, NULL, 0, 0, 0, 15},
    {0x65, (char*)"\x44\x4f\x43\x54\x59\x50\x45", 7, 0, 0, 0},
    {0x2d, (char*)"\x2d\x2d", 2, 0, 0, 0}
};

struct pchvml_sbst* pchvml_sbst_new_markup_declaration_open_state(void)
{
    return pchvml_sbst_new(markup_declaration_open_state_sbst);
}
