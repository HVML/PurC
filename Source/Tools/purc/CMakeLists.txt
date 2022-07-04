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

set(sample_FILES
    "samples/hello.hvml"
    "samples/hello-10.hvml"
    "samples/hello-html.hvml"
)

PURC_COPY_FILES(purc_copy_samples
    DESTINATION "${CMAKE_BINARY_DIR}"
    FILES ${sample_FILES}
)

