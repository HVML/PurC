/**
 * @file event_target.c
 * @author
 * @date 2021/07/02
 * @brief The complementation of event target.
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


#include "private/edom/event_target.h"
#include "private/edom/document.h"


pcedom_event_target_t *
pcedom_event_target_create(pcedom_document_t *document)
{
    pcedom_event_target_t *element;

    element = pchtml_mraw_calloc(document->mraw,
                                 sizeof(pcedom_event_target_t));
    if (element == NULL) {
        return NULL;
    }

    pcedom_interface_node(element)->type = PCEDOM_NODE_TYPE_UNDEF;

    return element;
}

pcedom_event_target_t *
pcedom_event_target_destroy(pcedom_event_target_t *event_target,
                             pcedom_document_t *document)
{
    return pchtml_mraw_free(document->mraw, event_target);
}
