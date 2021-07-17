#include "private/tree.h"
#include "purc-rwstream.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct test_tree_node {
    uint32_t id;
};


pctree_node_t create_tree_node(uint32_t id)
{
    struct test_tree_node* node = (struct test_tree_node*) calloc(sizeof(struct test_tree_node), 1);
    node->id = id;
    return pctree_node_new(node);
}

void destroy_tree_node(void* node)
{
//    struct test_tree_node* tn = (struct test_tree_node*) node;
//    fprintf(stderr, "free tree node : %d\n", tn->id);
    free(node);
}

TEST(tree, append)
{
    pctree_node_t root = create_tree_node(0);
    pctree_node_t node_1 = create_tree_node(1);
    pctree_node_t node_2 = create_tree_node(2);
    pctree_node_t node_3 = create_tree_node(3);

    pctree_node_append_child(root, node_1);
    ASSERT_EQ(root->nr_children, 1);
    ASSERT_EQ(root->first_child, node_1);
    ASSERT_EQ(root->last_child, node_1);
    ASSERT_EQ(pctree_node_parent(node_1), root);
    ASSERT_EQ(pctree_node_child(root), node_1);
    ASSERT_EQ(pctree_node_last_child(root), node_1);
    ASSERT_EQ(pctree_node_children_number(root), 1);

    pctree_node_append_child(root, node_2);
    ASSERT_EQ(root->nr_children, 2);
    ASSERT_EQ(root->first_child, node_1);
    ASSERT_NE(root->last_child, node_1);
    ASSERT_EQ(root->last_child, node_2);
    ASSERT_EQ(pctree_node_parent(node_1), root);
    ASSERT_EQ(pctree_node_parent(node_2), root);
    ASSERT_EQ(pctree_node_child(root), node_1);
    ASSERT_EQ(pctree_node_last_child(root), node_2);
    ASSERT_EQ(pctree_node_children_number(root), 2);

    pctree_node_append_child(root, node_3);
    ASSERT_EQ(root->nr_children, 3);
    ASSERT_EQ(root->first_child, node_1);
    ASSERT_NE(root->last_child, node_1);
    ASSERT_NE(root->last_child, node_2);
    ASSERT_EQ(root->last_child, node_3);
    ASSERT_EQ(pctree_node_parent(node_1), root);
    ASSERT_EQ(pctree_node_parent(node_2), root);
    ASSERT_EQ(pctree_node_parent(node_3), root);
    ASSERT_EQ(pctree_node_child(root), node_1);
    ASSERT_EQ(pctree_node_last_child(root), node_3);
    ASSERT_EQ(pctree_node_children_number(root), 3);

    pctree_node_destroy(root, destroy_tree_node);
}

TEST(tree, prepend)
{
    pctree_node_t root = create_tree_node(0);
    pctree_node_t node_1 = create_tree_node(1);
    pctree_node_t node_2 = create_tree_node(2);
    pctree_node_t node_3 = create_tree_node(3);

    pctree_node_append_child(root, node_1);
    ASSERT_EQ(root->nr_children, 1);
    ASSERT_EQ(root->first_child, node_1);
    ASSERT_EQ(root->last_child, node_1);
    ASSERT_EQ(pctree_node_next(node_1), nullptr);

    pctree_node_prepend_child(root, node_2);
    ASSERT_EQ(root->nr_children, 2);
    ASSERT_EQ(root->first_child, node_2);
    ASSERT_EQ(root->last_child, node_1);
    ASSERT_EQ(pctree_node_next(node_2), node_1);
    ASSERT_EQ(pctree_node_prev(node_1), node_2);

    pctree_node_prepend_child(root, node_3);
    ASSERT_EQ(root->nr_children, 3);
    ASSERT_EQ(root->first_child, node_3);
    ASSERT_NE(root->first_child, node_2);
    ASSERT_EQ(root->last_child, node_1);
    ASSERT_EQ(pctree_node_next(node_3), node_2);
    ASSERT_EQ(pctree_node_prev(node_1), node_2);

    pctree_node_destroy(root, destroy_tree_node);
}

