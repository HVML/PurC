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

#include "purc/purc-macros.h"
#include "purc/purc-document.h"

#include "config.h"

#if OS(LINUX)
    #define TCS_NONE            "\e[0m"
    #define TCS_BLACK           "\e[0;30m"
    #define TCS_BOLD_BLACK      "\e[1;30m"
    #define TCS_RED             "\e[0;31m"
    #define TCS_BOLD_RED        "\e[1;31m"
    #define TCS_GREEN           "\e[0;32m"
    #define TCS_BOLD_GREEN      "\e[1;32m"
    #define TCS_BROWN           "\e[0;33m"
    #define TCS_YELLOW          "\e[1;33m"
    #define TCS_BLUE            "\e[0;34m"
    #define TCS_BOLD_BLUE       "\e[1;34m"
    #define TCS_PURPLE          "\e[0;35m"
    #define TCS_BOLD_PURPLE     "\e[1;35m"
    #define TCS_CYAN            "\e[0;36m"
    #define TCS_BOLD_CYAN       "\e[1;36m"
    #define TCS_GRAY            "\e[0;37m"
    #define TCS_WHITE           "\e[1;37m"
    #define TCS_BOLD            "\e[1m"
    #define TCS_UNDERLINE       "\e[4m"
    #define TCS_BLINK           "\e[5m"
    #define TCS_REVERSE         "\e[7m"
    #define TCS_HIDE            "\e[8m"
    #define TCS_CLEAR           "\e[2J"
    #define TCS_CLRLINE         "\e[1K\r"
#else
    #define TCS_NONE            ""
    #define TCS_BLACK           ""
    #define TCS_BOLD_BLACK      ""
    #define TCS_RED             ""
    #define TCS_BOLD_RED        ""
    #define TCS_GREEN           ""
    #define TCS_BOLD_GREEN      ""
    #define TCS_BROWN           ""
    #define TCS_YELLOW          ""
    #define TCS_BLUE            ""
    #define TCS_BOLD_BLUE       ""
    #define TCS_PURPLE          ""
    #define TCS_BOLD_PURPLE     ""
    #define TCS_CYAN            ""
    #define TCS_BOLD_CYAN       ""
    #define TCS_GRAY            ""
    #define TCS_WHITE           ""
    #define TCS_BOLD            ""
    #define TCS_UNDERLINE       ""
    #define TCS_BLINK           ""
    #define TCS_REVERSE         ""
    #define TCS_HIDE            ""
    #define TCS_CLEAR           ""
    #define TCS_CLRLINE         ""
#endif

PCA_EXTERN_C_BEGIN

char *
intr_util_dump_doc(purc_document_t doc, size_t *len);

char *
intr_util_comp_docs(purc_document_t doc_l, purc_document_t doc_r, int *diff);

PCA_EXTERN_C_END

#endif /* PURC_TEST_INTR_TOOLS_H */

