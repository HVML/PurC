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
    test_interpreter.cpp
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
    test_timer.cpp
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
    test_idle.cpp
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

## test_observe
PURC_EXECUTABLE_DECLARE(test_observe)

list(APPEND test_observe_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_observe)

set(test_observe_SOURCES
    test_observe.cpp
)

set(test_observe_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_observe)
PURC_FRAMEWORK(test_observe)
GTEST_DISCOVER_TESTS(test_observe DISCOVERY_TIMEOUT 10)

# test_observe_named
PURC_EXECUTABLE_DECLARE(test_observe_named)

list(APPEND test_observe_named_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_observe_named)

set(test_observe_named_SOURCES
    test_observe_named.cpp
)

set(test_observe_named_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_observe_named)
PURC_FRAMEWORK(test_observe_named)
GTEST_DISCOVER_TESTS(test_observe_named DISCOVERY_TIMEOUT 10)

# test_doc_var
PURC_EXECUTABLE_DECLARE(test_doc_var)

list(APPEND test_doc_var_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_doc_var)

set(test_doc_var_SOURCES
    test_doc_var.cpp
)

set(test_doc_var_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_doc_var)
PURC_FRAMEWORK(test_doc_var)
GTEST_DISCOVER_TESTS(test_doc_var DISCOVERY_TIMEOUT 10)

# test_test
PURC_EXECUTABLE_DECLARE(test_test)

list(APPEND test_test_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_test)

set(test_test_SOURCES
    test_test.cpp
)

set(test_test_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_test)
PURC_FRAMEWORK(test_test)
GTEST_DISCOVER_TESTS(test_test DISCOVERY_TIMEOUT 10)

# test_intr_samples
PURC_EXECUTABLE_DECLARE(test_intr_samples)

list(APPEND test_intr_samples_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_intr_samples)

set(test_intr_samples_SOURCES
    test_intr_samples.cpp
)

set(test_intr_samples_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_intr_samples)
PURC_FRAMEWORK(test_intr_samples)
GTEST_DISCOVER_TESTS(test_intr_samples DISCOVERY_TIMEOUT 10)

# test_attach_rdr
PURC_EXECUTABLE_DECLARE(test_attach_rdr)

list(APPEND test_attach_rdr_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_attach_rdr)

set(test_attach_rdr_SOURCES
    test_attach_rdr.cpp
)

set(test_attach_rdr_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_attach_rdr)
PURC_FRAMEWORK(test_attach_rdr)
GTEST_DISCOVER_TESTS(test_attach_rdr DISCOVERY_TIMEOUT 10)

# test_samples
PURC_EXECUTABLE_DECLARE(test_samples)

list(APPEND test_samples_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_samples)

set(test_samples_SOURCES
    test_samples.cpp
)

set(test_samples_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

target_compile_definitions(test_samples PRIVATE SOPATH="${CMAKE_BINARY_DIR}/lib")

PURC_COMPUTE_SOURCES(test_samples)
PURC_FRAMEWORK(test_samples)
GTEST_DISCOVER_TESTS(test_samples DISCOVERY_TIMEOUT 10)

# test_hvml_tag
PURC_EXECUTABLE_DECLARE(test_hvml_tag)

list(APPEND test_hvml_tag_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_hvml_tag)

set(test_hvml_tag_SOURCES
    test_hvml_tag.cpp
)

set(test_hvml_tag_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

target_compile_definitions(test_hvml_tag PRIVATE SOPATH="${CMAKE_BINARY_DIR}/lib")

PURC_COMPUTE_SOURCES(test_hvml_tag)
PURC_FRAMEWORK(test_hvml_tag)
GTEST_DISCOVER_TESTS(test_hvml_tag DISCOVERY_TIMEOUT 10)

# test_simple_runner
PURC_EXECUTABLE_DECLARE(test_simple_runner)

list(APPEND test_simple_runner_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_simple_runner)

set(test_simple_runner_SOURCES
    test_simple_runner.cpp
)

set(test_simple_runner_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_simple_runner)
PURC_FRAMEWORK(test_simple_runner)
GTEST_DISCOVER_TESTS(test_simple_runner DISCOVERY_TIMEOUT 10)

# test_runners
PURC_EXECUTABLE_DECLARE(test_runners)

list(APPEND test_runners_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_runners)

set(test_runners_SOURCES
    test_runners.cpp
)

set(test_runners_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_runners)
PURC_FRAMEWORK(test_runners)
GTEST_DISCOVER_TESTS(test_runners DISCOVERY_TIMEOUT 10)

