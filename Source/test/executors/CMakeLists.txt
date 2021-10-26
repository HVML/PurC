include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

macro(GEN_TEST _name)
    PURC_EXECUTABLE_DECLARE(test_exe${_name})

    list(APPEND test_exe${_name}_PRIVATE_INCLUDE_DIRECTORIES
        ${PURC_DIR}/include
        ${PurC_DERIVED_SOURCES_DIR}
        ${PURC_DIR}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/Source/PurC/executors
        ${WTF_DIR}
        ${PURC_DIR}/executors
    )

    PURC_EXECUTABLE(test_exe${_name})

    set(test_exe${_name}_SOURCES
        test-${_name}.cpp
    )

    set(test_exe${_name}_LIBRARIES
        PurC::PurC
        gtest_main
        gtest
        pthread
    )

    PURC_COMPUTE_SOURCES(test_exe${_name})
    PURC_FRAMEWORK(test_exe${_name})
    GTEST_DISCOVER_TESTS(test_exe${_name})
endmacro()

set(_targets
        key
        range
        filter
        char
        token
        add
        sub
        mul
        div
        formula
        objformula
        sql
        travel)

foreach (_target IN LISTS _targets)
    GEN_TEST(${_target})
endforeach()

unset(_targets)

