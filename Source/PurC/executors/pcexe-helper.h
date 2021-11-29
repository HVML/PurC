/*
 * @file pcexe-helper.h
 * @author Xu Xiaohong
 * @date 2021/10/23
 * @brief
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


#ifndef PURC_EXECUTOR_PCEXE_HELPER_H
#define PURC_EXECUTOR_PCEXE_HELPER_H

#include "config.h"

#include "purc-macros.h"
#include "purc-variant.h"
#include "private/list.h"
#include "private/tree.h"

#include <regex.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#define PCEXE_CLR_VAR(_v) do {                    \
    if (_v != PURC_VARIANT_INVALID) {             \
        purc_variant_unref(_v);                   \
        _v = PURC_VARIANT_INVALID;                \
    }                                             \
} while (0)

#define PCEXE_FREE(_v) do {                       \
    if (_v) {                                     \
        free(_v);                                 \
        _v = NULL;                                \
    }                                             \
} while (0)

PCA_EXTERN_C_BEGIN

int pcexe_ucs2utf8(char *utf, const char *uni, size_t n);

int pcexe_utf8_to_wchar(const char *utf8, wchar_t *wc);
int pcexe_wchar_to_utf8(const wchar_t wc, char *utf8, size_t n);
wchar_t* pcexe_wchar_from_utf8(const char *utf8, size_t *bytes, size_t *chars);
char* pcexe_utf8_from_wchar(const wchar_t *ws, size_t *chars, size_t *bytes);

static inline int
pcexe_obj_set(purc_variant_t obj, purc_variant_t k, double v)
{
    purc_variant_t t = purc_variant_make_number(v);
    bool ok = purc_variant_object_set(obj, k, t);
    purc_variant_unref(t);
    return ok ? 0 : -1;
}

struct pcexe_strlist {
    char         **strings;
    size_t         size;
    size_t         capacity;
};

struct pcexe_strlist* pcexe_strlist_create(void);
void pcexe_strlist_destroy(struct pcexe_strlist *list);

static inline void
pcexe_strlist_init(struct pcexe_strlist *list)
{
    list->strings  = NULL;
    list->size     = 0;
    list->capacity = 0;
}

void pcexe_strlist_reset(struct pcexe_strlist *list);
int pcexe_strlist_append_buf(struct pcexe_strlist *list,
        const char *buf, size_t n);

static inline int
pcexe_strlist_append_str(struct pcexe_strlist *list, const char *str)
{
    return pcexe_strlist_append_buf(list, str, strlen(str));
}

static inline int
pcexe_strlist_append_chr(struct pcexe_strlist *list, const char c)
{
    return pcexe_strlist_append_buf(list, &c, 1);
}

static inline int
pcexe_strlist_append_uni(struct pcexe_strlist *list,
        const char *uni, size_t n)
{
    char utf8[7];
    int r = pcexe_ucs2utf8(utf8, uni, n);
    if (r)
        return r;

    return pcexe_strlist_append_str(list, utf8);
}

char* pcexe_strlist_to_str(struct pcexe_strlist *list);

purc_variant_t
pcexe_make_cache(purc_variant_t input, bool asc_desc);

// typedef unsigned char     matching_flags;
#define MATCHING_FLAG_C 0x01
#define MATCHING_FLAG_I 0x02
#define MATCHING_FLAG_S 0x04

#define MATCHING_FLAGS_SET(_flags, _c) switch (_c) {          \
    case 'c':                                                 \
        _flags |= MATCHING_FLAG_C;                            \
        break;                                                \
    case 'i':                                                 \
        _flags |= MATCHING_FLAG_I;                            \
        break;                                                \
    case 's':                                                 \
        _flags |= MATCHING_FLAG_S;                            \
        break;                                                \
    default:                                                  \
        /* ignore */                                          \
        break;                                                \
};
#define MATCHING_FLAGS_IS_SET_WITH(_flags, _flag) ((_flags) & (_flag))

// typedef unsigned char     regexp_flags;
#define REGEXP_FLAG_G 0x01
#define REGEXP_FLAG_I 0x02
#define REGEXP_FLAG_M 0x04
#define REGEXP_FLAG_S 0x08
#define REGEXP_FLAG_U 0x10
#define REGEXP_FLAG_Y 0x20

#define REGEXP_FLAGS_SET(_flags, _c) switch (_c) {           \
    case 'g':                                                \
        _flags |= REGEXP_FLAG_G;                             \
        break;                                               \
    case 'i':                                                \
        _flags |= REGEXP_FLAG_I;                             \
        break;                                               \
    case 'm':                                                \
        _flags |= REGEXP_FLAG_M;                             \
        break;                                               \
    case 's':                                                \
        _flags |= REGEXP_FLAG_S;                             \
        break;                                               \
    case 'u':                                                \
        _flags |= REGEXP_FLAG_U;                             \
        break;                                               \
    case 'y':                                                \
        _flags |= REGEXP_FLAG_Y;                             \
        break;                                               \
    default:                                                 \
        /* ignore */                                         \
        break;                                               \
};
#define REGEXP_FLAGS_IS_SET_WITH(_flags, _flag) ((_flags) & (_flag))

