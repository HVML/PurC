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
#include "var-mgr.h"
#include "list.h"
#include "rbtree.h"

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define PCVARIANT_FLAG_CONSTANT        (0x01 << 0)  // for null, true, ...
#define PCVARIANT_FLAG_NOFREE          PCVARIANT_FLAG_CONSTANT
#define PCVARIANT_FLAG_EXTRA_SIZE      (0x01 << 1)  // when use extra space
#define PCVARIANT_FLAG_STRING_STATIC   (0x01 << 2)  // make_string_static

#define PVT(t) (PURC_VARIANT_TYPE##t)

#define MAX_RESERVED_VARIANTS   32
#define MAX_EMBEDDED_LEVELS     64

#define EXOBJ_LOAD_ENTRY        "__purcex_load_dynamic_variant"
#define EXOBJ_LOAD_HANDLE_KEY   "__intr_dlhandle"

#ifndef D
#define D(fmt, ...)                                           \
    if (TO_DEBUG) {                                           \
        fprintf(stderr, "%s[%d]:%s(): " fmt "\n",             \
            basename((char*)__FILE__), __LINE__, __func__,    \
            ##__VA_ARGS__);                                   \
    }
#endif // D


// mutually exclusive
#define PCVAR_LISTENER_PRE_OR_POST      (0x01)

#define PCVAR_LISTENER_PRE   (0x00)
#define PCVAR_LISTENER_POST  (0x01)

struct pcvar_listener {
    // the operation in which this listener is intersted.
    purc_atom_t         op;

    // the context for the listener
    void*               ctxt;

    // flags of the listener; only one flag currently: PRE or POST.
    unsigned int        flags;

    // the operation handler
    pcvar_op_handler    handler;

    // list node
    struct list_head    list_node;
};

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

    /* observer listeners */
    struct list_head        pre_listeners;
    struct list_head        post_listeners;

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

        /* for dynamic and native variant (two pointers)
           for native variant,
           ptr_ptr[0] stores the native entity of it's self, and
           ptr_ptr[1] stores the ops that's bound to the class of
           such entity. */
        void*       ptr_ptr[2];

        /* for long string, long byte sequence, array, object,
           and set (sz_ptr[0] for size, sz_ptr[1] for pointer).
           for atom string, sz_ptr[0] stores the atom. */
        /* for string_static, we also store strlen(sz_ptr[1]) into sz_ptr[0] */

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

    struct pcvarmgr_list      *variables;


    // experiment
    struct pcvariant_gc       *gc;
};

// initialize variant module (once)
void pcvariant_init_once(void) WTF_INTERNAL;

// experiment
void pcvariant_push_gc(void);
void pcvariant_pop_gc(void);
void pcvariant_gc_add(purc_variant_t val);
void pcvariant_gc_mov(purc_variant_t val);

struct pcinst;

// initialize the variant module for a PurC instance.
void pcvariant_init_instance(struct pcinst* inst) WTF_INTERNAL;
// clean up the variant module for a PurC instance.
void pcvariant_cleanup_instance(struct pcinst* inst) WTF_INTERNAL;

// internal struct used by variant-set object
typedef struct variant_set      *variant_set_t;

struct elem_node {
    struct rb_node   node;
    purc_variant_t   elem;  // actual variant-element
    purc_variant_t  *kvs;
    size_t           idx;
};

struct variant_set {
    char                   *unique_key; // unique-key duplicated
    const char            **keynames;
    size_t                  nr_keynames;
    struct rb_root          elems;  // multiple-variant-elements stored in set
    struct pcutils_arrlist *arr;    // also stored in arraylist
};

// internal struct used by variant-obj object
typedef struct variant_obj      *variant_obj_t;

struct obj_node {
    struct rb_node   node;
    purc_variant_t   key;
    purc_variant_t   val;
};

struct variant_obj {
    struct rb_root          kvs;  // struct obj_node*
    size_t                  size;
};


int pcvariant_array_swap(purc_variant_t value, int i, int j);
int pcvariant_set_swap(purc_variant_t value, int i, int j);

int pcvariant_array_sort(purc_variant_t value, void *ud,
        int (*cmp)(purc_variant_t l, purc_variant_t r, void *ud));
int pcvariant_set_sort(purc_variant_t value, void *ud,
        int (*cmp)(size_t nr_keynames, purc_variant_t l[], purc_variant_t r[],
            void *ud));

const char* pcvariant_get_typename(enum purc_variant_type type);

static inline const char*
pcvariant_typename(purc_variant_t v)
{
    enum purc_variant_type type;
    type = purc_variant_get_type(v);
    return pcvariant_get_typename(type);
}

ssize_t pcvariant_serialize(char *buf, size_t sz, purc_variant_t val);
char* pcvariant_serialize_alloc(char *buf, size_t sz, purc_variant_t val);

purc_variant_t pcvariant_make_object(size_t nr_kvs, ...);

__attribute__ ((format (printf, 1, 2)))
purc_variant_t pcvariant_make_with_printf(const char *fmt, ...);

