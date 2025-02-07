include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_document
PURC_EXECUTABLE_DECLARE(test_document)

list(APPEND test_document_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_document)

set(test_document_SOURCES
    test_document.cpp
)

set(test_document_LIBRARIES
    PurC::PurC
    "${GTEST_MAIN_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_document)
PURC_FRAMEWORK(test_document)
GTEST_DISCOVER_TESTS(test_document DISCOVERY_TIMEOUT 10)

