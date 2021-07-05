# -----------------------------------------------------------------------------
# Add components directories
# -----------------------------------------------------------------------------
#if (NOT USE_SYSTEM_MALLOC)
#    add_subdirectory(bmalloc)
#endif ()

add_subdirectory(WTF)

add_subdirectory(PurC)

if (ENABLE_API_TESTS)
string(REPLACE "-Wundef" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE "-Wundef" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
add_subdirectory(ThirdParty/gtest)
add_subdirectory(test)
endif ()

PURC_INCLUDE_CONFIG_FILES_IF_EXISTS()
