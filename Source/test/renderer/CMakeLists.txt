include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_renderer_message
PURC_EXECUTABLE_DECLARE(test_renderer_message)

list(APPEND test_renderer_message_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_renderer_message)

set(test_renderer_message_SOURCES
    test_renderer_message.cpp
    client_thread.cpp
    ../dvobjs/helper.cpp
    ../tools.cpp
)

set(test_renderer_message_LIBRARIES
    PurC::PurC
    "${GTEST_MAIN_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_renderer_message)
PURC_FRAMEWORK(test_renderer_message)
GTEST_DISCOVER_TESTS(test_renderer_message DISCOVERY_TIMEOUT 10)

# test_renderer_websocket
PURC_EXECUTABLE_DECLARE(test_renderer_websocket)

list(APPEND test_renderer_websocket_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_renderer_websocket)

set(test_renderer_websocket_SOURCES
    test_renderer_websocket.cpp
    client_thread.cpp
    ../dvobjs/helper.cpp
    ../tools.cpp
)

set(test_renderer_websocket_LIBRARIES
    PurC::PurC
    "${GTEST_MAIN_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_renderer_websocket)
PURC_FRAMEWORK(test_renderer_websocket)
GTEST_DISCOVER_TESTS(test_renderer_websocket DISCOVERY_TIMEOUT 10)

