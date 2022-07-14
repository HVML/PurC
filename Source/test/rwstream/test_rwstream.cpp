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

#include "purc.h"

#include "purc-rwstream.h"
#include "purc-utils.h"
#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


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

/* test stdio rwstream */
TEST(stdio_rwstream, new_destroy)
{
    char tmp_file[] = "/tmp/rwstream.txt";
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);
    create_temp_file(tmp_file, buf, buf_len);

    purc_rwstream_t rws = purc_rwstream_new_from_file(tmp_file, "r");
    ASSERT_NE(rws, nullptr);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);

    remove_temp_file(tmp_file);
}

TEST(stdio_rwstream, read_char)
{
    char tmp_file[] = "/tmp/rwstream.txt";
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);
    create_temp_file(tmp_file, buf, buf_len);

    purc_rwstream_t rws = purc_rwstream_new_from_file(tmp_file, "r");
    ASSERT_NE(rws, nullptr);

    char read_buf[1024] = {0};
    int read_len = purc_rwstream_read (rws, read_buf, buf_len);
    ASSERT_EQ(read_len, buf_len);

    ASSERT_STREQ(read_buf, buf);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);

    remove_temp_file(tmp_file);
}

TEST(stdio_rwstream, write_char)
{
    char tmp_file[] = "/tmp/rwstream.txt";
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);

    purc_rwstream_t rws = purc_rwstream_new_from_file(tmp_file, "w");
    ASSERT_NE(rws, nullptr);

    int write_len = purc_rwstream_write (rws, buf, buf_len);
    ASSERT_EQ(write_len, buf_len);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);


    FILE* fp = fopen(tmp_file, "r");
    char read_buf[1024] = {0};
    size_t rdlen = fread(read_buf, buf_len, 1, fp);
    fclose(fp);

    ASSERT_EQ(1, rdlen);
    ASSERT_STREQ(buf, read_buf);

    remove_temp_file(tmp_file);
}

TEST(stdio_rwstream, read_utf8_char)
{
    char tmp_file[] = "/tmp/rwstream.txt";
    char buf[] = "This这 is 测。";
    size_t buf_len = strlen(buf);
    create_temp_file(tmp_file, buf, buf_len);

    purc_rwstream_t rws = purc_rwstream_new_from_file(tmp_file, "r");
    ASSERT_NE(rws, nullptr);

    char read_buf[100] = {0};
    uint32_t wc = 0;
    int read_len = 0;

    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "T");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "h");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "i");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "s");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x8FD9);
    ASSERT_STREQ(read_buf, "这");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, ' ');
    ASSERT_STREQ(read_buf, " ");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, 'i');
    ASSERT_STREQ(read_buf, "i");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, 's');
    ASSERT_STREQ(read_buf, "s");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, ' ');
    ASSERT_STREQ(read_buf, " ");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x6D4B);
    ASSERT_STREQ(read_buf, "测");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x3002);
    ASSERT_STREQ(read_buf, "。");

    purc_rwstream_seek (rws, 0, SEEK_SET);
    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, 'T');
    ASSERT_STREQ(read_buf, "T");

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);

    remove_temp_file(tmp_file);
}


TEST(stdio_rwstream, seek_tell)
{
    char tmp_file[] = "/tmp/rwstream.txt";
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);
    create_temp_file(tmp_file, buf, buf_len);

    purc_rwstream_t rws = purc_rwstream_new_from_file(tmp_file, "rb");
    ASSERT_NE(rws, nullptr);

    off_t pos = purc_rwstream_seek (rws, 1, SEEK_SET);
    ASSERT_EQ(pos, 1);

    pos = purc_rwstream_seek (rws, 10, SEEK_CUR);
    ASSERT_EQ(pos, 11);

    off_t tpos = purc_rwstream_tell (rws);
    ASSERT_EQ(pos, tpos);

    pos = purc_rwstream_seek (rws, -1, SEEK_END);
    tpos = purc_rwstream_tell (rws);
    ASSERT_EQ(pos, tpos);

    pos = purc_rwstream_seek (rws, 0, SEEK_END);
    tpos = purc_rwstream_tell (rws);
    ASSERT_EQ(pos, tpos);

    char read_buf[10] = {0};
    int read_len = purc_rwstream_read (rws, read_buf, 1);
    ASSERT_EQ(read_len, 0);

    pos = purc_rwstream_seek (rws, 10, SEEK_END);
    tpos = purc_rwstream_tell (rws);
    ASSERT_EQ(pos, tpos);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);

    remove_temp_file(tmp_file);
}

