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

#include "purc.h"

#include "private/tkz-helper.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

#include "private/sbst.h"
#define PCHTML_HTML_TOKENIZER_RES_ENTITIES_SBST
#include "html/tokenizer/res.h"

TEST(hvml_entity, sbst_find)
{
    const pcutils_sbst_entry_static_t* strt =
        pchtml_html_tokenizer_res_entities_sbst;
    const pcutils_sbst_entry_static_t* root = strt + 1;

    const pcutils_sbst_entry_static_t* ret =
        pcutils_sbst_entry_static_find(strt, root, 'A');
    ASSERT_NE(ret, nullptr);

    root = &pchtml_html_tokenizer_res_entities_sbst[ ret->next ];
    ret = pcutils_sbst_entry_static_find(strt, root, 'M');
    ASSERT_NE(ret, nullptr);

    root = &pchtml_html_tokenizer_res_entities_sbst[ ret->next ];
    ret = pcutils_sbst_entry_static_find(strt, root, 'P');
    ASSERT_NE(ret, nullptr);

    root = &pchtml_html_tokenizer_res_entities_sbst[ ret->next ];
    ret = pcutils_sbst_entry_static_find(strt, root, ';');
    ASSERT_NE(ret, nullptr);
}

TEST(hvml_character_reference, new_destory)
{
    struct tkz_sbst* search = tkz_sbst_new_char_ref();
    ASSERT_NE(search, nullptr);
    tkz_sbst_destroy(search);
}

TEST(hvml_character_reference, match)
{
    struct tkz_sbst* search = tkz_sbst_new_char_ref();
    ASSERT_NE(search, nullptr);

    bool ret = tkz_sbst_advance(search, 'A');
    ASSERT_EQ(ret, true);

    ret = tkz_sbst_advance(search, 'M');
    ASSERT_EQ(ret, true);

    ret = tkz_sbst_advance(search, 'P');
    ASSERT_EQ(ret, true);

    ret = tkz_sbst_advance(search, ';');
    ASSERT_EQ(ret, true);

    tkz_sbst_destroy(search);
}


TEST(hvml_character_reference, unmatch)
{
    struct tkz_sbst* search = tkz_sbst_new_char_ref();
    ASSERT_NE(search, nullptr);

    bool ret = tkz_sbst_advance(search, 'A');
    ASSERT_EQ(ret, true);

    ret = tkz_sbst_advance(search, 'M');
    ASSERT_EQ(ret, true);

    ret = tkz_sbst_advance(search, 'P');
    ASSERT_EQ(ret, true);

    ret = tkz_sbst_advance(search, 'n');
    ASSERT_EQ(ret, false);

    struct pcutils_arrlist* ucs = tkz_sbst_get_buffered_ucs(search);
    size_t len = pcutils_arrlist_length(ucs);
    ASSERT_EQ(len, 4);

    wchar_t uc = (wchar_t)(uintptr_t) pcutils_arrlist_get_idx(ucs, 0);
    ASSERT_EQ(uc, 'A');

    uc = (wchar_t)(uintptr_t) pcutils_arrlist_get_idx(ucs, 1);
    ASSERT_EQ(uc, 'M');

    uc = (wchar_t)(uintptr_t) pcutils_arrlist_get_idx(ucs, 2);
    ASSERT_EQ(uc, 'P');

    uc = (wchar_t)(uintptr_t) pcutils_arrlist_get_idx(ucs, 3);
    ASSERT_EQ(uc, 'n');

    tkz_sbst_destroy(search);
}

TEST(hvml_markup_declaration_open_state, match_two_minus)
{
    struct tkz_sbst* search = tkz_sbst_new_markup_declaration_open_state();
    ASSERT_NE(search, nullptr);

    bool ret = tkz_sbst_advance(search, '-');
    ASSERT_EQ(ret, true);

    ret = tkz_sbst_advance(search, '-');
    ASSERT_EQ(ret, true);

    const char* match = tkz_sbst_get_match(search);
    ASSERT_STREQ(match, "--");

    struct pcutils_arrlist* ucs = tkz_sbst_get_buffered_ucs(search);
    size_t len = pcutils_arrlist_length(ucs);
    ASSERT_EQ(len, 2);

    tkz_sbst_destroy(search);
}

