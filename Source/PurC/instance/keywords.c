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

struct pchvml_keyword_cfg {
    purc_atom_t                 atom;
    const char                 *keyword;
};

static void
keywords_bucket_init(struct pchvml_keyword_cfg *cfgs,
        size_t start, size_t end, enum pcatom_bucket bucket)
{
    struct pchvml_keyword_cfg *cfg = cfgs + start;
    for (size_t i=start; i<end; ++i) {
        cfg->atom = purc_atom_from_string_ex(bucket, cfg->keyword);
        PC_ASSERT(cfg->atom);
        ++cfg;
    }
}

#include "keywords.inc"

purc_atom_t pchvml_keyword(enum pchvml_keyword_enum keyword)
{
    size_t nr = PCA_TABLESIZE(keywords);
    if (keyword < 0 || keyword >= nr)
        return 0;

    struct pchvml_keyword_cfg *cfg = keywords + keyword;
    return cfg->atom;
}

purc_atom_t pchvml_keyword_try_string(enum pcatom_bucket bucket,
        const char *keyword)
{
    return purc_atom_try_string_ex(bucket, keyword);
}