TEST(tree, insert_before)
{
    pctree_node_t root = create_tree_node(0);
    pctree_node_t node_1 = create_tree_node(1);
    pctree_node_t node_2 = create_tree_node(2);
    pctree_node_t node_3 = create_tree_node(3);

    pctree_node_append_child(root, node_1);
    ASSERT_EQ(root->nr_children, 1);
    ASSERT_EQ(root->first_child, node_1);
    ASSERT_EQ(root->last_child, node_1);

    pctree_node_insert_before(node_1,node_2);
    ASSERT_EQ(root->nr_children, 2);
    ASSERT_EQ(root->first_child, node_2);
    ASSERT_EQ(root->last_child, node_1);

    pctree_node_insert_before(node_1,node_3);
    ASSERT_EQ(root->nr_children, 3);
    ASSERT_EQ(root->first_child, node_2);
    ASSERT_NE(root->first_child, node_3);
    ASSERT_EQ(root->last_child, node_1);

    pctree_node_destroy(root, destroy_tree_node);
}

TEST(tree, insert_after)
{
    pctree_node_t root = create_tree_node(0);
    pctree_node_t node_1 = create_tree_node(1);
    pctree_node_t node_2 = create_tree_node(2);
    pctree_node_t node_3 = create_tree_node(3);

    pctree_node_append_child(root, node_1);
    ASSERT_EQ(root->nr_children, 1);
    ASSERT_EQ(root->first_child, node_1);
    ASSERT_EQ(root->last_child, node_1);

    pctree_node_insert_after(node_1,node_2);
    ASSERT_EQ(root->nr_children, 2);
    ASSERT_EQ(root->first_child, node_1);
    ASSERT_EQ(root->last_child, node_2);

    pctree_node_insert_after(node_1,node_3);
    ASSERT_EQ(root->nr_children, 3);
    ASSERT_EQ(root->first_child, node_1);
    ASSERT_NE(root->first_child, node_2);
    ASSERT_EQ(root->last_child, node_2);

    pctree_node_destroy(root, destroy_tree_node);
}

TEST(tree, insert)
{
    pctree_node_t root = create_tree_node(0);
    pctree_node_t node_1 = create_tree_node(1);
    pctree_node_t node_2 = create_tree_node(2);
    pctree_node_t node_3 = create_tree_node(3);

    pctree_node_append_child(root, node_1);
    ASSERT_EQ(root->nr_children, 1);
    ASSERT_EQ(root->first_child, node_1);
    ASSERT_EQ(root->last_child, node_1);

    pctree_node_insert_after(node_1,node_2);
    ASSERT_EQ(root->nr_children, 2);
    ASSERT_EQ(root->first_child, node_1);
    ASSERT_EQ(root->last_child, node_2);

    pctree_node_insert_before(node_1,node_3);
    ASSERT_EQ(root->nr_children, 3);
    ASSERT_EQ(root->first_child, node_3);
    ASSERT_NE(root->first_child, node_2);
    ASSERT_EQ(root->last_child, node_2);

    pctree_node_destroy(root, destroy_tree_node);
}

TEST(tree, build_tree)
{
    pctree_node_t root = create_tree_node(0);
    pctree_node_t node_1 = create_tree_node(1);
    pctree_node_t node_2 = create_tree_node(2);
    pctree_node_t node_3 = create_tree_node(3);
    pctree_node_t node_4 = create_tree_node(4);

    pctree_node_prepend_child(root, node_1);
    ASSERT_EQ(root->nr_children, 1);
    ASSERT_EQ((node_1)->nr_children, 0);
    ASSERT_EQ(root->first_child, node_1);
    ASSERT_EQ(root->first_child, node_1);

    pctree_node_append_child(root, node_2);
    ASSERT_EQ(root->nr_children, 2);
    ASSERT_EQ(root->first_child, node_1);
    ASSERT_NE(root->last_child, node_1);
    ASSERT_EQ(root->last_child, node_2);

    pctree_node_insert_before(node_1,node_3);
    ASSERT_EQ(root->nr_children, 3);
    ASSERT_EQ(root->first_child, node_3);
    ASSERT_EQ(root->last_child, node_2);

    pctree_node_insert_after(node_3,node_4);
    ASSERT_EQ(root->nr_children, 4);
    ASSERT_EQ(root->first_child, node_3);
    ASSERT_EQ(root->last_child, node_2);

    pctree_node_destroy(root, destroy_tree_node);
}


