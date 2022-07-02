/*
 * @file exe_class_tab.c
 * @author Xu Xiaohong
 * @date 2022/06/16
 * @brief The implementation of public part for CLASS parser.
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

#define _GNU_SOURCE
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "purc-errors.h"

#include "../pcexe-helper.h"
#include "../exe_class.h"
#include "../tab.h"

#include "exe_class.tab.h"
#include "exe_class.lex.c"

#include "exe_class.lex.h"
#undef yylloc
#undef yylval
#include "exe_class.tab.c"

