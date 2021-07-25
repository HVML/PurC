/**
 * @file window_element.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html window element.
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


#ifndef PCHTML_HTML_WINDOW_H
#define PCHTML_HTML_WINDOW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/html/interface.h"
#include "html/dom/interfaces/event_target.h"


struct pchtml_html_window {
    pchtml_dom_event_target_t event_target;
};


pchtml_html_window_t *
pchtml_html_window_create(pchtml_html_document_t *document) WTF_INTERNAL;

pchtml_html_window_t *
pchtml_html_window_destroy(pchtml_html_window_t *window) WTF_INTERNAL;


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_HTML_WINDOW_H */
