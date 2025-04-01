/**
 * @file observe.c
 * @author Xue Shuming
 * @date 2021/12/28
 * @brief The ops for <observe>
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
#include "private/instance.h"
#include "purc-runloop.h"

#include "../ops.h"

#include <pthread.h>
#include <unistd.h>

#define EVENT_SEPARATOR      ':'

struct ctxt_for_observe {
    struct pcvdom_node           *curr;
    purc_variant_t                on;
    purc_variant_t                for_var;
    purc_variant_t                at;
    purc_variant_t                as;
    purc_variant_t                with;
    purc_variant_t                against;
    purc_variant_t                implicit_data;

    pcvdom_element_t              define;

    char                         *msg_type;
    char                         *sub_type;
};

static void
ctxt_for_observe_destroy(struct ctxt_for_observe *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);
        PURC_VARIANT_SAFE_CLEAR(ctxt->for_var);
        PURC_VARIANT_SAFE_CLEAR(ctxt->at);
        PURC_VARIANT_SAFE_CLEAR(ctxt->as);
        PURC_VARIANT_SAFE_CLEAR(ctxt->with);
        PURC_VARIANT_SAFE_CLEAR(ctxt->against);
        PURC_VARIANT_SAFE_CLEAR(ctxt->implicit_data);

        if (ctxt->msg_type) {
            free(ctxt->msg_type);
            ctxt->msg_type = NULL;
        }
        if (ctxt->sub_type) {
            free(ctxt->sub_type);
            ctxt->sub_type = NULL;
        }
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_observe_destroy((struct ctxt_for_observe*)ctxt);
}

/* Since 0.9.22, use normalized event name:
    change:inflated, change:deflated, and change:modified */
bool base_variant_msg_listener(purc_variant_t source, pcvar_op_t msg_type,
        void* ctxt, size_t nr_args, purc_variant_t* argv)
{
    UNUSED_PARAM(source);
    UNUSED_PARAM(msg_type);
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);

    const char *smsg = NULL;
    switch (msg_type) {
        case PCVAR_OPERATION_INFLATED:
            smsg = MSG_SUB_TYPE_INFLATED;
            break;
        case PCVAR_OPERATION_DEFLATED:
            smsg = MSG_SUB_TYPE_DEFLATED;
            break;
        case PCVAR_OPERATION_MODIFIED:
            smsg = MSG_SUB_TYPE_MODIFIED;
            break;
        default:
            break;
    }

    pcintr_stack_t stack = (pcintr_stack_t)ctxt;
    pcintr_coroutine_post_event(stack->co->cid,
            PCRDR_MSG_EVENT_REDUCE_OPT_IGNORE,
            source, MSG_TYPE_CHANGE, smsg, PURC_VARIANT_INVALID,
            PURC_VARIANT_INVALID);

    return true;
}

#define TIMERS_EXPIRED_PREFIX                "expired:"
#define TIMERS_ACTIVATED_PREFIX              "activated:"
#define TIMERS_DEACTIVATED_PREFIX            "deactivated:"


static inline bool
is_mmutable_variant_msg(const char *type)
{
    if ((strcmp(type, MSG_SUB_TYPE_INFLATED) == 0) ||
            (strcmp(type, MSG_SUB_TYPE_DEFLATED) == 0) ||
            (strcmp(type, MSG_SUB_TYPE_MODIFIED) == 0)) {
        return true;
    }
    purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
        "unknown msg: %s", type);
    return false;
}

static bool
regist_variant_listener(pcintr_stack_t stack, purc_variant_t observed,
        const char *type, struct pcvar_listener** listener)
{
    if (strcmp(type, MSG_SUB_TYPE_INFLATED) == 0) {
        *listener = purc_variant_register_post_listener(observed,
                PCVAR_OPERATION_INFLATED, base_variant_msg_listener, stack);
    }
    else if (strcmp(type, MSG_SUB_TYPE_DEFLATED) == 0) {
        *listener = purc_variant_register_post_listener(observed,
                PCVAR_OPERATION_DEFLATED, base_variant_msg_listener, stack);
    }
    else if (strcmp(type, MSG_TYPE_CHANGE) == 0) {
        *listener = purc_variant_register_post_listener(observed,
                PCVAR_OPERATION_MODIFIED, base_variant_msg_listener, stack);
    }
    else {
        return false;
    }

    if (*listener != NULL) {
        return true;
    }
    return false;
}

