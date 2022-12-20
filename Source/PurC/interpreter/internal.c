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

#include "hvml-attr.h"

#include <stdlib.h>
#include <string.h>

#define ATTR_NAME_AS       "as"
#define MIN_BUFFER         512

#define REQUEST_ID_KEY_HANDLE   "__pcintr_request_id_handle"
#define REQUEST_ID_KEY_TYPE     "type"
#define REQUEST_ID_KEY_RID      "rid"
#define REQUEST_ID_KEY_CID      "cid"
#define REQUEST_ID_KEY_RES      "res"


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
pcintr_match_exception(purc_atom_t except, purc_variant_t constant)
{
    bool ret = false;
    if (except == 0 || !constant || !pcvariant_is_sorted_array(constant)) {
        goto out;
    }

    purc_atom_t any = purc_get_except_atom_by_id(PURC_EXCEPT_ANY);
    purc_variant_t v = purc_variant_make_ulongint((uint64_t)any);
    if (!v) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    ssize_t r = purc_variant_sorted_array_find(constant, v);
    purc_variant_unref(v);
    if (r >= 0) {
        ret = true;
        goto out;
    }

    v = purc_variant_make_ulongint((uint64_t)except);
    if (!v) {
        purc_set_error(PURC_ERROR_OUT_OF_MEMORY);
        goto out;
    }

    r = purc_variant_sorted_array_find(constant, v);
    if (r >= 0) {
        ret = true;
    }
    purc_variant_unref(v);
out:
    return ret;
}

bool
pcintr_is_hvml_attr(const char *name)
{
    bool ret = false;
    const struct pchvml_attr_entry *entry;
    if (!name) {
        goto out;
    }

    entry = pchvml_attr_static_search(name, strlen(name));
    if (entry && ((entry->type == PCHVML_ATTR_TYPE_ADVERB) ||
                (entry->type == PCHVML_ATTR_TYPE_PREP))) {
        ret = true;
    }

out:
    return ret;
}

static bool
check_hvml_run_resource(const char *uri)
{
    if (strstr(uri, PCINTR_HVML_RUN_RES_CRTN) == 0
            || strstr(uri, PCINTR_HVML_RUN_RES_CHAN) == 0) {
        return true;
    }
    return false;
}

/*
 * hvml+run://<host_name>/<app_name>/<runner_name>
 * //<host_name>/<app_name>/<runner_name>
 * /<app_name>/<runner_name>
 */
static inline const char *
check_hvml_run_schema(const char *uri, enum HVML_RUN_URI_TYPE *type)
{
    const char *ret = NULL;
    if (strncasecmp(uri, PCINTR_HVML_RUN_SCHEMA,
                PCINTR_LEN_HVML_RUN_SCHEMA) == 0) {
        ret = uri + PCINTR_LEN_HVML_RUN_SCHEMA;
        if (!check_hvml_run_resource(ret)) {
            ret = NULL;
            goto out;
        }

        if (type) {
            *type = HVML_RUN_URI_FULL;
        }
    }
    else if (strncmp(uri, "//", 2) == 0) {
        ret = uri + 2;
        if (!check_hvml_run_resource(ret)) {
            ret = NULL;
            goto out;
        }

        if (type) {
            *type = HVML_RUN_URI_OMIT_SCHEMA;
        }
    }
    else if (strncmp(uri, "/", 1) == 0) {
        ret = uri + 1;
        if (!check_hvml_run_resource(ret)) {
            ret = NULL;
            goto out;
        }

        if (type) {
            *type = HVML_RUN_URI_OMIT_SCHEMA_AND_HOST;
        }
    }

out:
    return ret;
}

/*
 * hvml+run://<host_name>/<app_name>/<runner_name>
 * //<host_name>/<app_name>/<runner_name>
 * /<app_name>/<runner_name>
 */
