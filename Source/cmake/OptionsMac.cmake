include(GNUInstallDirs)

# FIXME: These should line up with versions in Version.xcconfig
set(PURC_MAC_VERSION 0.0.1)
set(MACOSX_FRAMEWORK_BUNDLE_VERSION 0.0.1)

# These are shared variables, but we special case their definition so that we can use the
# CMAKE_INSTALL_* variables that are populated by the GNUInstallDirs macro.
set(LIB_INSTALL_DIR "${CMAKE_INSTALL_FULL_LIBDIR}" CACHE PATH "Absolute path to library installation directory")
set(EXEC_INSTALL_DIR "${CMAKE_INSTALL_FULL_BINDIR}" CACHE PATH "Absolute path to executable installation directory")
set(LIBEXEC_INSTALL_DIR "${CMAKE_INSTALL_FULL_LIBEXECDIR}/purc" CACHE PATH "Absolute path to install executables executed by the library")
set(HEADER_INSTALL_DIR "${CMAKE_INSTALL_FULL_INCLUDEDIR}" CACHE PATH "Absolute path to header installation directory")
set(PURC_HEADER_INSTALL_DIR "${CMAKE_INSTALL_FULL_INCLUDEDIR}/purc" CACHE PATH "Absolute path to PurC header installation directory")

add_definitions(-DBUILDING_MAC__=1)
add_definitions(-DPURC_LIBEXEC_DIR="${LIBEXEC_INSTALL_DIR}")

find_package(ZLIB 1.2.0 REQUIRED)
find_package(GLIB 2.44.0 REQUIRED COMPONENTS gio gio-unix gmodule gobject)
find_package(BISON 3.0 REQUIRED)
find_package(FLEX 2.6.4 REQUIRED)
find_package(Ncurses 5.0)
find_package(LibXml2 2.8.0)
find_package(OpenSSL 1.1.1)
#find_package(LibXslt 1.1.7)
#find_package(CURL 7.60.0)
#find_package(SQLite3 3.10.0)

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

if (NOT Ncurses_FOUND)
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

PURC_OPTION_BEGIN()
# Private options shared with other PurC ports. Add options here only if
# we need a value different from the default defined in PurCFeatures.cmake.

PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_SOCKET_STREAM PUBLIC ${ENABLE_SOCKET_STREAM_DEFAULT})
PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_RDR_FOIL PUBLIC ${ENABLE_RDR_FOIL_DEFAULT})
PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_XML PUBLIC ${ENABLE_XML_DEFAULT})
PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_HIBUS PUBLIC ${ENABLE_HIBUS_DEFAULT})
PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_SSL PUBLIC ${ENABLE_SSL_DEFAULT})

PURC_OPTION_END()

set(PurC_PKGCONFIG_FILE ${CMAKE_BINARY_DIR}/Source/PurC/purc.pc)

set(PurC_LIBRARY_TYPE SHARED)
set(PurCTestSupport_LIBRARY_TYPE SHARED)

