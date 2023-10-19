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

#define ATTR_NAME_IDD_BY    "idd-by"
#define DEFAULT_RULE        "RANGE: FROM 0"

enum step_for_iterate {
    STEP_BEFORE_FIRST_ITERATE,
    STEP_BEFORE_ITERATE,
    STEP_ITERATE,
    STEP_AFTER_ITERATE,
    STEP_CHECK_STOP,
    STEP_DONE,
};

enum step_for_func {
    FUNC_STEP_1ST,
    FUNC_STEP_2ND,
    FUNC_STEP_3RD,
    FUNC_STEP_4TH,
    FUNC_STEP_5TH,
    FUNC_STEP_DONE,
};

struct ctxt_for_iterate {
    struct pcvdom_node           *curr;

    purc_variant_t                on;
    purc_variant_t                in;

    struct pcvdom_attr           *onlyif_attr;
    size_t                        onlyif_attr_idx;
    struct pcvdom_attr           *while_attr;
    size_t                        while_attr_idx;
    struct pcvdom_attr           *with_attr;
    size_t                        with_attr_idx;

    struct pcvdom_attr           *rule_attr;
    size_t                        rule_attr_idx;


    struct pcvdom_attr           *on_attr;
    size_t                        on_attr_idx;
    struct pcvdom_attr           *in_attr;
    size_t                        in_attr_idx;
    struct pcvcm_node            *content_vcm;

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
    unsigned int                  is_rerun:1;
    enum step_for_iterate         step;
    enum step_for_func            before_first_iterate_step;
    enum step_for_func            do_iterate_step;
    enum step_for_func            after_iterate_without_executor_step;
};

