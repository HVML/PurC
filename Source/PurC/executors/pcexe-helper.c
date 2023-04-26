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

#include "purc-errors.h"
#include "private/debug.h"
#include "private/errors.h"
#include "private/variant.h"

#include <stdio.h>
#include <stdlib.h>

#if HAVE(GLIB)
#include <glib.h>
#endif // HAVE(GLIB)

#define BUF_SIZE           8192

int pcexe_ucs2utf8(char *utf8, const char *uni, size_t n)
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

int pcexe_utf8_to_wchar(const char *utf8, wchar_t *wc)
{
    unsigned int codepoint;
    const char *p = utf8;
    if (*p == 0) {
        *wc = 0;
        return 0;
    }

    int tails = 0;

    unsigned char ch = (unsigned char)(*p);
    if (ch < 0b10000000) {
        codepoint = ch;
    }
    else if (ch < 0b11000000) {
        return -1;
    }
    else if (ch < 0b11100000) {
        codepoint = ch & 0b00011111;
        tails = 1;
    }
    else if (ch < 0b11110000) {
        codepoint = (ch & 0b00001111);
        tails = 2;
    }
    else if (ch < 0b11111000) {
        codepoint = ch & 0b00000111;
        tails = 3;
    }
    else if (ch < 0b11111100) {
        codepoint = ch & 0b00000011;
        tails = 4;
    }
    else if (ch < 0b11111110) {
        codepoint = ch & 0b00000001;
        tails = 5;
    }
    else {
        return -1;
    }

    ++p;
    for (int i=0; i<tails; ++i) {
        unsigned char ch = (unsigned char)(*p++);
        if ((ch & 0b10000000) == 0b10000000) {
            codepoint <<= 6;
            codepoint |= (ch & 0b00111111);
        } else {
            return -1;
        }
    }
    *wc = codepoint;

    return tails + 1;
}

int pcexe_wchar_to_utf8(const wchar_t wc, char *utf8, size_t n)
{
    if (wc < 0x80) {
        if (n < 1)
            return -1;
        utf8[0] = wc;
        return 1;
    }

    if (wc < 0x800) {
        if (n < 2)
            return -2;

        utf8[0] = 0b11000000 | ((wc >> 6) & 0b00011111);
        utf8[1] = 0b10000000 | (wc & 0b00111111);
        return 2;
    }

    if ((wc < 0xd800) || ((wc >= 0xe000) && (wc < 0x010000))) {
        if (n < 3)
            return -3;

        utf8[0] = 0b11100000 | ((wc >> 12) & 0b00001111);
        utf8[1] = 0b10000000 | ((wc >> 6) & 0b00111111);
        utf8[2] = 0b10000000 | (wc & 0b00111111);
        return 3;
    }

    if ((wc >= 0x010000) && (wc < 0x110000)) {
        if (n < 4)
            return -4;

        utf8[0] = 0b11110000 | ((wc >> 18) & 0b00000111);
        utf8[1] = 0b10000000 | ((wc >> 12) & 0b00111111);
        utf8[2] = 0b10000000 | ((wc >> 6) & 0b00111111);
        utf8[3] = 0b10000000 | (wc & 0b00111111);
        return 4;
    }

    return 0; // denote error
}

wchar_t* pcexe_wchar_from_utf8(const char *utf8, size_t *bytes, size_t *chars)
{
    size_t len = strlen(utf8);
    size_t sz = len * sizeof(wchar_t);
    wchar_t *ws = (wchar_t*)malloc(sz + sizeof(wchar_t));
    *bytes = 0;
    *chars = 0;
    if (!ws)
        return NULL;

    const char *p = utf8;

    size_t nc = 0;
    size_t bc = 0;
    while (*p) {
        int n = pcexe_utf8_to_wchar(p, ws + nc);
        if (n < 0)
            break;

        ++nc;
        bc += n;
        p  += n;
    }

    *bytes = bc;
    *chars = nc;
    ws[nc] = 0;
    return ws;
}

