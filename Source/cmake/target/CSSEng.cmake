if (NOT TARGET PurC::CSSEng)
    if (NOT INTERNAL_BUILD)
        message(FATAL_ERROR "PurC::CSSEng target not found")
    endif ()

    # This should be moved to an if block if the Apple Mac/iOS build moves completely to CMake
    # Just assuming Windows for the moment
    add_library(PurC::CSSEng STATIC IMPORTED)
    set_target_properties(PurC::CSSEng PROPERTIES
        IMPORTED_LOCATION ${PURC_LIBRARIES_LINK_DIR}/CSSEng${DEBUG_SUFFIX}.lib
    )
    set(CSSEng_PRIVATE_FRAMEWORK_HEADERS_DIR "${CSSENG_DIR}")
    target_include_directories(PurC::CSSEng INTERFACE
        ${CSSEng_PRIVATE_FRAMEWORK_HEADERS_DIR}
    )
endif ()
