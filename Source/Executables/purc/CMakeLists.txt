include(PurCCommon)
include(target/PurC)

# purc
PURC_EXECUTABLE_DECLARE(purc)

if (HAVE_GLIB)
    list(APPEND purc_SYSTEM_INCLUDE_DIRECTORIES
        ${GLIB_INCLUDE_DIRS}
    )
    list(APPEND purc_LIBRARIES
        ${GLIB_LIBRARIES}
        ${GLIB_GMODULE_LIBRARIES}
    )
endif ()

list(APPEND purc_PRIVATE_INCLUDE_DIRECTORIES
    "${CMAKE_CURRENT_SOURCE_DIR}"   # for config.h
    "${CMAKE_BINARY_DIR}"           # for cmakeconfig
    "${FORWARDING_HEADERS_DIR}"
)

PURC_EXECUTABLE(purc)

# TODO: List the directorires which do not contain any platform-dependent C source files
set(purc_PLATFORM_INDEPENDENT_DIRS )

if (ENABLE_RDR_FOIL)
    list(APPEND purc_PLATFORM_INDEPENDENT_DIRS
        "tty/"
        "strutil/"
        "util/"
        "unicode/"
        "region/"
    )
    PURC_APPEND_ALL_SOURCE_FILES_IN_DIRLIST(purc_SOURCES "${purc_PLATFORM_INDEPENDENT_DIRS}")
endif ()

list(APPEND purc_SOURCES
    purc.c
)

if (ENABLE_RDR_FOIL)
    list(APPEND purc_SOURCES
        foil.c
        helpers.c
        screen.c
        endpoint.c
        callbacks.c
        workspace.c
        css-selection.c
        udom.c
        page.c
        rdrbox.c
        rdrbox-marker.c
        rdrbox-inline.c
        rdrbox-layout.c
        widget.c
    )
endif ()

set(purc_LIBRARIES
    PurC::PurC
    PurC::CSSEng
    Ncurses::Ncurses
    -lpthread
)

set_target_properties(purc PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

PURC_COMPUTE_SOURCES(purc)
PURC_FRAMEWORK(purc)

install(TARGETS purc DESTINATION "${EXEC_INSTALL_DIR}/")
