set(PurC_OUTPUT_NAME purc)

if (HAVE_GLIB)
    list(APPEND PurC_SYSTEM_INCLUDE_DIRECTORIES
        ${GLIB_INCLUDE_DIRS}
    )
    list(APPEND PurC_LIBRARIES
        ${GLIB_LIBRARIES}
        ${GLIB_GMODULE_LIBRARIES}
    )
endif ()

list(APPEND PurC_PRIVATE_INCLUDE_DIRECTORIES
    "${PURC_DIR}/fetchers"
    "${PURC_DIR}/fetchers/ipc"
    "${PURC_DIR}/fetchers/ipc/unix"
    "${PURC_DIR}/fetchers/launcher"
    "${PURC_DIR}/fetchers/messages"
    "${PURC_DIR}/fetchers/messages/soup"
)

list(APPEND PurC_UNIFIED_SOURCE_LIST_FILES
)

list(APPEND PurC_SOURCES
    "${PURC_DIR}/ports/posix/rwlock.c"
    "${PURC_DIR}/ports/posix/mutex.c"
    "${PURC_DIR}/ports/posix/sleep.c"
    "${PURC_DIR}/ports/posix/file.c"
)

list(APPEND PurC_LIBRARIES
    ${GLIB_GIO_LIBRARIES}
    ${GLIB_GOBJECT_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GLIB_GMODULE_LIBRARIES}
    -lpthread
    -lm
    -ldl
)

if (ENABLE_SOCKET_STREAM)
    list(APPEND PurC_LIBRARIES
        ${GLIB_GIO_LIBRARIES}
    )
endif ()

configure_file(ports/linux/purc.pc.in ${PurC_PKGCONFIG_FILE} @ONLY)
install(FILES "${PurC_PKGCONFIG_FILE}"
        DESTINATION "${LIB_INSTALL_DIR}/pkgconfig"
)

