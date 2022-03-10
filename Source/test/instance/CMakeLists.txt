include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_def
PURC_EXECUTABLE_DECLARE(test_def)

list(APPEND test_def_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_def)

set(test_def_SOURCES
    test_def.cpp
)

set(test_def_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_def)
PURC_FRAMEWORK(test_def)
GTEST_DISCOVER_TESTS(test_def DISCOVERY_TIMEOUT 10)

# test_syslog
PURC_EXECUTABLE_DECLARE(test_syslog)

list(APPEND test_syslog_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_syslog)

set(test_syslog_SOURCES
    test_syslog.cpp
)

set(test_syslog_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_syslog)
PURC_FRAMEWORK(test_syslog)
GTEST_DISCOVER_TESTS(test_syslog DISCOVERY_TIMEOUT 10)

# test_mylog
PURC_EXECUTABLE_DECLARE(test_mylog)

list(APPEND test_mylog_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_mylog)

set(test_mylog_SOURCES
    test_mylog.cpp
)

set(test_mylog_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_mylog)
PURC_FRAMEWORK(test_mylog)
GTEST_DISCOVER_TESTS(test_mylog DISCOVERY_TIMEOUT 10)

# test_threads
PURC_EXECUTABLE_DECLARE(test_threads)

list(APPEND test_threads_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_threads)

set(test_threads_SOURCES
    test_threads.cpp
)

set(test_threads_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_threads)
PURC_FRAMEWORK(test_threads)
GTEST_DISCOVER_TESTS(test_threads DISCOVERY_TIMEOUT 10)
