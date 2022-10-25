include(PurCCommon)
set(MATH_LIBRARY_TYPE SHARED)

set(MATH_SOURCES
    math.c
    parsers/math_tab.c
    parsers/math_l_tab.c
)

macro(MATH_FLEX_BISON _target _suffix _relpath _parser)
    BISON_TARGET(${_parser}${_suffix}_Parser
        ${CMAKE_CURRENT_SOURCE_DIR}/parsers/math.y
        ${CMAKE_CURRENT_BINARY_DIR}/${_parser}${_suffix}.tab.c
        COMPILE_FLAGS "-Dapi.prefix={${_parser}${_suffix}_yy}")
    FLEX_TARGET(${_parser}${_suffix}_Scanner
        ${CMAKE_CURRENT_SOURCE_DIR}/parsers/math.l
        ${CMAKE_CURRENT_BINARY_DIR}/${_parser}${_suffix}.lex.c
        COMPILE_FLAGS "--prefix=${_parser}${_suffix}_yy --header-file=${CMAKE_CURRENT_BINARY_DIR}/${_parser}${_suffix}.lex.h")
    ADD_FLEX_BISON_DEPENDENCY(${_parser}${_suffix}_Scanner ${_parser}${_suffix}_Parser)
    list(APPEND ${_target}_SOURCES
            ${BISON_${_parser}${_suffix}_Parser_OUTPUTS}
            ${FLEX_${_parser}${_suffix}_Scanner_OUTPUTS})
    set(_flags "-DM_${_parser}${_suffix}")
    set_source_files_properties(${BISON_${_parser}${_suffix}_Parser_OUTPUTS}
        PROPERTIES COMPILE_FLAGS "${_flags}"
    )
    set_source_files_properties(${FLEX_${_parser}${_suffix}_Scanner_OUTPUTS}
        PROPERTIES COMPILE_FLAGS "${_flags}"
    )
    unset(_flags)
endmacro()

set(_parsers math)
PURC_PARSERS_GEN_IN(parsers "${_parsers}" "_l")
unset(_parsers)

set(MATH_PRIVATE_INCLUDE_DIRECTORIES
    "${FORWARDING_HEADERS_DIR}/purc"
    "${PURC_DIR}/include"
    "${PurC_DERIVED_SOURCES_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

set(MATH_LIBRARIES
    ${CMAKE_DL_LIBS}
)

link_directories (${CMAKE_BINARY_DIR}/Source/PurC)
list(APPEND MATH_LIBRARIES PurC::PurC m)

PURC_FRAMEWORK_DECLARE(MATH)
PURC_INCLUDE_CONFIG_FILES_IF_EXISTS()

set_target_properties(MATH PROPERTIES OUTPUT_NAME "purc-dvobj-MATH")
set_target_properties(MATH PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

PURC_WRAP_SOURCELIST(${MATH_SOURCES})
PURC_FRAMEWORK(MATH)

install(TARGETS MATH DESTINATION "${LIB_INSTALL_DIR}/purc-${PURC_API_VERSION}")

if (MSVC)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif ()

