include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_stack
PURC_EXECUTABLE_DECLARE(test_stack)

list(APPEND test_stack_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_stack)

set(test_stack_SOURCES
    test_stack.cpp
)

set(test_stack_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_stack)
PURC_FRAMEWORK(test_stack)
GTEST_DISCOVER_TESTS(test_stack DISCOVERY_TIMEOUT 10)

# test_ejson
PURC_EXECUTABLE_DECLARE(test_ejson)

list(APPEND test_ejson_PRIVATE_INCLUDE_DIRECTORIES
        ${FORWARDING_HEADERS_DIR}
        ${PURC_DIR} ${PURC_DIR}/include
        ${CMAKE_BINARY_DIR}
        ${WTF_DIR}
        "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_ejson)

set(test_ejson_SOURCES
    test_ejson.cpp
)

set(test_ejson_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_ejson)
PURC_FRAMEWORK(test_ejson)
GTEST_DISCOVER_TESTS(test_ejson DISCOVERY_TIMEOUT 10)

# test_ejson_vcm
PURC_EXECUTABLE_DECLARE(test_ejson_vcm)

list(APPEND test_ejson_vcm_PRIVATE_INCLUDE_DIRECTORIES
        ${FORWARDING_HEADERS_DIR}
        ${PURC_DIR} ${PURC_DIR}/include
        ${CMAKE_BINARY_DIR}
        ${WTF_DIR}
        "${GTEST_INCLUDE_DIRS}"
        )

PURC_EXECUTABLE(test_ejson_vcm)

set(test_ejson_vcm_SOURCES
    test_vcm.cpp
)

set(test_ejson_vcm_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_ejson_vcm)
PURC_FRAMEWORK(test_ejson_vcm)
GTEST_DISCOVER_TESTS(test_ejson_vcm DISCOVERY_TIMEOUT 10)

# test_ref
PURC_EXECUTABLE_DECLARE(test_ref)

list(APPEND test_ref_PRIVATE_INCLUDE_DIRECTORIES
        ${FORWARDING_HEADERS_DIR}
        ${PURC_DIR} ${PURC_DIR}/include
        ${CMAKE_BINARY_DIR}
        ${WTF_DIR}
        "${GTEST_INCLUDE_DIRS}"
        )

PURC_EXECUTABLE(test_ref)

set(test_ref_SOURCES
    test_ref.cpp
)

set(test_ref_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_ref)
PURC_FRAMEWORK(test_ref)
GTEST_DISCOVER_TESTS(test_ref DISCOVERY_TIMEOUT 10)

# test_jsonee
PURC_EXECUTABLE_DECLARE(test_jsonee)

list(APPEND test_jsonee_PRIVATE_INCLUDE_DIRECTORIES
        ${FORWARDING_HEADERS_DIR}
        ${PURC_DIR} ${PURC_DIR}/include
        ${CMAKE_BINARY_DIR}
        ${WTF_DIR}
        "${GTEST_INCLUDE_DIRS}"
        )

PURC_EXECUTABLE(test_jsonee)

set(test_jsonee_SOURCES
    test_jsonee.cpp
)

set(test_jsonee_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_jsonee)
PURC_FRAMEWORK(test_jsonee)
GTEST_DISCOVER_TESTS(test_jsonee DISCOVERY_TIMEOUT 10)
