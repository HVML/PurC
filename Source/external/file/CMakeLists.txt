set(EXTERNAL_DVOBJS_INCLUDE_DIRECTORIES
    ${CMAKE_BINARY_DIR}
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}/include
    ${PURC_DIR}/include/private
)

find_library(PURC_LIBRARY NAMES purc HITS ${PURC_DIR})

set(EXTERNAL_DVOBJS_FILE_SOURCES
    file.c
    helper.c
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

include_directories(SYSTEM ${EXTERNAL_DVOBJS_INCLUDE_DIRECTORIES})
add_library(purc-dvobj-FILE SHARED ${EXTERNAL_DVOBJS_FILE_SOURCES})
target_link_libraries(purc-dvobj-FILE ${PURC_LIBRARY})
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-as-needed")
install(TARGETS purc-dvobj-FILE DESTINATION "${LIB_INSTALL_DIR}")