int
pcintr_hvml_run_extract_host_name(const char *uri, char *host_name)
{
    int len = 0;
    char *slash;
    enum HVML_RUN_URI_TYPE type = HVML_RUN_URI_INVALID;

    if ((uri = check_hvml_run_schema(uri, &type)) == NULL) {
        goto out;
    }

    switch (type) {
    case HVML_RUN_URI_FULL:
    case HVML_RUN_URI_OMIT_SCHEMA:
    {
        if ((slash = strchr(uri, '/')) == NULL) {
            goto out;
        }

        len = (uintptr_t)slash - (uintptr_t)uri;
        if (len <= 0 || len > PURC_LEN_HOST_NAME) {
            goto out;
        }

        strncpy(host_name, uri, len);
        host_name[len] = '\0';
        break;
    }

    case HVML_RUN_URI_OMIT_SCHEMA_AND_HOST:
    {
        host_name[0] = '-';
        host_name[1] = '\0';
        len = 1;
        break;
    }

    default:
        break;
    }


out:
    return len;
}

/*
 * hvml+run://<host_name>/<app_name>/<runner_name>
 * //<host_name>/<app_name>/<runner_name>
 * /<app_name>/<runner_name>
 */
int
pcintr_hvml_run_extract_app_name(const char *uri, char *app_name)
{
    int len = 0;
    enum HVML_RUN_URI_TYPE type = HVML_RUN_URI_INVALID;
    char *first_slash, *second_slash;

    if ((uri = check_hvml_run_schema(uri, &type)) == NULL) {
        goto out;
    }

    switch (type) {
    case HVML_RUN_URI_FULL:
    case HVML_RUN_URI_OMIT_SCHEMA:
    {
        if ((first_slash = strchr(uri, '/')) == 0 ||
                (second_slash = strchr(first_slash + 1, '/')) == 0) {
            goto out;
        }

        first_slash++;
        len = (uintptr_t)second_slash - (uintptr_t)first_slash;
        if (len <= 0 || len > PURC_LEN_APP_NAME) {
            goto out;
        }

        strncpy(app_name, first_slash, len);
        app_name[len] = '\0';
        break;
    }

    case HVML_RUN_URI_OMIT_SCHEMA_AND_HOST:
    {
        if ((first_slash = strchr(uri, '/')) == NULL) {
            goto out;
        }

        len = (uintptr_t)first_slash - (uintptr_t)uri;
        if (len <= 0 || len > PURC_LEN_APP_NAME) {
            goto out;
        }

        strncpy(app_name, uri, len);
        app_name[len] = '\0';
        break;
    }

    default:
        break;
    }


out:
    return len;
}

/*
 * hvml+run://<host_name>/<app_name>/<runner_name>/
 * //<host_name>/<app_name>/<runner_name>/
 * /<app_name>/<runner_name>/
 */
int
pcintr_hvml_run_extract_runner_name(const char *uri, char *runner_name)
{
    int len = 0;
    enum HVML_RUN_URI_TYPE type = HVML_RUN_URI_INVALID;
    char *first_slash, *second_slash, *third_slash;

    if ((uri = check_hvml_run_schema(uri, &type)) == NULL) {
        goto out;
    }

    switch (type) {
    case HVML_RUN_URI_FULL:
    case HVML_RUN_URI_OMIT_SCHEMA:
    {
        if ((first_slash = strchr(uri, '/')) == 0 ||
                (second_slash = strchr(first_slash + 1, '/')) == 0 ||
                (third_slash = strchr(second_slash + 1, '/')) == 0
                ) {
            goto out;
        }

        second_slash++;
        len = (uintptr_t)third_slash - (uintptr_t)second_slash;
        if (len <= 0 || len > PURC_LEN_RUNNER_NAME) {
            goto out;
        }

        strncpy(runner_name, second_slash, len);
        runner_name[len] = '\0';
        break;
    }

    case HVML_RUN_URI_OMIT_SCHEMA_AND_HOST:
    {
        if ((first_slash = strchr(uri, '/')) == 0 ||
                (second_slash = strchr(first_slash + 1, '/')) == 0) {
            goto out;
        }

        first_slash++;
        len = (uintptr_t)second_slash - (uintptr_t)first_slash;
        if (len <= 0 || len > PURC_LEN_APP_NAME) {
            goto out;
        }

        strncpy(runner_name, first_slash, len);
        runner_name[len] = '\0';
        break;
    }

    default:
        break;
    }

out:
    return len;
}

