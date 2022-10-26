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
    remote_fetcher
    pthread
)

PURC_EXECUTABLE(purcfetcher)

PURC_COMPUTE_SOURCES(purcfetcher)
PURC_FRAMEWORK(purcfetcher)

