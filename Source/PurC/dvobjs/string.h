/*
 * @file string.h
 * @author Geng Yue
 * @date 2021/07/02
 * @brief The header file of string dynamic variant object.
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

#include "purc-variant.h"

#ifndef _DVOJBS_STRING_H_
#define _DVOJBS_SYSTEM_H_ 

purc_variant_t
string_contains (purc_variant_t root, int nr_args, purc_variant_t* argv);

purc_variant_t
string_ends_with (purc_variant_t root, int nr_args, purc_variant_t* argv);

purc_variant_t
string_explode (purc_variant_t root, int nr_args, purc_variant_t* argv);

purc_variant_t
string_shuffle (purc_variant_t root, int nr_args, purc_variant_t* argv);

purc_variant_t
string_replace (purc_variant_t root, int nr_args, purc_variant_t* argv);

purc_variant_t
string_format_c (purc_variant_t root, int nr_args, purc_variant_t* argv);

purc_variant_t
string_format_p (purc_variant_t root, int nr_args, purc_variant_t* argv);

#endif  // _DVOJBS_STRING_H_ 

