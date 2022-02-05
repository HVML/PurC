/**
 * @file update.c
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
#include "private/dvobjs.h"
#include "private/runloop.h"

#include "html/interfaces/document.h"

#include "ops.h"

#include <pthread.h>
#include <unistd.h>
#include <libgen.h>

#define TO_DEBUG 1

struct ctxt_for_update {
    struct pcvdom_node           *curr;

    purc_variant_t                on;
    purc_variant_t                to;
    purc_variant_t                at;
    purc_variant_t                from;
    purc_variant_t                with;
    enum pchvml_attr_assignment   with_op;
    purc_variant_t                src;
    enum pchvml_attr_assignment   src_op;
};

static void
ctxt_for_update_destroy(struct ctxt_for_update *ctxt)
{
    if (ctxt) {
        PURC_VARIANT_SAFE_CLEAR(ctxt->on);
        PURC_VARIANT_SAFE_CLEAR(ctxt->to);
        PURC_VARIANT_SAFE_CLEAR(ctxt->at);
        PURC_VARIANT_SAFE_CLEAR(ctxt->from);
        PURC_VARIANT_SAFE_CLEAR(ctxt->with);
        PURC_VARIANT_SAFE_CLEAR(ctxt->src);
        free(ctxt);
    }
}

static void
ctxt_destroy(void *ctxt)
{
    ctxt_for_update_destroy((struct ctxt_for_update*)ctxt);
}

static purc_variant_t
get_source_by_with(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
    purc_variant_t with)
{
    UNUSED_PARAM(frame);
    if (purc_variant_is_type(with, PURC_VARIANT_TYPE_ULONGINT)) {
        bool ok;
        uint64_t u64;
        ok = purc_variant_cast_to_ulongint(with, &u64, false);
        PC_ASSERT(ok);
        struct pcvcm_node *vcm_content;
        vcm_content = (struct pcvcm_node*)u64;
        PC_ASSERT(vcm_content);

        pcintr_stack_t stack = co->stack;
        PC_ASSERT(stack);

        purc_variant_t v = pcvcm_eval(vcm_content, stack);
        if (v == PURC_VARIANT_INVALID)
            PRINT_VCM_NODE(vcm_content);
        return v;
    }
    else if (purc_variant_is_type(with, PURC_VARIANT_TYPE_STRING)) {
        purc_variant_ref(with);
        return with;
    }
    else {
        purc_variant_ref(with);
        return with;
    }
}

static purc_variant_t
get_source_by_from(pcintr_coroutine_t co, struct pcintr_stack_frame *frame,
    purc_variant_t from, purc_variant_t with)
{
    PC_ASSERT(0);
    UNUSED_PARAM(frame);
    PC_ASSERT(with == PURC_VARIANT_INVALID);

    const char* uri = purc_variant_get_string_const(from);
    return pcintr_load_from_uri(co->stack, uri);
}

static int
merge_object(pcintr_stack_t stack,
        purc_variant_t on, purc_variant_t at,
        purc_variant_t src)
{
    UNUSED_PARAM(stack);

    const char *s_at = "";
    if (at != PURC_VARIANT_INVALID) {
        PC_ASSERT(purc_variant_is_string(at));
        s_at = purc_variant_get_string_const(at);
    }

    if (s_at[0] == '\0') {
        bool ok;
        ok = purc_variant_object_merge_another(on, src, true);
        if (!ok) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
        return 0;
    }

    D("s_at: %s", s_at);
    PC_ASSERT(0);
    return -1;
}

static int
displace_object(pcintr_stack_t stack,
        purc_variant_t on, purc_variant_t at,
        purc_variant_t src)
{
    UNUSED_PARAM(stack);

    const char *s_at = "";
    if (at != PURC_VARIANT_INVALID) {
        PC_ASSERT(purc_variant_is_string(at));
        s_at = purc_variant_get_string_const(at);
    }

    if (s_at[0] == '.') {
        s_at += 1;
        purc_variant_t k = purc_variant_make_string(s_at, true);
        if (k == PURC_VARIANT_INVALID)
            return -1;
        bool ok;
        ok = purc_variant_object_set(on, k, src);
        purc_variant_unref(k);
        if (!ok) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
        return 0;
    }

    D("s_at: %s", s_at);
    PC_ASSERT(0);
    return -1;
}

static int
update_object(pcintr_stack_t stack,
        purc_variant_t on, purc_variant_t at, purc_variant_t to,
        purc_variant_t src)
{
    const char *s_to = "displace";
    if (to != PURC_VARIANT_INVALID) {
        PC_ASSERT(purc_variant_is_string(to));
        s_to = purc_variant_get_string_const(to);
    }

    if (strcmp(s_to, "merge") == 0) {
        return merge_object(stack, on, at, src);
    }

    if (strcmp(s_to, "displace") == 0) {
        return displace_object(stack, on, at, src);
    }

    D("s_to: %s", s_to);
    PC_ASSERT(0);
    return -1;
}

#if 0
static int
update_object(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    PC_ASSERT(ctxt);
    purc_variant_t on  = ctxt->on;
    purc_variant_t to  = ctxt->to;
    purc_variant_t src = ctxt->src;
    PC_ASSERT(on != PURC_VARIANT_INVALID);
    PC_ASSERT(to != PURC_VARIANT_INVALID);
    PC_ASSERT(src != PURC_VARIANT_INVALID);

    purc_variant_t target = on;
    purc_variant_t at  = ctxt->at;
    if (at != PURC_VARIANT_INVALID) {
        PC_ASSERT(purc_variant_is_string(at));
        const char *s_at = purc_variant_get_string_const(at);
        PC_ASSERT(s_at && s_at[0]=='.');
        s_at += 1;
        purc_variant_t v = purc_variant_object_get_by_ckey(on, s_at, false);
        PC_ASSERT(v != PURC_VARIANT_INVALID);
        target = v;
        return -1;
    }

    const char *op = purc_variant_get_string_const(to);
    PC_ASSERT(op);
    if (strcmp(op, "merge")==0) {
    // TODO
        if (!purc_variant_object_merge_another(target, src, true)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
        return 0;
    }
    if (strcmp(op, "displace")==0) {
        PC_ASSERT(0); // Not implemented yet
        purc_variant_t at = ctxt->at;
        bool ok = purc_variant_object_set(on, at, src);
        return ok ? 0 : -1;
    }
    PC_ASSERT(0); // Not implemented yet
    return -1;
}
#endif

static int
update_array(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    PC_ASSERT(0);
    UNUSED_PARAM(co);
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    PC_ASSERT(ctxt);
    purc_variant_t on  = ctxt->on;
    purc_variant_t to  = ctxt->to;
    purc_variant_t src = ctxt->src;
    PC_ASSERT(on != PURC_VARIANT_INVALID);
    PC_ASSERT(to != PURC_VARIANT_INVALID);
    PC_ASSERT(src != PURC_VARIANT_INVALID);

    purc_variant_t target = on;
    purc_variant_t at  = ctxt->at;
    if (at != PURC_VARIANT_INVALID) {
        double d = purc_variant_numberify(at);
        size_t idx = d;
        purc_variant_t v = purc_variant_array_get(on, idx);
        PC_ASSERT(v != PURC_VARIANT_INVALID);
        if (v == PURC_VARIANT_INVALID)
            return -1;
        PC_ASSERT(v != PURC_VARIANT_INVALID); // Not implemented yet
        target = v;
    }

    const char *op = purc_variant_get_string_const(to);
    PC_ASSERT(op);

    if (strcmp(op, "append") == 0) {
        bool ok = purc_variant_array_append(target, src);
        return ok ? 0 : -1;
    }

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    purc_set_error_with_info(PURC_ERROR_NOT_SUPPORTED,
            "vdom attribute '%s'='%s' for element <%s>",
            pchvml_keyword_str(PCHVML_KEYWORD_ENUM(HVML, TO)),
            op, element->tag_name);
    PC_ASSERT(0);

    return -1;
}

static int
update_set(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    PC_ASSERT(ctxt);
    purc_variant_t on  = ctxt->on;
    purc_variant_t to  = ctxt->to;
    purc_variant_t src = ctxt->src;
    PC_ASSERT(on != PURC_VARIANT_INVALID);
    PC_ASSERT(to != PURC_VARIANT_INVALID);
    PC_ASSERT(src != PURC_VARIANT_INVALID);

    purc_variant_t at  = ctxt->at;
    if (at != PURC_VARIANT_INVALID) {
        PC_ASSERT(0); // Not implemented yet
        return -1;
    }

    const char *op = purc_variant_get_string_const(to);
    PC_ASSERT(op);
    if (strcmp(op, "displace")==0) {
        if (!purc_variant_is_type(src, PURC_VARIANT_TYPE_ARRAY)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }
        if (!purc_variant_is_type(on, PURC_VARIANT_TYPE_SET)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }

    // TODO
#if 1
        if (!purc_variant_container_displace(on, src, true)) {
            return -1;
        }
#else
        purc_variant_t v;
        foreach_value_in_variant_set_safe(on, v)
            bool ok = purc_variant_set_remove(on, v, false);
            PC_ASSERT(ok); // FIXME: debug-only-now
        end_foreach;

        size_t curr;
        foreach_value_in_variant_array(src, v, curr)
            (void)curr;
            char buf[1024];
            pcvariant_serialize(buf, sizeof(buf), v);
            D("v: %s", buf);
            bool ok = purc_variant_set_add(on, v, true);
            if (!ok)
                return -1;
        end_foreach;
#endif
        return 0;
    }
    if (strcmp(op, "unite")==0) {
        if (!purc_variant_is_type(on, PURC_VARIANT_TYPE_SET)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }

    // TODO
#if 1
        if (!purc_variant_set_unite(on, src, true)) {
            return -1;
        }
#endif
        return 0;
    }
    if (strcmp(op, "overwrite")==0) {
        if (!purc_variant_is_type(on, PURC_VARIANT_TYPE_SET)) {
            purc_set_error(PURC_ERROR_INVALID_VALUE);
            return -1;
        }

    // TODO
#if 1
        if (!purc_variant_set_overwrite(on, src, true)) {
            return -1;
        }
#endif
        return 0;
    }
    D("op: %s", op);
    PC_ASSERT(0); // Not implemented yet
    return -1;
}

__attribute__ ((format (printf, 5, 6)))
static int
update_element(pcintr_stack_t stack,
        purc_variant_t on, purc_variant_t at, purc_variant_t to,
        const char *fmt, ...);

static int
update_element_content(pcintr_stack_t stack,
        pcdom_element_t *target, const char *to,
        const char *fragment_chunk, size_t nr)
{
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;

    PC_ASSERT(ctxt->src_op == PCHVML_ATTRIBUTE_ASSIGNMENT);

    pcdom_node_t *fragment;
    fragment = pcintr_parse_fragment(stack, fragment_chunk, nr);
    if (!fragment)
        return -1;

    if (strcmp(to, "displace")==0) {
        pcdom_displace_fragment(pcdom_interface_node(target), fragment);
        pcintr_dump_edom_node(stack, pcdom_interface_node(target));
        return 0;
    }

    if (strcmp(to, "append")==0) {
        pcintr_dump_edom_node(stack, pcdom_interface_node(target));
        pcintr_dump_edom_node(stack, fragment);
        pcdom_merge_fragment_append(pcdom_interface_node(target), fragment);
        pcintr_dump_edom_node(stack, pcdom_interface_node(target));
        return 0;
    }

    D("to: %s", to);
    PC_ASSERT(0);
    return -1;
}

static int
update_element_attr(pcintr_stack_t stack,
        pcdom_element_t *target, const char *attr_name, const char *to,
        const char *fragment_chunk, size_t nr)
{
    PC_ASSERT(*attr_name);

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;

    if (strcmp(to, "displace") == 0) {
        PC_ASSERT(fragment_chunk[nr] == '\0');
        if (ctxt->src_op == PCHVML_ATTRIBUTE_ASSIGNMENT) {
            int r = pcdom_element_set_attr(target, attr_name, fragment_chunk);
            pcintr_dump_edom_node(stack, pcdom_interface_node(target));
            return r ? -1 : 0;
        }
        if (ctxt->src_op == PCHVML_ATTRIBUTE_TAIL_ASSIGNMENT) {
            const unsigned char *s;
            size_t len;
            s = pcdom_element_get_attribute(target,
                (const unsigned char*)attr_name, strlen(attr_name),
                &len);
            PC_ASSERT(s);
            char buf[1024];
            size_t nr = sizeof(buf);
            char *p;
            p = pcutils_snprintf(buf, &nr,
                    "%.*s%s", (int)len, (const char*)s, fragment_chunk);
            PC_ASSERT(p);
            int r = pcdom_element_set_attr(target, attr_name, p);
            if (p != buf)
                free(p);
            pcintr_dump_edom_node(stack, pcdom_interface_node(target));
            return r ? -1 : 0;
        }
        D("op: 0x%x", ctxt->src_op);
        PC_ASSERT(0);
    }

    D("to: %s", to);
    PC_ASSERT(0);
    return -1;
}

static int
update_element_with_fragment(pcintr_stack_t stack,
        pcdom_element_t *target, purc_variant_t at, purc_variant_t to,
        const char *fragment_chunk, size_t nr)
{
    const char *s_to = "displace";
    if (to != PURC_VARIANT_INVALID) {
        PC_ASSERT(purc_variant_is_string(to));
        s_to = purc_variant_get_string_const(to);
    }

    if (at == PURC_VARIANT_INVALID) {
        return update_element_content(stack, target, s_to,
                fragment_chunk, nr);
    }

    PC_ASSERT(purc_variant_is_string(at));
    const char *s_at = purc_variant_get_string_const(at);
    if (strcmp(s_at, "textContent") == 0) {
        return update_element_content(stack, target, s_to, fragment_chunk, nr);
    }
    if (strncmp(s_at, "attr.", 5) == 0) {
        s_at += 5;
        return update_element_attr(stack, target, s_at, s_to,
                fragment_chunk, nr);
    }

    PRINT_VARIANT(at);
    PC_ASSERT(0);
    return -1;
}

static int
update_element(pcintr_stack_t stack,
        purc_variant_t on, purc_variant_t at, purc_variant_t to,
        const char *fmt, ...)
{
    char buf[1024];
    size_t nr = sizeof(buf);
    va_list ap;
    va_start(ap, fmt);
    char *p = pcutils_vsnprintf(buf, &nr, fmt, ap);
    va_end(ap);

    if (!p) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return -1;
    }

    int r = 0;

    PC_ASSERT(purc_variant_is_native(on));
    size_t idx = 0;
    while (1) {
        struct pcdom_element *target;
        target = pcdvobjs_get_element_from_elements(on, idx++);
        if (!target)
            break;
        r = update_element_with_fragment(stack, target, at, to, p, nr);
        if (r)
            break;
    }

    if (p != buf)
        free(p);

    return r ? -1 : 0;
}

static int
process(pcintr_coroutine_t co, struct pcintr_stack_frame *frame)
{
    UNUSED_PARAM(co);
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    PC_ASSERT(ctxt);
    purc_variant_t on  = ctxt->on;
    purc_variant_t to  = ctxt->to;
    purc_variant_t at  = ctxt->at;
    purc_variant_t src = ctxt->src;
    PC_ASSERT(on != PURC_VARIANT_INVALID);
    PC_ASSERT(src != PURC_VARIANT_INVALID);

    /* FIXME: what if array of elements? */
    enum purc_variant_type type = purc_variant_get_type(on);
    if (type == PURC_VARIANT_TYPE_NATIVE) {
        const char *s = purc_variant_get_string_const(src);
        PC_ASSERT(to != PURC_VARIANT_INVALID);
        return update_element(co->stack, on, at, to, "%s", s);
    }
    if (type == PURC_VARIANT_TYPE_OBJECT) {
        return update_object(co->stack, on, at, to, src);
    }
    if (type == PURC_VARIANT_TYPE_ARRAY) {
        return update_array(co, frame);
    }
    if (type == PURC_VARIANT_TYPE_SET) {
        return update_set(co, frame);
    }
    if (type == PURC_VARIANT_TYPE_STRING) {
        const char *s = purc_variant_get_string_const(on);
        PC_ASSERT(s);
        PC_ASSERT(s[0] == '#'); // TODO:
        pchtml_html_document_t *doc = co->stack->doc;
        pcdom_element_t *body = (pcdom_element_t*)doc->body;
        pcdom_document_t *document = (pcdom_document_t*)doc;
        pcdom_collection_t *collection;
        collection = pcdom_collection_create(document);
        PC_ASSERT(collection);
        unsigned int ui;
        ui = pcdom_collection_init(collection, 10);
        PC_ASSERT(ui == 0);
        ui = pcdom_elements_by_attr(body, collection,
                (const unsigned char*)"id", 2,
                (const unsigned char*)s+1, strlen(s+1),
                false);
        PC_ASSERT(ui == 0);
        int r = 0;
        for (unsigned int i=0; i<pcdom_collection_length(collection); ++i) {
            pcdom_node_t *node;
            node = pcdom_collection_node(collection, i);
            PC_ASSERT(node);
            pcdom_element_t *elem = (pcdom_element_t*)node;
            purc_variant_t o = pcdvobjs_make_elements(elem);
            PC_ASSERT(o != PURC_VARIANT_INVALID);
            if (purc_variant_is_string(src)) {
                const char *content = purc_variant_get_string_const(src);
                r = update_element(co->stack, o, at, to, "%s", content);
                if (r)
                    break;
            }
            else if (purc_variant_is_number(src)) {
                double d;
                bool ok;
                ok = purc_variant_cast_to_number(src, &d, false);
                PC_ASSERT(ok);
                r = update_element(co->stack, o, at, to, "%g", d);
                if (r)
                    break;
            }
            else if (purc_variant_is_undefined(src)) {
                PC_ASSERT(0);
                pcintr_printf_to_fragment(co->stack, o, to, at, "%s", "");
            }
            else {
                PRINT_VARIANT(on);
                PRINT_VARIANT(src);
                PC_ASSERT(0); // Not implemented yet
            }
            purc_variant_unref(o);
        }
        pcdom_collection_destroy(collection, true);
        return r ? -1 : 0;
    }
    PC_ASSERT(0); // Not implemented yet
    return -1;
}

