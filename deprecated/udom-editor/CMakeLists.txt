include(PurCCommon)
include(MyMacros.cmake)

configure_file(ue-version.h.in ${PurC_DERIVED_SOURCES_DIR}/ue-version.h)
configure_file(ue-features.h.in ${PurC_DERIVED_SOURCES_DIR}/ue-features.h)

set(UE_DIR "${CMAKE_SOURCE_DIR}/Source/Tools/udom-editor")

# udom-editor
PURC_EXECUTABLE_DECLARE(ue)

list(APPEND ue_PRIVATE_INCLUDE_DIRECTORIES
    "${CMAKE_BINARY_DIR}"
    "${PURC_DIR}/include"
    "${PurC_DERIVED_SOURCES_DIR}"
    "${UE_DIR}"
)

list(APPEND ue_SYSTEM_INCLUDE_DIRECTORIES
    ${GLIB_INCLUDE_DIRS}
)

PURC_EXECUTABLE(ue)

set(ue_PLATFORM_INDEPENDENT_DIRS
    "lib"
    "src"
)

APPEND_ALL_SOURCE_FILES_IN_DIRLIST(ue_SOURCES "${ue_PLATFORM_INDEPENDENT_DIRS}")

list(APPEND ue_SOURCES
)

set(ue_LIBRARIES
    ${GLIB_LIBRARIES}
    Ncurses::Ncurses
    PurC::PurC
    pthread
)

PURC_COMPUTE_SOURCES(ue)
PURC_FRAMEWORK(ue)

