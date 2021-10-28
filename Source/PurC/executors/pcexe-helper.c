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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#if HAVE(GLIB)
#include <glib.h>
#endif // HAVE(GLIB)

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

void logical_expression_reset(struct logical_expression *exp)
{
    if (!exp)
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
            case LOGICAL_EXPRESSION_EXP:
                string_matching_expression_reset(&p->mexp);
                break;
        }
        if (p!=exp)
            free(p);
    }
}

static int
eval_by_matching_literal(struct literal_expression *lexp,
    struct logical_expression *exp, purc_variant_t val)
{
    UNUSED_PARAM(lexp);
    UNUSED_PARAM(exp);
    UNUSED_PARAM(val);
    return -1;
}

static int
eval_by_matching_literals(struct logical_expression *exp,
    purc_variant_t val)
{
    struct list_head *p;
    list_for_each(p, &exp->mexp.literals->list) {
        struct literal_expression *lexp;
        lexp = container_of(p, struct literal_expression, node);
        int r = eval_by_matching_literal(lexp, exp, val);
        if (r)
            return r;
        if (!exp->result)
            return 0;
    }

    return true;
}

static int
eval_by_matching_wildcard(struct wildcard_expression *wildcard,
    struct logical_expression *exp, purc_variant_t val)
{
    UNUSED_PARAM(wildcard);
    UNUSED_PARAM(exp);
    UNUSED_PARAM(val);
    return -1;
}

static int
eval_by_matching_regexp(struct regular_expression *regexp,
    struct logical_expression *exp, purc_variant_t val)
{
    UNUSED_PARAM(regexp);
    UNUSED_PARAM(exp);
    UNUSED_PARAM(val);
    return -1;
}

static int
eval_by_matching_pattern(struct string_pattern_expression *pexp,
    struct logical_expression *exp, purc_variant_t val)
{
    switch (pexp->type)
    {
        case STRING_PATTERN_WILDCARD:
        {
            return eval_by_matching_wildcard(&pexp->wildcard, exp, val);
        } break;
        case STRING_PATTERN_REGEXP:
        {
            return eval_by_matching_regexp(&pexp->regexp, exp, val);
        } break;
    }

    return -1;
}

static int
eval_by_matching_patterns(struct logical_expression *exp,
    purc_variant_t val)
{
    struct list_head *p;
    list_for_each(p, &exp->mexp.literals->list) {
        struct string_pattern_expression *pexp;
        pexp = container_of(p, struct string_pattern_expression, node);
        int r = eval_by_matching_pattern(pexp, exp, val);
        if (r)
            return r;
        if (!exp->result)
            return 0;
    }

    return true;
}

static int
eval_by_matching_expression(struct logical_expression *exp,
    purc_variant_t val)
{
    switch (exp->mexp.type)
    {
        case STRING_MATCHING_PATTERN:
        {
            return eval_by_matching_patterns(exp, val);
        } break;
        case STRING_MATCHING_LITERAL:
        {
            return eval_by_matching_literals(exp, val);
        } break;
        default:
        {
            return -1;
        } break;
    }
}

int logical_expression_eval(struct logical_expression *exp,
        purc_variant_t val, bool *result)
{
    if (!exp)
        return -1;

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
                r = p->op(p);
            } break;
            case LOGICAL_EXPRESSION_EXP:
            {
                r = eval_by_matching_expression(p, val);
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
literal_expression_eval(struct literal_expression *lexp, const char *s,
    bool *result)
{
    if (!lexp || !s)
        return -1;

    int r = -1;
    int (*cmp)(const char *s1, const char *s2, size_t n) = NULL;
    char *literal = NULL;
    char *target  = NULL;
    size_t n = 0;

    if (MATCHING_FLAGS_IS_SET_WITH(lexp->suffix.matching_flags, MATCHING_FLAG_I)) {
        cmp = strncasecmp;
    } else {
        cmp = strncmp;
    }

    if (MATCHING_FLAGS_IS_SET_WITH(lexp->suffix.matching_flags, MATCHING_FLAG_S)) {
        literal = strdup(lexp->literal);
        target  = strdup(s);
        if (!literal || !target)
            goto end;

        normalize_space(literal);
        normalize_space(target);
    }

    if (MATCHING_FLAGS_IS_SET_WITH(lexp->suffix.matching_flags, MATCHING_FLAG_C)) {
        if (!literal)
            literal = strdup(lexp->literal);
        if (!target)
            target = strdup(s);
        if (!literal || !target)
            goto end;

        compress_spaces(literal);
        compress_spaces(target);
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
        r = cmp(lexp->literal, s, n);
    }

    if (result)
        *result = r == 0 ? true : false;

    r = 0;

end:
    free(literal);
    free(target);

    return r ? -1 : 0;
}

