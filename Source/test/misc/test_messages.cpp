#include "purc.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#define ATOM_BITS_NR        (sizeof(purc_atom_t) << 3)
#define BUCKET_BITS(bucket)       \
    ((purc_atom_t)bucket << (ATOM_BITS_NR - PURC_ATOM_BUCKET_BITS))

static char buffer_a[4096];
static char buffer_b[4096];

struct buff_info {
    char *  buf;
    size_t  size;
    off_t   pos;
};

static ssize_t write_to_buf (void *ctxt, const void *buf, size_t count)
{
    struct buff_info *info = (struct buff_info *)ctxt;

    if (info->pos + count <= info->size) {
        memcpy (info->buf + info->pos, buf, count);
        info->pos += count;
        return count;
    }
    else {
        ssize_t n = info->size - info->pos;

        if (n > 0) {
            memcpy (info->buf + info->pos, buf, n);
            info->pos += n;
            return n;
        }

        return 0;
    }

    return -1;
}

TEST(instance, messages)
{
    int ret = purc_init_ex(PURC_MODULE_VARIANT, NULL, NULL, NULL);
    ASSERT_EQ(ret, PURC_ERROR_OK);

    pcrdr_msg *msg;
    msg = pcrdr_make_request_message(PCRDR_MSG_TARGET_SESSION,
            random(), "to_do_something", NULL, "request-id",
            PCRDR_MSG_ELEMENT_TYPE_VOID, NULL, NULL,
            PCRDR_MSG_DATA_TYPE_TEXT, "The data", 0);

    pcrdr_msg *msg_parsed;
    struct buff_info info_a = { buffer_a, sizeof (buffer_a), 0 };
    struct buff_info info_b = { buffer_b, sizeof (buffer_b), 0 };

    pcrdr_serialize_message(msg, write_to_buf, &info_a);
    buffer_a[info_a.pos] = '\0';

    ret = pcrdr_parse_packet(buffer_a, info_a.pos, &msg_parsed);
    ASSERT_EQ(ret, 0);

    pcrdr_serialize_message(msg_parsed, write_to_buf, &info_b);
    buffer_b[info_b.pos] = '\0';

    ret = pcrdr_compare_messages(msg, msg_parsed);
    ASSERT_EQ(ret, 0);

    pcrdr_release_message(msg_parsed);
    pcrdr_release_message(msg);

    purc_cleanup();
}

