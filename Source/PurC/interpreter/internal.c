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
"    <call on $%s with $REQ._args >\n"
"        $REQ._content\n"
"        <exit with $? />\n"
"    </call>\n"
"</hvml>\n";



static int
bind_at_element(purc_coroutine_t cor, struct pcvdom_element *elem,
        const char *name, purc_variant_t val)
{
    return pcintr_bind_scope_variable(cor, elem, name, val) ? 0 : -1 ;
}

static int
bind_at_coroutine(purc_coroutine_t cor, const char *name, purc_variant_t val)
{
    return purc_coroutine_bind_variable(cor, name, val) ? 0 : -1;
}

static int
bind_by_level(pcintr_stack_t stack, struct pcintr_stack_frame *frame,
        const char *name, purc_variant_t val, uint64_t level)
{
    bool silently = frame->silently;
    struct pcvdom_element *p = frame->pos;

    for (uint64_t i = 0; i < level; ++i) {
        if (p == NULL) {
            break;
        }
        p = pcvdom_element_parent(p);
    }

    if (p && p->node.type != PCVDOM_NODE_DOCUMENT) {
        return bind_at_element(stack->co, p, name, val);
    }

    if (silently) {
        return bind_at_coroutine(stack->co, name, val);
    }
    purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
            "no vdom element exists");
    return -1;
}

static int
bind_at_default(pcintr_stack_t stack, struct pcintr_stack_frame *frame,
        const char *name, purc_variant_t val)
{
    return bind_by_level(stack, frame, name, val, 1);
}

bool
pcintr_match_id(pcintr_stack_t stack, struct pcvdom_element *elem,
        const char *id)
{
    if (elem->node.type == PCVDOM_NODE_DOCUMENT) {
        return false;
    }
    struct pcvdom_attr *attr = pcvdom_element_find_attr(elem, "id");
    if (!attr) {
        return false;
    }

    bool silently = false;
    purc_variant_t v = pcvcm_eval(attr->val, stack, silently);
    purc_clr_error();
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
bind_by_elem_id(pcintr_stack_t stack, struct pcintr_stack_frame *frame,
        const char *id, const char *name, purc_variant_t val)
{
    struct pcvdom_element *p = frame->pos;
    struct pcvdom_element *dest = NULL;
    while (p) {
        if (pcintr_match_id(stack, p, id)) {
            dest = p;
            break;
        }
        p = pcvdom_element_parent(p);
    }

    if (dest && dest->node.type != PCVDOM_NODE_DOCUMENT) {
        return bind_at_element(stack->co, dest, name, val);
    }

    if (frame->silently) {
        return bind_at_default(stack, frame, name, val);
    }

    purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
            "no vdom element exists");
    return -1;
}

static int
bind_by_name_space(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame, const char *ns, const char *name,
        purc_variant_t val)
{
    purc_atom_t atom = PCHVML_KEYWORD_ATOM(HVML, ns);
    if (atom == 0) {
        goto not_found;
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _PARENT)) == atom) {
        return bind_by_level(stack, frame, name, val, 1);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _GRANDPARENT)) == atom ) {
        return bind_by_level(stack, frame, name, val, 2);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _ROOT)) == atom ) {
        return bind_at_coroutine(stack->co, name, val);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _LAST)) == atom) {
        return bind_by_level(stack, frame, name, val, 1);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _NEXTTOLAST)) == atom) {
        return bind_by_level(stack, frame, name, val, 2);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _TOPMOST)) == atom) {
        return bind_at_coroutine(stack->co, name, val);
    }

not_found:
    if (frame->silently) {
        return bind_at_default(stack, frame, name, val);
    }
    purc_set_error_with_info(PURC_ERROR_BAD_NAME,
            "at = '%s'", name);
    return -1;
}

int
pcintr_bind_named_variable(pcintr_stack_t stack,
        struct pcintr_stack_frame *frame, const char *name, purc_variant_t at,
        purc_variant_t v)
{
    int bind_ret = -1;
    if (!at) {
        bind_ret = bind_at_default(stack, frame, name, v);
        goto out;
    }

    if (purc_variant_is_string(at)) {
        const char *s_at = purc_variant_get_string_const(at);
        if (s_at[0] == '#') {
            bind_ret = bind_by_elem_id(stack, frame, s_at + 1,
                    name, v);
        }
        else if (s_at[0] == '_') {
            bind_ret = bind_by_name_space(stack, frame, s_at,
                    name, v);
        }
        else {
            uint64_t level;
            bool ok = purc_variant_cast_to_ulongint(at, &level,
                    true);
            if (ok) {
                bind_ret = bind_by_level(stack, frame, name, v, level);
            }
            else {
                bind_ret = bind_at_coroutine(stack->co, name, v);
            }
        }
    }
    else {
        uint64_t level;
        bool ok = purc_variant_cast_to_ulongint(at, &level, true);
        if (ok) {
            bind_ret = bind_by_level(stack, frame, name, v, level);
        }
        else {
            bind_ret = bind_at_coroutine(stack->co, name, v);
        }
    }

out:
    return bind_ret;
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

    as_var = pcintr_eval_vdom_attr(stack, as_attr);
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
