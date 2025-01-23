include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_tree
PURC_EXECUTABLE_DECLARE(test_tree)

list(APPEND test_tree_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
)

PURC_EXECUTABLE(test_tree)

set(test_tree_SOURCES
    test_tree.cpp
)

set(test_tree_LIBRARIES
    gtest_main
    gtest
    PurC::PurC
    pthread
)

PURC_COMPUTE_SOURCES(test_tree)
PURC_FRAMEWORK(test_tree)
GTEST_DISCOVER_TESTS(test_tree DISCOVERY_TIMEOUT 10)

