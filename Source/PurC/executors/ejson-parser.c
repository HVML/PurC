/*
 * @file ejson.c
 * @author Xu Xiaohong
 * @date 2021/11/05
 * @brief The impl for eJSON-parser parser
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

#include "config.h"

#include "private/ejson-parser.h"

#include "ejson.tab.h"

purc_variant_t
pcejson_parser_parse_string(const char *str)
{
    struct ejson_param param;
    param.var = PURC_VARIANT_INVALID;

    int r = ejson_parse(str, &param);

    if (r)
        return PURC_VARIANT_INVALID;

    return param.var;
}