static int
process_attr_on(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
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
process_attr_to(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    if (ctxt->to != PURC_VARIANT_INVALID) {
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
    ctxt->to = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_with(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
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

    ctxt->with_op = attr->op;

    ctxt->with = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_from(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    if (ctxt->from != PURC_VARIANT_INVALID) {
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

    ctxt->from = val;
    purc_variant_ref(val);

    return 0;
}

static int
process_attr_at(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val)
{
    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
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
attr_found(struct pcintr_stack_frame *frame,
        struct pcvdom_element *element,
        purc_atom_t name, purc_variant_t val,
        struct pcvdom_attr *attr,
        void *ud)
{
    UNUSED_PARAM(ud);

    PC_ASSERT(name);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, WITH)) == name) {
        return process_attr_with(frame, element, name, val, attr);
    }

    PC_ASSERT(attr->op == PCHVML_ATTRIBUTE_ASSIGNMENT);

    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, ON)) == name) {
        return process_attr_on(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, TO)) == name) {
        return process_attr_to(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, FROM)) == name) {
        return process_attr_from(frame, element, name, val);
    }
    if (pchvml_keyword(PCHVML_KEYWORD_ENUM(HVML, AT)) == name) {
        return process_attr_at(frame, element, name, val);
    }

    purc_set_error_with_info(PURC_ERROR_NOT_IMPLEMENTED,
            "vdom attribute '%s' for element <%s>",
            purc_atom_to_string(name), element->tag_name);

    PC_ASSERT(0); // Not implemented yet
    return -1;
}

