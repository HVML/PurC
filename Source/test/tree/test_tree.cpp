#include "private/tree.h"
#include "purc-rwstream.h"

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

TEST(tree, insert_after)
{
    struct test_tree_node* root = create_tree_node(0);
    struct test_tree_node* node_1 = create_tree_node(1);
    struct test_tree_node* node_2 = create_tree_node(2);
    struct test_tree_node* node_3 = create_tree_node(3);

    pctree_node_append_child((pctree_node_t)root, (pctree_node_t)node_1);
    ASSERT_EQ(((pctree_node_t)root)->nr_children, 1);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_1);
    ASSERT_EQ(((pctree_node_t)root)->last_child, (pctree_node_t)node_1);

    pctree_node_insert_after((pctree_node_t)node_1,(pctree_node_t)node_2);
    ASSERT_EQ(((pctree_node_t)root)->nr_children, 2);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_1);
    ASSERT_EQ(((pctree_node_t)root)->last_child, (pctree_node_t)node_2);

    pctree_node_insert_after((pctree_node_t)node_1,(pctree_node_t)node_3);
    ASSERT_EQ(((pctree_node_t)root)->nr_children, 3);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_1);
    ASSERT_NE(((pctree_node_t)root)->first_child, (pctree_node_t)node_2);
    ASSERT_EQ(((pctree_node_t)root)->last_child, (pctree_node_t)node_2);

    destroy_tree_node(root);
    destroy_tree_node(node_1);
    destroy_tree_node(node_2);
    destroy_tree_node(node_3);
}

TEST(tree, insert)
{
    struct test_tree_node* root = create_tree_node(0);
    struct test_tree_node* node_1 = create_tree_node(1);
    struct test_tree_node* node_2 = create_tree_node(2);
    struct test_tree_node* node_3 = create_tree_node(3);

    pctree_node_append_child((pctree_node_t)root, (pctree_node_t)node_1);
    ASSERT_EQ(((pctree_node_t)root)->nr_children, 1);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_1);
    ASSERT_EQ(((pctree_node_t)root)->last_child, (pctree_node_t)node_1);

    pctree_node_insert_after((pctree_node_t)node_1,(pctree_node_t)node_2);
    ASSERT_EQ(((pctree_node_t)root)->nr_children, 2);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_1);
    ASSERT_EQ(((pctree_node_t)root)->last_child, (pctree_node_t)node_2);

    pctree_node_insert_before((pctree_node_t)node_1,(pctree_node_t)node_3);
    ASSERT_EQ(((pctree_node_t)root)->nr_children, 3);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_3);
    ASSERT_NE(((pctree_node_t)root)->first_child, (pctree_node_t)node_2);
    ASSERT_EQ(((pctree_node_t)root)->last_child, (pctree_node_t)node_2);

    destroy_tree_node(root);
    destroy_tree_node(node_1);
    destroy_tree_node(node_2);
    destroy_tree_node(node_3);
}

TEST(tree, build_tree)
{
    struct test_tree_node* root = create_tree_node(0);
    struct test_tree_node* node_1 = create_tree_node(1);
    struct test_tree_node* node_2 = create_tree_node(2);
    struct test_tree_node* node_3 = create_tree_node(3);
    struct test_tree_node* node_4 = create_tree_node(4);

    pctree_node_prepend_child((pctree_node_t)root, (pctree_node_t)node_1);
    ASSERT_EQ(((pctree_node_t)root)->nr_children, 1);
    ASSERT_EQ(((pctree_node_t)node_1)->nr_children, 0);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_1);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_1);

    pctree_node_append_child((pctree_node_t)root, (pctree_node_t)node_2);
    ASSERT_EQ(((pctree_node_t)root)->nr_children, 2);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_1);
    ASSERT_NE(((pctree_node_t)root)->last_child, (pctree_node_t)node_1);
    ASSERT_EQ(((pctree_node_t)root)->last_child, (pctree_node_t)node_2);

    pctree_node_insert_before((pctree_node_t)node_1,(pctree_node_t)node_3);
    ASSERT_EQ(((pctree_node_t)root)->nr_children, 3);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_3);
    ASSERT_EQ(((pctree_node_t)root)->last_child, (pctree_node_t)node_2);

    pctree_node_insert_after((pctree_node_t)node_3,(pctree_node_t)node_4);
    ASSERT_EQ(((pctree_node_t)root)->nr_children, 4);
    ASSERT_EQ(((pctree_node_t)root)->first_child, (pctree_node_t)node_3);
    ASSERT_EQ(((pctree_node_t)root)->last_child, (pctree_node_t)node_2);

    destroy_tree_node(root);
    destroy_tree_node(node_1);
    destroy_tree_node(node_2);
    destroy_tree_node(node_3);
    destroy_tree_node(node_4);
}


