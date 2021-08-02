
enable_testing()

include_directories(${PURC_DIR}/include ${PurC_DERIVED_SOURCES_DIR})
link_directories(${CMAKE_BINARY_DIR}/Source/PurC/)
add_executable(test_utils test_utils.cpp)
target_link_libraries(test_utils purc gtest_main gtest pthread)

include(GoogleTest)
gtest_discover_tests(test_utils)
