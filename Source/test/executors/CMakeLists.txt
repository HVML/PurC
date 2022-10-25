include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

macro(GEN_TEST _name)
    PURC_EXECUTABLE_DECLARE(test_exe${_name})

    list(APPEND test_exe${_name}_PRIVATE_INCLUDE_DIRECTORIES
        ${FORWARDING_HEADERS_DIR}
        ${PURC_DIR} ${PURC_DIR}/include
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/Source/PurC/executors/parsers
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
    GTEST_DISCOVER_TESTS(test_exe${_name} DISCOVERY_TIMEOUT 10)
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

# test_executors
PURC_EXECUTABLE_DECLARE(test_executors)

list(APPEND test_executors_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_executors)

set(test_executors_SOURCES
    test-executors.cpp
)

set(test_executors_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_executors)
PURC_FRAMEWORK(test_executors)
GTEST_DISCOVER_TESTS(test_executors DISCOVERY_TIMEOUT 10)