static int
process_attr_on(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;
    if (ctxt->on != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (val == PURC_VARIANT_INVALID && !frame->handle_event) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    if (val) {
        ctxt->on = val;
        purc_variant_ref(val);
    }

    return 0;
}

static int
process_attr_at(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;
    if (ctxt->at != PURC_VARIANT_INVALID) {
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
    ctxt->at = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_as(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;
    if (ctxt->as != PURC_VARIANT_INVALID) {
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
    ctxt->as = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_with(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;
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
process_attr_for(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;
    if (ctxt->for_var != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (val == PURC_VARIANT_INVALID && !frame->handle_event) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }

    if (val) {
        ctxt->for_var = val;
        purc_variant_ref(val);
    }
    else {
        return 0;
    }

    const char *s = purc_variant_get_string_const(ctxt->for_var);
    const char *p = strchr(s, EVENT_SEPARATOR);
    if (p) {
        ctxt->msg_type = strndup(s, p-s);
        ctxt->sub_type = strdup(p+1);
    }
    else {
        ctxt->msg_type = strdup(s);
    }

    if (ctxt->msg_type == NULL) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "unknown vdom attribute '%s = %s' for element <%s>",
                purc_atom_to_string(name), s, element->tag_name);
        return -1;
    }

    return 0;
}

static int
process_attr_against(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;
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
    UNUSED_PARAM(attr);
    UNUSED_PARAM(ud);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, FOR)) == name) {
        return process_attr_for(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ON)) == name) {
        return process_attr_on(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AT)) == name) {
        return process_attr_at(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AS)) == name) {
        return process_attr_as(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITH)) == name) {
        return process_attr_with(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AGAINST)) == name) {
        return process_attr_against(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SILENTLY)) == name) {
        return 0;
    }

    /* ignore other attr */
    return 0;
}

static void
on_named_observe_release(void* native_entity)
{
    struct pcintr_observer *observer = (struct pcintr_observer*)native_entity;
    pcintr_revoke_observer(observer);
}

static struct pcintr_observer *
register_named_var_observer(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame,
        purc_variant_t at_var
        )
{
    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;

    struct pcvdom_element *element = frame->pos;
    const char* name = purc_variant_get_string_const(at_var);
    purc_variant_t observed = pcintr_get_named_var_for_observed(stack, name,
            pcvdom_element_parent(element));

    if (observed == PURC_VARIANT_INVALID) {
        return NULL;
    }

    purc_variant_t at = pcintr_get_at_var(frame);
    pcdoc_element_t edom_element;
    edom_element = pcdvobjs_get_element_from_elements(at, 0);

    struct pcintr_observer *result = pcintr_register_observer(stack,
            OBSERVER_SOURCE_HVML,
            CO_STAGE_OBSERVING, CO_STATE_OBSERVING,
            observed, ctxt->implicit_data,
            ctxt->msg_type, ctxt->sub_type,
            frame->pos, edom_element, frame->pos, NULL, NULL, NULL,
            NULL, NULL, false);
    purc_variant_unref(observed);
    return result;
}

static struct pcintr_observer *
register_native_var_observer(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame,
        purc_variant_t on
        )
{
    UNUSED_PARAM(stack);
    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;

    purc_variant_t observed = on;
    struct pcintr_observer *observer = NULL;
    struct purc_native_ops *ops = purc_variant_native_get_ops(observed);

    void *native_entity = purc_variant_native_get_entity(observed);

    if(!ops->on_observe(native_entity, ctxt->msg_type, ctxt->sub_type)) {
        // TODO: purc_set_error
        return NULL;
    }

    purc_variant_t at = pcintr_get_at_var(frame);
    pcdoc_element_t edom_element;
    edom_element = pcdvobjs_get_element_from_elements(at, 0);

    observer = pcintr_register_observer(stack,
            OBSERVER_SOURCE_HVML,
            CO_STAGE_OBSERVING, CO_STATE_OBSERVING,
            observed, ctxt->implicit_data,
            ctxt->msg_type, ctxt->sub_type,
            frame->pos,
            edom_element, frame->pos, NULL, NULL, NULL, NULL, NULL, false);

    return observer;
}

static struct pcintr_observer *
register_timer_observer(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame,
        purc_variant_t on
        )
{
    UNUSED_PARAM(stack);
    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;

    purc_variant_t at = pcintr_get_at_var(frame);
    pcdoc_element_t edom_element;
    edom_element = pcdvobjs_get_element_from_elements(at, 0);
    if (edom_element == NULL) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "`in` not valid");
        return NULL;
    }

    return pcintr_register_observer(stack,
            OBSERVER_SOURCE_HVML,
            CO_STAGE_OBSERVING, CO_STATE_OBSERVING,
            on, ctxt->implicit_data,
            ctxt->msg_type, ctxt->sub_type,
            frame->pos,
            edom_element, frame->pos, NULL, NULL, NULL, NULL, NULL, false);
}

