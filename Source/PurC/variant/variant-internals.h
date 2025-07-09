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
#include "private/mpops.h"

#define PCVRNT_CHECK_FAIL_RET(cond, ret)                        \
    if (!(cond)) {                                              \
        pcinst_set_error(PURC_ERROR_INVALID_VALUE);             \
        return (ret);                                           \
    }

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/*
 * Since 0.9.22, set the extra size (extra_size field) of one variant,
 * and update the statistics data.
 * This function should be called only for variant with
 * the flag PCVRNT_FLAG_EXTRA_SIZE
 *
 * Note that the caller should not set the extra_size directly.
 */
void pcvariant_stat_set_extra_size(purc_variant_t v, size_t sz) WTF_INTERNAL;

/*
 * Since 0.9.26, increase or decrease the memory use statistics data.
 * This function should be called for the ordinary variants who
 * manage the extra memory by themselves.
 */
void pcvariant_stat_inc_extra_size(purc_variant_t v, size_t sz) WTF_INTERNAL;
void pcvariant_stat_dec_extra_size(purc_variant_t v, size_t sz) WTF_INTERNAL;

/* Allocate a variant for the specific type. */
purc_variant_t pcvariant_get(enum purc_variant_type type) WTF_INTERNAL;

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
void pcvariant_longdouble_release(purc_variant_t value)     WTF_INTERNAL;
void pcvariant_bigint_release  (purc_variant_t value)       WTF_INTERNAL;
void pcvariant_string_release  (purc_variant_t value)       WTF_INTERNAL;
void pcvariant_sequence_release(purc_variant_t value)       WTF_INTERNAL;
void pcvariant_native_release  (purc_variant_t value)       WTF_INTERNAL;
void pcvariant_object_release  (purc_variant_t value)       WTF_INTERNAL;
void pcvariant_array_release   (purc_variant_t value)       WTF_INTERNAL;
void pcvariant_set_release     (purc_variant_t value)       WTF_INTERNAL;
void pcvariant_tuple_release   (purc_variant_t value)       WTF_INTERNAL;
void pcvariant_sorted_array_release(purc_variant_t value)   WTF_INTERNAL;

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
pcvar_variant_from_rev_update_edge(struct pcvar_rev_update_edge *edge)
    WTF_INTERNAL;

// break children's reverse update edges recursively
void
pcvar_break_rue_downward(purc_variant_t val) WTF_INTERNAL;
void
pcvar_array_break_rue_downward(purc_variant_t arr) WTF_INTERNAL;
void
pcvar_object_break_rue_downward(purc_variant_t obj) WTF_INTERNAL;
void
pcvar_tuple_break_rue_downward(purc_variant_t tuple) WTF_INTERNAL;

// break edge belongs to `val` and it's children's edges
// when `val` becomes dangling
void
pcvar_break_edge_to_parent(purc_variant_t val,
        struct pcvar_rev_update_edge *edge) WTF_INTERNAL;
void
pcvar_array_break_edge_to_parent(purc_variant_t arr,
        struct pcvar_rev_update_edge *edge) WTF_INTERNAL;
void
pcvar_object_break_edge_to_parent(purc_variant_t obj,
        struct pcvar_rev_update_edge *edge) WTF_INTERNAL;
void
pcvar_set_break_edge_to_parent(purc_variant_t set,
        struct pcvar_rev_update_edge *edge) WTF_INTERNAL;
void
pcvar_tuple_break_edge_to_parent(purc_variant_t tuple,
        struct pcvar_rev_update_edge *edge) WTF_INTERNAL;

// build children's reverse update edges recursively
int
pcvar_build_rue_downward(purc_variant_t val) WTF_INTERNAL;
int
pcvar_array_build_rue_downward(purc_variant_t arr) WTF_INTERNAL;
int
pcvar_object_build_rue_downward(purc_variant_t obj) WTF_INTERNAL;
int
pcvar_tuple_build_rue_downward(purc_variant_t tuple) WTF_INTERNAL;

// build edge for `val` and it's children's edges
int
pcvar_build_edge_to_parent(purc_variant_t val,
        struct pcvar_rev_update_edge *edge) WTF_INTERNAL;
int
pcvar_array_build_edge_to_parent(purc_variant_t arr,
        struct pcvar_rev_update_edge *edge) WTF_INTERNAL;
int
pcvar_object_build_edge_to_parent(purc_variant_t obj,
        struct pcvar_rev_update_edge *edge) WTF_INTERNAL;
int
pcvar_set_build_edge_to_parent(purc_variant_t set,
        struct pcvar_rev_update_edge *edge) WTF_INTERNAL;
int
pcvar_tuple_build_edge_to_parent(purc_variant_t tuple,
        struct pcvar_rev_update_edge *edge) WTF_INTERNAL;


struct obj_iterator {
    purc_variant_t                obj;