static void
ctxt_for_iterate_destroy(struct ctxt_for_iterate *ctxt)
{
    if (ctxt) {
        if (ctxt->exec_inst) {
            ctxt->ops.internal_ops->destroy(ctxt->exec_inst);
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


static void
set_attr_val(pcintr_stack_t stack, struct pcintr_stack_frame *frame,
        size_t idx, purc_variant_t val)
{
    purc_variant_t v = pcutils_array_get(frame->attrs_result, idx);
    PURC_VARIANT_SAFE_CLEAR(v);

    stack->vcm_eval_pos = idx;
    pcutils_array_set(frame->attrs_result, idx, val);
    if (val) {
        purc_variant_ref(val);
    }
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

static struct ctxt_for_iterate*
first_iterate_without_executor(pcintr_coroutine_t co,
        struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;

    /* verify with attr if on attr not exists */
    if (!ctxt->on_attr) {
        purc_variant_t val;
        if (ctxt->with_attr) {
            val = pcintr_eval_vcm(&co->stack, ctxt->with_attr->val,
                    frame->silently);
            set_attr_val(&co->stack, frame, ctxt->with_attr_idx, val);
        }
        else {
            val = purc_variant_make_undefined();
        }

        if (!val) {
            return NULL;
        }

        if (check_stop(val)) {
            ctxt->stop = true;
        }
        else {
            pcintr_set_input_var(&co->stack, val);
        }
        purc_variant_unref(val);
    }

    if (ctxt->stop) {
        return NULL;
    }

    /* $0< set to  $0? */
    purc_variant_t v = frame->symbol_vars[PURC_SYMBOL_VAR_LESS_THAN];
    pcintr_set_question_var(frame, v);

    return ctxt;
}

static int
rerun_iterate_without_executor(pcintr_coroutine_t co,
        struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;

    /* verify with attr if on attr not exists */
    if (!ctxt->on_attr) {
        purc_variant_t val;
        if (ctxt->with_attr) {
            val = pcintr_eval_vcm(&co->stack, ctxt->with_attr->val,
                    frame->silently);
            set_attr_val(&co->stack, frame, ctxt->with_attr_idx, val);
        }
        else {
            val = purc_variant_make_undefined();
        }

        if (!val) {
            return purc_get_last_error();
        }

        if (check_stop(val)) {
            ctxt->stop = true;
        }
        else {
            pcintr_set_input_var(&co->stack, val);
        }
        purc_variant_unref(val);
    }

    /* $0< set to  $0? */
    purc_variant_t v = frame->symbol_vars[PURC_SYMBOL_VAR_LESS_THAN];
    pcintr_set_question_var(frame, v);

    return 0;
}

static struct ctxt_for_iterate*
post_process_by_internal_rule(struct ctxt_for_iterate *ctxt,
        struct pcintr_stack_frame *frame, const char *rule,
        purc_variant_t on, purc_variant_t with)
{
    PC_DEBUGX("rule: %s", rule);
    purc_exec_ops_t ops = ctxt->ops.internal_ops;

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
        int err = purc_get_last_error();
        if (err) {
            if (err == PURC_ERROR_NOT_EXISTS) {
                ctxt->stop = true;
                purc_clr_error();
            }
            return ctxt;
        }
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
    pcexec_class_ops_t ops = ctxt->ops.external_class_ops;

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

    pcexec_func_ops_t ops = ctxt->ops.external_func_ops;
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

    pcintr_set_question_var(frame, value);

    return ctxt;
}

static struct ctxt_for_iterate *
first_iterate_by_executor(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;

    purc_variant_t on = ctxt->on;
    if (on == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "lack of vdom attribute 'on' for element <iterate>");
        return NULL;
    }
    purc_variant_t with = ctxt->with;

    const char *rule = purc_variant_get_string_const(ctxt->evalued_rule);
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
            break;
    }

    return NULL;
}

static bool
rerun_internal_rule(struct ctxt_for_iterate *ctxt,
        struct pcintr_stack_frame *frame, pcintr_stack_t stack)
{
    purc_exec_inst_t exec_inst;
    exec_inst = ctxt->exec_inst;

    purc_exec_iter_t it = ctxt->it;

    purc_exec_ops_t ops = ctxt->ops.internal_ops;

    purc_variant_t value;
    value = ops->it_value(exec_inst, it);
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
rerun_external_class(struct ctxt_for_iterate *ctxt,
        struct pcintr_stack_frame *frame, pcintr_stack_t stack)
{
    pcexec_class_iter_t it = ctxt->it_class;

    pcexec_class_ops_t ops = ctxt->ops.external_class_ops;

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

static int
rerun_iterate_by_executor(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;

    int r;
    r = pcintr_inc_percent_var(frame);
    if (r)
        return -1;

    pcintr_stack_t stack = &co->stack;
    bool b = false;
    switch (ctxt->ops.type) {
        case PCEXEC_TYPE_INTERNAL:
            b = rerun_internal_rule(ctxt, frame, stack);
            break;

        case PCEXEC_TYPE_EXTERNAL_FUNC:
            b = rerun_external_func(ctxt, frame, stack);
            break;

        case PCEXEC_TYPE_EXTERNAL_CLASS:
            b = rerun_external_class(ctxt, frame, stack);
            break;

        default:
            break;
    }

    return  b ? 0 : -1;
}

static int
process_attr_onlyif(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, struct pcvdom_attr *attr)
{
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
        purc_atom_t name, struct pcvdom_attr *attr)
{
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
        purc_atom_t name,
        struct pcvdom_attr *attr,
        size_t idx,
        void *ud)
{
    UNUSED_PARAM(ud);

    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ON)) == name) {
        ctxt->on_attr = attr;
        ctxt->on_attr_idx = idx;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, IN)) == name) {
        ctxt->in_attr = attr;
        ctxt->in_attr_idx = idx;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, BY)) == name) {
        ctxt->rule_attr = attr;
        ctxt->rule_attr_idx = idx;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ONLYIF)) == name) {
        ctxt->onlyif_attr_idx = idx;
        return process_attr_onlyif(frame, element, name, attr);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WHILE)) == name) {
        ctxt->while_attr_idx = idx;
        return process_attr_while(frame, element, name, attr);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITH)) == name) {
        ctxt->with_attr = attr;
        ctxt->with_attr_idx = idx;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, NOSETOTAIL)) == name
            || pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, NOSE_TO_TAIL)) == name) {
        ctxt->nosetotail = 1;
        purc_variant_t val = purc_variant_make_boolean(true);
        pcutils_array_set(frame->attrs_result, idx, val);
        return 0;
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SILENTLY)) == name) {
        purc_variant_t val = purc_variant_make_boolean(true);
        pcutils_array_set(frame->attrs_result, idx, val);
        return 0;
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, MUST_YIELD)) == name) {
        purc_variant_t val = purc_variant_make_boolean(true);
        pcutils_array_set(frame->attrs_result, idx, val);
        return 0;
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, IN)) == name) {
        frame->attr_in = pcintr_eval_vcm((pcintr_stack_t)ud, attr->val,
                frame->silently);
        return pcintr_common_handle_attr_in(((pcintr_stack_t)ud)->co, frame);
    }

    /* ignore other attr */
    return 0;
}

