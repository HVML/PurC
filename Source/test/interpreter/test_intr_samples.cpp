#include "purc.h"
#include "private/utils.h"
#include "private/interpreter.h"

#include "../helpers.h"

#include <gtest/gtest.h>

struct sample_data {
    const char             *input_hvml;
    const char             *expected_html;
    pchtml_html_document_t *html;

    int                     terminated;
};

static void
on_terminated(pcintr_stack_t stack, void *ctxt)
{
    struct sample_data *ud = (struct sample_data*)ctxt;
    pchtml_html_document_t *doc = stack->doc;

    if (ud->terminated) {
        ADD_FAILURE() << "internal logic error: reentrant" << std::endl;
        return;
    }
    ud->terminated = 1;

    int diff = 0;
    int r = 0;
    pcintr_util_comp_docs(doc, ud->html, &diff);
    if (r == 0 && diff == 0)
        return;

    char buf[8192];
    size_t nr = sizeof(nr);
    char *p = pchtml_doc_snprintf_plain(doc, buf, &nr, "");

    ADD_FAILURE()
        << "failed to compare:" << std::endl
        << "input:" << std::endl << ud->input_hvml << std::endl
        << "output:" << std::endl << p << std::endl
        << "expected:" << std::endl << ud->expected_html << std::endl;

    if (p != buf)
        free(p);
}

TEST(samples, basic)
{
    PurCInstance purc;

    ASSERT_TRUE(purc);

    struct pcintr_supervisor_ops ops = {
        .on_terminated         = on_terminated,
        .on_cleanup            = NULL,
    };
    const char *input = "<hvml><head></head><body>hello</body></hvml>";
    const char *expected_html = "hello";

    struct sample_data sample = {};
    sample.input_hvml      = input,
    sample.expected_html   = expected_html,
    sample.html = pchmtl_html_load_document_with_buf(
            (const unsigned char*)expected_html, strlen(expected_html));

    if (!sample.html) {
        ADD_FAILURE()
            << "failed to parsing html:" << std::endl
            << expected_html << std::endl;

        return;
    }

    purc_vdom_t vdom = purc_load_hvml_from_string_ex(input, &ops, &sample);

    if (vdom == NULL) {
        pchtml_html_document_destroy(sample.html);
        ADD_FAILURE()
            << "failed to loading hvml:" << std::endl
            << input << std::endl;
        return;
    }

    purc_run(PURC_VARIANT_INVALID, NULL);

    if (sample.terminated == 0) {
        ADD_FAILURE()
            << "internal logic error: running failure" << std::endl;
        return;
    }

    pchtml_html_document_destroy(sample.html);
}