TEST(stdio_rwstream, seek_read)
{
    char tmp_file[] = "/tmp/rwstream.txt";
    char buf[] = "This这 is 测。";
    size_t buf_len = strlen(buf);
    create_temp_file(tmp_file, buf, buf_len);

    purc_rwstream_t rws = purc_rwstream_new_from_file(tmp_file, "r");
    ASSERT_NE(rws, nullptr);

    char read_buf[100] = {0};
    uint32_t wc = 0;
    int read_len = 0;

    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "T");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "h");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "i");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "s");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x8FD9);
    ASSERT_STREQ(read_buf, "这");

    off_t pos = purc_rwstream_seek (rws, 0, SEEK_SET);
    ASSERT_EQ(pos, 0);

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, 'T');
    ASSERT_STREQ(read_buf, "T");

    pos = purc_rwstream_seek (rws, 4, SEEK_SET);
    ASSERT_EQ(pos, 4);

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x8FD9);
    ASSERT_STREQ(read_buf, "这");

    pos = purc_rwstream_seek (rws, 5, SEEK_SET);
    ASSERT_EQ(pos, 5);

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, -1);

    purc_rwstream_seek (rws, 0, SEEK_END);
    memset(read_buf, 0, sizeof(read_buf));
    wc = 0;
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 0);
    ASSERT_EQ(wc, 0);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);

    remove_temp_file(tmp_file);
}

/* test mem rwstream */
TEST(mem_rwstream, new_destroy)
{
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);

    purc_rwstream_t rws = purc_rwstream_new_from_mem (buf, buf_len);
    ASSERT_NE(rws, nullptr);

    size_t sz = 0;
    char* mem_buffer = (char*)purc_rwstream_get_mem_buffer (rws, &sz);
    ASSERT_EQ(mem_buffer, buf);
    ASSERT_EQ(sz, buf_len);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);
}

TEST(mem_rwstream, read_char)
{
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);

    purc_rwstream_t rws = purc_rwstream_new_from_mem (buf, buf_len);
    ASSERT_NE(rws, nullptr);

    char read_buf[1024] = {0};
    int read_len = purc_rwstream_read (rws, read_buf, buf_len);
    ASSERT_EQ(read_len, buf_len);

    ASSERT_STREQ(read_buf, buf);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);
}

TEST(mem_rwstream, write_char)
{
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);

    char write_buf[1024] = {0};
    size_t write_buf_len = buf_len + 5;
    purc_rwstream_t rws = purc_rwstream_new_from_mem (write_buf, write_buf_len);
    ASSERT_NE(rws, nullptr);

    int write_len = purc_rwstream_write (rws, buf, buf_len);
    ASSERT_EQ(write_len, buf_len);

    ASSERT_STREQ(write_buf, buf);

    write_len = purc_rwstream_write (rws, buf, buf_len);
    ASSERT_EQ(write_len, 5);

    write_len = purc_rwstream_write (rws, buf, buf_len);
    ASSERT_EQ(write_len, -1);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);
}

