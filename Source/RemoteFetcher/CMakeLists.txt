set_property(DIRECTORY . PROPERTY FOLDER "RemoteFetcher")

add_definitions(-Wno-conversion)

include(PurCCommon)
include(MyMacros.cmake)

configure_file(include/purc_fetcher_version.h.in ${RemoteFetcher_DERIVED_SOURCES_DIR}/purc_fetcher_version.h)
configure_file(include/purc_fetcher_features.h.in ${RemoteFetcher_DERIVED_SOURCES_DIR}/purc_fetcher_features.h)

set(RemoteFetcher_PRIVATE_INCLUDE_DIRECTORIES
    "${CMAKE_BINARY_DIR}"
    "${REMOTEFETCHER_DIR}"
    "${REMOTEFETCHER_DIR}/include"
    "${REMOTEFETCHER_DIR}/ipc"
    "${REMOTEFETCHER_DIR}/database"
    "${REMOTEFETCHER_DIR}/auxiliary"
    "${REMOTEFETCHER_DIR}/auxiliary/crypto"
    "${REMOTEFETCHER_DIR}/network"
    "${REMOTEFETCHER_DIR}/network/Downloads"
    "${REMOTEFETCHER_DIR}/network/WebStorage"
    "${REMOTEFETCHER_DIR}/network/cache"
    "${REMOTEFETCHER_DIR}/network/Cookies"
    "${REMOTEFETCHER_DIR}/network/glib"
    "${RemoteFetcher_DERIVED_SOURCES_DIR}"
    "${Messages_DERIVED_SOURCES_DIR}"
    "${FORWARDING_HEADERS_DIR}"
)

# TODO: List the headers should be installed to system here.
set(RemoteFetcher_INSTALLED_HEADERS
    #"${RemoteFetcher_DERIVED_SOURCES_DIR}/purc_fetcher_version.h"
    #"${RemoteFetcher_DERIVED_SOURCES_DIR}/purc_fetcher_features.h"
    #"${REMOTEFETCHER_DIR}/include/purc_fetcher_macros.h"
    #"${REMOTEFETCHER_DIR}/include/purc_fetcher_errors.h"
    #"${REMOTEFETCHER_DIR}/include/purc_fetcher_utils.h"
    #"${REMOTEFETCHER_DIR}/include/purc_fetcher_ports.h"
    #"${REMOTEFETCHER_DIR}/include/purc_fetcher.h"
)

# TODO: List the directorires which do not contain any platform-dependent C source files
set(RemoteFetcher_PLATFORM_INDEPENDENT_DIRS
    "instance"
    "ipc"
    "database"
    "auxiliary"
    "network"
)

# This macro will append all C/CXX source files in ${RemoteFetcher_PLATFORM_INDEPENDENT_DIRS}
# to ${RemoteFetcher_SOURCES}
APPEND_ALL_SOURCE_FILES_IN_DIRLIST(RemoteFetcher_SOURCES "${RemoteFetcher_PLATFORM_INDEPENDENT_DIRS}")

# TODO: List the source files individually.
list(APPEND RemoteFetcher_SOURCES
)

# Put the CXX source files which can be unified in `Sources.txt`
list(APPEND RemoteFetcher_UNIFIED_SOURCE_LIST_FILES
    "Sources.txt"
)

set(RemoteFetcher_MESSAGES_IN_FILES
    messages/AuxiliaryProcess
    messages/CacheStorageEngineConnection
    messages/DownloadProxy
    messages/NetworkConnectionToWebProcess
    messages/NetworkProcessConnection
    messages/NetworkProcess
    messages/NetworkProcessProxy
    messages/NetworkResourceLoader
    messages/StorageAreaMap
    messages/StorageManagerSet
    messages/WebCookieManager
    messages/WebCookieManagerProxy
    messages/WebResourceLoader
)

GENERATE_MESSAGE_SOURCES(RemoteFetcher_SOURCES "${RemoteFetcher_MESSAGES_IN_FILES}")

set(RemoteFetcher_LIBRARIES)

# TODO: the source files and libraries for optional features.
if (ENABLE_LSQL)
    list(APPEND RemoteFetcher_LIBRARIES SQLite::SQLite3)
endif ()

if (ENABLE_XML)
    list(APPEND RemoteFetcher_LIBRARIES LibXml2::LibXml2)
