
enable_testing()

add_executable(test_rwstream test_rwstream.cpp)
target_link_libraries(test_rwstream PurC gtest_main gtest pthread)

include(GoogleTest)
gtest_discover_tests(test_rwstream)