struct matching_suffix
{
    unsigned char        matching_flags;
    long int             max_matching_length; // non-positive: unset
};

struct literal_expression
{
    char                   *literal;
    struct matching_suffix  suffix;

    struct list_head        node;
};

static inline void
literal_expression_reset(struct literal_expression *lexp)
{
    if (!lexp)
        return;

    free(lexp->literal);
}

struct wildcard_expression
{
    char                   *wildcard;
    struct matching_suffix  suffix;

    void                   *pattern_spec;
};

struct regular_expression
{
    char                   *regexp;
    unsigned char           flags;

    int                     eflags;
    regex_t                 reg;
    unsigned int            reg_valid:1;
};

enum STRING_PATTERN_TYPE {
    STRING_PATTERN_WILDCARD,
    STRING_PATTERN_REGEXP,
};

struct string_pattern_expression
{
    enum STRING_PATTERN_TYPE    type;
    union {
        struct wildcard_expression       wildcard;
        struct regular_expression        regexp;
    };

    struct list_head            node;
};

void
string_pattern_expression_reset(struct string_pattern_expression *spexp);

struct string_literal_list
{
    struct list_head              list;
};

static inline struct string_literal_list*
string_literal_list_create(void)
{
    struct string_literal_list *list;
    list = (struct string_literal_list*)calloc(1, sizeof(*list));
    if (!list)
        return NULL;

    INIT_LIST_HEAD(&list->list);

    return list;
}

static inline void
string_literal_list_destroy(struct string_literal_list *list)
{
    if (!list)
        return;

    struct list_head *p, *n;
    list_for_each_safe(p, n, &list->list) {
        struct literal_expression *lexp;
        lexp = container_of(p, struct literal_expression, node);
        list_del(p);
        literal_expression_reset(lexp);
        free(lexp);
    }

    free(list);
}

struct string_pattern_list
{
    struct list_head              list;
};

static inline struct string_pattern_list*
string_pattern_list_create(void)
{
    struct string_pattern_list *list;
    list = (struct string_pattern_list*)calloc(1, sizeof(*list));
    if (!list)
        return NULL;

    INIT_LIST_HEAD(&list->list);

    return list;
}

static inline void
string_pattern_list_destroy(struct string_pattern_list *list)
{
    if (!list)
        return;

    struct list_head *p, *n;
    list_for_each_safe(p, n, &list->list) {
        struct string_pattern_expression *lexp;
        lexp = container_of(p, struct string_pattern_expression, node);
        list_del(p);
        string_pattern_expression_reset(lexp);
        free(lexp);
    }

    free(list);
}

enum string_matching_type
{
    STRING_MATCHING_PATTERN,
    STRING_MATCHING_LITERAL,
};

struct string_matching_condition
{
    enum string_matching_type       type;
    union {
        struct string_pattern_list          *patterns;
        struct string_literal_list          *literals;
    };
};

static inline void
string_matching_condition_reset(struct string_matching_condition *mexp)
{
    if (!mexp)
        return;

    switch(mexp->type)
    {
        case STRING_MATCHING_PATTERN:
            string_pattern_list_destroy(mexp->patterns);
            break;
        case STRING_MATCHING_LITERAL:
            string_literal_list_destroy(mexp->literals);
            break;
    }
}

enum number_comparing_op_type
{
    NUMBER_COMPARING_LT,
    NUMBER_COMPARING_GT,
    NUMBER_COMPARING_LE,
    NUMBER_COMPARING_GE,
    NUMBER_COMPARING_EQ,
    NUMBER_COMPARING_NE,
};

struct number_comparing_condition
{
    enum number_comparing_op_type         op_type;
    double                                nexp;
};

enum for_clause_type {
    FOR_CLAUSE_VALUE,
    FOR_CLAUSE_KEY,
    FOR_CLAUSE_KV,
};

enum number_comparing_logical_expression_node_type
{
    NUMBER_COMPARING_LOGICAL_EXPRESSION_AND,
    NUMBER_COMPARING_LOGICAL_EXPRESSION_OR,
    NUMBER_COMPARING_LOGICAL_EXPRESSION_XOR,
    NUMBER_COMPARING_LOGICAL_EXPRESSION_NOT,
    NUMBER_COMPARING_LOGICAL_EXPRESSION_NUM,
};

struct number_comparing_logical_expression
{
    enum number_comparing_logical_expression_node_type          type;

    union {
        struct number_comparing_condition ncc;
    };

