/**
 * @file variant.h
 * @author Vincent Wei
 * @date 2021/07/02
 * @brief The internal interfaces for variant.
 *
 * Copyright (C) 2021 ~ 2025 FMSoft <https://www.fmsoft.cn>
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
#include "private/debug.h"
#include "private/map.h"

PCA_EXTERN_C_BEGIN

#define PCVRNT_FLAG_CONSTANT        (0x01 << 0)  // for null, true, ...
#define PCVRNT_FLAG_NOFREE          PCVRNT_FLAG_CONSTANT
#define PCVRNT_FLAG_EXTRA_SIZE      (0x01 << 1)  // when use extra space
#define PCVRNT_FLAG_STATIC_DATA     (0x01 << 2)  // make_string_static

#define PVT(t)          (PURC_VARIANT_TYPE##t)
#define IS_CONTAINER(t) (t == PURC_VARIANT_TYPE_OBJECT || \
                        t == PURC_VARIANT_TYPE_ARRAY || \
                        t == PURC_VARIANT_TYPE_SET || \
                        t == PURC_VARIANT_TYPE_TUPLE)

#ifndef NDEBUG
// VW (NOTE): use 0 for easy finding memory leaks.
#define MAX_RESERVED_VARIANTS   0
#else
#define MAX_RESERVED_VARIANTS   32
#endif

#define DEF_EMBEDDED_LEVELS     64
#define MAX_EMBEDDED_LEVELS     1024

#define EXOBJ_LOAD_ENTRY        "__purcex_load_dynamic_variant"
#define EXOBJ_LOAD_HANDLE_KEY   "__intr_dlhandle"

#define PRINT_MIN_BUFFER     512
#define PRINT_MAX_BUFFER     1024 * 1024 * 1024

#ifdef NDEBUG
#define PRINT_VARIANT(_v)   do { (void)_v; } while (0)
#else
#define PRINT_VARIANT(_v)                                                     \
do {                                                                          \
    if (_v == PURC_VARIANT_INVALID) {                                         \
        PC_DEBUG("%s[%d]:%s(): %s[%p]=PURC_VARIANT_INVALID\n",                \
            pcutils_basename((char*)__FILE__), __LINE__, __func__, #_v, _v);  \
        break;                                                                \
    }                                                                         \
    char* _buf = pcvariant_to_string(_v);                                     \
    const char* _type = pcvariant_typename(_v);                               \
    PC_DEBUG("%s[%d]:%s(): %s[%p][%s]=%s\n",                                  \
            pcutils_basename((char*)__FILE__), __LINE__, __func__,            \
            #_v, _v, _type, _buf);                                            \
    free(_buf);                                                               \
} while (0)
#endif


// mutually exclusive
#define PCVAR_LISTENER_PRE_OR_POST      (0x01)

#define PCVAR_LISTENER_PRE   (0x00)
#define PCVAR_LISTENER_POST  (0x01)

struct pcvar_listener {
    // the operation in which this listener is intersted.
    pcvar_op_t          op;

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

    /* The length for short string and byte sequence (both in bytes).
       The length for short bigint (in limbs).
       When the extra space is used, the value of this field is 0.
     */
    unsigned int size:8;

    /* flags */
    unsigned int flags:16;

    /* reference count */
    unsigned int refc;

    /* value */
    union {
        /* for boolean */
        bool        b;

        /* for exception and atom string (uint32_t) */
        purc_atom_t atom;

        /* for number */
        double      d;

        /* for long integer */
        int64_t     i64;

        /* for unsigned long integer */
        uint64_t    u64;

        /* for native variant, the pointer to the native entity;
           for dynamic variant, the pointer to the getter.  */
        void       *ptr;

        /* for long double */
        long double *ld;

        /* for array, object, and set: the number of children;
           for bigint, the length of limbs if using the extra size;
           for long byte sequence, the length in bytes;
           for long string, the length in bytes
           (including the terminating null byte) */
        size_t      len;

        /* for short string and byte sequence; the real space size of `bytes`
           is `NR_BYTES_IN_WRAPPER` (see below) */
        uint8_t     bytes[0];

        /* for bigint; the real space size of `words`
           is `NR_WORDS_IN_WRAPPER` (see below) */
        uint16_t    words[0];

        /* for bigint; the real space size of `dwords`
           is `NR_DWORDS_IN_WRAPPER` (see below) */
        uint32_t    dwords[0];

        /* for bigint; the real space size of `qwords`
           is `NR_QWORDS_IN_WRAPPER` (see below) */
        uint64_t    qwords[0];
    };

    void           *ptr2;           // for native entity: ops
                                    // for dynamic variant: setter

    union {
        void           *extra_data; // for native entity only.
        size_t          extra_size; // for other complex variants
                                    // which use extra memory space.
        /* other aliases */
        uintptr_t       extra_uintptr;
        intptr_t        extra_intptr;
        uint8_t         extra_bytes[0];
        uint16_t        extra_words[0];
        uint32_t        extra_dwords[0];
        uint64_t        extra_qwords[0];
    };

    /* for observable or complex variants. */
    union {
        struct list_head    listeners;  // the list head for listeners.
        struct list_head    reserved;   // the list node for reserved variants.
    };
};

