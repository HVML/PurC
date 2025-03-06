/*
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "private/tree.h"
#include "private/list.h" /* for container_of */
#include "purc/purc-rwstream.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct test_tree_node {
    size_t id;
};


struct pctree_node* create_tree_node(uint32_t id)
{
    struct test_tree_node* node = (struct test_tree_node*) calloc(1, sizeof(struct test_tree_node));
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
    struct pctree_node* root = create_tree_node(0);
    struct pctree_node* node_1 = create_tree_node(1);
    struct pctree_node* node_2 = create_tree_node(2);
    struct pctree_node* node_3 = create_tree_node(3);

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
    struct pctree_node* root = create_tree_node(0);
    struct pctree_node* node_1 = create_tree_node(1);
    struct pctree_node* node_2 = create_tree_node(2);
    struct pctree_node* node_3 = create_tree_node(3);

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
    struct pctree_node* root = create_tree_node(0);
    struct pctree_node* node_1 = create_tree_node(1);
    struct pctree_node* node_2 = create_tree_node(2);
    struct pctree_node* node_3 = create_tree_node(3);

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
    struct pctree_node* root = create_tree_node(0);
    struct pctree_node* node_1 = create_tree_node(1);
    struct pctree_node* node_2 = create_tree_node(2);
    struct pctree_node* node_3 = create_tree_node(3);

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
    struct pctree_node* root = create_tree_node(0);
    struct pctree_node* node_1 = create_tree_node(1);
    struct pctree_node* node_2 = create_tree_node(2);
    struct pctree_node* node_3 = create_tree_node(3);

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
    struct pctree_node* root = create_tree_node(0);
    struct pctree_node* node_1 = create_tree_node(1);
    struct pctree_node* node_2 = create_tree_node(2);
    struct pctree_node* node_3 = create_tree_node(3);
    struct pctree_node* node_4 = create_tree_node(4);

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


void  build_output_buf(struct pctree_node* node,  void* data)
{
    char buf[1000] = {0};
    purc_rwstream_t rws = (purc_rwstream_t) data;
    struct test_tree_node* tree_node = (struct test_tree_node*)node->user_data;
    snprintf(buf, 1000, "%zd ", tree_node->id);
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
    struct pctree_node* node_1 = create_tree_node(1);
    struct pctree_node* node_2 = create_tree_node(2);
    struct pctree_node* node_3 = create_tree_node(3);
    struct pctree_node* node_4 = create_tree_node(4);
    struct pctree_node* node_5 = create_tree_node(5);
    struct pctree_node* node_6 = create_tree_node(6);
    struct pctree_node* node_7 = create_tree_node(7);
    struct pctree_node* node_8 = create_tree_node(8);
    struct pctree_node* node_9 = create_tree_node(9);
    struct pctree_node* node_10 = create_tree_node(10);
    struct pctree_node* node_11 = create_tree_node(11);

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

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);