TEST(mem_rwstream, read_utf8_char)
{
    char buf[] = "This这 is 测。";
    size_t buf_len = strlen(buf);

    purc_rwstream_t rws = purc_rwstream_new_from_mem (buf, buf_len);
    ASSERT_NE(rws, nullptr);

    char read_buf[100] = {0};
    uint32_t wc = 0;
    int read_len = 0;

    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "T");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "h");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "i");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "s");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x8FD9);
    ASSERT_STREQ(read_buf, "这");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, ' ');
    ASSERT_STREQ(read_buf, " ");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, 'i');
    ASSERT_STREQ(read_buf, "i");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, 's');
    ASSERT_STREQ(read_buf, "s");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, ' ');
    ASSERT_STREQ(read_buf, " ");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x6D4B);
    ASSERT_STREQ(read_buf, "测");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x3002);
    ASSERT_STREQ(read_buf, "。");

    purc_rwstream_seek (rws, 0, SEEK_SET);
    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, 'T');
    ASSERT_STREQ(read_buf, "T");

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);
}


TEST(mem_rwstream, seek_tell)
{
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);

    purc_rwstream_t rws = purc_rwstream_new_from_mem (buf, buf_len);
    ASSERT_NE(rws, nullptr);

    off_t pos = purc_rwstream_seek (rws, 1, SEEK_SET);
    ASSERT_EQ(pos, 1);

    pos = purc_rwstream_seek (rws, 10, SEEK_CUR);
    ASSERT_EQ(pos, 11);

    off_t tpos = purc_rwstream_tell (rws);
    ASSERT_EQ(pos, tpos);

    pos = purc_rwstream_seek (rws, -1, SEEK_END);
    tpos = purc_rwstream_tell (rws);
    ASSERT_EQ(pos, tpos);

    pos = purc_rwstream_seek (rws, 0, SEEK_END);
    tpos = purc_rwstream_tell (rws);
    ASSERT_EQ(pos, tpos);

    pos = purc_rwstream_seek (rws, 10, SEEK_END);
    tpos = purc_rwstream_tell (rws);
    ASSERT_EQ(pos, tpos);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);

}

TEST(mem_rwstream, seek_read)
{
    char buf[] = "This这 is 测。";
    size_t buf_len = strlen(buf);

    purc_rwstream_t rws = purc_rwstream_new_from_mem (buf, buf_len);
    ASSERT_NE(rws, nullptr);

    char read_buf[100] = {0};
    uint32_t wc = 0;
    int read_len = 0;

    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "T");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "h");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "i");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "s");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x8FD9);
    ASSERT_STREQ(read_buf, "这");

    off_t pos = purc_rwstream_seek (rws, 0, SEEK_SET);
    ASSERT_EQ(pos, 0);

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, 'T');
    ASSERT_STREQ(read_buf, "T");

    pos = purc_rwstream_seek (rws, 4, SEEK_SET);
    ASSERT_EQ(pos, 4);

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x8FD9);
    ASSERT_STREQ(read_buf, "这");

    pos = purc_rwstream_seek (rws, 5, SEEK_SET);
    ASSERT_EQ(pos, 5);

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, -1);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);
}

/* test buffer rwstream */
TEST(buffer_rwstream, new_destroy)
{
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);

    purc_rwstream_t rws = purc_rwstream_new_buffer (buf_len, buf_len * 2);
    ASSERT_NE(rws, nullptr);

    size_t sz = 0;
    char* mem_buffer = (char*)purc_rwstream_get_mem_buffer (rws, &sz);
    ASSERT_NE(mem_buffer, nullptr);

    ASSERT_EQ(sz, 0);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);
}

TEST(buffer_rwstream, read_char)
{
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);

    purc_rwstream_t rws = purc_rwstream_new_buffer (buf_len, buf_len * 2);
    ASSERT_NE(rws, nullptr);

    int write_len = purc_rwstream_write (rws, buf, buf_len);
    ASSERT_EQ(write_len, buf_len);

    size_t sz = 0;
    char* mem_buffer = (char*)purc_rwstream_get_mem_buffer (rws, &sz);
    ASSERT_NE(mem_buffer, nullptr);

    purc_rwstream_seek (rws, 0, SEEK_SET);

    char read_buf[1024] = {0};
    int read_len = purc_rwstream_read (rws, read_buf, buf_len);
    ASSERT_EQ(read_len, buf_len);

    ASSERT_STREQ(read_buf, buf);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);
}

