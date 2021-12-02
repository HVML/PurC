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
GTEST_DISCOVER_TESTS(test_variant DISCOVERY_TIMEOUT 10)

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
GTEST_DISCOVER_TESTS(test_serializer DISCOVERY_TIMEOUT 10)

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
GTEST_DISCOVER_TESTS(test_variant_array DISCOVERY_TIMEOUT 10)

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
GTEST_DISCOVER_TESTS(test_variant_object DISCOVERY_TIMEOUT 10)

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
GTEST_DISCOVER_TESTS(test_variant_set DISCOVERY_TIMEOUT 10)

# test_random
PURC_EXECUTABLE_DECLARE(test_random)

list(APPEND test_random_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_random)

set(test_random_SOURCES
    test_random.cpp
)

set(test_random_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_random)
PURC_FRAMEWORK(test_random)
GTEST_DISCOVER_TESTS(test_random DISCOVERY_TIMEOUT 10)

# test_load_from_json
PURC_EXECUTABLE_DECLARE(test_load_from_json)

list(APPEND test_load_from_json_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_load_from_json)

set(test_load_from_json_SOURCES
    test_load_from_json.cpp
)

set(test_load_from_json_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_load_from_json)
PURC_FRAMEWORK(test_load_from_json)
GTEST_DISCOVER_TESTS(test_load_from_json DISCOVERY_TIMEOUT 10)

# test_numberify_booleanize_stringify
PURC_EXECUTABLE_DECLARE(test_numberify_booleanize_stringify)

list(APPEND test_numberify_booleanize_stringify_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
)

PURC_EXECUTABLE(test_numberify_booleanize_stringify)

set(test_numberify_booleanize_stringify_SOURCES
    test_numberify_booleanize_stringify.cpp
)

set(test_numberify_booleanize_stringify_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(test_numberify_booleanize_stringify)
PURC_FRAMEWORK(test_numberify_booleanize_stringify)
GTEST_DISCOVER_TESTS(test_numberify_booleanize_stringify DISCOVERY_TIMEOUT 10)

