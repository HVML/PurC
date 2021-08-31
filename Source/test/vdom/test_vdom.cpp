#include "purc.h"
#include "private/vdom.h"

#include <gtest/gtest.h>


TEST(vdom, basic)
{
    struct pcvdom_document *doc = pcvdom_document_create("v: MATH FS");
    ASSERT_NE(doc, nullptr);

    struct pcvdom_comment *comment = pcvdom_comment_create("hello world");
    ASSERT_NE(comment, nullptr);
    EXPECT_EQ(0, pcvdom_document_append_comment(doc, comment));

    struct pcvdom_content *content = pcvdom_content_create(NULL);
    ASSERT_NE(content, nullptr);
    EXPECT_EQ(0, pcvdom_document_append_content(doc, content));

    struct pcvdom_element *root = pcvdom_element_create_c("hvml");
    ASSERT_NE(root, nullptr);
    EXPECT_EQ(0, pcvdom_document_set_root(doc, root));

    struct pcvdom_element *elem1 = pcvdom_element_create_c("hvml");
    ASSERT_NE(elem1, nullptr);
    EXPECT_EQ(0, pcvdom_element_append_element(root, elem1));

    struct pcvdom_element *elem2 = pcvdom_element_create_c("hvml");
    ASSERT_NE(elem2, nullptr);
    EXPECT_EQ(0, pcvdom_element_append_element(root, elem2));

    struct pcvdom_element *elem3 = pcvdom_element_create_c("hvml");
    ASSERT_NE(elem3, nullptr);
    EXPECT_EQ(0, pcvdom_element_append_element(root, elem3));

    struct pcvdom_element *elem4 = pcvdom_element_create(PCHVML_TAG_INIT);
    ASSERT_NE(elem4, nullptr);
    EXPECT_EQ(0, pcvdom_element_append_element(root, elem4));

    struct pcvdom_element *elem21 = pcvdom_element_create_c("hvml");
    ASSERT_NE(elem21, nullptr);
    EXPECT_EQ(0, pcvdom_element_append_element(elem2, elem21));

    struct pcvdom_element *elem22 = pcvdom_element_create_c("hvml");
    ASSERT_NE(elem22, nullptr);
    EXPECT_EQ(0, pcvdom_element_append_element(elem2, elem22));

    struct pcvdom_attr *attr31;
    attr31 = pcvdom_attr_create("for", PCVDOM_ATTR_OP_EQ, NULL);
    ASSERT_NE(attr31, nullptr);
    ASSERT_EQ(0, pcvdom_element_append_attr(elem3, attr31));

    struct pcvdom_attr *attr32;
    attr32 = pcvdom_attr_create("on", PCVDOM_ATTR_OP_EQ, NULL);
    ASSERT_NE(attr32, nullptr);
    ASSERT_EQ(0, pcvdom_element_append_attr(elem3, attr32));


    pcvdom_document_destroy(doc);

#if 0
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
#endif // 0
}

