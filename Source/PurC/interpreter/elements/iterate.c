/**
 * @file iterate.c
 * @author Xu Xiaohong
 * @date 2021/12/06
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
#include "private/executor.h"
#include "purc-runloop.h"

#include "../ops.h"

#include "purc-executor.h"

#include <pthread.h>
#include <unistd.h>

struct ctxt_for_iterate {
    struct pcvdom_node           *curr;

    purc_variant_t                on;
    purc_variant_t                in;

    struct pcvdom_attr           *onlyif_attr;
    struct pcvdom_attr           *while_attr;
    struct pcvdom_attr           *with_attr;

    struct pcvdom_attr           *rule_attr;
    purc_variant_t                evalued_rule;
    purc_variant_t                with;

    pcexec_ops                    ops;
    purc_exec_inst_t              exec_inst;
    union {
        purc_exec_iter_t          it;
        pcexec_class_iter_t       it_class;
    };

    purc_variant_t                val_from_func;
    size_t                        sz;
    size_t                        idx_curr;

    unsigned int                  stop:1;
    unsigned int                  by_rule:1;
    unsigned int                  nosetotail:1;
};

static void
ctxt_for_iterate_destroy(struct ctxt_for_iterate *ctxt)
{
    if (ctxt) {
        if (ctxt->exec_inst) {
            PC_ASSERT(ctxt->ops.type == PCEXEC_TYPE_INTERNAL);
            bool ok = ctxt->ops.internal_ops.destroy(ctxt->exec_inst);
            PC_ASSERT(ok);
            ctxt->exec_inst = NULL;
        }
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);
        PURC_VARIANT_SAFE_CLEAR(ctxt->in);
        PURC_VARIANT_SAFE_CLEAR(ctxt->evalued_rule);
        PURC_VARIANT_SAFE_CLEAR(ctxt->with);
        PURC_VARIANT_SAFE_CLEAR(ctxt->val_from_func);

        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_iterate_destroy((struct ctxt_for_iterate*)ctxt);
}

static bool
check_stop(purc_variant_t val)
{
    if (purc_variant_is_undefined(val)) {
        return true;
    }

    if (purc_variant_is_null(val)) {
        return true;
    }

    if (purc_variant_is_boolean(val)) {
        if (pcvariant_is_false(val)) {
            return true;
        }
    }

    return false;
}

static int
check_onlyif(struct pcvdom_attr *onlyif, bool *stop, pcintr_stack_t stack)
{
    purc_variant_t val;
    val = pcintr_eval_vdom_attr(stack, onlyif);
    if (val == PURC_VARIANT_INVALID)
        return -1;

    int64_t i64;
    bool force = true;
    bool ok;
    ok = purc_variant_cast_to_longint(val, &i64, force);

    PURC_VARIANT_SAFE_CLEAR(val);
    if (!ok)
        return -1;

    *stop = i64 ? false : true;

    return 0;
}

static int
check_while(struct pcvdom_attr *_while, bool *stop, pcintr_stack_t stack)
{
    purc_variant_t val;
    val = pcintr_eval_vdom_attr(stack, _while);
    if (val == PURC_VARIANT_INVALID)
        return -1;

    int64_t i64;
    bool force = true;
    bool ok;
    ok = purc_variant_cast_to_longint(val, &i64, force);
    PURC_VARIANT_SAFE_CLEAR(val);
    if (!ok)
        return -1;

    *stop = i64 ? false : true;

    return 0;
}

/*
 * if on != PURC_VARIANT_INVALID ,
 *    call re_eval_with after each iteration 
 * else
 *    call re_eval_with in each iteration, before upate $0?
 *
 */
static int
re_eval_with(struct pcintr_stack_frame *frame,
        struct pcvdom_attr *with, bool *stop, pcintr_stack_t stack)
{
    purc_variant_t val;
    if (with) {
        val = pcintr_eval_vdom_attr(stack, with);
    }
    else {
        val = pcintr_get_symbol_var(frame, PURC_SYMBOL_VAR_CARET);
    }

    PC_ASSERT(val != PURC_VARIANT_INVALID);
    if (val == PURC_VARIANT_INVALID) {
        return -1;
    }

    if (check_stop(val)) {
        *stop = true;
        return 0;
    }

    *stop = false;

    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;
    if (ctxt->nosetotail || ctxt->on == PURC_VARIANT_INVALID) {
        pcintr_set_input_var(stack, val);
    }

    purc_variant_unref(val);
    return 0;
}

