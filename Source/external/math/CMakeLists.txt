set(EXTERNAL_DVOBJS_INCLUDE_DIRECTORIES
    ${CMAKE_BINARY_DIR}
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR}
    ${PurC_DERIVED_SOURCES_DIR}
    ${PURC_DIR}/include
    ${PURC_DIR}/include/private
    ${CMAKE_BINARY_DIR}/Source/PurC/dvobjs
)


set(EXTERNAL_DVOBJS_LIBRARIES
    purc
    m
)

set(EXTERNAL_DVOBJS_MATH_SOURCES
    math.c
#    ${CMAKE_BINARY_DIR}/Source/PurC/dvobjs/math.lex.c
#    ${CMAKE_BINARY_DIR}/Source/PurC/dvobjs/math.tab.c
    ../pub/helper.c
)

link_directories (${CMAKE_BINARY_DIR}/Source/PurC)

include_directories(SYSTEM ${EXTERNAL_DVOBJS_INCLUDE_DIRECTORIES})
add_library(purc-dvobj-MATH SHARED ${EXTERNAL_DVOBJS_MATH_SOURCES})
target_link_libraries(purc-dvobj-MATH ${EXTERNAL_DVOBJS_LIBRARIES})
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-as-needed")
install(TARGETS purc-dvobj-MATH DESTINATION "/usr/lib/purc-${PURC_API_VERSION}")
