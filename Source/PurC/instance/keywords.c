/*
 * @file keywords.c
 * @author Xu Xiaohong
 * @date 2022/01/22
 * @brief The instance of PurC.
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

#include "keywords.h"
#include "private/debug.h"
#include "private/rbtree.h"

struct pchvml_keyword_cfg {
    struct rb_node              node;
    const char                 *keyword;
    purc_atom_t                 atom;
};

#include "keywords.inc"

static struct rb_root       root;

static int
cfg_cmp(struct rb_node *node, void *ud)
{
    struct pchvml_keyword_cfg *cfg = (struct pchvml_keyword_cfg*)ud;
    struct pchvml_keyword_cfg *curr;
    curr = container_of(node, struct pchvml_keyword_cfg, node);
    return strcmp(curr->keyword, cfg->keyword);
}

struct rb_node*
new_entry(void *ud)
{
    struct pchvml_keyword_cfg *cfg = (struct pchvml_keyword_cfg*)ud;
    cfg->atom = purc_atom_from_string(cfg->keyword);

    return &cfg->node;
}

void pchvml_keywords_init(void)
{
    root = RB_ROOT;
    size_t nr = PCA_TABLESIZE(keywords);
    for (size_t i=0; i<nr; ++i) {
        struct pchvml_keyword_cfg *cfg = keywords + i;
        int r;
        r = pcutils_rbtree_insert(&root, cfg, cfg_cmp, new_entry);
        PC_ASSERT(r == 0);
    }
}

purc_atom_t pchvml_keyword(enum pchvml_keyword_enum keyword)
{
    size_t nr = PCA_TABLESIZE(keywords);
    if (keyword < 0 || keyword >= nr)
        return 0;

    struct pchvml_keyword_cfg *cfg = keywords + keyword;
    return cfg->atom;
}

struct find_data {
    const char             *keyword;
    purc_atom_t             found;
};

static int
walk(struct rb_node *node, void *ud)
{
    struct find_data *data = (struct find_data*)ud;
    struct pchvml_keyword_cfg *curr;
    curr = container_of(node, struct pchvml_keyword_cfg, node);
    if (strcmp(curr->keyword, data->keyword))
        return 0;

    data->found = curr->atom;

    return -1;
}

purc_atom_t pchvml_keyword_try_string(const char *keyword)
{
    struct find_data data = {
        .keyword       = keyword,
        .found         = 0,
    };

    pcutils_rbtree_traverse(&root, &data, walk);

    return data.found;
}

