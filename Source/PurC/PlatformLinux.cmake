set(PurC_OUTPUT_NAME purc)

list(APPEND PurC_PRIVATE_INCLUDE_DIRECTORIES
)

list(APPEND PurC_UNIFIED_SOURCE_LIST_FILES
)

list(APPEND PurC_SOURCES
    "${PURC_DIR}/ports/posix/rwlock.c"
    "${PURC_DIR}/ports/posix/mutex.c"
)

list(APPEND PurC_LIBRARIES
    -lpthread
)

if (ENABLE_SOCKET_STREAM)
    list(APPEND PurC_SYSTEM_INCLUDE_DIRECTORIES
        ${GLIB_INCLUDE_DIRS}
    )

    list(APPEND PurC_LIBRARIES
        ${GLIB_GIO_LIBRARIES}
        ${GLIB_LIBRARIES}
    )
endif ()

configure_file(ports/linux/purc.pc.in ${PurC_PKGCONFIG_FILE} @ONLY)
install(FILES "${PurC_PKGCONFIG_FILE}"
        DESTINATION "${LIB_INSTALL_DIR}/pkgconfig"
)

