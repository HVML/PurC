include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_dvobjs
PURC_EXECUTABLE_DECLARE(test_dvobjs)

list(APPEND test_dvobjs_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_dvobjs)

set(test_dvobjs_SOURCES
    test_dvobjs.cpp
    test_helper.cpp
)

set(test_dvobjs_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dvobjs)
PURC_FRAMEWORK(test_dvobjs)
GTEST_DISCOVER_TESTS(test_dvobjs DISCOVERY_TIMEOUT 10)


# test_math
PURC_EXECUTABLE_DECLARE(test_math)

list(APPEND test_math_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_math)

set(test_math_SOURCES
    test_math.cpp
    test_helper.cpp
)

set(test_math_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_math)
PURC_FRAMEWORK(test_math)
GTEST_DISCOVER_TESTS(test_math DISCOVERY_TIMEOUT 10)


# test_logical
PURC_EXECUTABLE_DECLARE(test_logical)

list(APPEND test_logical_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_logical)

set(test_logical_SOURCES
    test_logical.cpp
    test_helper.cpp
)

set(test_logical_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_logical)
PURC_FRAMEWORK(test_logical)
GTEST_DISCOVER_TESTS(test_logical DISCOVERY_TIMEOUT 10)


# test_string
PURC_EXECUTABLE_DECLARE(test_string)

list(APPEND test_string_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_string)

set(test_string_SOURCES
    test_string.cpp
    test_helper.cpp
)

set(test_string_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_string)
PURC_FRAMEWORK(test_string)
GTEST_DISCOVER_TESTS(test_string DISCOVERY_TIMEOUT 10)


# test_fs
PURC_EXECUTABLE_DECLARE(test_fs)

list(APPEND test_fs_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_fs)

set(test_fs_SOURCES
    test_fs.cpp
    test_helper.cpp
)

set(test_fs_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_fs)
PURC_FRAMEWORK(test_fs)
GTEST_DISCOVER_TESTS(test_fs DISCOVERY_TIMEOUT 10)


# test_file
PURC_EXECUTABLE_DECLARE(test_file)

list(APPEND test_file_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_file)

set(test_file_SOURCES
    test_file.cpp
    test_helper.cpp
)

set(test_file_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_file)
PURC_FRAMEWORK(test_file)
GTEST_DISCOVER_TESTS(test_file DISCOVERY_TIMEOUT 10)



# test_ejson
PURC_EXECUTABLE_DECLARE(test_dvobjejson)

list(APPEND test_dvobjejson_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_dvobjejson)

set(test_dvobjejson_SOURCES
    test_ejson.cpp
    test_helper.cpp
)

set(test_dvobjejson_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dvobjejson)
PURC_FRAMEWORK(test_dvobjejson)
GTEST_DISCOVER_TESTS(test_dvobjejson DISCOVERY_TIMEOUT 10)


# test_dvobjhvml
PURC_EXECUTABLE_DECLARE(test_dvobjhvml)

list(APPEND test_dvobjhvml_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_dvobjhvml)

set(test_dvobjhvml_SOURCES
    test_hvml.cpp
    test_helper.cpp
)

set(test_dvobjhvml_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_dvobjhvml)
PURC_FRAMEWORK(test_dvobjhvml)
GTEST_DISCOVER_TESTS(test_dvobjhvml DISCOVERY_TIMEOUT 10)