static int
before_first_iterate(pcintr_stack_t stack, struct pcintr_stack_frame *frame,
        struct ctxt_for_iterate *ctxt)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(ctxt);
    int err = 0;
    purc_variant_t val;

    if (ctxt->rule_attr || !ctxt->with_attr) {
        ctxt->by_rule = 1;
    }

    while(ctxt->before_first_iterate_step != FUNC_STEP_DONE) {
        switch (ctxt->before_first_iterate_step) {
            case FUNC_STEP_1ST:
                if (!ctxt->on_attr) {
                    ctxt->before_first_iterate_step = FUNC_STEP_2ND;
                    break;
                }

                val = pcintr_eval_vcm(stack, ctxt->on_attr->val,
                        frame->silently);
                set_attr_val(stack, frame, ctxt->on_attr_idx, val);
                if (!val) {
                    err = purc_get_last_error();
                    goto out;
                }

                if (purc_variant_is_undefined(val)) {
                    purc_variant_unref(val);
                    purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                            "vdom attribute '%s' for element <%s> undefined",
                            ctxt->on_attr->key, frame->pos->tag_name);
                    return -1;
                }

                pcintr_set_input_var(stack, val);
                ctxt->on = val;
                if (ctxt->stop) {
                    ctxt->before_first_iterate_step = FUNC_STEP_DONE;
                }
                else {
                    ctxt->before_first_iterate_step = FUNC_STEP_2ND;
                }
                break;

            case FUNC_STEP_2ND:
                if (!ctxt->in_attr) {
                    ctxt->before_first_iterate_step = FUNC_STEP_3RD;
                    break;
                }
                val = pcintr_eval_vcm(stack, ctxt->in_attr->val,
                        frame->silently);
                set_attr_val(stack, frame, ctxt->in_attr_idx, val);
                if (!val) {
                    err = purc_get_last_error();
                    goto out;
                }

                if (purc_variant_is_undefined(val)) {
                    purc_variant_unref(val);
                    purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                            "vdom attribute '%s' for element <%s> undefined",
                            ctxt->on_attr->key, frame->pos->tag_name);
                    return -1;
                }

                if (!purc_variant_is_string(val)) {
                    purc_set_error(PURC_ERROR_INVALID_VALUE);
                    err = PURC_ERROR_INVALID_VALUE;
                    purc_variant_unref(val);
                    goto out;
                }

                purc_variant_t elements = pcintr_doc_query(stack->co,
                        purc_variant_get_string_const(val), frame->silently);
                if (elements == PURC_VARIANT_INVALID) {
                    purc_set_error(PURC_ERROR_INVALID_VALUE);
                    err = PURC_ERROR_INVALID_VALUE;
                    purc_variant_unref(val);
                    goto out;
                }

                err = pcintr_set_at_var(frame, elements);
                purc_variant_unref(elements);
                if (err) {
                    purc_variant_unref(val);
                    goto out;
                }
                ctxt->in = val;
                if (ctxt->stop) {
                    ctxt->before_first_iterate_step = FUNC_STEP_DONE;
                }
                else {
                    ctxt->before_first_iterate_step = FUNC_STEP_3RD;
                }
                break;

            case FUNC_STEP_3RD:
                if (ctxt->by_rule) {
                    purc_variant_t with;
                    if (ctxt->with_attr) {
                        with = pcintr_eval_vcm(stack, ctxt->with_attr->val,
                                frame->silently);
                        set_attr_val(stack, frame, ctxt->with_attr_idx, val);
                    }
                    else {
                        with = purc_variant_make_undefined();
                    }
                    if (!with) {
                        err = purc_get_last_error();
                        goto out;
                    }

                    PURC_VARIANT_SAFE_CLEAR(ctxt->with);
                    ctxt->with = with;
                }
                if (ctxt->stop) {
                    ctxt->before_first_iterate_step = FUNC_STEP_DONE;
                }
                else {
                    ctxt->before_first_iterate_step = FUNC_STEP_4TH;
                }
                break;

            case FUNC_STEP_4TH:
                if (ctxt->by_rule) {
                    if (ctxt->rule_attr) {
                        val = pcintr_eval_vcm(stack, ctxt->rule_attr->val,
                                frame->silently);
                        set_attr_val(stack, frame, ctxt->rule_attr_idx, val);
                    }
                    else {
                        val = purc_variant_make_string_static(DEFAULT_RULE,
                                false);
                    }

                    if (!val) {
                        err = purc_get_last_error();
                        goto out;
                    }

                    PURC_VARIANT_SAFE_CLEAR(ctxt->evalued_rule);
                    ctxt->evalued_rule = val;
                }
                if (ctxt->stop) {
                    ctxt->before_first_iterate_step = FUNC_STEP_DONE;
                }
                else {
                    ctxt->before_first_iterate_step = FUNC_STEP_DONE;
                }
                break;

            case FUNC_STEP_DONE:
            default:
                break;
        }
    }

    ctxt->before_first_iterate_step = FUNC_STEP_1ST;

