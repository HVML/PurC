include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_hvml
PURC_EXECUTABLE_DECLARE(test_hvml)

list(APPEND test_hvml_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_hvml)

set(test_hvml_SOURCES
    test_hvml.cpp
)

set(test_hvml_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_hvml)
PURC_FRAMEWORK(test_hvml)
GTEST_DISCOVER_TESTS(test_hvml DISCOVERY_TIMEOUT 10)

# test_tempbuffer
PURC_EXECUTABLE_DECLARE(test_tempbuffer)

list(APPEND test_tempbuffer_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PURC_DIR}/hvml
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_tempbuffer)

set(test_tempbuffer_SOURCES
    test_tempbuffer.cpp
)

set(test_tempbuffer_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_tempbuffer)
PURC_FRAMEWORK(test_tempbuffer)
GTEST_DISCOVER_TESTS(test_tempbuffer DISCOVERY_TIMEOUT 10)

# test_sbst
PURC_EXECUTABLE_DECLARE(test_sbst)

list(APPEND test_sbst_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PURC_DIR}/hvml
    ${PURC_DIR}/html
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_sbst)

set(test_sbst_SOURCES
    test_sbst.cpp
)

set(test_sbst_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_sbst)
PURC_FRAMEWORK(test_sbst)
GTEST_DISCOVER_TESTS(test_sbst DISCOVERY_TIMEOUT 10)

# test_rwswrap
PURC_EXECUTABLE_DECLARE(test_rwswrap)

list(APPEND test_rwswrap_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PURC_DIR}/hvml
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_rwswrap)

set(test_rwswrap_SOURCES
    test_rwswrap.cpp
)

set(test_rwswrap_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_rwswrap)
PURC_FRAMEWORK(test_rwswrap)
GTEST_DISCOVER_TESTS(test_rwswrap DISCOVERY_TIMEOUT 10)

# test_token
PURC_EXECUTABLE_DECLARE(test_token)

list(APPEND test_token_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PURC_DIR}/hvml
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_token)

set(test_token_SOURCES
    test_token.cpp
)

set(test_token_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_token)
PURC_FRAMEWORK(test_token)
GTEST_DISCOVER_TESTS(test_token DISCOVERY_TIMEOUT 10)

# test_tokenizer
PURC_EXECUTABLE_DECLARE(test_tokenizer)

list(APPEND test_tokenizer_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PURC_DIR}/hvml
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_tokenizer)

set(test_tokenizer_SOURCES
    test_tokenizer.cpp
)

set(test_tokenizer_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_tokenizer)
PURC_FRAMEWORK(test_tokenizer)
GTEST_DISCOVER_TESTS(test_tokenizer DISCOVERY_TIMEOUT 10)
