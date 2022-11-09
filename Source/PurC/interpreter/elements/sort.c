/**
 * @file sort.c
 * @author Xue Shuming
 * @date 2022/06/14
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
 *
 */

#include "purc.h"

#include "../internal.h"

#include "private/debug.h"
#include "private/dvobjs.h"
#include "private/executor.h"
#include "purc-runloop.h"

#include "../executors/exe_func.h"

#include "../ops.h"

#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

struct sort_key {
    char *key;
    bool by_number;
};

struct ctxt_for_sort {
    struct pcvdom_node           *curr;
    purc_variant_t                on;
    purc_variant_t                by;
    purc_variant_t                with;
    purc_variant_t                against;
    unsigned int                  casesensitively:1;
    unsigned int                  ascendingly:1;

    struct pcutils_arrlist       *keys;

    void                         *handle;
    void                         *symbol;
};

static void
sort_key_free_fn(void *data)
{
    if (!data) {
        return;
    }
    struct sort_key *key = data;
    if (key->key) {
        free(key->key);
    }
    free(key);
}

static void
ctxt_for_sort_destroy(struct ctxt_for_sort *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);
        PURC_VARIANT_SAFE_CLEAR(ctxt->by);
        PURC_VARIANT_SAFE_CLEAR(ctxt->with);
        PURC_VARIANT_SAFE_CLEAR(ctxt->against);
        if (ctxt->keys) {
            pcutils_arrlist_free(ctxt->keys);
        }
        if (ctxt->symbol) {
            ctxt->symbol = NULL;
        }
        if (ctxt->handle) {
            pcintr_unload_module(ctxt->handle);
            ctxt->handle = NULL;
        }
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_sort_destroy((struct ctxt_for_sort*)ctxt);
}

static int
process_attr_on(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_sort *ctxt;
    ctxt = (struct ctxt_for_sort*)frame->ctxt;
    if (ctxt->on != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    ctxt->on = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_by(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_sort *ctxt;
    ctxt = (struct ctxt_for_sort*)frame->ctxt;
    if (ctxt->by != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    ctxt->by = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_with(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_sort *ctxt;
    ctxt = (struct ctxt_for_sort*)frame->ctxt;
    if (ctxt->with != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    ctxt->with = val;
    purc_variant_ref(val);
    return 0;
}

static int
process_attr_against(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_sort *ctxt;
    ctxt = (struct ctxt_for_sort*)frame->ctxt;
    if (ctxt->against != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    ctxt->against = val;
    purc_variant_ref(val);
    return 0;
}

static int
attr_found_val(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud)
{
    UNUSED_PARAM(ud);

    PC_ASSERT(name);
    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_OPERATOR);

    struct ctxt_for_sort *ctxt;
    ctxt = (struct ctxt_for_sort*)frame->ctxt;

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ON)) == name) {
        return process_attr_on(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, BY)) == name) {
        return process_attr_by(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITH)) == name) {
        return process_attr_with(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AGAINST)) == name) {
        return process_attr_against(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CASESENSITIVELY)) == name
            || pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CASE)) == name) {
        PC_ASSERT(purc_variant_is_undefined(val));
        ctxt->casesensitively= 1;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CASEINSENSITIVELY)) == name
            || pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CASELESS)) == name) {
        PC_ASSERT(purc_variant_is_undefined(val));
        ctxt->casesensitively= 0;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ASCENDINGLY)) == name
            || pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ASC)) == name) {
        PC_ASSERT(purc_variant_is_undefined(val));
        ctxt->ascendingly= 1;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, DESCENDINGLY)) == name
            || pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, DESC)) == name) {
        PC_ASSERT(purc_variant_is_undefined(val));
        ctxt->ascendingly= 0;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SILENTLY)) == name) {
        return 0;
    }

    /* ignore other attr */
    return 0;
}

struct pcutils_arrlist  *
split_key(const char *key)
{
    if (key == NULL) {
        return NULL;
    }

    struct pcutils_arrlist *keys = pcutils_arrlist_new(sort_key_free_fn);
    if (keys == NULL) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    char *key_cp = strdup(key);
    char *ctxt = key_cp;
    char *token;
    while ((token = strtok_r(ctxt, " ", &ctxt))) {
        struct sort_key *skey = (struct sort_key*)calloc(1,
                sizeof(struct sort_key));
        if (skey == NULL) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            break;
        }

        skey->key = strdup(token);
        if (!skey->key) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            break;
        }
        if (pcutils_arrlist_append(keys, skey) != 0) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            break;
        }
    }
    free(key_cp);
    return keys;
}

