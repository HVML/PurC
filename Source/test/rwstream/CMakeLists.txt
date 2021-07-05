
enable_testing()

include_directories(${PURC_DIR}/include)
link_directories(${CMAKE_BINARY_DIR}/Source/PurC/)
add_executable(test_rwstream test_rwstream.cpp)
target_link_libraries(test_rwstream purc gtest_main gtest pthread)

include(GoogleTest)
gtest_discover_tests(test_rwstream)
