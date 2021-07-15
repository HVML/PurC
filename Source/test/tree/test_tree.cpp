#include "private/tree.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct test_tree_node {
    struct pctree_node base;
    uint32_t id;
};


struct test_tree_node* create_tree_node(uint32_t id)
{
    struct test_tree_node* node = (struct test_tree_node*) calloc(sizeof(struct test_tree_node), 1);
    node->id = id;
    return node;
}

void destroy_tree_node(struct test_tree_node* node)
{
    free(node);
}

TEST(tree, append)
{
    struct test_tree_node* root = create_tree_node(0);
    struct test_tree_node* node_1 = create_tree_node(1);
    struct test_tree_node* node_2 = create_tree_node(2);
    struct test_tree_node* node_3 = create_tree_node(2);

    pctree_node_append_child((pctree_node_t)root, (pctree_node_t)node_1);
    ASSERT_EQ(((pctree_node_t)root)->nr_children, 1);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_1);
    ASSERT_EQ(((pctree_node_t)root)->last_child, (pctree_node_t)node_1);

    pctree_node_append_child((pctree_node_t)root, (pctree_node_t)node_2);
    ASSERT_EQ(((pctree_node_t)root)->nr_children, 2);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_1);
    ASSERT_NE(((pctree_node_t)root)->last_child, (pctree_node_t)node_1);
    ASSERT_EQ(((pctree_node_t)root)->last_child, (pctree_node_t)node_2);

    pctree_node_append_child((pctree_node_t)root, (pctree_node_t)node_3);
    ASSERT_EQ(((pctree_node_t)root)->nr_children, 3);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_1);
    ASSERT_NE(((pctree_node_t)root)->last_child, (pctree_node_t)node_1);
    ASSERT_NE(((pctree_node_t)root)->last_child, (pctree_node_t)node_2);
    ASSERT_EQ(((pctree_node_t)root)->last_child, (pctree_node_t)node_3);

    destroy_tree_node(root);
    destroy_tree_node(node_1);
    destroy_tree_node(node_2);
    destroy_tree_node(node_3);
}

TEST(tree, prepend)
{
    struct test_tree_node* root = create_tree_node(0);
    struct test_tree_node* node_1 = create_tree_node(1);
    struct test_tree_node* node_2 = create_tree_node(2);
    struct test_tree_node* node_3 = create_tree_node(3);

    pctree_node_append_child((pctree_node_t)root, (pctree_node_t)node_1);
    ASSERT_EQ(((pctree_node_t)root)->nr_children, 1);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_1);
    ASSERT_EQ(((pctree_node_t)root)->last_child, (pctree_node_t)node_1);

    pctree_node_prepend_child((pctree_node_t)root, (pctree_node_t)node_2);
    ASSERT_EQ(((pctree_node_t)root)->nr_children, 2);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_2);
    ASSERT_EQ(((pctree_node_t)root)->last_child, (pctree_node_t)node_1);

    pctree_node_prepend_child((pctree_node_t)root, (pctree_node_t)node_3);
    ASSERT_EQ(((pctree_node_t)root)->nr_children, 3);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_3);
    ASSERT_NE(((pctree_node_t)root)->first_child, (pctree_node_t)node_2);
    ASSERT_EQ(((pctree_node_t)root)->last_child, (pctree_node_t)node_1);

    destroy_tree_node(root);
    destroy_tree_node(node_1);
    destroy_tree_node(node_2);
    destroy_tree_node(node_3);
}

TEST(tree, insert_before)
{
    struct test_tree_node* root = create_tree_node(0);
    struct test_tree_node* node_1 = create_tree_node(1);
    struct test_tree_node* node_2 = create_tree_node(2);
    struct test_tree_node* node_3 = create_tree_node(3);

    pctree_node_append_child((pctree_node_t)root, (pctree_node_t)node_1);
    ASSERT_EQ(((pctree_node_t)root)->nr_children, 1);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_1);
    ASSERT_EQ(((pctree_node_t)root)->last_child, (pctree_node_t)node_1);

    pctree_node_insert_before((pctree_node_t)node_1,(pctree_node_t)node_2);
    ASSERT_EQ(((pctree_node_t)root)->nr_children, 2);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_2);
    ASSERT_EQ(((pctree_node_t)root)->last_child, (pctree_node_t)node_1);

    pctree_node_insert_before((pctree_node_t)node_1,(pctree_node_t)node_3);
    ASSERT_EQ(((pctree_node_t)root)->nr_children, 3);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_2);
    ASSERT_NE(((pctree_node_t)root)->first_child, (pctree_node_t)node_3);
    ASSERT_EQ(((pctree_node_t)root)->last_child, (pctree_node_t)node_1);

    destroy_tree_node(root);
    destroy_tree_node(node_1);
    destroy_tree_node(node_2);
    destroy_tree_node(node_3);
}
