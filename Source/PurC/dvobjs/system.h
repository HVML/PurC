/*
 * @file dvobjs.c
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The implementation of public part for html parser.
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
 *
 * This implementation of HTML parser is derived from Lexbor
 * <https://github.com/lexbor/lexbor>, which is licensed under the Apache
 * License, Version 2.0:
 *
 * Copyright (C) 2018-2020 Alexander Borisov
 *
 * Author: Alexander Borisov <borisov@lexbor.com>
 */

#include "purc-variant.h"

#ifndef _DVOJBS_SYSTEM_H_
#define _DVOJBS_SYSTEM_H_ 

purc_variant_t
get_uname_all (purc_variant_t root, int nr_args, purc_variant_t* argv);

purc_variant_t
get_uname (purc_variant_t root, int nr_args, purc_variant_t* argv);

purc_variant_t
get_locale (purc_variant_t root, int nr_args, purc_variant_t* argv);

purc_variant_t
set_locale (purc_variant_t root, int nr_args, purc_variant_t* argv);

purc_variant_t
get_random (purc_variant_t root, int nr_args, purc_variant_t* argv);

purc_variant_t
get_time (purc_variant_t root, int nr_args, purc_variant_t* argv);

purc_variant_t
set_time (purc_variant_t root, int nr_args, purc_variant_t* argv);

#endif  // _DVOJBS_SYSTEM_H_ 

