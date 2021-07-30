/**
 * @file variant.h
 * @author 
 * @date 2021/07/02
 * @brief The internal interfaces for variant.
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

#ifndef PURC_PRIVATE_VARIANT_H
#define PURC_PRIVATE_VARIANT_H

#include "config.h"
#include "purc-variant.h"
#include "arraylist.h"
#include "avl.h"
#include "hashtable.h"

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define PCVARIANT_FLAG_CONSTANT     (0x01 << 0)     // for null, true, ...
#define PCVARIANT_FLAG_NOFREE       PCVARIANT_FLAG_CONSTANT
#define PCVARIANT_FLAG_EXTRA_SIZE   (0x01 << 1)     // when use extra space

#define PVT(t) (PURC_VARIANT_TYPE##t)

#define MAX_RESERVED_VARIANTS   32
#define MAX_EMBEDDED_LEVELS     64

// structure for variant
struct purc_variant {

    /* variant type */
    unsigned int type:8;

    /* real length for short string and byte sequence.
       use the extra space (long string and byte sequence)
       if the value of this field is 0. */
    unsigned int size:8;

    /* flags */
    unsigned int flags:16;

    /* reference count */
    unsigned int refc;

    /* value */
    union {
        /* for boolean */
        bool        b;

        /* for number */
        double      d;

        /* for long integer */
        int64_t     i64;

        /* for unsigned long integer */
        uint64_t    u64;

        /* for long double */
        long double ld;

        /* for dynamic and native variant (two pointers) */
        void*       ptr_ptr[2];

        /* for long string, long byte sequence, array, object,
           and set (sz_ptr[0] for size, sz_ptr[1] for pointer).
           for atom string, sz_ptr[0] stores the atom.  */

        uintptr_t   sz_ptr[2];

        /* for short string and byte sequence; the real space size of `bytes`
           is `max(sizeof(long double), sizeof(void*) * 2)` */
        uint8_t     bytes[0];
    };
};

#define MAX_RESERVED_VARIANTS   32

struct pcvariant_heap {
    // the constant values.
    struct purc_variant v_undefined;
    struct purc_variant v_null;
    struct purc_variant v_false;
    struct purc_variant v_true;

    // the statistics of memory usage of variant values
    struct purc_variant_stat stat;

    // the loop buffer for reserved values.
    purc_variant_t v_reserved [MAX_RESERVED_VARIANTS];
    int headpos;
    int tailpos;
};

// initialize variant module (once)
void pcvariant_init_once(void) WTF_INTERNAL;

struct pcinst;

// initialize the variant module for a PurC instance.
void pcvariant_init_instance(struct pcinst* inst) WTF_INTERNAL;
// clean up the variant module for a PurC instance.
void pcvariant_cleanup_instance(struct pcinst* inst) WTF_INTERNAL;

// internal struct used by variant-set object
typedef struct variant_set      *variant_set_t;
struct keyname {
    struct list_head  list;
    const char       *keyname;   // key name, no ownership
};

struct keyval {
    struct list_head list;
    purc_variant_t   val;    // value bounded to keyname
};

struct obj_node {
    struct avl_node  avl;
    purc_variant_t   obj;       // actual variant-object
    struct list_head keyvals;   // keyvals of this variant-object
};

struct variant_set {
    char                   *unique_key; // unique-key duplicated
    struct list_head        keynames;   // multiple-keys parsed from unique
    struct avl_tree         objs;       // multiple-variant-objects stored in set
};

#ifdef __cplusplus
}
#endif  /* __cplusplus */

/* VWNOTE (WARN)
 * 1. Make these macros as private interfaces. Please reimplement them
 *   by using the internal structure of the variant type for
 *   a better performance.
 *
 * 2. Implement the _safe version for easy change, e.g. removing an item,
 *  in an interation.
 */
#define foreach_value_in_variant_array(_arr, _val)          \
    do {                                                    \
        struct pcutils_arrlist *_al;                        \
        _al = (struct pcutils_arrlist*)_arr->sz_ptr[1];     \
        for (size_t _i = 0; _i < _al->length; _i++) {       \
            _val = (purc_variant_t)_al->array[_i];          \
     /* } */                                                \
 /* } while (0) */

#define foreach_value_in_variant_array_safe(_arr, _val, _curr)         \
    do {                                                               \
        struct pcutils_arrlist *_al;                                   \
        _al = (struct pcutils_arrlist*)_arr->sz_ptr[1];                \
        for (_curr = 0;                                                \
             _curr < _al->length;                                      \
             ++_curr)                                                  \
        {                                                              \
            _val = (purc_variant_t)_al->array[_curr];                  \
     /* } */                                                           \
 /* } while (0) */

#define foreach_value_in_variant_object(_obj, _val)                 \
    do {                                                            \
        struct pchash_table *_ht;                                   \
        _ht = (struct pchash_table*)_obj->sz_ptr[1];                \
        struct pchash_entry *_entry;                                \
        pchash_foreach(_ht, _entry)                                 \
        {                                                           \
            _val = (purc_variant_t)_entry->v;                       \
     /* } */                                                        \
 /* } while (0) */

#define foreach_key_value_in_variant_object(_obj, _key, _val)       \
    do {                                                            \
        struct pchash_table *_ht;                                   \
        _ht = (struct pchash_table*)_obj->sz_ptr[1];                \
        struct pchash_entry *_entry;                                \
        pchash_foreach(_ht, _entry)                                 \
        {                                                           \
            _key   = (const char*)_entry->k;                        \
            _val = (purc_variant_t)_entry->v;                       \
     /* } */                                                        \
 /* } while (0) */

#define foreach_in_variant_object_safe(_obj, _curr)                     \
    do {                                                                \
        struct pchash_table *_ht;                                       \
        _ht = (struct pchash_table*)_obj->sz_ptr[1];                    \
        struct pchash_entry *_tmp;                                      \
        pchash_foreach_safe(_ht, _curr, _tmp)                           \
        {                                                               \
     /* } */                                                            \
 /* } while (0) */

#define foreach_value_in_variant_set(_set, _val)                        \
    do {                                                                \
        struct avl_tree *_tree;                                         \
        _tree = (struct avl_tree*)_set->sz_ptr[1];                      \
        struct obj_node *_elem;                                         \
        avl_for_each_element(_tree, _elem, avl) {                       \
            _val = _elem->obj;                                          \
     /* } */                                                            \
  /* } while (0) */

#define foreach_value_in_variant_set_safe(_set, _val, _curr)            \
    do {                                                                \
        struct avl_tree *_tree;                                         \
        _tree = (struct avl_tree*)_set->sz_ptr[1];                      \
        struct obj_node *_tmp;                                          \
        avl_for_each_element_safe(_tree, _curr, avl, _tmp) {            \
            _val = _curr->obj;                                          \
     /* } */                                                            \
  /* } while (0) */

#define end_foreach                                                     \
 /* do { */                                                             \
     /* for (...) { */                                                  \
        }                                                               \
    } while (0)
#endif  /* PURC_PRIVATE_VARIANT_H */