    struct obj_node              *curr;
    struct obj_node              *next;
    struct obj_node              *prev;
};

struct obj_iterator
pcvar_obj_it_first(purc_variant_t obj) WTF_INTERNAL;
struct obj_iterator
pcvar_obj_it_last(purc_variant_t obj) WTF_INTERNAL;
void
pcvar_obj_it_next(struct obj_iterator *it) WTF_INTERNAL;
void
pcvar_obj_it_prev(struct obj_iterator *it) WTF_INTERNAL;

struct arr_iterator {
    purc_variant_t                arr;

    struct arr_node              *curr;
    struct arr_node              *next;
    struct arr_node              *prev;
};

struct arr_iterator
pcvar_arr_it_first(purc_variant_t arr) WTF_INTERNAL;
struct arr_iterator
pcvar_arr_it_last(purc_variant_t arr) WTF_INTERNAL;
void
pcvar_arr_it_next(struct arr_iterator *it) WTF_INTERNAL;
void
pcvar_arr_it_prev(struct arr_iterator *it) WTF_INTERNAL;

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
pcvar_set_it_first(purc_variant_t set, enum set_it_type it_type) WTF_INTERNAL;
struct set_iterator
pcvar_set_it_last(purc_variant_t set, enum set_it_type it_type) WTF_INTERNAL;
void
pcvar_set_it_next(struct set_iterator *it) WTF_INTERNAL;
void
pcvar_set_it_prev(struct set_iterator *it) WTF_INTERNAL;

struct kv_iterator {
    purc_variant_t                set;

    struct obj_iterator           it;
    size_t                        accu;
};

struct kv_iterator
pcvar_kv_it_first(purc_variant_t set, purc_variant_t obj) WTF_INTERNAL;
void
pcvar_kv_it_next(struct kv_iterator *it) WTF_INTERNAL;

struct tuple_iterator {
    purc_variant_t                tuple;
    size_t                        nr_members;
    size_t                        idx;

    purc_variant_t                curr;
    purc_variant_t                next;
    purc_variant_t                prev;
};

struct tuple_iterator
pcvar_tuple_it_first(purc_variant_t tuple) WTF_INTERNAL;
struct tuple_iterator
pcvar_tuple_it_last(purc_variant_t tuple) WTF_INTERNAL;
void
pcvar_tuple_it_next(struct tuple_iterator *it) WTF_INTERNAL;
void
pcvar_tuple_it_prev(struct tuple_iterator *it) WTF_INTERNAL;

bool
pcvar_rev_update_chain_pre_handler(
        purc_variant_t src,  // the source variant.
        pcvar_op_t op,       // the operation identifier.
        void *ctxt,          // the context stored when registering the handler.
        size_t nr_args,      // the number of the relevant child variants.
        purc_variant_t *argv // the array of all relevant child variants.
        ) WTF_INTERNAL;

bool
pcvar_rev_update_chain_post_handler(
        purc_variant_t src,  // the source variant.
        pcvar_op_t op,       // the operation identifier.
        void *ctxt,          // the context stored when registering the handler.
        size_t nr_args,      // the number of the relevant child variants.
        purc_variant_t *argv // the array of all relevant child variants.
        ) WTF_INTERNAL;

purc_variant_t
pcvar_set_clone_struct(purc_variant_t set) WTF_INTERNAL;

// constraint-releated
purc_variant_t
pcvar_make_arr(void); WTF_INTERNAL

int
pcvar_arr_append(purc_variant_t arr, purc_variant_t val) WTF_INTERNAL;

purc_variant_t
pcvar_make_obj(void) WTF_INTERNAL;

int
pcvar_obj_set(purc_variant_t obj,
        purc_variant_t k, purc_variant_t v) WTF_INTERNAL;

purc_variant_t
pcvar_make_set(variant_set_t data) WTF_INTERNAL;

int
pcvar_set_add(purc_variant_t set, purc_variant_t val) WTF_INTERNAL;

int
pcvar_readjust_set(purc_variant_t set, struct set_node *node) WTF_INTERNAL;

// compare both variant-type and variant-value
// recursive-implementation, thus caller's responsible for enough stack space
// except stack space, no extra memory is required
// when `caseless` is set, use strcasecmp rather than strcmp internally
// when `unify_number` is set, convert both number-variants into long doubles
// before doing actuall comparison
int
pcvar_compare_ex(purc_variant_t l, purc_variant_t r,
        bool caseless, bool unify_number) WTF_INTERNAL;

void
pcvar_parallel_walk(purc_variant_t l, purc_variant_t r, void *ctxt,
        int (*cb)(purc_variant_t l, purc_variant_t r, void *ctxt)) WTF_INTERNAL;

double
pcvar_str_numerify(purc_variant_t val) WTF_INTERNAL;

