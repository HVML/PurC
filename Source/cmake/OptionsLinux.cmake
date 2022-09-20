include(GNUInstallDirs)

PURC_OPTION_BEGIN()

CALCULATE_LIBRARY_VERSIONS_FROM_LIBTOOL_TRIPLE(PURC 0 0 0)

# These are shared variables, but we special case their definition so that we can use the
# CMAKE_INSTALL_* variables that are populated by the GNUInstallDirs macro.
set(LIB_INSTALL_DIR "${CMAKE_INSTALL_FULL_LIBDIR}" CACHE PATH "Absolute path to library installation directory")
set(EXEC_INSTALL_DIR "${CMAKE_INSTALL_FULL_BINDIR}" CACHE PATH "Absolute path to executable installation directory")
set(LIBEXEC_INSTALL_DIR "${CMAKE_INSTALL_FULL_LIBEXECDIR}/purc" CACHE PATH "Absolute path to install executables executed by the library")
set(HEADER_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}" CACHE PATH "Absolute path to header installation directory")
set(PURC_HEADER_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}/purc" CACHE PATH "Absolute path to PurC header installation directory")

add_definitions(-DBUILDING_LINUX__=1)
# add_definitions(-DPURC_API_VERSION_STRING="${PURC_API_VERSION}")
add_definitions(-DPURC_LIBEXEC_DIR="${LIBEXEC_INSTALL_DIR}")

find_package(ZLIB 1.2.0 REQUIRED)
find_package(GLIB 2.44.0 REQUIRED COMPONENTS gio gio-unix gmodule gobject)
find_package(Threads REQUIRED)
find_package(BISON 3.0 REQUIRED)
find_package(FLEX 2.6.4 REQUIRED)
find_package(Ncurses 5.0)
find_package(CSSEng 0.9.1)
find_package(LibXml2 2.8.0)
find_package(HiBus 100)
find_package(OpenSSL 1.1.1)
#find_package(LibSoup 2.54.0)
#find_package(CURL 7.60.0)
#find_package(SQLite3 3.10.0)
#find_package(MySQLClient 20.0.0)

if (NOT GLIB_FOUND)
    set(ENABLE_SOCKET_STREAM_DEFAULT OFF)
    SET_AND_EXPOSE_TO_BUILD(HAVE_GLIB OFF)
else ()
    set(ENABLE_SOCKET_STREAM_DEFAULT ON)
    SET_AND_EXPOSE_TO_BUILD(HAVE_GLIB ON)
    if (${GLIB_VERSION} VERSION_LESS 2.70)
        SET_AND_EXPOSE_TO_BUILD(HAVE_GLIB_LESS_2_70 ON)
    else ()
        SET_AND_EXPOSE_TO_BUILD(HAVE_GLIB_LESS_2_70 OFF)
    endif ()
endif ()

if (NOT (Ncurses_FOUND AND CSSEng_FOUND))
    set(ENABLE_RDR_FOIL_DEFAULT OFF)
    SET_AND_EXPOSE_TO_BUILD(HAVE_NCURSES OFF)
else ()
    set(ENABLE_RDR_FOIL_DEFAULT ON)
    SET_AND_EXPOSE_TO_BUILD(HAVE_NCURSES ON)
endif ()

if (NOT LIBXML2_FOUND)
    set(ENABLE_XML_DEFAULT OFF)
    SET_AND_EXPOSE_TO_BUILD(HAVE_LIBXML2 OFF)
else ()
    set(ENABLE_XML_DEFAULT ON)
    SET_AND_EXPOSE_TO_BUILD(HAVE_LIBXML2 ON)
endif ()

if (NOT HIBUS_FOUND)
    set(ENABLE_HIBUS_DEFAULT OFF)
    SET_AND_EXPOSE_TO_BUILD(HAVE_HIBUS OFF)
else ()
    set(ENABLE_HIBUS_DEFAULT ON)
    SET_AND_EXPOSE_TO_BUILD(HAVE_HIBUS ON)
endif ()

if (NOT OPENSSL_FOUND)
    set(ENABLE_SSL_DEFAULT OFF)
    SET_AND_EXPOSE_TO_BUILD(HAVE_OPENSSL OFF)
else ()
    set(ENABLE_SSL_DEFAULT ON)
    SET_AND_EXPOSE_TO_BUILD(HAVE_OPENSSL ON)
endif ()

# Public options specific to the HybridOS port. Do not add any options here unless
# there is a strong reason we should support changing the value of the option,
# and the option is not relevant to any other PurC ports.
#PURC_OPTION_DEFINE(USE_SYSTEMD "Whether to enable journald logging" PUBLIC ON)

# Private options specific to the HybridOS port. Changing these options is
# completely unsupported. They are intended for use only by PurC developers.
#PURC_OPTION_DEFINE(USE_ANGLE_WEBGL "Whether to use ANGLE as WebGL backend." PRIVATE OFF)
#PURC_OPTION_DEPEND(ENABLE_WEBGL ENABLE_GRAPHICS_CONTEXT_GL)

PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_SOCKET_STREAM PUBLIC ${ENABLE_SOCKET_STREAM_DEFAULT})
PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_RDR_FOIL PUBLIC ${ENABLE_RDR_FOIL_DEFAULT})
PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_XML PUBLIC ${ENABLE_XML_DEFAULT})
PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_HIBUS PUBLIC ${ENABLE_HIBUS_DEFAULT})
PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_SSL PUBLIC ${ENABLE_SSL_DEFAULT})

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
