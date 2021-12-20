include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_vcm_eval
PURC_EXECUTABLE_DECLARE(test_vcm_eval)

list(APPEND test_vcm_eval_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_vcm_eval)

set(test_vcm_eval_SOURCES
    test_vcm_eval.cpp
)

set(test_vcm_eval_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_vcm_eval)
PURC_FRAMEWORK(test_vcm_eval)
GTEST_DISCOVER_TESTS(test_vcm_eval DISCOVERY_TIMEOUT 10)


# test_interpreter
PURC_EXECUTABLE_DECLARE(test_interpreter)

list(APPEND test_interpreter_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_interpreter)

set(test_interpreter_SOURCES
    test-interpreter.cpp
)

set(test_interpreter_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_interpreter)
PURC_FRAMEWORK(test_interpreter)
GTEST_DISCOVER_TESTS(test_interpreter DISCOVERY_TIMEOUT 10)


# test_timer
PURC_EXECUTABLE_DECLARE(test_timer)

list(APPEND test_timer_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_timer)

set(test_timer_SOURCES
    test-timer.cpp
)

set(test_timer_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_timer)
PURC_FRAMEWORK(test_timer)
GTEST_DISCOVER_TESTS(test_timer DISCOVERY_TIMEOUT 10)

# test_idle
PURC_EXECUTABLE_DECLARE(test_idle)

list(APPEND test_idle_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_idle)

set(test_idle_SOURCES
    test-idle.cpp
)

set(test_idle_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_idle)
PURC_FRAMEWORK(test_idle)
GTEST_DISCOVER_TESTS(test_idle DISCOVERY_TIMEOUT 10)
