/*
 * @file purc.c
 * @author XueShuming
 * @date 2022/03/07
 * @brief A standalone HVML interpreter/debugger based-on PurC.
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


#include "purc.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    purc_instance_extra_info info = {};

    unsigned int modules = PURC_MODULE_HVML;
    purc_init_ex(modules, "cn.fmsoft.hybridos.purc", "purc", &info);


    purc_cleanup();

    return 0;
}
