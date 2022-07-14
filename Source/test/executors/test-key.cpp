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

#include "purc.h"

#include "private/executor.h"
#include "private/utils.h"

#include <gtest/gtest.h>
#include <glob.h>
#include <limits.h>

#include "../helpers.h"

extern "C" {
#include "pcexe-helper.h"
#include "exe_key.h"
#include "exe_key.tab.h"
}

#include "utils.cpp.in"

TEST(exe_key, basic)
{
    purc_instance_extra_info info = {};
    bool cleanup = false;

    // initial purc
    int ret = purc_init_ex(PURC_MODULE_HVML, "cn.fmsoft.hvml.test", "exe_key",
            &info);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    bool ok;

    struct purc_exec_ops ops;
    memset(&ops, 0, sizeof(ops));
    ok = purc_register_executor("KEY", &ops);
    EXPECT_FALSE(ok);
    EXPECT_EQ(purc_get_last_error(), PCEXECUTOR_ERROR_ALREAD_EXISTS);

    cleanup = purc_cleanup();
    ASSERT_EQ(cleanup, true);
}

static inline bool
parse_ex(const char *rule, purc_variant_t input,
    char *err_msg, size_t sz_err_msg)
{
    purc_exec_ops ops;
    bool ok = purc_get_executor("KEY", &ops);
    if (!ok) {
        snprintf(err_msg, sz_err_msg,
            "%s", "failed to get executor of [KEY]");
        return false;
    }

    pcexecutor_set_debug(debug_flex, debug_bison);

    purc_exec_inst_t inst = ops.create(PURC_EXEC_TYPE_CHOOSE,
            input, true);

    if (!inst) {
        snprintf(err_msg, sz_err_msg,
            "%s", "failed to create [KEY] instance");
        return false;
    }

    ok = true;

    purc_variant_t v = ops.choose(inst, rule);
    if (v == PURC_VARIANT_INVALID) {
        if (inst->err_msg) {
            snprintf(err_msg, sz_err_msg, "%s", inst->err_msg);
            ok = false;
        }
    } else {
        purc_variant_unref(v);
    }

    ops.destroy(inst);

    return ok;
}

TEST(exe_key, files)
{
    int r = 0;
    glob_t globbuf;
    memset(&globbuf, 0, sizeof(globbuf));

    purc_instance_extra_info info = {};
    r = purc_init_ex(PURC_MODULE_HVML, "cn.fmsoft.hvml.test", "exe_key", &info);
    EXPECT_EQ(r, PURC_ERROR_OK);
    if (r)
        return;

    const char *rel = "data/key.*.rule";
    get_option_from_env(rel, false);

    purc_variant_t key = purc_variant_make_string_static("hello", true);
    purc_variant_t val = purc_variant_make_string_static("world", true);
    purc_variant_t obj = purc_variant_make_object(1, key, val);
    purc_variant_unref(val);
    purc_variant_unref(key);

    process_sample_files(sample_files,
            [&](const char *rule, char *err_msg, size_t sz_err_msg) -> bool {
        return parse_ex(rule, obj, err_msg, sz_err_msg);
    });

    purc_variant_unref(obj);

    bool ok = purc_cleanup ();

    std::cerr << std::endl;
    get_option_from_env(rel, true); // print
    print_statics();
    std::cerr << std::endl;

    ASSERT_TRUE(ok);
}

