#include "purc.h"

#include "private/hvml.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

TEST(hvml_tokenization, new_destory)
{
    struct pchvml_parser* parser = pchvml_create(0, 32);
    ASSERT_NE(parser, nullptr);

    pchvml_destroy (parser);
}



