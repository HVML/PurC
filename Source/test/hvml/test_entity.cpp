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