    struct pctree_node              node;
};

static inline struct number_comparing_logical_expression*
number_comparing_logical_expression_create(void)
{
    struct number_comparing_logical_expression *exp;
    exp = (struct number_comparing_logical_expression*)calloc(1, sizeof(*exp));
    return exp;
}

void number_comparing_logical_expression_reset(
        struct number_comparing_logical_expression *exp);

static inline void
number_comparing_logical_expression_destroy(
        struct number_comparing_logical_expression *exp)
{
    if (!exp)
        return;

    number_comparing_logical_expression_reset(exp);
    free(exp);
}

int
number_comparing_logical_expression_match(
        struct number_comparing_logical_expression *exp,
        const double curr, bool *match);

enum string_matching_logical_expression_node_type
{
    STRING_MATCHING_LOGICAL_EXPRESSION_AND,
    STRING_MATCHING_LOGICAL_EXPRESSION_OR,
    STRING_MATCHING_LOGICAL_EXPRESSION_XOR,
    STRING_MATCHING_LOGICAL_EXPRESSION_NOT,
    STRING_MATCHING_LOGICAL_EXPRESSION_STR,
};

struct string_matching_logical_expression
{
    enum string_matching_logical_expression_node_type          type;

    union {
        struct string_matching_condition smc;
    };

    struct pctree_node              node;
};

static inline struct string_matching_logical_expression*
string_matching_logical_expression_create(void)
{
    struct string_matching_logical_expression *exp;
    exp = (struct string_matching_logical_expression*)calloc(1, sizeof(*exp));
    return exp;
}

void string_matching_logical_expression_reset(
        struct string_matching_logical_expression *exp);

static inline void
string_matching_logical_expression_destroy(
        struct string_matching_logical_expression *exp)
{
    if (!exp)
        return;

    string_matching_logical_expression_reset(exp);
    free(exp);
}

int
string_matching_logical_expression_match(
        struct string_matching_logical_expression *exp,
        purc_variant_t curr, bool *match);

enum iterative_formula_expression_node_type
{
    ITERATIVE_FORMULA_EXPRESSION_OP,
    ITERATIVE_FORMULA_EXPRESSION_NUM,
    ITERATIVE_FORMULA_EXPRESSION_ID,
};

struct iterative_formula_expression
{
    enum iterative_formula_expression_node_type           type;
    union {
        int (*op)(struct iterative_formula_expression *);
        double  d;
        purc_variant_t key_name;
    };

    struct pctree_node                                    node;

    double                                                result;
};

static inline struct iterative_formula_expression*
iterative_formula_expression_create(void)
{
    struct iterative_formula_expression *exp;
    exp = (struct iterative_formula_expression*)calloc(1, sizeof(*exp));
    return exp;
}

void iterative_formula_expression_release(
        struct iterative_formula_expression *exp);

static inline void
iterative_formula_expression_destroy(struct iterative_formula_expression *exp)
{
    if (!exp)
        return;

    iterative_formula_expression_release(exp);
    free(exp);
}

int iterative_formula_add(struct iterative_formula_expression *exp);
int iterative_formula_sub(struct iterative_formula_expression *exp);
int iterative_formula_mul(struct iterative_formula_expression *exp);
int iterative_formula_div(struct iterative_formula_expression *exp);
int iterative_formula_neg(struct iterative_formula_expression *exp);

int iterative_formula_iterate(struct iterative_formula_expression *exp,
        purc_variant_t curr, double *result);

int
literal_expression_eval(struct literal_expression *lexp,
        purc_variant_t val, bool *result);
int
wildcard_expression_eval(struct wildcard_expression *wexp,
        purc_variant_t val, bool *result);
int
regular_expression_eval(struct regular_expression *rexp,
        purc_variant_t val, bool *result);

static inline int
string_pattern_expression_eval(struct string_pattern_expression *spexp,
        purc_variant_t val, bool *result)
{
    switch (spexp->type)
    {
        case STRING_PATTERN_WILDCARD:
            return wildcard_expression_eval(&spexp->wildcard, val, result);
        case STRING_PATTERN_REGEXP:
            return regular_expression_eval(&spexp->regexp, val, result);
    }
    return -1;
}

static inline int
string_pattern_list_eval(struct string_pattern_list *list,
        purc_variant_t val, bool *result)
{
    struct list_head *p;
    list_for_each(p, &list->list) {
        struct string_pattern_expression *lexp;
        lexp = container_of(p, struct string_pattern_expression, node);
        int r = string_pattern_expression_eval(lexp, val, result);
        if (r)
            return -1;
        if (result && *result)
            return 0;
    }

    if (result)
        *result = false;

    return 0;
}

