include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_variant
PURC_EXECUTABLE_DECLARE(test_variant)

list(APPEND test_variant_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_variant)

set(test_variant_SOURCES
    test_variant.cpp
)

set(test_variant_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_variant)
PURC_FRAMEWORK(test_variant)
GTEST_DISCOVER_TESTS(test_variant)

# test_serializer
PURC_EXECUTABLE_DECLARE(test_serializer)

list(APPEND test_serializer_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_serializer)

set(test_serializer_SOURCES
    test_serializer.cpp
)

set(test_serializer_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_serializer)
PURC_FRAMEWORK(test_serializer)
GTEST_DISCOVER_TESTS(test_serializer)

# test_variant_array
PURC_EXECUTABLE_DECLARE(test_variant_array)

list(APPEND test_variant_array_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_variant_array)

set(test_variant_array_SOURCES
    test_variant_array.cpp
)

set(test_variant_array_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_variant_array)
PURC_FRAMEWORK(test_variant_array)
GTEST_DISCOVER_TESTS(test_variant_array)

# test_variant_object
PURC_EXECUTABLE_DECLARE(test_variant_object)

list(APPEND test_variant_object_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_variant_object)

set(test_variant_object_SOURCES
    test_variant_object.cpp
)

set(test_variant_object_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_variant_object)
PURC_FRAMEWORK(test_variant_object)
GTEST_DISCOVER_TESTS(test_variant_object)

# test_variant_set
PURC_EXECUTABLE_DECLARE(test_variant_set)

list(APPEND test_variant_set_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_variant_set)

set(test_variant_set_SOURCES
    test_variant_set.cpp
)

set(test_variant_set_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_variant_set)
PURC_FRAMEWORK(test_variant_set)
GTEST_DISCOVER_TESTS(test_variant_set)

