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

// create an empty edom element collection
pcedom_collection_t pcedom_collection_create(size_t size)
{
    UNUSED_PARAM(size);

    return true;
}


// clear an empty edom element collection
bool pcedom_collection_clean(pcedom_collection_t collection)
{
    UNUSED_PARAM(collection);

    return true;
}


// destroy an empty edom element collection
bool pcedom_collection_destroy(pcedom_collection_t collection)
{
    UNUSED_PARAM(collection);

    return true;
}


// create collection by tag
pcedom_collection_t pcedom_get_elements_by_tag_id(pcedom_tree_t tree,
                                pcedom_collection_t collection,
                                pcedom_tag_id_t tag_id)
{
    UNUSED_PARAM(tree);
    UNUSED_PARAM(collection);
    UNUSED_PARAM(tag_id);

    return NULL;
}

// create collection by name 
pcedom_collection_t pcedom_get_elements_by_name(pcedom_tree_t tree,
                                pcedom_collection_t collection,
                                const char* name, size_t length)
{
    UNUSED_PARAM(tree);
    UNUSED_PARAM(collection);
    UNUSED_PARAM(name);
    UNUSED_PARAM(name_len);

    return NULL;
}


// create collection by attribution name 
pcedom_collection_t pcedom_get_elements_by_attribute_name(pcedom_tree_t tree,
                                pcedom_collection_t collection,
                                pcedom_element_t scope_node,
                                const char* key, size_t key_len)
{
    UNUSED_PARAM(tree);
    UNUSED_PARAM(collection);
    UNUSED_PARAM(scope_node);
    UNUSED_PARAM(key);
    UNUSED_PARAM(key_len);

    return NULL;
}

// create collection by attribution value 
pcedom_collection_t pcedom_get_elements_by_attribute_value(pcedom_tree_t tree,
                                pcedom_collection_t collection,
                                pcedom_element_t node,
                                bool case_insensitive,
                                const char* key, size_t key_len,
                                const char* value,
                                size_t value_len)
{
    UNUSED_PARAM(tree);
    UNUSED_PARAM(collection);
    UNUSED_PARAM(node);
    UNUSED_PARAM(case_insensitive);
    UNUSED_PARAM(key);
    UNUSED_PARAM(key_len);
    UNUSED_PARAM(value);
    UNUSED_PARAM(value_len);

    return NULL;
}

// create collection by attribution value 
pcedom_collection_t pcedom_get_elements_by_attribute_value_whitespace_separated(
                                pcedom_tree_t tree,
                                pcedom_collection_t collection,
                                pcedom_element_t node,
                                bool case_insensitive,
                                const char* key, size_t key_len,
                                const char* value,
                                size_t value_len)
{
    UNUSED_PARAM(tree);
    UNUSED_PARAM(collection);
    UNUSED_PARAM(node);
    UNUSED_PARAM(case_insensitive);
    UNUSED_PARAM(key);
    UNUSED_PARAM(key_len);
    UNUSED_PARAM(value);
    UNUSED_PARAM(value_len);

    return NULL;
}
