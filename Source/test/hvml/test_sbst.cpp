#include "purc.h"

#include "hvml-buffer.h"
#include "hvml-sbst.h"
#include "private/arraylist.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

#include "sbst.h"
#define PCHTML_HTML_TOKENIZER_RES_ENTITIES_SBST
#include "html/tokenizer/res.h"

TEST(hvml_entity, sbst_find)
{
    const pchtml_sbst_entry_static_t* strt =
        pchtml_html_tokenizer_res_entities_sbst;
    const pchtml_sbst_entry_static_t* root = strt + 1;

    const pchtml_sbst_entry_static_t* ret =
        pchtml_sbst_entry_static_find(strt, root, 'A');
    ASSERT_NE(ret, nullptr);

    root = &pchtml_html_tokenizer_res_entities_sbst[ ret->next ];
    ret = pchtml_sbst_entry_static_find(strt, root, 'M');
    ASSERT_NE(ret, nullptr);

    root = &pchtml_html_tokenizer_res_entities_sbst[ ret->next ];
    ret = pchtml_sbst_entry_static_find(strt, root, 'P');
    ASSERT_NE(ret, nullptr);

    root = &pchtml_html_tokenizer_res_entities_sbst[ ret->next ];
    ret = pchtml_sbst_entry_static_find(strt, root, ';');
    ASSERT_NE(ret, nullptr);
}

TEST(hvml_character_reference, new_destory)
{
    struct pchvml_sbst* search = pchvml_sbst_new_char_ref();
    ASSERT_NE(search, nullptr);
    pchvml_sbst_destroy(search);
}

TEST(hvml_character_reference, match)
{
    struct pchvml_sbst* search = pchvml_sbst_new_char_ref();
    ASSERT_NE(search, nullptr);

    bool ret = pchvml_sbst_advance(search, 'A');
    ASSERT_EQ(ret, true);

    ret = pchvml_sbst_advance(search, 'M');
    ASSERT_EQ(ret, true);

    ret = pchvml_sbst_advance(search, 'P');
    ASSERT_EQ(ret, true);

    ret = pchvml_sbst_advance(search, ';');
    ASSERT_EQ(ret, true);

    pchvml_sbst_destroy(search);
}


TEST(hvml_character_reference, unmatch)
{
    struct pchvml_sbst* search = pchvml_sbst_new_char_ref();
    ASSERT_NE(search, nullptr);

    bool ret = pchvml_sbst_advance(search, 'A');
    ASSERT_EQ(ret, true);

    ret = pchvml_sbst_advance(search, 'M');
    ASSERT_EQ(ret, true);

    ret = pchvml_sbst_advance(search, 'P');
    ASSERT_EQ(ret, true);

    ret = pchvml_sbst_advance(search, 'n');
    ASSERT_EQ(ret, false);

    struct pcutils_arrlist* ucs = pchvml_sbst_get_buffered_ucs(search);
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

    pchvml_sbst_destroy(search);
}

TEST(hvml_markup_declaration_open_state, match_two_minus)
{
    struct pchvml_sbst* search = pchvml_sbst_new_markup_declaration_open_state();
    ASSERT_NE(search, nullptr);

    bool ret = pchvml_sbst_advance(search, '-');
    ASSERT_EQ(ret, true);

    ret = pchvml_sbst_advance(search, '-');
    ASSERT_EQ(ret, true);

    const char* match = pchvml_sbst_get_match(search);
    ASSERT_STREQ(match, "--");

    struct pcutils_arrlist* ucs = pchvml_sbst_get_buffered_ucs(search);
    size_t len = pcutils_arrlist_length(ucs);
    ASSERT_EQ(len, 2);

    pchvml_sbst_destroy(search);
}

TEST(hvml_markup_declaration_open_state, match_doctype)
{
    struct pchvml_sbst* search = pchvml_sbst_new_markup_declaration_open_state();
    ASSERT_NE(search, nullptr);

    bool ret = false;

    ret = pchvml_sbst_advance_ex(search, 'd', true);
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance_ex(search, 'O', true);
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance_ex(search, 'C', true);
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance_ex(search, 'T', true);
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance_ex(search, 'y', true);
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance_ex(search, 'P', true);
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance_ex(search, 'e', true);
    ASSERT_EQ(ret, true);

    const char* match = pchvml_sbst_get_match(search);
    ASSERT_STREQ(match, "DOCTYPE");

    struct pcutils_arrlist* ucs = pchvml_sbst_get_buffered_ucs(search);
    size_t len = pcutils_arrlist_length(ucs);
    ASSERT_EQ(len, 7);

    pchvml_sbst_destroy(search);
}

TEST(hvml_markup_declaration_open_state, match_cdata)
{
    struct pchvml_sbst* search = pchvml_sbst_new_markup_declaration_open_state();
    ASSERT_NE(search, nullptr);

    bool ret = false;

    ret = pchvml_sbst_advance(search, '[');
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance(search, 'C');
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance(search, 'D');
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance(search, 'A');
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance(search, 'T');
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance(search, 'A');
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance(search, '[');
    ASSERT_EQ(ret, true);

    const char* match = pchvml_sbst_get_match(search);
    ASSERT_STREQ(match, "[CDATA[");

    struct pcutils_arrlist* ucs = pchvml_sbst_get_buffered_ucs(search);
    size_t len = pcutils_arrlist_length(ucs);
    ASSERT_EQ(len, 7);

    pchvml_sbst_destroy(search);
}

TEST(hvml_new_after_doctype_name_state, match_public)
{
    struct pchvml_sbst* search = pchvml_sbst_new_after_doctype_name_state();
    ASSERT_NE(search, nullptr);

    bool ret = false;

    ret = pchvml_sbst_advance_ex(search, 'P', true);
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance_ex(search, 'U', true);
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance_ex(search, 'B', true);
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance_ex(search, 'L', true);
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance_ex(search, 'I', true);
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance_ex(search, 'C', true);
    ASSERT_EQ(ret, true);

    const char* match = pchvml_sbst_get_match(search);
    ASSERT_STREQ(match, "PUBLIC");

    struct pcutils_arrlist* ucs = pchvml_sbst_get_buffered_ucs(search);
    size_t len = pcutils_arrlist_length(ucs);
    ASSERT_EQ(len, 6);

    pchvml_sbst_destroy(search);
}

TEST(hvml_new_after_doctype_name_state, match_system)
{
    struct pchvml_sbst* search = pchvml_sbst_new_after_doctype_name_state();
    ASSERT_NE(search, nullptr);

    bool ret = false;

    ret = pchvml_sbst_advance_ex(search, 'S', true);
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance_ex(search, 'Y', true);
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance_ex(search, 'S', true);
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance_ex(search, 'T', true);
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance_ex(search, 'E', true);
    ASSERT_EQ(ret, true);
    ret = pchvml_sbst_advance_ex(search, 'M', true);
    ASSERT_EQ(ret, true);

    const char* match = pchvml_sbst_get_match(search);
    ASSERT_STREQ(match, "SYSTEM");

    struct pcutils_arrlist* ucs = pchvml_sbst_get_buffered_ucs(search);
    size_t len = pcutils_arrlist_length(ucs);
    ASSERT_EQ(len, 6);

    pchvml_sbst_destroy(search);
}
