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
#include "purc/purc-variant.h"
#include "hvml/hvml-token.h"
#include "private/dvobjs.h"

#include "../helpers.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <gtest/gtest.h>

using namespace std;

#define PRINTF(...)                                                       \
    do {                                                                  \
        fprintf(stdout, "\e[0;32m[          ] \e[0m");                    \
        fprintf(stdout, __VA_ARGS__);                                     \
    } while(false)

#if OS(LINUX) || OS(UNIX)
// get path from env or __FILE__/../<rel> otherwise
#define getpath_from_env_or_rel(_path, _len, _env, _rel) do {  \
    const char *p = getenv(_env);                                      \
    if (p) {                                                           \
        snprintf(_path, _len, "%s", p);                                \
    } else {                                                           \
        char tmp[PATH_MAX+1];                                          \
        snprintf(tmp, sizeof(tmp), __FILE__);                          \
        const char *folder = dirname(tmp);                             \
        snprintf(_path, _len, "%s/%s", folder, _rel);                  \
    }                                                                  \
} while (0)

#endif // OS(LINUX) || OS(UNIX)

struct vcm_eval_test_data {
    char *name;
    char *hvml;
    char *comp;
    int error;
};

struct find_var_ctxt {
    purc_variant_t dsystem;
    purc_variant_t nobj;
    purc_variant_t array_var;
    purc_variant_t set_var;
    purc_variant_t obj_set_var;
    purc_variant_t obj_with_nobj;
};