TEST(buffer_rwstream, write_char)
{
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);

    purc_rwstream_t rws = purc_rwstream_new_buffer (buf_len, buf_len * 2);
    ASSERT_NE(rws, nullptr);

    size_t sz = 0;
    char* mem_buffer = (char*)purc_rwstream_get_mem_buffer (rws, &sz);
    ASSERT_NE(mem_buffer, nullptr);

    int write_len = purc_rwstream_write (rws, buf, buf_len);
    ASSERT_EQ(write_len, buf_len);

    ASSERT_STREQ(mem_buffer, buf);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);
}

TEST(buffer_rwstream, extend_memory)
{
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);

    purc_rwstream_t rws = purc_rwstream_new_buffer (buf_len, buf_len * 2 + 5);
    ASSERT_NE(rws, nullptr);

    size_t sz = 0;
    char* mem_buffer = (char*)purc_rwstream_get_mem_buffer (rws, &sz);
    ASSERT_NE(mem_buffer, nullptr);
    ASSERT_EQ(sz, 0);

    int write_len = purc_rwstream_write (rws, buf, buf_len);
    ASSERT_EQ(write_len, buf_len);
    ASSERT_STREQ(mem_buffer, buf);

    mem_buffer = (char*)purc_rwstream_get_mem_buffer (rws, &sz);
    ASSERT_EQ(sz, write_len);

    write_len = purc_rwstream_write (rws, buf, buf_len);
    ASSERT_EQ(write_len, buf_len);

    size_t sz2 = 0;
    char* mem_buffer2 = (char*)purc_rwstream_get_mem_buffer (rws, &sz2);
    ASSERT_NE(mem_buffer2, nullptr);
    ASSERT_GE(sz2, sz);
    ASSERT_EQ(sz2, sz + write_len);

    write_len = purc_rwstream_write (rws, buf, buf_len);
    ASSERT_EQ(write_len, 5);

    mem_buffer = (char*)purc_rwstream_get_mem_buffer (rws, &sz);
    ASSERT_EQ(sz, buf_len * 2 + 5);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);
}


TEST(buffer_rwstream, read_utf8_char)
{
    char buf[] = "This这 is 测。";
    size_t buf_len = strlen(buf);

    purc_rwstream_t rws = purc_rwstream_new_buffer (buf_len, buf_len * 2);
    ASSERT_NE(rws, nullptr);

    size_t sz = 0;
    char* mem_buffer = (char*)purc_rwstream_get_mem_buffer (rws, &sz);
    ASSERT_NE(mem_buffer, nullptr);

    int write_len = purc_rwstream_write (rws, buf, buf_len);
    ASSERT_EQ(write_len, buf_len);
    ASSERT_STREQ(mem_buffer, buf);

    purc_rwstream_seek (rws, 0, SEEK_SET);

    char read_buf[100] = {0};
    uint32_t wc = 0;
    int read_len = 0;

    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "T");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "h");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "i");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "s");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x8FD9);
    ASSERT_STREQ(read_buf, "这");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, ' ');
    ASSERT_STREQ(read_buf, " ");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, 'i');
    ASSERT_STREQ(read_buf, "i");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, 's');
    ASSERT_STREQ(read_buf, "s");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, ' ');
    ASSERT_STREQ(read_buf, " ");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x6D4B);
    ASSERT_STREQ(read_buf, "测");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x3002);
    ASSERT_STREQ(read_buf, "。");

    purc_rwstream_seek (rws, 0, SEEK_SET);
    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, 'T');
    ASSERT_STREQ(read_buf, "T");

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);
}


