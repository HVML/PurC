/*
 * @file pcexe-helper.c
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

#include "pcexe-helper.h"

#include "private/debug.h"
#include "private/errors.h"
#include "private/variant.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#if HAVE(GLIB)
#include <glib.h>
#endif // HAVE(GLIB)

static inline const char*
stringify(purc_variant_t val, purc_variant_t *vs)
{
    purc_variant_t v = purc_variant_stringify(val);
    if (v == PURC_VARIANT_INVALID)
        return NULL;

    const char *s = NULL;
    if (purc_variant_is_atomstring(v)) {
        s = purc_variant_get_atom_string_const(v);
    }
    if (purc_variant_is_string(v)) {
        s = purc_variant_get_string_const(v);
    }

    if (s == NULL) {
        purc_variant_unref(v);
        return NULL;
    }

    *vs = v;
    return s;
}
static inline char*
stringify_and_strdup(purc_variant_t val)
{
    purc_variant_t v = purc_variant_stringify(val);
    if (v == PURC_VARIANT_INVALID)
        return NULL;

    char *s = NULL;
    if (purc_variant_is_atomstring(v)) {
        s = strdup(purc_variant_get_atom_string_const(v));
    }
    if (purc_variant_is_string(v)) {
        s = strdup(purc_variant_get_string_const(v));
    }

    purc_variant_unref(v);

    return s;
}

int pcexe_unitoutf8(char *utf8, const char *uni, size_t n)
{
    if (n != 6)
        return -1;

    uni += 2;
    n -= 2;

    char buf[7];
    strncpy(buf, uni, n);
    buf[4] = '\0';

    char *end;
    unsigned long int r = strtoul(buf, &end, 16);
    if (end && *end)
        return -1;

    // FIXME: big-endian?
    if (r < 0x80) {
        utf8[0] = (char)r;
        utf8[1] = '\0';
        return 0;
    }

    if (r < 0x800) {
        utf8[0] = ((r >> 6) & 0b11111) | 0b11000000;
        utf8[1] = (r & 0b111111) | 0b10000000;
        utf8[2] = '\0';
        return 0;
    }

    if (r < 0xD800 || ((r >= 0xE000) && (r <= 0xFFFF))) {
        utf8[0] = ((r >> 12) & 0b1111) | 0b11100000;
        utf8[1] = ((r >> 6) & 0b111111) | 0b10000000;
        utf8[2] = (r & 0b111111) | 0b10000000;
        utf8[3] = '\0';
        return 0;
    }

    if (r >= 0x10000 && r <= 0x10FFFF) {
        utf8[0] = ((r >> 18) & 0b111) | 0b11110000;
        utf8[1] = ((r >> 12) & 0b111111) | 0b10000000;
        utf8[2] = ((r >> 6) & 0b111111) | 0b10000000;
        utf8[3] = (r & 0b111111) | 0b10000000;
        utf8[4] = '\0';
        return 0;
    }

    return -1;
}

struct pcexe_strlist* pcexe_strlist_create(void)
{
    struct pcexe_strlist *list = (struct pcexe_strlist*)calloc(1, sizeof(*list));
    if (!list)
        return NULL;
    return list;
}

void pcexe_strlist_destroy(struct pcexe_strlist *list)
{
    if (!list)
        return;
    pcexe_strlist_reset(list);
    free(list);
}

void pcexe_strlist_reset(struct pcexe_strlist *list)
{
    if (!list)
        return;

    for (size_t i=0; i<list->size; ++i) {
        char *str = list->strings[i];
        free(str);
    }

    free(list->strings);
    list->strings = NULL;
    list->size = 0;
    list->capacity = 0;
}

int
pcexe_strlist_append_buf(struct pcexe_strlist *list, const char *buf, size_t len)
{
    if (list->size >= list->capacity) {
        size_t n = (list->size + 15) / 8 * 8;
        char **p = (char**)realloc(list->strings, n * sizeof(*p));
        if (!p)
            return -1;
        list->capacity = n;
        list->strings = p;
    }

    char *p = malloc(len+1);
    if (!p)
        return -1;
    memcpy(p, buf, len);
    p[len] = '\0';

    list->strings[list->size++] = p;

    return 0;
}

char* pcexe_strlist_to_str(struct pcexe_strlist *list)
{
    if (!list || !list->strings)
        return NULL;

    size_t n = 0;
    for (size_t i=0; i<list->size; ++i) {
        n += strlen(list->strings[i]);
    }

    char *buf = (char*)malloc(n+1);
    if (!buf)
        return NULL;

    buf[0] = '\0';
    for (size_t i=0; i<list->size; ++i) {
        strcat(buf, list->strings[i]);
    }

    return buf;
}

purc_variant_t
pcexe_cache_array(purc_variant_t input, bool asc_desc)
{
    size_t sz = purc_variant_array_get_size(input);

    purc_variant_t cache = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (cache == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    bool ok = true;
    for (size_t i=0; i<sz; ++i) {
        purc_variant_t v = purc_variant_array_get(input, i);
        PC_ASSERT(v != PURC_VARIANT_INVALID);
        if (asc_desc) {
            ok = purc_variant_array_append(cache, v);
        } else {
            ok = purc_variant_array_prepend(cache, v);
        }
        if (!ok)
            break;
    }

    if (!ok) {
        purc_variant_unref(cache);
        return PURC_VARIANT_INVALID;
    }

    return cache;
}

purc_variant_t
pcexe_cache_object(purc_variant_t input, bool asc_desc)
{
    purc_variant_t cache = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (cache == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    bool ok = true;
    purc_variant_t k, v;
    foreach_key_value_in_variant_object(input, k, v)
        purc_variant_t o = purc_variant_make_object(1, k, v);
        if (o == PURC_VARIANT_INVALID) {
            ok = false;
            break;
        }
        if (asc_desc) {
            ok = purc_variant_array_append(cache, o);
        } else {
            ok = purc_variant_array_prepend(cache, o);
        }
    end_foreach;

    if (!ok) {
        purc_variant_unref(cache);
        return PURC_VARIANT_INVALID;
    }

    return cache;
}

purc_variant_t
pcexe_cache_set(purc_variant_t input, bool asc_desc)
{
    purc_variant_t cache = purc_variant_make_array(0, PURC_VARIANT_INVALID);
    if (cache == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    bool ok = true;
    purc_variant_t v;
    foreach_value_in_variant_set(input, v)
        if (asc_desc) {
            ok = purc_variant_array_append(cache, v);
        } else {
            ok = purc_variant_array_prepend(cache, v);
        }
    end_foreach;

    if (!ok) {
        purc_variant_unref(cache);
        return PURC_VARIANT_INVALID;
    }

    return cache;
}

struct logical_expression* logical_expression_all(void)
{
    static struct logical_expression all = {0};
    return &all;
}

int is_logical_expression_all(struct logical_expression *lexp)
{
    if (logical_expression_all() == lexp)
        return 1;
    return 0;
}

void logical_expression_reset(struct logical_expression *exp)
{
    if (!exp)
        return;

    if (is_logical_expression_all(exp))
        return;

    struct pctree_node *top = &exp->node;
    struct pctree_node *node, *next;
    pctree_for_each_post_order(top, node, next) {
        struct logical_expression *p;
        p = container_of(node, struct logical_expression, node);
        pctree_node_remove(node);
        switch (p->type)
        {
            case LOGICAL_EXPRESSION_OP:
                break;
            case LOGICAL_EXPRESSION_STR:
                string_matching_expression_reset(&p->mexp);
                break;
            case LOGICAL_EXPRESSION_NUM:
                break;
        }
        if (p!=exp)
            free(p);
    }
}

int number_comparing_condition_eval(struct number_comparing_condition *ncc,
        purc_variant_t val, bool *result)
{
    double v = purc_variant_numberify(val);

    switch (ncc->op_type)
    {
        case NUMBER_COMPARING_LT:
            *result = v < ncc->nexp;
            return 0;
        case NUMBER_COMPARING_GT:
            *result = v > ncc->nexp;
            return 0;
        case NUMBER_COMPARING_LE:
            *result = v <= ncc->nexp;
            return 0;
        case NUMBER_COMPARING_GE:
            *result = v >= ncc->nexp;
            return 0;
        case NUMBER_COMPARING_EQ:
            *result = v == ncc->nexp;
            return 0;
        case NUMBER_COMPARING_NE:
            *result = v != ncc->nexp;
            return 0;
        default:
            return -1;
    }
}

int logical_expression_eval(struct logical_expression *exp,
        purc_variant_t val, bool *result)
{
    if (is_logical_expression_all(exp)) {
        exp->result = true;
        if (result)
            *result = exp->result;
        return 0;
    }

    struct pctree_node *top = &exp->node;
    struct pctree_node *node, *next;
    pctree_for_each_post_order(top, node, next) {
        struct logical_expression *p;
        p = container_of(node, struct logical_expression, node);

        int r = 0;
        switch (p->type)
        {
            case LOGICAL_EXPRESSION_OP:
            {
                PC_ASSERT(p->op);
                r = p->op(p);
            } break;
            case LOGICAL_EXPRESSION_STR:
            {
                struct string_matching_expression *mexp;
                mexp = &p->mexp;
                r = string_matching_expression_eval(mexp, val, &p->result);
            } break;
            case LOGICAL_EXPRESSION_NUM:
            {
                struct number_comparing_condition *ncc;
                ncc = &p->ncc;
                r = number_comparing_condition_eval(ncc, val, &p->result);
            } break;
        }

        if (r)
            return r;
    }

    if (result)
        *result = exp->result;

    return 0;
}

int logical_and(struct logical_expression *exp)
{
    size_t nr = pctree_node_children_number(&exp->node);
    if (nr != 2)
        return -1;
    struct logical_expression *l, *r;
    l = container_of(exp->node.first_child, struct logical_expression, node);
    r = container_of(exp->node.last_child, struct logical_expression, node);

    exp->result = l->result && r->result;

    return 0;
}

int logical_or(struct logical_expression *exp)
{
    size_t nr = pctree_node_children_number(&exp->node);
    if (nr != 2)
        return -1;
    struct logical_expression *l, *r;
    l = container_of(exp->node.first_child, struct logical_expression, node);
    r = container_of(exp->node.last_child, struct logical_expression, node);

    exp->result = l->result || r->result;

    return 0;
}

int logical_xor(struct logical_expression *exp)
{
    size_t nr = pctree_node_children_number(&exp->node);
    if (nr != 2)
        return -1;

    struct logical_expression *l, *r;
    l = container_of(exp->node.first_child, struct logical_expression, node);
    r = container_of(exp->node.last_child, struct logical_expression, node);

    exp->result = l->result ^ r->result;

    return 0;
}

int logical_not(struct logical_expression *exp)
{
    size_t nr = pctree_node_children_number(&exp->node);
    if (nr != 1)
        return -1;

    struct logical_expression *l;
    l = container_of(exp->node.first_child, struct logical_expression, node);

    exp->result = !l->result;

    return 0;
}

void
string_pattern_expression_reset(struct string_pattern_expression *spexp)
{
    if (!spexp)
        return;
    switch (spexp->type)
    {
        case STRING_PATTERN_WILDCARD:
            free(spexp->wildcard.wildcard);
            if (spexp->wildcard.pattern_spec) {
                GPatternSpec* ps = spexp->wildcard.pattern_spec;
                g_pattern_spec_free(ps);
                spexp->wildcard.pattern_spec = NULL;
            }
            break;
        case STRING_PATTERN_REGEXP:
            free(spexp->regexp.regexp);
            if (spexp->regexp.reg_valid) {
                regfree(&spexp->regexp.reg);
                spexp->regexp.reg_valid = 0;
            }
            break;
    }
}

static inline void
normalize_space(char *s)
{
    size_t n = strlen(s);
    for (size_t i=0; i<n; ++i) {
        const char c = s[i];
        if (isspace(c)) {
            s[i] = ' ';
        }
    }
}

static inline void
normalize_toupper(char *s)
{
    size_t n = strlen(s);
    for (size_t i=0; i<n; ++i) {
        const char c = s[i];
        s[i] = toupper(c);
    }
}

static inline void
compress_spaces(char *s)
{
    size_t n = strlen(s);
    if (n==0)
        return;

    if (isspace(*s)) {
        *s = ' ';
    }

    if (n==1)
        return;

    char *t = s;

    for (size_t i=1; i<n; ++i) {
        const char c = s[i];
        if (isspace(c)) {
            if (*t == ' ') {
                continue;
            }
        }
        *++t = c;
    }

    *++t = '\0';
}

int
literal_expression_eval(struct literal_expression *lexp,
        purc_variant_t val, bool *result)
{
    int r = -1;
    int (*cmp)(const char *s1, const char *s2, size_t n) = NULL;
    char *literal = NULL;
    char *target  = NULL;
    size_t n = 0;

    if (MATCHING_FLAGS_IS_SET_WITH(lexp->suffix.matching_flags, MATCHING_FLAG_C)) {
        if (!literal) {
            literal = strdup(lexp->literal);
            if (!literal)
                goto end;
        }
        if (!target) {
            target = stringify_and_strdup(val);
            if (!target)
                goto end;
        }

        compress_spaces(literal);
        compress_spaces(target);
    }

    if (MATCHING_FLAGS_IS_SET_WITH(lexp->suffix.matching_flags, MATCHING_FLAG_S)) {
        if (!literal) {
            literal = strdup(lexp->literal);
            if (!literal)
                goto end;
        }
        if (!target) {
            target = stringify_and_strdup(val);
            if (!target)
                goto end;
        }


        normalize_space(literal);
        normalize_space(target);
    }

    if (MATCHING_FLAGS_IS_SET_WITH(lexp->suffix.matching_flags, MATCHING_FLAG_I)) {
        cmp = strncasecmp;
    } else {
        cmp = strncmp;
    }

    if (lexp->suffix.max_matching_length > 0) {
        n = lexp->suffix.max_matching_length;
    } else {
        if (literal) {
            n = strlen(literal);
        } else {
            n = strlen(lexp->literal);
        }
    }

    if (literal && target) {
        r = cmp(literal, target, n);
    } else {
        purc_variant_t v;
        const char *s = stringify(val, &v);
        if (s) {
            r = cmp(lexp->literal, s, n);
            purc_variant_unref(v);
        } else {
            r = -1;
        }
    }

    if (result)
        *result = r == 0 ? true : false;

    r = 0;

end:
    free(literal);
    free(target);

    return r ? -1 : 0;
}

static inline int
wildcard_expression_init_pattern_spec(struct wildcard_expression *wexp)
{
    GPatternSpec *ps = NULL;
    char *wildcard = NULL;

    if (MATCHING_FLAGS_IS_SET_WITH(wexp->suffix.matching_flags, MATCHING_FLAG_C)) {
        if (!wildcard) {
            wildcard = strdup(wexp->wildcard);
            if (!wildcard)
                goto end;
        }

        compress_spaces(wildcard);
    }

    if (MATCHING_FLAGS_IS_SET_WITH(wexp->suffix.matching_flags, MATCHING_FLAG_S)) {
        if (!wildcard) {
            wildcard = strdup(wexp->wildcard);
            if (!wildcard)
                goto end;
        }

        normalize_space(wildcard);
    }

    if (MATCHING_FLAGS_IS_SET_WITH(wexp->suffix.matching_flags, MATCHING_FLAG_I)) {
        if (!wildcard) {
            wildcard = strdup(wexp->wildcard);
            if (!wildcard)
                goto end;
        }
        normalize_toupper(wildcard);
    }

    if (wexp->suffix.max_matching_length > 0) {
        size_t n = wexp->suffix.max_matching_length;
        if (!wildcard) {
            size_t len = strlen(wexp->wildcard);
            if (len > n) {
                wildcard = strndup(wexp->wildcard, n);
                if (!wildcard)
                    goto end;
            } else {
                n = len;
            }
        } else {
            size_t len = strlen(wildcard);
            if (len > n) {
                wildcard[n] = '\0';
            } else {
                n = len;
            }
        }
    }

    if (wildcard) {
        ps = g_pattern_spec_new(wildcard);
        if (!ps)
            goto end;
    } else {
        ps = g_pattern_spec_new(wexp->wildcard);
        if (!ps)
            goto end;
    }

    wexp->pattern_spec = ps;

end:
    free(wildcard);

    return ps ? 0 : -1;
}

int
wildcard_expression_eval(struct wildcard_expression *wexp,
        purc_variant_t val, bool *result)
{
    GPatternSpec *ps = NULL;
    char *target  = NULL;
    const char *t = NULL;

    purc_variant_t v = PURC_VARIANT_INVALID;
    const char *s = stringify(val, &v);
    if (!s)
        return -1;

    int r = -1;
    int rr = 0;

    if (wexp->pattern_spec == NULL) {
        if (wildcard_expression_init_pattern_spec(wexp))
            goto end;
    }

    ps = (GPatternSpec*)wexp->pattern_spec;

    if (MATCHING_FLAGS_IS_SET_WITH(wexp->suffix.matching_flags, MATCHING_FLAG_C)) {
        if (!target) {
            target = strdup(s);
            if (!target)
                goto end;
        }

        compress_spaces(target);
    }

    if (MATCHING_FLAGS_IS_SET_WITH(wexp->suffix.matching_flags, MATCHING_FLAG_S)) {
        if (!target) {
            target = strdup(s);
            if (!target)
                goto end;
        }

        normalize_space(target);
    }

    if (MATCHING_FLAGS_IS_SET_WITH(wexp->suffix.matching_flags, MATCHING_FLAG_I)) {
        if (!target) {
            target = strdup(s);
            if (!target)
                goto end;
        }

        normalize_toupper(target);
    }

    if (wexp->suffix.max_matching_length > 0) {
        size_t n = wexp->suffix.max_matching_length;
        if (!target) {
            size_t len = strlen(s);
            if (len > n) {
                target = strndup(s, n);
                if (!target) {
                    purc_variant_unref(v);
                    goto end;
                }
            } else {
                n = len;
            }
        } else {
            size_t len = strlen(target);
            if (len > n) {
                target[n] = '\0';
            } else {
                n = len;
            }
        }
    }

    r = 0;

    if (target) {
        t = target;
    } else {
        t = s;
    }

    rr = g_pattern_match(ps, strlen(t), t, NULL);

    if (result)
        *result = (rr) ? true : false;
end:
    free(target);
    purc_variant_unref(v);

    return r ? -1 : 0;
}

static inline int
regular_expression_init_reg(struct regular_expression *rexp)
{
    int cflags = REG_EXTENDED | REG_NOSUB; /* REG_NEWLINE */
    int eflags = 0; /* REG_NOTBOL | REG_NOTEOL */

    if (REGEXP_FLAGS_IS_SET_WITH(rexp->flags, REGEXP_FLAG_I)) {
        cflags |= REG_ICASE;
    }

    if (REGEXP_FLAGS_IS_SET_WITH(rexp->flags, REGEXP_FLAG_S)) {
        cflags |= REG_NEWLINE;
    }

    if (REGEXP_FLAGS_IS_SET_WITH(rexp->flags, REGEXP_FLAG_M)) {
        eflags &= ~(REG_NOTBOL|REG_NOTEOL);
    }

    // TODO: other flags

    int r = regcomp(&rexp->reg, rexp->regexp, cflags);
    if (r)
        return -1;

    rexp->eflags = eflags;
    rexp->reg_valid = 1;

    return 0;
}

int
regular_expression_eval(struct regular_expression *rexp,
        purc_variant_t val, bool *result)
{
    int r = -1;
    int v = 0;
    purc_variant_t vs;
    const char *s = stringify(val, &vs);
    if (s == NULL)
        return -1;

    if (!rexp->reg_valid) {
        if (regular_expression_init_reg(rexp))
            goto end;
    }

    r = 0;
    v = regexec(&rexp->reg, s, 0, NULL, rexp->eflags);

    if (result)
        *result = (v == 0) ? true : false;

end:
    purc_variant_unref(vs);

    return r ? -1 : 0;
}

