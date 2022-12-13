/*
** Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
**
** This file is a part of PurC (short for Purring Cat), an HVML interpreter.
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/


#include "purc/purc.h"
#include "private/utils.h"
#include "private/interpreter.h"

#include "../helpers.h"
#include "tools.h"

#include <glob.h>
#include <gtest/gtest.h>

struct sample_data {
    const char                   *input_hvml;
    const char                   *expected_html;
    const char                   *comp_file;
};

struct sample_ctxt {
    char               *input_hvml;
    char               *expected_html;
    char               *comp_file;
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

    if (ud->comp_file) {
        free(ud->comp_file);
        ud->comp_file = NULL;
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

static int my_cond_handler(purc_cond_k event, purc_coroutine_t cor,
        void *data)
{
    if (event == PURC_COND_COR_EXITED) {
        void *user_data = purc_coroutine_get_user_data(cor);
        if (!user_data) {
            return -1;
        }

        struct sample_ctxt *ud = (struct sample_ctxt*)user_data;

        struct purc_cor_exit_info *info = (struct purc_cor_exit_info *)data;
        purc_document_t doc = info->doc;

        if (ud->terminated) {
            ADD_FAILURE() << "internal logic error: reentrant" << std::endl;
            return -1;
        }
        ud->terminated = 1;

        if (ud->html) {
            int diff = 0;
            char *ctnt;
            ctnt = intr_util_comp_docs(doc, ud->html, &diff);
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
        else {
            char *doc_buf = intr_util_dump_doc(doc, NULL);
            FILE *fp = fopen(ud->comp_file, "w");
            fprintf(fp, "%s", doc_buf);
            fclose(fp);
            fprintf(stderr, "html written to `%s`\n", ud->comp_file);
            fprintf(stderr, "html:\n%s\n", doc_buf);
            free(doc_buf);
        }
    }
    else if (event == PURC_COND_COR_DESTROYED) {
        void *user_data = purc_coroutine_get_user_data(cor);
        if (!user_data) {
            return -1;
        }

        struct sample_ctxt *ud = (struct sample_ctxt*)user_data;
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

    ud->comp_file = strdup(sample->comp_file);
    if (!ud->comp_file) {
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
read_file(char *buf, size_t nr, const char *file, bool silently)
{
    FILE *f = fopen(file, "r");
    if (!f) {
        if (!silently) {
            ADD_FAILURE()
                << "Failed to open file [" << file << "]" << std::endl;
        }
        return -1;
    }

    size_t n = fread(buf, 1, nr, f);
    if (ferror(f)) {
        if (!silently) {
            ADD_FAILURE()
                << "Failed read file [" << file << "]" << std::endl;
        }
        fclose(f);
        return -1;
    }

    fclose(f);

    if (n == sizeof(buf)) {
        if (!silently) {
            ADD_FAILURE()
                << "Too small buffer to read file [" << file << "]" << std::endl;
        }
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
    int n = read_file(buf, sizeof(buf), file, false);
    if (n == -1)
        return -1;

    char path[PATH_MAX];
    char comp[1024*1024];
    sprintf(path, "%s.html", file);
    n = read_file(comp, sizeof(comp), path, true);

    struct sample_data sample = {
        .input_hvml = buf,
        .expected_html = n != -1 ? comp : NULL,
        .comp_file = path,
    };

    return add_sample(&sample);
}

TEST(samples, files)
{
    PurCInstance purc(false);

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
            purc_run((purc_cond_handler)my_cond_handler);
        } while (0);
        globfree(&globbuf);
    }

    std::cerr << "env: " << env << "=" << path << std::endl;
}

