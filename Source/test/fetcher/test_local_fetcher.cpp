#include "purc.h"

#include "purc-rwstream.h"
#include "purc-utils.h"
#include "private/fetcher.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
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
    purc_instance_extra_info info = {};
    info.enable_remote_fetcher = false;
    purc_init ("cn.fmsoft.hybridos.sample", "pcfetcher", &info);

    RunLoop::initializeMain();
    AtomString::init();
    WTF::RefCountedBase::enableThreadingChecksGlobally();

    const char* env = "HVML_TEST_LOCAL_FETCHER";
    char base_uri[PATH_MAX+1] =  {0};
    getpath_from_env_or_rel(base_uri, sizeof(base_uri), env, "data");

    const char* url = "buttons.json";

    pcfetcher_init(10, 1024, true);
    pcfetcher_set_base_url(base_uri);
    struct pcfetcher_resp_header resp_header;
    purc_rwstream_t resp = pcfetcher_request_sync(
        url,
        PCFETCHER_REQUEST_METHOD_GET,
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
}