void  build_output_buf(pctree_node_t node,  void* data)
{
    char buf[1000] = {0};
    purc_rwstream_t rws = (purc_rwstream_t) data;
    struct test_tree_node* tree_node = (struct test_tree_node*)node->user_data;
    snprintf(buf, 1000, "%d ", tree_node->id);
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
    pctree_node_t node_1 = create_tree_node(1);
    pctree_node_t node_2 = create_tree_node(2);
    pctree_node_t node_3 = create_tree_node(3);
    pctree_node_t node_4 = create_tree_node(4);
    pctree_node_t node_5 = create_tree_node(5);
    pctree_node_t node_6 = create_tree_node(6);
    pctree_node_t node_7 = create_tree_node(7);
    pctree_node_t node_8 = create_tree_node(8);
    pctree_node_t node_9 = create_tree_node(9);
    pctree_node_t node_10 = create_tree_node(10);
    pctree_node_t node_11 = create_tree_node(11);

    pctree_node_append_child(node_1, node_2);
    ASSERT_EQ(pctree_node_children_number(node_1), 1);
    pctree_node_append_child(node_1, node_3);
    ASSERT_EQ(pctree_node_children_number(node_1), 2);

    pctree_node_append_child(node_2, node_4);
    pctree_node_append_child(node_2, node_5);
    pctree_node_append_child(node_2, node_6);
    ASSERT_EQ(pctree_node_children_number(node_2), 3);

    pctree_node_append_child(node_3, node_7);
    ASSERT_EQ(pctree_node_children_number(node_3), 1);

    pctree_node_append_child(node_7, node_8);
    pctree_node_append_child(node_7, node_9);
    pctree_node_append_child(node_7, node_10);
    pctree_node_append_child(node_7, node_11);
    ASSERT_EQ(pctree_node_children_number(node_7), 4);


    char buf[1024] = {0};
    size_t buf_len = 1023;
    purc_rwstream_t rws = purc_rwstream_new_from_mem (buf, buf_len);
    ASSERT_NE(rws, nullptr);

    pctree_node_pre_order_traversal(node_1, build_output_buf, rws);
//    fprintf(stderr, "pre order = %s\n", buf);
    ASSERT_STREQ(buf, "1 2 4 5 6 3 7 8 9 10 11 ");

    purc_rwstream_seek (rws, 0, SEEK_SET);
    pctree_node_in_order_traversal(node_1, build_output_buf, rws);
//    fprintf(stderr, "in order = %s\n", buf);
    ASSERT_STREQ(buf, "4 2 5 6 1 8 7 9 10 11 3 ");

    purc_rwstream_seek (rws, 0, SEEK_SET);
    pctree_node_post_order_traversal(node_1, build_output_buf, rws);
//    fprintf(stderr, "post order = %s\n", buf);
    ASSERT_STREQ(buf, "4 5 6 2 8 9 10 11 7 3 1 ");

    purc_rwstream_seek (rws, 0, SEEK_SET);
    pctree_node_level_order_traversal(node_1, build_output_buf, rws);
//    fprintf(stderr, "level order = %s\n", buf);
    ASSERT_STREQ(buf, "1 2 3 4 5 6 7 8 9 10 11 ");

    int ret = purc_rwstream_close(rws);
    ASSERT_EQ(ret, 0);

    ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);

    pctree_node_destroy(node_1, destroy_tree_node);
}
