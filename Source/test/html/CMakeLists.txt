include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_html
PURC_EXECUTABLE_DECLARE(test_html)

list(APPEND test_html_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_html)

set(test_html_SOURCES
    test_html.cpp
)

set(test_html_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_html)
PURC_FRAMEWORK(test_html)
GTEST_DISCOVER_TESTS(test_html)