static int
comp_number(double l, double r, bool ascendingly)
{
    if (ascendingly) {
        return (l > r) ? 1 : (l < r ? -1 : 0);
    }
    return (l > r) ? -1 : (l < r ? 1 : 0);
}

static int
comp_string(const char *l, const char *r, bool ascendingly, bool casesensitively)
{
    if (!l || !r) {
        return 0;
    }
    int ret = casesensitively ? strcmp(l, r) : strcasecmp(l, r);
    return ascendingly ? ret : -ret;
}

static char *
variant_to_string(purc_variant_t v)
{
    if (!v) {
        return NULL;
    }

    char *buf = NULL;
    int total = purc_variant_stringify_alloc(&buf, v);
    if (total < 0) {
        return NULL;
    }
    return buf;
}

static int
comp_raw(purc_variant_t l, purc_variant_t r, bool by_number,
        bool ascendingly, bool casesensitively)
{
    if (by_number) {
        double dl = l ? purc_variant_numberify(l) : 0.0f;
        double dr = r ? purc_variant_numberify(r) : 0.0f;
        return comp_number(dl, dr, ascendingly);
    }

    char *buf_l = variant_to_string(l);
    char *buf_r = variant_to_string(r);
    int ret = comp_string(buf_l, buf_r, ascendingly, casesensitively);

    if (buf_l) {
        free(buf_l);
    }
    if (buf_r) {
        free(buf_r);
    }

    return ret;
}

static int
comp_by_key(purc_variant_t l, purc_variant_t r, const char *key, bool by_number,
        bool ascendingly, bool casesensitively)
{
    purc_variant_t lv = PURC_VARIANT_INVALID;
    purc_variant_t rv = PURC_VARIANT_INVALID;
    if (purc_variant_is_object(l)) {
        lv = purc_variant_object_get_by_ckey(l, key);
        purc_clr_error();
    }

    if (purc_variant_is_object(r)) {
        rv = purc_variant_object_get_by_ckey(r, key);
        purc_clr_error();
    }

    return comp_raw(lv, rv, by_number, ascendingly, casesensitively);
}

static int
sort_cmp(purc_variant_t l, purc_variant_t r, void *data)
{
    struct ctxt_for_sort *ctxt = data;
    size_t nr_keys = pcutils_arrlist_length(ctxt->keys);
    for (size_t i = 0; i < nr_keys; i++) {
        struct sort_key *key = pcutils_arrlist_get_idx(ctxt->keys, i);
        int ret = 0;
        if (key->key == NULL) {
            ret = comp_raw(l, r, key->by_number, ctxt->ascendingly,
                    ctxt->casesensitively);
        }
        else {
            ret = comp_by_key(l, r, key->key, key->by_number,
                    ctxt->ascendingly, ctxt->casesensitively);
        }
        if (ret != 0) {
            return ret;
        }
    }
    return 0;
}

static bool
sort_as_number(purc_variant_t val)
{
    enum purc_variant_type type = purc_variant_get_type(val);
    switch (type) {
    case PURC_VARIANT_TYPE_NUMBER:
    case PURC_VARIANT_TYPE_LONGINT:
    case PURC_VARIANT_TYPE_ULONGINT:
    case PURC_VARIANT_TYPE_LONGDOUBLE:
        return true;

    default:
        return false;
    }
}

static void
sort_array(struct ctxt_for_sort *ctxt, purc_variant_t array,
        purc_variant_t against)
{
    ssize_t nr = purc_variant_array_get_size(array);
    if (nr <= 1) {
        return;
    }

    if (against && purc_variant_is_string(against)) {
        ctxt->keys = split_key(purc_variant_get_string_const(against));
    }

    if (ctxt->keys == NULL) {
        struct pcutils_arrlist *keys = pcutils_arrlist_new(sort_key_free_fn);
        if (keys == NULL) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return;
        }
        ctxt->keys = keys;
        struct sort_key *key = (struct sort_key*)calloc(1,
                sizeof(struct sort_key));
        if (key == NULL) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return;
        }

        purc_variant_t val = purc_variant_array_get(array, 0);
        key->by_number = sort_as_number(val);
        if (pcutils_arrlist_append(keys, key) != 0) {
            free(key);
        }
    }
    else {
        size_t key_idx = 0;
        size_t nr_keys = pcutils_arrlist_length(ctxt->keys);
        for (ssize_t i = 0 ; i < nr && key_idx < nr_keys; i++) {
            purc_variant_t val = purc_variant_array_get(array, i);
            if (purc_variant_is_object(val)) {
                for (size_t j = key_idx; j < nr_keys; j++) {
                    struct sort_key *k = pcutils_arrlist_get_idx(ctxt->keys, j);
                    purc_variant_t v = purc_variant_object_get_by_ckey(val, k->key);
                    if (v) {
                        k->by_number = sort_as_number(v);
                        key_idx++;
                    }
                }
            }
        }
    }
    pcvariant_array_sort(array, ctxt, sort_cmp);
}


