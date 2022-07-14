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


#ifndef PURC_TEST_INTR_TOOLS_H
#define PURC_TEST_INTR_TOOLS_H

#include "purc-macros.h"
#include "purc-document.h"

#include "config.h"

PCA_EXTERN_C_BEGIN

char *
intr_util_dump_doc(purc_document_t doc, size_t *len);

char *
intr_util_comp_docs(purc_document_t doc_l, purc_document_t doc_r, int *diff);

PCA_EXTERN_C_END

#endif /* PURC_TEST_INTR_TOOLS_H */

