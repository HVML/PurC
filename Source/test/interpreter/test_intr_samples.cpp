#include "purc.h"
#include "private/utils.h"
#include "private/interpreter.h"

#include "../helpers.h"

#include <glob.h>
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

    if (ud->html) {
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

    if (sample->expected_html) {
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
    bool enable_remote_fetcher = true;
    PurCInstance purc(enable_remote_fetcher);

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

TEST(samples, samples)
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
            "        <init as='fibonacci' locally>"
            "            [0, 1, ]"
            "        </init>"
            ""
            "        <iterate on 1 by=\"ADD: LT 2000 BY $!.fibonacci[-2]\">"
            "            <update on=\"$1!.fibonacci\" to=\"append\" with=\"$?\" />"
            "        </iterate>"
            ""
            "        <section>"
            "            <ol>"
            "                <iterate on=\"$2!.fibonacci\">"
            "                    <li>$?</li>"
            "                </iterate>"
            "            </ol>"
            "        </section>"
            ""
            "        <footer>"
            "            <p>Totally $EJSON.count($1!.fibonacci) numbers.</p>"
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

static int
read_file(char *buf, size_t nr, const char *file)
{
    FILE *f = fopen(file, "r");
    if (!f) {
        ADD_FAILURE()
            << "Failed to open file [" << file << "]" << std::endl;
        return -1;
    }

    size_t n = fread(buf, 1, nr, f);
    if (ferror(f)) {
        ADD_FAILURE()
            << "Failed read file [" << file << "]" << std::endl;
        fclose(f);
        return -1;
    }

    fclose(f);

    if (n == sizeof(buf)) {
        ADD_FAILURE()
            << "Too small buffer to read file [" << file << "]" << std::endl;
        return -1;
    }

    buf[n] = '\0';

    return n;
}

static int
process_file(const char *file)
{
    std::cout << file << std::endl;
    char buf[1024*1024];
    int n = read_file(buf, sizeof(buf), file);
    if (n == -1)
        return -1;

    struct sample_data sample = {
        .input_hvml = buf,
        .expected_html = NULL,
    };

    return add_sample(&sample);
}

TEST(samples, files)
{
    bool enable_remote_fetcher = true;
    PurCInstance purc(enable_remote_fetcher);

    ASSERT_TRUE(purc);

    int r = 0;
    glob_t globbuf;
    memset(&globbuf, 0, sizeof(globbuf));

    char path[PATH_MAX+1];
    const char *env = "SOURCE_FILES";
    const char *rel = "data/*.hvml";
    test_getpath_from_env_or_rel(path, sizeof(path),
        env, rel);

    if (!path[0]) {
        ADD_FAILURE()
            << "internal logic error" << std::endl;
    }
    else {
        globbuf.gl_offs = 0;
        r = glob(path, GLOB_DOOFFS | GLOB_APPEND, NULL, &globbuf);
        do {
            if (r) {
                ADD_FAILURE()
                    << "Failed to globbing @["
                    << path << "]: [" << errno << "]" << strerror(errno)
                    << std::endl;
                break;
            }
            if (globbuf.gl_pathc == 0)
                break;

            for (size_t i=0; i<globbuf.gl_pathc; ++i) {
                r = process_file(globbuf.gl_pathv[i]);
                if (r)
                    break;
            }
            if (r)
                break;
            purc_run(PURC_VARIANT_INVALID, NULL);
        } while (0);
        globfree(&globbuf);
    }

    std::cerr << "env: " << env << "=" << path << std::endl;
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

