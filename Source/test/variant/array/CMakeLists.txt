
enable_testing()


include_directories(
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)
link_directories(${CMAKE_BINARY_DIR}/Source/PurC/)
add_executable(test_array test_array.cpp)
target_link_libraries(test_array purc gtest_main gtest pthread)

include(GoogleTest)
gtest_discover_tests(test_array)