static inline void
push_back(std::vector<vcm_eval_test_data> &vec,
        const char *name, const char *hvml, const char *comp, int error)
{
    vcm_eval_test_data data;
    memset(&data, 0, sizeof(data));
    data.name = MemCollector::strdup(name);
    data.hvml = MemCollector::strdup(hvml);
    data.comp = MemCollector::strdup(comp);
    data.error = error;

    vec.push_back(data);
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


class test_vcm_eval : public testing::TestWithParam<vcm_eval_test_data>
{
protected:
    void SetUp() {
        purc_init_ex (PURC_MODULE_HVML, "cn.fmsoft.hybridos.test",
                "vcm_eval", NULL);
        name = GetParam().name;
        hvml = GetParam().hvml;
        comp = GetParam().comp;
        error = GetParam().error;
    }
    void TearDown() {
        purc_cleanup ();
    }
    const char* get_name() {
        return name.c_str();
    }
    const char* get_hvml() {
        return hvml.c_str();
    }
    const char* get_comp() {
        return comp.c_str();
    }
    int get_error() {
        return error;
    }
private:
    string name;
    string hvml;
    string comp;
    int error;
};

#define TO_ERROR(err_name)                                 \
    if (strcmp (err, #err_name) == 0) {                  \
        return err_name;                                 \
    }

int to_error(const char* err)
{
    TO_ERROR(PCHVML_SUCCESS);
    TO_ERROR(PCEJSON_ERROR_BAD_JSONEE_KEYWORD);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_NULL_CHARACTER);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_QUESTION_MARK_INSTEAD_OF_TAG_NAME);
    TO_ERROR(PCHVML_ERROR_EOF_BEFORE_TAG_NAME);
    TO_ERROR(PCHVML_ERROR_MISSING_END_TAG_NAME);
    TO_ERROR(PCHVML_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME);
    TO_ERROR(PCHVML_ERROR_EOF_IN_TAG);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_EQUALS_SIGN_BEFORE_ATTRIBUTE_NAME);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER_IN_ATTRIBUTE_NAME);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER_IN_UNQUOTED_ATTRIBUTE_VALUE);
    TO_ERROR(PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_ATTRIBUTES);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_SOLIDUS_IN_TAG);
    TO_ERROR(PCHVML_ERROR_CDATA_IN_HTML_CONTENT);
    TO_ERROR(PCHVML_ERROR_INCORRECTLY_OPENED_COMMENT);
    TO_ERROR(PCHVML_ERROR_ABRUPT_CLOSING_OF_EMPTY_COMMENT);
    TO_ERROR(PCHVML_ERROR_EOF_IN_COMMENT);
    TO_ERROR(PCHVML_ERROR_EOF_IN_DOCTYPE);
    TO_ERROR(PCHVML_ERROR_MISSING_WHITESPACE_BEFORE_DOCTYPE_NAME);
    TO_ERROR(PCHVML_ERROR_MISSING_DOCTYPE_NAME);
    TO_ERROR(PCHVML_ERROR_INVALID_CHARACTER_SEQUENCE_AFTER_DOCTYPE_NAME);
    TO_ERROR(PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_PUBLIC_KEYWORD);
    TO_ERROR(PCHVML_ERROR_MISSING_DOCTYPE_PUBLIC_ID);
    TO_ERROR(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_PUBLIC_ID);
    TO_ERROR(PCHVML_ERROR_ABRUPT_DOCTYPE_PUBLIC_ID);
    TO_ERROR(PCHVML_ERROR_MISSING_WHITESPACE_BETWEEN_DOCTYPE_PUB_AND_SYS);
    TO_ERROR(PCHVML_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_SYSTEM_KEYWORD);
    TO_ERROR(PCHVML_ERROR_MISSING_DOCTYPE_SYSTEM);
    TO_ERROR(PCHVML_ERROR_ABRUPT_DOCTYPE_SYSTEM);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER_AFTER_DOCTYPE_SYSTEM);
    TO_ERROR(PCHVML_ERROR_EOF_IN_CDATA);
    TO_ERROR(PCHVML_ERROR_UNKNOWN_NAMED_CHARACTER_REFERENCE);
    TO_ERROR(PCHVML_ERROR_ABSENCE_OF_DIGITS_IN_NUMERIC_CHARACTER_REFERENCE);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_CHARACTER);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_EXPONENT);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_FRACTION);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER_INTEGER);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_NUMBER);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_RIGHT_BRACE);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_RIGHT_BRACKET);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_KEY_NAME);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_COMMA);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_JSON_KEYWORD);
    TO_ERROR(PCHVML_ERROR_UNEXPECTED_BASE64);
    TO_ERROR(PCHVML_ERROR_BAD_JSON_NUMBER);
    TO_ERROR(PCHVML_ERROR_BAD_JSON_STRING_ESCAPE_ENTITY);
    TO_ERROR(PCHVML_ERROR_BAD_JSONEE);
    TO_ERROR(PCHVML_ERROR_BAD_JSONEE_ESCAPE_ENTITY);
    TO_ERROR(PCHVML_ERROR_BAD_JSONEE_VARIABLE_NAME);
    TO_ERROR(PCHVML_ERROR_EMPTY_JSONEE_NAME);
    TO_ERROR(PCHVML_ERROR_BAD_JSONEE_NAME);
    TO_ERROR(PCHVML_ERROR_BAD_JSONEE_KEYWORD);
    TO_ERROR(PCHVML_ERROR_EMPTY_JSONEE_KEYWORD);
    TO_ERROR(PCHVML_ERROR_BAD_JSONEE_UNEXPECTED_COMMA);
    TO_ERROR(PCHVML_ERROR_BAD_JSONEE_UNEXPECTED_PARENTHESIS);
    TO_ERROR(PCHVML_ERROR_BAD_JSONEE_UNEXPECTED_LEFT_ANGLE_BRACKET);
    TO_ERROR(PCHVML_ERROR_MISSING_MISSING_ATTRIBUTE_VALUE);
    TO_ERROR(PCHVML_ERROR_NESTED_COMMENT);
    TO_ERROR(PCHVML_ERROR_INCORRECTLY_CLOSED_COMMENT);
    TO_ERROR(PCHVML_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM);
    TO_ERROR(PCHVML_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE);
    TO_ERROR(PCHVML_ERROR_CHARACTER_REFERENCE_OUTSIDE_UNICODE_RANGE);
    TO_ERROR(PCHVML_ERROR_SURROGATE_CHARACTER_REFERENCE);
    TO_ERROR(PCHVML_ERROR_NONCHARACTER_CHARACTER_REFERENCE);
    TO_ERROR(PCHVML_ERROR_NULL_CHARACTER_REFERENCE);
    TO_ERROR(PCHVML_ERROR_CONTROL_CHARACTER_REFERENCE);
    TO_ERROR(PCHVML_ERROR_INVALID_UTF8_CHARACTER);
    return -1;
}


static inline purc_variant_t
attr_getter(void* native_entity, const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    return purc_variant_make_string("call get success!", false);
}

static inline purc_variant_t
attr_setter(void* native_entity, const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    return purc_variant_make_string("call setter success!", false);
}

static inline purc_variant_t
chain_getter(void* native_entity, const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    struct find_var_ctxt *ctxt = (struct find_var_ctxt *) native_entity;
    return purc_variant_ref(ctxt->nobj);
}

static inline purc_variant_t
chain_setter(void* native_entity, const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(argv);
    UNUSED_PARAM(call_flags);
    struct find_var_ctxt *ctxt = (struct find_var_ctxt *) native_entity;
    return purc_variant_ref(ctxt->nobj);
}

static inline purc_nvariant_method property_getter(void *entity,
        const char* key_name)
{
    UNUSED_PARAM(entity);
    if (strcmp(key_name, "attr") == 0) {
        return attr_getter;
    }
    else if (strcmp(key_name, "chain") == 0) {
        return chain_getter;
    }

    return NULL;
}