void  build_output_buf(pctree_node_t node,  void* data)
{
    purc_rwstream_t rws = (purc_rwstream_t) data;
    char buf[1000] = {0};
    snprintf(buf, 1000, "%d ", ((struct test_tree_node*)node)->id);
    purc_rwstream_write (rws, buf, strlen(buf));
}

/*               1
 *             /   \
 *           2       3
 *         / | \       \
 *       4   5   6       7
 *                     / /\ \
 *                   8  9  10  11
 *
 */
TEST(tree, traversal)
{
    struct test_tree_node* node_1 = create_tree_node(1);
    struct test_tree_node* node_2 = create_tree_node(2);
    struct test_tree_node* node_3 = create_tree_node(3);
    struct test_tree_node* node_4 = create_tree_node(4);
    struct test_tree_node* node_5 = create_tree_node(5);
    struct test_tree_node* node_6 = create_tree_node(6);
    struct test_tree_node* node_7 = create_tree_node(7);
    struct test_tree_node* node_8 = create_tree_node(8);
    struct test_tree_node* node_9 = create_tree_node(9);
    struct test_tree_node* node_10 = create_tree_node(10);
    struct test_tree_node* node_11 = create_tree_node(11);

    pctree_node_append_child((pctree_node_t)node_1, (pctree_node_t)node_2);
    pctree_node_append_child((pctree_node_t)node_1, (pctree_node_t)node_3);

    pctree_node_append_child((pctree_node_t)node_2, (pctree_node_t)node_4);
    pctree_node_append_child((pctree_node_t)node_2, (pctree_node_t)node_5);
    pctree_node_append_child((pctree_node_t)node_2, (pctree_node_t)node_6);

    pctree_node_append_child((pctree_node_t)node_3, (pctree_node_t)node_7);

    pctree_node_append_child((pctree_node_t)node_7, (pctree_node_t)node_8);
    pctree_node_append_child((pctree_node_t)node_7, (pctree_node_t)node_9);
    pctree_node_append_child((pctree_node_t)node_7, (pctree_node_t)node_10);
    pctree_node_append_child((pctree_node_t)node_7, (pctree_node_t)node_11);


    char buf[1024] = {0};
    size_t buf_len = 1023;
    purc_rwstream_t rws = purc_rwstream_new_from_mem (buf, buf_len);
    ASSERT_NE(rws, nullptr);

    pctree_node_pre_order_traversal((pctree_node_t)node_1, build_output_buf, rws);
//    fprintf(stderr, "pre order = %s\n", buf);
    ASSERT_STREQ(buf, "1 2 4 5 6 3 7 8 9 10 11 ");

    purc_rwstream_seek (rws, 0, SEEK_SET);
    pctree_node_in_order_traversal((pctree_node_t)node_1, build_output_buf, rws);
//    fprintf(stderr, "in order = %s\n", buf);
    ASSERT_STREQ(buf, "4 2 5 6 1 8 7 9 10 11 3 ");

    purc_rwstream_seek (rws, 0, SEEK_SET);
    pctree_node_post_order_traversal((pctree_node_t)node_1, build_output_buf, rws);
//    fprintf(stderr, "post order = %s\n", buf);
    ASSERT_STREQ(buf, "4 5 6 2 8 9 10 11 7 3 1 ");

    purc_rwstream_seek (rws, 0, SEEK_SET);
    pctree_node_level_order_traversal((pctree_node_t)node_1, build_output_buf, rws);
//    fprintf(stderr, "level order = %s\n", buf);
    ASSERT_STREQ(buf, "1 2 3 4 5 6 7 8 9 10 11 ");

    int ret = purc_rwstream_close(rws);
    ASSERT_EQ(ret, 0);

    ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);

    destroy_tree_node(node_1);
    destroy_tree_node(node_2);
    destroy_tree_node(node_3);
    destroy_tree_node(node_4);
    destroy_tree_node(node_5);
    destroy_tree_node(node_6);
    destroy_tree_node(node_7);
    destroy_tree_node(node_8);
    destroy_tree_node(node_9);
    destroy_tree_node(node_10);
    destroy_tree_node(node_11);
}
