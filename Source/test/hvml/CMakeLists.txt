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
GTEST_DISCOVER_TESTS(test_hvml)

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
GTEST_DISCOVER_TESTS(test_tempbuffer)
