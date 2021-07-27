/**
 * @file processing_instruction.h 
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for processsing instruction.
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


#ifndef PCHTML_DOM_PROCESSING_INSTRUCTION_H
#define PCHTML_DOM_PROCESSING_INSTRUCTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "private/edom/document.h"
#include "private/edom/text.h"


struct pchtml_dom_processing_instruction {
    pchtml_dom_character_data_t char_data;

    pchtml_str_t             target;
};


pchtml_dom_processing_instruction_t *
pchtml_dom_processing_instruction_interface_create(
        pchtml_dom_document_t *document) WTF_INTERNAL;

pchtml_dom_processing_instruction_t *
pchtml_dom_processing_instruction_interface_destroy(
        pchtml_dom_processing_instruction_t *processing_instruction) WTF_INTERNAL;


/*
 * Inline functions
 */
static inline const unsigned char *
pchtml_dom_processing_instruction_target(pchtml_dom_processing_instruction_t *pi,
                                      size_t *len)
{
    if (len != NULL) {
        *len = pi->target.length;
    }

    return pi->target.data;
}

/*
 * No inline functions for ABI.
 */
const unsigned char *
pchtml_dom_processing_instruction_target_noi(pchtml_dom_processing_instruction_t *pi,
                                          size_t *len);


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_DOM_PROCESSING_INSTRUCTION_H */
