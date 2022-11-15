include(PurCCommon)
include(target/RemoteFetcher)

PURC_EXECUTABLE_DECLARE(purcfetcher)

list(APPEND purcfetcher_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
)

set(purcfetcher_SOURCES
    fetcher.cpp
)

set(purcfetcher_LIBRARIES
    PurC::RemoteFetcher
    pthread
)

set_target_properties(purcfetcher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

PURC_COMPUTE_SOURCES(purcfetcher)
PURC_FRAMEWORK(purcfetcher)

install(TARGETS purcfetcher DESTINATION "${EXEC_INSTALL_DIR}/")

