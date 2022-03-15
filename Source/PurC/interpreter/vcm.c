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
#include "private/avl.h"

#include "purc-errors.h"
#include "purc-variant.h"

struct evalued_constant {
    struct avl_node             node;

    purc_variant_t              const_value;
};

static inline int
evalued_constant_init(struct evalued_constant *value, purc_variant_t v)
{
    // TODO: set key properly
    PC_ASSERT(value->const_value == PURC_VARIANT_INVALID);

    value->const_value = v; // NOTE: no need to ref!!!
    return 0;
}

static inline void
evalued_constant_release(struct evalued_constant *value)
{
    struct avl_node *node = &value->node;
    const char *key = node->key;
    // TODO: destroy(key);
    (void)key;

    if (value->const_value != PURC_VARIANT_INVALID) {
        purc_variant_unref(value->const_value);
        value->const_value = PURC_VARIANT_INVALID;
    }
}

struct pcintr_vcm {
    struct pcvcm_node          *vcm;

    struct avl_tree             values;
};

static inline void
vcm_clean(struct pcintr_vcm *vcm)
{
    if (!vcm)
        return;

    if (vcm->vcm) {
        pcvcm_node_destroy(vcm->vcm);
        vcm->vcm = NULL;
    }

    struct avl_tree *values = &vcm->values;

    if (avl_is_empty(values))
        return;

    struct evalued_constant *p, *n;

    avl_remove_all_elements(values, p, node, n) {
        evalued_constant_release(p);
        free(p);
    }
}

static purc_variant_t
eval(void* native_entity, size_t nr_args, purc_variant_t* argv, bool silently)
{
    UNUSED_PARAM(silently);
    PC_ASSERT(native_entity);

    struct pcintr_vcm *vcm = (struct pcintr_vcm*)native_entity;
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    PC_ASSERT(vcm->vcm);

    // purc_variant_t pcvcm_eval (struct pcvcm_node* tree,
    //        struct pcintr_stack* stack, bool silently);
    struct pcvcm_node *tree = vcm->vcm;
    struct pcintr_stack *stack = NULL;
    // FIXME: struct pcintr_stack *stack = pcintr_get_stack();

    return pcvcm_eval(tree, stack, silently);
}

static purc_variant_t
eval_const(void* native_entity, size_t nr_args, purc_variant_t* argv,
        bool silently)
{
    UNUSED_PARAM(silently);
    PC_ASSERT(native_entity);

    struct pcintr_vcm *vcm = (struct pcintr_vcm*)native_entity;
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    PC_ASSERT(vcm->vcm);

    // purc_variant_t pcvcm_eval (struct pcvcm_node* tree,
    //        struct pcintr_stack* stack, bool silently);
    struct pcvcm_node *tree = vcm->vcm;
    struct pcintr_stack *stack = NULL;
    // FIXME: struct pcintr_stack *stack = pcintr_get_stack();
    // check if already evalued
    // step 1: with key set to stack->bm_frame->scope, search vcm->values
    // step 2: if found, return found_evalued_constant->const_value
    // otherwise as follows:

    purc_variant_t v = pcvcm_eval(tree, stack, silently);
    if (v == PURC_VARIANT_INVALID)
        return PURC_VARIANT_INVALID;

    struct evalued_constant *const_value;
    const_value = (struct evalued_constant*)calloc(1, sizeof(*const_value));
    if (!const_value) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto error;
    }

    if (evalued_constant_init(const_value, v)) {
        goto error;
    }

    if (pcutils_avl_insert(&vcm->values, &const_value->node)) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto error;
    }

    return v;

error:
    if (const_value) free(const_value);
    purc_variant_unref(v);
    return PURC_VARIANT_INVALID;
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

// the cleaner to clear the content represented by the native entity.
static purc_variant_t
cleaner(void* native_entity, bool silently)
{
    PC_ASSERT(native_entity);
    UNUSED_PARAM(silently);

    struct pcintr_vcm *vcm = (struct pcintr_vcm*)native_entity;
    vcm_clean(vcm);
    return purc_variant_make_boolean(true);
}

// the callback to release the native entity.
static void
on_release(void* native_entity)
{
    PC_ASSERT(native_entity);

    struct pcintr_vcm *vcm = (struct pcintr_vcm*)native_entity;
    vcm_clean(vcm);
    free(vcm);
}

static inline int
cmp(const void *k1, const void *k2, void *ptr)
{
    UNUSED_PARAM(ptr);
    // TODO: with k1/k2 to scope1/scope2
    //       check if equal
    UNUSED_PARAM(k1);
    UNUSED_PARAM(k2);

    PC_ASSERT(0); // NOT implemented yet!
    return 0;
}

// FIXME: where to put the declaration
purc_variant_t
pcintr_create_vcm_variant(struct pcvcm_node *vcm_node)
{
    PC_ASSERT(vcm_node);

    static struct purc_native_ops ops = {
        .property_getter        = property_getter,
        .property_setter        = NULL,
        .property_eraser        = NULL,
        .property_cleaner       = NULL,

        .updater                = NULL,
        .cleaner                = cleaner,
        .eraser                 = NULL,

        .on_observe            = NULL,
        .on_release            = on_release,
    };

    struct pcintr_vcm *vcm;
    vcm = (struct pcintr_vcm*)calloc(1, sizeof(*vcm));
    if (!vcm) {
        pcinst_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return PURC_VARIANT_INVALID;
    }
    pcutils_avl_init(&vcm->values, cmp, false, NULL);

    purc_variant_t v;
    v = purc_variant_make_native(vcm, &ops);
    if (v == PURC_VARIANT_INVALID) {
        vcm_clean(vcm);
        free(vcm);
        return PURC_VARIANT_INVALID;
    }

    vcm->vcm = vcm_node;

    return v;
}

