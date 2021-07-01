/*
** Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML parser
** and interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
**
*/


#include "errno.h"
#include <stdio.h>

purc_error_t purc_err_code;

void purc_set_last_error (purc_error_t err_code)
{
    purc_err_code = err_code;
}

purc_error_t purc_get_last_error (void)
{
    return purc_err_code;
}

const char* purc_get_error_message (purc_error_t err_code)
{
    return NULL;
}
