#include "rwstream.h"

#include <stdio.h>
#include <gtest/gtest.h>


void create_temp_file(const char* file, const char* buf, size_t buf_len)
{
    FILE* fp = fopen(file, "wb");
    fwrite(buf, buf_len, 1, fp);
    fflush(fp);
    fclose(fp);
}

void remove_temp_file(const char* file)
{
    remove(file);
}

TEST(stdio_rwstream, new_destroy) {
    char tmp_file[] = "/tmp/stdio.txt";
    char buf[] = "This is test file. 这是测试文件。";
    create_temp_file(tmp_file, buf, strlen(buf));

    purc_rwstream_t rws = purc_rwstream_new_from_file(tmp_file, "r");
    ASSERT_NE(rws, nullptr);

    int ret = purc_rwstream_close(rws);
    ASSERT_EQ(ret, 0);

    ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);

    remove_temp_file(tmp_file);
}
