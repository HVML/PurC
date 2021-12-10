include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_vcm_eval
PURC_EXECUTABLE_DECLARE(test_vcm_eval)

list(APPEND test_vcm_eval_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_vcm_eval)

set(test_vcm_eval_SOURCES
    test_vcm_eval.cpp
)

set(test_vcm_eval_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_vcm_eval)
PURC_FRAMEWORK(test_vcm_eval)
GTEST_DISCOVER_TESTS(test_vcm_eval DISCOVERY_TIMEOUT 10)


