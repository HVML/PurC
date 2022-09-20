include(PurCCommon)
include(target/PurC)

# purc
PURC_EXECUTABLE_DECLARE(purc)

list(APPEND purc_PRIVATE_INCLUDE_DIRECTORIES
    "${CMAKE_BINARY_DIR}"
    "${PURC_DIR}/include"
    "${PurC_DERIVED_SOURCES_DIR}"
)

PURC_EXECUTABLE(purc)

set(purc_SOURCES
    purc.c
)

if (ENABLE_RDR_FOIL)
    list(APPEND purc_SOURCES
    )
endif ()

set(purc_LIBRARIES
    PurC::PurC
    -lpthread
)

PURC_COMPUTE_SOURCES(purc)
PURC_FRAMEWORK(purc)

install(TARGETS purc DESTINATION "${EXEC_INSTALL_DIR}/")
