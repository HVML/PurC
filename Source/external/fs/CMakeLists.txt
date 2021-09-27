set(EXTERNAL_DVOBJS_INCLUDE_DIRECTORIES
    ${CMAKE_BINARY_DIR}
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}/include
    ${PURC_DIR}/include/private
)

link_directories (${CMAKE_BINARY_DIR}/Source/PurC)

set(EXTERNAL_DVOBJS_LIBRARIES
    purc
)

set(EXTERNAL_DVOBJS_FS_SOURCES
    fs.c
    ../pub/helper.c
)

include_directories(SYSTEM ${EXTERNAL_DVOBJS_INCLUDE_DIRECTORIES})
add_library(purc-dvobj-FS SHARED ${EXTERNAL_DVOBJS_FS_SOURCES})
target_link_libraries(purc-dvobj-FS ${EXTERNAL_DVOBJS_LIBRARIES})
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-as-needed")
install(TARGETS purc-dvobj-FS DESTINATION "/usr/lib")
