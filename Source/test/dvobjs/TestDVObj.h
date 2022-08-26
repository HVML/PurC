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
    TestDVObj(bool hvml = false);
    ~TestDVObj();

    purc_variant_t dvobj_new(const char *dvobj_name);

    static purc_variant_t get_dvobj(void* ctxt, const char* name);

    void run_testcases(const struct dvobj_result *test_cases, size_t n);

    void run_testcases_in_file(const char *file_name);

private:
    typedef map <string, purc_variant_t> dvobj_map_t;
    dvobj_map_t m_dvobjs;
    struct purc_variant_stat m_init_stat;
};

