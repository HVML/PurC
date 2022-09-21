# Define minimum supported Windows version
# https://msdn.microsoft.com/en-us/library/6sehtctf.aspx
#
# Currently set to Windows 7
add_definitions(-D_WINDOWS -DWINVER=0x601 -D_WIN32_WINNT=0x601)

add_definitions(-DNOMINMAX)
add_definitions(-DUNICODE -D_UNICODE)

PURC_OPTION_BEGIN()

if (${WTF_PLATFORM_WIN_MINGW})
    PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_SOCKET_STREAM PUBLIC ${ENABLE_SOCKET_STREAM_DEFAULT})
    PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_XML PUBLIC ${ENABLE_XML_DEFAULT})
    PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_HIBUS PUBLIC ${ENABLE_HIBUS_DEFAULT})
    PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_SSL PUBLIC ${ENABLE_SSL_DEFAULT})
else ()
    PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_SOCKET_STREAM PUBLIC OFF)
endif ()

PURC_OPTION_DEFAULT_PORT_VALUE(ENABLE_ICU PUBLIC ON)

PURC_OPTION_END()

#if (DEFINED ENV{PURC_IGNORE_PATH})
#    set(CMAKE_IGNORE_PATH $ENV{PURC_IGNORE_PATH})
#endif ()

#if (NOT PURC_LIBRARIES_DIR)
#    if (DEFINED ENV{PURC_LIBRARIES})
#        file(TO_CMAKE_PATH "$ENV{PURC_LIBRARIES}" PURC_LIBRARIES_DIR)
#    else ()
#        file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/PurCLibraries/win" PURC_LIBRARIES_DIR)
#    endif ()
#endif ()

#set(CMAKE_PREFIX_PATH ${PURC_LIBRARIES_DIR})

#set(PURC_LIBRARIES_INCLUDE_DIR "${PURC_LIBRARIES_DIR}/include")
#include_directories(${PURC_LIBRARIES_INCLUDE_DIR})

if (${WTF_CPU_X86})
    set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB32_PATHS ON)
    set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)
    set(PURC_LIBRARIES_LINK_DIR "${PURC_LIBRARIES_DIR}/lib32")
    # FIXME: Remove ${PURC_LIBRARIES_LINK_DIR} when find_library is used for everything
    link_directories("${CMAKE_BINARY_DIR}/lib32" "${PURC_LIBRARIES_LINK_DIR}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib32)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin32)
else ()
    set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB32_PATHS OFF)
    set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS ON)
    set(PURC_LIBRARIES_LINK_DIR "${PURC_LIBRARIES_DIR}/lib64")
    # FIXME: Remove ${PURC_LIBRARIES_LINK_DIR} when find_library is used for everything
    link_directories("${CMAKE_BINARY_DIR}/lib64" "${PURC_LIBRARIES_LINK_DIR}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib64)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib64)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin64)
endif ()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

set(PORT Win)
set(WTF_LIBRARY_TYPE STATIC)

# If <winsock2.h> is not included before <windows.h> redefinition errors occur
# unless _WINSOCKAPI_ is defined before <windows.h> is included
add_definitions(-D_WINSOCKAPI_=)
