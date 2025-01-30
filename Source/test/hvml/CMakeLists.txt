include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_hvml
PURC_EXECUTABLE_DECLARE(test_hvml)

list(APPEND test_hvml_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_hvml)

set(test_hvml_SOURCES
    test_hvml.cpp
)

set(test_hvml_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_hvml)
PURC_FRAMEWORK(test_hvml)
GTEST_DISCOVER_TESTS(test_hvml DISCOVERY_TIMEOUT 10)

# test_sbst
PURC_EXECUTABLE_DECLARE(test_sbst)

list(APPEND test_sbst_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR}/hvml
    ${PURC_DIR}/html
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_sbst)

set(test_sbst_SOURCES
    test_sbst.cpp
)

set(test_sbst_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_sbst)
PURC_FRAMEWORK(test_sbst)
GTEST_DISCOVER_TESTS(test_sbst DISCOVERY_TIMEOUT 10)

# test_tokenizer
PURC_EXECUTABLE_DECLARE(test_tokenizer)

list(APPEND test_tokenizer_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${PURC_DIR}/hvml
    ${CMAKE_BINARY_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_tokenizer)

set(test_tokenizer_SOURCES
    test_tokenizer.cpp
)

set(test_tokenizer_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_tokenizer)
PURC_FRAMEWORK(test_tokenizer)
GTEST_DISCOVER_TESTS(test_tokenizer DISCOVERY_TIMEOUT 10)

