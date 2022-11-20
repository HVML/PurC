# The settings in this file are the PurC project default values, and
# are recommended for most ports. Ports can override these settings in
# Options*.cmake, but should do so only if there is strong reason to
# deviate from the defaults of the PurC project (e.g. if the feature
# requires platform-specific implementation that does not exist).

set(_PURC_AVAILABLE_OPTIONS "")

set(PUBLIC YES)
set(PRIVATE NO)

macro(_ENSURE_OPTION_MODIFICATION_IS_ALLOWED)
    if (NOT _SETTING_PURC_OPTIONS)
        message(FATAL_ERROR "Options must be set between PURC_OPTION_BEGIN and PURC_OPTION_END")
    endif ()
endmacro()

macro(_ENSURE_IS_PURC_OPTION _name)
    list(FIND _PURC_AVAILABLE_OPTIONS ${_name} ${_name}_OPTION_INDEX)
    if (${_name}_OPTION_INDEX EQUAL -1)
        message(FATAL_ERROR "${_name} is not a valid PurC option")
    endif ()
endmacro()

macro(PURC_OPTION_DEFINE _name _description _public _initial_value)
    _ENSURE_OPTION_MODIFICATION_IS_ALLOWED()

    set(_PURC_AVAILABLE_OPTIONS_DESCRIPTION_${_name} ${_description})
    set(_PURC_AVAILABLE_OPTIONS_IS_PUBLIC_${_name} ${_public})
    set(_PURC_AVAILABLE_OPTIONS_INITIAL_VALUE_${_name} ${_initial_value})
    set(_PURC_AVAILABLE_OPTIONS_${_name}_CONFLICTS "")
    set(_PURC_AVAILABLE_OPTIONS_${_name}_DEPENDENCIES "")
    list(APPEND _PURC_AVAILABLE_OPTIONS ${_name})

    EXPOSE_VARIABLE_TO_BUILD(${_name})
endmacro()

macro(PURC_OPTION_DEFAULT_PORT_VALUE _name _public _value)
    _ENSURE_OPTION_MODIFICATION_IS_ALLOWED()
    _ENSURE_IS_PURC_OPTION(${_name})

    set(_PURC_AVAILABLE_OPTIONS_IS_PUBLIC_${_name} ${_public})
    set(_PURC_AVAILABLE_OPTIONS_INITIAL_VALUE_${_name} ${_value})
endmacro()

macro(PURC_OPTION_CONFLICT _name _conflict)
    _ENSURE_OPTION_MODIFICATION_IS_ALLOWED()
    _ENSURE_IS_PURC_OPTION(${_name})
    _ENSURE_IS_PURC_OPTION(${_conflict})

    list(APPEND _PURC_AVAILABLE_OPTIONS_${_name}_CONFLICTS ${_conflict})
endmacro()

macro(PURC_OPTION_DEPEND _name _depend)
    _ENSURE_OPTION_MODIFICATION_IS_ALLOWED()
    _ENSURE_IS_PURC_OPTION(${_name})
    _ENSURE_IS_PURC_OPTION(${_depend})

    list(APPEND _PURC_AVAILABLE_OPTIONS_${_name}_DEPENDENCIES ${_depend})
endmacro()

