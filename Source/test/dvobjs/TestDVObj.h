#pragma once

#include "purc.h"
#include <iostream>

typedef purc_variant_t (*fn_expected)(purc_variant_t dvobj, const char* name);
typedef bool (*fn_cmp)(purc_variant_t result, purc_variant_t expected);

struct dvobj_result {
    const char             *name;
    const char             *jsonee;

    fn_expected             expected;
    fn_cmp                  vrtcmp;
    int                     errcode;
};

class TestDVObj {
public:
    static void get_variant_total_info (size_t *mem, size_t *value, size_t *resv)
    {
        struct purc_variant_stat * stat = purc_variant_usage_stat();

        *mem = stat->sz_total_mem;
        *value = stat->nr_total_values;
        *resv = stat->nr_reserved;
    }

    static purc_variant_t dvobj_new(const char *dvobj_name)
    {
        if (strcmp(dvobj_name, "SYSTEM") == 0) {
            return purc_dvobj_system_new();
        }
        else if (strcmp(dvobj_name, "DATETIME") == 0) {
            return purc_dvobj_datetime_new();
        }
        else if (strcmp(dvobj_name, "HVML") == 0) {
            return purc_dvobj_hvml_new(NULL);
        }
        else if (strcmp(dvobj_name, "EJSON") == 0) {
            return purc_dvobj_ejson_new();
        }
        else if (strcmp(dvobj_name, "SESSION") == 0) {
            return purc_dvobj_session_new();
        }
        else if (strcmp(dvobj_name, "L") == 0) {
            return purc_dvobj_logical_new();
        }
        else if (strcmp(dvobj_name, "T") == 0) {
            return purc_dvobj_text_new();
        }
        else if (strcmp(dvobj_name, "STR") == 0) {
            return purc_dvobj_string_new();
        }
#if 0
        else if (strcmp(dvobj_name, "URL") == 0) {
            return purc_dvobj_url_new();
        }
#endif

        return PURC_VARIANT_INVALID;
    }

    static purc_variant_t get_dvobj(void* ctxt, const char* name) {
        (void)name;
        return (purc_variant_t)ctxt;
    }

    static void run_testcases(const char *dvobj_name,
            const struct dvobj_result *test_cases, size_t n);

    static void run_testcases_in_file(const char *dvobj_name,
            const char *path_name, const char *file_name);
};

