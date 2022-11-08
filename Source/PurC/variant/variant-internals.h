/**
 * @file variant-public.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for public.
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


#ifndef _VARIANT_PUBLIC_H_
#define _VARIANT_PUBLIC_H_

#include "config.h"
#include "private/debug.h"
#include "private/variant.h"

#define PCVARIANT_CHECK_FAIL_RET(cond, ret)                     \
    if (!(cond)) {                                              \
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);             \
        return (ret);                                           \
    }

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/*
 * Set the extra size sz_ptr[0] of one variant, and update the statistics data.
 * This function should be called only for variant with
 * the flag PCVARIANT_FLAG_EXTRA_SIZE
 *
 * Note that the caller should not set the sz_ptr[0] directly.
 */
void pcvariant_stat_set_extra_size(purc_variant_t v, size_t sz) WTF_INTERNAL;

/* Allocate a variant for the specific type. */
purc_variant_t pcvariant_get (enum purc_variant_type type) WTF_INTERNAL;

/*
 * Release a unused variant.
 *
 * Note that the caller is responsible to release the extra memory
 * used by the variant.
 */
void pcvariant_put(purc_variant_t value) WTF_INTERNAL;

// for release the resource in a variant
typedef void (* pcvariant_release_fn) (purc_variant_t value);

// for release the resource in a variant
void pcvariant_string_release  (purc_variant_t value)    WTF_INTERNAL;
void pcvariant_sequence_release(purc_variant_t value)    WTF_INTERNAL;
void pcvariant_native_release  (purc_variant_t value)    WTF_INTERNAL;
void pcvariant_object_release  (purc_variant_t value)    WTF_INTERNAL;
void pcvariant_array_release   (purc_variant_t value)    WTF_INTERNAL;
void pcvariant_set_release     (purc_variant_t value)    WTF_INTERNAL;
void pcvariant_tuple_release   (purc_variant_t value)    WTF_INTERNAL;
void pcvariant_sorted_array_release (purc_variant_t value)    WTF_INTERNAL;

variant_arr_t
pcvar_arr_get_data(purc_variant_t arr) WTF_INTERNAL;
variant_obj_t
pcvar_obj_get_data(purc_variant_t obj) WTF_INTERNAL;
variant_set_t
pcvar_set_get_data(purc_variant_t set) WTF_INTERNAL;
variant_tuple_t
pcvar_tuple_get_data(purc_variant_t tuple) WTF_INTERNAL;
void
pcvar_adjust_set_by_descendant(purc_variant_t val) WTF_INTERNAL;

pcutils_map*
pcvar_create_rev_update_chain(void) WTF_INTERNAL;
void
pcvar_destroy_rev_update_chain(pcutils_map *chain) WTF_INTERNAL;
int
pcvar_rev_update_chain_add(pcutils_map *chain,
        struct pcvar_rev_update_edge *edge) WTF_INTERNAL;
void
pcvar_rev_update_chain_del(pcutils_map *chain,
        struct pcvar_rev_update_edge *edge) WTF_INTERNAL;

int
pcvar_reverse_check(purc_variant_t old, purc_variant_t _new) WTF_INTERNAL;

bool
pcvar_container_belongs_to_set(purc_variant_t val) WTF_INTERNAL;

purc_variant_t
pcvariant_container_clone(purc_variant_t cntr, bool recursively) WTF_INTERNAL;

purc_variant_t
pcvariant_array_clone(purc_variant_t arr, bool recursively) WTF_INTERNAL;
purc_variant_t
pcvariant_object_clone(purc_variant_t obj, bool recursively) WTF_INTERNAL;
purc_variant_t
pcvariant_set_clone(purc_variant_t set, bool recursively) WTF_INTERNAL;
purc_variant_t
pcvariant_tuple_clone(purc_variant_t tuple, bool recursively) WTF_INTERNAL;

purc_variant_t
pcvar_variant_from_rev_update_edge(struct pcvar_rev_update_edge *edge);