static struct ctxt_for_iterate*
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;
    PC_ASSERT(ctxt);

    PC_ASSERT(ctxt->by_rule == 0);

    if (ctxt->onlyif_attr) {
        bool stop;
        int r = check_onlyif(ctxt->onlyif_attr, &stop, &co->stack);
        if (r) {
            if (purc_get_last_error())
                return ctxt;
            return NULL;
        }

        if (stop) {
            ctxt->stop = 1;
            return NULL;
        }
    }

    /* $0< set to  $0? */
    purc_variant_t v = frame->symbol_vars[PURC_SYMBOL_VAR_LESS_THAN];
    pcintr_set_question_var(frame, v);

    return ctxt;
}

const char *
eval_rule(struct ctxt_for_iterate *ctxt, pcintr_stack_t stack)
{
    const char *rule = "RANGE: FROM 0";
    if (ctxt->rule_attr) {
        purc_variant_t val;
        val = pcintr_eval_vdom_attr(stack, ctxt->rule_attr);
        if (val == PURC_VARIANT_INVALID)
            return NULL;

        if (!purc_variant_is_string(val)) {
            purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                    "rule is not of string type");
            purc_variant_unref(val);
            return NULL;
        }

        PURC_VARIANT_SAFE_CLEAR(ctxt->evalued_rule);
        ctxt->evalued_rule = val;

        rule = purc_variant_get_string_const(val);
        PC_ASSERT(rule);
    }

    return rule;
}

static struct ctxt_for_iterate*
post_process_by_internal_rule(struct ctxt_for_iterate *ctxt,
        struct pcintr_stack_frame *frame, const char *rule,
        purc_variant_t on, purc_variant_t with)
{
    PC_DEBUGX("rule: %s", rule);
    purc_exec_ops_t ops = &ctxt->ops.internal_ops;

    PC_ASSERT(ops->create);
    PC_ASSERT(ops->it_begin);
    PC_ASSERT(ops->it_next);
    PC_ASSERT(ops->it_value);
    PC_ASSERT(ops->destroy);

    purc_exec_inst_t exec_inst;
    exec_inst = ops->create(PURC_EXEC_TYPE_ITERATE, on, false);
    if (!exec_inst) {
        if (purc_get_last_error())
            return ctxt;
        return NULL;
    }

    exec_inst->with = with;

    ctxt->exec_inst = exec_inst;

    purc_exec_iter_t it;
    it = ops->it_begin(exec_inst, rule);
    if (!it) {
        if (purc_get_last_error())
            return ctxt;
        return NULL;
    }

    ctxt->it = it;

    purc_variant_t value;
    value = ops->it_value(exec_inst, it);
    if (value == PURC_VARIANT_INVALID) {
        if (purc_get_last_error())
            return ctxt;
        return NULL;
    }

    pcintr_set_question_var(frame, value);

    return ctxt;
}

static struct ctxt_for_iterate*
post_process_by_external_class(struct ctxt_for_iterate *ctxt,
        struct pcintr_stack_frame *frame, const char *rule,
        purc_variant_t on, purc_variant_t with)
{
    pcexec_class_ops_t ops = &ctxt->ops.external_class_ops;

    PC_ASSERT(ops->it_begin);
    PC_ASSERT(ops->it_next);
    PC_ASSERT(ops->it_value);
    PC_ASSERT(ops->it_destroy);

    pcexec_class_iter_t it;
    it = ops->it_begin(rule, on, with);
    if (!it) {
        if (purc_get_last_error())
            return ctxt;
        return NULL;
    }

    ctxt->it_class = it;

    purc_variant_t value;
    value = ops->it_value(ctxt->it_class);
    if (value == PURC_VARIANT_INVALID) {
        if (purc_get_last_error())
            return ctxt;
        return NULL;
    }

    pcintr_set_question_var(frame, value);

    return ctxt;
}

static struct ctxt_for_iterate*
post_process_by_external_func(struct ctxt_for_iterate *ctxt,
        struct pcintr_stack_frame *frame, const char *rule,
        purc_variant_t on, purc_variant_t with)
{
    UNUSED_PARAM(frame);

    pcexec_func_ops_t ops = &ctxt->ops.external_func_ops;
    purc_variant_t v = ops->iterator(rule, on, with);
    if (v == PURC_VARIANT_INVALID) {
        if (purc_get_last_error())
            return ctxt;
        return NULL;
    }

    bool ok;
    size_t sz = 0;
    ok = purc_variant_linear_container_size(v, &sz);
    if (!ok) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "not a linear container from external func executor");
        return ctxt;
    }
    if (sz == 0)
        return NULL;

    PURC_VARIANT_SAFE_CLEAR(ctxt->val_from_func);
    ctxt->val_from_func = v;
    ctxt->sz = sz;
    ctxt->idx_curr = 0;

    purc_variant_t value;
    value = purc_variant_linear_container_get(v, ctxt->idx_curr);

    PC_ASSERT(value != PURC_VARIANT_INVALID);

    pcintr_set_question_var(frame, value);

    return ctxt;
}