    pctree_node_destroy(node_1, destroy_tree_node);
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
TEST(tree, remove)
{
    struct pctree_node* node_1 = create_tree_node(1);
    struct pctree_node* node_2 = create_tree_node(2);
    struct pctree_node* node_3 = create_tree_node(3);
    struct pctree_node* node_4 = create_tree_node(4);
    struct pctree_node* node_5 = create_tree_node(5);
    struct pctree_node* node_6 = create_tree_node(6);
    struct pctree_node* node_7 = create_tree_node(7);
    struct pctree_node* node_8 = create_tree_node(8);
    struct pctree_node* node_9 = create_tree_node(9);
    struct pctree_node* node_10 = create_tree_node(10);
    struct pctree_node* node_11 = create_tree_node(11);

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

    pctree_node_remove(node_7);

    pctree_node_destroy(node_1, destroy_tree_node);
    pctree_node_destroy(node_7, destroy_tree_node);
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
struct number_node {
    struct pctree_node      node;
    size_t                  val;
};

static void
build_output_buf2(struct pctree_node* node,  void* data)
{
    char buf[1000] = {0};
    purc_rwstream_t rws = (purc_rwstream_t) data;
    struct number_node *p = container_of(node, struct number_node, node);
    snprintf(buf, 1000, "%zd ", p->val);
    purc_rwstream_write (rws, buf, strlen(buf));
}

TEST(tree, pod)
{
    struct number_node nodes[11];
    memset(&nodes[0], 0, sizeof(nodes));

    for (size_t i=0; i<PCA_TABLESIZE(nodes); ++i) {
        nodes[i].val = i+1;
    }

    ASSERT_TRUE(pctree_node_append_child(&nodes[0].node, &nodes[1].node));
    ASSERT_EQ(pctree_node_children_number(&nodes[0].node), 1);
    ASSERT_TRUE(pctree_node_append_child(&nodes[0].node, &nodes[2].node));
    ASSERT_EQ(pctree_node_children_number(&nodes[0].node), 2);

    ASSERT_TRUE(pctree_node_append_child(&nodes[1].node, &nodes[3].node));
    ASSERT_TRUE(pctree_node_append_child(&nodes[1].node, &nodes[4].node));
    ASSERT_TRUE(pctree_node_append_child(&nodes[1].node, &nodes[5].node));
    ASSERT_EQ(pctree_node_children_number(&nodes[1].node), 3);

    ASSERT_TRUE(pctree_node_append_child(&nodes[2].node, &nodes[6].node));
    ASSERT_EQ(pctree_node_children_number(&nodes[2].node), 1);

    ASSERT_TRUE(pctree_node_append_child(&nodes[6].node, &nodes[7].node));
    ASSERT_TRUE(pctree_node_append_child(&nodes[6].node, &nodes[8].node));
    ASSERT_TRUE(pctree_node_append_child(&nodes[6].node, &nodes[9].node));
    ASSERT_TRUE(pctree_node_append_child(&nodes[6].node, &nodes[10].node));
    ASSERT_EQ(pctree_node_children_number(&nodes[6].node), 4);

    char buf[1024] = {0};
    size_t buf_len = 1023;
    purc_rwstream_t rws = purc_rwstream_new_from_mem (buf, buf_len);
    ASSERT_NE(rws, nullptr);

    pctree_node_pre_order_traversal(&nodes[0].node, build_output_buf2, rws);
    ASSERT_STREQ(buf, "1 2 4 5 6 3 7 8 9 10 11 ");

    purc_rwstream_seek (rws, 0, SEEK_SET);
    pctree_node_in_order_traversal(&nodes[0].node, build_output_buf2, rws);
    ASSERT_STREQ(buf, "4 2 5 6 1 8 7 9 10 11 3 ");

    purc_rwstream_seek (rws, 0, SEEK_SET);
    pctree_node_post_order_traversal(&nodes[0].node, build_output_buf2, rws);
    ASSERT_STREQ(buf, "4 5 6 2 8 9 10 11 7 3 1 ");

    purc_rwstream_seek (rws, 0, SEEK_SET);
    pctree_node_level_order_traversal(&nodes[0].node, build_output_buf2, rws);
    ASSERT_STREQ(buf, "1 2 3 4 5 6 7 8 9 10 11 ");

    struct pctree_node *node, *next;

    purc_rwstream_seek (rws, 0, SEEK_SET);
    memset(buf, 0, sizeof(buf));
    ASSERT_EQ(nodes[1].node.next, &nodes[2].node);
    struct pctree_node *top = &nodes[0].node;
    pctree_for_each_post_order(top, node, next) {
        char tmp[1000] = {0};
        struct number_node *p = container_of(node, struct number_node, node);
        snprintf(tmp, 1000, "%zd ", p->val);
        purc_rwstream_write (rws, tmp, strlen(tmp));
    }
    ASSERT_STREQ(buf, "4 5 6 2 8 9 10 11 7 3 1 ");

/*               1
 *             /   \
 *           2       3
 *         / | \       \
 *       4   5   6       7
 *                     / /\ \
 *                   8  9  10  11
 *
 */
    const char *post_orders[] = {
        "4 5 6 2 8 9 10 11 7 3 1 ",
        "4 5 6 2 ",
        "8 9 10 11 7 3 ",
        "4 ",
        "5 ",
        "6 ",
        "8 9 10 11 7 ",
        "8 ",
        "9 ",
        "10 ",
        "11 ",
    };
    for (size_t i=0; i<PCA_TABLESIZE(nodes); ++i) {
        for (size_t j=0; j<2; ++j) {
            purc_rwstream_seek (rws, 0, SEEK_SET);
            memset(buf, 0, sizeof(buf));
            struct pctree_node *top = &nodes[i].node;
            pctree_for_each_post_order(top, node, next) {
                char tmp[1000] = {0};
                struct number_node *p = container_of(node, struct number_node, node);
                snprintf(tmp, 1000, "%zd ", p->val);
                purc_rwstream_write (rws, tmp, strlen(tmp));
            }
            EXPECT_STREQ(buf, post_orders[i]);
        }
    }

    const char *pre_orders[] = {
        "1 2 4 5 6 3 7 8 9 10 11 ",
        "2 4 5 6 ",
        "3 7 8 9 10 11 ",
        "4 ",
        "5 ",
        "6 ",
        "7 8 9 10 11 ",
        "8 ",
        "9 ",
        "10 ",
        "11 ",
    };
    for (size_t i=0; i<PCA_TABLESIZE(nodes); ++i) {
        for (size_t j=0; j<2; ++j) {
            purc_rwstream_seek (rws, 0, SEEK_SET);
            memset(buf, 0, sizeof(buf));
            pctree_for_each_pre_order(&nodes[i].node, node) {
                char tmp[1000] = {0};
                struct number_node *p = container_of(node, struct number_node, node);
                snprintf(tmp, 1000, "%zd ", p->val);
                purc_rwstream_write (rws, tmp, strlen(tmp));
            }
            EXPECT_STREQ(buf, pre_orders[i]);
        }
    }

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);
}

static void
_do_pre_order(struct pctree_node *node, purc_rwstream_t rws)
{
    pctree_node_pre_order_traversal(node, build_output_buf2, rws);
}

static void
_do_post_order(struct pctree_node *node, purc_rwstream_t rws)
{
    pctree_node_post_order_traversal(node, build_output_buf2, rws);
}

static void
_do_level_order(struct pctree_node *node, purc_rwstream_t rws)
{
    pctree_node_level_order_traversal(node, build_output_buf2, rws);
}

static void
_do_pre_order_loop(struct pctree_node *node, purc_rwstream_t rws)
{
    struct pctree_node *n;
    pctree_for_each_pre_order(node, n) {
        char tmp[1000] = {0};
        struct number_node *p = container_of(n, struct number_node, node);
        snprintf(tmp, 1000, "%zd ", p->val);
        purc_rwstream_write (rws, tmp, strlen(tmp));
    }
}

static void
_do_post_order_loop(struct pctree_node *node, purc_rwstream_t rws)
{
    struct pctree_node *n, *next;
    pctree_for_each_post_order(node, n, next) {
        char tmp[1000] = {0};
        struct number_node *p = container_of(n, struct number_node, node);
        snprintf(tmp, 1000, "%zd ", p->val);
        purc_rwstream_write (rws, tmp, strlen(tmp));
    }
}

typedef void (*_do_f)(struct pctree_node *node, purc_rwstream_t rws);

TEST(tree, perf)
{
    const char *method = getenv("METHOD");
    const char *loops  = getenv("LOOPS");
    if (!method) {
        fprintf(stderr, "You shall designate `method` by specifying env METHOD\n");
        fprintf(stderr, "METHOD can be pre_order/post_order/level_order/pre_order_loop/post_order_loop\n");
    }

    size_t nr_loops = loops ? atoll(loops) : 0;
    if (nr_loops <= 0) {
        nr_loops = 1;
    }

    const char *post_orders[] = {
        "4 5 6 2 8 9 10 11 7 3 1 ",
        "4 5 6 2 ",
        "8 9 10 11 7 3 ",
        "4 ",
        "5 ",
        "6 ",
        "8 9 10 11 7 ",
        "8 ",
        "9 ",
        "10 ",
        "11 ",
    };
    const char *pre_orders[] = {
        "1 2 4 5 6 3 7 8 9 10 11 ",
        "2 4 5 6 ",
        "3 7 8 9 10 11 ",
        "4 ",
        "5 ",
        "6 ",
        "7 8 9 10 11 ",
        "8 ",
        "9 ",
        "10 ",
        "11 ",
    };
    const char *level_orders[] = {
        "1 2 3 4 5 6 7 8 9 10 11 ",
        "2 4 5 6 ",
        "3 7 8 9 10 11 ",
        "4 ",
        "5 ",
        "6 ",
        "7 8 9 10 11 ",
        "8 ",
        "9 ",
        "10 ",
        "11 ",
    };

    const size_t levels[] = {
        4,
        2, 3,
        1, 1, 1, 2,
        1, 1, 1, 1,
    };

    _do_f method_f = NULL;
    const char **expects = NULL;

    if (method) {
        if (strcasecmp(method, "pre_order")==0) {
            method_f = _do_pre_order;
            expects = pre_orders;
        }
        if (strcasecmp(method, "post_order")==0) {
            method_f = _do_post_order;
            expects = post_orders;
        }
        if (strcasecmp(method, "level_order")==0) {
            method_f = _do_level_order;
            expects = level_orders;
        }
        if (strcasecmp(method, "pre_order_loop")==0) {
            method_f = _do_pre_order_loop;
            expects = pre_orders;
        }
        if (strcasecmp(method, "post_order_loop")==0) {
            method_f = _do_post_order_loop;
            expects = post_orders;
        }
    }

    struct number_node nodes[11];
    memset(&nodes[0], 0, sizeof(nodes));

    for (size_t i=0; i<PCA_TABLESIZE(nodes); ++i) {
        nodes[i].val = i+1;
    }

    ASSERT_TRUE(pctree_node_append_child(&nodes[0].node, &nodes[1].node));
    ASSERT_EQ(pctree_node_children_number(&nodes[0].node), 1);
    ASSERT_TRUE(pctree_node_append_child(&nodes[0].node, &nodes[2].node));
    ASSERT_EQ(pctree_node_children_number(&nodes[0].node), 2);

    ASSERT_TRUE(pctree_node_append_child(&nodes[1].node, &nodes[3].node));
    ASSERT_TRUE(pctree_node_append_child(&nodes[1].node, &nodes[4].node));
    ASSERT_TRUE(pctree_node_append_child(&nodes[1].node, &nodes[5].node));
    ASSERT_EQ(pctree_node_children_number(&nodes[1].node), 3);

    ASSERT_TRUE(pctree_node_append_child(&nodes[2].node, &nodes[6].node));
    ASSERT_EQ(pctree_node_children_number(&nodes[2].node), 1);

    ASSERT_TRUE(pctree_node_append_child(&nodes[6].node, &nodes[7].node));
    ASSERT_TRUE(pctree_node_append_child(&nodes[6].node, &nodes[8].node));
    ASSERT_TRUE(pctree_node_append_child(&nodes[6].node, &nodes[9].node));
    ASSERT_TRUE(pctree_node_append_child(&nodes[6].node, &nodes[10].node));
    ASSERT_EQ(pctree_node_children_number(&nodes[6].node), 4);

    char buf[1024] = {0};
    size_t buf_len = 1023;
    purc_rwstream_t rws = purc_rwstream_new_from_mem (buf, buf_len);
    ASSERT_NE(rws, nullptr);

    for (size_t i=0; i<PCA_TABLESIZE(nodes); ++i) {
        for (size_t j=0; j<nr_loops; ++j) {
            if (method_f) {
                purc_rwstream_seek (rws, 0, SEEK_SET);
                memset(buf, 0, sizeof(buf));
                method_f(&nodes[i].node, rws);
                EXPECT_STREQ(buf, expects[i]);
            }

            size_t lvls = pctree_levels(&nodes[i].node);
            ASSERT_EQ(lvls, levels[i]) << "i: " << i << std::endl;
        }
    }

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);
}

