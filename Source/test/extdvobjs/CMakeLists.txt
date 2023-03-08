include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_extdvobjs_math
PURC_EXECUTABLE_DECLARE(test_extdvobjs_math)

list(APPEND test_extdvobjs_math_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

target_compile_definitions(test_extdvobjs_math PRIVATE SOPATH="${CMAKE_BINARY_DIR}/lib")

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
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_extdvobjs_fs)

target_compile_definitions(test_extdvobjs_fs PRIVATE SOPATH="${CMAKE_BINARY_DIR}/lib")

set(test_extdvobjs_fs_SOURCES
    test_extdvobjs_fs.cpp
    TestExtDVObj.cpp
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
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_extdvobjs_file)

target_compile_definitions(test_extdvobjs_file PRIVATE SOPATH="${CMAKE_BINARY_DIR}/lib")

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

# test_extdvobjs_py
PURC_EXECUTABLE_DECLARE(test_extdvobjs_py)

list(APPEND test_extdvobjs_py_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_extdvobjs_py)

target_compile_definitions(test_extdvobjs_py PRIVATE SOPATH="${CMAKE_BINARY_DIR}/lib")

set(test_extdvobjs_py_SOURCES
    test_extdvobjs_py.cpp
    TestExtDVObj.cpp
    helper.cpp
)

set(test_extdvobjs_py_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_extdvobjs_py)
PURC_FRAMEWORK(test_extdvobjs_py)
GTEST_DISCOVER_TESTS(test_extdvobjs_py DISCOVERY_TIMEOUT 10)

