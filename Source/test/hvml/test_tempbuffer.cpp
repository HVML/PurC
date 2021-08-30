#include "purc.h"

#include "tempbuffer.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

TEST(temp_buffer, new_destory)
{
    struct pchvml_temp_buffer* buffer = pchvml_temp_buffer_new (32);
    ASSERT_NE(buffer, nullptr);
    ASSERT_EQ(0, pchvml_temp_buffer_get_size_in_bytes(buffer));
    ASSERT_EQ(0, pchvml_temp_buffer_get_size_in_chars(buffer));

    pchvml_temp_buffer_destroy(buffer);
}

TEST(temp_buffer, append)
{
    struct pchvml_temp_buffer* buffer = pchvml_temp_buffer_new (32);
    ASSERT_NE(buffer, nullptr);
    ASSERT_EQ(0, pchvml_temp_buffer_get_size_in_bytes(buffer));
    ASSERT_EQ(0, pchvml_temp_buffer_get_size_in_chars(buffer));

    char c[8] = {0};
    wchar_t wc = 0;
    pchvml_temp_buffer_append (buffer, c, 1, wc);
    ASSERT_EQ(1, pchvml_temp_buffer_get_size_in_bytes(buffer));
    ASSERT_EQ(1, pchvml_temp_buffer_get_size_in_chars(buffer));
    ASSERT_EQ(wc, pchvml_temp_buffer_get_last_char(buffer));

    c[0] = 1;
    wc = 1;
    pchvml_temp_buffer_append (buffer, c, 1, wc);
    ASSERT_EQ(2, pchvml_temp_buffer_get_size_in_bytes(buffer));
    ASSERT_EQ(2, pchvml_temp_buffer_get_size_in_chars(buffer));
    ASSERT_EQ(wc, pchvml_temp_buffer_get_last_char(buffer));
    ASSERT_EQ(1, pchvml_temp_buffer_end_with(buffer, c, 1));

    c[0] = 'a';
    wc = 'a';
    char cmp[8] = {1, 'a', 0};
    pchvml_temp_buffer_append (buffer, c, 1, wc);
    ASSERT_EQ(3, pchvml_temp_buffer_get_size_in_bytes(buffer));
    ASSERT_EQ(3, pchvml_temp_buffer_get_size_in_chars(buffer));
    ASSERT_EQ(wc, pchvml_temp_buffer_get_last_char(buffer));
    ASSERT_EQ(1, pchvml_temp_buffer_end_with(buffer, cmp, 2));

    pchvml_temp_buffer_destroy(buffer);
}

TEST(temp_buffer, end_with_and_is_equal)
{
    struct pchvml_temp_buffer* buffer = pchvml_temp_buffer_new (32);
    ASSERT_NE(buffer, nullptr);
    ASSERT_EQ(0, pchvml_temp_buffer_get_size_in_bytes(buffer));
    ASSERT_EQ(0, pchvml_temp_buffer_get_size_in_chars(buffer));

    char c[8] = {0};
    wchar_t wc = 0;
    pchvml_temp_buffer_append (buffer, c, 1, wc);
    ASSERT_EQ(1, pchvml_temp_buffer_get_size_in_bytes(buffer));
    ASSERT_EQ(1, pchvml_temp_buffer_get_size_in_chars(buffer));
    ASSERT_EQ(wc, pchvml_temp_buffer_get_last_char(buffer));

    c[0] = 1;
    wc = 1;
    pchvml_temp_buffer_append (buffer, c, 1, wc);
    ASSERT_EQ(2, pchvml_temp_buffer_get_size_in_bytes(buffer));
    ASSERT_EQ(2, pchvml_temp_buffer_get_size_in_chars(buffer));
    ASSERT_EQ(wc, pchvml_temp_buffer_get_last_char(buffer));
    ASSERT_EQ(1, pchvml_temp_buffer_end_with(buffer, c, 1));

    c[0] = 'a';
    wc = 'a';
    char cmp[8] = {1, 'a', 0};
    pchvml_temp_buffer_append (buffer, c, 1, wc);
    ASSERT_EQ(3, pchvml_temp_buffer_get_size_in_bytes(buffer));
    ASSERT_EQ(3, pchvml_temp_buffer_get_size_in_chars(buffer));
    ASSERT_EQ(wc, pchvml_temp_buffer_get_last_char(buffer));
    ASSERT_EQ(1, pchvml_temp_buffer_end_with(buffer, cmp, 2));

    c[0] = 'b';
    wc = 'b';
    char p[8] = {0, 1, 'a', 'b', 0};
    pchvml_temp_buffer_append (buffer, c, 1, wc);
    ASSERT_EQ(4, pchvml_temp_buffer_get_size_in_bytes(buffer));
    ASSERT_EQ(4, pchvml_temp_buffer_get_size_in_chars(buffer));
    ASSERT_EQ(wc, pchvml_temp_buffer_get_last_char(buffer));
    ASSERT_EQ(1, pchvml_temp_buffer_end_with(buffer, p, 4));
    ASSERT_EQ(1, pchvml_temp_buffer_is_equal(buffer, p, 4));

    pchvml_temp_buffer_reset(buffer);
    ASSERT_NE(buffer, nullptr);
    ASSERT_EQ(0, pchvml_temp_buffer_get_size_in_bytes(buffer));
    ASSERT_EQ(0, pchvml_temp_buffer_get_size_in_chars(buffer));
    ASSERT_EQ(0, pchvml_temp_buffer_get_last_char(buffer));

    pchvml_temp_buffer_destroy(buffer);
}

