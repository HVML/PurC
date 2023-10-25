include(PurCCommon)
include(target/PurC)

configure_file(manifest.json.in ${CMAKE_BINARY_DIR}/manifest.json @ONLY)

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
set(purc_PLATFORM_INDEPENDENT_DIRS
    "seeker/"
    "util/"
)

if (ENABLE_RENDERER_FOIL)
    list(APPEND purc_PLATFORM_INDEPENDENT_DIRS
        "foil/"
    )
endif ()

PURC_APPEND_ALL_SOURCE_FILES_IN_DIRLIST(purc_SOURCES "${purc_PLATFORM_INDEPENDENT_DIRS}")

list(APPEND purc_SOURCES
    purc.c
    seeker.c
    timer.c
    endpoint.c
)

if (ENABLE_RENDERER_FOIL)
    list(APPEND purc_SOURCES
        foil.c
    )
endif ()

set(purc_LIBRARIES
    PurC::PurC
)

if (ENABLE_RENDERER_FOIL)
    list(APPEND purc_LIBRARIES
        PurC::CSSEng
        Ncurses::Ncurses
        -lpthread
    )
endif ()

set_target_properties(purc PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

PURC_COMPUTE_SOURCES(purc)
PURC_FRAMEWORK(purc)

install(TARGETS purc DESTINATION "${EXEC_INSTALL_DIR}/")
