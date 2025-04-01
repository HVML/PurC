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

#include "purc/purc.h"

#include "private/hvml.h"
#include "private/utils.h"
#include "purc/purc-rwstream.h"
#include "hvml/hvml-token.h"
#include "private/ejson-parser.h"
#include "private/debug.h"

#include "../helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <gtest/gtest.h>

#include <dirent.h>
#include <glob.h>

using namespace std;

#define MIN_BUFFER     512
#define MAX_BUFFER     1024 * 1024 * 1024

char* variant_to_string(purc_variant_t v)
{
    purc_rwstream_t my_rws = purc_rwstream_new_buffer(MIN_BUFFER, MAX_BUFFER);
    size_t len_expected = 0;
    purc_variant_serialize(v, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    char* buf = (char*)purc_rwstream_get_mem_buffer_ex(my_rws, NULL, NULL, true);
    purc_rwstream_destroy(my_rws);
    return buf;
}


enum container_ops_type {
    CONTAINER_OPS_TYPE_DISPLACE,
    CONTAINER_OPS_TYPE_APPEND,
    CONTAINER_OPS_TYPE_PREPEND,
    CONTAINER_OPS_TYPE_MERGE,
    CONTAINER_OPS_TYPE_REMOVE,
    CONTAINER_OPS_TYPE_INSERT_BEFORE,
    CONTAINER_OPS_TYPE_INSERT_AFTER,
    CONTAINER_OPS_TYPE_UNITE,
    CONTAINER_OPS_TYPE_INTERSECT,
    CONTAINER_OPS_TYPE_SUBTRACT,
    CONTAINER_OPS_TYPE_XOR,
    CONTAINER_OPS_TYPE_OVERWRITE
};

struct test_case {
    char* filename;
    char* data;
};

#define TO_TYPE(type_name, type_enum)                       \
    if (strcmp (type, type_name) == 0) {                    \
        return type_enum;                                   \
    }

enum container_ops_type to_ops_type(const char* type)
{
    TO_TYPE("displace", CONTAINER_OPS_TYPE_DISPLACE);
    TO_TYPE("append", CONTAINER_OPS_TYPE_APPEND);
    TO_TYPE("prepend", CONTAINER_OPS_TYPE_PREPEND);
    TO_TYPE("merge", CONTAINER_OPS_TYPE_MERGE);
    TO_TYPE("remove", CONTAINER_OPS_TYPE_REMOVE);
    TO_TYPE("insertBefore", CONTAINER_OPS_TYPE_INSERT_BEFORE);
    TO_TYPE("insertAfter", CONTAINER_OPS_TYPE_INSERT_AFTER);
    TO_TYPE("unite", CONTAINER_OPS_TYPE_UNITE);
    TO_TYPE("intersect", CONTAINER_OPS_TYPE_INTERSECT);
    TO_TYPE("subtract", CONTAINER_OPS_TYPE_SUBTRACT);
    TO_TYPE("xor", CONTAINER_OPS_TYPE_XOR);
    TO_TYPE("overwrite", CONTAINER_OPS_TYPE_OVERWRITE);

    return CONTAINER_OPS_TYPE_DISPLACE;
}

purc_variant_type to_variant_type(const char* type)
{
    TO_TYPE("object", PURC_VARIANT_TYPE_OBJECT);
    TO_TYPE("array", PURC_VARIANT_TYPE_ARRAY);
    TO_TYPE("set", PURC_VARIANT_TYPE_SET);
    return PURC_VARIANT_TYPE_OBJECT;
}

static inline void
add_test_case(std::vector<test_case> &test_cases,
        const char* filename, const char* data)
{
    struct test_case test;
    test.filename = MemCollector::strdup(filename);
    test.data = MemCollector::strdup(data);
    test_cases.push_back(test);
}

char* trim(char *str)
{
    if (!str)
    {
        return NULL;
    }
    char *end;

    while (isspace((unsigned char)*str)) {
        str++;
    }

    if(*str == 0) {
        return str;
    }

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    end[1] = '\0';
    return str;
}

class TestCaseData : public testing::TestWithParam<test_case>
{
protected:
    void SetUp() {
        purc_init_ex (PURC_MODULE_VARIANT, "cn.fmsoft.hybridos.test",
                "purc_variant", NULL);
    }
    void TearDown() {
        purc_cleanup ();
    }
    const test_case get_data() {
        return GetParam();
    }
};

purc_variant_t to_variant_set(const char* unique_key, purc_variant_t var)
{
    purc_variant_t set = PURC_VARIANT_INVALID;

    if (unique_key && strlen(unique_key)) {
        set = purc_variant_make_set_by_ckey(0, unique_key,
                PURC_VARIANT_INVALID);
    }
    else {
        set = purc_variant_make_set(0, PURC_VARIANT_INVALID,
                PURC_VARIANT_INVALID);
    }
    if (var == PURC_VARIANT_INVALID) {
        goto end;
    }

    if (purc_variant_is_object(var)) {
        purc_variant_set_add(set, var, PCVRNT_CR_METHOD_COMPLAIN);
    }
    else if (purc_variant_is_array(var)) {
        size_t sz = purc_variant_array_get_size(var);
        for (size_t i = 0; i < sz; i++) {
            purc_variant_t v = purc_variant_array_get(var, i);
            purc_variant_set_add(set, v, PCVRNT_CR_METHOD_COMPLAIN);
        }
    }

end:
    return set;
}

purc_variant_t build_test_dst(purc_variant_t test_case_variant)
{
    const char* dst_unique_key = NULL;
    purc_variant_t dst_unique_key_var = purc_variant_object_get_by_ckey_ex(
            test_case_variant, "dst_unique_key", true);
    if (dst_unique_key_var != PURC_VARIANT_INVALID) {
        dst_unique_key = purc_variant_get_string_const(dst_unique_key_var);
    }

    const char* dst_type = NULL;
    purc_variant_t dst_type_var = purc_variant_object_get_by_ckey_ex(
            test_case_variant, "dst_type", true);
    if (dst_type_var != PURC_VARIANT_INVALID) {
        dst_type = purc_variant_get_string_const(dst_type_var);
    }

    purc_variant_t dst = purc_variant_object_get_by_ckey_ex(test_case_variant,
                "dst", true);
    if (dst == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    enum purc_variant_type type = to_variant_type(dst_type);
    if (type == PURC_VARIANT_TYPE_SET) {
        return to_variant_set(dst_unique_key, dst);
    }
    purc_variant_ref(dst);
    return dst;
}

purc_variant_t build_test_src(purc_variant_t test_case_variant)
{
    const char* src_unique_key = NULL;
    purc_variant_t src_unique_key_var = purc_variant_object_get_by_ckey_ex(
            test_case_variant, "src_unique_key", true);
    if (src_unique_key_var != PURC_VARIANT_INVALID) {
        src_unique_key = purc_variant_get_string_const(src_unique_key_var);
    }

    const char* src_type = NULL;
    purc_variant_t src_type_var = purc_variant_object_get_by_ckey_ex(
            test_case_variant, "src_type", true);
    if (src_type_var != PURC_VARIANT_INVALID) {
        src_type = purc_variant_get_string_const(src_type_var);
    }

    purc_variant_t src = purc_variant_object_get_by_ckey_ex(test_case_variant,
                "src", true);
    if (src == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    enum purc_variant_type type = to_variant_type(src_type);
    if (type == PURC_VARIANT_TYPE_SET) {
        return to_variant_set(src_unique_key, src);
    }
    purc_variant_ref(src);
    return src;
}

purc_variant_t build_test_cmp(purc_variant_t test_case_variant)
{
    const char* cmp_unique_key = NULL;
    purc_variant_t cmp_unique_key_var = purc_variant_object_get_by_ckey_ex(
            test_case_variant, "cmp_unique_key", true);
    if (cmp_unique_key_var != PURC_VARIANT_INVALID) {
        cmp_unique_key = purc_variant_get_string_const(cmp_unique_key_var);
    }

    const char* cmp_type = "array";
    purc_variant_t cmp_type_var = purc_variant_object_get_by_ckey_ex(
            test_case_variant, "cmp_type", true);
    if (cmp_type_var != PURC_VARIANT_INVALID) {
        cmp_type = purc_variant_get_string_const(cmp_type_var);
    }

    purc_variant_t cmp = purc_variant_object_get_by_ckey_ex(test_case_variant,
                "cmp", true);
    if (cmp == PURC_VARIANT_INVALID) {
        return PURC_VARIANT_INVALID;
    }

    enum purc_variant_type type = to_variant_type(cmp_type);
    if (type == PURC_VARIANT_TYPE_SET) {
        return to_variant_set(cmp_unique_key, cmp);
    }
    purc_variant_ref(cmp);
    return cmp;
}

#if 0
static inline int
cmp(purc_variant_t l, purc_variant_t r, void *ud)
{
    (void)ud;
    double dl = purc_variant_numerify(l);
    double dr = purc_variant_numerify(r);

    if (dl < dr)
        return -1;
    if (dl == dr)
        return 0;
    return 1;
}
#endif

void compare_result(purc_variant_t dst, purc_variant_t cmp)
{
    char* dst_result = variant_to_string(dst);
    char* cmp_result = variant_to_string(cmp);
    PRINTF("dst=%s\n", dst_result);
    PRINTF("cmp=%s\n", cmp_result);
    PRINTF("orig=%s\n", cmp_result);
    free(cmp_result);
    free(dst_result);

    PRINT_VARIANT(dst);
    PRINT_VARIANT(cmp);
    if (dst->type == PVT(_ARRAY) && cmp->type == PVT(_SET)) {
        const char* unique_key = NULL;
        purc_variant_t v = to_variant_set(unique_key, dst);
        ASSERT_NE(v, nullptr);
        dst = v;
    }
    else {
        dst = purc_variant_ref(dst);
    }
    cmp = purc_variant_ref(cmp);
    int diff = pcvariant_diff(dst, cmp);
    PURC_VARIANT_SAFE_CLEAR(dst);
    PURC_VARIANT_SAFE_CLEAR(cmp);
    ASSERT_EQ(diff, 0);
}

TEST_P(TestCaseData, container_ops)
{
    const struct test_case data = get_data();
    PRINTF("filename=%s\n", data.filename);

    purc_variant_t test_case_variant = purc_variant_make_from_json_string(
            data.data, strlen(data.data));
    ASSERT_NE(test_case_variant, PURC_VARIANT_INVALID);

    purc_variant_t ignore_var = purc_variant_object_get_by_ckey_ex(test_case_variant,
                "ignore", true);
    if (ignore_var != PURC_VARIANT_INVALID
            && purc_variant_booleanize(ignore_var)) {
        return;
    }

    purc_variant_t dst = build_test_dst(test_case_variant);
    ASSERT_NE(dst, PURC_VARIANT_INVALID);

    purc_variant_t src = build_test_src(test_case_variant);
    ASSERT_NE(src, PURC_VARIANT_INVALID);

    // purc_variant_t cmp = purc_variant_object_get_by_ckey_ex(test_case_variant,
    //             "cmp", true);
    // ASSERT_NE(cmp, PURC_VARIANT_INVALID);
    purc_variant_t cmp = build_test_cmp(test_case_variant);
    ASSERT_NE(cmp, PURC_VARIANT_INVALID);

    //  do container ops
    purc_variant_t ops_type_var = purc_variant_object_get_by_ckey_ex(test_case_variant,
                "ops", true);
    ASSERT_NE(ops_type_var, PURC_VARIANT_INVALID);

    const char* ops_type_str = purc_variant_get_string_const(ops_type_var);
    enum container_ops_type ops_type = to_ops_type(ops_type_str);

    bool result = false;
    switch (ops_type) {
        case CONTAINER_OPS_TYPE_DISPLACE:
            result = pcvariant_container_displace(dst, src, true);
            ASSERT_EQ(result, true);
            break;

        case CONTAINER_OPS_TYPE_APPEND:
            result = pcvariant_array_append_another(dst, src, true);
            ASSERT_EQ(result, true);
            break;

        case CONTAINER_OPS_TYPE_PREPEND:
            result = pcvariant_array_prepend_another(dst, src, true);
            ASSERT_EQ(result, true);
            break;

        case CONTAINER_OPS_TYPE_REMOVE:
            result = pcvariant_container_remove(dst, src, true);
            ASSERT_EQ(result, true);
            break;

        case CONTAINER_OPS_TYPE_INSERT_BEFORE:
            {
                purc_variant_t idx_var = purc_variant_object_get_by_ckey_ex(
                        test_case_variant, "idx", true);
                ASSERT_NE(idx_var, PURC_VARIANT_INVALID);
                int64_t idx = 0;
                purc_variant_cast_to_longint(idx_var, &idx, false);
                result = pcvariant_array_insert_another_before(
                        dst, idx, src, true);
                ASSERT_EQ(result, true);
            }
            break;

        case CONTAINER_OPS_TYPE_INSERT_AFTER:
            {
                purc_variant_t idx_var = purc_variant_object_get_by_ckey_ex(
                        test_case_variant, "idx", true);
                ASSERT_NE(idx_var, PURC_VARIANT_INVALID);
                int64_t idx = 0;
                purc_variant_cast_to_longint(idx_var, &idx, false);
                result = pcvariant_array_insert_another_after(
                        dst, idx, src, true);
                ASSERT_EQ(result, true);
            }
            break;

        case CONTAINER_OPS_TYPE_MERGE:
        case CONTAINER_OPS_TYPE_UNITE:
            {
                ssize_t r;
                if (purc_variant_is_object(dst)) {
                    r = purc_variant_object_unite(dst, src,
                            PCVRNT_CR_METHOD_OVERWRITE);
                }
                else {
                    r = purc_variant_set_unite(dst, src,
                            PCVRNT_CR_METHOD_OVERWRITE);
                }
                ASSERT_NE(r, -1);
            }
            break;

        case CONTAINER_OPS_TYPE_INTERSECT:
            {
                ssize_t r;
                if (purc_variant_is_object(dst)) {
                    r = purc_variant_object_intersect(dst, src);
                }
                else {
                    r = purc_variant_set_intersect(dst, src);
                }
                ASSERT_NE(r, -1);
            }
            break;

        case CONTAINER_OPS_TYPE_SUBTRACT:
            {
                ssize_t r;
                if (purc_variant_is_object(dst)) {
                    r = purc_variant_object_subtract(dst, src);
                }
                else {
                    r = purc_variant_set_subtract(dst, src);
                }
                ASSERT_NE(r, -1);
            }
            break;

        case CONTAINER_OPS_TYPE_XOR:
            {
                ssize_t r;
                if (purc_variant_is_object(dst)) {
                    r = purc_variant_object_xor(dst, src);
                }
                else {
                    r = purc_variant_set_xor(dst, src);
                }
                ASSERT_NE(r, -1);
            }
            break;

        case CONTAINER_OPS_TYPE_OVERWRITE:
            {
                ssize_t r;
                if (purc_variant_is_object(dst)) {
                    r = purc_variant_object_overwrite(dst, src,
                            PCVRNT_NR_METHOD_IGNORE);
                }
                else {
                    r = purc_variant_set_overwrite(dst, src,
                            PCVRNT_NR_METHOD_IGNORE);
                }
                ASSERT_NE(r, -1);
            }
            break;
    }

    compare_result(dst, cmp);

    purc_variant_unref(src);
    purc_variant_unref(dst);
    purc_variant_unref(cmp);
    purc_variant_unref(test_case_variant);
}

char* read_file (const char* file)
{
    FILE* fp = fopen (file, "r");
    if (fp == NULL) {
        return NULL;
    }
    fseek (fp, 0, SEEK_END);
    size_t sz = ftell (fp);
    char* buf = (char*) malloc(sz + 1);
    fseek (fp, 0, SEEK_SET);
    sz = fread (buf, 1, sz, fp);
    fclose (fp);
    buf[sz] = 0;
    return buf;
}

char inner_test_data[] = "" \
    "{" \
    "    \"ignore\": false," \
    "    \"error\": 0," \
    "    \"ops\": \"displace\"," \
    "    \"idx\": 0," \
    "    \"src_type\": \"object\"," \
    "    \"src_unique_key\": null," \
    "    \"src\": {" \
    "        \"id\": 2," \
    "        \"name\": \"name src\"," \
    "        \"title\": \"title src\"" \
    "    }," \
    "    \"dst_type\": \"object\"," \
    "    \"dst_unique_key\": null," \
    "    \"dst\": {" \
    "        \"id\": 1," \
    "        \"name\": \"name dst\"" \
    "    }," \
    "    \"cmp\": {" \
    "        \"id\": 2," \
    "        \"name\": \"name src\"," \
    "        \"title\": \"title src\"" \
    "    }" \
    "}";

std::vector<test_case> load_test_case()
{
    int r = 0;
    std::vector<test_case> test_cases;
    glob_t globbuf;
    memset(&globbuf, 0, sizeof(globbuf));


    char path[PATH_MAX+1];
    const char* env = "VARIANT_TEST_CONTAINER_OPS_PATH";
    test_getpath_from_env_or_rel(path, sizeof(path), env, "/data/*.json");

    if (!path[0])
        goto end;

    globbuf.gl_offs = 0;
    r = glob(path, GLOB_DOOFFS | GLOB_APPEND, NULL, &globbuf);

    if (r == 0) {
        for (size_t i = 0; i < globbuf.gl_pathc; ++i) {
            char *buf  = read_file(globbuf.gl_pathv[i]);
            add_test_case(test_cases,
                    basename((char *)globbuf.gl_pathv[i]), buf);
            free(buf);
        }
    }
    globfree(&globbuf);

    if (test_cases.empty()) {
        add_test_case(test_cases, "inner_test", inner_test_data);
    }

end:
    return test_cases;
}

INSTANTIATE_TEST_SUITE_P(purc_variant, TestCaseData,
        testing::ValuesIn(load_test_case()));

TEST(variant, clone)
{
    PurCInstance purc("cn.fmsoft.hybridos.test", "purc_variant", false);

    const char *s;
    purc_variant_t set, cloned;
    int diff;

    s = "[!'name', {name:[{first:xiaohong,last:xu}]}, {name:[{first:shuming, last:xue}]}]";
    set = pcejson_parser_parse_string(s, 0, 0);
    ASSERT_NE(set, nullptr);
    ASSERT_EQ(2, purc_variant_set_get_size(set));

    do {
        cloned = purc_variant_container_clone_recursively(set);
        if (cloned == PURC_VARIANT_INVALID) {
            PRINT_VARIANT(set);
            ADD_FAILURE() << "clone failed" << std::endl;
            break;
        }

        diff = purc_variant_compare_ex(set, cloned, PCVRNT_COMPARE_METHOD_AUTO);
        if (diff) {
            PRINT_VARIANT(set);
            PRINT_VARIANT(cloned);
            ADD_FAILURE() << "internal logic error for clone" << std::endl;
            break;
        }
    } while (0);

    PURC_VARIANT_SAFE_CLEAR(cloned);
    PURC_VARIANT_SAFE_CLEAR(set);
}

