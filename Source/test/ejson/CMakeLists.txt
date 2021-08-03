
enable_testing()

include_directories(${PURC_DIR}/include ${PurC_DERIVED_SOURCES_DIR} ${PURC_DIR} ${CMAKE_BINARY_DIR} ${WTF_DIR})
link_directories(${CMAKE_BINARY_DIR}/Source/PurC/)

add_executable(test_stack test_stack.cpp)
target_link_libraries(test_stack purc gtest_main gtest pthread)

add_executable(test_ejson test_ejson.cpp)
target_link_libraries(test_ejson purc gtest_main gtest pthread)

add_executable(test_vcm test_vcm.cpp)
target_link_libraries(test_vcm purc gtest_main gtest pthread)

include(GoogleTest)
gtest_discover_tests(test_stack)
gtest_discover_tests(test_ejson)
gtest_discover_tests(test_vcm)
