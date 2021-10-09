#include "purc.h"
#include "private/vdom.h"

#include <gtest/gtest.h>

int _element_count(struct pcvdom_element *top,
    struct pcvdom_element *elem, void *ctx)
{
    UNUSED_PARAM(top);
    UNUSED_PARAM(elem);

    int *elems = (int*)ctx;
    *elems += 1;
    return 0;
}

int _node_count(struct pcvdom_node *top,
    struct pcvdom_node *node, void *ctx)
{
    UNUSED_PARAM(top);
    UNUSED_PARAM(node);

    int *nodes = (int*)ctx;
    *nodes += 1;
    return 0;
}

TEST(vdom, basic)
{
    struct pcvdom_document *doc;
    doc = pcvdom_document_create_with_doctype("hvml", "v: MATH FS");
    ASSERT_NE(doc, nullptr);

    struct pcvdom_comment *comment = pcvdom_comment_create("hello world");
    ASSERT_NE(comment, nullptr);
    EXPECT_EQ(0, pcvdom_document_append_comment(doc, comment));

    struct pcvdom_content *content = pcvdom_content_create("foo bar");
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
    EXPECT_EQ(pcvdom_element_parent(elem22), elem2);

    struct pcvdom_attr *attr31;
    attr31 = pcvdom_attr_create("for", PCVDOM_ATTR_OP_EQ, NULL);
    ASSERT_NE(attr31, nullptr);
    ASSERT_EQ(0, pcvdom_element_append_attr(elem3, attr31));

    struct pcvdom_attr *attr32;
    attr32 = pcvdom_attr_create("on", PCVDOM_ATTR_OP_EQ, NULL);
    ASSERT_NE(attr32, nullptr);
    ASSERT_EQ(0, pcvdom_element_append_attr(elem3, attr32));

    struct pcvdom_comment *comment41 = pcvdom_comment_create("hello world");
    ASSERT_NE(comment41, nullptr);
    EXPECT_EQ(0, pcvdom_element_append_comment(elem4, comment41));
    EXPECT_EQ(pcvdom_comment_parent(comment41), elem4);

    const char *text411 = "hello world";

    struct pcvdom_content *content41 = pcvdom_content_create(text411);
    ASSERT_NE(content41, nullptr);
    EXPECT_EQ(0, pcvdom_element_append_content(elem4, content41));
    EXPECT_EQ(pcvdom_content_parent(content41), elem4);

    int elems = 0;
    EXPECT_EQ(0, pcvdom_element_traverse(doc->root, &elems, _element_count));
    EXPECT_EQ(elems, 7);

    int nodes = 0;
    EXPECT_EQ(0, pcvdom_node_traverse(&doc->node,
        &nodes, _node_count));
    EXPECT_EQ(nodes, 12);

    pcvdom_document_destroy(doc);
}