macro(PURC_OPTION_BEGIN)
    set(_SETTING_PURC_OPTIONS TRUE)

    if (WTF_CPU_ARM64 OR WTF_CPU_X86_64)
        set(USE_SYSTEM_MALLOC_DEFAULT OFF)
    elseif (WTF_CPU_ARM AND WTF_OS_LINUX AND ARM_THUMB2_DETECTED)
        set(USE_SYSTEM_MALLOC_DEFAULT OFF)
    elseif (WTF_CPU_MIPS AND WTF_OS_LINUX)
        set(USE_SYSTEM_MALLOC_DEFAULT OFF)
    else ()
        set(USE_SYSTEM_MALLOC_DEFAULT ON)
    endif ()

    set(USE_SYSTEM_MALLOC_DEFAULT ON)

    if (DEFINED ClangTidy_EXE OR DEFINED IWYU_EXE)
        message(STATUS "Unified builds are disabled when analyzing sources")
        set(ENABLE_UNIFIED_BUILDS_DEFAULT OFF)
    else ()
        set(ENABLE_UNIFIED_BUILDS_DEFAULT ON)
    endif ()

    PURC_OPTION_DEFINE(ENABLE_CHINESE_NAMES "Toggle support for variable and key names in Chinese (TEST only)" PUBLIC OFF)
    PURC_OPTION_DEFINE(ENABLE_SOCKET_STREAM "Toggle socket stream" PUBLIC ON)
    PURC_OPTION_DEFINE(ENABLE_HTML "Toggle HTML parser" PUBLIC ON)
    PURC_OPTION_DEFINE(ENABLE_XGML "Toggle XGML parser" PUBLIC ON)
    PURC_OPTION_DEFINE(ENABLE_XML "Toggle XML parser" PUBLIC ON)
    PURC_OPTION_DEFINE(ENABLE_REMOTE_FETCHER "Toggle use of the remote PurC Fetcher" PUBLIC ON)
    PURC_OPTION_DEFINE(ENABLE_RENDERER_THREAD "Toggle THREAD renderer" PUBLIC ON)
    PURC_OPTION_DEFINE(ENABLE_RENDERER_SOCKET "Toggle SOCKET renderer" PUBLIC ON)
    PURC_OPTION_DEFINE(ENABLE_RENDERER_HIBUS "Toggle HIBUS renderer" PUBLIC ON)
    PURC_OPTION_DEFINE(ENABLE_HIBUS "Toggle support for hiBus protocol" PUBLIC ON)
    PURC_OPTION_DEFINE(ENABLE_MQTT "Toggle support for MQTT protocol" PUBLIC ON)
    PURC_OPTION_DEFINE(ENABLE_WEB_SOCKET "Toggle support for WebSocket protocol" PUBLIC ON)
    PURC_OPTION_DEFINE(ENABLE_SSL "Toggle support for SSL" PUBLIC OFF)
    PURC_OPTION_DEFINE(ENABLE_API_TESTS "Enable public API unit tests" PUBLIC ON)
    PURC_OPTION_DEFINE(ENABLE_DEVELOPER_MODE "Toggle developer mode" PUBLIC OFF)
    PURC_OPTION_DEFINE(ENABLE_RDR_FOIL "Toggle the built-in `foil` renderer in `purc`" PUBLIC ON)

    PURC_OPTION_DEFINE(USE_SYSTEM_MALLOC "Toggle system allocator instead of PurC's custom allocator" PUBLIC ${USE_SYSTEM_MALLOC_DEFAULT})
    PURC_OPTION_DEFINE(ENABLE_ICU "Enable icu" PUBLIC OFF)

    PURC_OPTION_DEFINE(ENABLE_LCMD "Toggle support for LCMD protocol" PUBLIC ON)
    PURC_OPTION_DEFINE(ENABLE_LSQL "Toggle support for LSQL protocol" PUBLIC ON)
    PURC_OPTION_DEFINE(ENABLE_RSQL "Toggle support for RSQL protocol" PUBLIC ON)
    PURC_OPTION_DEFINE(ENABLE_HTTP "Toggle support for HTTP protocol" PUBLIC ON)
#PURC_OPTION_DEPEND(ENABLE_XSLT ENABLE_XML)
endmacro()

macro(_PURC_OPTION_ENFORCE_DEPENDS _name)
    foreach (_dependency ${_PURC_AVAILABLE_OPTIONS_${_name}_DEPENDENCIES})
        if (NOT ${_dependency})
            message(STATUS "Disabling ${_name} since ${_dependency} is disabled.")
            set(${_name} OFF)
            set(_OPTION_CHANGED TRUE)
            break ()
        endif ()
    endforeach ()
endmacro()

macro(_PURC_OPTION_ENFORCE_ALL_DEPENDS)
    set(_OPTION_CHANGED TRUE)
    while (${_OPTION_CHANGED})
        set(_OPTION_CHANGED FALSE)
        foreach (_name ${_PURC_AVAILABLE_OPTIONS})
            if (${_name})
                _PURC_OPTION_ENFORCE_DEPENDS(${_name})
            endif ()
        endforeach ()
    endwhile ()
endmacro()

macro(_PURC_OPTION_ENFORCE_CONFLICTS _name)
    foreach (_conflict ${_PURC_AVAILABLE_OPTIONS_${_name}_CONFLICTS})
        if (${_conflict})
            message(FATAL_ERROR "${_name} conflicts with ${_conflict}. You must disable one or the other.")
        endif ()
    endforeach ()
endmacro()

macro(_PURC_OPTION_ENFORCE_ALL_CONFLICTS)
    foreach (_name ${_PURC_AVAILABLE_OPTIONS})
        if (${_name})
            _PURC_OPTION_ENFORCE_CONFLICTS(${_name})
        endif ()
    endforeach ()
endmacro()

macro(PURC_OPTION_END)
    set(_SETTING_PURC_OPTIONS FALSE)

    list(SORT _PURC_AVAILABLE_OPTIONS)
    set(_MAX_FEATURE_LENGTH 0)
    foreach (_name ${_PURC_AVAILABLE_OPTIONS})
        string(LENGTH ${_name} _name_length)
        if (_name_length GREATER _MAX_FEATURE_LENGTH)
            set(_MAX_FEATURE_LENGTH ${_name_length})
        endif ()

        option(${_name} "${_PURC_AVAILABLE_OPTIONS_DESCRIPTION_${_name}}" ${_PURC_AVAILABLE_OPTIONS_INITIAL_VALUE_${_name}})
        if (NOT ${_PURC_AVAILABLE_OPTIONS_IS_PUBLIC_${_name}})
            mark_as_advanced(FORCE ${_name})
        endif ()
    endforeach ()

    # Run through every possible depends to make sure we have disabled anything
    # that could cause an unnecessary conflict before processing conflicts.
    _PURC_OPTION_ENFORCE_ALL_DEPENDS()
    _PURC_OPTION_ENFORCE_ALL_CONFLICTS()

    foreach (_name ${_PURC_AVAILABLE_OPTIONS})
        if (${_name})
            list(APPEND FEATURE_DEFINES ${_name})
            set(FEATURE_DEFINES_WITH_SPACE_SEPARATOR "${FEATURE_DEFINES_WITH_SPACE_SEPARATOR} ${_name}")
        endif ()
    endforeach ()
