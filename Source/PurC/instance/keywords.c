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
    enum pcatom_bucket          bucket;
    const char                 *keyword;
    purc_atom_t                 atom;
};

#include "keywords.inc"

void pchvml_keywords_init(void)
{
    size_t nr = PCA_TABLESIZE(keywords);
    for (size_t i=0; i<nr; ++i) {
        struct pchvml_keyword_cfg *cfg = keywords + i;
        cfg->atom = purc_atom_from_string_ex(cfg->bucket, cfg->keyword);
        PC_ASSERT(cfg->atom);
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

purc_atom_t pchvml_keyword_try_string(enum pcatom_bucket bucket,
        const char *keyword)
{
    return purc_atom_try_string_ex(bucket, keyword);
}

