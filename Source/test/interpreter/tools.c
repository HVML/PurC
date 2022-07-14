/*
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/


#include "tools.h"

#include "private/debug.h"

#include <string.h>

char *
intr_util_dump_doc(purc_document_t doc, size_t *len)
{
    char *retp = NULL;
    unsigned opt = 0;

    opt |= PCDOC_SERIALIZE_OPT_UNDEF;
    opt |= PCDOC_SERIALIZE_OPT_SKIP_WS_NODES;
    opt |= PCDOC_SERIALIZE_OPT_WITHOUT_TEXT_INDENT;
    opt |= PCDOC_SERIALIZE_OPT_FULL_DOCTYPE;

    purc_rwstream_t stm = NULL;
    stm = purc_rwstream_new_buffer(0, 8192);
    if (stm == NULL)
        goto failed;

    if (purc_document_serialize_contents_to_stream(doc, opt, stm))
        goto failed;

    retp = purc_rwstream_get_mem_buffer_ex(stm, len, NULL, true);

failed:
    if (stm)
        purc_rwstream_destroy(stm);

    return retp;
}

char *
intr_util_comp_docs(purc_document_t doc_l, purc_document_t doc_r, int *diff)
{
    char *retp = NULL;
    unsigned opt = 0;

    opt |= PCDOC_SERIALIZE_OPT_UNDEF;
    opt |= PCDOC_SERIALIZE_OPT_SKIP_WS_NODES;
    opt |= PCDOC_SERIALIZE_OPT_WITHOUT_TEXT_INDENT;
    opt |= PCDOC_SERIALIZE_OPT_FULL_DOCTYPE;

    purc_rwstream_t stm_l = NULL, stm_r = NULL;
    stm_l = purc_rwstream_new_buffer(0, 8192);
    stm_r = purc_rwstream_new_buffer(0, 8192);
    if (stm_l == NULL || stm_r == NULL)
        goto failed;

    if (purc_document_serialize_contents_to_stream(doc_l, opt, stm_l) ||
            purc_document_serialize_contents_to_stream(doc_r, opt, stm_r))
        goto failed;

    size_t len_l, len_r;
    char *pl = purc_rwstream_get_mem_buffer_ex(stm_l, &len_l, NULL, true);
    char *pr = purc_rwstream_get_mem_buffer_ex(stm_r, &len_r, NULL, true);

    if (pl && pr) {
        *diff = strcmp(pl, pr);
        if (*diff) {
            PC_DEBUGX("diff:\n%s\n%s", pl, pr);
        }
        retp = pl;
    }
    free(pr);
    // free(pl);

failed:
    if (stm_l)
        purc_rwstream_destroy(stm_l);
    if (stm_r)
        purc_rwstream_destroy(stm_r);

    return retp;
}

