
enable_testing()


include_directories(${PURC_DIR}/include ${PurC_DERIVED_SOURCES_DIR} ${PURC_DIR} ${CMAKE_BINARY_DIR} ${WTF_DIR})
link_directories(${CMAKE_BINARY_DIR}/Source/PurC/)
add_executable(test_html test_html.cpp)
target_link_libraries(test_html purc gtest_main gtest pthread)

include(GoogleTest)
gtest_discover_tests(test_html)
