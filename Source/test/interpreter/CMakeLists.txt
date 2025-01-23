include(PurCCommon)
include(target/PurC)
include(GoogleTest)

PURC_COPY_FILES(TEST_Assets
    DESTINATION "${CMAKE_BINARY_DIR}"
    FILES "assets/buttons.json"
    FILES "assets/plain.txt"
    FILES "assets/test.sh"
    FILES "assets/test.xml"
)

enable_testing()

# test_vcm_eval
PURC_EXECUTABLE_DECLARE(test_vcm_eval)

list(APPEND test_vcm_eval_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_vcm_eval)

set(test_vcm_eval_SOURCES
    test_vcm_eval.cpp
)

set(test_vcm_eval_LIBRARIES
    gtest_main
    gtest
    PurC::PurC
    pthread
)

PURC_COMPUTE_SOURCES(test_vcm_eval)
PURC_FRAMEWORK(test_vcm_eval)
GTEST_DISCOVER_TESTS(test_vcm_eval DISCOVERY_TIMEOUT 10)


# test_interpreter
PURC_EXECUTABLE_DECLARE(test_interpreter)

list(APPEND test_interpreter_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_interpreter)

set(test_interpreter_SOURCES
    test_interpreter.cpp
)

set(test_interpreter_LIBRARIES
    gtest_main
    gtest
    PurC::PurC
    pthread
)

PURC_COMPUTE_SOURCES(test_interpreter)
PURC_FRAMEWORK(test_interpreter)
GTEST_DISCOVER_TESTS(test_interpreter DISCOVERY_TIMEOUT 10)

## test_observe
PURC_EXECUTABLE_DECLARE(test_observe)

list(APPEND test_observe_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_observe)

set(test_observe_SOURCES
    test_observe.cpp
)

set(test_observe_LIBRARIES
    gtest_main
    gtest
    PurC::PurC
    pthread
)

PURC_COMPUTE_SOURCES(test_observe)
PURC_FRAMEWORK(test_observe)
GTEST_DISCOVER_TESTS(test_observe DISCOVERY_TIMEOUT 10)

if (0)
    # test_observe_named
    PURC_EXECUTABLE_DECLARE(test_observe_named)

    list(APPEND test_observe_named_PRIVATE_INCLUDE_DIRECTORIES
        ${FORWARDING_HEADERS_DIR}
        ${PURC_DIR} ${PURC_DIR}/include
        ${CMAKE_BINARY_DIR}
        ${PurC_DERIVED_SOURCES_DIR}
        ${WTF_DIR}
    )

    PURC_EXECUTABLE(test_observe_named)

    set(test_observe_named_SOURCES
        test_observe_named.cpp
    )

    set(test_observe_named_LIBRARIES
        gtest_main
        gtest
        PurC::PurC
        pthread
    )

    PURC_COMPUTE_SOURCES(test_observe_named)
    PURC_FRAMEWORK(test_observe_named)
    GTEST_DISCOVER_TESTS(test_observe_named DISCOVERY_TIMEOUT 10)
endif ()

# test_doc_var
PURC_EXECUTABLE_DECLARE(test_doc_var)

list(APPEND test_doc_var_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_doc_var)

set(test_doc_var_SOURCES
    tools.cpp
    test_doc_var.cpp
)

set(test_doc_var_LIBRARIES
    gtest_main
    gtest
    PurC::PurC
    pthread
)

PURC_COMPUTE_SOURCES(test_doc_var)
PURC_FRAMEWORK(test_doc_var)
GTEST_DISCOVER_TESTS(test_doc_var DISCOVERY_TIMEOUT 10)

# test_test
PURC_EXECUTABLE_DECLARE(test_test)

list(APPEND test_test_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_test)

set(test_test_SOURCES
    test_test.cpp
)

set(test_test_LIBRARIES
    gtest_main
    gtest
    PurC::PurC
    pthread
)

PURC_COMPUTE_SOURCES(test_test)
PURC_FRAMEWORK(test_test)
GTEST_DISCOVER_TESTS(test_test DISCOVERY_TIMEOUT 10)

# test_intr_samples
PURC_EXECUTABLE_DECLARE(test_intr_samples)

list(APPEND test_intr_samples_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_intr_samples)

set(test_intr_samples_SOURCES
    tools.cpp
    test_intr_samples.cpp
)

set(test_intr_samples_LIBRARIES
    gtest_main
    gtest
    PurC::PurC
    pthread
)

PURC_COMPUTE_SOURCES(test_intr_samples)
PURC_FRAMEWORK(test_intr_samples)
GTEST_DISCOVER_TESTS(test_intr_samples DISCOVERY_TIMEOUT 10)

# test_attach_rdr
PURC_EXECUTABLE_DECLARE(test_attach_rdr)

list(APPEND test_attach_rdr_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_attach_rdr)

set(test_attach_rdr_SOURCES
    test_attach_rdr.cpp
)

