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
GTEST_DISCOVER_TESTS(test_html DISCOVERY_TIMEOUT 10)

# test_html_parser
PURC_EXECUTABLE_DECLARE(test_html_parser)

list(APPEND test_html_parser_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_html_parser)

set(test_html_parser_SOURCES
    test_html_parser.cpp
)

set(test_html_parser_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_html_parser)
PURC_FRAMEWORK(test_html_parser)
GTEST_DISCOVER_TESTS(test_html_parser DISCOVERY_TIMEOUT 10)

# test_html_edom
PURC_EXECUTABLE_DECLARE(test_html_edom)

list(APPEND test_html_edom_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_html_edom)

set(test_html_edom_SOURCES
    html_ops.c
    test_html_edom.cpp
)

set(test_html_edom_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_html_edom)
PURC_FRAMEWORK(test_html_edom)
GTEST_DISCOVER_TESTS(test_html_edom DISCOVERY_TIMEOUT 10)

# test_dom
PURC_EXECUTABLE_DECLARE(test_dom)

list(APPEND test_dom_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_dom)

set(test_dom_SOURCES
    html_ops.c
    test_dom.cpp
)

set(test_dom_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dom)
PURC_FRAMEWORK(test_dom)
GTEST_DISCOVER_TESTS(test_dom DISCOVERY_TIMEOUT 10)