void on_revoke_mmutable_var_observer(struct pcintr_observer *observer,
        void *data)
{
    if (observer && data) {
        struct pcvar_listener *listener = (struct pcvar_listener*)data;
        purc_variant_revoke_listener(observer->observed, listener);
    }
}

static struct pcintr_observer *
register_mmutable_var_observer(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame,
        purc_variant_t on
        )
{
    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;

    struct pcvar_listener *listener = NULL;
    if (!regist_variant_listener(stack, on, ctxt->msg_type, &listener))
        return NULL;

    purc_variant_t at = pcintr_get_at_var(frame);
    pcdoc_element_t edom_element;
    edom_element = pcdvobjs_get_element_from_elements(at, 0);

    return pcintr_register_observer(stack,
            OBSERVER_SOURCE_HVML,
            CO_STAGE_OBSERVING, CO_STATE_OBSERVING,
            on, ctxt->implicit_data,
            ctxt->msg_type, ctxt->sub_type,
            frame->pos,
            edom_element, frame->pos,
            on_revoke_mmutable_var_observer, listener, NULL, NULL, NULL, false);
}

static bool
is_css_select(const char *s)
{
    bool match = false;
    if (!(s && s[0])) {
        goto out;
    }

    if (s[0] == '.' || s[0] == '#' || purc_is_valid_css_identifier(s)) {
        match = true;
    }
    else {
        pcdoc_selector_t selector =  pcdoc_selector_new(s);
        if (selector) {
            pcdoc_selector_delete(selector);
            match = true;
        }
    }

out:
    return match;
}

static struct pcintr_observer *
register_elements_observer(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame,
        purc_variant_t observed)
{
    struct pcintr_observer *observer = NULL;
    const char *s = purc_variant_get_string_const(observed);
    purc_document_t doc = stack->doc;
    purc_variant_t elems = pcdvobjs_elements_by_css(doc, s);
    if (elems) {
        observer = register_native_var_observer(stack, frame, elems);
        purc_variant_unref(elems);
    }
    return observer;
}

static struct pcintr_observer *
register_default_observer(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame,
        purc_variant_t observed)
{
    UNUSED_PARAM(stack);
    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;

    purc_variant_t at = pcintr_get_at_var(frame);
    pcdoc_element_t edom_element;
    edom_element = pcdvobjs_get_element_from_elements(at, 0);

    if (pcintr_is_crtn_object(observed, NULL)) {
        observed = pcintr_crtn_observed_create(stack->co->cid);
    }
    else {
        purc_variant_ref(observed);
    }

    struct pcintr_observer * ret = pcintr_register_observer(stack,
            OBSERVER_SOURCE_HVML,
            CO_STAGE_OBSERVING, CO_STATE_OBSERVING,
            observed, ctxt->implicit_data,
            ctxt->msg_type, ctxt->sub_type,
            frame->pos, edom_element, frame->pos,
            NULL, NULL, NULL, NULL, NULL, false);

    purc_variant_unref(observed);
    return ret;
}

static struct pcintr_observer *
process_named_var_observer(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame, purc_variant_t name)
{
    if (purc_variant_is_string(name)) {
        return register_named_var_observer(stack, frame, name);
    }
    return NULL;
}

