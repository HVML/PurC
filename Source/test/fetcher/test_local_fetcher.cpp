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

#include "purc/purc-rwstream.h"
#include "purc/purc-utils.h"
#include "private/fetcher.h"
#include "config.h"

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <gtest/gtest.h>
#include <wtf/RunLoop.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if OS(LINUX) || OS(UNIX)
// get path from env or __FILE__/../<rel> otherwise
#define getpath_from_env_or_rel(_path, _len, _env, _rel) do {  \
    const char *p = getenv(_env);                                      \
    if (p) {                                                           \
        snprintf(_path, _len, "file://%s/", p);                        \
    } else {                                                           \
        char tmp[PATH_MAX+1];                                          \
        snprintf(tmp, sizeof(tmp), __FILE__);                          \
        const char *folder = dirname(tmp);                             \
        snprintf(_path, _len, "file://%s/%s/", folder, _rel);          \
    }                                                                  \
} while (0)

#endif // OS(LINUX) || OS(UNIX)

TEST(local_fetcher, sync)
{
#if 0                         /* { */
    purc_instance_extra_info info = {};
    purc_init_ex (PURC_MODULE_HVML,"cn.fmsoft.hybridos.sample",
            "pcfetcher", &info);

    const char* env = "HVML_TEST_LOCAL_FETCHER";
    char base_uri[PATH_MAX+1] =  {0};
    getpath_from_env_or_rel(base_uri, sizeof(base_uri), env, "data");

    const char* url = "buttons.json";

    pcfetcher_init(10, 1024, true);
    pcfetcher_set_base_url(base_uri);
    struct pcfetcher_resp_header resp_header;
    purc_rwstream_t resp = pcfetcher_request_sync(
        url,
        PCFETCHER_METHOD_GET,
        NULL,
        10,
        &resp_header);

    fprintf(stderr, "....................................\n");
    fprintf(stderr, "%s\n", url);
    fprintf(stderr, ".................head begin\n");
    fprintf(stderr, "ret_code=%d\n", resp_header.ret_code);
    fprintf(stderr, "mime_type=%s\n", resp_header.mime_type);
    fprintf(stderr, "sz_resp=%ld\n", resp_header.sz_resp);
    fprintf(stderr, ".................head end\n");
    fprintf(stderr, ".................body begin\n");
    if (resp) {
        purc_rwstream_t rws_out = purc_rwstream_new_buffer(1024, 1024*1024);

        size_t sz = 0;
        sz = purc_rwstream_dump_to_another (resp, rws_out, resp_header.sz_resp);
        ASSERT_EQ(sz, resp_header.sz_resp);

        char* mem_buffer = (char*)purc_rwstream_get_mem_buffer (rws_out, &sz);
        ASSERT_EQ(sz, resp_header.sz_resp);
        fprintf(stderr, "content=%s\n", mem_buffer);

        purc_rwstream_destroy(rws_out);
        purc_rwstream_destroy(resp);
        if (resp_header.mime_type) {
            free(resp_header.mime_type);
        }
    }

    fprintf(stderr, ".................body end\n");
    fprintf(stderr, "....................................\n");

    pcfetcher_term();
    purc_cleanup();
#endif                        /* } */
}

void async_response_handler(
        purc_variant_t request_id, void* ctxt,
        const struct pcfetcher_resp_header *resp_header,
        purc_rwstream_t resp)
{
    UNUSED_PARAM(request_id);
    UNUSED_PARAM(ctxt);
    UNUSED_PARAM(resp);
    fprintf(stderr, "....................................\n");
    fprintf(stderr, ".................head begin\n");
    fprintf(stderr, "ret_code=%d\n", resp_header->ret_code);
    fprintf(stderr, "mime_type=%s\n", resp_header->mime_type);
    fprintf(stderr, "sz_resp=%ld\n", resp_header->sz_resp);
    fprintf(stderr, ".................head end\n");
    fprintf(stderr, ".................body begin\n");
    if (resp) {
        purc_rwstream_t rws_out = purc_rwstream_new_buffer(1024, 1024*1024);

        size_t sz = 0;
        sz = purc_rwstream_dump_to_another (resp, rws_out, resp_header->sz_resp);
        ASSERT_EQ(sz, resp_header->sz_resp);

        char* mem_buffer = (char*)purc_rwstream_get_mem_buffer (rws_out, &sz);
        ASSERT_EQ(sz, resp_header->sz_resp);
        fprintf(stderr, "content=%s\n", mem_buffer);

        purc_rwstream_destroy(rws_out);
        purc_rwstream_destroy(resp);
    }
    fprintf(stderr, ".................body end\n");
    fprintf(stderr, "....................................request_id=%p\n", request_id);
    purc_variant_unref(request_id);
}

TEST(local_fetcher, async)
{
#if 0                         /* { */
    purc_instance_extra_info info = {};
    purc_init_ex (PURC_MODULE_HVML,"cn.fmsoft.hybridos.sample",
            "pcfetcher", &info);

    const char* env = "HVML_TEST_LOCAL_FETCHER";
    char base_uri[PATH_MAX+1] =  {0};
    getpath_from_env_or_rel(base_uri, sizeof(base_uri), env, "data");

    const char* url = "buttons.json";

    pcfetcher_init(10, 1024, true);
    pcfetcher_set_base_url(base_uri);
    pcfetcher_request_async(
                url,
                PCFETCHER_METHOD_GET,
                NULL,
                0,
                async_response_handler,
                NULL);

    fprintf(stderr, "....................................async\n");

    pcfetcher_term();
    purc_cleanup();
#endif                        /* } */
}
