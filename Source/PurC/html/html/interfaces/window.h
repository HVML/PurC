/*
 * Copyright (C) 2018 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef PCHTML_HTML_WINDOW_H
#define PCHTML_HTML_WINDOW_H

#ifdef __cplusplus
extern "C" {
#endif

#include "html/html/interface.h"
#include "html/dom/interfaces/event_target.h"


struct pchtml_html_window {
    pchtml_dom_event_target_t event_target;
};


GENGYUE_API pchtml_html_window_t *
pchtml_html_window_create(pchtml_html_document_t *document);

GENGYUE_API pchtml_html_window_t *
pchtml_html_window_destroy(pchtml_html_window_t *window);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PCHTML_HTML_WINDOW_H */