// TODO: better generate with tool
extern purc_atom_t pcvariant_atom_grow;
extern purc_atom_t pcvariant_atom_shrink;
extern purc_atom_t pcvariant_atom_change;
extern purc_atom_t pcvariant_atom_reference;
extern purc_atom_t pcvariant_atom_unreference;

bool pcvariant_on_pre_fired(
        purc_variant_t source,  // the source variant
        purc_atom_t op,  // the atom of the operation,
                         // such as `grow`,  `shrink`, or `change`
        size_t nr_args,  // the number of the relevant child variants
                         // (only for container).
        purc_variant_t *argv    // the array of all relevant child variants
                                // (only for container).
        );

void pcvariant_on_post_fired(
        purc_variant_t source,  // the source variant
        purc_atom_t op,  // the atom of the operation,
                         // such as `grow`,  `shrink`, or `change`
        size_t nr_args,  // the number of the relevant child variants
                         // (only for container).
        purc_variant_t *argv    // the array of all relevant child variants
                                // (only for container).
        );

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
#define foreach_value_in_variant_array(_arr, _val)                       \
    do {                                                                 \
        struct pcutils_arrlist *_al;                                     \
        _al = (struct pcutils_arrlist*)_arr->sz_ptr[1];                  \
        for (size_t _i = 0; _i < _al->length; _i++) {                    \
            _val = (purc_variant_t)_al->array[_i];                       \
     /* } */                                                             \
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
        variant_obj_t _data;                                        \
        _data = (variant_obj_t)_obj->sz_ptr[1];                     \
        struct rb_root *_root = &_data->kvs;                        \
        struct rb_node *_p = pcutils_rbtree_first(_root);           \
        for (; _p; _p = pcutils_rbtree_next(_p))                    \
        {                                                           \
            struct obj_node *node;                                  \
            node = container_of(_p, struct obj_node, node);         \
            _val = node->val;                                       \
     /* } */                                                        \
 /* } while (0) */

#define foreach_key_value_in_variant_object(_obj, _key, _val)       \
    do {                                                            \
        variant_obj_t _data;                                        \
        _data = (variant_obj_t)_obj->sz_ptr[1];                     \
        struct rb_root *_root = &_data->kvs;                        \
        struct rb_node *_p = pcutils_rbtree_first(_root);           \
        for (; _p; _p = pcutils_rbtree_next(_p))                    \
        {                                                           \
            struct obj_node *node;                                  \
            node = container_of(_p, struct obj_node, node);         \
            _key = node->key;                                       \
            _val = node->val;                                       \
     /* } */                                                        \
 /* } while (0) */

#define foreach_in_variant_object_safe_x(_obj, _key, _val)          \
    do {                                                            \
        variant_obj_t _data;                                        \
        _data = (variant_obj_t)_obj->sz_ptr[1];                     \
        struct rb_root *_root = &_data->kvs;                        \
        struct rb_node *_p, _next;                                  \
        for (_p = pcutils_rbtree_first(_root);                      \
            ({_next = _p ? pcutils_rbtree_next(_p) : NULL; _p;});   \
            _p = _next)                                             \
        {                                                           \
            struct obj_node *node;                                  \
            node = container_of(_p, struct obj_node, node);         \
            _key = node->key;                                       \
            _val = node->val;                                       \
     /* } */                                                        \
 /* } while (0) */

#define foreach_value_in_variant_set(_set, _val)                        \
    do {                                                                \
        variant_set_t _data;                                            \
        struct pcutils_arrlist *_arr;                                   \
        _data = (variant_set_t)_set->sz_ptr[1];                         \
        _arr  = _data->arr;                                             \
        size_t _i;                                                      \
        for (_i=0; _i<_arr->length; ++_i) {                             \
            struct elem_node *_p;                                       \
            _p = (struct elem_node*)pcutils_arrlist_get_idx(_arr, _i);  \
            _val = _p->elem;                                            \
     /* } */                                                            \
  /* } while (0) */

#define foreach_value_in_variant_set_safe_x(_set, _val, _next)               \
    do {                                                                     \
        PC_ASSERT(purc_variant_is_type(_set, PURC_VARIANT_TYPE_SET));        \
        variant_set_t _data;                                                 \
        struct rb_root *_root;                                               \
        struct rb_node *_node;                                               \
        _data = (variant_set_t)_set->sz_ptr[1];                              \
        _root = &_data->elems;                                               \
        UNUSED_VARIABLE(_next);                                              \
        for (_node = pcutils_rbtree_first(_root);                            \
             ({_next = _node ? pcutils_rbtree_next(_node) : NULL; _node; }); \
             _node = _next)                                                  \
        {                                                                    \
            struct elem_node *_p;                                            \
            _p = container_of(_node, struct elem_node, node);                \
            _val = _p->elem;                                                 \
     /* } */                                                                 \
  /* } while (0) */

#define end_foreach                                                     \
 /* do { */                                                             \
     /* for (...) { */                                                  \
        }                                                               \
    } while (0)
#endif  /* PURC_PRIVATE_VARIANT_H */

