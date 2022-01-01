/**
 * @file variant.c
 * @author Xu Xiaohong
 * @date 2021/11/18
 * @brief The internal interfaces for interpreter
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
 *
 */

#include "config.h"

#include "internal.h"

#include "private/debug.h"
#include "private/instance.h"

#include <stdarg.h>

#define TO_DEBUG 1

// query the getter for a specific property.
static purc_nvariant_method
property_getter(const char* key_name)
{
    UNUSED_PARAM(key_name);
    PC_ASSERT(0); // TODO:
    return NULL;
};

// query the setter for a specific property.
static purc_nvariant_method
property_setter(const char* key_name)
{
    UNUSED_PARAM(key_name);
    PC_ASSERT(0); // TODO:
    return NULL;
}

// query the eraser for a specific property.
static purc_nvariant_method
property_eraser(const char* key_name)
{
    UNUSED_PARAM(key_name);
    PC_ASSERT(0); // TODO:
    return NULL;
}

// query the cleaner for a specific property.
static purc_nvariant_method
property_cleaner(const char* key_name)
{
    UNUSED_PARAM(key_name);
    PC_ASSERT(0); // TODO:
    return NULL;
}

// the cleaner to clear the content of the native entity.
static bool
cleaner(void* native_entity)
{
    UNUSED_PARAM(native_entity);
    PC_ASSERT(0); // TODO:
    return false;
}

// the eraser to erase the native entity.
static bool
eraser(void* native_entity)
{
    UNUSED_PARAM(native_entity);
    return true;
}

// the callback when the variant was observed (nullable).
static bool
observe(void* native_entity, ...)
{
    UNUSED_PARAM(native_entity);
    PC_ASSERT(0); // TODO:
    return false;
}

purc_variant_t
pcintr_make_element_variant(struct pcvdom_element *element)
{
    PC_ASSERT(element);
    static struct purc_native_ops ops = {
        .property_getter      = property_getter,
        .property_setter      = property_setter,
        .property_eraser      = property_eraser,
        .property_cleaner     = property_cleaner,
        .cleaner              = cleaner,
        .eraser               = eraser,
        .observe              = observe,
    };

    return purc_variant_make_native(element, &ops);
}

