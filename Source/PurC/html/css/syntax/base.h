/**
 * @file base.h
 * @author 
 * @date 2021/07/02
 * @brief The basic hearder file for css parser.
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


#ifndef PCHTML_CSS_SYNTAX_BASE_H
#define PCHTML_CSS_SYNTAX_BASE_H

#ifdef __cplusplus
extern "C" {
#endif


#include "config.h"
#include "html/core/base.h"


#define PCHTML_CSS_SYNTAX_VERSION_MAJOR 0
#define PCHTML_CSS_SYNTAX_VERSION_MINOR 1
#define PCHTML_CSS_SYNTAX_VERSION_PATCH 2

#define PCHTML_CSS_SYNTAX_VERSION_STRING                                          \
    PCHTML_STRINGIZE(PCHTML_CSS_SYNTAX_VERSION_MAJOR) "."                         \
    PCHTML_STRINGIZE(PCHTML_CSS_SYNTAX_VERSION_MINOR) "."                         \
    PCHTML_STRINGIZE(PCHTML_CSS_SYNTAX_VERSION_PATCH)


#ifdef __cplusplus
}       /* __cplusplus */
#endif

#endif  /* PCHTML_CSS_SYNTAX_BASE_H */
