#include <gtest/gtest.h>
#include "rwstream.h"

TEST(HelloTest, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");

  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}



TEST(stdio_rwstream, new_destroy) {
  purc_rwstream_t rws = purc_rwstream_new_from_file("/tmp/x.c", "r");
  ASSERT_NE(rws, nullptr);
}