static void
_random_destroy(struct number_node *nodes)
{
    struct pctree_node *p, *next;
    struct pctree_node *top = &nodes[0].node;
    pctree_for_each_post_order(top, p, next) {
        pctree_node_remove(p);
    }
    free(nodes);
}

static struct number_node*
_random_gen(size_t count)
{
    if (count<=0)
        return NULL;

    struct number_node *nodes;
    nodes = (struct number_node*)calloc(count, sizeof(*nodes));
    if (!nodes)
        return NULL;

    for (size_t i=0; i<count; ++i) {
        nodes[i].val = i;
    }

    for (size_t i=1; i<count; ++i) {
        size_t idx = rand() % i;
        pctree_node_append_child(&nodes[idx].node, &nodes[i].node);
    }

    return nodes;
}

static void
_build(struct pctree_node* node,  void* data)
{
    char buf[64] = {0};
    struct number_node* nn = container_of(node, struct number_node, node);
    snprintf(buf, sizeof(buf), "%zd ", nn->val);
    purc_rwstream_t rws = (purc_rwstream_t)data;
    purc_rwstream_write(rws, buf, strlen(buf));
}

TEST(tree, random)
{
    srand(time(0));
    const char *env_count = getenv("COUNT");
    size_t count = env_count ? atoll(env_count) : 0;
    if (count<=0)
        count = 10;

    if (!env_count)
        fprintf(stderr, "you can change `count` by specifying env `COUNT`\n");


    struct number_node *nodes;
    nodes = _random_gen(count);
    ASSERT_NE(nodes, nullptr);

    purc_rwstream_t pre_order = purc_rwstream_new_buffer(1024, 1024*1024);
    ASSERT_NE(pre_order, nullptr);
    purc_rwstream_t post_order = purc_rwstream_new_buffer(1024, 1024*1024);
    ASSERT_NE(post_order, nullptr);
    purc_rwstream_t level_order = purc_rwstream_new_buffer(1024, 1024*1024);
    ASSERT_NE(level_order, nullptr);

    pctree_node_pre_order_traversal(&nodes->node, _build, pre_order);
    pctree_node_post_order_traversal(&nodes->node, _build, post_order);
    pctree_node_level_order_traversal(&nodes->node, _build, level_order);

    purc_rwstream_write(pre_order, "\0", 1);
    purc_rwstream_write(post_order, "\0", 1);
    purc_rwstream_write(level_order, "\0", 1);

    const char *s_pre_order;
    const char *s_post_order;
    const char *s_level_order;
    s_pre_order = (const char*)purc_rwstream_get_mem_buffer(pre_order, NULL);
    s_post_order = (const char*)purc_rwstream_get_mem_buffer(post_order, NULL);
    s_level_order = (const char*)purc_rwstream_get_mem_buffer(level_order, NULL);

    // fprintf(stderr, "pre_order: %s\n", s_pre_order);
    // fprintf(stderr, "post_order: %s\n", s_post_order);
    // fprintf(stderr, "level_order: %s\n", s_level_order);
    (void)s_level_order;

    purc_rwstream_t rws;
    struct pctree_node *n, *next;
    const char *sz;

    rws = purc_rwstream_new_buffer(1024, 1024*1024);
    pctree_for_each_pre_order(&nodes->node, n) {
        char tmp[64] = {0};
        struct number_node *p = container_of(n, struct number_node, node);
        snprintf(tmp, sizeof(tmp), "%zd ", p->val);
        purc_rwstream_write(rws, tmp, strlen(tmp));
    }
    purc_rwstream_write(rws, "\0", 1);
    sz = (const char*)purc_rwstream_get_mem_buffer(rws, NULL);
    ASSERT_STREQ(sz, s_pre_order);
    purc_rwstream_destroy(rws);

    rws = purc_rwstream_new_buffer(1024, 1024*1024);
    struct pctree_node *top = &nodes->node;
    pctree_for_each_post_order(top, n, next) {
        char tmp[64] = {0};
        struct number_node *p = container_of(n, struct number_node, node);
        snprintf(tmp, sizeof(tmp), "%zd ", p->val);
        purc_rwstream_write(rws, tmp, strlen(tmp));
    }
    purc_rwstream_write(rws, "\0", 1);
    sz = (const char*)purc_rwstream_get_mem_buffer(rws, NULL);
    ASSERT_STREQ(sz, s_post_order);
    purc_rwstream_destroy(rws);

    size_t lvls = pctree_levels(&nodes[0].node);
    fprintf(stderr, "levels: %zd\n", lvls);

    purc_rwstream_destroy(pre_order);
    purc_rwstream_destroy(post_order);
    purc_rwstream_destroy(level_order);

    _random_destroy(nodes);
}