#define SZ_VARIANT_HEADER       8
#define NR_BYTES_IN_WRAPPER                                     \
    (sizeof(void *) + sizeof(void *) + sizeof(void *) +         \
        sizeof(struct list_head))

#define NR_WORDS_IN_WRAPPER     (NR_BYTES_IN_WRAPPER / 2)
#define NR_DWORDS_IN_WRAPPER    (NR_BYTES_IN_WRAPPER / 4)
#define NR_QWORDS_IN_WRAPPER    (NR_BYTES_IN_WRAPPER / 8)

/* Use this structure for scalar variants, like undefined, null,
   boolean, number, longint, ulongint, bigint, longdouble,
   exception, and atom. */
struct purc_variant_scalar {
    unsigned int type:8;
    unsigned int size:8;
    unsigned int flags:16;
    unsigned int refc;

    /* scalar value */
    union {
        /* for boolean */
        bool        b;

        /* for exception or atom string (uint32_t) */
        purc_atom_t atom;

        /* for number */
        double      d;

        /* for long integer */
        int64_t     i64;

        /* for unsigned long integer */
        uint64_t    u64;

        /* for pointer */
        void       *ptr;

        /* for long double */
        long double *ld;
    };
};

typedef struct purc_variant_scalar purc_variant_scalar;

typedef enum {
    OP_add,
    OP_sub,
    OP_mul,
    OP_floordiv,
    OP_truediv,
    OP_mod,
    OP_pow,
    OP_and,
    OP_or,
    OP_xor,
    OP_not,
} purc_variant_operator;

struct pcvariant_heap {
    // the constant values.
    struct purc_variant_scalar v_undefined;
    struct purc_variant_scalar v_null;
    struct purc_variant_scalar v_false;
    struct purc_variant_scalar v_true;

    // the statistics of memory usage of variant values
    struct purc_variant_stat stat;

    // the loop buffer for reserved scalar variants.
    purc_variant_t      v_reserved_sca[MAX_RESERVED_VARIANTS];
    int                 headpos;
    int                 tailpos;

    // use linked loop list for other reserved variants.
    struct list_head    v_reserved;
};

// internal interfaces for moving variant.
purc_variant_t pcvariant_move_heap_in(purc_variant_t v) WTF_INTERNAL;
purc_variant_t pcvariant_move_heap_out(purc_variant_t v) WTF_INTERNAL;

void pcvariant_use_move_heap(void) WTF_INTERNAL;
void pcvariant_use_norm_heap(void) WTF_INTERNAL;

static inline bool is_type_scalar(enum purc_variant_type type) {
    return (type <= PURC_VARIANT_TYPE_LAST_SCALAR) ? true : false;
}

