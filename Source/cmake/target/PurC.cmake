if (NOT TARGET PurC::PurC)
    if (NOT INTERNAL_BUILD)
        message(FATAL_ERROR "PurC::PurC target not found")
    endif ()

    # This should be moved to an if block if the Apple Mac/iOS build moves completely to CMake
    # Just assuming Windows for the moment
    add_library(PurC::PurC STATIC IMPORTED)
    set_target_properties(PurC::PurC PROPERTIES
        IMPORTED_LOCATION ${PURC_LIBRARIES_LINK_DIR}/PurC${DEBUG_SUFFIX}.lib
        # Should add Apple libraries here when https://bugs.webkit.org/show_bug.cgi?id=205085 lands
        INTERFACE_LINK_LIBRARIES "PurC::WTF"
    )
    set(PurC_PRIVATE_FRAMEWORK_HEADERS_DIR "${CMAKE_BINARY_DIR}/../include/private")
    target_include_directories(PurC::PurC INTERFACE
        ${PurC_PRIVATE_FRAMEWORK_HEADERS_DIR}
    )
endif ()