static struct ctxt_for_iterate *
post_process_by_rule(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;
    PC_ASSERT(ctxt);

    purc_variant_t on;
    on = ctxt->on;
    if (on == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "lack of vdom attribute 'on' for element <iterate>");
        return NULL;
    }

    purc_variant_t with;
    if (ctxt->with_attr) {
        with = pcintr_eval_vdom_attr(&co->stack, ctxt->with_attr);
    }
    else {
        with = purc_variant_make_undefined();
    }

    if (with == PURC_VARIANT_INVALID)
        return ctxt;

    PURC_VARIANT_SAFE_CLEAR(ctxt->with);
    ctxt->with = with;

    const char *rule = eval_rule(ctxt, &co->stack);
    if (!rule)
        return ctxt;

    int r;
    r = pcexecutor_get_by_rule(rule, &ctxt->ops);
    if (r)
        return ctxt;

    switch (ctxt->ops.type) {
        case PCEXEC_TYPE_INTERNAL:
            return post_process_by_internal_rule(ctxt, frame, rule, on, with);
            break;

        case PCEXEC_TYPE_EXTERNAL_FUNC:
            return post_process_by_external_func(ctxt, frame, rule, on, with);
            break;

        case PCEXEC_TYPE_EXTERNAL_CLASS:
            return post_process_by_external_class(ctxt, frame, rule, on, with);
            break;

        default:
            PC_ASSERT(0);
    }
}

static int
process_attr_on(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val, pcintr_stack_t stack)
{
    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;
    if (val == PURC_VARIANT_INVALID ||
            purc_variant_is_undefined(val))
    {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    PURC_VARIANT_SAFE_CLEAR(ctxt->on);
    ctxt->on = purc_variant_ref(val);

    /* before the first iteration set attr 'on' to $0< */
    pcintr_set_input_var(stack, val);

    return 0;
}

static int
process_attr_in(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val, pcintr_stack_t stack)
{
    UNUSED_PARAM(stack);
    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;
    if (val == PURC_VARIANT_INVALID ||
            purc_variant_is_undefined(val))
    {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    PURC_VARIANT_SAFE_CLEAR(ctxt->in);
    ctxt->in = purc_variant_ref(val);

    return 0;
}


static int
process_attr_onlyif(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val, struct pcvdom_attr *attr)
{
    UNUSED_PARAM(val);

    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;
    if (ctxt->rule_attr) {
        purc_set_error_with_info(PURC_ERROR_NOT_SUPPORTED,
                "vdom attribute '%s' for element <%s> conflicts with"
                "vdom attribute 'by'",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    ctxt->onlyif_attr = attr;

    return 0;
}

static int
process_attr_while(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val, struct pcvdom_attr *attr)
{
    UNUSED_PARAM(val);

    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;
    if (ctxt->rule_attr) {
        purc_set_error_with_info(PURC_ERROR_NOT_SUPPORTED,
                "vdom attribute '%s' for element <%s> conflicts with"
                "vdom attribute 'by'",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    ctxt->while_attr = attr;

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

    pcintr_stack_t stack = (pcintr_stack_t) ud;
    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;

    PC_ASSERT(name);
    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_OPERATOR);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ON)) == name) {
        return process_attr_on(frame, element, name, val, stack);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, IN)) == name) {
        return process_attr_in(frame, element, name, val, stack);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, BY)) == name) {
        ctxt->rule_attr = attr;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ONLYIF)) == name) {
        return process_attr_onlyif(frame, element, name, val, attr);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WHILE)) == name) {
        return process_attr_while(frame, element, name, val, attr);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITH)) == name) {
        ctxt->with_attr = attr;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, NOSETOTAIL)) == name) {
        ctxt->nosetotail = 1;
        return 0;
    }

    purc_set_error_with_info(PURC_ERROR_NOT_IMPLEMENTED,
            "vdom attribute '%s' for element <%s>",
            purc_atom_to_string(name), element->tag_name);

    return -1;
}

