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
    pchvml_document_set_doctype(doc, doctype);

    int r = pchvml_document_doctype_set_prefix(doctype, "hvml");
    ASSERT_EQ(r, 0);
    r = pchvml_document_doctype_append_builtin(doctype, "MATH");
    ASSERT_EQ(r, 0);
    r = pchvml_document_doctype_append_builtin(doctype, "FS");
    ASSERT_EQ(r, 0);
    r = pchvml_document_doctype_append_builtin(doctype, "FILE");
    ASSERT_EQ(r, 0);

    pchvml_dom_element_t *root = pchvml_dom_element_create();
    ASSERT_NE(root, nullptr);
    pchvml_document_set_root(doc, root);

    pchvml_dom_element_tag_t *tag = pchvml_dom_element_tag_create();
    ASSERT_NE(tag, nullptr);
    pchvml_dom_element_set_tag(root, tag);

    pchvml_dom_element_attr_t *attr = pchvml_dom_element_attr_create();
    ASSERT_NE(attr, nullptr);
    pchvml_dom_element_tag_append_attr(tag, attr);

    pchvml_dom_element_t *head = pchvml_dom_element_create();
    ASSERT_NE(head, nullptr);
    pchvml_dom_element_append_child(root, head);

    pchvml_document_destroy(doc);
}

