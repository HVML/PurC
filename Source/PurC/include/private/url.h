/*
 * @file url.h
 * @author gengyue
 * @date 2021/12/26
 * @brief The header file of URL implementation.
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

#ifndef PURC_PRIVATE_URL_H
#define PURC_PRIVATE_URL_H

#include "purc-utils.h"
#include "purc-variant.h"

#define PCUTILS_URL_OPT_REAL_JSON               0x00000000
#define PCUTILS_URL_OPT_REAL_EJSON              0x00000001
#define PCUTILS_URL_OPT_RFC1738                 0x00000000
#define PCUTILS_URL_OPT_RFC3986                 0x00000002

#ifdef __cplusplus
extern "C" {
#endif

purc_variant_t
pcutils_url_build_query(purc_variant_t v, const char *numeric_prefix,
        char arg_separator, unsigned int flags);


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* not defined PURC_PRIVATE_URL_H*/