endif ()

list(APPEND RemoteFetcher_LIBRARIES PurC::WTF)

list(APPEND RemoteFetcher_PRIVATE_INCLUDE_DIRECTORIES "${THIRDPARTY_DIR}/xdgmime/src")
list(APPEND RemoteFetcher_LIBRARIES xdgmime)
list(APPEND RemoteFetcher_LIBRARIES ${ICU_LIBRARIES})
list(APPEND RemoteFetcher_LIBRARIES  ${LIBGCRYPT_LIBRARIES})

if (ENABLE_RSQL)
    list(APPEND RemoteFetcher_LIBRARIES ${MYSQLCLIENT_LIBRARIES})
endif ()

if (UNIX)
    check_function_exists(shm_open SHM_OPEN_EXISTS)
    if (NOT SHM_OPEN_EXISTS)
        set(CMAKE_REQUIRED_LIBRARIES rt)
        check_function_exists(shm_open SHM_OPEN_REQUIRES_LIBRT)
        if (SHM_OPEN_REQUIRES_LIBRT)
            list(APPEND RemoteFetcher_LIBRARIES rt)
        endif ()
        unset(CMAKE_REQUIRED_LIBRARIES)
    endif ()
endif ()

set(RemoteFetcher_INTERFACE_LIBRARIES PurCFetcher)
set(RemoteFetcher_INTERFACE_INCLUDE_DIRECTORIES ${RemoteFetcher_PRIVATE_FRAMEWORK_HEADERS_DIR})

PURC_FRAMEWORK_DECLARE(RemoteFetcher)
PURC_INCLUDE_CONFIG_FILES_IF_EXISTS()

PURC_WRAP_SOURCELIST(${RemoteFetcher_SOURCES})

PURC_COPY_FILES(RemoteFetcher_CopyPrivateHeaders
    DESTINATION ${RemoteFetcher_PRIVATE_FRAMEWORK_HEADERS_DIR}/purc_fetcher
    FILES ${RemoteFetcher_PRIVATE_FRAMEWORK_HEADERS}
    FLATTENED
)
list(APPEND RemoteFetcher_INTERFACE_DEPENDENCIES RemoteFetcher_CopyPrivateHeaders)
# RemoteFetcher_CopyPrivateHeaders needs to have a direct or indirect
# dependency of RemoteFetcher target for CMake Visual Studio generator to
# eliminate duplicated custom commands. Otherwise, CSSPropertyNames.h
# will be generated in both projects.
if (NOT INTERNAL_BUILD)
    add_dependencies(RemoteFetcher_CopyPrivateHeaders RemoteFetcher)
endif ()

PURC_COMPUTE_SOURCES(RemoteFetcher)
PURC_FRAMEWORK(RemoteFetcher)
PURC_FRAMEWORK_TARGET(RemoteFetcher)

# The -ftree-sra optimization (implicit with -O2) causes crashes when
# allocating large chunks of memory using bmalloc on Intel 32bit.
# See https://bugs.webkit.org/show_bug.cgi?id=146440
string(TOLOWER ${CMAKE_HOST_SYSTEM_PROCESSOR} LOWERCASE_CMAKE_HOST_SYSTEM_PROCESSOR)
if (CMAKE_COMPILER_IS_GNUCXX AND "${LOWERCASE_CMAKE_HOST_SYSTEM_PROCESSOR}" MATCHES "(i[3-6]86|x86)$")
    PURC_ADD_TARGET_CXX_FLAGS(RemoteFetcher -fno-tree-sra)
endif ()

if (${RemoteFetcher_LIBRARY_TYPE} MATCHES "SHARED")
    set_target_properties(RemoteFetcher PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
    install(TARGETS RemoteFetcher DESTINATION "${LIB_INSTALL_DIR}")
endif ()

if (MSVC)
    set_target_properties(RemoteFetcher PROPERTIES
        COMPILE_PDB_NAME ${RemoteFetcher_OUTPUT_NAME}
    )
endif ()

# add_subdirectory(res)

install(TARGETS RemoteFetcher
        DESTINATION "${LIB_INSTALL_DIR}/"
)

install(FILES ${RemoteFetcher_INSTALLED_HEADERS}
        DESTINATION "${HEADER_INSTALL_DIR}/purc_fetcher"
)
