/**
 * @file variant.h
 * @author Vincent Wei
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
#include "array_list.h"

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

#define PRINT_MIN_BUFFER     512
#define PRINT_MAX_BUFFER     1024 * 1024 * 1024

#define PRINT_VARIANT(v)                                                    \
    do {                                                                    \
        purc_rwstream_t rws = purc_rwstream_new_buffer(PRINT_MIN_BUFFER,    \
                PRINT_MAX_BUFFER);                                          \
        size_t len_expected = 0;                                            \
        purc_variant_serialize(v, rws,                                      \
                0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);           \
        purc_rwstream_write(rws, "", 1);                                    \
        char* buf = (char*)purc_rwstream_get_mem_buffer_ex(rws, NULL, NULL, \
                true);                                                      \
        fprintf(stderr, "%s[%d]:%s(): %s=%s\n",                             \
                basename((char*)__FILE__), __LINE__, __func__, #v, buf);    \
        free(buf);                                                          \
        purc_rwstream_destroy(rws);                                         \
    } while (0)


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

    /* The length for short string (in characters) and byte sequence (in bytes).
       When the extra space (long string and long byte sequence) is used,
       the value of this field is 0. */
    unsigned int size:8;

    /* flags */
    unsigned int flags:16;

    /* reference count */
    unsigned int refc;

    /* FIXME: listeners: use only one struct list_head field. */
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
           ptr_ptr[0] stores the pointer to the native entity, and
           ptr_ptr[1] stores the ops that's bound to the class of
           such entity. */
        void*       ptr_ptr[2];

        /* For long byte sequence, array, object, and set,
              - `sz_ptr[0]` stores the size in bytes;
              - `sz_ptr[1]` stores the pointer.

           For long string,
              - `sz_ptr[0]` stores the length in characters;
              - `sz_ptr[1]` stores the pointer.

           For exception and atom string,
             - `sz_ptr[0]` should always be 0.
             - `sz_ptr[1]` stores the atom. */
        uintptr_t   sz_ptr[2];
        /* FIXME: DONT do this:
           for string_static, we store strlen(sz_ptr[1]) into sz_ptr[0] */

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

    // managed by variant_set
    purc_variant_t   set; // owner
    struct pcvar_listener      *grow;
    struct pcvar_listener      *change;
    struct pcvar_listener      *shrink;
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

// internal struct used by variant-arr
typedef struct variant_arr      *variant_arr_t;

struct arr_node {
    struct pcutils_array_list_node       node;
    purc_variant_t   val;
};

struct variant_arr {
    struct pcutils_array_list     al;  // struct arr_node*
};


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

// return -1 if not valid set
// on return:
// if *keynames == NULL, this is a generic-set
// otherwise, (*keynames)[0] ~ (*keynames)[*nr_keynames-1] designates uniqkey
// of the set
int pcvariant_set_get_uniqkeys(purc_variant_t set, size_t *nr_keynames,
        const char ***keynames);

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

bool pcvariant_is_mutable(purc_variant_t val);

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

purc_variant_t pcvariant_set_find (purc_variant_t set, purc_variant_t value);

static inline bool
pcvariant_is_in_set (purc_variant_t set, purc_variant_t value)
{
    return (PURC_VARIANT_INVALID != pcvariant_set_find(set, value));
}

purc_variant_t
pcvariant_object_shallow_copy(purc_variant_t obj);

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

// purc_variant_t _arr;
#define variant_array_get_data(_arr)        \
    &(((variant_arr_t)_arr->sz_ptr[1])->al)

// purc_variant_t _arr;
// struct arr_node *_p;
#define foreach_in_variant_array(_arr, _p)                               \
    array_list_for_each_entry(variant_array_get_data(_arr),              \
            _p, node)

// purc_variant_t _arr;
// struct arr_node *_p, *_n;
#define foreach_in_variant_array_safe(_arr, _p, _n)                      \
    array_list_for_each_entry_safe(variant_array_get_data(_arr),         \
            _p, _n, node)

// purc_variant_t _arr;
// struct arr_node *_p;
#define foreach_in_variant_array_reverse(_arr, _p)                       \
    array_list_for_each_entry_reverse(variant_array_get_data(_arr),      \
            _p, node)

// purc_variant_t _arr;
// struct arr_node *_p, *_n;
#define foreach_in_variant_array_reverse_safe(_arr, _p, _n)              \
    array_list_for_each_entry_reverse_safe(variant_array_get_data(_arr), \
            _p, _n, node)

