include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_vdom
PURC_EXECUTABLE_DECLARE(test_vdom)

list(APPEND test_vdom_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_vdom)

set(test_vdom_SOURCES
    test_vdom.cpp
)

set(test_vdom_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_vdom)
PURC_FRAMEWORK(test_vdom)
GTEST_DISCOVER_TESTS(test_vdom)

# test_vdom_construction
PURC_EXECUTABLE_DECLARE(test_vdom_construction)

list(APPEND test_vdom_construction_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    #ugly: for hvml_token and hvml_parser
    ${PURC_DIR}/hvml
)

PURC_EXECUTABLE(test_vdom_construction)

set(test_vdom_construction_SOURCES
    test-vdom-construction.cpp
)

set(test_vdom_construction_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_vdom_construction)
PURC_FRAMEWORK(test_vdom_construction)
GTEST_DISCOVER_TESTS(test_vdom_construction)

