include(PurCCommon)
include(target/RemoteFetcher)

PURC_EXECUTABLE_DECLARE(purc_fetcher)

list(APPEND purc_fetcher_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
)

set(purc_fetcher_SOURCES
    fetcher.cpp
)

set(purc_fetcher_LIBRARIES
    PurC::RemoteFetcher
    pthread
)

set_target_properties(purc_fetcher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

PURC_COMPUTE_SOURCES(purc_fetcher)
PURC_FRAMEWORK(purc_fetcher)

install(TARGETS purc_fetcher DESTINATION "${EXEC_INSTALL_DIR}/")

