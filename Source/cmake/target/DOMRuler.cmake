if (NOT TARGET PurC::DOMRuler)
    if (NOT INTERNAL_BUILD)
        message(FATAL_ERROR "PurC::DOMRuler target not found")
    endif ()

    # This should be moved to an if block if the Apple Mac/iOS build moves completely to CMake
    # Just assuming Windows for the moment
    add_library(PurC::DOMRuler STATIC IMPORTED)
    set_target_properties(PurC::DOMRuler PROPERTIES
        IMPORTED_LOCATION ${WEBKIT_LIBRARIES_LINK_DIR}/DOMRuler${DEBUG_SUFFIX}.lib
    )
    set(DOMRuler_PRIVATE_FRAMEWORK_HEADERS_DIR "${DOMRULER_DIR}")
    target_include_directories(PurC::DOMRuler INTERFACE
        ${DOMRuler_PRIVATE_FRAMEWORK_HEADERS_DIR}
    )
endif ()
