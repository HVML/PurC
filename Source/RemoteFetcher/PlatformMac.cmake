set(RemoteFetcher_OUTPUT_NAME purc-fetcher)

include_directories(/usr/local/opt/libgpg-error/include/)
add_definitions(-Wno-deprecated-declarations)

list(APPEND RemoteFetcher_PRIVATE_INCLUDE_DIRECTORIES
    "${REMOTEFETCHER_DIR}/ipc/unix"
    "${REMOTEFETCHER_DIR}/auxiliary/soup"
    "${REMOTEFETCHER_DIR}/network/soup"
    "${REMOTEFETCHER_DIR}/network/filter"
)

list(APPEND RemoteFetcher_UNIFIED_SOURCE_LIST_FILES
    "SourcesMac.txt"
)

list(APPEND RemoteFetcher_SOURCES
)

list(APPEND RemoteFetcher_LIBRARIES
    -lpthread
)

list(APPEND RemoteFetcher_SYSTEM_INCLUDE_DIRECTORIES
    ${GIO_UNIX_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${LIBSOUP_INCLUDE_DIRS}
    ${LIBGCRYPT_INCLUDE_DIR}
    ${LIBGPGERROR_INCLUDE_DIR}
)

list(APPEND RemoteFetcher_LIBRARIES
    ${GLIB_GIO_LIBRARIES}
    ${GLIB_GOBJECT_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${LIBSOUP_LIBRARIES}
    ${PC_SQLITE3_LIBRARIES}
)

if (ENABLE_SOCKET_STREAM)
    list(APPEND RemoteFetcher_SYSTEM_INCLUDE_DIRECTORIES
    )

    list(APPEND RemoteFetcher_LIBRARIES
    )
endif ()

configure_file(ports/linux/purc-fetcher.pc.in ${RemoteFetcher_PKGCONFIG_FILE} @ONLY)
install(FILES "${RemoteFetcher_PKGCONFIG_FILE}"
        DESTINATION "${LIB_INSTALL_DIR}/pkgconfig"
)