TEST(buffer_rwstream, seek_tell)
{
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);

    purc_rwstream_t rws = purc_rwstream_new_buffer (buf_len, buf_len * 2);
    ASSERT_NE(rws, nullptr);

    size_t sz = 0;
    char* mem_buffer = (char*)purc_rwstream_get_mem_buffer (rws, &sz);
    ASSERT_NE(mem_buffer, nullptr);

    int write_len = purc_rwstream_write (rws, buf, buf_len);
    ASSERT_EQ(write_len, buf_len);
    ASSERT_STREQ(mem_buffer, buf);

    off_t pos = purc_rwstream_seek (rws, 1, SEEK_SET);
    ASSERT_EQ(pos, 1);

    pos = purc_rwstream_seek (rws, 10, SEEK_CUR);
    ASSERT_EQ(pos, 11);

    off_t tpos = purc_rwstream_tell (rws);
    ASSERT_EQ(pos, tpos);

    pos = purc_rwstream_seek (rws, -1, SEEK_END);
    tpos = purc_rwstream_tell (rws);
    ASSERT_EQ(pos, tpos);

    pos = purc_rwstream_seek (rws, 0, SEEK_END);
    tpos = purc_rwstream_tell (rws);
    ASSERT_EQ(pos, tpos);

    pos = purc_rwstream_seek (rws, 10, SEEK_END);
    tpos = purc_rwstream_tell (rws);
    ASSERT_EQ(pos, tpos);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);
}

TEST(buffer_rwstream, seek_read)
{
    char buf[] = "This这 is 测。";
    size_t buf_len = strlen(buf);

    purc_rwstream_t rws = purc_rwstream_new_buffer (buf_len, buf_len * 2);
    ASSERT_NE(rws, nullptr);

    size_t sz = 0;
    char* mem_buffer = (char*)purc_rwstream_get_mem_buffer (rws, &sz);
    ASSERT_NE(mem_buffer, nullptr);

    int write_len = purc_rwstream_write (rws, buf, buf_len);
    ASSERT_EQ(write_len, buf_len);
    ASSERT_STREQ(mem_buffer, buf);

    off_t pos = purc_rwstream_seek (rws, 0, SEEK_SET);
    ASSERT_EQ(pos, 0);

    char read_buf[100] = {0};
    uint32_t wc = 0;
    int read_len = 0;

    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "T");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "h");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "i");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "s");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x8FD9);
    ASSERT_STREQ(read_buf, "这");

    pos = purc_rwstream_seek (rws, 0, SEEK_SET);
    ASSERT_EQ(pos, 0);

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, 'T');
    ASSERT_STREQ(read_buf, "T");

    pos = purc_rwstream_seek (rws, 4, SEEK_SET);
    ASSERT_EQ(pos, 4);

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x8FD9);
    ASSERT_STREQ(read_buf, "这");

    pos = purc_rwstream_seek (rws, 5, SEEK_SET);
    ASSERT_EQ(pos, 5);

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, -1);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);
}

#if HAVE(GLIB)
/* test gio fd rwstream */
TEST(gio_rwstream, new_destroy)
{
    char tmp_file[] = "/tmp/rwstream.txt";
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);
    create_temp_file(tmp_file, buf, buf_len);

    int fd = open(tmp_file, O_RDWR);
    purc_rwstream_t rws = purc_rwstream_new_from_unix_fd (fd);
    ASSERT_NE(rws, nullptr);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);

    fd = open(tmp_file, O_RDWR | O_NONBLOCK);
    rws = purc_rwstream_new_from_unix_fd (fd);
    ASSERT_NE(rws, nullptr);

    int flags = fcntl (fd, F_GETFL);
    ASSERT_EQ(flags & O_NONBLOCK, O_NONBLOCK);

    ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);
    remove_temp_file(tmp_file);
}

TEST(gio_rwstream, read_char)
{
    char tmp_file[] = "/tmp/rwstream.txt";
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);
    create_temp_file(tmp_file, buf, buf_len);

    int fd = open(tmp_file, O_RDWR, S_IRGRP | S_IWGRP | S_IRUSR
            | S_IWUSR | S_IROTH);

    purc_rwstream_t rws = purc_rwstream_new_from_unix_fd (fd);
    ASSERT_NE(rws, nullptr);

    char read_buf[1024] = {0};
    int read_len = purc_rwstream_read (rws, read_buf, buf_len);
    ASSERT_EQ(read_len, buf_len);

    ASSERT_STREQ(read_buf, buf);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);

    remove_temp_file(tmp_file);
}

