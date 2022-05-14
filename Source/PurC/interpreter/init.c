/**
 * @file init.c
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

#include "internal.h"

#include "private/debug.h"
#include "purc-runloop.h"

#include "ops.h"

#include <pthread.h>
#include <unistd.h>

enum VIA {
    VIA_LOAD,
    VIA_GET,
    VIA_POST,
    VIA_DELETE,
};

struct ctxt_for_init {
    struct pcvdom_node           *curr;

    purc_variant_t                as;
    purc_variant_t                at;
    purc_variant_t                from;
    purc_variant_t                from_result;
    purc_variant_t                with;
    purc_variant_t                against;

    purc_variant_t                literal;

    enum VIA                      via;

    unsigned int                  under_head:1;
    unsigned int                  temporarily:1;
    unsigned int                  async:1;
    unsigned int                  casesensitively:1;
    unsigned int                  uniquely:1;
};

struct fetcher_for_init {
    pcintr_stack_t                stack;
    struct pcvdom_element         *element;
    purc_variant_t                name;
    unsigned int                  under_head:1;
    pthread_t                     current;
};

static void
ctxt_for_init_destroy(struct ctxt_for_init *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->as);
        PURC_VARIANT_SAFE_CLEAR(ctxt->at);
        PURC_VARIANT_SAFE_CLEAR(ctxt->from);
        PURC_VARIANT_SAFE_CLEAR(ctxt->from_result);
        PURC_VARIANT_SAFE_CLEAR(ctxt->with);
        PURC_VARIANT_SAFE_CLEAR(ctxt->against);
        PURC_VARIANT_SAFE_CLEAR(ctxt->literal);
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_init_destroy((struct ctxt_for_init*)ctxt);
}

#define UNDEFINED       "undefined"

static int
post_process_bind_at_frame(pcintr_coroutine_t co, struct ctxt_for_init *ctxt,
        struct pcintr_stack_frame *frame, purc_variant_t src)
{
    UNUSED_PARAM(co);

    purc_variant_t name = ctxt->as;

    if (name == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    if (purc_variant_is_string(name) == false) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return -1;
    }

    purc_variant_t exclamation_var;
    exclamation_var = pcintr_get_exclamation_var(frame);
    if (purc_variant_is_object(exclamation_var) == false) {
        purc_set_error_with_info(PURC_ERROR_INTERNAL_FAILURE,
                "temporary variable on stack frame is not object");
        return -1;
    }

    bool ok = purc_variant_object_set(exclamation_var, name, src);
    if (ok)
        purc_clr_error();
    return ok ? 0 : -1;
}

static const char*
get_name(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    purc_variant_t name = ctxt->as;

    if (name == PURC_VARIANT_INVALID) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    if (purc_variant_is_string(name) == false) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        return NULL;
    }

    const char *s_name = purc_variant_get_string_const(name);
    return s_name;
}

static int
post_process_bind_at_vdom(pcintr_coroutine_t co,
        struct pcintr_stack_frame *frame,
        struct pcvdom_element *elem, purc_variant_t src)
{
    UNUSED_PARAM(co);

    const char *s_name = get_name(co, frame);
    if (!s_name)
        return -1;

    bool ok;
    ok = pcintr_bind_scope_variable(elem, s_name, src);
    return ok ? 0 : -1;
}

static int
post_process_src_by_level(pcintr_coroutine_t co,
        struct pcintr_stack_frame *frame, purc_variant_t src, uint64_t level)
{
    PC_ASSERT(level > 0);

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    bool silently = frame->silently;

    if (ctxt->temporarily) {
        struct pcintr_stack_frame *p = frame;
        struct pcintr_stack_frame *parent;
        parent = pcintr_stack_frame_get_parent(frame);
        if (parent == NULL) {
            purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
                    "no frame exists");
            return -1;
        }
        for (uint64_t i=0; i<level; ++i) {
            if (p == NULL)
                break;
            p = pcintr_stack_frame_get_parent(p);
        }
        if (p == NULL) {
            if (!silently) {
                purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
                        "no frame exists");
                return -1;
            }
            p = parent;
        }
        return post_process_bind_at_frame(co, ctxt, p, src);
    }
    else {
        struct pcvdom_element *p = frame->pos;
        struct pcvdom_element *parent = NULL;
        if (p)
            parent = pcvdom_element_parent(p);

        if (parent == NULL) {
            purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
                    "no vdom element exists");
            return -1;
        }
        for (uint64_t i=0; i<level; ++i) {
            if (p == NULL)
                break;
            p = pcvdom_element_parent(p);
        }
        if (p == NULL) {
            if (!silently) {
                purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
                        "no vdom element exists");
                return -1;
            }
            p = parent;
        }
        return post_process_bind_at_vdom(co, frame, p, src);
    }
}

static bool
match_id(pcintr_coroutine_t co,
        struct pcvdom_element *elem, const char *id)
{
    struct pcvdom_attr *attr;
    attr = pcvdom_element_find_attr(elem, "id");
    if (!attr)
        return false;

    bool silently = false;
    purc_variant_t v = pcvcm_eval(attr->val, &co->stack, silently);
    purc_clr_error();
    if (v == PURC_VARIANT_INVALID)
        return false;

    bool matched = false;

    do {
        if (purc_variant_is_string(v) == false)
            break;
        const char *sv = purc_variant_get_string_const(v);
        if (!sv)
            break;

        if (strcmp(sv, id) == 0)
            matched = true;
    } while (0);

    purc_variant_unref(v);

    return matched;
}

static int
post_process_src_by_id(pcintr_coroutine_t co,
        struct pcintr_stack_frame *frame, purc_variant_t src, const char *id)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    bool silently = frame->silently;

    if (ctxt->temporarily) {
        purc_set_error(PURC_ERROR_NOT_IMPLEMENTED);
        return -1;
    }
    else {
        struct pcvdom_element *p = frame->pos;
        struct pcvdom_element *parent = NULL;
        if (p)
            parent = pcvdom_element_parent(p);

        if (parent == NULL) {
            purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
                    "no vdom element exists");
            return -1;
        }
        while (p) {
            if (p == NULL)
                break;
            if (match_id(co, p, id))
                break;
            p = pcvdom_element_parent(p);
        }
        if (p == NULL) {
            if (!silently) {
                purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
                        "no vdom element exists");
                return -1;
            }
            p = parent;
            if (match_id(co, p, id) == false) {
                purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
                        "no vdom element exists");
                return -1;
            }
        }
        return post_process_bind_at_vdom(co, frame, p, src);
    }
}

static int
post_process_src_by_topmost(pcintr_coroutine_t co,
        struct pcintr_stack_frame *frame, purc_variant_t src)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    if (ctxt->temporarily) {
        struct pcintr_stack_frame *p = frame;
        struct pcintr_stack_frame *parent;
        parent = pcintr_stack_frame_get_parent(p);
        uint64_t level = 0;
        while (parent) {
            p = parent;
            parent = pcintr_stack_frame_get_parent(p);
            level += 1;
        }
        PC_ASSERT(level > 0);
        return post_process_src_by_level(co, frame, src, level);
    }
    else {
        const char *s_name = get_name(co, frame);
        if (!s_name)
            return -1;
        bool ok;
        ok = purc_bind_document_variable(co->stack.vdom, s_name, src);
        return ok ? 0 : -1;
    }
}

static int
post_process_src_by_atom(pcintr_coroutine_t co,
        struct pcintr_stack_frame *frame, purc_variant_t src, purc_atom_t atom)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _PARENT)) == atom) {
        if (ctxt->temporarily) {
            purc_set_error_with_info(PURC_ERROR_BAD_NAME,
                    "at = '%s' conflicts with temporarily",
                    purc_atom_to_string(atom));
            return -1;
        }
        return post_process_src_by_level(co, frame, src, 1);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _GRANDPARENT)) == atom ) {
        if (ctxt->temporarily) {
            purc_set_error_with_info(PURC_ERROR_BAD_NAME,
                    "at = '%s' conflicts with temporarily",
                    purc_atom_to_string(atom));
            return -1;
        }
        return post_process_src_by_level(co, frame, src, 2);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _ROOT)) == atom ) {
        if (ctxt->temporarily) {
            purc_set_error_with_info(PURC_ERROR_BAD_NAME,
                    "at = '%s' conflicts with temporarily",
                    purc_atom_to_string(atom));
            return -1;
        }
        return post_process_src_by_topmost(co, frame, src);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _LAST)) == atom) {
        ctxt->temporarily = 1;
        return post_process_src_by_level(co, frame, src, 1);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _NEXTTOLAST)) == atom) {
        ctxt->temporarily = 1;
        return post_process_src_by_level(co, frame, src, 2);
    }

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, _TOPMOST)) == atom) {
        ctxt->temporarily = 1;
        return post_process_src_by_topmost(co, frame, src);
    }

    purc_set_error_with_info(PURC_ERROR_BAD_NAME,
            "at = '%s'", purc_atom_to_string(atom));
    return -1;
}

static int
post_process_src(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        purc_variant_t src)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    if (ctxt->at == PURC_VARIANT_INVALID) {
        const char *s_name = get_name(co, frame);
        if (!s_name)
            return -1;

        if (ctxt->under_head) {
            uint64_t level = 0;
            struct pcvdom_node *node = &frame->pos->node;
            while (node && node != &co->stack.vdom->document->node) {
                node = pcvdom_node_parent(node);
                level += 1;
            }
            if (node == NULL) {
                purc_set_error_with_info(PURC_ERROR_INTERNAL_FAILURE,
                        "<init> not under vdom Document");
                return -1;
            }
            bool ok;
            ok = purc_bind_document_variable(co->stack.vdom, s_name, src);
            return ok ? 0 : -1;
        }
        return post_process_src_by_level(co, frame, src, 1);
    }

    if (purc_variant_is_string(ctxt->at)) {
        const char *s_at = purc_variant_get_string_const(ctxt->at);
        if (s_at[0] == '#')
            return post_process_src_by_id(co, frame, src, s_at+1);
        else if (s_at[0] == '_') {
            purc_atom_t atom = PCHVML_KEYWORD_ATOM(HVML, s_at);
            if (atom == 0) {
                purc_set_error_with_info(PURC_ERROR_BAD_NAME,
                        "at = '%s'", s_at);
                return -1;
            }
            return post_process_src_by_atom(co, frame, src, atom);
        }
    }
    bool ok;
    bool force = true;
    uint64_t level;
    ok = purc_variant_cast_to_ulongint(ctxt->at, &level, force);
    if (!ok)
        return -1;
    return post_process_src_by_level(co, frame, src, level);
}

static int
post_process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    purc_variant_t src = frame->ctnt_var;

    if (ctxt->against != PURC_VARIANT_INVALID) {
        PC_ASSERT(purc_variant_is_string(ctxt->against));

        purc_variant_t set;
        set = purc_variant_make_set(0, ctxt->against, PURC_VARIANT_INVALID);
        if (set == PURC_VARIANT_INVALID)
            return -1;

        if (!purc_variant_container_displace(set, src, frame->silently)) {
            purc_variant_unref(set);
            return -1;
        }

        src = set;
    }
    else {
        purc_variant_ref(src);
    }

    int r = post_process_src(co, frame, src);
    purc_variant_unref(src);

    return r ? -1 : 0;
}

static int
process_attr_as(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
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
process_attr_at(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
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
process_attr_from(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
    if (ctxt->from != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_DUPLICATED,
                "vdom attribute '%s' for element <%s>",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    if (ctxt->with != PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_NOT_SUPPORTED,
                "vdom attribute '%s' for element <%s> conflicts with '%s'",
                purc_atom_to_string(name), element->tag_name,
                pchvml_keyword_str(PCHVML_KEYWORD_ENUM(HVML, FROM)));
        return -1;
    }
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    ctxt->from = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_with(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
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
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
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
process_attr_via(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
    if (val == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "vdom attribute '%s' for element <%s> undefined",
                purc_atom_to_string(name), element->tag_name);
        return -1;
    }
    const char *s_val = purc_variant_get_string_const(val);
    if (!s_val)
        return -1;

    if (strcmp(s_val, "LOAD") == 0) {
        ctxt->via = VIA_LOAD;
        return 0;
    }

    if (strcmp(s_val, "GET") == 0) {
        ctxt->via = VIA_GET;
        return 0;
    }

    if (strcmp(s_val, "POST") == 0) {
        ctxt->via = VIA_POST;
        return 0;
    }

    if (strcmp(s_val, "DELETE") == 0) {
        ctxt->via = VIA_DELETE;
        return 0;
    }

    purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
            "unknown vdom attribute '%s = %s' for element <%s>",
            purc_atom_to_string(name), s_val, element->tag_name);
    return -1;
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

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AS)) == name) {
        return process_attr_as(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AT)) == name) {
        return process_attr_at(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, UNIQUELY)) == name) {
        PC_ASSERT(purc_variant_is_undefined(val));
        ctxt->uniquely = 1;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CASESENSITIVELY)) == name) {
        PC_ASSERT(purc_variant_is_undefined(val));
        ctxt->casesensitively= 1;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, CASEINSENSITIVELY)) == name) {
        PC_ASSERT(purc_variant_is_undefined(val));
        ctxt->casesensitively= 0;
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, FROM)) == name) {
        return process_attr_from(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITH)) == name) {
        return process_attr_with(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AGAINST)) == name) {
        return process_attr_against(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, VIA)) == name) {
        return process_attr_via(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, TEMPORARILY)) == name ||
            pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, TEMP)) == name)
    {
        PC_ASSERT(purc_variant_is_undefined(val));
        ctxt->temporarily = 1;
        if (ctxt->async) {
            purc_log_warn("'asynchronously' is ignored because of 'temporarily'");
            ctxt->async = 0;
        }
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ASYNCHRONOUSLY)) == name
            || pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ASYNC)) == name) {
        PC_ASSERT(purc_variant_is_undefined(val));
        ctxt->async = 1;
        if (ctxt->temporarily) {
            purc_log_warn("'asynchronously' is ignored because of 'temporarily'");
            ctxt->async = 0;
        }
        return 0;
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SYNCHRONOUSLY)) == name
            || pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, SYNC)) == name) {
        PC_ASSERT(purc_variant_is_undefined(val));
        ctxt->async = 0;
        return 0;
    }

    purc_set_error_with_info(PURC_ERROR_NOT_IMPLEMENTED,
            "unknown vdom attribute '%s' for element <%s>",
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
    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_OPERATOR);
    if (!name) {
        purc_set_error_with_info(PURC_ERROR_NOT_IMPLEMENTED,
                "unknown vdom attribute '%s' for element <%s>",
                attr->key, element->tag_name);
        return -1;
    }

    purc_variant_t val = pcintr_eval_vdom_attr(pcintr_get_stack(), attr);
    if (val == PURC_VARIANT_INVALID)
        return -1;

    int r = attr_found_val(frame, element, name, val, attr, ud);
    purc_variant_unref(val);

    return r ? -1 : 0;
}

static void load_response_handler(purc_variant_t request_id, void *ctxt,
        const struct pcfetcher_resp_header *resp_header,
        purc_rwstream_t resp)
{
    PC_DEBUG("load_async|callback|ret_code=%d\n", resp_header->ret_code);
    PC_DEBUG("load_async|callback|mime_type=%s\n", resp_header->mime_type);
    PC_DEBUG("load_async|callback|sz_resp=%ld\n", resp_header->sz_resp);
    struct fetcher_for_init *fetcher = (struct fetcher_for_init*)ctxt;
    pthread_t current = pthread_self();
    PC_ASSERT(current == fetcher->current);

    if (resp_header->ret_code == RESP_CODE_USER_STOP) {
        goto clean_rws;
    }

    pcintr_remove_async_request_id(fetcher->stack, request_id);
    bool has_except = false;
    if (!resp || resp_header->ret_code != 200) {
        has_except = true;
        goto dispatch_except;
    }

    bool ok;
    struct pcvdom_element *element = fetcher->element;
    purc_variant_t ret = purc_variant_load_from_json_stream(resp);
    const char *s_name = purc_variant_get_string_const(fetcher->name);
    if (ret != PURC_VARIANT_INVALID) {
        if (fetcher->under_head) {
            ok = purc_bind_document_variable(fetcher->stack->vdom, s_name,
                    ret);
        } else {
            element = pcvdom_element_parent(element);
            ok = pcintr_bind_scope_variable(element, s_name, ret);
        }
        purc_variant_unref(ret);
        if (ok) {
            goto clean_rws;
        }
        has_except = true;
        goto dispatch_except;
    }
    else {
        has_except = true;
        goto dispatch_except;
    }

dispatch_except:
    if (has_except) {
        purc_atom_t atom = purc_get_error_exception(purc_get_last_error());
        pcvarmgr_t varmgr;
        if (fetcher->under_head) {
            varmgr = pcvdom_document_get_variables(fetcher->stack->vdom);
        }
        else {
            element = pcvdom_element_parent(element);
            varmgr = pcvdom_element_get_variables(element);
        }
        pcvarmgr_dispatch_except(varmgr, s_name, purc_atom_to_string(atom));
    }

clean_rws:
    if (resp) {
        purc_rwstream_destroy(resp);
    }

    if (request_id != PURC_VARIANT_INVALID) {
        purc_variant_unref(request_id);
    }
    free(fetcher);
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    PC_ASSERT(stack && pos);
    PC_ASSERT(stack == pcintr_get_stack());


    if (stack->except)
        return NULL;

    if (pcintr_check_insertion_mode_for_normal_element(stack))
        return NULL;

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    PC_ASSERT(frame && frame->pos);

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    ctxt->casesensitively = 1;

    frame->ctxt = ctxt;
    frame->ctxt_destroy = ctxt_destroy;

    frame->pos = pos; // ATTENTION!!

    frame->attr_vars = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (frame->attr_vars == PURC_VARIANT_INVALID)
        return NULL;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    int r;
    r = pcintr_vdom_walk_attrs(frame, element, NULL, attr_found);
    if (r)
        return NULL;

    if (ctxt->temporarily) {
        ctxt->async = 0;
    }

    while ((element=pcvdom_element_parent(element))) {
        if (element->tag_id == PCHVML_TAG_HEAD) {
            ctxt->under_head = 1;
        }
    }

    purc_clr_error(); // pcvdom_element_parent

    purc_variant_t from = ctxt->from;
    if (from != PURC_VARIANT_INVALID && purc_variant_is_string(from)
            && pcfetcher_is_init()) {
        const char* uri = purc_variant_get_string_const(from);
        if (!ctxt->async) {
            purc_variant_t v = pcintr_load_from_uri(stack, uri);
            if (v == PURC_VARIANT_INVALID)
                return NULL;
            PURC_VARIANT_SAFE_CLEAR(ctxt->from_result);
            ctxt->from_result = v;
        }
        else {
//            PC_ASSERT(0); // TODO: test
            struct fetcher_for_init *fetcher = (struct fetcher_for_init*)
                malloc(sizeof(struct fetcher_for_init));
            if (!fetcher) {
                purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
                return NULL;
            }
            fetcher->stack = stack;
            fetcher->element = element;
            fetcher->name = ctxt->as;
            fetcher->current = pthread_self();
            purc_variant_ref(fetcher->name);
            fetcher->under_head = ctxt->under_head;
            purc_variant_t v = pcintr_load_from_uri_async(stack, uri,
                    load_response_handler, fetcher);
            if (v == PURC_VARIANT_INVALID)
                return NULL;
            pcintr_save_async_request_id(stack, v);
        }
    }

    if (r)
        return NULL;

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);
    PC_ASSERT(stack == pcintr_get_stack());

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);
    PC_ASSERT(ud == frame->ctxt);

    if (frame->ctxt == NULL)
        return true;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
    if (ctxt) {
        ctxt_for_init_destroy(ctxt);
        frame->ctxt = NULL;
    }

    return true;
}

static int
on_element(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_element *element)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(element);

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
    PC_ASSERT(ctxt);

    if (ctxt->from || ctxt->with) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "no element is permitted "
                "since `from/with` attribute already set");
        return -1;
    }

    return 0;
}

static int
on_content(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_content *content)
{
    UNUSED_PARAM(co);
    PC_ASSERT(content);

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
    PC_ASSERT(ctxt);

    struct pcvcm_node *vcm = content->vcm;
    if (!vcm)
        return 0;

    // FIXME
    if ((ctxt->from && !ctxt->async) || ctxt->with) {
        purc_set_error_with_info(PURC_ERROR_INVALID_VALUE,
                "no content is permitted "
                "since there's no `from/with` attribute");
        return -1;
    }

    // NOTE: element is still the owner of vcm_content
    purc_variant_t v = pcvcm_eval(vcm, &co->stack, frame->silently);
    if (v == PURC_VARIANT_INVALID)
        return -1;

    PURC_VARIANT_SAFE_CLEAR(ctxt->literal);
    ctxt->literal = v;

    return 0;
}

static int
on_comment(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
        struct pcvdom_comment *comment)
{
    UNUSED_PARAM(co);
    UNUSED_PARAM(frame);
    PC_ASSERT(comment);
    return 0;
}

static int
on_child_finished(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;
    PC_ASSERT(ctxt);

    if (ctxt->from) {
        if (ctxt->from_result != PURC_VARIANT_INVALID) {
            PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);
            frame->ctnt_var = ctxt->from_result;
            purc_variant_ref(ctxt->from_result);
            return post_process(co, frame);
        }
    }
    if (!ctxt->from && ctxt->with) {
        PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);
        frame->ctnt_var = ctxt->with;
        purc_variant_ref(ctxt->with);
        return post_process(co, frame);
    }

    if (ctxt->literal == PURC_VARIANT_INVALID) {
        ctxt->literal = purc_variant_make_undefined();
    }

    if (ctxt->literal != PURC_VARIANT_INVALID) {
        frame->ctnt_var = ctxt->literal;
        purc_variant_ref(ctxt->literal);
        return post_process(co, frame);
    }

    // FIXME:
    if (ctxt->async) {
        return 0;
    }

    purc_set_error_with_info(PURC_ERROR_ENTITY_NOT_FOUND,
            "no value defined for <init>");
    return -1;
}

static pcvdom_element_t
select_child(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);
    PC_ASSERT(stack == pcintr_get_stack());

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

    struct ctxt_for_init *ctxt;
    ctxt = (struct ctxt_for_init*)frame->ctxt;

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
        on_child_finished(co, frame);
        return NULL;
    }

    switch (curr->type) {
        case PCVDOM_NODE_DOCUMENT:
            PC_ASSERT(0); // Not implemented yet
            break;
        case PCVDOM_NODE_ELEMENT:
            {
                pcvdom_element_t element = PCVDOM_ELEMENT_FROM_NODE(curr);
                if (on_element(co, frame, element))
                    return NULL;
                return element;
            }
        case PCVDOM_NODE_CONTENT:
            if (on_content(co, frame, PCVDOM_CONTENT_FROM_NODE(curr)))
                return NULL;
            goto again;
        case PCVDOM_NODE_COMMENT:
            if (on_comment(co, frame, PCVDOM_COMMENT_FROM_NODE(curr)))
                return NULL;
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

struct pcintr_element_ops* pcintr_get_init_ops(void)
{
    return &ops;
}