#define foreach_value_in_variant_array(_arr, _val, _idx)              \
    do {                                                              \
        struct arr_node *_p;                                          \
        foreach_in_variant_array(_arr, _p) {                          \
            _val = _p->val;                                           \
            _idx = _p->node.idx;                                      \
     /* } */                                                          \
 /* } while (0) */

#define foreach_value_in_variant_array_safe(_arr, _val, _idx)      \
    do {                                                           \
        struct arr_node *_p, *_n;                                  \
        foreach_in_variant_array_safe(_arr, _p, _n) {              \
            _val = _p->val;                                        \
            _idx = _p->node.idx;                                   \
     /* } */                                                       \
 /* } while (0) */

#define foreach_value_in_variant_array_reverse(_arr, _val, _idx)      \
    do {                                                              \
        struct arr_node *_p;                                          \
        foreach_in_variant_array_reverse(_arr, _p) {                  \
            _val = _p->val;                                           \
            _idx = _p->node.idx;                                      \
     /* } */                                                          \
 /* } while (0) */

#define foreach_value_in_variant_array_reverse_safe(_arr, _val, _idx)   \
    do {                                                                \
        struct arr_node *_p, *_n;                                       \
        foreach_in_variant_array_reverse_safe(_arr, _p, _n) {           \
            _val = _p->val;                                             \
            _idx = _p->node.idx;                                        \
     /* } */                                                            \
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
        struct rb_node *_p, *_next;                                 \
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
        struct elem_node *_p, *_n;                                      \
        for (_p = (struct elem_node*) pcutils_arrlist_get_first(_arr);  \
             ({ _n = _p ? (struct elem_node*)pcutils_arrlist_get_idx(   \
                                                _arr,  _p->idx+1)       \
                        : NULL;                                         \
              _p; });                                                   \
             _p = _n)                                                   \
        {                                                               \
            _val = _p->elem;                                            \
     /* } */                                                            \
  /* } while (0) */

#define foreach_value_in_variant_set_reverse(_set, _val)                \
    do {                                                                \
        variant_set_t _data;                                            \
        struct pcutils_arrlist *_arr;                                   \
        _data = (variant_set_t)_set->sz_ptr[1];                         \
        _arr  = _data->arr;                                             \
        struct elem_node *_p, *_n;                                      \
        for (_p = (struct elem_node*) pcutils_arrlist_get_last(_arr);   \
             ({ _n = (_p && _p->idx > 0)                                \
                        ? (struct elem_node*)pcutils_arrlist_get_idx(   \
                                                _arr,  _p->idx-1)       \
                        : NULL;                                         \
              _p; });                                                   \
             _p = _n)                                                   \
        {                                                               \
            _val = _p->elem;                                            \
     /* } */                                                            \
  /* } while (0) */

#define foreach_value_in_variant_set_safe(_set, _val)                   \
    do {                                                                \
        variant_set_t _data;                                            \
        struct pcutils_arrlist *_arr;                                   \
        _data = (variant_set_t)_set->sz_ptr[1];                         \
        _arr  = _data->arr;                                             \
        struct elem_node *_p, *_n;                                      \
        for (_p = (struct elem_node*) pcutils_arrlist_get_first(_arr);  \
             ({ _n = _p ? (struct elem_node*)pcutils_arrlist_get_idx(   \
                                                _arr,  _p->idx+1)       \
                        : NULL;                                         \
              _p; });                                                   \
             _p = _n)                                                   \
        {                                                               \
            _val = _p->elem;                                            \
     /* } */                                                            \
  /* } while (0) */

#define foreach_value_in_variant_set_reverse_safe(_set, _val)           \
    do {                                                                \
        variant_set_t _data;                                            \
        struct pcutils_arrlist *_arr;                                   \
        _data = (variant_set_t)_set->sz_ptr[1];                         \
        _arr  = _data->arr;                                             \
        struct elem_node *_p, *_n;                                      \
        for (_p = (struct elem_node*) pcutils_arrlist_get_last(_arr);   \
             ({ _n = (_p && _p->idx > 0)                                \
                        ? (struct elem_node*)pcutils_arrlist_get_idx(   \
                                                _arr,  _p->idx-1)       \
                        : NULL;                                         \
              _p; });                                                   \
             _p = _n)                                                   \
        {                                                               \
            _val = _p->elem;                                            \
     /* } */                                                            \
  /* } while (0) */

#define end_foreach                                                     \
 /* do { */                                                             \
     /* for (...) { */                                                  \
        }                                                               \
    } while (0)
#endif  /* PURC_PRIVATE_VARIANT_H */

