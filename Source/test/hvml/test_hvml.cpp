#include "purc.h"
#include "private/hvml.h"
#include "private/vdom.h"

#include <gtest/gtest.h>


TEST(hvml, basic)
{
    pchvml_document_t *doc = pchvml_document_create();
    ASSERT_NE(doc, nullptr);
    pchvml_document_destroy(doc);
}