/*
 * hvml+run://<host_name>/<app_name>/<runner_name>/CRTN/1
 * //<host_name>/<app_name>/<runner_name>/CRTN/1
 * /<app_name>/<runner_name>/CRTN/1
 */
int
pcintr_hvml_run_extract_res_name(const char *uri,
        enum HVML_RUN_RES_TYPE *res_type, char *res_name)
{
    int len = 0;
    enum HVML_RUN_URI_TYPE type = HVML_RUN_URI_INVALID;
    char *first_slash, *second_slash, *third_slash, *fourth_slash;

    if ((uri = check_hvml_run_schema(uri, &type)) == NULL) {
        goto out;
    }

    switch (type) {
    case HVML_RUN_URI_FULL:
    case HVML_RUN_URI_OMIT_SCHEMA:
    {
        if ((first_slash = strchr(uri, '/')) == 0 ||
                (second_slash = strchr(first_slash + 1, '/')) == 0 ||
                (third_slash = strchr(second_slash + 1, '/')) == 0 ||
                (fourth_slash = strchr(third_slash + 1, '/')) == 0
                ) {
            goto out;
        }

        third_slash++;
        len = (uintptr_t)fourth_slash - (uintptr_t)third_slash;
        if (len <= 0 || len > PCINTR_LEN_HVML_RUN_RES) {
            goto out;
        }

        if (strncmp(third_slash, HVML_RUN_RES_TYPE_NAME_CRTN, len) == 0) {
            if (res_type) {
                *res_type = HVML_RUN_RES_TYPE_CRTN;
            }
        }
        else if (strncmp(third_slash, HVML_RUN_RES_TYPE_NAME_CHAN, len) == 0) {
            if (res_type) {
                *res_type = HVML_RUN_RES_TYPE_CHAN;
            }
        }
        else {
            len = 0;
            goto out;
        }

        strcpy(res_name, fourth_slash + 1);
        len = strlen(res_name);
        break;
    }

    case HVML_RUN_URI_OMIT_SCHEMA_AND_HOST:
    {
        if ((first_slash = strchr(uri, '/')) == 0 ||
                (second_slash = strchr(first_slash + 1, '/')) == 0 ||
                (third_slash = strchr(second_slash + 1, '/')) == 0
                ) {
            goto out;
        }

        second_slash++;
        len = (uintptr_t)third_slash - (uintptr_t)second_slash;
        if (len <= 0 || len > PCINTR_LEN_HVML_RUN_RES) {
            goto out;
        }

        if (strncmp(second_slash, HVML_RUN_RES_TYPE_NAME_CRTN, len) == 0) {
            if (res_type) {
                *res_type = HVML_RUN_RES_TYPE_CRTN;
            }
        }
        else if (strncmp(second_slash, HVML_RUN_RES_TYPE_NAME_CHAN, len) == 0) {
            if (res_type) {
                *res_type = HVML_RUN_RES_TYPE_CHAN;
            }
        }
        else {
            len = 0;
            goto out;
        }

        strcpy(res_name, third_slash + 1);
        len = strlen(res_name);
        break;
    }

    default:
        break;
    }

out:
    return len;
}

static inline const char *
get_hvml_res_type_name(enum HVML_RUN_RES_TYPE res_type)
{
    switch (res_type) {
    case HVML_RUN_RES_TYPE_CRTN:
        return HVML_RUN_RES_TYPE_NAME_CRTN;

    case HVML_RUN_RES_TYPE_CHAN:
        return HVML_RUN_RES_TYPE_NAME_CHAN;

    case HVML_RUN_RES_TYPE_INVALID:
    default:
        return HVML_RUN_RES_TYPE_NAME_INVALID;
    }
}