endmacro()

macro(PRINT_PURC_OPTIONS)
    message(STATUS "Enabled features:")

    set(_should_print_dots ON)
    foreach (_name ${_PURC_AVAILABLE_OPTIONS})
        if (${_PURC_AVAILABLE_OPTIONS_IS_PUBLIC_${_name}})
            string(LENGTH ${_name} _name_length)
            set(_message " ${_name} ")

            # Print dots on every other row, for readability.
            foreach (IGNORE RANGE ${_name_length} ${_MAX_FEATURE_LENGTH})
                if (${_should_print_dots})
                    set(_message "${_message}.")
                else ()
                    set(_message "${_message} ")
                endif ()
            endforeach ()

            set(_should_print_dots (NOT ${_should_print_dots}))

            set(_message "${_message} ${${_name}}")
            message(STATUS "${_message}")
        endif ()
    endforeach ()
endmacro()

set(_PURC_CONFIG_FILE_VARIABLES "")

macro(EXPOSE_VARIABLE_TO_BUILD _variable_name)
    list(APPEND _PURC_CONFIG_FILE_VARIABLES ${_variable_name})
endmacro()

macro(SET_AND_EXPOSE_TO_BUILD _variable_name)
    # It's important to handle the case where the value isn't passed, because often
    # during configuration an empty variable is the result of a failed package search.
    if (${ARGC} GREATER 1)
        set(_variable_value ${ARGV1})
    else ()
        set(_variable_value OFF)
    endif ()

    set(${_variable_name} ${_variable_value})
    EXPOSE_VARIABLE_TO_BUILD(${_variable_name})
endmacro()

macro(_ADD_CONFIGURATION_LINE_TO_HEADER_STRING _string _variable_name _output_variable_name)
    if (${${_variable_name}})
        set(${_string} "${_file_contents}#define ${_output_variable_name} 1\n")
    else ()
        set(${_string} "${_file_contents}#define ${_output_variable_name} 0\n")
    endif ()
endmacro()

macro(CREATE_CONFIGURATION_HEADER)
    list(SORT _PURC_CONFIG_FILE_VARIABLES)
    set(_file_contents "#ifndef CMAKECONFIG_H\n")
    set(_file_contents "${_file_contents}#define CMAKECONFIG_H\n\n")

    foreach (_variable_name ${_PURC_CONFIG_FILE_VARIABLES})
        _ADD_CONFIGURATION_LINE_TO_HEADER_STRING(_file_contents ${_variable_name} ${_variable_name})
    endforeach ()
    set(_file_contents "${_file_contents}\n#endif /* CMAKECONFIG_H */\n")

    file(WRITE "${CMAKE_BINARY_DIR}/cmakeconfig.h.tmp" "${_file_contents}")
    execute_process(COMMAND ${CMAKE_COMMAND}
        -E copy_if_different
        "${CMAKE_BINARY_DIR}/cmakeconfig.h.tmp"
        "${CMAKE_BINARY_DIR}/cmakeconfig.h"
    )
    file(REMOVE "${CMAKE_BINARY_DIR}/cmakeconfig.h.tmp")
endmacro()

macro(PURC_CHECK_HAVE_INCLUDE _variable _header)
    check_include_file(${_header} ${_variable}_value)
    SET_AND_EXPOSE_TO_BUILD(${_variable} ${${_variable}_value})
endmacro()

macro(PURC_CHECK_HAVE_FUNCTION _variable _function)
    check_function_exists(${_function} ${_variable}_value)
    SET_AND_EXPOSE_TO_BUILD(${_variable} ${${_variable}_value})
endmacro()

macro(PURC_CHECK_HAVE_SYMBOL _variable _symbol _header)
    check_symbol_exists(${_symbol} "${_header}" ${_variable}_value)
    SET_AND_EXPOSE_TO_BUILD(${_variable} ${${_variable}_value})
endmacro()

macro(PURC_CHECK_HAVE_STRUCT _variable _struct _member _header)
    check_struct_has_member(${_struct} ${_member} "${_header}" ${_variable}_value)
    SET_AND_EXPOSE_TO_BUILD(${_variable} ${${_variable}_value})
endmacro()

macro(PURC_CHECK_SOURCE_COMPILES _variable _source)
    check_cxx_source_compiles("${_source}" ${_variable}_value)
    SET_AND_EXPOSE_TO_BUILD(${_variable} ${${_variable}_value})
endmacro()

option(ENABLE_EXPERIMENTAL_FEATURES "Enable experimental features" OFF)
SET_AND_EXPOSE_TO_BUILD(ENABLE_EXPERIMENTAL_FEATURES ${ENABLE_EXPERIMENTAL_FEATURES})