static struct pcintr_observer *
process_variant_observer(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame, purc_variant_t observed)
{
    if (pcintr_is_timers(stack->co, observed)) {
        return register_timer_observer(stack, frame, observed);
    }
    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;

    enum purc_variant_type type = purc_variant_get_type(observed);
    switch (type) {
    case PURC_VARIANT_TYPE_NATIVE:
        return register_native_var_observer(stack, frame, observed);

    case PURC_VARIANT_TYPE_OBJECT:
    case PURC_VARIANT_TYPE_ARRAY:
    case PURC_VARIANT_TYPE_SET:
        if (strcmp(ctxt->msg_type, MSG_TYPE_CHANGE) == 0 &&
                is_mmutable_variant_msg(ctxt->sub_type)) {
            return register_mmutable_var_observer(stack, frame, observed);
        }
        return register_default_observer(stack, frame, observed);

    case PURC_VARIANT_TYPE_STRING:
        if (is_css_select(purc_variant_get_string_const(observed))) {
            return register_elements_observer(stack, frame, observed);
        }
        return register_default_observer(stack, frame, observed);

    default:
        return register_default_observer(stack, frame, observed);
    }

     // NOTE: never reached here!!!
    return NULL;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    if (stack->except)
        return NULL;

    pcintr_check_insertion_mode_for_normal_element(stack);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_observe *ctxt = frame->ctxt;
    if (!ctxt) {
        ctxt = (struct ctxt_for_observe*)calloc(1, sizeof(*ctxt));
        if (!ctxt) {
            purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
            return NULL;
        }

        frame->ctxt = ctxt;
        frame->ctxt_destroy = ctxt_destroy;

        frame->pos = pos; // ATTENTION!!
    }

    if (NULL == pcintr_stack_frame_get_parent(frame)) {
        return ctxt;
    }

    if (frame->handle_event) {
        purc_variant_t exclamation_var = pcintr_get_exclamation_var(frame);

        ctxt->implicit_data = purc_variant_ref(exclamation_var);

        purc_variant_t v = purc_variant_object_get_by_ckey_ex(exclamation_var,
                PCINTR_EXCLAMATION_OBSERVEDAGAINST, true);
        if (v) {
            ctxt->against = purc_variant_ref(v);
        }

        v = purc_variant_object_get_by_ckey_ex(exclamation_var,
                PCINTR_EXCLAMATION_OBSERVEDON, true);
        if (v) {
            ctxt->on = purc_variant_ref(v);
        }

        v = purc_variant_object_get_by_ckey_ex(exclamation_var,
                PCINTR_EXCLAMATION_OBSERVEDFOR, true);
        if (v) {
            ctxt->for_var = purc_variant_ref(v);
            const char *s = purc_variant_get_string_const(ctxt->for_var);
            const char *p = strchr(s, EVENT_SEPARATOR);
            if (p) {
                ctxt->msg_type = strndup(s, p-s);
                ctxt->sub_type = strdup(p+1);
            }
            else {
                ctxt->msg_type = strdup(s);
            }
        }

        v = purc_variant_object_get_by_ckey_ex(exclamation_var,
                PCINTR_EXCLAMATION_OBSERVEDIN, true);
        if (v) {
            frame->attr_in = purc_variant_ref(v);
        }

        v = purc_variant_object_get_by_ckey_ex(exclamation_var,
                PCINTR_EXCLAMATION_OBSERVEDWITH, true);
        if (v) {
            ctxt->with = purc_variant_ref(v);
        }

        v = purc_variant_object_get_by_ckey_ex(exclamation_var,
                PCINTR_EXCLAMATION_OBSERVEDCONTENT, true);
        if (v) {
            pcintr_set_symbol_var(frame, PURC_SYMBOL_VAR_CARET, v);
        }
    }
    else {
        if (0 != pcintr_stack_frame_eval_attr_and_content(stack, frame,
                    false)) {
            return NULL;
        }

        int r = pcintr_walk_attrs(frame, frame->pos, stack, attr_found_val);
        if (r) {
            return ctxt;
        }

        ctxt->implicit_data = purc_variant_make_object_0();

        if (ctxt->against) {
            purc_variant_object_set_by_ckey(ctxt->implicit_data,
                    PCINTR_EXCLAMATION_OBSERVEDAGAINST, ctxt->against);
        }

        if (ctxt->on) {
            purc_variant_object_set_by_ckey(ctxt->implicit_data,
                    PCINTR_EXCLAMATION_OBSERVEDON, ctxt->on);
        }

        if (ctxt->for_var) {
            purc_variant_object_set_by_ckey(ctxt->implicit_data,
                    PCINTR_EXCLAMATION_OBSERVEDFOR, ctxt->for_var);
        }

        if (frame->attr_in) {
            purc_variant_object_set_by_ckey(ctxt->implicit_data,
                    PCINTR_EXCLAMATION_OBSERVEDIN, frame->attr_in);
        }

        if (ctxt->with) {
            purc_variant_object_set_by_ckey(ctxt->implicit_data,
                    PCINTR_EXCLAMATION_OBSERVEDWITH, ctxt->with);
        }

        purc_variant_t tmp = pcintr_get_symbol_var(frame, PURC_SYMBOL_VAR_CARET);
        if (tmp) {
            purc_variant_object_set_by_ckey(ctxt->implicit_data,
                    PCINTR_EXCLAMATION_OBSERVEDCONTENT, tmp);
        }
    }

#if 0
    if (!ctxt->with) {
        purc_variant_t caret = pcintr_get_symbol_var(frame,
                PURC_SYMBOL_VAR_CARET);
        if (caret && !purc_variant_is_undefined(caret)) {
            ctxt->with = caret;
            purc_variant_ref(ctxt->with);
        }
    }
#endif

    if (ctxt->with != PURC_VARIANT_INVALID) {
        pcvdom_element_t define = pcintr_get_vdom_from_variant(ctxt->with);
        if (define == NULL) {
            purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                    "no vdom element was found for `with`");
            return ctxt;
        }

        if (pcvdom_element_first_child_element(define) == NULL) {
            purc_set_error(PURC_ERROR_NO_DATA);
            return ctxt;
        }

        ctxt->define = define;
    }

