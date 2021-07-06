/*
** Copyright (C) 2015-2017 Alexander Borisov
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
** Author: lex.borisov@gmail.com (Alexander Borisov)
*/

#include "mycore/myosi.h"

void * mycore_malloc(size_t size)
{
    return malloc(size);
}

void * mycore_realloc(void* dst, size_t size)
{
    return realloc(dst, size);
}

void * mycore_calloc(size_t num, size_t size)
{
    return calloc(num, size);
}

void * mycore_free(void* dst)
{
    free(dst);
    return NULL;
}
