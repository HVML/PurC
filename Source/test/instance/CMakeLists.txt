include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_def
PURC_EXECUTABLE_DECLARE(test_def)

list(APPEND test_def_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_def)

set(test_def_SOURCES
    test_def.cpp
)

set(test_def_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_def)
PURC_FRAMEWORK(test_def)
GTEST_DISCOVER_TESTS(test_def DISCOVERY_TIMEOUT 10)

# test_syslog
PURC_EXECUTABLE_DECLARE(test_syslog)

list(APPEND test_syslog_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_syslog)

set(test_syslog_SOURCES
    test_syslog.cpp
)

set(test_syslog_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_syslog)
PURC_FRAMEWORK(test_syslog)
GTEST_DISCOVER_TESTS(test_syslog DISCOVERY_TIMEOUT 10)

# test_mylog
PURC_EXECUTABLE_DECLARE(test_mylog)

list(APPEND test_mylog_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_mylog)

set(test_mylog_SOURCES
    test_mylog.cpp
)

set(test_mylog_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_mylog)
PURC_FRAMEWORK(test_mylog)
GTEST_DISCOVER_TESTS(test_mylog DISCOVERY_TIMEOUT 10)

# test_threads
PURC_EXECUTABLE_DECLARE(test_threads)

list(APPEND test_threads_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_threads)

set(test_threads_SOURCES
    test_threads.cpp
)

set(test_threads_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_threads)
PURC_FRAMEWORK(test_threads)
GTEST_DISCOVER_TESTS(test_threads DISCOVERY_TIMEOUT 10)

# test_responser
PURC_EXECUTABLE_DECLARE(test_responser)

list(APPEND test_responser_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_responser)

set(test_responser_SOURCES
    test_responser.cpp
)

set(test_responser_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_responser)
PURC_FRAMEWORK(test_responser)
GTEST_DISCOVER_TESTS(test_responser DISCOVERY_TIMEOUT 10)

# test_requester
PURC_EXECUTABLE_DECLARE(test_requester)

list(APPEND test_requester_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_requester)

set(test_requester_SOURCES
    test_requester.cpp
)

set(test_requester_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_requester)
PURC_FRAMEWORK(test_requester)
GTEST_DISCOVER_TESTS(test_requester DISCOVERY_TIMEOUT 10)

# test_init
PURC_EXECUTABLE_DECLARE(test_init)

list(APPEND test_init_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_init)

set(test_init_SOURCES
    test_init.cpp
)

set(test_init_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_init)
PURC_FRAMEWORK(test_init)
GTEST_DISCOVER_TESTS(test_init DISCOVERY_TIMEOUT 10)

# test_pcrdr_init
PURC_EXECUTABLE_DECLARE(test_pcrdr_init)

list(APPEND test_pcrdr_init_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_pcrdr_init)

set(test_pcrdr_init_SOURCES
    test_pcrdr_init.cpp
)

set(test_pcrdr_init_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_pcrdr_init)
PURC_FRAMEWORK(test_pcrdr_init)
GTEST_DISCOVER_TESTS(test_pcrdr_init DISCOVERY_TIMEOUT 10)

