include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_dvobjs
PURC_EXECUTABLE_DECLARE(test_dvobjs)

list(APPEND test_dvobjs_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_dvobjs)

set(test_dvobjs_SOURCES
    test_dvobjs.cpp
)

set(test_dvobjs_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dvobjs)
PURC_FRAMEWORK(test_dvobjs)
GTEST_DISCOVER_TESTS(test_dvobjs)

# test_math
PURC_EXECUTABLE_DECLARE(test_math)

list(APPEND test_math_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_math)

set(test_math_SOURCES
    test_math.cpp
)

set(test_math_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_math)
PURC_FRAMEWORK(test_math)
GTEST_DISCOVER_TESTS(test_math)
