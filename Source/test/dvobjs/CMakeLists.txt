include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_dvobjs_system
PURC_EXECUTABLE_DECLARE(test_dvobjs_system)

list(APPEND test_dvobjs_system_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_dvobjs_system)

set(test_dvobjs_system_SOURCES
    test_dvobjs_system.cpp
    helper.cpp
)

set(test_dvobjs_system_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dvobjs_system)
PURC_FRAMEWORK(test_dvobjs_system)
GTEST_DISCOVER_TESTS(test_dvobjs_system DISCOVERY_TIMEOUT 10)

# test_dvobjs_datetime
PURC_EXECUTABLE_DECLARE(test_dvobjs_datetime)

list(APPEND test_dvobjs_datetime_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_dvobjs_datetime)

set(test_dvobjs_datetime_SOURCES
    test_dvobjs_datetime.cpp
    helper.cpp
)

set(test_dvobjs_datetime_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dvobjs_datetime)
PURC_FRAMEWORK(test_dvobjs_datetime)
GTEST_DISCOVER_TESTS(test_dvobjs_datetime DISCOVERY_TIMEOUT 10)

# test_ejson
PURC_EXECUTABLE_DECLARE(test_dvobjs_ejson)

list(APPEND test_dvobjs_ejson_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    ${GLIB_INCLUDE_DIR}
    ${GLIBCONFIG_INCLUDE_DIR}
)

PURC_EXECUTABLE(test_dvobjs_ejson)

set(test_dvobjs_ejson_SOURCES
    test_dvobjs_ejson.cpp
    helper.cpp
)

set(test_dvobjs_ejson_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dvobjs_ejson)
PURC_FRAMEWORK(test_dvobjs_ejson)
GTEST_DISCOVER_TESTS(test_dvobjs_ejson DISCOVERY_TIMEOUT 10)

# test_ejson_more
PURC_EXECUTABLE_DECLARE(test_dvobjs_ejson_more)

list(APPEND test_dvobjs_ejson_more_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    ${GLIB_INCLUDE_DIR}
    ${GLIBCONFIG_INCLUDE_DIR}
)

PURC_EXECUTABLE(test_dvobjs_ejson_more)

set(test_dvobjs_ejson_more_SOURCES
    test_dvobjs_ejson_more.cpp
    helper.cpp
)

set(test_dvobjs_ejson_more_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dvobjs_ejson_more)
PURC_FRAMEWORK(test_dvobjs_ejson_more)
GTEST_DISCOVER_TESTS(test_dvobjs_ejson_more DISCOVERY_TIMEOUT 10)

# test_dvobjs_coroutine
PURC_EXECUTABLE_DECLARE(test_dvobjs_coroutine)

list(APPEND test_dvobjs_coroutine_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_dvobjs_coroutine)

set(test_dvobjs_coroutine_SOURCES
    test_dvobjs_coroutine.cpp
    helper.cpp
)

set(test_dvobjs_coroutine_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dvobjs_coroutine)
PURC_FRAMEWORK(test_dvobjs_coroutine)
GTEST_DISCOVER_TESTS(test_dvobjs_coroutine DISCOVERY_TIMEOUT 10)

# test_dvobjs_text
PURC_EXECUTABLE_DECLARE(test_dvobjs_text)

list(APPEND test_dvobjs_text_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_dvobjs_text)

set(test_dvobjs_text_SOURCES
    test_dvobjs_text.cpp
    helper.cpp
)

set(test_dvobjs_text_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dvobjs_text)
PURC_FRAMEWORK(test_dvobjs_text)
GTEST_DISCOVER_TESTS(test_dvobjs_text DISCOVERY_TIMEOUT 10)

# test_dvobjs_logical
PURC_EXECUTABLE_DECLARE(test_dvobjs_logical)

list(APPEND test_dvobjs_logical_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_dvobjs_logical)

set(test_dvobjs_logical_SOURCES
    test_dvobjs_logical.cpp
    helper.cpp
)

set(test_dvobjs_logical_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dvobjs_logical)
PURC_FRAMEWORK(test_dvobjs_logical)
GTEST_DISCOVER_TESTS(test_dvobjs_logical DISCOVERY_TIMEOUT 10)


# test_dvobjs_string
PURC_EXECUTABLE_DECLARE(test_dvobjs_string)

list(APPEND test_dvobjs_string_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_dvobjs_string)

set(test_dvobjs_string_SOURCES
    test_dvobjs_string.cpp
    helper.cpp
)

set(test_dvobjs_string_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dvobjs_string)
PURC_FRAMEWORK(test_dvobjs_string)
GTEST_DISCOVER_TESTS(test_dvobjs_string DISCOVERY_TIMEOUT 10)

# test_dvobjs_overall
PURC_EXECUTABLE_DECLARE(test_dvobjs_overall)

list(APPEND test_dvobjs_overall_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_dvobjs_overall)

set(test_dvobjs_overall_SOURCES
    test_dvobjs_overall.cpp
    helper.cpp
    TestDVObj.cpp
)

set(test_dvobjs_overall_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dvobjs_overall)
PURC_FRAMEWORK(test_dvobjs_overall)
GTEST_DISCOVER_TESTS(test_dvobjs_overall DISCOVERY_TIMEOUT 10)

# test_dvobjs_stream
PURC_EXECUTABLE_DECLARE(test_dvobjs_stream)

list(APPEND test_dvobjs_stream_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_dvobjs_stream)

set(test_dvobjs_stream_SOURCES
    test_dvobjs_stream.cpp
    helper.cpp
    TestDVObj.cpp
)

set(test_dvobjs_stream_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dvobjs_stream)
PURC_FRAMEWORK(test_dvobjs_stream)
GTEST_DISCOVER_TESTS(test_dvobjs_stream DISCOVERY_TIMEOUT 10)

# test_stream_observe_readable
PURC_EXECUTABLE_DECLARE(test_stream_observe_readable)

list(APPEND test_stream_observe_readable_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_stream_observe_readable)

set(test_stream_observe_readable_SOURCES
    test_stream_observe_readable.cpp
)

set(test_stream_observe_readable_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_stream_observe_readable)
PURC_FRAMEWORK(test_stream_observe_readable)
GTEST_DISCOVER_TESTS(test_stream_observe_readable DISCOVERY_TIMEOUT 10)

# test_stream_observe_writable
PURC_EXECUTABLE_DECLARE(test_stream_observe_writable)

list(APPEND test_stream_observe_writable_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_stream_observe_writable)

set(test_stream_observe_writable_SOURCES
    test_stream_observe_writable.cpp
)

set(test_stream_observe_writable_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_stream_observe_writable)
PURC_FRAMEWORK(test_stream_observe_writable)
GTEST_DISCOVER_TESTS(test_stream_observe_writable DISCOVERY_TIMEOUT 10)

# test_dvobjs_runner
PURC_EXECUTABLE_DECLARE(test_dvobjs_runner)

list(APPEND test_dvobjs_runner_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_dvobjs_runner)

set(test_dvobjs_runner_SOURCES
    test_dvobjs_runner.cpp
    helper.cpp
    TestDVObj.cpp
)

set(test_dvobjs_runner_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dvobjs_runner)
PURC_FRAMEWORK(test_dvobjs_runner)
GTEST_DISCOVER_TESTS(test_dvobjs_runner DISCOVERY_TIMEOUT 10)

