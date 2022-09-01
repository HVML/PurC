include(PurCCommon)
include(target/PurC)

# purc
PURC_EXECUTABLE_DECLARE(purc)

list(APPEND purc_PRIVATE_INCLUDE_DIRECTORIES
    "${PURC_DIR}/include"
    "${PurC_DERIVED_SOURCES_DIR}"
    "${FORWARDING_HEADERS_DIR}"
    "${GIO_UNIX_INCLUDE_DIRS}"
    "${GLIB_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(purc)

set(purc_SOURCES
    purc.c
)

set(purc_LIBRARIES
    PurC::PurC
    -lpthread
)

PURC_COMPUTE_SOURCES(purc)
PURC_FRAMEWORK(purc)

install(TARGETS purc DESTINATION "${EXEC_INSTALL_DIR}/")
