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


# test_dvobjs_hvml
PURC_EXECUTABLE_DECLARE(test_dvobjs_hvml)

list(APPEND test_dvobjs_hvml_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_dvobjs_hvml)

set(test_dvobjs_hvml_SOURCES
    test_dvobjs_hvml.cpp
    helper.cpp
)

set(test_dvobjs_hvml_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dvobjs_hvml)
PURC_FRAMEWORK(test_dvobjs_hvml)
GTEST_DISCOVER_TESTS(test_dvobjs_hvml DISCOVERY_TIMEOUT 10)

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

# test_dvobjs_math
PURC_EXECUTABLE_DECLARE(test_dvobjs_math)

list(APPEND test_dvobjs_math_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

target_compile_definitions(test_dvobjs_math PRIVATE SOPATH="${CMAKE_BINARY_DIR}/Source/ExtDVObjs/math")

PURC_EXECUTABLE(test_dvobjs_math)

set(test_dvobjs_math_SOURCES
    test_dvobjs_math.cpp
    helper.cpp
)

set(test_dvobjs_math_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dvobjs_math)
PURC_FRAMEWORK(test_dvobjs_math)
GTEST_DISCOVER_TESTS(test_dvobjs_math DISCOVERY_TIMEOUT 10)

# test_dvobjs_fs
PURC_EXECUTABLE_DECLARE(test_dvobjs_fs)

list(APPEND test_dvobjs_fs_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_dvobjs_fs)

target_compile_definitions(test_dvobjs_fs PRIVATE SOPATH="${CMAKE_BINARY_DIR}/Source/ExtDVObjs/fs")

set(test_dvobjs_fs_SOURCES
    test_dvobjs_fs.cpp
    helper.cpp
)

set(test_dvobjs_fs_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dvobjs_fs)
PURC_FRAMEWORK(test_dvobjs_fs)
GTEST_DISCOVER_TESTS(test_dvobjs_fs DISCOVERY_TIMEOUT 10)

# test_dvobjs_file
PURC_EXECUTABLE_DECLARE(test_dvobjs_file)

list(APPEND test_dvobjs_file_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_dvobjs_file)

target_compile_definitions(test_dvobjs_file PRIVATE SOPATH="${CMAKE_BINARY_DIR}/Source/ExtDVObjs/fs")

set(test_dvobjs_file_SOURCES
    test_dvobjs_file.cpp
    helper.cpp
)

set(test_dvobjs_file_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dvobjs_file)
PURC_FRAMEWORK(test_dvobjs_file)
GTEST_DISCOVER_TESTS(test_dvobjs_file DISCOVERY_TIMEOUT 10)

