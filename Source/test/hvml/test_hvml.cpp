#include "purc.h"
#include "private/hvml.h"
#include "private/vdom.h"

#include <gtest/gtest.h>


TEST(hvml, basic)
{
    pchvml_document_t *doc = pchvml_document_create();
    ASSERT_NE(doc, nullptr);
    pchvml_document_doctype_t *doctype = pchvml_document_doctype_create();
    ASSERT_NE(doctype, nullptr);
    pchvml_dom_element_t *root = pchvml_dom_element_create();
    ASSERT_NE(root, nullptr);
    pchvml_document_set_doctype(doc, doctype);
    pchvml_document_set_root(doc, root);
    pchvml_document_destroy(doc);
}

