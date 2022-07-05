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

#if 0 // VW: use event handler instead
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

}
#endif

static void my_event_handler(purc_coroutine_t cor,
        pccor_event_t event, void *data)
{
    void *user_data = purc_coroutine_get_user_data(cor);
    if (!user_data) {
        return;
    }
    struct sample_ctxt *ud = (struct sample_ctxt*)user_data;

    if (event == PCCOR_EVENT_EXIT) {
        pchtml_html_document_t *doc = (pchtml_html_document_t *)data;

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
    else if (event == PCCOR_EVENT_DESTROY) {
        sample_destroy(ud);
    }
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
            purc_run(my_event_handler);
        } while (0);
        globfree(&globbuf);
    }

    std::cerr << "env: " << env << "=" << path << std::endl;
}

