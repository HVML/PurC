#include "config.h"
#include "purc.h"

#include "TestExtDVObj.h"
#include "../helpers.h"

#include <stdio.h>
#include <errno.h>
#include <gtest/gtest.h>


TEST(dvobjs, stream)
{
    TestExtDVObj tester;
    tester.run_testcases_in_file("stream");
}

