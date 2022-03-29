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

class TestDVObj {
public:
    TestDVObj();
    ~TestDVObj();

    static void get_variant_total_info (size_t *mem, size_t *value, size_t *resv)
    {
        struct purc_variant_stat * stat = purc_variant_usage_stat();

        *mem = stat->sz_total_mem;
        *value = stat->nr_total_values;
        *resv = stat->nr_reserved;
    }

    purc_variant_t dvobj_new(const char *dvobj_name);

    static purc_variant_t get_dvobj(void* ctxt, const char* name);

    void run_testcases(const struct dvobj_result *test_cases, size_t n);

    void run_testcases_in_file(const char *path_name, const char *file_name);

private:
    typedef map <string, purc_variant_t> dvobj_map_t;
    dvobj_map_t m_dvobjs;
};

