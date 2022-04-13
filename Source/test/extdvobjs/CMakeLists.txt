include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

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

target_compile_definitions(test_dvobjs_stream PRIVATE SOPATH="${CMAKE_BINARY_DIR}/Source/ExtDVObjs/fs")

set(test_dvobjs_stream_SOURCES
    test_dvobjs_stream.cpp
    helper.cpp
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
