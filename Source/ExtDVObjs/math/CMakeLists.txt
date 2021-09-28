include(PurCCommon)

list(APPEND purc-dvobj-MATH_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../math
)

set(purc-dvobj-MATH_SOURCES
    math.c
)

set(math_PARSERS math)
PURC_FLEX_BISON(purc-dvobj-MATH parsers "${math_PARSERS}")
unset(math_PARSERS)

add_library(purc-dvobj-MATH SHARED
    "${purc-dvobj-MATH_SOURCES}")

target_include_directories(purc-dvobj-MATH PRIVATE
    "${purc-dvobj-MATH_PRIVATE_INCLUDE_DIRECTORIES}")
target_link_libraries(purc-dvobj-MATH PurC)

install(TARGETS purc-dvobj-MATH DESTINATION "/usr/lib/purc-${PURC_API_VERSION}")