static inline bool is_variant_scalar(purc_variant *v) {
    return (v->type <= PURC_VARIANT_TYPE_LAST_SCALAR) ? true : false;
}

purc_variant *pcvariant_alloc(bool scalar) WTF_INTERNAL;
purc_variant *pcvariant_alloc_0(bool scalar) WTF_INTERNAL;
void pcvariant_free(purc_variant *v) WTF_INTERNAL;

struct pcinst;
struct tuple_node;

struct pcvar_rev_update_edge {
    purc_variant_t                   parent;
    union {
        // where to locate in parent
        struct set_node             *set_me;
        struct obj_node             *obj_me;
        struct arr_node             *arr_me;
        struct tuple_node           *tuple_me;
    };
};

// internal struct used by variant-set object
typedef struct variant_set      *variant_set_t;

struct set_node {
    struct rb_node                       rbnode;
    struct pcutils_array_list_node       alnode;
    purc_variant_t   val;  // actual variant-element
    char             md5[33];
};

struct variant_set {
    char                   *unique_key; // unique-key duplicated
    const char            **keynames;
    size_t                  nr_keynames;
    bool                    caseless;
    struct rb_root          elems;  // multiple-variant-elements stored in set
    struct pcutils_array_list al;    // struct set_node

    // key: arr_node/obj_node/set_node
    // val: parent
    pcutils_map                     *rev_update_chain;
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

    // key: arr_node/obj_node/set_node
    // val: parent
    pcutils_map                     *rev_update_chain;
};

// internal struct used by variant-arr
typedef struct variant_arr      *variant_arr_t;

struct arr_node {
    struct pcutils_array_list_node       node;
    purc_variant_t   val;
};

struct variant_arr {
    struct pcutils_array_list     al;  // struct arr_node*

    // key: arr_node/obj_node/set_node
    // val: parent
    pcutils_map                     *rev_update_chain;
};


// internal struct used by variant-tuple
typedef struct variant_tuple      *variant_tuple_t;

struct variant_tuple {
    purc_variant_t                *members; // struct tuple_node* (purc_variant_t)

    // key: arr_node/obj_node/set_node/tuple_node
    // val: parent
    pcutils_map                   *rev_update_chain;
};


#define PCVRNT_SORT_DESC            0x10000000
#define PCVRNT_SORT_ASC             0x00000000
#define PCVRNT_CMPOPT_MASK          0x0000FFFF

int pcvariant_array_sort(purc_variant_t value, void *ud,
        int (*cmp)(purc_variant_t l, purc_variant_t r, void *ud));
int pcvariant_set_sort(purc_variant_t value, void *ud,
        int (*cmp)(purc_variant_t l, purc_variant_t r, void *ud));

int pcvariant_diff(purc_variant_t l, purc_variant_t r);
int pcvariant_diff_ex(purc_variant_t l, purc_variant_t r,
        enum pcvrnt_compare_method opt);

static inline const char*
pcvariant_typename(purc_variant_t v)
{
    enum purc_variant_type type;
    type = purc_variant_get_type(v);
    return purc_variant_typename(type);
}

static inline bool
pcvariant_is_true(purc_variant_t v)
{
    if (v->type == PURC_VARIANT_TYPE_BOOLEAN)
        return v->b;

    return false;
}

static inline bool
pcvariant_is_false(purc_variant_t v)
{
    if (v->type == PURC_VARIANT_TYPE_BOOLEAN)
        return v->b == false;

    return false;
}

bool
pcvariant_is_scalar(purc_variant_t v);

bool
pcvariant_is_of_string(purc_variant_t v);

bool
pcvariant_is_of_number(purc_variant_t v);

// return -1 if not valid set
// on return:
// if *keynames == NULL, this is a generic-set
// otherwise, (*keynames)[0] ~ (*keynames)[*nr_keynames-1] designates uniqkey
// of the set
int pcvariant_set_get_uniqkeys(purc_variant_t set, size_t *nr_keynames,
        const char ***keynames);