char* pcexe_utf8_from_wchar(const wchar_t *ws, size_t *chars, size_t *bytes)
{
    size_t len = wcslen(ws);
    size_t sz = len * 6;
    char *utf8 = (char*)malloc(sz + sizeof(char));
    *chars = 0;
    *bytes = 0;
    if (!utf8)
        return NULL;

    const wchar_t *s = ws;
    char *d = utf8;

    size_t nc = 0;
    size_t bc = 0;
    while (*s) {
        int n = pcexe_wchar_to_utf8(*s, d, sz - bc);
        if (n == 0)
            break;
        PC_ASSERT(n > 0);

        ++nc;
        bc += n;
        d  += n;
        ++s;
    }

    *bytes = bc;
    *chars = nc;
    utf8[bc] = '\0';
    return utf8;
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

static inline purc_variant_t
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

static inline purc_variant_t
pcexe_cache_object(purc_variant_t input, bool asc_desc)
{
    UNUSED_PARAM(asc_desc);

    purc_variant_t cache = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (cache == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    bool ok = true;
    purc_variant_t k, v;
    foreach_key_value_in_variant_object(input, k, v)
        ok = purc_variant_object_set(cache, k, v);
        if (!ok)
            break;
    end_foreach;

    if (!ok) {
        purc_variant_unref(cache);
        return PURC_VARIANT_INVALID;
    }

    return cache;
}

static inline char*
make_unique_key(variant_set_t data)
{
    PC_ASSERT(data->unique_key);

    size_t sz = 0;
    for (size_t i=0; i<data->nr_keynames; ++i) {
        if (i)
            sz += 1;
        sz += strlen(data->keynames[i]);
    }

    char *s = (char*)malloc(sz + 1);
    if (!s)
        return NULL;

    char *p = s;
    for (size_t i=0; i<data->nr_keynames; ++i) {
        if (i) {
            *p++ = ' ';
        }
        strcpy(p, data->keynames[i]);
        p += strlen(p);
    }

    return s;
}

static inline purc_variant_t
pcexe_cache_set(purc_variant_t input, bool asc_desc)
{
    UNUSED_PARAM(asc_desc);

    variant_set_t data = (variant_set_t)input->sz_ptr[1];
    char *unique_key = NULL;
    if (data->unique_key) {
        unique_key = make_unique_key(data);
        if (!unique_key)
            return PURC_VARIANT_INVALID;
    }

    purc_variant_t cache = purc_variant_make_set_by_ckey(0,
            unique_key, PURC_VARIANT_INVALID);
    if (cache == PURC_VARIANT_INVALID) {
        if (unique_key) {
            free(unique_key);
            unique_key = NULL;
        }

        return PURC_VARIANT_INVALID;
    }

    ssize_t r;
    purc_variant_t v;
    foreach_value_in_variant_set(input, v)
        r = purc_variant_set_add(cache, v, PCVRNT_CR_METHOD_COMPLAIN);
        if (r == -1)
            break;
    end_foreach;

    if (unique_key) {
        free(unique_key);
        unique_key = NULL;
    }

    if (r == -1) {
        purc_variant_unref(cache);
        return PURC_VARIANT_INVALID;
    }

    return cache;
}

purc_variant_t
pcexe_make_cache(purc_variant_t input, bool asc_desc)
{
    purc_variant_t cache;
    enum purc_variant_type vt = purc_variant_get_type(input);
    switch (vt)
    {
        case PURC_VARIANT_TYPE_OBJECT:
        {
            cache = pcexe_cache_object(input, asc_desc);
        } break;
        case PURC_VARIANT_TYPE_ARRAY:
        {
            cache = pcexe_cache_array(input, asc_desc);
        } break;
        case PURC_VARIANT_TYPE_SET:
        {
            cache = pcexe_cache_set(input, asc_desc);
        } break;
        case PURC_VARIANT_TYPE_DYNAMIC:
        {
            pcinst_set_error(PCEXECUTOR_ERROR_NOT_IMPLEMENTED);
            return PURC_VARIANT_INVALID;
        } break;
        case PURC_VARIANT_TYPE_NATIVE:
        {
            pcinst_set_error(PCEXECUTOR_ERROR_NOT_IMPLEMENTED);
            return PURC_VARIANT_INVALID;
        } break;
        default:
        {
            purc_variant_ref(input);
            return input;
        }
    }

    return cache;
}

int number_comparing_condition_eval(struct number_comparing_condition *ncc,
        const double curr, bool *result)
{
    double v = curr;

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

void
number_comparing_logical_expression_reset(
        struct number_comparing_logical_expression *exp)
{
    if (!exp)
        return;

    struct pctree_node *top = &exp->node;
    struct pctree_node *node, *next;
    pctree_for_each_post_order(top, node, next) {
        struct number_comparing_logical_expression *p;
        p = container_of(node,
                struct number_comparing_logical_expression, node);
        pctree_node_remove(node);
        if (p!=exp)
            free(p);
    }
}

static inline void
ncle_get_children(struct number_comparing_logical_expression *exp,
        struct number_comparing_logical_expression **l,
        struct number_comparing_logical_expression **r)
{
    struct pctree_node *node = &exp->node;
    size_t nr = pctree_node_children_number(node);
    struct pctree_node *n;
    PC_ASSERT(nr<=2);
    if (nr == 0)
        return;

    n = pctree_node_child(node);
    PC_ASSERT(n);
    *l = container_of(n,
            struct number_comparing_logical_expression, node);
    if (nr == 1)
        return;

    n = pctree_node_next(n);
    PC_ASSERT(n);
    *r = container_of(n,
            struct number_comparing_logical_expression, node);
}

int
number_comparing_logical_expression_match(
        struct number_comparing_logical_expression *exp,
        const double curr, bool *match)
{
    struct number_comparing_logical_expression *l = NULL, *r = NULL;
    ncle_get_children(exp, &l, &r);

    switch (exp->type)
    {
        case NUMBER_COMPARING_LOGICAL_EXPRESSION_AND:
        {
            PC_ASSERT(l && r);
            if (number_comparing_logical_expression_match(l, curr, match))
                return -1;
            if (*match == false)
                return 0;
            return number_comparing_logical_expression_match(r, curr, match);
        } break;
        case NUMBER_COMPARING_LOGICAL_EXPRESSION_OR:
        {
            PC_ASSERT(l && r);
            if (number_comparing_logical_expression_match(l, curr, match))
                return -1;
            if (*match == true)
                return 0;
            return number_comparing_logical_expression_match(r, curr, match);
        } break;
        case NUMBER_COMPARING_LOGICAL_EXPRESSION_XOR:
        {
            PC_ASSERT(l && r);
            bool a, b;
            if (number_comparing_logical_expression_match(l, curr, &a))
                return -1;
            if (number_comparing_logical_expression_match(r, curr, &b))
                return -1;
            *match =(a != b);
            return 0;
        } break;
        case NUMBER_COMPARING_LOGICAL_EXPRESSION_NOT:
        {
            PC_ASSERT(l && !r);
            int r = number_comparing_logical_expression_match(l, curr, match);
            if (r)
                return r;
            *match = !*match;
            return 0;
        } break;
        case NUMBER_COMPARING_LOGICAL_EXPRESSION_NUM:
        {
            PC_ASSERT(!l && !r);
            struct number_comparing_condition *ncc = &exp->ncc;
            return number_comparing_condition_eval(ncc, curr, match);
        } break;
    }
    PC_ASSERT(0);
    return -1;
}

static inline void
vncle_get_children(struct value_number_comparing_logical_expression *exp,
        struct value_number_comparing_logical_expression **l,
        struct value_number_comparing_logical_expression **r)
{
    struct pctree_node *node = &exp->node;
    size_t nr = pctree_node_children_number(node);
    struct pctree_node *n;
    PC_ASSERT(nr<=2);
    if (nr == 0)
        return;

    n = pctree_node_child(node);
    PC_ASSERT(n);
    *l = container_of(n,
            struct value_number_comparing_logical_expression, node);
    if (nr == 1)
        return;

    n = pctree_node_next(n);
    PC_ASSERT(n);
    *r = container_of(n,
            struct value_number_comparing_logical_expression, node);
}

int vncc_match(struct value_number_comparing_condition *vncc,
        purc_variant_t curr, bool *result)
{
    purc_variant_t k = vncc->key_name;
    PC_ASSERT(k != PURC_VARIANT_INVALID); // FIXME: error code or exception???
    PC_ASSERT(curr != PURC_VARIANT_INVALID); // FIXME: error code or exception???

    purc_variant_t v = purc_variant_object_get(curr, k);
    PC_ASSERT(v != PURC_VARIANT_INVALID); // FIXME: error code or exception???
    double d = purc_variant_numerify(v);

    struct number_comparing_condition *ncc = &vncc->ncc;

    return number_comparing_condition_eval(ncc, d, result);
}

int
vncle_match(struct value_number_comparing_logical_expression *vncle,
        purc_variant_t curr, bool *match)
{
    struct value_number_comparing_logical_expression *l = NULL, *r = NULL;
    vncle_get_children(vncle, &l, &r);

    switch (vncle->type)
    {
        case NUMBER_COMPARING_LOGICAL_EXPRESSION_AND:
        {
            PC_ASSERT(l && r);
            if (vncle_match(l, curr, match))
                return -1;
            if (*match == false)
                return 0;
            return vncle_match(r, curr, match);
        } break;
        case NUMBER_COMPARING_LOGICAL_EXPRESSION_OR:
        {
            PC_ASSERT(l && r);
            if (vncle_match(l, curr, match))
                return -1;
            if (*match == true)
                return 0;
            return vncle_match(r, curr, match);
        } break;
        case NUMBER_COMPARING_LOGICAL_EXPRESSION_XOR:
        {
            PC_ASSERT(l && r);
            bool a, b;
            if (vncle_match(l, curr, &a))
                return -1;
            if (vncle_match(r, curr, &b))
                return -1;
            *match =(a != b);
            return 0;
        } break;
        case NUMBER_COMPARING_LOGICAL_EXPRESSION_NOT:
        {
            PC_ASSERT(l && !r);
            int r = vncle_match(l, curr, match);
            if (r)
                return r;
            *match = !*match;
            return 0;
        } break;
        case NUMBER_COMPARING_LOGICAL_EXPRESSION_NUM:
        {
            PC_ASSERT(!l && !r);
            struct value_number_comparing_condition *vncc = &vncle->vncc;
            return vncc_match(vncc, curr, match);
        } break;
    }
    PC_ASSERT(0);
    return -1;
}


void
string_matching_logical_expression_reset(
        struct string_matching_logical_expression *exp)
{
    if (!exp)
        return;

    struct pctree_node *top = &exp->node;
    struct pctree_node *node, *next;
    pctree_for_each_post_order(top, node, next) {
        struct string_matching_logical_expression *p;
        p = container_of(node,
                struct string_matching_logical_expression, node);
        pctree_node_remove(node);
        switch (p->type)
        {
            case STRING_MATCHING_LOGICAL_EXPRESSION_AND:
                // fall-through
            case STRING_MATCHING_LOGICAL_EXPRESSION_OR:
                // fall-through
            case STRING_MATCHING_LOGICAL_EXPRESSION_XOR:
                // fall-through
            case STRING_MATCHING_LOGICAL_EXPRESSION_NOT:
                break;
            case STRING_MATCHING_LOGICAL_EXPRESSION_STR:
                string_matching_condition_reset(&p->smc);
                break;
        }
        if (p!=exp)
            free(p);
    }
}

static inline void
smle_get_children(struct string_matching_logical_expression *exp,
        struct string_matching_logical_expression **l,
        struct string_matching_logical_expression **r)
{
    struct pctree_node *node = &exp->node;
    size_t nr = pctree_node_children_number(node);
    struct pctree_node *n;
    PC_ASSERT(nr<=2);
    if (nr == 0)
        return;

    n = pctree_node_child(node);
    PC_ASSERT(n);
    *l = container_of(n,
            struct string_matching_logical_expression, node);
    if (nr == 1)
        return;

    n = pctree_node_next(n);
    PC_ASSERT(n);
    *r = container_of(n,
            struct string_matching_logical_expression, node);
}


int
string_matching_logical_expression_match(
        struct string_matching_logical_expression *exp,
        purc_variant_t curr, bool *match)
{
    struct string_matching_logical_expression *l = NULL, *r = NULL;
    smle_get_children(exp, &l, &r);

    switch (exp->type)
    {
        case STRING_MATCHING_LOGICAL_EXPRESSION_AND:
        {
            PC_ASSERT(l && r);
            if (string_matching_logical_expression_match(l, curr, match))
                return -1;
            if (*match == false)
                return 0;
            return string_matching_logical_expression_match(r, curr, match);
        } break;
        case STRING_MATCHING_LOGICAL_EXPRESSION_OR:
        {
            PC_ASSERT(l && r);
            if (string_matching_logical_expression_match(l, curr, match))
                return -1;
            if (*match == true)
                return 0;
            return string_matching_logical_expression_match(r, curr, match);
        } break;
        case STRING_MATCHING_LOGICAL_EXPRESSION_XOR:
        {
            PC_ASSERT(l && r);
            bool a, b;
            if (string_matching_logical_expression_match(l, curr, &a))
                return -1;
            if (string_matching_logical_expression_match(r, curr, &b))
                return -1;
            *match =(a != b);
            return 0;
        } break;
        case STRING_MATCHING_LOGICAL_EXPRESSION_NOT:
        {
            PC_ASSERT(l && !r);
            return string_matching_logical_expression_match(l, curr, match);
        } break;
        case STRING_MATCHING_LOGICAL_EXPRESSION_STR:
        {
            PC_ASSERT(!l && !r);
            struct string_matching_condition *smc = &exp->smc;
            return string_matching_condition_eval(smc, curr, match);
        } break;
    }
    PC_ASSERT(0);
    return -1;
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
        if (purc_isspace(c)) {
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
        s[i] = purc_toupper(c);
    }
}

static inline void
compress_spaces(char *s)
{
    size_t n = strlen(s);
    if (n==0)
        return;

    if (purc_isspace(*s)) {
        *s = ' ';
    }

    if (n==1)
        return;

    char *t = s;

    for (size_t i=1; i<n; ++i) {
        const char c = s[i];
        if (purc_isspace(c)) {
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
    char buf[BUF_SIZE];
    bool specified_length = false;

    if (MATCHING_FLAGS_IS_SET_WITH(lexp->suffix.matching_flags, MATCHING_FLAG_C)) {
        if (!literal) {
            literal = strdup(lexp->literal);
            if (!literal)
                goto end;
        }
        if (!target) {
            int n = purc_variant_stringify_buff(buf, sizeof(buf), val);
            if (n < 0 || (size_t)n >= sizeof(buf))
                goto end;
            target = buf;
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
            int n = purc_variant_stringify_buff(buf, sizeof(buf), val);
            if (n < 0 || (size_t)n >= sizeof(buf))
                goto end;
            target = buf;
        }

        normalize_space(literal);
        normalize_space(target);
    }

    if (MATCHING_FLAGS_IS_SET_WITH(lexp->suffix.matching_flags, MATCHING_FLAG_I)) {
        cmp = pcutils_strncasecmp;
    } else {
        cmp = strncmp;
    }

    if (lexp->suffix.max_matching_length > 0) {
        n = lexp->suffix.max_matching_length;
        specified_length = true;
    } else {
        if (literal) {
            n = strlen(literal);
        } else {
            n = strlen(lexp->literal);
        }
    }

    if (literal && target) {
        if (!specified_length && strlen(target) != n) {
            r = -1;
        }
        else {
            r = cmp(literal, target, n);
        }
    } else {
        r = purc_variant_stringify_buff(buf, sizeof(buf), val);
        if (r < 0 || (size_t)r >= sizeof(buf)) {
            // FIXME:
            r = -1;
        } else {
            if (!specified_length && n != (size_t)r) {
                r = -1;
            }
            else {
                r = cmp(lexp->literal, buf, n);
            }
        }
    }

    if (result)
        *result = r == 0 ? true : false;

    r = 0;

end:
    free(literal);

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

    char buf[BUF_SIZE];

    int r = -1;
    r = purc_variant_stringify_buff(buf, sizeof(buf), val);
    if (r < 0 || (size_t)r >= sizeof(buf))
        return -1;

    r = -1;
    const char *s = buf;
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
                if (!target)
                    goto end;
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

#if HAVE(GLIB_LESS_2_70)
    rr = g_pattern_match(ps, strlen(t), t, NULL);
#else
    rr = g_pattern_spec_match(ps, strlen(t), t, NULL);
#endif

    if (result)
        *result = (rr) ? true : false;
end:
    free(target);

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

    char buf[BUF_SIZE];
    r = purc_variant_stringify_buff(buf, sizeof(buf), val);
    if (r < 0 || (size_t)r >= sizeof(buf))
        return -1;
    const char *s = buf;
    r = -1;

    if (!rexp->reg_valid) {
        if (regular_expression_init_reg(rexp))
            goto end;
    }

    r = 0;
    v = regexec(&rexp->reg, s, 0, NULL, rexp->eflags);

    if (result)
        *result = (v == 0) ? true : false;

end:
    return r ? -1 : 0;
}

int iterative_formula_add(struct iterative_formula_expression *exp)
{
    size_t nr = pctree_node_children_number(&exp->node);
    if (nr != 2)
        return -1;
    struct iterative_formula_expression *l, *r;
    l = container_of(exp->node.first_child,
            struct iterative_formula_expression, node);
    r = container_of(exp->node.last_child,
            struct iterative_formula_expression, node);

    exp->result = l->result + r->result;

    return 0;
}

int iterative_formula_sub(struct iterative_formula_expression *exp)
{
    size_t nr = pctree_node_children_number(&exp->node);
    if (nr != 2)
        return -1;
    struct iterative_formula_expression *l, *r;
    l = container_of(exp->node.first_child,
            struct iterative_formula_expression, node);
    r = container_of(exp->node.last_child,
            struct iterative_formula_expression, node);

    exp->result = l->result - r->result;

    return 0;
}

int iterative_formula_mul(struct iterative_formula_expression *exp)
{
    size_t nr = pctree_node_children_number(&exp->node);
    if (nr != 2)
        return -1;
    struct iterative_formula_expression *l, *r;
    l = container_of(exp->node.first_child,
            struct iterative_formula_expression, node);
    r = container_of(exp->node.last_child,
            struct iterative_formula_expression, node);

    exp->result = l->result * r->result;

    return 0;
}

int iterative_formula_div(struct iterative_formula_expression *exp)
{
    size_t nr = pctree_node_children_number(&exp->node);
    if (nr != 2)
        return -1;
    struct iterative_formula_expression *l, *r;
    l = container_of(exp->node.first_child,
            struct iterative_formula_expression, node);
    r = container_of(exp->node.last_child,
            struct iterative_formula_expression, node);

    exp->result = l->result / r->result;

    return 0;
}

int iterative_formula_neg(struct iterative_formula_expression *exp)
{
    size_t nr = pctree_node_children_number(&exp->node);
    if (nr != 1)
        return -1;
    struct iterative_formula_expression *l;
    l = container_of(exp->node.first_child,
            struct iterative_formula_expression, node);

    exp->result = -l->result;

    return 0;
}

int iterative_formula_iterate(struct iterative_formula_expression *exp,
        purc_variant_t curr, double *result)
{
    struct pctree_node *root = &exp->node;
    struct pctree_node *p, *n;
    pctree_for_each_post_order(root, p, n) {
        struct iterative_formula_expression *ife;
        ife = container_of(p, struct iterative_formula_expression, node);
        switch (ife->type)
        {
            case ITERATIVE_FORMULA_EXPRESSION_OP:
            {
                int r = ife->op(ife);
                if (r)
                    return -1;
            } break;
            case ITERATIVE_FORMULA_EXPRESSION_NUM:
            {
                ife->result = ife->d;
            } break;
            case ITERATIVE_FORMULA_EXPRESSION_ID:
            {
                purc_variant_t k = ife->key_name;
                purc_variant_t v = purc_variant_object_get(curr, k);
                PC_ASSERT(v != PURC_VARIANT_INVALID);
                double d = purc_variant_numerify(v);
                ife->result = d;
            } break;
        }
    }

    *result = exp->result;
    return 0;
}

void iterative_formula_expression_release(
        struct iterative_formula_expression *exp)
{
    if (!exp)
        return;

    struct pctree_node *top = &exp->node;
    struct pctree_node *node, *next;
    pctree_for_each_post_order(top, node, next) {
        struct iterative_formula_expression *p;
        p = container_of(node, struct iterative_formula_expression, node);
        pctree_node_remove(node);
        switch (p->type)
        {
            case ITERATIVE_FORMULA_EXPRESSION_OP:
                break;
            case ITERATIVE_FORMULA_EXPRESSION_ID:
                PCEXE_CLR_VAR(p->key_name);
                break;
            case ITERATIVE_FORMULA_EXPRESSION_NUM:
                break;
        }
        if (p!=exp)
            free(p);
    }
}

void
vncle_release(struct value_number_comparing_logical_expression *vncle)
{
    if (!vncle)
        return;

    struct pctree_node *top = &vncle->node;
    struct pctree_node *node, *next;
    pctree_for_each_post_order(top, node, next) {
        struct value_number_comparing_logical_expression *p;
        p = container_of(node,
                struct value_number_comparing_logical_expression, node);
        pctree_node_remove(node);

        vncc_release(&p->vncc);

        if (p!=vncle)
            free(p);
    }
}