set(test_attach_rdr_LIBRARIES
    gtest_main
    gtest
    PurC::PurC
    pthread
)

PURC_COMPUTE_SOURCES(test_attach_rdr)
PURC_FRAMEWORK(test_attach_rdr)
GTEST_DISCOVER_TESTS(test_attach_rdr DISCOVERY_TIMEOUT 10)

# test_samples
PURC_EXECUTABLE_DECLARE(test_samples)

list(APPEND test_samples_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_samples)

set(test_samples_SOURCES
    tools.cpp
    test_samples.cpp
)

set(test_samples_LIBRARIES
    gtest_main
    gtest
    PurC::PurC
    pthread
)

target_compile_definitions(test_samples PRIVATE SOPATH="${CMAKE_BINARY_DIR}/lib")

PURC_COMPUTE_SOURCES(test_samples)
PURC_FRAMEWORK(test_samples)
GTEST_DISCOVER_TESTS(test_samples DISCOVERY_TIMEOUT 10)

# test_hvml_tag
PURC_EXECUTABLE_DECLARE(test_hvml_tag)

list(APPEND test_hvml_tag_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_hvml_tag)

set(test_hvml_tag_SOURCES
    tools.cpp
    test_hvml_tag.cpp
)

set(test_hvml_tag_LIBRARIES
    gtest_main
    gtest
    PurC::PurC
    pthread
)

target_compile_definitions(test_hvml_tag PRIVATE SOPATH="${CMAKE_BINARY_DIR}/lib")

PURC_COMPUTE_SOURCES(test_hvml_tag)
PURC_FRAMEWORK(test_hvml_tag)
GTEST_DISCOVER_TESTS(test_hvml_tag DISCOVERY_TIMEOUT 10)

# test_simple_runner
PURC_EXECUTABLE_DECLARE(test_simple_runner)

list(APPEND test_simple_runner_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_simple_runner)

set(test_simple_runner_SOURCES
    test_simple_runner.cpp
)

set(test_simple_runner_LIBRARIES
    gtest_main
    gtest
    PurC::PurC
    pthread
)

PURC_COMPUTE_SOURCES(test_simple_runner)
PURC_FRAMEWORK(test_simple_runner)
GTEST_DISCOVER_TESTS(test_simple_runner DISCOVERY_TIMEOUT 10)

# test_runners
PURC_EXECUTABLE_DECLARE(test_runners)

list(APPEND test_runners_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_runners)

set(test_runners_SOURCES
    test_runners.cpp
)

set(test_runners_LIBRARIES
    gtest_main
    gtest
    PurC::PurC
    pthread
)

PURC_COMPUTE_SOURCES(test_runners)
PURC_FRAMEWORK(test_runners)
GTEST_DISCOVER_TESTS(test_runners DISCOVERY_TIMEOUT 10)

# test_void_document
PURC_EXECUTABLE_DECLARE(test_void_document)

list(APPEND test_void_document_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_void_document)

set(test_void_document_SOURCES
    test_void_document.cpp
)

set(test_void_document_LIBRARIES
    gtest_main
    gtest
    PurC::PurC
    pthread
)

PURC_COMPUTE_SOURCES(test_void_document)
PURC_FRAMEWORK(test_void_document)

# test_comprehensive_programs
PURC_EXECUTABLE_DECLARE(test_comprehensive_programs)

list(APPEND test_comprehensive_programs_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_comprehensive_programs)

set(test_comprehensive_programs_SOURCES
    tools.cpp
    test_comprehensive_programs.cpp
)

set(test_comprehensive_programs_LIBRARIES
    gtest_main
    gtest
    PurC::PurC
    pthread
)

PURC_COMPUTE_SOURCES(test_comprehensive_programs)
PURC_FRAMEWORK(test_comprehensive_programs)

# test_quit_main_runner
PURC_EXECUTABLE_DECLARE(test_quit_main_runner)

list(APPEND test_quit_main_runner_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_quit_main_runner)

set(test_quit_main_runner_SOURCES
    test_quit_main_runner.cpp
)

set(test_quit_main_runner_LIBRARIES
    gtest_main
    gtest
    PurC::PurC
    pthread
)

PURC_COMPUTE_SOURCES(test_quit_main_runner)
PURC_FRAMEWORK(test_quit_main_runner)
GTEST_DISCOVER_TESTS(test_quit_main_runner DISCOVERY_TIMEOUT 10)

# test_inherit_document
PURC_EXECUTABLE_DECLARE(test_inherit_document)

list(APPEND test_inherit_document_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_inherit_document)

set(test_inherit_document_SOURCES
    tools.cpp
    test_inherit_document.cpp
)

set(test_inherit_document_LIBRARIES
    gtest_main
    gtest
    PurC::PurC
    pthread
)

PURC_COMPUTE_SOURCES(test_inherit_document)
PURC_FRAMEWORK(test_inherit_document)
GTEST_DISCOVER_TESTS(test_inherit_document DISCOVERY_TIMEOUT 10)