static inline purc_nvariant_method property_setter(void *entity,
        const char* key_name)
{
    UNUSED_PARAM(entity);
    if (strcmp(key_name, "attr") == 0) {
        return attr_setter;
    }
    else if (strcmp(key_name, "chain") == 0) {
        return chain_setter;
    }

    return NULL;
}

static purc_variant_t
nobj_getter(void* native_entity, const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(call_flags);
    return purc_variant_ref(argv[0]);
}

static purc_variant_t
nobj_setter(void* native_entity, const char *property_name,
        size_t nr_args, purc_variant_t* argv, unsigned call_flags)
{
    UNUSED_PARAM(native_entity);
    UNUSED_PARAM(property_name);
    UNUSED_PARAM(nr_args);
    UNUSED_PARAM(call_flags);
    struct find_var_ctxt *ctxt = (struct find_var_ctxt *) native_entity;
    if (property_name && strcmp(property_name, "chain") == 0) {
        return purc_variant_ref(ctxt->nobj);
    }
    return purc_variant_ref(argv[0]);
}

struct purc_native_ops native_ops = {
    .getter                     = nobj_getter,
    .setter                     = nobj_setter,

    .property_getter            = property_getter,
    .property_setter            = property_setter,
    .property_cleaner           = NULL,
    .property_eraser            = NULL,

    .updater                    = NULL,
    .cleaner                    = NULL,
    .eraser                     = NULL,
    .did_matched                = NULL,

    .on_observe                 = NULL,
    .on_forget                  = NULL,
    .on_release                 = NULL,
};

purc_variant_t find_var(void* ctxt, const char* name)
{
    struct find_var_ctxt* find_ctxt = (struct find_var_ctxt*) ctxt;
    if (strcmp(name, "SYS") == 0) {
        return find_ctxt->dsystem;
    }
    else if (strcmp(name, "NOBJ") == 0) {
        return find_ctxt->nobj;
    }
    else if (strcmp(name, "VARRAY") == 0) {
        return find_ctxt->array_var;
    }
    else if (strcmp(name, "VSET") == 0) {
        return find_ctxt->set_var;
    }
    else if (strcmp(name, "VOBJSET") == 0) {
        return find_ctxt->obj_set_var;
    }
    else if (strcmp(name, "OBJWITHNOBJ") == 0) {
        return find_ctxt->obj_with_nobj;
    }
    return PURC_VARIANT_INVALID;
}

TEST_P(test_vcm_eval, parse_and_serialize)
{
    const char* hvml = get_hvml();
    const char* comp = get_comp();
    int error_code = get_error();
    PRINTF("test case : %s\n", get_name());

    struct pchvml_parser* parser = pchvml_create(0, 32);
    size_t sz = strlen (hvml);
    purc_rwstream_t rws = purc_rwstream_new_from_mem((void*)hvml, sz);

    pchvml_switch_to_ejson_state(parser);

    struct pchvml_token* token = pchvml_next_token(parser, rws);

    int error = purc_get_last_error();
    ASSERT_EQ (error, error_code) << "Test Case : "<< get_name();

    if (error_code != PCHVML_SUCCESS)
    {
        purc_rwstream_destroy(rws);
        pchvml_destroy(parser);
        pchvml_token_destroy(token);
        return;
    }

    struct find_var_ctxt ctxt;
    purc_variant_t sys = purc_dvobj_system_new();

    purc_variant_t nobj = purc_variant_make_native((void*)&ctxt, &native_ops);

    purc_variant_t array_member_0 = purc_variant_make_string("array member 0", false);
    purc_variant_t array_member_1 = purc_variant_make_string("array member 1", false);
    purc_variant_t array_var = purc_variant_make_array(2, array_member_0,
            array_member_1);

    purc_variant_t set_value_0 = purc_variant_make_string("value 0", false);
    purc_variant_t set_value_1 = purc_variant_make_string("value 1", false);
    purc_variant_t set_var = purc_variant_make_set_by_ckey(0, NULL, NULL);
    purc_variant_set_add(set_var, set_value_0, PCVRNT_CR_METHOD_COMPLAIN);
    purc_variant_set_add(set_var, set_value_1, PCVRNT_CR_METHOD_COMPLAIN);


    purc_variant_t obj_set_val_0_k = purc_variant_make_string("kk0", false);
    purc_variant_t obj_set_val_0_v = purc_variant_make_string("vv0", false);
    purc_variant_t obj_set_val_0 = purc_variant_make_object_by_static_ckey(2,
            "okey", obj_set_val_0_k, "ovalue", obj_set_val_0_v);

    purc_variant_t obj_set_val_1_k = purc_variant_make_string("kk1", false);
    purc_variant_t obj_set_val_1_v = purc_variant_make_string("vv1", false);
    purc_variant_t obj_set_val_1 = purc_variant_make_object_by_static_ckey(2,
            "okey", obj_set_val_1_k, "ovalue", obj_set_val_1_v);

    purc_variant_t obj_set_var = purc_variant_make_set_by_ckey(2,
            "okey", obj_set_val_0, obj_set_val_1);

    purc_variant_t obj_with_nobj = purc_variant_make_object_by_static_ckey(2,
            "nobj", nobj, "chain", nobj);

    struct pcvcm_node* root = pchvml_token_get_vcm_content(token);

    ctxt = { sys, nobj, array_var, set_var, obj_set_var, obj_with_nobj};

    purc_variant_t vt = pcvcm_eval_ex (root, NULL, find_var, &ctxt, false);
    if (vt == PURC_VARIANT_INVALID) {
        PRINT_VCM_NODE(root);
    }
    ASSERT_NE(vt, PURC_VARIANT_INVALID) << "Test Case : "<< get_name();

    char buf[1024] = {0};
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr) << "Test Case : "<< get_name();

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(vt, my_rws,
            0, PCVRNT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0) << "Test Case : "<< get_name();

    buf[n] = 0;
    //fprintf(stderr, "buf=%s\n", buf);
    //fprintf(stderr, "com=%s\n", comp);
    if (strcmp(comp, "#####") != 0) {
        ASSERT_STREQ(buf, comp) << "Test Case : "<< get_name();
    }

    purc_variant_unref(obj_with_nobj);
    purc_variant_unref(obj_set_val_0_k);
    purc_variant_unref(obj_set_val_0_v);
    purc_variant_unref(obj_set_val_0);
    purc_variant_unref(obj_set_val_1_k);
    purc_variant_unref(obj_set_val_1_v);
    purc_variant_unref(obj_set_val_1);
    purc_variant_unref(obj_set_var);

    purc_variant_unref(set_value_0);
    purc_variant_unref(set_value_1);
    purc_variant_unref(set_var);

    purc_variant_unref(array_member_0);
    purc_variant_unref(array_member_1);
    purc_variant_unref(array_var);

    purc_variant_unref(vt);
    pchvml_token_destroy(token);
    purc_variant_unref(nobj);
    purc_variant_unref(sys);
    purc_rwstream_destroy(my_rws);
    purc_rwstream_destroy(rws);
    pchvml_destroy(parser);
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

