include(PurCCommon)
include(target/PurC)
include(GoogleTest)

enable_testing()

# test_variant
PURC_EXECUTABLE_DECLARE(test_variant)

list(APPEND test_variant_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_variant)

set(test_variant_SOURCES
    test_variant.cpp
)

set(test_variant_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_variant)
PURC_FRAMEWORK(test_variant)
GTEST_DISCOVER_TESTS(test_variant DISCOVERY_TIMEOUT 10)

# test_serializer
PURC_EXECUTABLE_DECLARE(test_serializer)

list(APPEND test_serializer_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_serializer)

set(test_serializer_SOURCES
    test_serializer.cpp
)

set(test_serializer_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_serializer)
PURC_FRAMEWORK(test_serializer)
GTEST_DISCOVER_TESTS(test_serializer DISCOVERY_TIMEOUT 10)

# test_variant_array
PURC_EXECUTABLE_DECLARE(test_variant_array)

list(APPEND test_variant_array_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_variant_array)

set(test_variant_array_SOURCES
    test_variant_array.cpp
)

set(test_variant_array_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_variant_array)
PURC_FRAMEWORK(test_variant_array)
GTEST_DISCOVER_TESTS(test_variant_array DISCOVERY_TIMEOUT 10)

# test_variant_object
PURC_EXECUTABLE_DECLARE(test_variant_object)

list(APPEND test_variant_object_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_variant_object)

set(test_variant_object_SOURCES
    test_variant_object.cpp
)

set(test_variant_object_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_variant_object)
PURC_FRAMEWORK(test_variant_object)
GTEST_DISCOVER_TESTS(test_variant_object DISCOVERY_TIMEOUT 10)

# test_variant_set
PURC_EXECUTABLE_DECLARE(test_variant_set)

list(APPEND test_variant_set_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_variant_set)

set(test_variant_set_SOURCES
    test_variant_set.cpp
)

set(test_variant_set_LIBRARIES
    PurC::PurC
    "${GTEST_INCLUDE_DIRS}"
    pthread
)

PURC_COMPUTE_SOURCES(test_variant_set)
PURC_FRAMEWORK(test_variant_set)
GTEST_DISCOVER_TESTS(test_variant_set DISCOVERY_TIMEOUT 10)

# test_random
PURC_EXECUTABLE_DECLARE(test_random)

list(APPEND test_random_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_random)

set(test_random_SOURCES
    test_random.cpp
)

set(test_random_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_random)
PURC_FRAMEWORK(test_random)
GTEST_DISCOVER_TESTS(test_random DISCOVERY_TIMEOUT 10)

# test_load_from_json
PURC_EXECUTABLE_DECLARE(test_load_from_json)

list(APPEND test_load_from_json_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_load_from_json)

set(test_load_from_json_SOURCES
    test_load_from_json.cpp
)

set(test_load_from_json_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_load_from_json)
PURC_FRAMEWORK(test_load_from_json)
GTEST_DISCOVER_TESTS(test_load_from_json DISCOVERY_TIMEOUT 10)

# test_numerify_booleanize_stringify
PURC_EXECUTABLE_DECLARE(test_numerify_booleanize_stringify)

list(APPEND test_numerify_booleanize_stringify_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_numerify_booleanize_stringify)

set(test_numerify_booleanize_stringify_SOURCES
    test_numerify_booleanize_stringify.cpp
)

set(test_numerify_booleanize_stringify_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_numerify_booleanize_stringify)
PURC_FRAMEWORK(test_numerify_booleanize_stringify)
GTEST_DISCOVER_TESTS(test_numerify_booleanize_stringify DISCOVERY_TIMEOUT 10)

# test_container
PURC_EXECUTABLE_DECLARE(test_container)

list(APPEND test_container_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${WTF_DIR}
    ${PURC_DIR}/hvml
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_container)

set(test_container_SOURCES
    test_container.cpp
)

set(test_container_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_container)
PURC_FRAMEWORK(test_container)
GTEST_DISCOVER_TESTS(test_container DISCOVERY_TIMEOUT 10)

# test_set_find
PURC_EXECUTABLE_DECLARE(test_set_find)

list(APPEND test_set_find_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_set_find)

set(test_set_find_SOURCES
    test_set_find.cpp
)

set(test_set_find_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_set_find)
PURC_FRAMEWORK(test_set_find)
GTEST_DISCOVER_TESTS(test_set_find DISCOVERY_TIMEOUT 10)

# test_constraint
PURC_EXECUTABLE_DECLARE(test_constraint)

list(APPEND test_constraint_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_constraint)

set(test_constraint_SOURCES
    test_constraint.cpp
)

set(test_constraint_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_constraint)
PURC_FRAMEWORK(test_constraint)
GTEST_DISCOVER_TESTS(test_constraint DISCOVERY_TIMEOUT 10)

# test_variant_map
PURC_EXECUTABLE_DECLARE(test_variant_map)

list(APPEND test_variant_map_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_variant_map)

set(test_variant_map_SOURCES
    test_variant_map.cpp
)

set(test_variant_map_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_variant_map)
PURC_FRAMEWORK(test_variant_map)
GTEST_DISCOVER_TESTS(test_variant_map DISCOVERY_TIMEOUT 10)

# test_bugs_json
PURC_EXECUTABLE_DECLARE(test_bugs_json)

list(APPEND test_bugs_json_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_bugs_json)

set(test_bugs_json_SOURCES
    test_bugs_json.cpp
)

set(test_bugs_json_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_bugs_json)
PURC_FRAMEWORK(test_bugs_json)
GTEST_DISCOVER_TESTS(test_bugs_json DISCOVERY_TIMEOUT 10)

# test_variant_tuple
PURC_EXECUTABLE_DECLARE(test_variant_tuple)

list(APPEND test_variant_tuple_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_variant_tuple)

set(test_variant_tuple_SOURCES
    test_variant_tuple.cpp
)

set(test_variant_tuple_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_variant_tuple)
PURC_FRAMEWORK(test_variant_tuple)
GTEST_DISCOVER_TESTS(test_variant_tuple DISCOVERY_TIMEOUT 10)

# test_variant_sorted_array
PURC_EXECUTABLE_DECLARE(test_variant_sorted_array)

list(APPEND test_variant_sorted_array_PRIVATE_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${GTEST_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(test_variant_sorted_array)

set(test_variant_sorted_array_SOURCES
    test_variant_sorted_array.cpp
)

set(test_variant_sorted_array_LIBRARIES
    PurC::PurC
    "${GTEST_LIBRARIES}"
    pthread
)

PURC_COMPUTE_SOURCES(test_variant_sorted_array)
PURC_FRAMEWORK(test_variant_sorted_array)
GTEST_DISCOVER_TESTS(test_variant_sorted_array DISCOVERY_TIMEOUT 10)
