add_subdirectory(purc)

if (ENABLE_BUILD_REMOTE_FETCHER)
    add_subdirectory(purc-fetcher)
endif ()