TEST(gio_rwstream, write_char)
{

    char tmp_file[] = "/tmp/rwstream.txt";
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);

    int fd = open(tmp_file, O_RDWR | O_CREAT, S_IRGRP | S_IWGRP | S_IRUSR
            | S_IWUSR | S_IROTH);

    purc_rwstream_t rws = purc_rwstream_new_from_unix_fd (fd);
    ASSERT_NE(rws, nullptr);

    int write_len = purc_rwstream_write (rws, buf, buf_len);
    ASSERT_EQ(write_len, buf_len);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);


    FILE* fp = fopen(tmp_file, "r");
    char read_buf[1024] = {0};
    size_t rdlen = fread(read_buf, buf_len, 1, fp);
    fclose(fp);

    ASSERT_EQ(1, rdlen);
    ASSERT_STREQ(buf, read_buf);

    remove_temp_file(tmp_file);
}

TEST(gio_rwstream, read_utf8_char)
{
    char tmp_file[] = "/tmp/rwstream.txt";
    char buf[] = "This这 is 测。";
    size_t buf_len = strlen(buf);
    create_temp_file(tmp_file, buf, buf_len);

    int fd = open(tmp_file, O_RDWR, S_IRGRP | S_IWGRP | S_IRUSR
            | S_IWUSR | S_IROTH);

    purc_rwstream_t rws = purc_rwstream_new_from_unix_fd (fd);
    ASSERT_NE(rws, nullptr);

    char read_buf[100] = {0};
    uint32_t wc = 0;
    int read_len = 0;

    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "T");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "h");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "i");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "s");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x8FD9);
    ASSERT_STREQ(read_buf, "这");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, ' ');
    ASSERT_STREQ(read_buf, " ");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, 'i');
    ASSERT_STREQ(read_buf, "i");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, 's');
    ASSERT_STREQ(read_buf, "s");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, ' ');
    ASSERT_STREQ(read_buf, " ");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x6D4B);
    ASSERT_STREQ(read_buf, "测");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x3002);
    ASSERT_STREQ(read_buf, "。");

    purc_rwstream_seek (rws, 0, SEEK_SET);
    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, 'T');
    ASSERT_STREQ(read_buf, "T");

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);

    remove_temp_file(tmp_file);
}

TEST(gio_rwstream, seek_tell)
{
    char tmp_file[] = "/tmp/rwstream.txt";
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);
    create_temp_file(tmp_file, buf, buf_len);

    int fd = open(tmp_file, O_RDWR, S_IRGRP | S_IWGRP | S_IRUSR
            | S_IWUSR | S_IROTH);

    purc_rwstream_t rws = purc_rwstream_new_from_unix_fd (fd);
    ASSERT_NE(rws, nullptr);

    off_t pos = purc_rwstream_seek (rws, 1, SEEK_SET);
    ASSERT_EQ(pos, 1);

    pos = purc_rwstream_seek (rws, 10, SEEK_CUR);
    ASSERT_EQ(pos, 11);

    off_t tpos = purc_rwstream_tell (rws);
    ASSERT_EQ(tpos, 11);

    char read_buf[10] = {0};
    int read_len = purc_rwstream_read (rws, read_buf, 1);
    ASSERT_EQ(read_len, 1);

    pos = purc_rwstream_seek (rws, 10, SEEK_END);
    tpos = purc_rwstream_tell (rws);
    ASSERT_EQ(pos, tpos);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);

    remove_temp_file(tmp_file);
}

