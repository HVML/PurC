cmake_minimum_required(VERSION 3.10)

enable_testing()

add_executable(test_rwstream test_rwstream.cpp)
target_link_libraries(test_rwstream gtest_main gtest pthread)

include(GoogleTest)
gtest_discover_tests(test_rwstream)
