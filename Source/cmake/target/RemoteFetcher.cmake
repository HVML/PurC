if (NOT TARGET PurC::RemoteFetcher)
    if (NOT INTERNAL_BUILD)
        message(FATAL_ERROR "PurC::RemoteFetcher target not found")
    endif ()

    # This should be moved to an if block if the Apple Mac/iOS build moves completely to CMake
    # Just assuming Windows for the moment
    add_library(PurC::RemoteFetcher STATIC IMPORTED)
    set_target_properties(PurC::RemoteFetcher PROPERTIES
        IMPORTED_LOCATION ${PURC_LIBRARIES_LINK_DIR}/RemoteFetcher${DEBUG_SUFFIX}.lib
    )
    set(RemoteFetcher_PRIVATE_FRAMEWORK_HEADERS_DIR "${DOMRULER_DIR}")
    target_include_directories(PurC::RemoteFetcher INTERFACE
        ${RemoteFetcher_PRIVATE_FRAMEWORK_HEADERS_DIR}
    )
endif ()