// break children's reverse update edges recursively
void
pcvar_break_rue_downward(purc_variant_t val);
void
pcvar_array_break_rue_downward(purc_variant_t arr);
void
pcvar_object_break_rue_downward(purc_variant_t obj);
void
pcvar_tuple_break_rue_downward(purc_variant_t tuple);

// break edge belongs to `val` and it's children's edges
// when `val` becomes dangling
void
pcvar_break_edge_to_parent(purc_variant_t val,
        struct pcvar_rev_update_edge *edge);
void
pcvar_array_break_edge_to_parent(purc_variant_t arr,
        struct pcvar_rev_update_edge *edge);
void
pcvar_object_break_edge_to_parent(purc_variant_t obj,
        struct pcvar_rev_update_edge *edge);
void
pcvar_set_break_edge_to_parent(purc_variant_t set,
        struct pcvar_rev_update_edge *edge);
void
pcvar_tuple_break_edge_to_parent(purc_variant_t tuple,
        struct pcvar_rev_update_edge *edge);

// build children's reverse update edges recursively
int
pcvar_build_rue_downward(purc_variant_t val);
int
pcvar_array_build_rue_downward(purc_variant_t arr);
int
pcvar_object_build_rue_downward(purc_variant_t obj);
int
pcvar_tuple_build_rue_downward(purc_variant_t tuple);

// build edge for `val` and it's children's edges
int
pcvar_build_edge_to_parent(purc_variant_t val,
        struct pcvar_rev_update_edge *edge);
int
pcvar_array_build_edge_to_parent(purc_variant_t arr,
        struct pcvar_rev_update_edge *edge);
int
pcvar_object_build_edge_to_parent(purc_variant_t obj,
        struct pcvar_rev_update_edge *edge);
int
pcvar_set_build_edge_to_parent(purc_variant_t set,
        struct pcvar_rev_update_edge *edge);
int
pcvar_tuple_build_edge_to_parent(purc_variant_t tuple,
        struct pcvar_rev_update_edge *edge);


struct obj_iterator {
    purc_variant_t                obj;

    struct obj_node              *curr;
    struct obj_node              *next;
    struct obj_node              *prev;
};

struct obj_iterator
pcvar_obj_it_first(purc_variant_t obj);
struct obj_iterator
pcvar_obj_it_last(purc_variant_t obj);
void
pcvar_obj_it_next(struct obj_iterator *it);
void
pcvar_obj_it_prev(struct obj_iterator *it);

struct arr_iterator {
    purc_variant_t                arr;

    struct arr_node              *curr;
    struct arr_node              *next;
    struct arr_node              *prev;
};

struct arr_iterator
pcvar_arr_it_first(purc_variant_t arr);
struct arr_iterator
pcvar_arr_it_last(purc_variant_t arr);
void
pcvar_arr_it_next(struct arr_iterator *it);
void
pcvar_arr_it_prev(struct arr_iterator *it);

enum set_it_type {
    SET_IT_ARRAY,
    SET_IT_RBTREE,
};

struct set_iterator {
    purc_variant_t                set;
    enum set_it_type              it_type;

    struct set_node              *curr;
    struct set_node              *next;
    struct set_node              *prev;
};

struct set_iterator
pcvar_set_it_first(purc_variant_t set, enum set_it_type it_type);
struct set_iterator
pcvar_set_it_last(purc_variant_t set, enum set_it_type it_type);
void
pcvar_set_it_next(struct set_iterator *it);
void
pcvar_set_it_prev(struct set_iterator *it);

struct kv_iterator {
    purc_variant_t                set;

    struct obj_iterator           it;
    size_t                        accu;
};

struct kv_iterator
pcvar_kv_it_first(purc_variant_t set, purc_variant_t obj);
void
pcvar_kv_it_next(struct kv_iterator *it);

struct tuple_iterator {
    purc_variant_t                tuple;
    size_t                        nr_members;
    size_t                        idx;

    purc_variant_t                curr;
    purc_variant_t                next;
    purc_variant_t                prev;
};