bool
pcintr_parse_hvml_run_uri(const char *uri, char *host_name, char *app_name,
        char *runner_name, enum HVML_RUN_RES_TYPE *res_type, char *res_name)
{
    bool ret = false;
    if (pcintr_hvml_run_extract_host_name(uri, host_name) <= 0) {
        goto out;
    }

    if (pcintr_hvml_run_extract_app_name(uri, app_name) <= 0) {
        goto out;
    }

    if (pcintr_hvml_run_extract_runner_name(uri, runner_name) <= 0) {
        goto out;
    }

    if (pcintr_hvml_run_extract_res_name(uri, res_type, res_name) <= 0) {
        goto out;
    }


    if(!((purc_is_valid_host_name(host_name) ||
                    strcmp(host_name, PCINTR_HVML_RUN_CURR_ID) == 0) &&
                (purc_is_valid_app_name(app_name) ||
                 strcmp(app_name, PCINTR_HVML_RUN_CURR_ID) == 0) &&
                (purc_is_valid_runner_name(runner_name) ||
                 strcmp(runner_name, PCINTR_HVML_RUN_CURR_ID) == 0))) {
        goto out;
    }

    if ((*res_type == HVML_RUN_RES_TYPE_CHAN &&
                pcintr_is_variable_token(res_name)) ||
            (*res_type == HVML_RUN_RES_TYPE_CRTN &&
             pcintr_is_valid_crtn_token(res_name))) {
        ret = true;
    }

out:
    PC_DEBUG("parse hvml+run %s !|uri=%s|host_name=%s|app_name=%s|runner_name=%s"
            "|res_type=%s|res_name=%s\n", ret ? "success" : "failed", uri,
            host_name, app_name, runner_name,
            get_hvml_res_type_name(*res_type), res_name);
    return ret;
}

bool
pcintr_is_valid_hvml_run_uri(const char *uri)
{
    char host_name[PURC_LEN_HOST_NAME + 1];
    char app_name[PURC_LEN_APP_NAME + 1];
    char runner_name[PURC_LEN_RUNNER_NAME + 1];
    char res_name[PURC_LEN_IDENTIFIER + 1];
    enum HVML_RUN_RES_TYPE res_type = HVML_RUN_RES_TYPE_INVALID;
    return pcintr_parse_hvml_run_uri(uri, host_name, app_name, runner_name,
            &res_type, res_name);
}

bool
pcintr_is_crtn_object(purc_variant_t v, purc_atom_t *cid)
{
    purc_variant_t r_cid = PURC_VARIANT_INVALID;
    bool ret = false;
    if (!purc_variant_is_object(v)) {
        goto out;
    }

    purc_variant_t v_cid = purc_variant_object_get_by_ckey(v, "cid");
    if (!v_cid || !purc_variant_is_dynamic(v_cid)) {
        goto out;
    }

    purc_dvariant_method getter = purc_variant_dynamic_get_getter(v_cid);
    if (!getter) {
        goto out;
    }

    r_cid = getter(v, 0, NULL, PCVRT_CALL_FLAG_SILENTLY);
    if (!r_cid || !purc_variant_is_ulongint(r_cid)) {
        goto out;
    }

    if (cid) {
        uint64_t u64;
        purc_variant_cast_to_ulongint(r_cid, &u64, true);
        *cid = (purc_atom_t) u64;
    }
    ret = true;

out:
    if (r_cid) {
        purc_variant_unref(r_cid);
    }
    return ret;
}

bool
pcintr_is_request_id(purc_variant_t v)
{
    if (purc_variant_is_object(v) &&
            purc_variant_object_get_by_ckey(v, REQUEST_ID_KEY_HANDLE)) {
        return true;
    }
    purc_clr_error();
    return false;
}