static int
attr_found(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name,
        struct pcvdom_attr *attr,
        void *ud)
{
    PC_ASSERT(name);
    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_OPERATOR);

    pcintr_stack_t stack = (pcintr_stack_t) ud;
    purc_variant_t val = pcintr_eval_vdom_attr(stack, attr);
    if ((pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITH)) != name)
            && (val == PURC_VARIANT_INVALID)) {
            return -1;
    }

    int r = attr_found_val(frame, element, name, val, attr, ud);

    if (val) {
        purc_variant_unref(val);
    }

    return r ? -1 : 0;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    PC_ASSERT(stack && pos);

    if (stack->except)
        return NULL;

    pcintr_check_insertion_mode_for_normal_element(stack);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    frame->ctxt = ctxt;
    frame->ctxt_destroy = ctxt_destroy;

    frame->pos = pos; // ATTENTION!!

    frame->attr_vars = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (frame->attr_vars == PURC_VARIANT_INVALID)
        return ctxt;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    int r;
    r = pcintr_vdom_walk_attrs(frame, element, stack, attr_found);
    if (r)
        return ctxt;

    pcintr_calc_and_set_caret_symbol(stack, frame);

    /* before the first iteration, set attr 'in' to $0@ */
    purc_variant_t in = ctxt->in;
    if (in != PURC_VARIANT_INVALID) {
        if (!purc_variant_is_string(in)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return ctxt;
        }

        purc_variant_t elements = pcintr_doc_query(stack->co,
                purc_variant_get_string_const(in), frame->silently);
        if (elements == PURC_VARIANT_INVALID) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return ctxt;
        }

        r = pcintr_set_at_var(frame, elements);
        purc_variant_unref(elements);
        if (r) {
            return ctxt;
        }
    }

    purc_clr_error();

    if (ctxt->rule_attr || !ctxt->with_attr) {
        ctxt->by_rule = 1;
    }

    if (ctxt->by_rule) {
        return post_process_by_rule(stack->co, frame);
    }
    else {
        return post_process(stack->co, frame);
    }
}

static bool
on_popping_with(pcintr_stack_t stack)
{
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;

    if (ctxt->stop) {
        return true;
    }

    bool stop;
    int r;
    if (ctxt->on) {
        r = re_eval_with(frame, ctxt->with_attr, &stop, stack);
        if (r) {
            // FIXME: let catch to effect afterward???
            return true;
        }

        if (stop) {
            ctxt->stop = 1;
            return true;
        }
    }

    if (ctxt->while_attr) {
        bool stop;
        int r = check_while(ctxt->while_attr, &stop, stack);
        PC_ASSERT(r == 0);

        if (stop) {
            ctxt->stop = 1;
            return true;
        }
    }

    r = pcintr_inc_percent_var(frame);
    PC_ASSERT(r == 0);

    return false;
}

static bool
on_popping_internal_rule(struct ctxt_for_iterate *ctxt, pcintr_stack_t stack)
{
    purc_exec_inst_t exec_inst;
    exec_inst = ctxt->exec_inst;
    if (!exec_inst)
        return true;

    purc_exec_iter_t it = ctxt->it;
    if (!it)
        return true;

    const char *rule = eval_rule(ctxt, stack);
    if (!rule)
        return true;

    purc_exec_ops_t ops = &ctxt->ops.internal_ops;

    it = ops->it_next(exec_inst, it, rule);

    ctxt->it = it;
    if (!it) {
        int err = purc_get_last_error();
        if (err == PURC_ERROR_NOT_EXISTS) {
            purc_clr_error();
        }
        return true;
    }

    return false;
}

static bool
on_popping_external_class(struct ctxt_for_iterate *ctxt)
{
    pcexec_class_iter_t it = ctxt->it_class;
    if (!it)
        return true;

    pcexec_class_ops_t ops = &ctxt->ops.external_class_ops;

    it = ops->it_next(it);

    ctxt->it_class = it;
    if (!it) {
        int err = purc_get_last_error();
        if (err == PURC_ERROR_NOT_EXISTS) {
            purc_clr_error();
        }
        return true;
    }

    return false;
}