struct tuple_iterator
pcvar_tuple_it_first(purc_variant_t tuple);
struct tuple_iterator
pcvar_tuple_it_last(purc_variant_t tuple);
void
pcvar_tuple_it_next(struct tuple_iterator *it);
void
pcvar_tuple_it_prev(struct tuple_iterator *it);

bool
pcvar_rev_update_chain_pre_handler(
        purc_variant_t src,  // the source variant.
        pcvar_op_t op,       // the operation identifier.
        void *ctxt,          // the context stored when registering the handler.
        size_t nr_args,      // the number of the relevant child variants.
        purc_variant_t *argv // the array of all relevant child variants.
        );

bool
pcvar_rev_update_chain_post_handler(
        purc_variant_t src,  // the source variant.
        pcvar_op_t op,       // the operation identifier.
        void *ctxt,          // the context stored when registering the handler.
        size_t nr_args,      // the number of the relevant child variants.
        purc_variant_t *argv // the array of all relevant child variants.
        );

purc_variant_t
pcvar_set_clone_struct(purc_variant_t set);

// constraint-releated
purc_variant_t
pcvar_make_arr(void);

int
pcvar_arr_append(purc_variant_t arr, purc_variant_t val);

purc_variant_t
pcvar_make_obj(void);

int
pcvar_obj_set(purc_variant_t obj, purc_variant_t k, purc_variant_t v);

purc_variant_t
pcvar_make_set(variant_set_t data);

int
pcvar_set_add(purc_variant_t set, purc_variant_t val);

int
pcvar_readjust_set(purc_variant_t set, struct set_node *node);

// compare both variant-type and variant-value
// recursive-implementation, thus caller's responsible for enough stack space
// except stack space, no extra memory is required
// when `caseless` is set, use strcasecmp rather than strcmp internally
// when `unify_number` is set, convert both number-variants into long doubles
// before doing actuall comparison
int
pcvar_compare_ex(purc_variant_t l, purc_variant_t r,
        bool caseless, bool unify_number);

void
pcvar_parallel_walk(purc_variant_t l, purc_variant_t r, void *ctxt,
        int (*cb)(purc_variant_t l, purc_variant_t r, void *ctxt));

double
pcvar_str_numberify(purc_variant_t val);

double
pcvar_atom_numberify(purc_variant_t val);

double
pcvar_bs_numberify(purc_variant_t val);

double
pcvar_dynamic_numberify(purc_variant_t val);

double
pcvar_native_numberify(purc_variant_t val);

double
pcvar_obj_numberify(purc_variant_t val);

double
pcvar_arr_numberify(purc_variant_t val);

double
pcvar_set_numberify(purc_variant_t val);

double
pcvar_tuple_numberify(purc_variant_t val);

double
pcvar_numberify(purc_variant_t val);

int
pcvar_diff_numberify(purc_variant_t l, purc_variant_t r);


typedef int (*stringify_f)(const unsigned char *s, size_t len, void *ctxt);

int
pcvar_str_stringify(purc_variant_t val, void *ctxt, stringify_f cb);

int
pcvar_atom_stringify(purc_variant_t val, void *ctxt, stringify_f cb);

int
pcvar_bs_stringify(purc_variant_t val, void *ctxt, stringify_f cb);

int
pcvar_dynamic_stringify(purc_variant_t val, void *ctxt, stringify_f cb);

int
pcvar_native_stringify(purc_variant_t val, void *ctxt, stringify_f cb);

int
pcvar_obj_stringify(purc_variant_t val, void *ctxt, stringify_f cb);

int
pcvar_arr_stringify(purc_variant_t val, void *ctxt, stringify_f cb);

int
pcvar_set_stringify(purc_variant_t val, void *ctxt, stringify_f cb);

int
pcvar_tuple_stringify(purc_variant_t val, void *ctxt, stringify_f cb);

int
pcvar_stringify(purc_variant_t val, void *ctxt, stringify_f cb);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _VARIANT_PUBLIC_H_ */
