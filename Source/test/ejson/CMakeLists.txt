include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_stack
PURC_EXECUTABLE_DECLARE(test_stack)

list(APPEND test_stack_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_stack)

set(test_stack_SOURCES
    test_stack.cpp
)

set(test_stack_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_stack)
PURC_FRAMEWORK(test_stack)
GTEST_DISCOVER_TESTS(test_stack DISCOVERY_TIMEOUT 10)

# test_ejson
PURC_EXECUTABLE_DECLARE(test_ejson)

list(APPEND test_ejson_PRIVATE_INCLUDE_DIRECTORIES
        ${PURC_DIR}/include
        ${PurC_DERIVED_SOURCES_DIR}
        ${PURC_DIR}
        ${CMAKE_BINARY_DIR}
        ${WTF_DIR})

PURC_EXECUTABLE(test_ejson)

set(test_ejson_SOURCES
    test_ejson.cpp
)

set(test_ejson_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_ejson)
PURC_FRAMEWORK(test_ejson)
GTEST_DISCOVER_TESTS(test_ejson DISCOVERY_TIMEOUT 10)

# test_ejson_vcm
PURC_EXECUTABLE_DECLARE(test_ejson_vcm)

list(APPEND test_ejson_vcm_PRIVATE_INCLUDE_DIRECTORIES
        ${PURC_DIR}/include
        ${PurC_DERIVED_SOURCES_DIR}
        ${PURC_DIR}
        ${CMAKE_BINARY_DIR}
        ${WTF_DIR})

PURC_EXECUTABLE(test_ejson_vcm)

set(test_ejson_vcm_SOURCES
    test_vcm.cpp
)

set(test_ejson_vcm_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_ejson_vcm)
PURC_FRAMEWORK(test_ejson_vcm)
GTEST_DISCOVER_TESTS(test_ejson_vcm DISCOVERY_TIMEOUT 10)

# test_ref
PURC_EXECUTABLE_DECLARE(test_ref)

list(APPEND test_ref_PRIVATE_INCLUDE_DIRECTORIES
        ${PURC_DIR}/include
        ${PurC_DERIVED_SOURCES_DIR}
        ${PURC_DIR}
        ${CMAKE_BINARY_DIR}
        ${WTF_DIR})

PURC_EXECUTABLE(test_ref)

set(test_ref_SOURCES
    test_ref.cpp
)

set(test_ref_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_ref)
PURC_FRAMEWORK(test_ref)
GTEST_DISCOVER_TESTS(test_ref DISCOVERY_TIMEOUT 10)