purc_variant_t
pcintr_request_id_create(enum pcintr_request_id_type type, purc_atom_t rid,
        purc_atom_t cid, const char *res)
{
    purc_variant_t v_type = PURC_VARIANT_INVALID;
    purc_variant_t v_rid = PURC_VARIANT_INVALID;
    purc_variant_t v_cid = PURC_VARIANT_INVALID;
    purc_variant_t v_res = PURC_VARIANT_INVALID;
    purc_variant_t handle = PURC_VARIANT_INVALID;
    bool success = false;

    purc_variant_t ret = purc_variant_make_object(0, PURC_VARIANT_INVALID,
            PURC_VARIANT_INVALID);
    if (!ret) {
        goto out;
    }

    v_type = purc_variant_make_ulongint((uint64_t)type);
    if (!v_type) {
        goto out_clear_object;
    }

    if (!purc_variant_object_set_by_static_ckey(ret,
                REQUEST_ID_KEY_TYPE, v_type)) {
        goto out_clear_type;
    }

    v_rid = purc_variant_make_ulongint((uint64_t)rid);
    if (!v_rid) {
        goto out_clear_type;
    }

    if (!purc_variant_object_set_by_static_ckey(ret,
                REQUEST_ID_KEY_RID, v_rid)) {
        goto out_clear_rid;
    }

    v_cid = purc_variant_make_ulongint((uint64_t)cid);
    if (!v_cid) {
        goto out_clear_rid;
    }

    if (!purc_variant_object_set_by_static_ckey(ret,
                REQUEST_ID_KEY_CID, v_cid)) {
        goto out_clear_cid;
    }

    if (res) {
        v_res = purc_variant_make_string(res, true);
        if (!v_res) {
            goto out_clear_cid;
        }

        if (!purc_variant_object_set_by_static_ckey(ret,
                    REQUEST_ID_KEY_RES, v_res)) {
            goto out_clear_res;
        }
    }

    handle = purc_variant_make_boolean(true);
    if (!handle) {
        goto out_clear_res;
    }

    if (!purc_variant_object_set_by_static_ckey(ret,
                REQUEST_ID_KEY_HANDLE, handle)) {
        goto out_clear_handle;
    }

    success = true;

out_clear_handle:
    purc_variant_unref(handle);

out_clear_res:
    if (v_res) {
        purc_variant_unref(v_res);
    }

out_clear_cid:
    purc_variant_unref(v_cid);

out_clear_rid:
    purc_variant_unref(v_rid);

out_clear_type:
    purc_variant_unref(v_type);

out_clear_object:
    if (!success) {
        purc_variant_unref(ret);
        ret = PURC_VARIANT_INVALID;
    }

out:
    return ret;
}

purc_atom_t
pcintr_request_id_get_rid(purc_variant_t v)
{
    if (!pcintr_is_request_id(v)) {
        return 0;
    }
    uint64_t u64;
    purc_variant_t val = purc_variant_object_get_by_ckey(v, REQUEST_ID_KEY_RID);
    purc_variant_cast_to_ulongint(val, &u64, true);
    return (purc_atom_t) u64;
}

purc_atom_t
pcintr_request_id_get_cid(purc_variant_t v)
{
    if (!pcintr_is_request_id(v)) {
        return 0;
    }
    uint64_t u64;
    purc_variant_t val = purc_variant_object_get_by_ckey(v, REQUEST_ID_KEY_CID);
    purc_variant_cast_to_ulongint(val, &u64, true);
    return (purc_atom_t) u64;
}

enum pcintr_request_id_type
pcintr_request_id_get_type(purc_variant_t v)
{
    if (!pcintr_is_request_id(v)) {
        return 0;
    }
    uint64_t u64;
    purc_variant_t val = purc_variant_object_get_by_ckey(v, REQUEST_ID_KEY_TYPE);
    purc_variant_cast_to_ulongint(val, &u64, true);
    return (enum pcintr_request_id_type) u64;
}

