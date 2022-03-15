#include "purc.h"
#include "private/hvml.h"
#include "private/utils.h"
#include "purc-rwstream.h"
#include "hvml/hvml-token.h"

#include <gtest/gtest.h>


TEST(hvml, basic)
{
    const char* hvml = "<hvml><head a='b'/></hvml>";

    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;

    // initial purc
    ret = purc_init_ex (PURC_MODULE_HVML, "cn.fmsoft.hybridos.test",
            "test_init", &info);

    ASSERT_EQ (ret, PURC_ERROR_OK);

    struct pchvml_parser* parser = pchvml_create(0, 32);
    size_t sz = strlen (hvml);
    purc_rwstream_t rws = purc_rwstream_new_from_mem((void*)hvml, sz);

    struct pchvml_buffer* buffer = pchvml_buffer_new();

    struct pchvml_token* token = NULL;
    while((token = pchvml_next_token(parser, rws)) != NULL) {
        struct pchvml_buffer* token_buff = pchvml_token_to_string(token);
        if (token_buff) {
            pchvml_buffer_append_another(buffer, token_buff);
            pchvml_buffer_destroy(token_buff);
        }
        enum pchvml_token_type type = pchvml_token_get_type(token);
        pchvml_token_destroy(token);
        token = NULL;
        if (type == PCHVML_TOKEN_EOF) {
            break;
        }
    }

    const char* serial = pchvml_buffer_get_buffer(buffer);
    fprintf(stderr, "input: [%s]\n", hvml);
    fprintf(stderr, "parsed: [%s]\n", serial);

    purc_rwstream_destroy(rws);
    pchvml_buffer_destroy(buffer);
    pchvml_destroy(parser);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

