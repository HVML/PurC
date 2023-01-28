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
#include "private/document.h"

struct pcdvobjs_element
{
    purc_document_t doc;
    pcdoc_element_t elem;
};

struct pcdvobjs_elements {
    purc_document_t         doc;
    pcutils_array_t        *elements;
    char                   *css;
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
pcdvobjs_element_attr_getter(purc_document_t doc, pcdoc_element_t elem,
        size_t nr_args, purc_variant_t *argv, bool silently);

purc_variant_t
pcdvobjs_element_prop_getter(purc_document_t doc, pcdoc_element_t elem,
        size_t nr_args, purc_variant_t *argv, bool silently);

purc_variant_t
pcdvobjs_element_style_getter(purc_document_t doc, pcdoc_element_t elem,
        size_t nr_args, purc_variant_t* argv, bool silently);

purc_variant_t
pcdvobjs_element_content_getter(purc_document_t doc, pcdoc_element_t elem,
        size_t nr_args, purc_variant_t* argv, bool silently);

purc_variant_t
pcdvobjs_element_text_content_getter(purc_document_t doc, pcdoc_element_t elem,
        size_t nr_args, purc_variant_t* argv, bool silently);

purc_variant_t
pcdvobjs_element_data_content_getter(purc_document_t doc, pcdoc_element_t elem,
        size_t nr_args, purc_variant_t* argv, bool silently);

purc_variant_t
pcdvobjs_element_val_getter(purc_document_t doc, pcdoc_element_t elem,
        size_t nr_args, purc_variant_t* argv, bool silently);

purc_variant_t
pcdvobjs_element_has_class_getter(purc_document_t doc, pcdoc_element_t elem,
        size_t nr_args, purc_variant_t* argv, bool silently);

purc_variant_t
pcdvobjs_make_elem_coll(pcdoc_elem_coll_t elem_coll);

pcdoc_element_t
pcdvobjs_find_element_in_doc(purc_document_t doc, const char *selector);

pcdoc_elem_coll_t
pcdvobjs_elem_coll_from_descendants(purc_document_t doc,
        pcdoc_element_t ancestor, const char *sel);

int
pcdvobjs_elem_coll_update(pcdoc_elem_coll_t elem_coll);

purc_variant_t
pcdvobjs_elem_coll_query(purc_document_t doc,
        pcdoc_element_t ancestor, const char *sel);

purc_variant_t
pcdvobjs_elem_coll_select_by_id(purc_document_t doc, const char *id);


PCA_EXTERN_C_END

#endif  /* PURC_DVOBJS_ELEMENT_H */

