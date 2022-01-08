/**
 * @file element.h
 * @author Xu Xiaohong
 * @date 2021/11/18
 * @brief The internal interfaces for dvobjs/element
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

#ifndef PURC_DVOBJS_ELEMENT_H
#define PURC_DVOBJS_ELEMENT_H

#include "purc-macros.h"
#include "purc-variant.h"

#include "private/debug.h"
#include "private/errors.h"
#include "private/edom.h"

struct pcintr_element
{
    struct pcedom_element          *elem;       // NOTE: no ownership
};

struct pcintr_elements {
    pcutils_array_t          *elements; // struct pcintr_element *
};

struct native_property_cfg {
    const char            *property_name;

    purc_nvariant_method   property_getter;
    purc_nvariant_method   property_setter;
    purc_nvariant_method   property_eraser;
    purc_nvariant_method   property_cleaner;
};

PCA_EXTERN_C_BEGIN

purc_variant_t
pcintr_query_elements(struct pcedom_element *root,
    const char *css) WTF_INTERNAL;


purc_variant_t
pcintr_element_attr_getter(pcedom_element_t *element,
        size_t nr_args, purc_variant_t *argv);

purc_variant_t
pcintr_element_prop_getter(pcedom_element_t *element,
        size_t nr_args, purc_variant_t *argv);

purc_variant_t
pcintr_element_style_getter(pcedom_element_t *element,
        size_t nr_args, purc_variant_t* argv);

purc_variant_t
pcintr_element_content_getter(pcedom_element_t *element,
        size_t nr_args, purc_variant_t* argv);

purc_variant_t
pcintr_element_text_content_getter(pcedom_element_t *element,
        size_t nr_args, purc_variant_t* argv);

purc_variant_t
pcintr_element_json_content_getter(pcedom_element_t *element,
        size_t nr_args, purc_variant_t* argv);

purc_variant_t
pcintr_element_val_getter(pcedom_element_t *element,
        size_t nr_args, purc_variant_t* argv);

purc_variant_t
pcintr_element_has_class_getter(pcedom_element_t *element,
        size_t nr_args, purc_variant_t* argv);

PCA_EXTERN_C_END

#endif  /* PURC_DVOBJS_ELEMENT_H */