TEST(gio_rwstream, seek_read)
{
    char tmp_file[] = "/tmp/rwstream.txt";
    char buf[] = "This这 is 测。";
    size_t buf_len = strlen(buf);
    create_temp_file(tmp_file, buf, buf_len);

    int fd = open(tmp_file, O_RDWR, S_IRGRP | S_IWGRP | S_IRUSR
            | S_IWUSR | S_IROTH);

    purc_rwstream_t rws = purc_rwstream_new_from_unix_fd (fd);

    char read_buf[100] = {0};
    uint32_t wc = 0;
    int read_len = 0;

    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "T");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "h");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "i");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_STREQ(read_buf, "s");

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x8FD9);
    ASSERT_STREQ(read_buf, "这");

    off_t pos = purc_rwstream_seek (rws, 0, SEEK_SET);
    ASSERT_EQ(pos, 0);

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 1);
    ASSERT_EQ(wc, 'T');
    ASSERT_STREQ(read_buf, "T");

    pos = purc_rwstream_seek (rws, 4, SEEK_SET);
    ASSERT_EQ(pos, 4);

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, 3);
    ASSERT_EQ(wc, 0x8FD9);
    ASSERT_STREQ(read_buf, "这");

    pos = purc_rwstream_seek (rws, 5, SEEK_SET);
    ASSERT_EQ(pos, 5);

    memset(read_buf, 0, sizeof(read_buf));
    read_len = purc_rwstream_read_utf8_char (rws, read_buf, &wc);
    ASSERT_EQ(read_len, -1);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);

    remove_temp_file(tmp_file);
}
#endif


off_t filesize(const char* filename)
{
    struct stat statbuf;
    stat(filename,&statbuf);
    return statbuf.st_size;
}

TEST(dump_rwstream, stdio)
{
    char in_file[] = "/bin/ls";
    char out_file[] = "/tmp/ls2";

    off_t in_size = filesize(in_file);

    purc_rwstream_t rws = purc_rwstream_new_from_file(in_file, "r");
    ASSERT_NE(rws, nullptr);

    purc_rwstream_t rws_out = purc_rwstream_new_from_file(out_file, "w");
    ASSERT_NE(rws_out, nullptr);

    size_t sz = 0;
    sz = purc_rwstream_dump_to_another (rws, rws_out, 5);
    ASSERT_EQ(sz, 5);

    purc_rwstream_seek (rws, 0, SEEK_SET);
    purc_rwstream_seek (rws_out, 0, SEEK_SET);
    sz = purc_rwstream_dump_to_another (rws, rws_out, -1);
    ASSERT_EQ(sz, in_size);

    int ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);

    ret = purc_rwstream_destroy (rws_out);
    ASSERT_EQ(ret, 0);

    remove_temp_file(out_file);
}

TEST(dump_rwstream, stdio_mem)
{
    char in_file[] = "/tmp/rwstream.txt";
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);
    create_temp_file(in_file, buf, buf_len);

    purc_rwstream_t rws = purc_rwstream_new_from_file(in_file, "r");
    ASSERT_NE(rws, nullptr);

    char out_buf[1024] = {0};
    size_t out_buf_len = 1024;
    purc_rwstream_t rws_out = purc_rwstream_new_from_mem (out_buf, out_buf_len);
    ASSERT_NE(rws_out, nullptr);

    size_t sz = 0;
    sz = purc_rwstream_dump_to_another (rws, rws_out, 5);
    ASSERT_EQ(sz, 5);

    char* mem_buffer = (char*)purc_rwstream_get_mem_buffer (rws_out, &sz);
    ASSERT_EQ(0, strncmp(buf, mem_buffer, 5));

    purc_rwstream_seek (rws, 0, SEEK_SET);
    purc_rwstream_seek (rws_out, 0, SEEK_SET);

    sz = purc_rwstream_dump_to_another (rws, rws_out, -1);
    ASSERT_EQ(sz, buf_len);
    ASSERT_STREQ(mem_buffer, buf);

    int ret = purc_rwstream_destroy (rws_out);
    ASSERT_EQ(ret, 0);

    ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);

    remove_temp_file(in_file);
}