#if 0      /* { */
    purc_variant_t on;
    on = ctxt->on;
    if (on == PURC_VARIANT_INVALID)
        return NULL;
#endif     /* } */

    purc_variant_t for_var;
    for_var = ctxt->for_var;
    if (!frame->handle_event &&
            (for_var == PURC_VARIANT_INVALID || !purc_variant_is_string(for_var))) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return ctxt;
    }

    if (pcintr_common_handle_attr_in(stack->co, frame)) {
        return ctxt;
    }

    if (frame->handle_event) {
        purc_clr_error();
        return ctxt;
    }

    struct pcintr_observer* observer = NULL;

    if (ctxt->against) {
        observer = process_named_var_observer(stack, frame, ctxt->against);
    }
    else if (ctxt->on) {
        observer = process_variant_observer(stack, frame, ctxt->on);
    }

    if (observer == NULL) {
        return ctxt;
    }

    if (ctxt->as != PURC_VARIANT_INVALID && purc_variant_is_string(ctxt->as)) {
        const char* name = purc_variant_get_string_const(ctxt->as);
        static struct purc_native_ops ops = {
            .on_release                   = on_named_observe_release,
        };

        purc_variant_t v = purc_variant_make_native(observer, &ops);
        if (v == PURC_VARIANT_INVALID) {
            pcintr_revoke_observer(observer);
            return ctxt;
        }

        int bind_ret = pcintr_bind_named_variable(stack, frame, name,
                ctxt->at, false, false, v);

        if (bind_ret != 0) {
            purc_variant_unref(v); // on_release
            return ctxt;
        }
        purc_variant_unref(v);
    }

    purc_clr_error();

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    UNUSED_PARAM(ud);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    if (frame->ctxt == NULL)
        return true;

    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;

    if (!ctxt) {
        goto out;
    }

    if (frame->handle_event) {
        if ((ctxt->msg_type && strcmp(ctxt->msg_type, MSG_TYPE_REQUEST) == 0) &&
                stack->co->curator && pcintr_is_crtn_exists(stack->co->curator)) {
            purc_variant_t exclamation_var = pcintr_get_exclamation_var(frame);
            purc_variant_t observed = PURC_VARIANT_INVALID;
            if (exclamation_var) {
                observed = purc_variant_object_get_by_ckey_ex(exclamation_var,
                        PCINTR_EXCLAMATION_EVENT_REQUEST_ID, true);
            }
            if (observed) {
                purc_variant_ref(observed);
            }
            else {
                observed = pcintr_crtn_observed_create(stack->co->cid);
            }

            purc_variant_t result = pcintr_coroutine_get_result(stack->co);
            pcintr_coroutine_post_event(stack->co->curator,
                    PCRDR_MSG_EVENT_REDUCE_OPT_KEEP,
                    observed,
                    MSG_TYPE_RESPONSE, ctxt->sub_type,
                    result, observed);
            purc_variant_unref(observed);
        }
    }

    ctxt_for_observe_destroy(ctxt);
    frame->ctxt = NULL;

out:
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

    pcintr_coroutine_t co = stack->co;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    if (!frame->handle_event) {
        return NULL;
    }

    struct ctxt_for_observe *ctxt;
    ctxt = (struct ctxt_for_observe*)frame->ctxt;

    struct pcvdom_node *curr;

    if (stack->back_anchor == frame) {
        stack->back_anchor = NULL;
        ctxt->define = NULL;
        ctxt->curr = NULL;
    }

    if (frame->ctxt == NULL)
        return NULL;

    if (stack->back_anchor)
        return NULL;

again:
    curr = ctxt->curr;

    if (curr == NULL) {
        struct pcvdom_element *element = frame->pos;
        if (ctxt->define)
            element = ctxt->define;

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
    .rerun              = NULL,
    .select_child       = select_child,
};

struct pcintr_element_ops* pcintr_get_observe_ops(void)
{
    return &ops;
}

