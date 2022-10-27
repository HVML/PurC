include(PurCCommon)
include(target/RemoteFetcher)

PURC_EXECUTABLE_DECLARE(purc-fetcher)

list(APPEND purc-fetcher_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
)

set(purc-fetcher_SOURCES
    fetcher.cpp
)

set(purc-fetcher_LIBRARIES
    PurC::RemoteFetcher
    pthread
)

set_target_properties(purc-fetcher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

PURC_COMPUTE_SOURCES(purc-fetcher)
PURC_FRAMEWORK(purc-fetcher)

install(TARGETS purc-fetcher DESTINATION "${EXEC_INSTALL_DIR}/")

