/**
 * @file base.h
 * @author 
 * @date 2021/07/02
 * @brief The hearder file for html parser.
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


#ifndef PCHTML_PARSER_BASE_H
#define PCHTML_PARSER_BASE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "html/core/base.h"


#define PCHTML_PARSER_VERSION_MAJOR 2
#define PCHTML_PARSER_VERSION_MINOR 0
#define PCHTML_PARSER_VERSION_PATCH 0

#define PCHTML_PARSER_VERSION_STRING PCHTML_STRINGIZE(PCHTML_PARSER_VERSION_MAJOR) "."   \
                                PCHTML_STRINGIZE(PCHTML_PARSER_VERSION_MINOR) "."   \
                                PCHTML_STRINGIZE(PCHTML_PARSER_VERSION_PATCH)


typedef struct pchtml_html_tokenizer pchtml_html_tokenizer_t;
typedef unsigned int pchtml_html_tokenizer_opt_t;
typedef struct pchtml_html_tree pchtml_html_tree_t;

/*
 * Please, see html/base.h pchtml_status_t
 */
typedef enum {
    PCHTML_PARSER_STATUS_OK = 0x0000,
}
pchtml_html_status_t;



#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_PARSER_BASE_H */
