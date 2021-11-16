/*
 * @file vcm.c
 * @author Xu Xiaohong
 * @date 2021/11/16
 * @brief The implementation for VCM native variant
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

#include "private/debug.h"
#include "private/errors.h"
#include "private/vcm.h"

#include "purc-variant.h"

struct pcintr_vcm {
    struct pcvcm_node          *vcm;
    purc_variant_t              evalued; // only valid for eval_const
};

static inline void
vcm_clean(struct pcintr_vcm *vcm)
{
    if (!vcm)
        return;

    if (vcm->evalued != PURC_VARIANT_INVALID) {
        purc_variant_unref(vcm->evalued);
        vcm->evalued = PURC_VARIANT_INVALID;
    }
}

static inline void
vcm_release(struct pcintr_vcm *vcm)
{
    if (!vcm)
        return;

    if (vcm->vcm) {
        pcvcm_node_destroy(vcm->vcm);
        vcm->vcm = NULL;
    }

    vcm_clean(vcm);
}

static purc_variant_t
eval(void* native_entity, size_t nr_args, purc_variant_t* argv)
{
    PC_ASSERT(native_entity);

    struct pcintr_vcm *vcm = (struct pcintr_vcm*)native_entity;
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    PC_ASSERT(vcm->vcm);

    // purc_variant_t pcvcm_eval (struct pcvcm_node* tree,
    //        struct pcintr_stack* stack);
    struct pcvcm_node *tree = vcm->vcm;
    struct pcintr_stack *stack = NULL;
    // FIXME: struct pcintr_stack *stack = purc_get_stack();

    return pcvcm_eval(tree, stack);
}

static purc_variant_t
eval_const(void* native_entity, size_t nr_args, purc_variant_t* argv)
{
    PC_ASSERT(native_entity);

    struct pcintr_vcm *vcm = (struct pcintr_vcm*)native_entity;
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    PC_ASSERT(vcm->vcm);

    if (vcm->evalued != PURC_VARIANT_INVALID)
        return vcm->evalued;

    // purc_variant_t pcvcm_eval (struct pcvcm_node* tree,
    //        struct pcintr_stack* stack);
    struct pcvcm_node *tree = vcm->vcm;
    struct pcintr_stack *stack = NULL;
    // FIXME: struct pcintr_stack *stack = purc_get_stack();

    return pcvcm_eval(tree, stack);
}

// query the getter for a specific property.
static inline purc_nvariant_method
property_getter (const char* key_name)
{
    PC_ASSERT(key_name);

    if (strcmp(key_name, "eval") == 0) {
        return eval;
    }

    if (strcmp(key_name, "eval_const") == 0) {
        return eval_const;
    }

    return NULL;
}

// the cleaner to clear the content of the native entity.
static inline bool
cleaner(void* native_entity)
{
    PC_ASSERT(native_entity);

    struct pcintr_vcm *vcm = (struct pcintr_vcm*)native_entity;
    vcm_clean(vcm);

    return true;
}

// the eraser to erase the native entity.
static inline bool
eraser(void* native_entity)
{
    PC_ASSERT(native_entity);

    struct pcintr_vcm *vcm = (struct pcintr_vcm*)native_entity;
    vcm_release(vcm);

    return true;
}

// the callback when the variant was observed (nullable).
static inline bool
observe(void* native_entity, ...)
{
    UNUSED_PARAM(native_entity);

    pcinst_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    return false;
}

// FIXME: where to put the declaration
purc_variant_t
pcintr_create_vcm_variant(struct pcvcm_node *vcm_node)
{
    PC_ASSERT(vcm_node);

    struct purc_native_ops ops = { NULL, };
    ops.property_getter          = property_getter;
    ops.cleaner                  = cleaner;
    ops.eraser                   = eraser;
    ops.observe                  = observe;

    struct pcintr_vcm *vcm;
    vcm = (struct pcintr_vcm*)calloc(1, sizeof(*vcm));
    if (!vcm) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }

    purc_variant_t v;
    v = purc_variant_make_native(vcm, &ops);
    if (v == PURC_VARIANT_INVALID) {
        vcm_release(vcm);
        free(vcm);
        return PURC_VARIANT_INVALID;
    }

    vcm->vcm = vcm_node;

    return v;
}

