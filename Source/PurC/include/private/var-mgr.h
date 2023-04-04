/*
 * @file var-mgr.h
 * @author XueShuming
 * @date 2021/12/06
 * @brief The api for PurC variable manager.
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

#ifndef PURC_PRIVATE_VAR_MGR_H
#define PURC_PRIVATE_VAR_MGR_H

#include "purc.h"

#include "config.h"

#include "private/variant.h"
#include "private/map.h"
#include "private/rbtree.h"

struct pcvarmgr;
typedef struct pcvarmgr*  pcvarmgr_t;

struct pcvarmgr {
    purc_variant_t object;
    struct pcvar_listener *listener;

    struct rb_node            node;
    struct pcvdom_node       *vdom_node;
};

struct pcns_varmgr {
    pcutils_map           *map;
};


struct pcintr_stack;

PCA_EXTERN_C_BEGIN

pcvarmgr_t pcvarmgr_create(void);

int pcvarmgr_destroy(pcvarmgr_t mgr);

bool pcvarmgr_add(pcvarmgr_t mgr, const char* name,
        purc_variant_t variant);

purc_variant_t pcvarmgr_get(pcvarmgr_t mgr, const char* name);

bool pcvarmgr_remove_ex(pcvarmgr_t mgr, const char* name, bool silently);

static inline bool pcvarmgr_remove(pcvarmgr_t mgr, const char* name)
{
    return pcvarmgr_remove_ex(mgr, name, false);
}

bool pcvarmgr_dispatch_except(pcvarmgr_t mgr, const char* name,
        const char* except);

struct pcns_varmgr *pcns_varmgr_create(void);

int pcns_varmgr_destroy(struct pcns_varmgr *ns_mgr);

bool pcns_varmgr_add(struct pcns_varmgr *ns_mgr, const char *name,
        purc_variant_t value, purc_variant_t ns);

purc_variant_t pcns_varmgr_get(struct pcns_varmgr *ns_mgr, const char *name,
        purc_variant_t ns);

PCA_EXTERN_C_END

#endif /* not defined PURC_PRIVATE_VAR_MGR_H */

