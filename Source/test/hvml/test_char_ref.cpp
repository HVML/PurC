#include "purc.h"

#include "tempbuffer.h"
#include "hvml-char-ref.h"
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
    struct pchvml_char_ref_search* search = pchvml_char_ref_search_new();
    ASSERT_NE(search, nullptr);
    pchvml_char_ref_search_destroy(search);
}

TEST(hvml_character_reference, match)
{
    struct pchvml_char_ref_search* search = pchvml_char_ref_search_new();
    ASSERT_NE(search, nullptr);

    bool ret = pchvml_char_ref_advance(search, 'A');
    ASSERT_EQ(ret, true);

    ret = pchvml_char_ref_advance(search, 'M');
    ASSERT_EQ(ret, true);

    ret = pchvml_char_ref_advance(search, 'P');
    ASSERT_EQ(ret, true);

    ret = pchvml_char_ref_advance(search, ';');
    ASSERT_EQ(ret, true);

    pchvml_char_ref_search_destroy(search);
}


TEST(hvml_character_reference, unmatch)
{
    struct pchvml_char_ref_search* search = pchvml_char_ref_search_new();
    ASSERT_NE(search, nullptr);

    bool ret = pchvml_char_ref_advance(search, 'A');
    ASSERT_EQ(ret, true);

    ret = pchvml_char_ref_advance(search, 'M');
    ASSERT_EQ(ret, true);

    ret = pchvml_char_ref_advance(search, 'P');
    ASSERT_EQ(ret, true);

    ret = pchvml_char_ref_advance(search, 'n');
    ASSERT_EQ(ret, false);

    struct pcutils_arrlist* ucs = pchvml_char_ref_get_buffered_ucs(search);
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

    pchvml_char_ref_search_destroy(search);
}
