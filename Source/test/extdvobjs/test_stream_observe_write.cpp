#include "purc.h"

#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

bool is_file_exists(const char* file)
{
    struct stat filestat;
    return (0 == stat(file, &filestat));
}

TEST(observe, basic)
{
    const char *hvml =
    "<!DOCTYPE hvml>"
    "<hvml target=\"html\" lang=\"en\">"
    "    <head>"
    "        <update on=\"$TIMERS\" to=\"unite\">"
    "            ["
    "                { \"id\" : \"clock\", \"interval\" : 1000, \"active\" : \"yes\" }"
    "            ]"
    "        </update>"
    "    </head>"
    ""
    "    <body>"
    "        <div id=\"stream\">"
    ""
    "            <div id=\"c_title\">"
    "                <h2 id=\"c_title\">Stream observe<br/>"
    "                    <span id=\"content\">$DATETIME.fmtbdtime('%Y-%m-%dT%H:%M:%S', null)</span>"
    "                </h2>"
    "                <init as='stream_pipe' with=\"$STREAM.open('pipe:///var/tmp/test_stream_observe_write', 'write nonblock')\"/>"
    ""
    "                <observe on=\"$stream_pipe\" for=\"event:write\">"
    "                    <update on=\"#content\" at=\"textContent\" with=\"$EJSON.stringify($STREAM.writelines($stream_pipe, 'write to pipe'))\" />"
    "                    <forget on=\"$stream_pipe\" for=\"event:write\"/>"
    "                </observe>"
    ""
    ""
    "                <observe on=\"$TIMERS\" for=\"expired:clock\">"
    "                    <forget on=\"$TIMERS\" for=\"expired:clock\"/>"
    "                    <update on=\"#content\" at=\"textContent\" with=\"$EJSON.stringify($STREAM.readbytes($STREAM.open('pipe:///var/tmp/test_stream_observe_write', 'read'), 4096))\" />"
    "                </observe>"
    ""
    "                <p>this is after observe</p>"
    "            </div>"
    ""
    "        </div>"
    "    </body>"
    ""
    "</hvml>";

    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;
    const char *file = "/var/tmp/test_stream_observe_write";

    // initial purc
    ret = purc_init_ex (PURC_MODULE_HVML, "cn.fmsoft.hybridos.test",
            "test_stream_observe", &info);

    ASSERT_EQ (ret, PURC_ERROR_OK);

    purc_variant_t stream = purc_variant_load_dvobj_from_so ("STREAM", "STREAM");
    ASSERT_NE(stream, nullptr);

    if(!is_file_exists(file)) {
        ret = mkfifo(file, 0777);
        ASSERT_EQ(ret, 0);
    }

    int rdfd = open(file, O_RDONLY|O_NONBLOCK);
    ASSERT_NE(rdfd, -1);

    int fd = open(file, O_WRONLY|O_NONBLOCK);
    ASSERT_NE(fd, -1);

    int n = 0;
    while (true) {
        int w = write(fd, "a", 1);
        if (w == -1) {
            fprintf(stderr, "######write n=%d|w=%d\n", n, w);
            break;
        }
        n++;
    }
    close(fd);

    // get statitics information
    const struct purc_variant_stat * stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);

    purc_vdom_t vdom = purc_load_hvml_from_string(hvml);
    ASSERT_NE(vdom, nullptr);

    bool bind = purc_bind_document_variable(vdom, "STREAM", stream);
    ASSERT_EQ(bind, true);

    purc_run(PURC_VARIANT_INVALID, NULL);

    purc_variant_unload_dvobj(stream);
    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);

    close(rdfd);
    unlink(file);
}

