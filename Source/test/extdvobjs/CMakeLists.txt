include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_extdvobjs_math
PURC_EXECUTABLE_DECLARE(test_extdvobjs_math)

list(APPEND test_extdvobjs_math_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

target_compile_definitions(test_extdvobjs_math PRIVATE SOPATH="${CMAKE_BINARY_DIR}/Source/ExtDVObjs/math")

PURC_EXECUTABLE(test_extdvobjs_math)

set(test_extdvobjs_math_SOURCES
    test_extdvobjs_math.cpp
    helper.cpp
)

set(test_extdvobjs_math_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_extdvobjs_math)
PURC_FRAMEWORK(test_extdvobjs_math)
GTEST_DISCOVER_TESTS(test_extdvobjs_math DISCOVERY_TIMEOUT 10)

# test_extdvobjs_fs
PURC_EXECUTABLE_DECLARE(test_extdvobjs_fs)

list(APPEND test_extdvobjs_fs_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_extdvobjs_fs)

target_compile_definitions(test_extdvobjs_fs PRIVATE SOPATH="${CMAKE_BINARY_DIR}/Source/ExtDVObjs/fs")

set(test_extdvobjs_fs_SOURCES
    test_extdvobjs_fs.cpp
    helper.cpp
)

set(test_extdvobjs_fs_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_extdvobjs_fs)
PURC_FRAMEWORK(test_extdvobjs_fs)
GTEST_DISCOVER_TESTS(test_extdvobjs_fs DISCOVERY_TIMEOUT 10)

# test_extdvobjs_file
PURC_EXECUTABLE_DECLARE(test_extdvobjs_file)

list(APPEND test_extdvobjs_file_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_extdvobjs_file)

target_compile_definitions(test_extdvobjs_file PRIVATE SOPATH="${CMAKE_BINARY_DIR}/Source/ExtDVObjs/fs")

set(test_extdvobjs_file_SOURCES
    test_extdvobjs_file.cpp
    helper.cpp
)

set(test_extdvobjs_file_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_extdvobjs_file)
PURC_FRAMEWORK(test_extdvobjs_file)
GTEST_DISCOVER_TESTS(test_extdvobjs_file DISCOVERY_TIMEOUT 10)

# test_extdvobjs_stream
PURC_EXECUTABLE_DECLARE(test_extdvobjs_stream)

list(APPEND test_extdvobjs_stream_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_extdvobjs_stream)

target_compile_definitions(test_extdvobjs_stream PRIVATE SOPATH="${CMAKE_BINARY_DIR}/Source/ExtDVObjs/fs")

set(test_extdvobjs_stream_SOURCES
    test_extdvobjs_stream.cpp
    helper.cpp
    TestExtDVObj.cpp
)

set(test_extdvobjs_stream_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_extdvobjs_stream)
PURC_FRAMEWORK(test_extdvobjs_stream)
GTEST_DISCOVER_TESTS(test_extdvobjs_stream DISCOVERY_TIMEOUT 10)

# test_stream_observe
PURC_EXECUTABLE_DECLARE(test_stream_observe)

list(APPEND test_stream_observe_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_stream_observe)

set(test_stream_observe_SOURCES
    test_stream_observe.cpp
)

set(test_stream_observe_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_stream_observe)
PURC_FRAMEWORK(test_stream_observe)
GTEST_DISCOVER_TESTS(test_stream_observe DISCOVERY_TIMEOUT 10)

# test_stream_observe_write
PURC_EXECUTABLE_DECLARE(test_stream_observe_write)

list(APPEND test_stream_observe_write_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_stream_observe_write)

set(test_stream_observe_write_SOURCES
    test_stream_observe_write.cpp
)

set(test_stream_observe_write_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_stream_observe_write)
PURC_FRAMEWORK(test_stream_observe_write)
GTEST_DISCOVER_TESTS(test_stream_observe_write DISCOVERY_TIMEOUT 10)
