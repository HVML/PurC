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
#include "private/vdom.h"
#include "private/hvml.h"
#include "hvml-token.h"
#include "hvml-gen.h"

#include <gtest/gtest.h>
#include <dirent.h>
#include <glob.h>

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
        pcvdom_document_unref(doc);
}

static int
_process_file(const char *fn)
{
    FILE *fin = NULL;
    purc_rwstream_t rin = NULL;

    bool neg = false;

    /* FIXME */
    const char *base = pcutils_basename(fn);
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
        ADD_FAILURE() << "Failed to open ["
            << fn << "]: [" << err << "]" << strerror(err) << std::endl;
        return -1;
    }

    rin = purc_rwstream_new_from_unix_fd(dup(fileno(fin)));
    if (!rin) {
        int err = errno;
        ADD_FAILURE() << "Failed to open stream for ["
            << fn << "]: [" << err << "]" << strerror(err) << std::endl;
        fclose(fin);
        return -1;
    }

    struct pcvdom_pos pos;
    struct pcvdom_document *doc = NULL;
    doc = pcvdom_util_document_from_stream(rin, &pos);
    if (!doc) {
        char buf[1024];
        snprintf(buf, sizeof(buf),
                "Parsing failed: [0x%02x]'%c' @line%d/col%d/pos%d",
                (unsigned char)pos.c, pos.c, pos.line, pos.col, pos.pos);
        std::cerr << buf << std::endl;
    }
    else {
        PRINT_VDOM_NODE(pcvdom_node_from_document(doc));
    }
    int r = 0;
    if (doc && neg) {
        r = -1;
        ADD_FAILURE() << "Unexpected successful parsing for negative sample: [" << fn << "]" << std::endl;
    }
    else if (!doc && !neg) {
        r = -1;
        ADD_FAILURE() << "Parsing positive sample: [" << fn << "]" << std::endl;
    }

    if (doc)
        pcvdom_document_unref(doc);

    if (rin)
        purc_rwstream_destroy(rin);

    if (fin)
        fclose(fin);

    return r ? -1 : 0;
}

TEST(vdom_gen, files)
{
    int r = 0;
    glob_t globbuf;
    memset(&globbuf, 0, sizeof(globbuf));

    purc_instance_extra_info info = {};
    r = purc_init_ex(PURC_MODULE_HVML, "cn.fmsoft.hybridos.test",
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
            r = _process_file(globbuf.gl_pathv[i]);
            if (r)
                break;
        }
    }
    globfree(&globbuf);

end:
    purc_cleanup ();
}

