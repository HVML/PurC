/*
 * @file internal.c
 * @author XueShuming
 * @date 2022/07/26
 * @brief The public function for interpreter
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

#include "purc.h"

#include "config.h"

#include "internal.h"

#include "purc-rwstream.h"
#include "private/var-mgr.h"
#include "private/errors.h"
#include "private/instance.h"
#include "private/utils.h"
#include "private/variant.h"

#include <stdlib.h>
#include <string.h>

#define ATTR_NAME_AS       "as"
#define MIN_BUFFER         512

static const char doctypeTemplate[] = "<!DOCTYPE hvml SYSTEM \"%s\">\n";

static const char callTemplateHead[] =
"<hvml target=\"void\">\n";

static const char callTemplateFoot[] =
"    <call on $%s with $REQ._args silently>\n"
"        $REQ._content\n"
"        <exit with $? />\n"
"    </call>\n"
"</hvml>\n";

#define  ATTR_ID            "id"
#define  ATTR_IDD_BY        "idd-by"

bool
pcintr_match_id(pcintr_stack_t stack, struct pcvdom_element *elem,
        const char *id)
{
    if (elem->node.type == PCVDOM_NODE_DOCUMENT) {
        return false;
    }

    struct pcvdom_attr *attr;

    const char *name = elem->tag_name;
    const struct pchvml_tag_entry* entry = pchvml_tag_static_search(name,
            strlen(name));
    if (entry &&
            (entry->cats & (PCHVML_TAGCAT_TEMPLATE | PCHVML_TAGCAT_VERB))) {
        attr = pcvdom_element_find_attr(elem, ATTR_IDD_BY);
    }
    else {
        attr = pcvdom_element_find_attr(elem, ATTR_ID);
    }
    if (!attr) {
        return false;
    }

    bool silently = false;
    purc_variant_t v = pcintr_eval_vcm(stack, attr->val, silently);
    purc_clr_error();
    pcvcm_eval_ctxt_destroy(stack->vcm_ctxt);
    stack->vcm_ctxt = NULL;
    if (v == PURC_VARIANT_INVALID) {
        return false;
    }

    bool matched = false;

    do {
        if (!purc_variant_is_string(v)) {
            break;
        }
        const char *sv = purc_variant_get_string_const(v);
        if (!sv) {
            break;
        }

        if (strcmp(sv, id) == 0) {
            matched = true;
        }
    } while (0);

    purc_variant_unref(v);
    return matched;
}

static int
bind_at_frame(struct pcintr_stack_frame *frame, const char *name,
        purc_variant_t v)
{
    purc_variant_t exclamation_var;
    exclamation_var = pcintr_get_exclamation_var(frame);
    if (purc_variant_is_object(exclamation_var) == false) {
        purc_set_error_with_info(PURC_ERROR_INTERNAL_FAILURE,
                "temporary variable on stack frame is not object");
        return -1;
    }

    purc_variant_t k = purc_variant_make_string(name, true);
    if (k == PURC_VARIANT_INVALID) {
        return -1;
    }

    bool ok = purc_variant_object_set(exclamation_var, k, v);
    purc_variant_unref(k);

    if (ok) {
        purc_clr_error();
        return 0;
    }
    return -1;
}

static int
bind_at_element(purc_coroutine_t cor, struct pcvdom_element *elem,
        const char *name, purc_variant_t val, pcvarmgr_t *mgr)
{
    return pcintr_bind_scope_variable(cor, elem, name, val, mgr) ? 0 : -1 ;
}

static int
bind_temp_by_level(struct pcintr_stack_frame *frame,
        const char *name, purc_variant_t val, uint64_t level)
{
    struct pcintr_stack_frame *p = frame;
    struct pcintr_stack_frame *parent;
    parent = pcintr_stack_frame_get_parent(frame);
    if (parent == NULL) {
        purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
                "no frame exists");
        return -1;
    }
    if (level == (uint64_t)-1) {
        while (p && p->pos && p->pos->tag_id != PCHVML_TAG_HVML) {
            p = pcintr_stack_frame_get_parent(p);
        }
    }
    else {
        for (uint64_t i = 0; i < level; ++i) {
            if (p == NULL) {
                break;
            }
            p = pcintr_stack_frame_get_parent(p);
        }
    }


    if (p == NULL) {
        if (!frame->silently) {
            purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
                    "no frame exists");
            return -1;
        }
        p = parent;
    }
    return bind_at_frame(p, name, val);
}

static int
bind_by_level(pcintr_stack_t stack, struct pcintr_stack_frame *frame,
        const char *name, bool temporarily, purc_variant_t val, uint64_t level,
        pcvarmgr_t *mgr)
{
    if (temporarily) {
        return bind_temp_by_level(frame, name, val, level);
    }

    bool silently = frame->silently;
    struct pcvdom_element *p = frame->pos;

    if (level == (uint64_t)-1) {
        while (p && p->tag_id != PCHVML_TAG_HVML) {
            p = pcvdom_element_parent(p);
        }
    }
    else {
        for (uint64_t i = 0; i < level; ++i) {
            if (p == NULL) {
                break;
            }
            p = pcvdom_element_parent(p);
        }
    }
    purc_clr_error();

    if (p && p->node.type != PCVDOM_NODE_DOCUMENT) {
        int ret = bind_at_element(stack->co, p, name, val, mgr);
        return ret;
    }

    if (silently) {
        p = frame->pos;
        while (p && p->tag_id != PCHVML_TAG_HVML) {
            p = pcvdom_element_parent(p);
        }
        purc_clr_error();
        return bind_at_element(stack->co, p, name, val, mgr);
    }
    purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
            "no vdom element exists");
    return -1;
}

static int
bind_at_default(pcintr_stack_t stack, struct pcintr_stack_frame *frame,
        const char *name, bool temporarily, purc_variant_t val, pcvarmgr_t *mgr)
{
    bool under_head = false;
    if (frame) {
        struct pcvdom_element *element = frame->pos;
        while ((element = pcvdom_element_parent(element))) {
            if (element->tag_id == PCHVML_TAG_HEAD) {
                under_head = true;
            }
        }
        purc_clr_error();
    }
    if (under_head) {
        return bind_by_level(stack, frame, name, temporarily, val,
                (uint64_t)-1, mgr);
    }
    return bind_by_level(stack, frame, name, temporarily, val, 1, mgr);
}

static int
bind_temp_by_elem_id(pcintr_stack_t stack, struct pcintr_stack_frame *frame,
        const char *id, const char *name, purc_variant_t val)
{
    struct pcintr_stack_frame *parent = pcintr_stack_frame_get_parent(frame);
    struct pcintr_stack_frame *dest_frame = NULL;
    struct pcintr_stack_frame *p = frame;
    while (p && p->pos) {
        struct pcvdom_element *elem = p->pos;
        if (pcintr_match_id(stack, elem, id)) {
            dest_frame = p;
            break;
        }

        p = pcintr_stack_frame_get_parent(p);
    }

    if (dest_frame == NULL) {
        if (!frame->silently) {
            purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
                    "no vdom element exists");
            return -1;
        }

        // not found, bind at parent default
        dest_frame = parent;
    }

    return bind_at_frame(dest_frame, name, val);
}

static int
bind_by_elem_id(pcintr_stack_t stack, struct pcintr_stack_frame *frame,
        const char *id, const char *name, bool temporarily, purc_variant_t val,
        pcvarmgr_t *mgr)
{
    if (temporarily) {
        return bind_temp_by_elem_id(stack, frame, id, name, val);
    }

    struct pcvdom_element *p = frame->pos;
    struct pcvdom_element *dest = NULL;
    while (p) {
        if (pcintr_match_id(stack, p, id)) {
            dest = p;
            break;
        }
        p = pcvdom_element_parent(p);
    }

    purc_clr_error();
    if (dest && dest->node.type != PCVDOM_NODE_DOCUMENT) {
        return bind_at_element(stack->co, dest, name, val, mgr);
    }

    if (frame->silently) {
        return bind_at_default(stack, frame, name, temporarily, val, mgr);
    }

    purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
            "no vdom element exists");
    return -1;
}

static int
bind_by_name_space(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame, const char *ns, const char *name,
        bool temporarily, bool runner_level_enable, purc_variant_t val,
        pcvarmgr_t *mgr)
{
    purc_atom_t atom = PCHVML_KEYWORD_ATOM(HVML, ns);
    if (atom == 0) {
        goto not_found;
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _PARENT)) == atom) {
        return bind_by_level(stack, frame, name, temporarily, val, 1, mgr);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _GRANDPARENT)) == atom ) {
        return bind_by_level(stack, frame, name, temporarily, val, 2, mgr);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _ROOT)) == atom ) {
        return bind_by_level(stack, frame, name, temporarily, val,
                (uint64_t)-1, mgr);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _LAST)) == atom) {
        return bind_by_level(stack, frame, name, temporarily, val, 1, mgr);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _NEXTTOLAST)) == atom) {
        return bind_by_level(stack, frame, name, temporarily, val, 2, mgr);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _TOPMOST)) == atom) {
        return bind_by_level(stack, frame, name, temporarily, val,
                (uint64_t)-1, mgr);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _RUNNER)) == atom) {
        if (runner_level_enable) {
            if (mgr) {
                *mgr = pcinst_get_variables();
            }
            if (!name || !val) {
                pcinst_set_error(PURC_ERROR_INVALID_VALUE);
                return -1;
            }
            bool ret = purc_bind_runner_variable(name, val);
            return ret ? 0 : -1;
        }
        else {
            purc_set_error_with_info(PURC_ERROR_NOT_SUPPORTED,
                    "at = '%s'", name);
            return -1;
        }
    }

not_found:
    if (frame->silently) {
        return bind_at_default(stack, frame, name, temporarily, val, mgr);
    }
    purc_set_error_with_info(PURC_ERROR_BAD_NAME,
            "at = '%s'", name);
    return -1;
}

static int
_bind_named_variable(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame, const char *name, purc_variant_t at,
        bool temporarily, bool runner_level_enable, purc_variant_t v,
        pcvarmgr_t *mgr)
{
    int bind_ret = -1;
    if (!at) {
        bind_ret = bind_at_default(stack, frame, name, temporarily, v, mgr);
        goto out;
    }

    if (purc_variant_is_string(at)) {
        const char *s_at = purc_variant_get_string_const(at);
        if (s_at[0] == '#') {
            bind_ret = bind_by_elem_id(stack, frame, s_at + 1,
                    name, temporarily, v, mgr);
        }
        else if (s_at[0] == '_') {
            bind_ret = bind_by_name_space(stack, frame, s_at,
                    name, temporarily, runner_level_enable, v, mgr);
        }
        else {
            uint64_t level;
            bool ok = purc_variant_cast_to_ulongint(at, &level,
                    true);
            if (ok) {
                bind_ret = bind_by_level(stack, frame, name, temporarily,
                        v, level, mgr);
            }
            else {
                bind_ret = bind_at_default(stack, frame, name, temporarily, v,
                        mgr);
            }
        }
    }
    else {
        uint64_t level;
        bool ok = purc_variant_cast_to_ulongint(at, &level, true);
        if (ok) {
            bind_ret = bind_by_level(stack, frame, name, temporarily, v, level,
                    mgr);
        }
        else {
            bind_ret = bind_at_default(stack, frame, name, temporarily, v, mgr);
        }
    }

out:
    return bind_ret;
}

int
pcintr_bind_named_variable(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame, const char *name, purc_variant_t at,
        bool temporarily, bool runner_level_enable, purc_variant_t v)
{
    return _bind_named_variable(stack, frame, name, at, temporarily,
            runner_level_enable, v, NULL);
}

pcvarmgr_t
pcintr_get_named_variable_mgr_by_at(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame, purc_variant_t at, bool temporarily,
        bool runner_level_enable)
{
    pcvarmgr_t mgr = NULL;
    _bind_named_variable(stack, frame, NULL, at, temporarily,
            runner_level_enable, PURC_VARIANT_INVALID, &mgr);
    purc_clr_error();
    return mgr;
}

static int
serial_element(const char *buf, size_t len, void *ctxt)
{
    purc_rwstream_t rws = (purc_rwstream_t) ctxt;
    purc_rwstream_write(rws, buf, len);
    return 0;
}

purc_vdom_t
pcintr_build_concurrently_call_vdom(pcintr_stack_t stack,
        pcvdom_element_t element)
{
    purc_vdom_t vdom = NULL;
    char *foot = NULL;
    purc_rwstream_t rws = NULL;
    const char *as;
    struct pcvdom_doctype  *doctype;
    size_t nr_hvml;
    const char *hvml;
    purc_variant_t as_var = PURC_VARIANT_INVALID;

    struct pcvdom_attr *as_attr = pcvdom_element_get_attr_c(element,
            ATTR_NAME_AS);
    if (!as_attr) {
        PC_WARN("Can not get %s attr\n", ATTR_NAME_AS);
        goto out;
    }

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    as_var = pcintr_eval_vcm(stack, as_attr->val, frame->silently);
    pcvcm_eval_ctxt_destroy(stack->vcm_ctxt);
    stack->vcm_ctxt = NULL;
    if (!as_var) {
        PC_WARN("eval vdom attr %s failed\n", ATTR_NAME_AS);
        goto out;
    }

    if (!purc_variant_is_string(as_var)) {
        PC_WARN("invalid vdom attr %s type %s\n", ATTR_NAME_AS,
                pcvariant_typename(as_var));
        goto out;
    }

    rws = purc_rwstream_new_buffer(MIN_BUFFER, 0);
    if (rws == NULL) {
        PC_WARN("create rwstream failed\n");
        goto out;
    }
    as = purc_variant_get_string_const(as_var);
    foot = (char*)malloc(strlen(callTemplateFoot) + strlen(as) + 1);
    if (!foot) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    sprintf(foot, callTemplateFoot, as);

    //FIXME: generate DOCTYPE
    doctype = &stack->vdom->doctype;
    if (doctype) {
        char *doc = (char *)malloc(
                strlen(doctypeTemplate) + strlen(doctype->system_info) + 1);
        sprintf(doc, doctypeTemplate, doctype->system_info);
        purc_rwstream_write(rws, doc, strlen(doc));
        free(doc);
    }

    purc_rwstream_write(rws, callTemplateHead, strlen(callTemplateHead));
    pcvdom_util_node_serialize(&element->node, serial_element, rws);
    purc_rwstream_write(rws, foot, strlen(foot));

    nr_hvml = 0;
    hvml = purc_rwstream_get_mem_buffer(rws, &nr_hvml);

    vdom = purc_load_hvml_from_string(hvml);
    if (!vdom) {
        PC_WARN("create vdom for call concurrently failed! hvml is %s\n", hvml);
    }
out:
    if (rws) {
        purc_rwstream_destroy(rws);
    }
    if (foot)
        free(foot);
    PURC_VARIANT_SAFE_CLEAR(as_var);
    return vdom;
}

int
pcintr_coroutine_dump(pcintr_coroutine_t co)
{
    purc_rwstream_t rws = purc_rwstream_new_buffer(1024, 0);
    purc_coroutine_dump_stack(co, rws);
    size_t nr_hvml = 0;
    const char *hvml = purc_rwstream_get_mem_buffer(rws, &nr_hvml);
    fprintf(stderr, "%s\n", hvml);
    purc_rwstream_destroy(rws);
    return 0;
}

purc_variant_t
pcintr_eval_vcm(pcintr_stack_t stack, struct pcvcm_node *node, bool silently)
{
    int err = 0;
    purc_variant_t val = PURC_VARIANT_INVALID;
    if (!node) {
        val = purc_variant_make_undefined();
    }
    else if (stack->vcm_ctxt) {
        val = pcvcm_eval_again(node, stack, silently,
                stack->timeout);
        stack->timeout = false;
    }
    else {
        val = pcvcm_eval(node, stack, silently);
    }

    err = purc_get_last_error();
    if (!val) {
        goto out;
    }

    if (err == PURC_ERROR_AGAIN && val) {
        purc_variant_unref(val);
        val = PURC_VARIANT_INVALID;
        goto out;
    }

    purc_clr_error();
    pcvcm_eval_ctxt_destroy(stack->vcm_ctxt);
    stack->vcm_ctxt = NULL;
out:
    return val;
}

enum pcfetcher_request_method
pcintr_method_from_via(enum VIA via)
{
    enum pcfetcher_request_method method;
    switch (via) {
        case VIA_GET:
            method = PCFETCHER_REQUEST_METHOD_GET;
            break;
        case VIA_POST:
            method = PCFETCHER_REQUEST_METHOD_POST;
            break;
        case VIA_DELETE:
            method = PCFETCHER_REQUEST_METHOD_DELETE;
            break;
        case VIA_UNDEFINED:
            method = PCFETCHER_REQUEST_METHOD_GET;
            break;
        default:
            // TODO VW: raise exception for no required value
            // PC_ASSERT(0);
            method = PCFETCHER_REQUEST_METHOD_GET;
            break;
    }

    return method;
}

bool
pcintr_match_exception(int except, purc_variant_t constant)
{
    bool ret = false;
    if (except == 0 || !constant || !pcvariant_is_sorted_array(constant)) {
        goto out;
    }

    purc_atom_t atom = purc_get_except_atom_by_id(except);
    if (!atom) {
        goto out;
    }

    purc_variant_t v = purc_variant_make_ulongint((uint64_t)atom);
    if (!v) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    ret = purc_variant_sorted_array_find(constant, v);
    purc_variant_unref(v);
out:
    return ret;
}