static bool
on_popping_external_func(struct ctxt_for_iterate *ctxt)
{
    if (ctxt->sz == 0)
        return true;
    if (ctxt->idx_curr >= ctxt->sz)
        return true;

    ++ctxt->idx_curr;

    if (ctxt->idx_curr >= ctxt->sz)
        return true;

    return false;
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

    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;

    if (!ctxt->by_rule) {
        return on_popping_with(stack);
    }

    switch (ctxt->ops.type) {
        case PCEXEC_TYPE_INTERNAL:
            return on_popping_internal_rule(ctxt, stack);
            break;

        case PCEXEC_TYPE_EXTERNAL_FUNC:
            return on_popping_external_func(ctxt);
            break;

        case PCEXEC_TYPE_EXTERNAL_CLASS:
            return on_popping_external_class(ctxt);
            break;

        default:
            PC_ASSERT(0);
    }
}

static bool
rerun_with(pcintr_stack_t stack)
{
    PC_ASSERT(stack);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);

    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;

    /* before each iteration, veify onlyif */
    if (ctxt->onlyif_attr) {
        bool stop;
        int r = check_onlyif(ctxt->onlyif_attr, &stop, stack);
        if (r || stop) {
            ctxt->stop = 1;
            return true;
        }
    }

    /* in each iteration, evaluate content and set as $0^ */
    pcintr_calc_and_set_caret_symbol(stack, frame);

    if (ctxt->on == PURC_VARIANT_INVALID) {
        bool stop;
        int r = re_eval_with(frame, ctxt->with_attr, &stop, stack);
        if (r) {
            // FIXME: let catch to effect afterward???
            return true;
        }

        if (stop) {
            ctxt->stop = 1;
            return true;
        }
    }

    /* in each iteration, set $0< to $0? */
    purc_variant_t v = frame->symbol_vars[PURC_SYMBOL_VAR_LESS_THAN];
    pcintr_set_question_var(frame, v);
    return true;
}

static bool
rerun_internal_rule(struct ctxt_for_iterate *ctxt,
        struct pcintr_stack_frame *frame, pcintr_stack_t stack)
{
    purc_exec_inst_t exec_inst;
    exec_inst = ctxt->exec_inst;

    purc_exec_iter_t it = ctxt->it;
    PC_ASSERT(it);

    purc_exec_ops_t ops = &ctxt->ops.internal_ops;

    purc_variant_t value;
    value = ops->it_value(exec_inst, it);
    if (value == PURC_VARIANT_INVALID)
        return false;

    int r;
    r = pcintr_set_question_var(frame, value);
    if (r == 0) {
        pcintr_set_input_var(stack, value);
    }

    pcintr_calc_and_set_caret_symbol(stack, frame);
    return r ? false : true;
}

static bool
rerun_external_class(struct ctxt_for_iterate *ctxt,
        struct pcintr_stack_frame *frame, pcintr_stack_t stack)
{
    pcexec_class_iter_t it = ctxt->it_class;
    PC_ASSERT(it);

    pcexec_class_ops_t ops = &ctxt->ops.external_class_ops;

    purc_variant_t value;
    value = ops->it_value(it);
    if (value == PURC_VARIANT_INVALID)
        return false;

    int r;
    r = pcintr_set_question_var(frame, value);
    if (r == 0) {
        pcintr_set_input_var(stack, value);
    }

    return r ? false : true;
}

static bool
rerun_external_func(struct ctxt_for_iterate *ctxt,
        struct pcintr_stack_frame *frame, pcintr_stack_t stack)
{
    purc_variant_t value;
    value = purc_variant_linear_container_get(ctxt->val_from_func,
            ctxt->idx_curr);
    if (value == PURC_VARIANT_INVALID)
        return false;

    int r;
    r = pcintr_set_question_var(frame, value);
    if (r == 0) {
        pcintr_set_input_var(stack, value);
    }

    return r ? false : true;
}

static bool
rerun(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);
    PC_ASSERT(ud == frame->ctxt);

    if (frame->ctxt == NULL)
        return false;

    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;

    if (!ctxt->by_rule) {
        return rerun_with(stack);
    }

    int r;
    r = pcintr_inc_percent_var(frame);
    if (r)
        return false;

    switch (ctxt->ops.type) {
        case PCEXEC_TYPE_INTERNAL:
            return rerun_internal_rule(ctxt, frame, stack);
            break;

        case PCEXEC_TYPE_EXTERNAL_FUNC:
            return rerun_external_func(ctxt, frame, stack);
            break;

        case PCEXEC_TYPE_EXTERNAL_CLASS:
            return rerun_external_class(ctxt, frame, stack);
            break;

        default:
            PC_ASSERT(0);
    }
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

    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;

    if (ctxt->stop)
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
    .rerun              = rerun,
    .select_child       = select_child,
};

struct pcintr_element_ops* pcintr_get_iterate_ops(void)
{
    return &ops;
}

