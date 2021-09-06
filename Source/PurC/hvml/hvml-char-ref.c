/*
 * @file hvml-char-ref.c
 * @author XueShuming
 * @date 2021/09/03
 * @brief The impl for hvml character reference entity.
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



#include "hvml-char-ref.h"
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

struct pchvml_char_ref_search {
    const pchtml_sbst_entry_static_t* strt;
    const pchtml_sbst_entry_static_t* root;
    struct pcutils_arrlist* ucs;
};

struct pchvml_char_ref_search* pchvml_char_ref_search_new(void)
{
    struct pchvml_char_ref_search* search = (struct pchvml_char_ref_search*)
        PCHVML_ALLOC(sizeof(struct pchvml_char_ref_search));
    if (!search) {
        return NULL;
    }
    search->strt = pchtml_html_tokenizer_res_entities_sbst;
    search->root = search->strt + 1;
    search->ucs = pcutils_arrlist_new(NULL);
    return search;
}

void pchvml_char_ref_search_destroy (struct pchvml_char_ref_search* search)
{
    if (search) {
        pcutils_arrlist_free(search->ucs);
        PCHVML_FREE(search);
    }
}

bool pchvml_char_ref_advance (struct pchvml_char_ref_search* search,
        wchar_t uc)
{
    pcutils_arrlist_add (search->ucs, (void*)(uintptr_t)uc);
    if (uc > 0x7F) {
        return false;
    }

    const pchtml_sbst_entry_static_t* ret =
                pchtml_sbst_entry_static_find(search->strt, search->root, uc);
    if (ret) {
        search->root = &pchtml_html_tokenizer_res_entities_sbst[ ret->next ];
        return true;
    }
    search->root = NULL;
    return false;
}

const char* pchvml_char_ref_get_match (struct pchvml_char_ref_search* search)
{
    if (search->root) {
        return (char*)search->root->value;
    }
    return NULL;
}

struct pcutils_arrlist* pchvml_char_ref_get_buffered_ucs (
        struct pchvml_char_ref_search* search)
{
    return search->ucs;
}
