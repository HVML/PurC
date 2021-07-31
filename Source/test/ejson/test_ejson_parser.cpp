#include "purc.h"

#include "private/ejson.h"
#include "purc-rwstream.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

using namespace std;

struct T {
    const char* json;
    const char* comp;
};

class ejson_parser_vcm_eval : public testing::TestWithParam<struct T>
{
protected:
    void SetUp() {
        t.json = GetParam().json;
        t.comp = GetParam().comp;
    }
    void TearDown() {}
    struct T t;
};

TEST_P(ejson_parser_vcm_eval, test0)
{
    int ret = purc_init ("cn.fmsoft.hybridos.test", "ejson", NULL);
    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_rwstream_t rws = purc_rwstream_new_from_mem((void*)t.json, strlen(t.json));

    struct pcvcm_node* root = NULL;
    struct pcejson* parser = NULL;
    pcejson_parse (&root, &parser, rws);
    ASSERT_NE (root, nullptr);

    purc_variant_t vt = pcvcm_eval (root, NULL);
    ASSERT_NE(vt, PURC_VARIANT_INVALID);

    char buf[1024] = {0};
    purc_rwstream_t my_rws = purc_rwstream_new_from_mem(buf, sizeof(buf) - 1);
    ASSERT_NE(my_rws, nullptr);

    size_t len_expected = 0;
    ssize_t n = purc_variant_serialize(vt, my_rws,
            0, PCVARIANT_SERIALIZE_OPT_PLAIN, &len_expected);
    ASSERT_GT(n, 0);
    buf[n] = 0;
    ASSERT_STREQ(buf, t.comp);

    purc_variant_unref(vt);
    purc_rwstream_destroy(my_rws);
    purc_rwstream_destroy(rws);

    pctree_node_destroy (pcvcm_node_to_pctree_node(root),
            pcvcm_node_pctree_node_destory_callback);

    pcejson_destroy(parser);
    purc_cleanup ();
}

#if 0
    T {
        "[-123123123123123123123123123123]",
        "[-123123123123123123123123123123]"
    },
    T {
        "[-23746237467327689427983274983242347982324632784]",
        "[-23746237467327689645882849906976246361885769728]"
    }
#endif

INSTANTIATE_TEST_CASE_P(ejson, ejson_parser_vcm_eval, testing::Values(
    T {
        "[123.456e-789]",
        "[0]"
    },
    T {
        "[0.4e006699999999999999999999999999999999999999999999999999999999999\
        99999999999999999999999999999999999999999999999999999999969999999006]",
        "[Infinity]"
    },
    T {
        "[-1e+9999]",
        "[-Infinity]"
    },
    T {
        "[1.5e+9999]",
        "[Infinity]"
    },
    T {
        "[-123123e100000]",
        "[-Infinity]"
    },
    T {
        "[123123e100000]",
        "[Infinity]"
    },
    T {
        "[123e-10000000]",
        "[0]"
    },
    T {
        "[100000000000000000000]",
        "[100000000000000000000]"
    },
    T {
        "{\"\\uDFAA\":0}",
        "{\"\\\\uDFAA\":0}",
    },
    T {
        "[\"\\uDADA\n\"]",
        "[\"\\\\uDADA\\n\"]",
    },
    T {
        "[\"\\uD888\\u1234\"]",
        "[\"\\\\uD888\\\\u1234\"]",
    }





));

