/*
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
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
*/


#pragma once

#include "purc.h"
#include <iostream>
#include <map>

typedef purc_variant_t (*fn_expected)(void *ctxt, const char* name);
typedef bool (*fn_cmp)(purc_variant_t result, purc_variant_t expected);

struct dvobj_result {
    const char             *name;
    const char             *jsonee;

    fn_expected             expected;
    fn_cmp                  vrtcmp;
    int                     errcode;
};

using namespace std;

class TestExtDVObj {
public:
    TestExtDVObj();
    ~TestExtDVObj();

    purc_variant_t dvobj_new(const char *dvobj_name);
    purc_variant_t extdvobj_new(const char *dvobj_name);

    static purc_variant_t get_dvobj(void* ctxt, const char* name);

    void run_testcases(const struct dvobj_result *test_cases, size_t n);

    void run_testcases_in_file(const char *file_name);

private:
    typedef map <string, purc_variant_t> dvobj_map_t;
    dvobj_map_t m_dvobjs;
    dvobj_map_t m_extdvobjs;
    struct purc_variant_stat m_init_stat;
};

