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

find_package(GLIB 2.44.0 COMPONENTS gio gio-unix gmodule gobject)
find_package(Ncurses 5.0)
find_package(LibXml2 2.8.0)
find_package(LibXslt 1.1.7)
find_package(CURL 7.60.0)
find_package(OpenSSL 1.1.1)
find_package(SQLite3 3.10.0)

# On macOS, search Homebrew for keg-only versions of Bison and Flex. Xcode does
# not provide new enough versions for us to use.
if (CMAKE_HOST_SYSTEM_NAME MATCHES "Darwin")
    execute_process(
        COMMAND brew --prefix bison
        RESULT_VARIABLE BREW_BISON
        OUTPUT_VARIABLE BREW_BISON_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (BREW_BISON EQUAL 0 AND EXISTS "${BREW_BISON_PREFIX}")
        message(STATUS "Found Bison keg installed by Homebrew at ${BREW_BISON_PREFIX}")
        set(BISON_EXECUTABLE "${BREW_BISON_PREFIX}/bin/bison")
    endif ()

    execute_process(
        COMMAND brew --prefix flex
        RESULT_VARIABLE BREW_FLEX
        OUTPUT_VARIABLE BREW_FLEX_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (BREW_FLEX EQUAL 0 AND EXISTS "${BREW_FLEX_PREFIX}")
        message(STATUS "Found Flex keg installed by Homebrew at ${BREW_FLEX_PREFIX}")
        set(FLEX_EXECUTABLE "${BREW_FLEX_PREFIX}/bin/flex")
    endif ()
endif ()

find_package(BISON 3.0 REQUIRED)
find_package(FLEX 2.6.4 REQUIRED)

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
    SET_AND_EXPOSE_TO_BUILD(HAVE_NCURSES OFF)
else ()
    SET_AND_EXPOSE_TO_BUILD(HAVE_NCURSES ON)
endif ()

PURC_OPTION_BEGIN()
# Private options shared with other PurC ports. Add options here only if
# we need a value different from the default defined in PurCFeatures.cmake.

PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_SOCKET_STREAM PUBLIC ${ENABLE_SOCKET_STREAM_DEFAULT})
PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_XML PUBLIC OFF)
PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_HIBUS PUBLIC OFF)
PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_SSL PUBLIC OFF)

PURC_OPTION_END()

set(PurC_PKGCONFIG_FILE ${CMAKE_BINARY_DIR}/Source/PurC/purc.pc)

set(PurC_LIBRARY_TYPE SHARED)
set(PurCTestSupport_LIBRARY_TYPE SHARED)