const char *
pcintr_request_id_get_res(purc_variant_t v)
{
    if (!pcintr_is_request_id(v)) {
        return 0;
    }
    purc_variant_t val = purc_variant_object_get_by_ckey(v, REQUEST_ID_KEY_RES);
    return purc_variant_get_string_const(val);
}

bool
pcintr_request_id_is_equal_to(purc_variant_t v1, purc_variant_t v2)
{
    bool ret = false;
    if (!pcintr_is_request_id(v1) || !pcintr_is_request_id(v2)) {
        goto out;
    }

    if ((pcintr_request_id_get_type(v1) != pcintr_request_id_get_type(v2))
            || (pcintr_request_id_get_rid(v1) != pcintr_request_id_get_rid(v2))
            ) {
        goto out;
    }

    purc_atom_t v1_cid = pcintr_request_id_get_cid(v1);
    purc_atom_t v2_cid = pcintr_request_id_get_cid(v2);
    if (v1_cid == v2_cid && v1_cid != 0) {
        ret = true;
        goto out;
    }

    if (strcmp(pcintr_request_id_get_res(v1),
                pcintr_request_id_get_res(v2)) == 0) {
        ret = true;
        goto out;
    }

out:
    return ret;
}

bool
pcintr_request_id_is_match(purc_variant_t v1, purc_variant_t v2)
{
    bool ret = false;
    if (pcintr_request_id_is_equal_to(v1, v2)) {
        ret = true;
        goto out;
    }

    if (!purc_variant_is_ulongint(v2)) {
        goto out;
    }

    uint64_t u64;
    purc_variant_cast_to_ulongint(v2, &u64, true);
    purc_atom_t rid = purc_get_rid_by_cid(u64);
    if (!rid) {
        goto out;
    }

    purc_variant_t v = pcintr_request_id_create(
                PCINTR_REQUEST_ID_TYPE_CRTN,
                rid, u64, NULL);
    ret = pcintr_request_id_is_equal_to(v1, v);
    purc_variant_unref(v);
out:
    return ret;
}

int
pcintr_chan_post(const char *chan_name, purc_variant_t data)
{
    int ret = -1;
    purc_variant_t  runner = PURC_VARIANT_INVALID;
    purc_variant_t  chan = PURC_VARIANT_INVALID;
    purc_variant_t  send_ret = PURC_VARIANT_INVALID;
    purc_variant_t  name = PURC_VARIANT_INVALID;

    void *entity = NULL;
    struct purc_native_ops *ops = NULL;

    if (!chan_name || !data) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    runner = purc_get_runner_variable(PURC_PREDEF_VARNAME_RUNNER);
    if (!runner) {
        purc_set_error(PURC_ERROR_NOT_SUPPORTED);
        goto out;
    }

    purc_variant_t v_chan = purc_variant_object_get_by_ckey(runner, "chan");
    if (!v_chan || !purc_variant_is_dynamic(v_chan)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    purc_dvariant_method chan_getter = purc_variant_dynamic_get_getter(v_chan);
    if (!chan_getter) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    name = purc_variant_make_string(chan_name, false);
    if (!name) {
        goto out;
    }

    chan = chan_getter(runner, 1, &name, PCVRT_CALL_FLAG_SILENTLY);
    if (!chan) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    entity = purc_variant_native_get_entity(chan);
    ops = purc_variant_native_get_ops(chan);
    purc_nvariant_method sender = ops->property_getter(entity, "send");
    if (!sender) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    send_ret = sender(entity, 1, &data, PCVRT_CALL_FLAG_SILENTLY);
    if (send_ret && purc_variant_booleanize(send_ret)) {
        ret = 0;
    }

out:
    if (send_ret) {
        purc_variant_unref(send_ret);
    }

    if (name) {
        purc_variant_unref(name);
    }
    return ret;
}