out:
    return err;
}

static int
before_iterate(pcintr_stack_t stack, struct pcintr_stack_frame *frame,
        struct ctxt_for_iterate *ctxt)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(ctxt);

    if (ctxt->by_rule) {
        return 0;
    }

    if (!ctxt->onlyif_attr) {
        return 0;
    }

    purc_variant_t val = pcintr_eval_vcm(stack, ctxt->onlyif_attr->val,
            frame->silently);
    set_attr_val(stack, frame, ctxt->onlyif_attr_idx, val);
    if (!val) {
        return purc_get_last_error();
    }

    bool v = purc_variant_booleanize(val);
    PURC_VARIANT_SAFE_CLEAR(val);

    ctxt->stop = !v;

    return 0;
}

static int
do_iterate(pcintr_stack_t stack, struct pcintr_stack_frame *frame,
        struct ctxt_for_iterate *ctxt)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(ctxt);
    int err = 0;

    while(ctxt->do_iterate_step != FUNC_STEP_DONE) {
        switch (ctxt->do_iterate_step) {
        case FUNC_STEP_1ST:
            if (!ctxt->is_rerun) {
                struct ctxt_for_iterate *ret;
                if (ctxt->by_rule) {
                    ret = first_iterate_by_executor(stack->co, frame);
                }
                else {
                    ret = first_iterate_without_executor(stack->co, frame);
                }
                if (!ret) {
                    err = -1;
                    goto out;
                }
            }
            else {
                if (ctxt->by_rule) {
                    err = rerun_iterate_by_executor(stack->co, frame);
                }
                else {
                    err = rerun_iterate_without_executor(stack->co, frame);
                }
                if (err != PURC_ERROR_OK) {
                    goto out;
                }
            }
            if (ctxt->stop) {
                ctxt->do_iterate_step = FUNC_STEP_DONE;
            }
            else {
                ctxt->do_iterate_step = FUNC_STEP_2ND;
            }
            break;

        case FUNC_STEP_2ND:
            if (ctxt->content_vcm) {
                purc_variant_t val = pcintr_eval_vcm(stack,
                        ctxt->content_vcm, frame->silently);
                stack->vcm_eval_pos = -1;
                if (!val) {
                    err = purc_get_last_error();
                    goto out;
                }
                pcintr_set_symbol_var(frame, PURC_SYMBOL_VAR_CARET, val);
                purc_variant_unref(val);
            }
            ctxt->do_iterate_step = FUNC_STEP_DONE;
            break;

        default:
            break;
        }
    }

    ctxt->do_iterate_step = FUNC_STEP_1ST;

