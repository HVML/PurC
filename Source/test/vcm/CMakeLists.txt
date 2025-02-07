include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_vcm
PURC_EXECUTABLE_DECLARE(test_vcm)

list(APPEND test_vcm_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_vcm)

set(test_vcm_SOURCES
    test_vcm.cpp
)

set(test_vcm_LIBRARIES
    PurC::PurC
    "${GTEST_MAIN_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_vcm)
PURC_FRAMEWORK(test_vcm)
GTEST_DISCOVER_TESTS(test_vcm DISCOVERY_TIMEOUT 10)


# test_eval
PURC_EXECUTABLE_DECLARE(test_eval)

list(APPEND test_eval_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_eval)

set(test_eval_SOURCES
    test_eval.cpp
)

set(test_eval_LIBRARIES
    PurC::PurC
    "${GTEST_MAIN_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_eval)
PURC_FRAMEWORK(test_eval)
GTEST_DISCOVER_TESTS(test_eval DISCOVERY_TIMEOUT 10)


