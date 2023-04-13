/*
 * @file child.c
 * @author XueShuming
 * @date 2022/07/25
 * @brief The operation for child coroutine
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

#include "private/var-mgr.h"
#include "private/errors.h"
#include "private/instance.h"
#include "private/utils.h"
#include "private/variant.h"

#include <stdlib.h>
#include <string.h>

#define DEFAULT_RUNNER_NAME     "_self"

static void
parse_info(const char *org, char **page_type,
        char **workspace, char **group, char **page_name)
{
    const char *end = org + strlen(org);
    const char *p = strchr(org, ':');
    const char *q = NULL;
    if (p) {
        *page_type = strndup(org, p - org);
        p = p + 1;
    }
    else {
        *page_type = NULL;
        p = org;
    }

    if (p == end) {
        goto out;
    }

    q = strchr(p, '@');
    if (q) {
        *page_name = strndup(p, q - p);
        q = q + 1;
    }
    else {
        *page_name = strdup(p);
        q = NULL;
    }

    if (!q || q == end) {
        goto out;
    }

    p = strchr(q, '/');
    if (p) {
        *workspace = strndup(q, p - q);
        *group = (p + 1 < end) ? strdup(p + 1) : NULL;
    }
    else {
        *group = strdup(q);
    }

out:
    return;
}

void
fill_vdom_rdr_param(const char *rdr_info, pcrdr_page_type_k *page_type,
        char **workspace, char **group, char **page_name)
{
    char *type = NULL;
    parse_info(rdr_info, &type, workspace, group, page_name);

    if (type != NULL) {
        if (strcmp(type, PCRDR_PAGE_TYPE_NAME_WIDGET) == 0) {
            *page_type = PCRDR_PAGE_TYPE_WIDGET;
        }
        else {
            *page_type = PCRDR_PAGE_TYPE_PLAINWIN;
        }
    }

    if (*page_name && *page_name[0] == '_') {
        if (strcmp(*page_name, PCRDR_PAGE_TYPE_NAME_NULL) == 0) {
            *page_type = PCRDR_PAGE_TYPE_NULL;
        }
        if (strcmp(*page_name, PCRDR_PAGE_TYPE_NAME_INHERIT) == 0) {
            *page_type = PCRDR_PAGE_TYPE_INHERIT;
        }
        if (strcmp(*page_name, PCRDR_PAGE_TYPE_NAME_SELF) == 0) {
            *page_type = PCRDR_PAGE_TYPE_SELF;
        }

#if 0   /* VW NOTE: deprecated duto PURCMC 120 */
        if (strcmp(*page_name, PCRDR_PAGE_TYPE_NAME_FIRST) == 0) {
            *page_type = PCRDR_PAGE_TYPE_FIRST;
        }
        if (strcmp(*page_name, PCRDR_PAGE_TYPE_NAME_LAST) == 0) {
            *page_type = PCRDR_PAGE_TYPE_LAST;
        }
        if (strcmp(*page_name, PCRDR_PAGE_TYPE_NAME_ACTIVE) == 0) {
            *page_type = PCRDR_PAGE_TYPE_ACTIVE;
        }
#endif
    }

    free(type);
}

void
fill_cor_rdr_info(purc_renderer_extra_info *rdr_info, purc_variant_t rdr)
{
    purc_variant_t tmp;

    tmp = purc_variant_object_get_by_ckey(rdr, "class");
    if (tmp)
        rdr_info->klass = purc_variant_get_string_const(tmp);

    tmp = purc_variant_object_get_by_ckey(rdr, "title");
    if (tmp)
        rdr_info->title = purc_variant_get_string_const(tmp);

    tmp = purc_variant_object_get_by_ckey(rdr, "layoutStyle");
    if (tmp)
        rdr_info->layout_style = purc_variant_get_string_const(tmp);

    rdr_info->toolkit_style = purc_variant_object_get_by_ckey(rdr,
            "toolkitStyle");

    tmp = purc_variant_object_get_by_ckey(rdr, "pageGroups");
    if (tmp) {
        rdr_info->page_groups = purc_variant_get_string_const(tmp);
    }
}

purc_atom_t
pcintr_schedule_child_co(purc_vdom_t vdom, purc_atom_t curator,
        const char *runner, const char *rdr_target, purc_variant_t request,
        const char *body_id, bool create_runner)
{

    purc_atom_t cid = 0;
    char endpoint_name[PURC_LEN_ENDPOINT_NAME + 1];

    pcrdr_page_type_k page_type= PCRDR_PAGE_TYPE_NULL;
    char *target_workspace = NULL;
    char *target_group = NULL;
    char *page_name = NULL;

    struct pcinst *inst = pcinst_current();
    const char *app_name = inst->app_name;
    const char *runner_name = runner;
    if (!runner || strcmp(runner, DEFAULT_RUNNER_NAME) == 0) {
        runner_name = inst->runner_name;
    }

    purc_assemble_endpoint_name_ex(PCRDR_LOCALHOST,
            app_name, runner_name,
            endpoint_name, sizeof(endpoint_name) - 1);
    purc_atom_t atom = purc_atom_try_string_ex(PURC_ATOM_BUCKET_DEF,
            endpoint_name);
    if (atom == 0 && !create_runner) {
        goto out;
    }

    if (rdr_target) {
        fill_vdom_rdr_param(rdr_target, &page_type, &target_group,
                &target_group, &page_name);
    }

    purc_atom_t dest_inst = purc_inst_create_or_get(app_name,
            runner_name, NULL, NULL);
    if (!dest_inst) {
        PC_WARN("create inst falied app_name=%s runner_name=%s\n", app_name,
                runner_name);
        goto out_free_names;
    }

    purc_renderer_extra_info rdr_info = {};
    if (request && purc_variant_is_object(request)) {
        purc_variant_t rdr =
            purc_variant_object_get_by_ckey(request, "_renderer");
        if (rdr && purc_variant_is_object(rdr)) {
            fill_cor_rdr_info(&rdr_info, rdr);
        }
    }

    if (inst->intr_heap->move_buff != dest_inst) {
        cid = purc_inst_schedule_vdom(dest_inst, vdom,
                curator, request, page_type,
                target_workspace, target_group, page_name,
                &rdr_info, body_id);
    }
    else {
        purc_coroutine_t cco = purc_schedule_vdom(vdom,
                curator, request, page_type,
                target_workspace, target_group, page_name,
                &rdr_info, body_id, NULL);
        if (cco) {
            cid = cco->cid;
        }
    }

out_free_names:
    free(page_name);
    free(target_group);
    free(target_workspace);

out:
    return cid;
}

purc_atom_t
pcintr_schedule_child_co_from_string(const char *hvml, purc_atom_t curator,
        const char *runner, const char *rdr_target, purc_variant_t request,
        const char *body_id, bool create_runner)
{
    purc_vdom_t vdom = purc_load_hvml_from_string(hvml);
    if (vdom == NULL) {
        return 0;
    }

    return pcintr_schedule_child_co(vdom, curator, runner, rdr_target,
            request, body_id, create_runner);
}

