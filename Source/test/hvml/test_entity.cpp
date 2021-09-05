#include "purc.h"

#include "tempbuffer.h"
#include "hvml-character-reference.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

TEST(hvml_character_reference, new_destory)
{
    struct pchvml_entity_search* search = pchvml_entity_search_new(NULL, NULL);
    ASSERT_NE(search, nullptr);
    pchvml_entity_search_destroy(search);
}

TEST(hvml_character_reference, init_search)
{
    struct pchvml_entity_search* search = pchvml_entity_search_new_ex(
            pchvml_character_reference_first(),
            pchvml_character_reference_last(),
            pchvml_character_reference_first_starting_with,
            pchvml_character_reference_last_starting_with);
    ASSERT_NE(search, nullptr);
    pchvml_entity_search_destroy(search);
}

TEST(hvml_character_reference, match)
{
    struct pchvml_entity_search* search = pchvml_entity_search_new_ex(
            pchvml_character_reference_first(),
            pchvml_character_reference_last(),
            pchvml_character_reference_first_starting_with,
            pchvml_character_reference_last_starting_with);
    ASSERT_NE(search, nullptr);

    bool ret = false;
    const struct pchvml_entity* entity = NULL;

    ret = pchvml_entity_advance(search, 'A');
    ASSERT_EQ(ret, true);
    entity = pchvml_entity_search_most_recent_match(search);
    ASSERT_EQ(entity, nullptr);

    ret = pchvml_entity_advance(search, 'M');
    ASSERT_EQ(ret, true);
    entity = pchvml_entity_search_most_recent_match(search);
    ASSERT_EQ(entity, nullptr);

    ret = pchvml_entity_advance(search, 'P');
    ASSERT_EQ(ret, true);
    entity = pchvml_entity_search_most_recent_match(search);
    ASSERT_NE(entity, nullptr);

    ret = pchvml_entity_advance(search, ';');
    ASSERT_EQ(ret, true);
    entity = pchvml_entity_search_most_recent_match(search);
    ASSERT_NE(entity, nullptr);

    pchvml_entity_search_destroy(search);
}

TEST(hvml_character_reference, unmatch)
{
    struct pchvml_entity_search* search = pchvml_entity_search_new_ex(
            pchvml_character_reference_first(),
            pchvml_character_reference_last(),
            pchvml_character_reference_first_starting_with,
            pchvml_character_reference_last_starting_with);
    ASSERT_NE(search, nullptr);

    bool ret = false;
    const struct pchvml_entity* entity = NULL;

    ret = pchvml_entity_advance(search, 'A');
    ASSERT_EQ(ret, true);
    entity = pchvml_entity_search_most_recent_match(search);
    ASSERT_EQ(entity, nullptr);

    ret = pchvml_entity_advance(search, 'M');
    ASSERT_EQ(ret, true);
    entity = pchvml_entity_search_most_recent_match(search);
    ASSERT_EQ(entity, nullptr);

    ret = pchvml_entity_advance(search, 'P');
    ASSERT_EQ(ret, true);
    entity = pchvml_entity_search_most_recent_match(search);
    ASSERT_NE(entity, nullptr);

    ret = pchvml_entity_advance(search, 'x');
    ASSERT_EQ(ret, false);
    entity = pchvml_entity_search_most_recent_match(search);
    ASSERT_NE(entity, nullptr);

    struct pcutils_arrlist* ucs = pchvml_entity_get_buffered_usc(search);
    wchar_t uc = (wchar_t)(uintptr_t)pcutils_arrlist_get_idx (ucs, 0);
    ASSERT_EQ(uc, 'A');
    uc = (wchar_t)(uintptr_t)pcutils_arrlist_get_idx (ucs, 1);
    ASSERT_EQ(uc, 'M');
    uc = (wchar_t)(uintptr_t)pcutils_arrlist_get_idx (ucs, 2);
    ASSERT_EQ(uc, 'P');
    uc = (wchar_t)(uintptr_t)pcutils_arrlist_get_idx (ucs, 3);
    ASSERT_EQ(uc, 'x');

    pchvml_entity_search_destroy(search);
}

TEST(hvml_character_reference, unmatch_1)
{
    struct pchvml_entity_search* search = pchvml_entity_search_new_ex(
            pchvml_character_reference_first(),
            pchvml_character_reference_last(),
            pchvml_character_reference_first_starting_with,
            pchvml_character_reference_last_starting_with);
    ASSERT_NE(search, nullptr);

    bool ret = false;
    const struct pchvml_entity* entity = NULL;

    ret = pchvml_entity_advance(search, '1');
    ASSERT_EQ(ret, false);
    entity = pchvml_entity_search_most_recent_match(search);
    ASSERT_EQ(entity, nullptr);

    pchvml_entity_search_destroy(search);
}