out:
    return err;
}

#if 0
static int
step_check_stop(pcintr_stack_t stack, struct pcintr_stack_frame *frame,
        struct ctxt_for_iterate *ctxt)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(ctxt);
    return 0;
}
#endif

static int
prepare(pcintr_stack_t stack, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(frame);
    struct ctxt_for_iterate *ctxt = frame->ctxt;
    if (!ctxt) {
        ctxt = (struct ctxt_for_iterate*)calloc(1, sizeof(*ctxt));
        if (!ctxt) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return PURC_ERROR_OUT_OF_MEMORY;
        }

        frame->ctxt = ctxt;
        frame->ctxt_destroy = ctxt_destroy;
    }
    return 0;
}

static int
eval_attr(pcintr_stack_t stack, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(frame);
    pcutils_array_t *attrs = frame->pos->attrs;
    size_t nr_params = pcutils_array_length(attrs);
    struct pcvdom_attr *attr = NULL;
    purc_variant_t val = PURC_VARIANT_INVALID;
    purc_atom_t name = 0;
    int err = 0;

    for (; frame->eval_attr_pos < nr_params; frame->eval_attr_pos++) {
        attr = pcutils_array_get(attrs, frame->eval_attr_pos);
        name = PCHVML_KEYWORD_ATOM(HVML, attr->key);
        if (strcmp(attr->key, ATTR_NAME_IDD_BY) == 0) {
            val = pcintr_eval_vcm(stack, attr->val, frame->silently);
            set_attr_val(stack, frame, frame->eval_attr_pos, val);
            if (!val) {
                goto out;
            }
            frame->elem_id = val;
            val = PURC_VARIANT_INVALID;
        }
        err = attr_found_val(frame, frame->pos, name, attr,
                frame->eval_attr_pos, stack);
        if (err) {
            goto out;
        }
    }

out:
    return err;
}

static int
eval_content(pcintr_stack_t stack, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(stack);
    struct pcvdom_node *node = &frame->pos->node;
    node = pcvdom_node_first_child(node);
    if (!node || node->type != PCVDOM_NODE_CONTENT) {
        purc_clr_error();
        frame->elem_step = ELEMENT_STEP_LOGIC;
        goto out;
    }

    struct pcvdom_content *content = PCVDOM_CONTENT_FROM_NODE(node);
    struct ctxt_for_iterate *ctxt = frame->ctxt;
    ctxt->content_vcm = content->vcm;
out:
    return 0;
}

