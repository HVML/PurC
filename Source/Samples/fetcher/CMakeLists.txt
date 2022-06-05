include(PurCCommon)
include(target/PurC)

# sync_req
PURC_EXECUTABLE_DECLARE(sync_req)

list(APPEND sync_req_PRIVATE_INCLUDE_DIRECTORIES
    "${PURC_DIR}/include"
    "${PURC_DIR}/fetchers"
    "${PurC_DERIVED_SOURCES_DIR}"
    "${FORWARDING_HEADERS_DIR}"
    "${GIO_UNIX_INCLUDE_DIRS}"
    "${GLIB_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(sync_req)

set(sync_req_SOURCES
    sync_req.cpp
)

set(sync_req_LIBRARIES
    PurC::PurC
    -lpthread
)

PURC_COMPUTE_SOURCES(sync_req)
PURC_FRAMEWORK(sync_req)


# async_req
PURC_EXECUTABLE_DECLARE(async_req)

list(APPEND async_req_PRIVATE_INCLUDE_DIRECTORIES
    "${PURC_DIR}/include"
    "${PURC_DIR}/fetchers"
    "${PurC_DERIVED_SOURCES_DIR}"
    "${FORWARDING_HEADERS_DIR}"
    "${GIO_UNIX_INCLUDE_DIRS}"
    "${GLIB_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(async_req)

set(async_req_SOURCES
    async_req.cpp
)

set(async_req_LIBRARIES
    PurC::PurC
    -lpthread
)

PURC_COMPUTE_SOURCES(async_req)
PURC_FRAMEWORK(async_req)

# multiple_async
PURC_EXECUTABLE_DECLARE(multiple_async)

list(APPEND multiple_async_PRIVATE_INCLUDE_DIRECTORIES
    ${PURC_DIR}/include
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}
    ${CMAKE_BINARY_DIR}
    ${WTF_DIR}
    "${FORWARDING_HEADERS_DIR}"
    "${GIO_UNIX_INCLUDE_DIRS}"
    "${GLIB_INCLUDE_DIRS}"
)

PURC_EXECUTABLE(multiple_async)

set(multiple_async_SOURCES
    multiple_async.cpp
)

set(multiple_async_LIBRARIES
    PurC::PurC
    gtest_main
    gtest
    pthread
)

PURC_COMPUTE_SOURCES(multiple_async)
PURC_FRAMEWORK(multiple_async)
