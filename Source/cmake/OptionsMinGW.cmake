include(GNUInstallDirs)

CALCULATE_LIBRARY_VERSIONS_FROM_LIBTOOL_TRIPLE(PURC 0 0 0)

# These are shared variables, but we special case their definition so that we can use the
# CMAKE_INSTALL_* variables that are populated by the GNUInstallDirs macro.
set(LIB_INSTALL_DIR "${CMAKE_INSTALL_FULL_LIBDIR}" CACHE PATH "Absolute path to library installation directory")
set(EXEC_INSTALL_DIR "${CMAKE_INSTALL_FULL_BINDIR}" CACHE PATH "Absolute path to executable installation directory")
set(LIBEXEC_INSTALL_DIR "${CMAKE_INSTALL_FULL_LIBEXECDIR}/purc" CACHE PATH "Absolute path to install executables executed by the library")
set(HEADER_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}" CACHE PATH "Absolute path to header installation directory")
set(PURC_HEADER_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}/purc" CACHE PATH "Absolute path to PurC header installation directory")

add_definitions(-DBUILDING_MINGW__=1)
add_definitions(-DPURC_LIBEXEC_DIR="${LIBEXEC_INSTALL_DIR}")

find_package(GLIB 2.44.0 REQUIRED COMPONENTS gio gio-win)
find_package(ICU 60.2 REQUIRED COMPONENTS data i18n uc)
find_package(BISON 3.0 REQUIRED)
find_package(FLEX 2.6.4 REQUIRED)
find_package(HiBus 100)
find_package(LibXml2 2.8.0)
find_package(OpenSSL 1.1.1)
find_package(ZLIB 1.2.0)
find_package(Threads)

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

SET_AND_EXPOSE_TO_BUILD(ENABLE_DEVELOPER_MODE ${DEVELOPER_MODE})

include(OptionsWin)

set(PurC_PKGCONFIG_FILE ${CMAKE_BINARY_DIR}/Source/PurC/purc.pc)

# Override headers directories
#set(WTF_FRAMEWORK_HEADERS_DIR ${CMAKE_BINARY_DIR}/WTF/Headers)
#set(PurC_FRAMEWORK_HEADERS_DIR ${CMAKE_BINARY_DIR}/PurC/Headers)
#set(PurC_PRIVATE_FRAMEWORK_HEADERS_DIR ${CMAKE_BINARY_DIR}/PurC/PrivateHeaders)

# Override derived sources directories
#set(WTF_DERIVED_SOURCES_DIR ${CMAKE_BINARY_DIR}/WTF/DerivedSources)
#set(PurC_DERIVED_SOURCES_DIR ${CMAKE_BINARY_DIR}/PurC/DerivedSources)

# Override library types
#set(PurC_LIBRARY_TYPE OBJECT)
#set(PurCTestSupport_LIBRARY_TYPE OBJECT)

