/**
 * @file token_attr.c
 * @author 
 * @date 2021/07/02
 * @brief The complementation of html token attribution.
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


#include "html/token_attr.h"


pchtml_html_token_attr_t *
pchtml_html_token_attr_create(pchtml_dobject_t *dobj)
{
    return pchtml_dobject_calloc(dobj);
}

void
pchtml_html_token_attr_clean(pchtml_html_token_attr_t *attr)
{
    memset(attr, 0, sizeof(pchtml_html_token_attr_t));
}

pchtml_html_token_attr_t *
pchtml_html_token_attr_destroy(pchtml_html_token_attr_t *attr, pchtml_dobject_t *dobj)
{
    return pchtml_dobject_free(dobj, attr);
}

const unsigned char *
pchtml_html_token_attr_name(pchtml_html_token_attr_t *attr, size_t *length)
{
    if (attr->name == NULL) {
        if (length != NULL) {
            *length = 0;
        }

        return NULL;
    }

    if (length != NULL) {
        *length = attr->name->entry.length;
    }

    return pchtml_hash_entry_str(&attr->name->entry);
}
