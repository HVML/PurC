#include "purc.h"
#include "private/hvml.h"
#include "private/vdom.h"

#include <gtest/gtest.h>


TEST(hvml, basic)
{
    pcvdom_document_t *doc = pcvdom_document_create();
    ASSERT_NE(doc, nullptr);

    pcvdom_doctype_t *doctype = pcvdom_doctype_create();
    ASSERT_NE(doctype, nullptr);
    pcvdom_document_set_doctype(doc, doctype);

    int r = pcvdom_doctype_set_prefix(doctype, "hvml");
    ASSERT_EQ(r, 0);
    r = pcvdom_doctype_append_builtin(doctype, "MATH");
    ASSERT_EQ(r, 0);
    r = pcvdom_doctype_append_builtin(doctype, "FS");
    ASSERT_EQ(r, 0);
    r = pcvdom_doctype_append_builtin(doctype, "FILE");
    ASSERT_EQ(r, 0);

    pcvdom_element_t *root = pcvdom_element_create();
    ASSERT_NE(root, nullptr);
    pcvdom_document_set_root(doc, root);

    pcvdom_tag_t *tag = pcvdom_tag_create();
    ASSERT_NE(tag, nullptr);
    pcvdom_element_set_tag(root, tag);

    r = pcvdom_tag_set_ns(tag, "hvml");
    ASSERT_EQ(r, 0);
    r = pcvdom_tag_set_name(tag, "init");
    ASSERT_EQ(r, 0);

    pcvdom_attr_t *attr = pcvdom_attr_create();
    ASSERT_NE(attr, nullptr);
    pcvdom_tag_append_attr(tag, attr);

    pcvdom_element_t *head = pcvdom_element_create();
    ASSERT_NE(head, nullptr);
    pcvdom_element_append_child(root, head);

    pcvdom_document_destroy(doc);
}