static int
logic(pcintr_stack_t stack, struct pcintr_stack_frame *frame)
{
    int err = 0;
    struct ctxt_for_iterate *ctxt = frame->ctxt;

    while (ctxt->step != STEP_AFTER_ITERATE) {
        switch(ctxt->step) {
            case STEP_BEFORE_FIRST_ITERATE:
                err = before_first_iterate(stack, frame, ctxt);
                if (err != PURC_ERROR_OK) {
                    goto out;
                }
                if (ctxt->stop) {
                    ctxt->step = STEP_AFTER_ITERATE;
                }
                else {
                    ctxt->step = STEP_BEFORE_ITERATE;
                }
                break;

            case STEP_BEFORE_ITERATE:
                err = before_iterate(stack, frame, ctxt);
                if (err != PURC_ERROR_OK) {
                    goto out;
                }
                if (ctxt->stop) {
                    ctxt->step = STEP_AFTER_ITERATE;
                }
                else {
                    ctxt->step = STEP_ITERATE;
                }
                break;

            case STEP_ITERATE:
                err = do_iterate(stack, frame, ctxt);
                if (err != PURC_ERROR_OK) {
                    goto out;
                }
                ctxt->step = STEP_AFTER_ITERATE;
                break;

            default:
                break;
        }
    }

    err = purc_get_last_error();
out:
    if (err != PURC_ERROR_OK && err != PURC_ERROR_OUT_OF_MEMORY
            && err != PURC_ERROR_AGAIN
            && frame->silently) {
        purc_clr_error();
    }
    return err;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    UNUSED_PARAM(pos);

    if (stack->except)
        return NULL;

    pcintr_check_insertion_mode_for_normal_element(stack);

    int err = 0;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    while (frame->elem_step != ELEMENT_STEP_DONE) {
        switch(frame->elem_step) {
            case ELEMENT_STEP_PREPARE:
                err = prepare(stack, frame);
                if (err != PURC_ERROR_OK) {
                    return NULL;
                }
                frame->elem_step = ELEMENT_STEP_EVAL_ATTR;
                break;

            case ELEMENT_STEP_EVAL_ATTR:
                err = eval_attr(stack, frame);
                if (err != PURC_ERROR_OK) {
                    return NULL;
                }
                frame->elem_step = ELEMENT_STEP_EVAL_CONTENT;
                break;

            case ELEMENT_STEP_EVAL_CONTENT:
                err = eval_content(stack, frame);
                if (err != PURC_ERROR_OK) {
                    return NULL;
                }
                frame->elem_step = ELEMENT_STEP_LOGIC;
                break;

            case ELEMENT_STEP_LOGIC:
                err = logic(stack, frame);
                if (err != PURC_ERROR_OK) {
                    return NULL;
                }
                frame->elem_step = ELEMENT_STEP_DONE;
                break;

            case ELEMENT_STEP_DONE:
                break;
        }
    }

    return frame->ctxt;
}

static bool
on_popping_internal_rule(struct ctxt_for_iterate *ctxt, pcintr_stack_t stack,
        struct pcintr_stack_frame *frame)
{
    purc_exec_inst_t exec_inst;
    exec_inst = ctxt->exec_inst;
    if (!exec_inst)
        return true;

    purc_exec_iter_t it = ctxt->it;
    if (!it)
        return true;

    purc_variant_t val;
    if (ctxt->rule_attr) {
        val = pcintr_eval_vcm(stack, ctxt->rule_attr->val, frame->silently);
        set_attr_val(stack, frame, ctxt->rule_attr_idx, val);
    }
    else {
        val = purc_variant_make_string_static(DEFAULT_RULE,
                false);
    }

    if (!val) {
        return false;
    }

    PURC_VARIANT_SAFE_CLEAR(ctxt->evalued_rule);
    ctxt->evalued_rule = val;

    const char *rule = purc_variant_get_string_const(ctxt->evalued_rule);
    if (!rule)
        return true;

    purc_exec_ops_t ops = ctxt->ops.internal_ops;

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

    pcexec_class_ops_t ops = ctxt->ops.external_class_ops;

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

static int
after_iterate_by_executor(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame, struct ctxt_for_iterate *ctxt)
{
    UNUSED_PARAM(frame);

    switch (ctxt->ops.type) {
        case PCEXEC_TYPE_INTERNAL:
            ctxt->stop = on_popping_internal_rule(ctxt, stack, frame);
            break;

        case PCEXEC_TYPE_EXTERNAL_FUNC:
            ctxt->stop = on_popping_external_func(ctxt);
            break;

        case PCEXEC_TYPE_EXTERNAL_CLASS:
            ctxt->stop = on_popping_external_class(ctxt);
            break;

        default:
            break;
    }
    return 0;
}

static int
after_iterate_without_executor(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame, struct ctxt_for_iterate *ctxt)
{
    UNUSED_PARAM(stack);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(ctxt);
    while (ctxt->after_iterate_without_executor_step != FUNC_STEP_DONE) {
        switch (ctxt->after_iterate_without_executor_step) {
        case FUNC_STEP_1ST:
            if (!ctxt->on_attr) {
                ctxt->after_iterate_without_executor_step = FUNC_STEP_2ND;
                break;
            }

            purc_variant_t val;
            if (ctxt->with_attr) {
                val = pcintr_eval_vcm(stack, ctxt->with_attr->val,
                        frame->silently);
                set_attr_val(stack, frame, ctxt->with_attr_idx, val);
            }
            else {
                val = purc_variant_make_undefined();
            }

            if (!val) {
                goto out;
            }

            if (check_stop(val)) {
                ctxt->stop = true;
            }
            else if (ctxt->nosetotail) {
                pcintr_set_input_var(stack, val);
            }
            purc_variant_unref(val);

            ctxt->after_iterate_without_executor_step = FUNC_STEP_2ND;
            break;
        case FUNC_STEP_2ND:
            if (ctxt->while_attr) {
                purc_variant_t val = pcintr_eval_vcm(stack,
                        ctxt->while_attr->val, frame->silently);
                set_attr_val(stack, frame, ctxt->while_attr_idx, val);

                if (!val) {
                    goto out;
                }

                if (!purc_variant_booleanize(val)) {
                    ctxt->stop = true;
                };
                PURC_VARIANT_SAFE_CLEAR(val);
            }
            ctxt->after_iterate_without_executor_step = FUNC_STEP_DONE;
            break;

        default:
            break;
        }
    }

    ctxt->after_iterate_without_executor_step = FUNC_STEP_1ST;

    pcintr_inc_percent_var(frame);
out:
    return 0;
}

static int
step_after_iterate(pcintr_stack_t stack, struct pcintr_stack_frame *frame,
        struct ctxt_for_iterate *ctxt)
{
    int err = 0;
    if (ctxt->stop) {
        goto out;
    }

