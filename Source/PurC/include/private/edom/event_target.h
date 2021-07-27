/**
 * @file event_target.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for event target.
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


#ifndef PCEDOM_EVENT_TARGET_H
#define PCEDOM_EVENT_TARGET_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "private/edom/interface.h"


struct pcedom_event_target {
    void *events;
};


pcedom_event_target_t *
pcedom_event_target_create(pcedom_document_t *document) WTF_INTERNAL;

pcedom_event_target_t *
pcedom_event_target_destroy(pcedom_event_target_t *event_target,
                pcedom_document_t *document) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCEDOM_EVENT_TARGET_H */
