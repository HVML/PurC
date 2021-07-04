include(GNUInstallDirs)
include(VersioningUtils)

PURC_OPTION_BEGIN()

SET_PROJECT_VERSION(0 0 1)

set(PURC_API_VERSION 0.0)

CALCULATE_LIBRARY_VERSIONS_FROM_LIBTOOL_TRIPLE(PURC 0 0 0)

# These are shared variables, but we special case their definition so that we can use the
# CMAKE_INSTALL_* variables that are populated by the GNUInstallDirs macro.
set(LIB_INSTALL_DIR "${CMAKE_INSTALL_FULL_LIBDIR}" CACHE PATH "Absolute path to library installation directory")
set(EXEC_INSTALL_DIR "${CMAKE_INSTALL_FULL_BINDIR}" CACHE PATH "Absolute path to executable installation directory")
set(LIBEXEC_INSTALL_DIR "${CMAKE_INSTALL_FULL_LIBEXECDIR}/purc" CACHE PATH "Absolute path to install executables executed by the library")
set(HEADER_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}" CACHE PATH "Absolute path to header installation directory")
set(PURC_HEADER_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}/purc" CACHE PATH "Absolute path to PurC header installation directory")

add_definitions(-DBUILDING_HBD__=1)
add_definitions(-DPURC_API_VERSION_STRING="${PURC_API_VERSION}")

find_package(HiBox REQUIRED)
find_package(GLIB 2.44.0 COMPONENTS gio gio-unix)
find_package(HiBus)
find_package(LibXml2 2.8.0)
find_package(LibSoup 2.54.0)
find_package(SQLite3)
find_package(MySQLClient)
find_package(ZLIB)

set(ENABLE_SOCKET_DEFAULT ON)
if (NOT GLIB_FOUND)
    set(ENABLE_SOCKET_DEFAULT OFF)
endif ()

set(ENABLE_XML_DEFAULT ON)
if (NOT LIBXML2_FOUND)
    set(ENABLE_XML_DEFAULT OFF)
endif ()

set(ENABLE_HTTP_DEFAULT ON)
if (NOT LIBSOUP_FOUND)
    set(ENABLE_HTTP_DEFAULT OFF)
endif ()

set(ENABLE_LSQL_DEFAULT ON)
if (NOT SQLITE3_FOUND)
    set(ENABLE_LSQL_DEFAULT OFF)
endif ()

set(ENABLE_RSQL_DEFAULT ON)
if (NOT MYSQLCLIENT_FOUND)
    set(ENABLE_RSQL_DEFAULT OFF)
endif ()

set(ENABLE_HIBUS_DEFAULT ON)
if (NOT HIBUS_FOUND)
    set(ENABLE_HIBUS_DEFAULT OFF)
endif ()

# Public options specific to the HybridOS port. Do not add any options here unless
# there is a strong reason we should support changing the value of the option,
# and the option is not relevant to any other PurC ports.
#PURC_OPTION_DEFINE(USE_SYSTEMD "Whether to enable journald logging" PUBLIC ON)

# Private options specific to the HybridOS port. Changing these options is
# completely unsupported. They are intended for use only by PurC developers.
#PURC_OPTION_DEFINE(USE_ANGLE_WEBGL "Whether to use ANGLE as WebGL backend." PRIVATE OFF)
#PURC_OPTION_DEPEND(ENABLE_WEBGL ENABLE_GRAPHICS_CONTEXT_GL)

PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_SOCKET PUBLIC ${ENABLE_SOCKET_DEFAULT})
PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_XML PUBLIC ${ENABLE_XML_DEFAULT})
PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_HTTP PUBLIC ${ENABLE_HTTP_DEFAULT})
PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_LSQL PUBLIC ${ENABLE_LSQL_DEFAULT})
PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_RSQL PUBLIC ${ENABLE_RSQL_DEFAULT})
PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_HIBUS PUBLIC ${ENABLE_HIBUS_DEFAULT})

# Finalize the value for all options. Do not attempt to use an option before
# this point, and do not attempt to change any option after this point.
PURC_OPTION_END()

if (USE_LIBSECRET)
    find_package(Libsecret)
    if (NOT LIBSECRET_FOUND)
        message(FATAL_ERROR "libsecret is needed for USE_LIBSECRET")
    endif ()
endif ()

set(PurC_PKGCONFIG_FILE ${CMAKE_BINARY_DIR}/Source/PurC/purc.pc)

# CMake does not automatically add --whole-archive when building shared objects from
# a list of convenience libraries. This can lead to missing symbols in the final output.
# We add --whole-archive to all libraries manually to prevent the linker from trimming
# symbols that we actually need later. With ld64 on darwin, we use -all_load instead.
macro(ADD_WHOLE_ARCHIVE_TO_LIBRARIES _list_name)
    if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
        list(APPEND ${_list_name} -Wl,-all_load)
    else ()
        set(_tmp)
        foreach (item IN LISTS ${_list_name})
            if ("${item}" STREQUAL "PRIVATE" OR "${item}" STREQUAL "PUBLIC")
                list(APPEND _tmp "${item}")
            else ()
                list(APPEND _tmp -Wl,--whole-archive "${item}" -Wl,--no-whole-archive)
            endif ()
        endforeach ()
        set(${_list_name} ${_tmp})
    endif ()
endmacro()

#include(BubblewrapSandboxChecks)
