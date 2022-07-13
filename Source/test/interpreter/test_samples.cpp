#include "purc.h"
#include "private/utils.h"
#include "private/interpreter.h"

#include "../helpers.h"
#include "tools.h"

#include <glob.h>
#include <gtest/gtest.h>

struct sample_data {
    const char                   *input_hvml;
    const char                   *expected_html;
};

struct sample_ctxt {
    char               *input_hvml;
    char               *expected_html;
    purc_document_t     html;
    int                 terminated;
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
        purc_document_delete(ud->html);
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

static int my_cond_handler(purc_cond_t event, purc_coroutine_t cor,
        void *data)
{
    void *user_data = purc_coroutine_get_user_data(cor);
    if (!user_data) {
        return -1;
    }

    struct sample_ctxt *ud = (struct sample_ctxt*)user_data;

    if (event == PURC_COND_COR_EXITED) {
        purc_document_t doc = (purc_document_t)data;

        if (ud->terminated) {
            ADD_FAILURE() << "internal logic error: reentrant" << std::endl;
            return -1;
        }
        ud->terminated = 1;

        if (ud->html) {
            int diff = 0;
            char *ctnt = intr_util_comp_docs(doc, ud->html, &diff);
            if (ctnt != NULL && diff == 0) {
                free(ctnt);
                return 0;
            }

            ADD_FAILURE()
                << "failed to compare:" << std::endl
                << "input:" << std::endl << ud->input_hvml << std::endl
                << "output:" << std::endl << ctnt << std::endl
                << "expected:" << std::endl << ud->expected_html << std::endl;

            free(ctnt);
        }
    }
    else if (event == PURC_COND_COR_DESTROYED) {
        sample_destroy(ud);
    }

    return 0;
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

    if (sample->expected_html) {
        ud->html = purc_document_load(PCDOC_K_TYPE_HTML,
                sample->expected_html, strlen(sample->expected_html));
        if (!ud->html) {
            ADD_FAILURE()
                << "failed to parsing html:" << std::endl
                << sample->expected_html << std::endl;
            sample_destroy(ud);
            return -1;
        }
        ud->expected_html = strdup(sample->expected_html);
        if (!ud->expected_html) {
            ADD_FAILURE()
                << "Out of memory" << std::endl;
            sample_destroy(ud);
            return -1;
        }

    }

    ud->input_hvml = strdup(sample->input_hvml);
    if (!ud->input_hvml) {
        ADD_FAILURE()
            << "Out of memory" << std::endl;
        sample_destroy(ud);
        return -1;
    }

#if 0 // VW: use event handler instead
    struct pcintr_supervisor_ops ops = {};
    ops.on_terminated = on_terminated;
    ops.on_cleanup    = on_cleanup;
#endif

    purc_vdom_t vdom;
    vdom = purc_load_hvml_from_string(sample->input_hvml);

    if (vdom == NULL) {
        ADD_FAILURE()
            << "failed to loading hvml:" << std::endl
            << sample->input_hvml << std::endl;
        sample_destroy(ud);
        return -1;
    }
    else {
        purc_coroutine_t cor = purc_schedule_vdom_null(vdom);
        purc_coroutine_set_user_data(cor, ud);
    }

    return 0;
}

TEST(samples, basic)
{
    bool enable_remote_fetcher = true;
    PurCInstance purc(enable_remote_fetcher);
    purc_bind_session_variables();

    ASSERT_TRUE(purc);

    struct sample_data sample = {
        .input_hvml = "<hvml><head></head><body>hello</body></hvml>",
        .expected_html = "hello",
    };

    add_sample(&sample);

    purc_run((purc_cond_handler)my_cond_handler);
}

static void
run_tests(struct sample_data *samples, size_t nr, int parallel)
{
    for (size_t i=0; i<nr; ++i) {
        const struct sample_data *sample = samples + i;
        add_sample(sample);
        if (!parallel)
            purc_run((purc_cond_handler)my_cond_handler);
    }

    if (parallel)
        purc_run((purc_cond_handler)my_cond_handler);
}

TEST(samples, samples)
{
    PurCInstance purc;

    ASSERT_TRUE(purc);
    purc_bind_session_variables();

    setenv(PURC_ENVV_DVOBJS_PATH, SOPATH, 1);
    setenv(PURC_ENVV_EXECUTOR_PATH, SOPATH, 1);

    struct sample_data samples[] = {
        {
            "<!DOCTYPE hvml>"
            "<hvml target=\"html\" lang=\"en\">"
            "    <head>"
            "        <title>计算器</title>"
            "        <link rel=\"stylesheet\" type=\"text/css\" href=\"calculator.css\" />"
            ""
            "        <init as=\"buttons\">"
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
            NULL,
        },
        {
            "<!DOCTYPE hvml SYSTEM 'v: MATH'>"
            "<hvml target=\"html\" lang=\"en\">"
            "    <head>"
            "        <title>Fibonacci Numbers</title>"
            "    </head>"
            ""
            "    <body>"
            "        <header>"
            "            <h1>Fibonacci Numbers less than 2000</h1>"
            "            <p hvml:raw>Using named array variable ($fibonacci), $MATH, and $EJSON</p>"
            "        </header>"
            ""
            "        <init as=\"fibonacci\">"
            "            [0, 1, ]"
            "        </init>"
            ""
            "        <iterate on 1 by=\"ADD: LT 2000 BY $fibonacci[$MATH.sub($EJSON.count($fibonacci), 2)]\">"
            "            <update on=\"$fibonacci\" to=\"append\" with=\"$?\" />"
            "        </iterate>"
            ""
            "        <section>"
            "            <ol>"
            "                <iterate on=\"$fibonacci\">"
            "                    <li>$?</li>"
            "                </iterate>"
            "            </ol>"
            "        </section>"
            ""
            "        <footer>"
            "            <p>Totally $EJSON.count($fibonacci) numbers.</p>"
            "        </footer>"
            "    </body>"
            ""
            "</hvml>",
            "<html lang=\"en\" target=\"html\"><head><title>Fibonacci Numbers</title></head><body><header><h1>Fibonacci Numbers less than 2000</h1><p hvml:raw=\"\">Using named array variable ($fibonacci), $MATH, and $EJSON</p></header><section><ol><li>0</li><li>1</li><li>1</li><li>2</li><li>3</li><li>5</li><li>8</li><li>13</li><li>21</li><li>34</li><li>55</li><li>89</li><li>144</li><li>233</li><li>377</li><li>610</li><li>987</li><li>1597</li></ol></section><footer><p>Totally 18 numbers.</p></footer></body></html>",
        },
        {
            "<!DOCTYPE hvml>"
            "<hvml target=\"html\" lang=\"en\">"
            "    <head>"
            "        <title>Fibonacci Numbers</title>"
            "    </head>"
            ""
            "    <body>"
            "        <header>"
            "            <h1>Fibonacci Numbers less than 2000</h1>"
            "            <p hvml:raw>Using local array variable ($!) and negative index</p>"
            "        </header>"
            ""
            "        <init as='fibonacci' temporarily>"
            "            [0, 1, ]"
            "        </init>"
            ""
            "        <iterate on 1 by=\"ADD: LT 2000 BY $!.fibonacci[-2]\">"
            "            <update on=\"$2!.fibonacci\" to=\"append\" with=\"$?\" />"
            "        </iterate>"
            ""
            "        <section>"
            "            <ol>"
            "                <iterate on=\"$3!.fibonacci\">"
            "                    <li>$?</li>"
            "                </iterate>"
            "            </ol>"
            "        </section>"
            ""
            "        <footer>"
            "            <p>Totally $EJSON.count($2!.fibonacci) numbers.</p>"
            "        </footer>"
            "    </body>"
            ""
            "</hvml>",
            "<html lang=\"en\" target=\"html\"><head><title>Fibonacci Numbers</title></head><body><header><h1>Fibonacci Numbers less than 2000</h1><p hvml:raw=\"\">Using local array variable ($!) and negative index</p></header><section><ol><li>0</li><li>1</li><li>1</li><li>2</li><li>3</li><li>5</li><li>8</li><li>13</li><li>21</li><li>34</li><li>55</li><li>89</li><li>144</li><li>233</li><li>377</li><li>610</li><li>987</li><li>1597</li></ol></section><footer><p>Totally 18 numbers.</p></footer></body></html>",
        },
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
            "<hvml><body><div id='owner'></div><update on='#owner' at='textContent' to='append' with='hello' /><update on='#owner' at='textContent' to='displace' with='world' /></body></hvml>",
            "<div id='owner'>world</div>",
        },
    };

    run_tests(samples, PCA_TABLESIZE(samples), 0);
}

TEST(samples, foo)
{
    do {
        struct purc_instance_extra_info info = {};
        if (purc_init_ex (PURC_MODULE_HVML, "cn.fmsoft.hybridos.test",
                    "test_init", &info))
            break;
        purc_cleanup();
    } while (0);
}