static void*
after_pushed(pcintr_stack_t stack, pcvdom_element_t pos)
{
    PC_ASSERT(stack && pos);
    PC_ASSERT(stack == purc_get_stack());

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);

    frame->pos = pos; // ATTENTION!!

    if (pcintr_set_symbol_var_at_sign())
        return NULL;

    frame->attr_vars = purc_variant_make_object(0,
            PURC_VARIANT_INVALID, PURC_VARIANT_INVALID);
    if (frame->attr_vars == PURC_VARIANT_INVALID)
        return NULL;

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)calloc(1, sizeof(*ctxt));
    if (!ctxt) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        return NULL;
    }

    frame->ctxt = ctxt;
    frame->ctxt_destroy = ctxt_destroy;

    int r;
    r = pcintr_vdom_walk_attrs(frame, element, NULL, attr_found);
    if (r)
        return NULL;

    struct pcvcm_node *vcm_content = element->vcm_content;
    if (vcm_content) {
        purc_variant_t v = pcvcm_eval(vcm_content, stack);
        if (v == PURC_VARIANT_INVALID)
            return NULL;

        PURC_VARIANT_SAFE_CLEAR(frame->ctnt_var);
        frame->ctnt_var = v;
    }

    if (ctxt->on == PURC_VARIANT_INVALID) {
        purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                "lack of vdom attribute 'on' for element <%s>",
                element->tag_name);
        return NULL;
    }

    purc_variant_t src = PURC_VARIANT_INVALID;
    ctxt->src_op = PCHVML_ATTRIBUTE_ASSIGNMENT;
    if (ctxt->from != PURC_VARIANT_INVALID) {
        if (ctxt->with != PURC_VARIANT_INVALID) {
            PC_ASSERT(ctxt->with_op == PCHVML_ATTRIBUTE_ASSIGNMENT);
        }
        src = get_source_by_from(&stack->co, frame, ctxt->from, ctxt->with);
        PC_ASSERT(src != PURC_VARIANT_INVALID);
    }
    else if (ctxt->with != PURC_VARIANT_INVALID) {
        ctxt->src_op = ctxt->with_op;
        src = get_source_by_with(&stack->co, frame, ctxt->with);
        PC_ASSERT(src != PURC_VARIANT_INVALID);
    }
    else {
        if (frame->ctnt_var == PURC_VARIANT_INVALID) {
            PC_ASSERT(frame->ctnt_var != PURC_VARIANT_INVALID);
            purc_set_error_with_info(PURC_ERROR_ARGUMENT_MISSED,
                    "lack of vdom attribute 'with'/'from' for element <%s>",
                    element->tag_name);
            return NULL;
        }
        src = frame->ctnt_var;
        purc_variant_ref(src);
        PC_ASSERT(src != PURC_VARIANT_INVALID);
    }

    PURC_VARIANT_SAFE_CLEAR(ctxt->src);
    ctxt->src = src;

    r = process(&stack->co, frame);
    PC_ASSERT(r == 0);
    if (r)
        return NULL;

    return ctxt;
}

static bool
on_popping(pcintr_stack_t stack, void* ud)
{
    PC_ASSERT(stack);
    PC_ASSERT(stack == purc_get_stack());

    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(frame);
    PC_ASSERT(ud == frame->ctxt);

    struct pcvdom_element *element = frame->pos;
    PC_ASSERT(element);

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;
    if (ctxt) {
        ctxt_for_update_destroy(ctxt);
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
    PC_ASSERT(stack == purc_get_stack());

    pcintr_coroutine_t co = &stack->co;
    struct pcintr_stack_frame *frame;
    frame = pcintr_stack_get_bottom_frame(stack);
    PC_ASSERT(ud == frame->ctxt);

    struct ctxt_for_update *ctxt;
    ctxt = (struct ctxt_for_update*)frame->ctxt;

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
                PC_ASSERT(stack->except == 0);
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

struct pcintr_element_ops* pcintr_get_update_ops(void)
{
    return &ops;
}

