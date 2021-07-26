/*
 * @file tree.c
 * @author 
 * @date 2021/07/02
 * @brief The implementation of edom tree.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "private/variant.h"
#include "private/instance.h"
#include "private/errors.h"
#include "private/debug.h"
#include "private/utils.h"
#include "private/edom.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>


// create a new empty edom tree
pcedom_tree_t pcedom_tree_create(void)
{
    return NULL;
}

// initialize edom tree with html parser
bool pcedom_tree_init(pcedom_tree_t tree, pchtml_t parser) 
{
    UNUSED_PARAM(tree);
    UNUSED_PARAM(parser);

    return true;
}

#if 0
// initialize edom tree with XGML parser
bool pcedom_tree_init(pcedom_tree_t tree, pcxgml_t parser)
{
    UNUSED_PARAM(tree);
    UNUSED_PARAM(parser);

    return true;
}

// initialize edom tree with XML parser
bool pcedom_tree_init(pcedom_tree_t tree, pcxml_t parser)
{
    UNUSED_PARAM(tree);
    UNUSED_PARAM(parser);

    return true;
}
#endif

// append an edom element as child
bool pcedom_tree_element_add_child(pcedom_element_t parent, 
                                pcedom_element_t elem)
{
    UNUSED_PARAM(parent);
    UNUSED_PARAM(elem);

    return true;
}

// insert an edom elment before indicated element
bool pcedom_tree_element_insert_before(pcedom_element_t current,
                                pcedom_element_t elem)
{
    UNUSED_PARAM(current);
    UNUSED_PARAM(elem);

    return true;
}

// insert an edom elment after indicated element
bool pcedom_tree_element_insert_after(pcedom_element_t current,
                                pcedom_element_t elem)
{
    UNUSED_PARAM(current);
    UNUSED_PARAM(elem);

    return true;
}

// remove an edom elment from tree
bool pcedom_tree_remove_element(pcedom_tree_t tree,
                                pcedom_element_t elem)
{
    UNUSED_PARAM(tree);
    UNUSED_PARAM(elem);

    return true;
}

// clean edom tree, the tree is existence
bool pcedom_tree_clean(pcedom_tree_t tree)
{
    UNUSED_PARAM(tree);

    return true;
}

// destroy edom tree, the tree is disappear
void pcedom_tree_destroy(pcedom_tree_t tree)
{
    UNUSED_PARAM(tree);

    return;
}

// get the first element for an edom tree
pcedom_element_t pcedom_element_first(pcedom_tree_t tree)
{
    UNUSED_PARAM(tree);

    return NULL;
}

// get the last element for an edom tree
pcedom_element_t pcedom_element_last(pcedom_tree_t tree)
{
    UNUSED_PARAM(tree);

    return NULL;
}

// get the first child element from an indicated edom element
pcedom_element_t pcedom_element_child(pcedom_element_t elem)
{
    UNUSED_PARAM(elem);

    return NULL;
}

// get the last child element from an indicated edom element
pcedom_element_t pcedom_element_last_child(pcedom_element_t elem)
{
    UNUSED_PARAM(elem);

    return NULL;
}

// get the next brother edom element of an indicated edom element
pcedom_element_t pcedom_element_next(pcedom_element_t elem)
{
    UNUSED_PARAM(elem);

    return NULL;
}

// get the previous brother edom element of an indicated edom element
pcedom_element_t pcedom_element_prev(pcedom_element_t elem)
{
    UNUSED_PARAM(elem);

    return NULL;
}