TEST(hvml_markup_declaration_open_state, match_doctype)
{
    struct tkz_sbst* search = tkz_sbst_new_markup_declaration_open_state();
    ASSERT_NE(search, nullptr);

    bool ret = false;

    ret = tkz_sbst_advance_ex(search, 'D', false);
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance_ex(search, 'O', false);
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance_ex(search, 'C', false);
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance_ex(search, 'T', false);
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance_ex(search, 'Y', false);
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance_ex(search, 'P', false);
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance_ex(search, 'E', false);
    ASSERT_EQ(ret, true);

    const char* match = tkz_sbst_get_match(search);
    ASSERT_STREQ(match, "DOCTYPE");

    struct pcutils_arrlist* ucs = tkz_sbst_get_buffered_ucs(search);
    size_t len = pcutils_arrlist_length(ucs);
    ASSERT_EQ(len, 7);

    tkz_sbst_destroy(search);
}

TEST(hvml_markup_declaration_open_state, match_cdata)
{
    struct tkz_sbst* search = tkz_sbst_new_markup_declaration_open_state();
    ASSERT_NE(search, nullptr);

    bool ret = false;

    ret = tkz_sbst_advance(search, '[');
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance(search, 'C');
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance(search, 'D');
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance(search, 'A');
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance(search, 'T');
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance(search, 'A');
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance(search, '[');
    ASSERT_EQ(ret, true);

    const char* match = tkz_sbst_get_match(search);
    ASSERT_STREQ(match, "[CDATA[");

    struct pcutils_arrlist* ucs = tkz_sbst_get_buffered_ucs(search);
    size_t len = pcutils_arrlist_length(ucs);
    ASSERT_EQ(len, 7);

    tkz_sbst_destroy(search);
}

TEST(hvml_new_after_doctype_name_state, match_public)
{
    struct tkz_sbst* search = tkz_sbst_new_after_doctype_name_state();
    ASSERT_NE(search, nullptr);

    bool ret = false;

    ret = tkz_sbst_advance_ex(search, 'P', true);
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance_ex(search, 'U', true);
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance_ex(search, 'B', true);
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance_ex(search, 'L', true);
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance_ex(search, 'I', true);
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance_ex(search, 'C', true);
    ASSERT_EQ(ret, true);

    const char* match = tkz_sbst_get_match(search);
    ASSERT_STREQ(match, "PUBLIC");

    struct pcutils_arrlist* ucs = tkz_sbst_get_buffered_ucs(search);
    size_t len = pcutils_arrlist_length(ucs);
    ASSERT_EQ(len, 6);

    tkz_sbst_destroy(search);
}

TEST(hvml_new_after_doctype_name_state, match_system)
{
    struct tkz_sbst* search = tkz_sbst_new_after_doctype_name_state();
    ASSERT_NE(search, nullptr);

    bool ret = false;

    ret = tkz_sbst_advance_ex(search, 'S', true);
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance_ex(search, 'Y', true);
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance_ex(search, 'S', true);
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance_ex(search, 'T', true);
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance_ex(search, 'E', true);
    ASSERT_EQ(ret, true);
    ret = tkz_sbst_advance_ex(search, 'M', true);
    ASSERT_EQ(ret, true);

    const char* match = tkz_sbst_get_match(search);
    ASSERT_STREQ(match, "SYSTEM");

    struct pcutils_arrlist* ucs = tkz_sbst_get_buffered_ucs(search);
    size_t len = pcutils_arrlist_length(ucs);
    ASSERT_EQ(len, 6);

    tkz_sbst_destroy(search);
}
