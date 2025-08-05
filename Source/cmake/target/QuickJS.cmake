if (NOT TARGET PurC::QuickJS)
    if (NOT INTERNAL_BUILD)
        message(FATAL_ERROR "PurC::QuickJS target not found")
    endif ()

    # This should be moved to an if block if the Apple Mac/iOS build moves completely to CMake
    # Just assuming Windows for the moment
    add_library(PurC::QuickJS STATIC IMPORTED)
    set_target_properties(PurC::QuickJS PROPERTIES
        IMPORTED_LOCATION ${PURC_LIBRARIES_LINK_DIR}/QuickJS${DEBUG_SUFFIX}.lib
    )
    set(QuickJS_PRIVATE_FRAMEWORK_HEADERS_DIR "${QuickJS_DIR}")
    target_include_directories(PurC::QuickJS INTERFACE
        ${QuickJS_PRIVATE_FRAMEWORK_HEADERS_DIR}
    )
endif ()