ssize_t pcvariant_serialize(char *buf, size_t sz, purc_variant_t val);
char* pcvariant_serialize_alloc(char *buf, size_t sz, purc_variant_t val);

char* pcvariant_to_string(purc_variant_t v);

purc_variant_t pcvariant_make_object(size_t nr_kvs, ...);

WTF_ATTRIBUTE_PRINTF(1, 2)
purc_variant_t pcvariant_make_with_printf(const char *fmt, ...);

bool pcvariant_is_mutable(purc_variant_t val);

bool pcvariant_on_pre_fired(
        purc_variant_t source,  // the source variant.
        pcvar_op_t op,          // the operation identifier.
        size_t nr_args,         // the number of the relevant child variants.
        purc_variant_t *argv    // the array of all relevant child variants.
        );

void pcvariant_on_post_fired(
        purc_variant_t source,  // the source variant.
        pcvar_op_t op,          // the operation identifier.
        size_t nr_args,         // the number of the relevant child variants.
        purc_variant_t *argv    // the array of all relevant child variants.
        );

purc_variant_t pcvariant_set_find (purc_variant_t set, purc_variant_t value);

static inline bool
pcvariant_is_in_set (purc_variant_t set, purc_variant_t value)
{
    return (PURC_VARIANT_INVALID != pcvariant_set_find(set, value));
}

purc_variant_t
pcvariant_object_shallow_copy(purc_variant_t obj);

bool
pcvariant_object_clear(purc_variant_t object, bool silently);

bool
pcvariant_array_clear(purc_variant_t array, bool silently);

bool
pcvariant_set_clear(purc_variant_t set, bool silently);

// md5 shall be at least 33 bytes long
void pcvariant_md5_ex(char *md5, purc_variant_t val, const char *salt,
    bool caseless, unsigned int serialize_flags) WTF_INTERNAL;

PCA_INLINE void
pcvariant_md5(char *md5, purc_variant_t val)
{
    const char *salt = NULL;
    unsigned int serialize_flags = 0;
    bool caseless = false;
    pcvariant_md5_ex(md5, val, salt, caseless, serialize_flags);
}

// md5 shall be at least 33 bytes long
void
pcvariant_md5_by_set(char *md5, purc_variant_t val,
        purc_variant_t set) WTF_INTERNAL;

int
pcvariant_diff_by_set(const char *md5l, purc_variant_t l,
        const char *md5r, purc_variant_t r, purc_variant_t set);

bool
pcvariant_is_sorted_array(purc_variant_t v);

bool
pcvariant_is_linear_container(purc_variant_t v);

bool
pcvariant_container_displace(purc_variant_t dst,
        purc_variant_t src, bool silently);

bool
pcvariant_container_remove(purc_variant_t dst,
        purc_variant_t src, bool silently);

bool
pcvariant_array_append_another(purc_variant_t array,
        purc_variant_t another, bool silently);
bool
pcvariant_array_prepend_another(purc_variant_t array,
        purc_variant_t another, bool silently);

bool
pcvariant_array_insert_another_before(purc_variant_t array,
        int idx, purc_variant_t another, bool silently);
bool
pcvariant_array_insert_another_after(purc_variant_t array, int idx,
        purc_variant_t another, bool silently);

PCA_EXTERN_C_END

static inline
purc_variant_t *tuple_members(purc_variant_t tuple, size_t *sz)
{
    if (UNLIKELY(!(tuple && tuple->type == PVT(_TUPLE))))
        return NULL;

    variant_tuple_t data = (variant_tuple_t) tuple->ptr2;
    *sz = (size_t)tuple->len;
    return data->members;
}

