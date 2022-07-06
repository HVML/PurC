set(PurC_OUTPUT_NAME purc)

list(APPEND PurC_PRIVATE_INCLUDE_DIRECTORIES
    "${PURC_DIR}/fetchers"
    "${PURC_DIR}/fetchers/ipc"
    "${PURC_DIR}/fetchers/ipc/unix"
    "${PURC_DIR}/fetchers/launcher"
    "${PURC_DIR}/fetchers/messages"
    "${PURC_DIR}/fetchers/messages/soup"

    "${GLIB_INCLUDE_DIRS}"
)

list(APPEND PurC_UNIFIED_SOURCE_LIST_FILES
    "SourcesLinux.txt"
)

list(APPEND PurC_SOURCES
    "${PURC_DIR}/ports/posix/rwlock.c"
    "${PURC_DIR}/ports/posix/mutex.c"
    "${PURC_DIR}/ports/posix/sleep.c"
    "${PURC_DIR}/ports/posix/file.c"
)

list(APPEND PurC_LIBRARIES
    PurC::WTF
    ${GLIB_GIO_LIBRARIES}
    ${GLIB_GOBJECT_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GLIB_GMODULE_LIBRARIES}
    -lpthread
    -lm
    -ldl
    -lrt
)


configure_file(ports/linux/purc.pc.in ${PurC_PKGCONFIG_FILE} @ONLY)
install(FILES "${PurC_PKGCONFIG_FILE}"
        DESTINATION "${LIB_INSTALL_DIR}/pkgconfig"
)