static void
sort_set(struct ctxt_for_sort *ctxt, purc_variant_t set, purc_variant_t against)
{
    ssize_t nr = purc_variant_set_get_size(set);
    if (nr <= 1) {
        return;
    }

    if (against && purc_variant_is_string(against)) {
        ctxt->keys = split_key(purc_variant_get_string_const(against));
    }

    if (ctxt->keys == NULL) {
        struct pcutils_arrlist *keys = pcutils_arrlist_new(sort_key_free_fn);
        if (keys == NULL) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return;
        }
        ctxt->keys = keys;
        struct sort_key *key = (struct sort_key*)calloc(1,
                sizeof(struct sort_key));
        if (key == NULL) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return;
        }

        purc_variant_t val = purc_variant_set_get_by_index(set, 0);
        key->by_number = sort_as_number(val);
        if (pcutils_arrlist_append(keys, key) != 0) {
            free(key);
        }
    }
    else {
        size_t key_idx = 0;
        size_t nr_keys = pcutils_arrlist_length(ctxt->keys);
        for (ssize_t i = 0 ; i < nr && key_idx < nr_keys; i++) {
            purc_variant_t val = purc_variant_set_get_by_index(set, i);
            if (purc_variant_is_object(val)) {
                for (size_t j = key_idx; j < nr_keys; j++) {
                    struct sort_key *k = pcutils_arrlist_get_idx(ctxt->keys, j);
                    purc_variant_t v = purc_variant_object_get_by_ckey(val, k->key);
                    if (v) {
                        k->by_number = sort_as_number(v);
                        key_idx++;
                    }
                }
            }
        }
    }
    pcvariant_set_sort(set, ctxt, sort_cmp);
}

static int
sort_val(pcintr_stack_t stack, purc_variant_t val)
{
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

    struct ctxt_for_sort *ctxt;
    ctxt = (struct ctxt_for_sort*)frame->ctxt;

    enum purc_variant_type type = purc_variant_get_type(val);
    switch (type) {
        case PURC_VARIANT_TYPE_ARRAY:
            sort_array(ctxt, val, ctxt->against);
            break;

        case PURC_VARIANT_TYPE_SET:
            sort_set(ctxt, val, ctxt->against);
            break;

        default:
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
    }

    return 0;
}

static purc_variant_t
do_internal(purc_exec_ops_t ops,
        const char *rule, purc_variant_t on, purc_variant_t with)
{
    PC_ASSERT(ops->create);
    PC_ASSERT(ops->choose);
    PC_ASSERT(ops->destroy);

    purc_exec_inst_t exec_inst;
    exec_inst = ops->create(PURC_EXEC_TYPE_CHOOSE, on, false);
    if (!exec_inst)
        return PURC_VARIANT_INVALID;

    exec_inst->with = with;

    purc_variant_t value;
    value = ops->choose(exec_inst, rule);
    bool ok;
    ok = ops->destroy(exec_inst);
    PC_ASSERT(ok);
    exec_inst = NULL;

    if (value == PURC_VARIANT_INVALID) {
        PC_ASSERT(purc_get_last_error());
        return PURC_VARIANT_INVALID;
    }

    return value;
}