    if (ctxt->by_rule) {
        err = after_iterate_by_executor(stack, frame, ctxt);
    }
    else {
        err = after_iterate_without_executor(stack, frame, ctxt);
    }

out:
    return err;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    UNUSED_PARAM(ud);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    if (frame->ctxt == NULL)
        return true;

    if (stack->except) {
        return true;
    }

    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;

    ctxt->step = STEP_BEFORE_ITERATE;
    int err = step_after_iterate(stack, frame, ctxt);
    if (err) {
        return false;
    }

    return ctxt->stop;
}

static int
rerun_logic(pcintr_stack_t stack, struct pcintr_stack_frame *frame)
{
    int err = 0;
    struct ctxt_for_iterate *ctxt = frame->ctxt;

    while (ctxt->step != STEP_AFTER_ITERATE) {
        switch(ctxt->step) {
            case STEP_BEFORE_ITERATE:
                err = before_iterate(stack, frame, ctxt);
                if (err != PURC_ERROR_OK) {
                    goto out;
                }
                ctxt->step = STEP_ITERATE;
                break;

            case STEP_ITERATE:
                err = do_iterate(stack, frame, ctxt);
                if (err != PURC_ERROR_OK) {
                    goto out;
                }
                ctxt->step = STEP_AFTER_ITERATE;
                break;

            default:
                break;
        }
    }

    err = purc_get_last_error();
out:
    return err;
}

static bool
rerun(pcintr_stack_t stack, void* ud)
{
    UNUSED_PARAM(ud);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    if (frame->ctxt == NULL)
        return false;

    struct ctxt_for_iterate *ctxt;
    ctxt = (struct ctxt_for_iterate*)frame->ctxt;
    ctxt->is_rerun = 1;

    int err = rerun_logic(stack, frame);
    return (err == PURC_ERROR_OK);
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
    UNUSED_PARAM(content);
}

static void
on_comment(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_comment *comment)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    UNUSED_PARAM(comment);
}

static pcvdom_element_t
select_child(pcintr_stack_t stack, void* ud)
{
    UNUSED_PARAM(ud);
    UNUSED_PARAM(stack);

    pcintr_coroutine_t co = stack->co;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    if (stack->back_anchor == frame) {
        stack->back_anchor = NULL;
        /* back operation in iterate is handled as continue */
        if (frame->ctxt) {
            struct ctxt_for_iterate *ctxt;
            ctxt = (struct ctxt_for_iterate*)frame->ctxt;
            ctxt->curr = NULL;
        }
        return NULL;
    }

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
            purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
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
            purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
    }

    purc_set_error(PURC_ERROR_NOT_SUPPORTED);
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

