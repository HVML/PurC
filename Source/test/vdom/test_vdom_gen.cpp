#include "purc.h"
#include "private/vdom.h"
#include "private/hvml.h"
#include "hvml-token.h"
#include "hvml-gen.h"

#include <gtest/gtest.h>
#include <dirent.h>
#include <glob.h>
#include <libgen.h>

#include "../helpers.h"

TEST(vdom_gen, basic)
{
    struct pcvdom_gen *gen = NULL;
    struct pcvdom_document *doc = NULL;
    gen = pcvdom_gen_create();
    if (!gen)
        goto end;

    doc = pcvdom_gen_end(gen);

end:
    if (gen)
        pcvdom_gen_destroy(gen);
    if (doc)
        pcvdom_document_destroy(doc);
}

static void
_process_file(const char *fn)
{
    FILE *fin = NULL;
    purc_rwstream_t rin = NULL;
    struct pchvml_parser *parser = NULL;
    struct pcvdom_gen *gen = NULL;
    struct pcvdom_document *doc = NULL;
    struct pchvml_token *token = NULL;
    bool neg = false;

    /* FIXME */
    char *base = basename((char *)fn);
    if (strstr(base, "neg.")==base) {
        neg = true;
    }

    if (neg) {
        std::cerr << "Start parsing neg sample: [" << fn << "]" << std::endl;
    } else {
        std::cerr << "Start parsing: [" << fn << "]" << std::endl;
    }

    fin = fopen(fn, "r");
    if (!fin) {
        int err = errno;
        EXPECT_NE(fin, nullptr) << "Failed to open ["
            << fn << "]: [" << err << "]" << strerror(err) << std::endl;
        goto end;
    }

    rin = purc_rwstream_new_from_unix_fd(dup(fileno(fin)), 1024);
    if (!rin) {
        EXPECT_NE(rin, nullptr);
        goto end;
    }

    parser = pchvml_create(0, 0);
    if (!parser)
        goto end;

    gen = pcvdom_gen_create();
    if (!gen)
        goto end;

again:
    if (token)
        pchvml_token_destroy(token);

    token = pchvml_next_token(parser, rin);

    if (token && 0==pcvdom_gen_push_token(gen, parser, token)) {
        if (pchvml_token_is_type(token, PCHVML_TOKEN_EOF)) {
            doc = pcvdom_gen_end(gen);
            if (neg) {
                EXPECT_TRUE(false) << "Unexpected successful in parsing neg sample: [" << fn << "]" << std::endl;
            } else {
                std::cerr << "Succeeded in parsing: [" << fn << "]" << std::endl;
            }
            goto end;
        }
        goto again;
    }

    if (!neg) {
        EXPECT_NE(token, nullptr) << "unexpected NULL token: ["
            << token << "]" << std::endl;
    }

    if (neg) {
        std::cerr << "Succeeded in failure-parsing neg sample: [" << fn << "]" << std::endl;
    } else {
        EXPECT_TRUE(false) << "Failed parsing: [" << fn << "]" << std::endl;
    }

end:
    if (token)
        pchvml_token_destroy(token);

    if (doc)
        pcvdom_document_destroy(doc);

    if (gen)
        pcvdom_gen_destroy(gen);

    if (parser)
        pchvml_destroy(parser);

    if (rin)
        purc_rwstream_destroy(rin);

    if (fin)
        fclose(fin);
}

TEST(vdom_gen, files)
{
    int r = 0;
    glob_t globbuf;
    memset(&globbuf, 0, sizeof(globbuf));

    purc_instance_extra_info info = {};
    r = purc_init("cn.fmsoft.hybridos.test",
        "vdom_gen", &info);
    EXPECT_EQ(r, PURC_ERROR_OK);
    if (r)
        return;

    char path[PATH_MAX+1];
    const char *env;
    env = "SOURCE_FILES";
    test_getpath_from_env_or_rel(path, sizeof(path),
        env, "/data/*.hvml");
    std::cerr << "env: " << env << "=" << path << std::endl;

    if (!path[0])
        goto end;

    globbuf.gl_offs = 0;
    r = glob(path, GLOB_DOOFFS | GLOB_APPEND, NULL, &globbuf);
    EXPECT_EQ(r, 0) << "Failed to globbing @["
            << path << "]: [" << errno << "]" << strerror(errno)
            << std::endl;

    if (r == 0) {
        for (size_t i=0; i<globbuf.gl_pathc; ++i) {
            _process_file(globbuf.gl_pathv[i]);
        }
    }
    globfree(&globbuf);

end:
    purc_cleanup ();
}

