/**
 * @file ops.c
 * @author Xu Xiaohong
 * @date 2021/12/17
 * @brief The internal interfaces for interpreter/ops
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

#include "config.h"

#include "internal.h"

#include "private/debug.h"
#include "private/executor.h"

#include "ops.h"

static struct pcintr_element_ops *all_ops[PCHVML_TAG_LAST_ENTRY];

struct tag_id_ops {
    enum pchvml_tag_id         tag_id;
    struct pcintr_element_ops* (*get)(void);
};

const struct tag_id_ops maps[] = {
    {PCHVML_TAG_HVML,              pcintr_get_hvml_ops},
    {PCHVML_TAG_HEAD,              pcintr_get_head_ops},
    {PCHVML_TAG_BODY,              pcintr_get_body_ops},
    {PCHVML_TAG_INIT,              pcintr_get_init_ops},
    {PCHVML_TAG_ARCHETYPE,         pcintr_get_archetype_ops},
    {PCHVML_TAG_ITERATE,           pcintr_get_iterate_ops},
    {PCHVML_TAG_UPDATE,            pcintr_get_update_ops},
    {PCHVML_TAG_EXCEPT,            pcintr_get_except_ops},
    {PCHVML_TAG_OBSERVE,           pcintr_get_observe_ops},
    {PCHVML_TAG_TEST,              pcintr_get_test_ops},
    {PCHVML_TAG_MATCH,             pcintr_get_match_ops},
    {PCHVML_TAG_CHOOSE,            pcintr_get_choose_ops},
    {PCHVML_TAG_CATCH,             pcintr_get_catch_ops},
   /* {PCHVML_TAG_FORGET,            pcintr_get_forget_ops}, */
};

void init_ops(void)
{
    static int inited = 0;
    if (inited)
        return;

    for (size_t i=0; i<PCA_TABLESIZE(all_ops); ++i) {
        all_ops[i] = pcintr_get_undefined_ops();
    }

    for (size_t i=0; i<PCA_TABLESIZE(maps); ++i) {
        const struct tag_id_ops *p = maps+i;
        if (p->tag_id < 0 || p->tag_id >= PCA_TABLESIZE(all_ops))
            continue;
        all_ops[p->tag_id] = p->get();
    }

    inited = 1;
}

struct pcintr_element_ops
pcintr_get_ops_by_tag_id(enum pchvml_tag_id tag_id)
{
    PC_ASSERT(tag_id >= 0);
    PC_ASSERT(tag_id < PCA_TABLESIZE(all_ops));
    struct pcintr_element_ops *ops = all_ops[tag_id];
    PC_ASSERT(ops);
    return *ops;
}

struct pcintr_element_ops pcintr_get_ops_by_element(pcvdom_element_t element)
{
    enum pchvml_tag_id tag_id = element->tag_id;
    return pcintr_get_ops_by_tag_id(tag_id);
}