TEST(dump_rwstream, stdio_buffer)
{
    char in_file[] = "/tmp/rwstream.txt";
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);
    create_temp_file(in_file, buf, buf_len);

    purc_rwstream_t rws = purc_rwstream_new_from_file(in_file, "r");
    ASSERT_NE(rws, nullptr);

    purc_rwstream_t rws_out = purc_rwstream_new_buffer (buf_len, buf_len*2);
    ASSERT_NE(rws_out, nullptr);

    size_t sz = 0;
    sz = purc_rwstream_dump_to_another (rws, rws_out, 5);
    ASSERT_EQ(sz, 5);

    char* mem_buffer = (char*)purc_rwstream_get_mem_buffer (rws_out, &sz);
    ASSERT_EQ(0, strncmp(buf, mem_buffer, 5));

    purc_rwstream_seek (rws, 0, SEEK_SET);
    purc_rwstream_seek (rws_out, 0, SEEK_SET);

    sz = purc_rwstream_dump_to_another (rws, rws_out, -1);
    ASSERT_EQ(sz, buf_len);
    ASSERT_STREQ(mem_buffer, buf);
    ASSERT_EQ(sz, purc_rwstream_tell(rws_out));

    int ret = purc_rwstream_destroy (rws_out);
    ASSERT_EQ(ret, 0);

    ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);

    remove_temp_file(in_file);
}

#if HAVE(GLIB)
TEST(dump_rwstream, stdio_gio)
{
    char in_file[] = "/tmp/rwstream.txt";
    char out_file[] = "/tmp/rwstream2.txt";
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);
    create_temp_file(in_file, buf, buf_len);

    purc_rwstream_t rws = purc_rwstream_new_from_file(in_file, "r");
    ASSERT_NE(rws, nullptr);

    int fd = open(out_file, O_WRONLY | O_APPEND | O_CREAT, 0644);
    purc_rwstream_t rws_out = purc_rwstream_new_from_unix_fd (fd);
    ASSERT_NE(rws_out, nullptr);

    size_t sz = 0;
    sz = purc_rwstream_dump_to_another (rws, rws_out, 5);
    ASSERT_EQ(sz, 5);

    purc_rwstream_seek (rws, 0, SEEK_SET);
    purc_rwstream_seek (rws_out, 0, SEEK_SET);

    sz = purc_rwstream_dump_to_another (rws, rws_out, -1);
    ASSERT_EQ(sz, buf_len);

    int ret = purc_rwstream_destroy (rws_out);
    ASSERT_EQ(ret, 0);

    ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);

    remove_temp_file(in_file);
    remove_temp_file(out_file);
}
#endif

TEST(dump_rwstream, mem_buffer)
{
    char buf[] = "This is test file. 这是测试文件。";
    size_t buf_len = strlen(buf);

    purc_rwstream_t rws = purc_rwstream_new_from_mem (buf, buf_len);
    ASSERT_NE(rws, nullptr);

    purc_rwstream_t rws_out = purc_rwstream_new_buffer (0, 0);
    ASSERT_NE(rws_out, nullptr);

    size_t sz = 0;
    sz = purc_rwstream_dump_to_another (rws, rws_out, 5);
    ASSERT_EQ(sz, 5);

    char* mem_buffer = (char*)purc_rwstream_get_mem_buffer (rws_out, &sz);
    ASSERT_EQ(0, strncmp(buf, mem_buffer, 5));

    purc_rwstream_seek (rws, 0, SEEK_SET);
    purc_rwstream_seek (rws_out, 0, SEEK_SET);
    sz = purc_rwstream_dump_to_another (rws, rws_out, -1);
    ASSERT_EQ(sz, buf_len);

    mem_buffer = (char*)purc_rwstream_get_mem_buffer (rws_out, &sz);
    ASSERT_EQ(0, strncmp(buf, mem_buffer, 5));
    ASSERT_STREQ(mem_buffer, buf);
    ASSERT_EQ(sz, purc_rwstream_tell(rws_out));

    int ret = purc_rwstream_destroy (rws_out);
    ASSERT_EQ(ret, 0);

    ret = purc_rwstream_destroy (rws);
    ASSERT_EQ(ret, 0);
}
