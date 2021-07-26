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

// create a new empty edom node
pcedom_element_t pcedom_element_create (pcedom_tree_t tree,
                    pcedom_tag_id_t tag_id, const char* tag_name,
                    enum purc_namespace ns)
{
    UNUSED_PARAM(tree);
    UNUSED_PARAM(tag_id);
    UNUSED_PARAM(tag_name);
    UNUSED_PARAM(ns);

    return NULL;
}

// set edom node attribution with string
pcedom_attr_t pcedom_element_set_attribute (pcedom_element_t element,
                     const char *attr_name, size_t name_len,
                     const char *attr_value, size_t value_len)
{
    UNUSED_PARAM(element);
    UNUSED_PARAM(attr_name);
    UNUSED_PARAM(name_len);
    UNUSED_PARAM(attr_value);
    UNUSED_PARAM(value_len);

    return NULL;
}

// get edom node attribution
pcedom_attr_t pcedom_element_get_attribute (pcedom_element_t element,
                     const char *attr_name, size_t name_len)
{
    UNUSED_PARAM(element);
    UNUSED_PARAM(attr_name);
    UNUSED_PARAM(name_len);

    return NULL
}

// remove indicated attribution
bool pcedom_element_remove_attribute (pcedom_element_t element,
                     pcedom_attr_t attr)
{
    UNUSED_PARAM(element);
    UNUSED_PARAM(attr);

    return NULL;
}

// remove indicated attribution with name
bool pcedom_element_remove_attribute_by_name (pcedom_element_t element,
                     const char* attr_name, size_t name_len)
{
    UNUSED_PARAM(element);
    UNUSED_PARAM(attr_name);
    UNUSED_PARAM(name_len);

    return true;
}

// destroy attribution of an edom node
bool pcedom_attribute_destroy (pcedom_attr_t attr)
{
    UNUSED_PARAM(attr);

    return true;
}

// set edom node content
bool pcedom_element_set_content (pcedom_element_t element,
                     purc_variant_t variant)
{
    UNUSED_PARAM(element);
    UNUSED_PARAM(variant);

    return true;
}

// destroy an edom node
bool pcedom_element_destroy(pcedom_element_t elem)
{
    UNUSED_PARAM(elem);
    return true;
}

