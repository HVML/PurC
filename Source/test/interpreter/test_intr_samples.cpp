#include "purc.h"
#include "private/utils.h"
#include "private/interpreter.h"

#include "../helpers.h"

#include <gtest/gtest.h>

struct sample_data {
    const char                   *input_hvml;
    const char                   *expected_html;
};

struct sample_ctxt {
    char                   *input_hvml;
    char                   *expected_html;
    pchtml_html_document_t *html;

    int                     terminated;
};

static void
sample_release(struct sample_ctxt *ud)
{
    if (!ud)
        return;

    if (ud->input_hvml) {
        free(ud->input_hvml);
        ud->input_hvml = NULL;
    }

    if (ud->expected_html) {
        free(ud->expected_html);
        ud->expected_html = NULL;
    }

    if (ud->html) {
        pchtml_html_document_destroy(ud->html);
        ud->html =  NULL;
    }
}

static void
sample_destroy(struct sample_ctxt *ud)
{
    if (!ud)
        return;

    sample_release(ud);
    free(ud);
}

static void
on_terminated(pcintr_stack_t stack, void *ctxt)
{
    struct sample_ctxt *ud = (struct sample_ctxt*)ctxt;
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

static void
on_cleanup(pcintr_stack_t stack, void *ctxt)
{
    UNUSED_PARAM(stack);

    struct sample_ctxt *ud = (struct sample_ctxt*)ctxt;
    sample_destroy(ud);
}

static int
add_sample(const struct sample_data *sample)
{
    struct sample_ctxt *ud;
    ud = (struct sample_ctxt*)calloc(1, sizeof(*ud));
    if (!ud) {
        ADD_FAILURE()
            << "Out of memory" << std::endl;
        return -1;
    }

    ud->html = pchmtl_html_load_document_with_buf(
            (const unsigned char*)sample->expected_html,
            strlen(sample->expected_html));
    if (!ud->html) {
        ADD_FAILURE()
            << "failed to parsing html:" << std::endl
            << sample->expected_html << std::endl;
        sample_destroy(ud);
        return -1;
    }


    ud->input_hvml = strdup(sample->input_hvml);
    ud->expected_html = strdup(sample->expected_html);

    if (!ud->input_hvml ||
        !ud->expected_html)
    {
        ADD_FAILURE()
            << "Out of memory" << std::endl;
        sample_destroy(ud);
        return -1;
    }

    struct pcintr_supervisor_ops ops = {};
    ops.on_terminated = on_terminated;
    ops.on_cleanup    = on_cleanup;

    purc_vdom_t vdom;
    vdom = purc_load_hvml_from_string_ex(sample->input_hvml, &ops, ud);

    if (vdom == NULL) {
        ADD_FAILURE()
            << "failed to loading hvml:" << std::endl
            << sample->input_hvml << std::endl;
        sample_destroy(ud);
        return -1;
    }

    return 0;
}

TEST(samples, basic)
{
    PurCInstance purc;

    ASSERT_TRUE(purc);

    struct sample_data sample = {
        .input_hvml = "<hvml><head></head><body>hello</body></hvml>",
        .expected_html = "hello",
    };

    add_sample(&sample);

    purc_run(PURC_VARIANT_INVALID, NULL);
}

static void
run_tests(struct sample_data *samples, size_t nr, int parallel)
{
    for (size_t i=0; i<nr; ++i) {
        const struct sample_data *sample = samples + i;
        add_sample(sample);
        if (!parallel)
            purc_run(PURC_VARIANT_INVALID, NULL);
    }

    if (parallel)
        purc_run(PURC_VARIANT_INVALID, NULL);
}

TEST(samples, multiples)
{
    PurCInstance purc;

    ASSERT_TRUE(purc);

    struct sample_data samples[] = {
        {
            "<!DOCTYPE hvml>"
            "<hvml target=\"html\" lang=\"en\">"
            "    <head>"
            "        <title>计算器</title>"
            "        <link rel=\"stylesheet\" type=\"text/css\" href=\"calculator.css\" />"
            ""
            "        <init as=\"buttons\" uniquely>"
            "            ["
            "                { \"letters\": \"7\", \"class\": \"number\" },"
            "                { \"letters\": \"8\", \"class\": \"number\" },"
            "                { \"letters\": \"9\", \"class\": \"number\" },"
            "                { \"letters\": \"←\", \"class\": \"c_blue backspace\" },"
            "                { \"letters\": \"C\", \"class\": \"c_blue clear\" },"
            "                { \"letters\": \"4\", \"class\": \"number\" },"
            "                { \"letters\": \"5\", \"class\": \"number\" },"
            "                { \"letters\": \"6\", \"class\": \"number\" },"
            "                { \"letters\": \"×\", \"class\": \"c_blue multiplication\" },"
            "                { \"letters\": \"÷\", \"class\": \"c_blue division\" },"
            "                { \"letters\": \"1\", \"class\": \"number\" },"
            "                { \"letters\": \"2\", \"class\": \"number\" },"
            "                { \"letters\": \"3\", \"class\": \"number\" },"
            "                { \"letters\": \"+\", \"class\": \"c_blue plus\" },"
            "                { \"letters\": \"-\", \"class\": \"c_blue subtraction\" },"
            "                { \"letters\": \"0\", \"class\": \"number\" },"
            "                { \"letters\": \"00\", \"class\": \"number\" },"
            "                { \"letters\": \".\", \"class\": \"number\" },"
            "                { \"letters\": \"%\", \"class\": \"c_blue percent\" },"
            "                { \"letters\": \"=\", \"class\": \"c_yellow equal\" },"
            "            ]"
            "        </init>"
            "    </head>"
            ""
            "    <body>"
            "        <div id=\"calculator\">"
            ""
            "            <div id=\"c_title\">"
            "                <h2>计算器</h2>"
            "            </div>"
            ""
            "            <div id=\"c_text\">"
            "                <input type=\"text\" id=\"text\" value=\"0\" readonly=\"readonly\" />"
            "            </div>"
            ""
            "            <div id=\"c_value\">"
            "                <archetype name=\"button\">"
            "                    <li class=\"$?.class\">$?.letters</li>"
            "                </archetype>"
            ""
            "                <ul>"
            "                    <iterate on=\"$buttons\">"
            "                        <update on=\"$@\" to=\"append\" with=\"$button\" />"
            "                        <except type=\"NoData\" raw>"
            "                            <p>Bad data!</p>"
            "                        </except>"
            "                    </iterate>"
            "                </ul>"
            "            </div>"
            "        </div>"
            "    </body>"
            ""
            "</hvml>",

            "<html lang=\"en\" target=\"html\">"
            "    <head>"
            "        <title>计算器</title>"
            "        <link href=\"calculator.css\" rel=\"stylesheet\" type=\"text/css\" />"
            "    </head>"
            ""
            "    <body>"
            "        <div id=\"calculator\">"
            "            <div id=\"c_title\">"
            "                <h2>计算器</h2>"
            "            </div>"
            "            <div id=\"c_text\">"
            "                <input id=\"text\" readonly=\"readonly\" type=\"text\" value=\"0\" />"
            "            </div>"
            "            <div id=\"c_value\">"
            "                <ul>"
            "                    <li class=\"number\">7</li>"
            "                    <li class=\"number\">8</li>"
            "                    <li class=\"number\">9</li>"
            "                    <li class=\"c_blue backspace\">←</li>"
            "                    <li class=\"c_blue clear\">C</li>"
            "                    <li class=\"number\">4</li>"
            "                    <li class=\"number\">5</li>"
            "                    <li class=\"number\">6</li>"
            "                    <li class=\"c_blue multiplication\">×</li>"
            "                    <li class=\"c_blue division\">÷</li>"
            "                    <li class=\"number\">1</li>"
            "                    <li class=\"number\">2</li>"
            "                    <li class=\"number\">3</li>"
            "                    <li class=\"c_blue plus\">+</li>"
            "                    <li class=\"c_blue subtraction\">-</li>"
            "                    <li class=\"number\">0</li>"
            "                    <li class=\"number\">00</li>"
            "                    <li class=\"number\">.</li>"
            "                    <li class=\"c_blue percent\">%</li>"
            "                    <li class=\"c_yellow equal\">=</li>"
            "                </ul>"
            "            </div>"
            "        </div>"
            "    </body>"
            ""
            "</html>",
        },
        {
            "<!DOCTYPE hvml>"
            "<hvml target=\"html\" lang=\"en\">"
            "    <head>"
            "        <base href=\"$HVML.base(! 'https://gitlab.fmsoft.cn/hvml/hvml-docs/raw/master/samples/calculator/' )\" />"
            ""
            "<!--"
            "        <update on=\"$T.map\" from=\"assets/{$SYSTEM.locale}.json\" to=\"merge\" />"
            "-->"
            ""
            "        <update on=\"$T.map\" to=\"merge\">"
            "           {"
            "               \"HVML Calculator\": \"HVML 计算器\","
            "               \"Current Time: \": \"当前时间：\""
            "           }"
            "        </update>"
            ""
            "<!--"
            "        <init as=\"buttons\" from=\"assets/buttons.json\" />"
            "-->"
            ""
            "        <init as=\"buttons\" uniquely>"
            "            ["
            "                { \"letters\": \"7\", \"class\": \"number\" },"
            "                { \"letters\": \"8\", \"class\": \"number\" },"
            "                { \"letters\": \"9\", \"class\": \"number\" },"
            "                { \"letters\": \"←\", \"class\": \"c_blue backspace\" },"
            "                { \"letters\": \"C\", \"class\": \"c_blue clear\" },"
            "                { \"letters\": \"4\", \"class\": \"number\" },"
            "                { \"letters\": \"5\", \"class\": \"number\" },"
            "                { \"letters\": \"6\", \"class\": \"number\" },"
            "                { \"letters\": \"×\", \"class\": \"c_blue multiplication\" },"
            "                { \"letters\": \"÷\", \"class\": \"c_blue division\" },"
            "                { \"letters\": \"1\", \"class\": \"number\" },"
            "                { \"letters\": \"2\", \"class\": \"number\" },"
            "                { \"letters\": \"3\", \"class\": \"number\" },"
            "                { \"letters\": \"+\", \"class\": \"c_blue plus\" },"
            "                { \"letters\": \"-\", \"class\": \"c_blue subtraction\" },"
            "                { \"letters\": \"0\", \"class\": \"number\" },"
            "                { \"letters\": \"00\", \"class\": \"number\" },"
            "                { \"letters\": \".\", \"class\": \"number\" },"
            "                { \"letters\": \"%\", \"class\": \"c_blue percent\" },"
            "                { \"letters\": \"=\", \"class\": \"c_yellow equal\" },"
            "            ]"
            "        </init>"
            ""
            "        <title>$T.get('HVML Calculator')</title>"
            ""
            "        <update on=\"$TIMERS\" to=\"displace\">"
            "            ["
            "                { \"id\" : \"clock\", \"interval\" : 1000, \"active\" : \"yes\" },"
            "            ]"
            "        </update>"
            ""
            "        <link rel=\"stylesheet\" type=\"text/css\" href=\"assets/calculator.css\" />"
            "    </head>"
            ""
            "    <body>"
            "        <div id=\"calculator\">"
            ""
            "            <div id=\"c_text\">"
            "                <input type=\"text\" id=\"text\" value=\"0\" readonly=\"readonly\" />"
            "            </div>"
            ""
            "            <div id=\"c_value\">"
            "                <archetype name=\"button\">"
            "                    <li class=\"$?.class\">$?.letters</li>"
            "                </archetype>"
            ""
            "                <ul>"
            "                    <iterate on=\"$buttons\">"
            "                        <update on=\"$@\" to=\"append\" with=\"$button\" />"
            "                        <except type=\"NoData\" raw>"
            "                            <p>Bad data!</p>"
            "                        </except>"
            "                    </iterate>"
            "                </ul>"
            "            </div>"

            "            <div id=\"c_title\">"
            "                <h2 id=\"c_title\">$T.get('HVML Calculator')"
            "                    <small>$T.get('Current Time: ')<span id=\"clock\">abc</span></small>"
            "                </h2>"
            "                <observe on=\"$TIMERS\" for=\"expired:clock\">"
            "                    <update on=\"#clock\" at=\"textContent\" with=\"xyz\" />"
            "<choose on=\"foo\" by=\"this is to throw exception intentionally\" />"
            "                </observe>"
            "            </div>"
            "        </div>"
            "    </body>"
            ""
            "</hvml>",
            "<html lang=\"en\" target=\"html\">"
            "    <head>"
            "        <base href=\"https://gitlab.fmsoft.cn/hvml/hvml-docs/raw/master/samples/calculator/\" />"
            "        <title>HVML 计算器</title>"
            "        <link href=\"assets/calculator.css\" rel=\"stylesheet\" type=\"text/css\" />"
            "    </head>"
            ""
            "    <body>"
            "        <div id=\"calculator\">"
            "            <div id=\"c_text\">"
            "                <input id=\"text\" readonly=\"readonly\" type=\"text\" value=\"0\" />"
            "            </div>"
            "            <div id=\"c_value\">"
            "                <ul>"
            "                    <li class=\"number\">7</li>"
            "                    <li class=\"number\">8</li>"
            "                    <li class=\"number\">9</li>"
            "                    <li class=\"c_blue backspace\">←</li>"
            "                    <li class=\"c_blue clear\">C</li>"
            "                    <li class=\"number\">4</li>"
            "                    <li class=\"number\">5</li>"
            "                    <li class=\"number\">6</li>"
            "                    <li class=\"c_blue multiplication\">×</li>"
            "                    <li class=\"c_blue division\">÷</li>"
            "                    <li class=\"number\">1</li>"
            "                    <li class=\"number\">2</li>"
            "                    <li class=\"number\">3</li>"
            "                    <li class=\"c_blue plus\">+</li>"
            "                    <li class=\"c_blue subtraction\">-</li>"
            "                    <li class=\"number\">0</li>"
            "                    <li class=\"number\">00</li>"
            "                    <li class=\"number\">.</li>"
            "                    <li class=\"c_blue percent\">%</li>"
            "                    <li class=\"c_yellow equal\">=</li>"
            "                </ul>"
            "            </div>"
            "            <div id=\"c_title\">"
            "                <h2 id=\"c_title\">HVML 计算器<small>当前时间：<span id=\"clock\">xyz</span></small>"
            "                </h2>"
            "            </div>"
            "        </div>"
            "    </body>"
            ""
            "</html>",
        },
        {
            "<hvml><body><div id='owner'></div><update on='#owner' at='textContent' to='append' with='hello' /><update on='#owner' at='textContent' to='displace' with='world' /></body></hvml>",
            "<div id='owner'>world</div>",
        },
    };

    run_tests(samples, PCA_TABLESIZE(samples), 0);
}