std::vector<vcm_eval_test_data> read_vcm_eval_test_data()
{
    std::vector<vcm_eval_test_data> vec;

    const char* env = "VCM_EVAL_TEST_FILES_PATH";
    char data_path[PATH_MAX+1] =  {0};
    getpath_from_env_or_rel(data_path, sizeof(data_path), env,
            "test_vcm_eval_files");

    if (strlen(data_path)) {
        char file_path[1024] = {0};
        strcpy (file_path, data_path);
        strcat (file_path, "/test_files_list");

        FILE* fp = fopen(file_path, "r");
        if (fp) {
            char file[PATH_MAX+16] = {0};

            char* line = NULL;
            size_t sz = 0;
            ssize_t read = 0;
            while ((read = getline(&line, &sz, fp)) != -1) {
                if (line && line[0] != '#') {
                    char* name = strtok (trim(line), " ");
                    if (!name) {
                        continue;
                    }

                    char* err = strtok (NULL, " ");
                    int error = PCEJSON_SUCCESS;
                    if (err != NULL) {
                        error = to_error (err);
                    }

                    int n;
                    n = snprintf(file, sizeof(file), "%s/%s.json", data_path, name);
                    if (n>=0 && (size_t)n>=sizeof(file)) {
                        // to circumvent format-truncation warning
                        ;
                    }
                    char* buf = read_file (file);

                    if (!buf) {
                        continue;
                    }

                    n = snprintf(file, sizeof(file), "%s/%s.serial", data_path, name);
                    if (n>=0 && (size_t)n>=sizeof(file)) {
                        // to circumvent format-truncation warning
                        ;
                    }
                    char* comp_buf = read_file (file);
                    if (!comp_buf) {
                        free (buf);
                        continue;
                    }

                    push_back(vec, name, buf, trim(comp_buf), error);

                    free (buf);
                    free (comp_buf);
                }
            }
            free (line);
            fclose(fp);
        }
    }

    if (vec.empty()) {
        push_back(vec, "array", "[123]\n", "[123]", 0);
        push_back(vec, "unquoted_key", "{key:1}\n", "{\"key\":1}", 0);
        push_back(vec, "single_quoted_key", "{'key':'2'}\n", "{\"key\":\"2\"}", 0);
    }
    return vec;
}

INSTANTIATE_TEST_SUITE_P(vcm_eval, test_vcm_eval,
        testing::ValuesIn(read_vcm_eval_test_data()));