double
pcvar_atom_numerify(purc_variant_t val) WTF_INTERNAL;

double
pcvar_bs_numerify(purc_variant_t val); WTF_INTERNAL

double
pcvar_dynamic_numerify(purc_variant_t val); WTF_INTERNAL

double
pcvar_native_numerify(purc_variant_t val); WTF_INTERNAL

double
pcvar_obj_numerify(purc_variant_t val); WTF_INTERNAL

double
pcvar_arr_numerify(purc_variant_t val); WTF_INTERNAL

double
pcvar_set_numerify(purc_variant_t val); WTF_INTERNAL

double
pcvar_tuple_numerify(purc_variant_t val); WTF_INTERNAL

double
pcvar_numerify(purc_variant_t val); WTF_INTERNAL

int
pcvar_diff_numerify(purc_variant_t l, purc_variant_t r) WTF_INTERNAL;

typedef int (*stringify_f)(const void *s, size_t len, void *ctxt);

#if 0 /* VW: deprecated since 0.9.26 */
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
#endif

size_t
pcvariant_array_children_memsize(purc_variant_t arr) WTF_INTERNAL;
size_t
pcvariant_object_children_memsize(purc_variant_t obj) WTF_INTERNAL;
size_t
pcvariant_set_children_memsize(purc_variant_t set) WTF_INTERNAL;
size_t
pcvariant_tuple_children_memsize(purc_variant_t tuple) WTF_INTERNAL;

#if BIGINT_LIMB_BITS == 32
#   define NR_CLIMBS_IN_BUFF    8
#else
#   define NR_CLIMBS_IN_BUFF    4
#endif

/* this bigint structure can hold at least four 64 bit unsigned integers */
typedef struct {
    /* space for ordinary variant */
    struct purc_variant_scalar vrt_hdr;
    /* must come just after */
    bi_limb_t tab[NR_CLIMBS_IN_BUFF];
} bigint_buf;

purc_variant *bigint_set_i64(bigint_buf *buf, int64_t a) WTF_INTERNAL;
purc_variant *bigint_set_u64(bigint_buf *buf, uint64_t a) WTF_INTERNAL;

void bigint_dump(FILE *fp, const char *prefx, purc_variant *p) WTF_INTERNAL;
int64_t bigint_get_si_sat(const purc_variant *a) WTF_INTERNAL;

purc_variant_t bigint_clone(const struct purc_variant *a) WTF_INTERNAL;
void bigint_move(struct purc_variant *to, struct purc_variant *from)
    WTF_INTERNAL;

int bigint_sign(const purc_variant *a) WTF_INTERNAL;
purc_variant *bigint_neg(const purc_variant *a) WTF_INTERNAL;
purc_variant *bigint_abs(const purc_variant *a) WTF_INTERNAL;

purc_variant *bigint_add(const purc_variant *a,
        const purc_variant *b, int b_neg) WTF_INTERNAL;
purc_variant *bigint_mul(const purc_variant *a,
        const purc_variant *b) WTF_INTERNAL;
purc_variant *bigint_divrem(const purc_variant *a,
        const purc_variant *b, bool is_rem) WTF_INTERNAL;
purc_variant *bigint_logic(const purc_variant *a,
        const purc_variant *b, purc_variant_operator op) WTF_INTERNAL;
purc_variant *bigint_not(const purc_variant *a) WTF_INTERNAL;
purc_variant *bigint_shl(const purc_variant *a,
        unsigned int shift1) WTF_INTERNAL;
purc_variant *bigint_shr(const purc_variant *a,
        unsigned int shift1) WTF_INTERNAL;
purc_variant *bigint_pow(const purc_variant *a,
        const purc_variant *b) WTF_INTERNAL;

bool bigint_to_i32(const purc_variant *a, int32_t *ret, bool force)
    WTF_INTERNAL;
bool bigint_to_u32(const purc_variant *a, uint32_t *ret, bool force)
    WTF_INTERNAL;
bool bigint_to_i64(const purc_variant *a, int64_t *ret, bool force)
    WTF_INTERNAL;
bool bigint_to_u64(const purc_variant *a, uint64_t *ret, bool force)
    WTF_INTERNAL;
double bigint_to_float64(const purc_variant *a) WTF_INTERNAL;

int bigint_i64_cmp(const purc_variant *a, int64_t i64) WTF_INTERNAL;
int bigint_u64_cmp(const purc_variant *a, uint64_t u64) WTF_INTERNAL;
int bigint_float64_cmp(const purc_variant *a, double f64) WTF_INTERNAL;
int bigint_cmp(const purc_variant *a, const purc_variant *b) WTF_INTERNAL;

ssize_t bigint_stringify(purc_variant_t val, int radix,
        void *ctxt, stringify_f cb) WTF_INTERNAL;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _VARIANT_PUBLIC_H_ */