static inline int
string_literal_list_eval(struct string_literal_list *list,
        purc_variant_t val, bool *result)
{
    struct list_head *p;
    list_for_each(p, &list->list) {
        struct literal_expression *lexp;
        lexp = container_of(p, struct literal_expression, node);
        int r = literal_expression_eval(lexp, val, result);
        if (r)
            return -1;
        if (result && *result)
            return 0;
    }

    if (result)
        *result = false;

    return 0;
}

static inline int
string_matching_condition_eval(struct string_matching_condition *mexp,
        purc_variant_t val, bool *result)
{
    switch(mexp->type)
    {
        case STRING_MATCHING_PATTERN:
            return string_pattern_list_eval(mexp->patterns, val, result);
        case STRING_MATCHING_LITERAL:
            return string_literal_list_eval(mexp->literals, val, result);
    }

    return -1;
}

int number_comparing_condition_eval(struct number_comparing_condition *ncc,
        const double curr, bool *result);

///////////////////////////////////////////////////

struct value_number_comparing_condition
{
    purc_variant_t                        key_name;
    struct number_comparing_condition     ncc;
};

struct value_number_comparing_logical_expression
{
    enum number_comparing_logical_expression_node_type          type;

    union {
        struct value_number_comparing_condition vncc;
    };

    struct pctree_node              node;
};

static inline void
vncc_release(struct value_number_comparing_condition *vncc)
{
    if (!vncc)
        return;

    if (vncc->key_name) {
        free(vncc->key_name);
        vncc->key_name = NULL;
    }
}

int vncc_match(struct value_number_comparing_condition *vncc,
        purc_variant_t curr, bool *result);

static inline struct value_number_comparing_logical_expression*
vncle_create(void)
{
    struct value_number_comparing_logical_expression *vncle;
    vncle = (struct value_number_comparing_logical_expression*)calloc(1,
            sizeof(*vncle));
    return vncle;
}

void
vncle_release(struct value_number_comparing_logical_expression *vncle);

static inline void
vncle_destroy(struct value_number_comparing_logical_expression *vncle)
{
    if (!vncle)
        return;

    vncle_release(vncle);
    free(vncle);
}

int
vncle_match(struct value_number_comparing_logical_expression *vncle,
        purc_variant_t curr, bool *match);

struct iterative_assignment_expression
{
    purc_variant_t                             key_name; // l-value
    struct iterative_formula_expression       *ife; // r-value

    struct list_head                           node;
};

struct iterative_assignment_list
{
    struct list_head              list;
};

static inline void
iae_release(struct iterative_assignment_expression* iae)
{
    if (!iae)
        return;

    if (iae->key_name != PURC_VARIANT_INVALID) {
        purc_variant_unref(iae->key_name);
        iae->key_name = PURC_VARIANT_INVALID;
    }
    if (iae->ife) {
        iterative_formula_expression_destroy(iae->ife);
        iae->ife = NULL;
    }
}

static inline void
iae_destroy(struct iterative_assignment_expression* iae)
{
    if (!iae)
        return;

    iae_release(iae);
    free(iae);
}

static inline struct iterative_assignment_expression*
iae_create(void) {
    struct iterative_assignment_expression *iae;
    iae = (struct iterative_assignment_expression*)calloc(1, sizeof(*iae));
    return iae;
}

static inline struct iterative_assignment_list*
ial_create(void)
{
    struct iterative_assignment_list *list;
    list = (struct iterative_assignment_list*)calloc(1, sizeof(*list));
    if (!list)
        return NULL;

    INIT_LIST_HEAD(&list->list);

    return list;
}

static inline void
ial_destroy(struct iterative_assignment_list *list)
{
    if (!list)
        return;

    struct list_head *p, *n;
    list_for_each_safe(p, n, &list->list) {
        struct iterative_assignment_expression *iae;
        iae = container_of(p, struct iterative_assignment_expression, node);
        list_del(p);
        iae_destroy(iae);
    }

    free(list);
}

static inline int
iae_iterate(struct iterative_assignment_expression *iae, purc_variant_t curr)
{
    double result;
    int r = iterative_formula_iterate(iae->ife, curr, &result);
    if (r)
        return r;

    purc_variant_t v = purc_variant_make_number(result);
    if (v == PURC_VARIANT_INVALID)
        return -1;

    purc_variant_t k = iae->key_name;
    bool ok = purc_variant_object_set(curr, k, v);
    purc_variant_unref(v);

    return ok ? 0 : -1;
}

static inline int
ial_iterate(struct iterative_assignment_list *ial, purc_variant_t curr)
{
    int r = 0;
    struct list_head *p;
    list_for_each(p, &ial->list) {
        struct iterative_assignment_expression *iae;
        iae = container_of(p, struct iterative_assignment_expression, node);
        r = iae_iterate(iae, curr);
        if (r)
            break;
    }

    return r;
}

PCA_EXTERN_C_END

#endif // PURC_EXECUTOR_PCEXE_HELPER_H

