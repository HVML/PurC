include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_wtf
PURC_EXECUTABLE_DECLARE(test_wtf)

list(APPEND test_wtf_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${FORWARDING_HEADERS_DIR}"
    "${GIO_UNIX_INCLUDE_DIRS}"
    "${GLIB_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_wtf)

set(test_wtf_SOURCES
    test_wtf.cpp
)

set(test_wtf_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_wtf)
PURC_FRAMEWORK(test_wtf)
GTEST_DISCOVER_TESTS(test_wtf DISCOVERY_TIMEOUT 10)