#define PURC_VARIANT_SAFE_CLEAR(_v)             \
do {                                            \
    if (_v != PURC_VARIANT_INVALID) {           \
        purc_variant_unref(_v);                 \
        _v = PURC_VARIANT_INVALID;              \
    }                                           \
} while (0)

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
    &(((variant_arr_t)_arr->ptr2)->al)

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
        _data = (variant_obj_t)_obj->ptr2;                          \
        struct rb_root *_root = &_data->kvs;                        \
        struct rb_node *_p = pcutils_rbtree_first(_root);           \
        for (; _p; _p = pcutils_rbtree_next(_p))                    \
        {                                                           \
            struct obj_node *_node;                                 \
            _node = container_of(_p, struct obj_node, node);        \
            _val = _node->val;                                      \
     /* } */                                                        \
 /* } while (0) */

#define foreach_key_value_in_variant_object(_obj, _key, _val)       \
    do {                                                            \
        variant_obj_t _data;                                        \
        _data = (variant_obj_t)_obj->ptr2;                          \
        struct rb_root *_root = &_data->kvs;                        \
        struct rb_node *_p = pcutils_rbtree_first(_root);           \
        for (; _p; _p = pcutils_rbtree_next(_p))                    \
        {                                                           \
            struct obj_node *_node;                                 \
            _node = container_of(_p, struct obj_node, node);        \
            _key = _node->key;                                      \
            _val = _node->val;                                      \
     /* } */                                                        \
 /* } while (0) */

#define foreach_in_variant_object_safe_x(_obj, _key, _val)          \
    do {                                                            \
        variant_obj_t _data;                                        \
        _data = (variant_obj_t)_obj->ptr2;                          \
        struct rb_root *_root = &_data->kvs;                        \
        struct rb_node *_p, *_next;                                 \
        for (_p = pcutils_rbtree_first(_root);                      \
            ({_next = _p ? pcutils_rbtree_next(_p) : NULL; _p;});   \
            _p = _next)                                             \
        {                                                           \
            struct obj_node *_node;                                 \
            _node = container_of(_p, struct obj_node, node);        \
            _key = _node->key;                                      \
            _val = _node->val;                                      \
     /* } */                                                        \
 /* } while (0) */

#define foreach_value_in_variant_set(_set, _val)                        \
    do {                                                                \
        variant_set_t _data;                                            \
        struct pcutils_array_list *_al;                                 \
        _data = (variant_set_t)_set->ptr2;                              \
        _al = &_data->al;                                               \
        struct pcutils_array_list_node *_p;                             \
        for (_p = pcutils_array_list_get_first(_al);                    \
             _p;                                                        \
             _p = pcutils_array_list_get(_al,  _p->idx+1))              \
        {                                                               \
            struct set_node *_sn;                                       \
            _sn = container_of(_p, struct set_node, alnode);            \
            _val = _sn->val;                                            \
     /* } */                                                            \
  /* } while (0) */

#define foreach_value_in_variant_set_reverse(_set, _val)                \
    do {                                                                \
        variant_set_t _data;                                            \
        struct pcutils_array_list *_al;                                 \
        _data = (variant_set_t)_set->ptr2;                              \
        _al = &_data->al;                                               \
        struct pcutils_array_list_node *_p;                             \
        for (_p = pcutils_array_list_get_last(_al);                     \
             _p;                                                        \
             _p = pcutils_array_list_get(_al,  _p->idx-1))              \
        {                                                               \
            struct set_node *_sn;                                       \
            _sn = container_of(_p, struct set_node, alnode);            \
            _val = _sn->val;                                            \
     /* } */                                                            \
  /* } while (0) */

#define foreach_value_in_variant_set_safe(_set, _val)                         \
    do {                                                                      \
        variant_set_t _data;                                                  \
        struct pcutils_array_list *_al;                                       \
        _data = (variant_set_t)_set->ptr2;                                    \
        _al = &_data->al;                                                     \
        struct pcutils_array_list_node *_p, *_n;                              \
        for (_p = pcutils_array_list_get_first(_al);                          \
             ({ _n = _p ? pcutils_array_list_get(_al,  _p->idx+1) : NULL;     \
              _p; });                                                         \
             _p = _n)                                                         \
        {                                                                     \
            struct set_node *_sn;                                             \
            _sn = container_of(_p, struct set_node, alnode);                  \
            _val = _sn->val;                                                  \
     /* } */                                                                  \
  /* } while (0) */

#define foreach_value_in_variant_set_reverse_safe(_set, _val)                 \
    do {                                                                      \
        variant_set_t _data;                                                  \
        struct pcutils_array_list *_al;                                       \
        _data = (variant_set_t)_set->ptr2;                                    \
        _al = &_data->al;                                                     \
        struct pcutils_array_list_node *_p, *_n;                              \
        for (_p = pcutils_array_list_get_last(_al);                           \
             ({ _n = _p ? pcutils_array_list_get(_al,  _p->idx-1) : NULL;     \
              _p; });                                                         \
             _p = _n)                                                         \
        {                                                                     \
            struct set_node *_sn;                                             \
            _sn = container_of(_p, struct set_node, alnode);                  \
            _val = _sn->val;                                                  \
     /* } */                                                                  \
  /* } while (0) */

#define foreach_value_in_variant_set_order(_set, _val)                  \
    do {                                                                \
        variant_set_t _data;                                            \
        struct rb_node *_first;                                         \
        _data = (variant_set_t)_set->ptr2;                              \
        _first = pcutils_rbtree_first(&_data->elems);                   \
        if (!_first)                                                    \
            break;                                                      \
        struct rb_node *_p;                                             \
        pcutils_rbtree_for_each(_first, _p)                             \
        {                                                               \
            struct set_node *_sn;                                       \
            _sn = container_of(_p, struct set_node, rbnode);            \
            _val = _sn->val;                                            \
     /* } */                                                            \
  /* } while (0) */

#define foreach_value_in_variant_set_order_reverse(_set, _val)          \
    do {                                                                \
        variant_set_t _data;                                            \
        struct rb_node *_first;                                         \
        _data = (variant_set_t)_set->ptr2;                              \
        _first = pcutils_rbtree_last(&_data->elems);                    \
        if (!_first)                                                    \
            break;                                                      \
        struct rb_node *_p;                                             \
        pcutils_rbtree_for_each_reverse(_first, _p)                     \
        {                                                               \
            struct set_node *_sn;                                       \
            _sn = container_of(_p, struct set_node, rbnode);            \
            _val = _sn->val;                                            \
     /* } */                                                            \
  /* } while (0) */

#define foreach_value_in_variant_set_order_safe(_set, _val)             \
    do {                                                                \
        variant_set_t _data;                                            \
        struct rb_node *_first;                                         \
        _data = (variant_set_t)_set->ptr2;                              \
        _first = pcutils_rbtree_first(&_data->elems);                   \
        if (!_first)                                                    \
            break;                                                      \
        struct rb_node *_p, *_n;                                        \
        pcutils_rbtree_for_each_safe(_first, _p, _n)                    \
        {                                                               \
            struct set_node *_sn;                                       \
            _sn = container_of(_p, struct set_node, rbnode);            \
            _val = _sn->val;                                            \
     /* } */                                                            \
  /* } while (0) */

#define foreach_value_in_variant_set_order_reverse_safe(_set, _val)     \
    do {                                                                \
        variant_set_t _data;                                            \
        struct rb_node *_last;                                          \
        _data = (variant_set_t)_set->ptr2;                              \
        _last = pcutils_rbtree_last(&_data->elems);                     \
        if (!_last)                                                     \
            break;                                                      \
        struct rb_node *_p, *_n;                                        \
        pcutils_rbtree_for_each_reverse_safe(_last, _p, _n)             \
        {                                                               \
            struct set_node *_sn;                                       \
            _sn = container_of(_p, struct set_node, rbnode);            \
            _val = _sn->val;                                            \
     /* } */                                                            \
  /* } while (0) */

#define end_foreach                                                     \
 /* do { */                                                             \
     /* for (...) { */                                                  \
        }                                                               \
    } while (0)

#endif  /* PURC_PRIVATE_VARIANT_H */