static int
do_external_func(struct pcintr_stack_frame *frame, pcexec_func_ops_t ops,
        const char *rule, purc_variant_t on, purc_variant_t with,
        purc_variant_t against, bool desc, bool caseless)
{
    PC_ASSERT(ops->chooser);
    PC_ASSERT(ops->iterator);
    PC_ASSERT(ops->reducer);
    PC_ASSERT(ops->sorter);

    purc_variant_t value;
    value = ops->sorter(rule, on, with, against, desc, caseless);

    if (value == PURC_VARIANT_INVALID) {
        PC_ASSERT(purc_get_last_error());
        return -1;
    }

    int r;

    r = pcintr_set_question_var(frame, value);
    purc_variant_unref(value);

    return r ? -1 : 0;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    PC_ASSERT(stack && pos);
    if (stack->except)
        return NULL;

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

    struct ctxt_for_sort *ctxt;
    ctxt = (struct ctxt_for_sort*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    ctxt->casesensitively = 1;
    ctxt->ascendingly= 1;

    frame->ctxt = ctxt;
    frame->ctxt_destroy = ctxt_destroy;

    frame->pos = pos; // ATTENTION!!

    if (0 != pcintr_stack_frame_eval_attr_and_content(stack, frame, false)) {
        return NULL;
    }

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    int r;
    r = pcintr_walk_attrs(frame, element, stack, attr_found_val);
    if (r)
        return ctxt;

    if (!ctxt->with) {
        purc_variant_t caret = pcintr_get_symbol_var(frame,
                PURC_SYMBOL_VAR_CARET);
        if (caret && !purc_variant_is_undefined(caret)) {
            ctxt->with = caret;
            purc_variant_ref(ctxt->with);
        }
    }

    if (ctxt->on == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "`on` not specified");
        return ctxt;
    }

    purc_variant_t result = PURC_VARIANT_INVALID;

    if (ctxt->by != PURC_VARIANT_INVALID) {
        // TODO:
        const char *rule = purc_variant_get_string_const(ctxt->by);
        PC_ASSERT(rule);

        purc_variant_t on   = ctxt->on;
        purc_variant_t with = ctxt->with;
        purc_variant_t against = ctxt->against;

        pcexec_ops ops;
        int r;
        r = pcexecutor_get_by_rule(rule, &ops);
        if (r)
            return ctxt;

        switch (ops.type) {
            case PCEXEC_TYPE_INTERNAL:
                result = do_internal(ops.internal_ops, rule, on, with);
                if (result == PURC_VARIANT_INVALID)
                    return ctxt;

                break;

            case PCEXEC_TYPE_EXTERNAL_FUNC:
                do_external_func(frame, ops.external_func_ops, rule,
                        on, with, against,
                        !ctxt->ascendingly,
                        !ctxt->casesensitively);
                return ctxt;

            case PCEXEC_TYPE_EXTERNAL_CLASS:
                purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                        "<choose> does NOT support CLASS executor");
                return ctxt;

            default:
                PC_ASSERT(0);
        }
    }
    else {
        result = purc_variant_ref(ctxt->on);
    }

    r = sort_val(stack, result);
    if (r == 0) {
        pcintr_set_question_var(frame, result);
    }
    purc_variant_unref(result);

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);
    PC_ASSERT(ud == frame->ctxt);

    if (frame->ctxt == NULL)
        return true;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    struct ctxt_for_sort *ctxt;
    ctxt = (struct ctxt_for_sort*)frame->ctxt;
    if (ctxt) {
        ctxt_for_sort_destroy(ctxt);
        frame->ctxt = NULL;
    }

    return true;
}

static void
on_element(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_element *element)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(element);
}

static void
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    PC_ASSERT(content);
}

static void
on_comment(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_comment *comment)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    PC_ASSERT(comment);
}


static pcvdom_element_t
select_child(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);

    pcintr_coroutine_t co = stack->co;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(ud == frame->ctxt);

    if (stack->back_anchor == frame)
        stack->back_anchor = NULL;

    if (frame->ctxt == NULL)
        return NULL;

    if (stack->back_anchor)
        return NULL;

    struct ctxt_for_sort *ctxt;
    ctxt = (struct ctxt_for_sort*)frame->ctxt;
    if (!ctxt)
        return NULL;

    struct pcvdom_node *curr;

again:
    curr = ctxt->curr;

    if (curr == NULL) {
        struct pcvdom_element *element = frame->pos;
        struct pcvdom_node *node = &element->node;
        node = pcvdom_node_first_child(node);
        curr = node;
    }
    else {
        curr = pcvdom_node_next_sibling(curr);
    }

    ctxt->curr = curr;

    if (curr == NULL) {
        purc_clr_error();
        return NULL;
    }

    switch (curr->type) {
        case PCVDOM_NODE_DOCUMENT:
            PC_ASSERT(0); // Not implemented yet
            break;
        case PCVDOM_NODE_ELEMENT:
            {
                pcvdom_element_t element = PCVDOM_ELEMENT_FROM_NODE(curr);
                on_element(co, frame, element);
                return element;
            }
        case PCVDOM_NODE_CONTENT:
            on_content(co, frame, PCVDOM_CONTENT_FROM_NODE(curr));
            goto again;
        case PCVDOM_NODE_COMMENT:
            on_comment(co, frame, PCVDOM_COMMENT_FROM_NODE(curr));
            goto again;
        default:
            PC_ASSERT(0); // Not implemented yet
    }

    PC_ASSERT(0);
    return NULL; // NOTE: never reached here!!!
}

static struct pcintr_element_ops
ops = {
    .after_pushed       = after_pushed,
    .on_popping         = on_popping,
    .rerun              = NULL,
    .select_child       = select_child,
};

struct pcintr_element_ops* pcintr_get_sort_ops(void)
{
    return &ops;
}

