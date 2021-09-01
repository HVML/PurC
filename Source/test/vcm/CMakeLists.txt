include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_vcm
PURC_EXECUTABLE_DECLARE(test_vcm)

list(APPEND test_vcm_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_vcm)

set(test_vcm_SOURCES
    test_vcm.cpp
)

set(test_vcm_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_vcm)
PURC_FRAMEWORK(test_vcm)
GTEST_DISCOVER_TESTS(test_vcm)


