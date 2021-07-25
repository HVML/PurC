/*
 * Copyright (C) 2018 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#ifndef PCHTML_HTML_VIDEO_ELEMENT_H
#define PCHTML_HTML_VIDEO_ELEMENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "html/html/interface.h"
#include "html/html/interfaces/media_element.h"


struct pchtml_html_video_element {
    pchtml_html_media_element_t media_element;
};


GENGYUE_API pchtml_html_video_element_t *
pchtml_html_video_element_interface_create(pchtml_html_document_t *document);

GENGYUE_API pchtml_html_video_element_t *
pchtml_html_video_element_interface_destroy(pchtml_html_video_element_t *video_element);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PCHTML_HTML_VIDEO_ELEMENT_H */
