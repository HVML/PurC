set(PurCExternalExecutor_INCLUDE_DIRECTORIES
    ${FORWARDING_HEADERS_DIR}
    ${PURC_DIR} ${PURC_DIR}/include
    ${WTF_DIR}
)

set(PurCExternalExecutor_SYSTEM_INCLUDE_DIRECTORIES
    ${PURC_INCLUDE_DIRS}
)

set(PurCExternalExecutor_DEFINITIONS
    USE_STDOUT_WHEN_LOG_DISABLED
)

set(PurCExternalExecutor_LIBRARIES
    PurC::PurC
)

macro(ADD_EXTERNAL_EXECUTOR extension_name)
    add_library(${extension_name} SHARED ${ARGN})
    add_dependencies(${extension_name} PurC)
    set_property(
        TARGET ${extension_name}
        APPEND
        PROPERTY COMPILE_DEFINITIONS WEBEXT_COMPILATION
    )
    set_target_properties(${extension_name} PROPERTIES
        OUTPUT_NAME "purc-executor-${extension_name}"
    )
    set_target_properties(${extension_name} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    )
    target_compile_definitions(${extension_name} PUBLIC ${PurCExternalExecutor_DEFINITIONS})
    target_include_directories(${extension_name} PUBLIC ${PurCExternalExecutor_INCLUDE_DIRECTORIES})
    target_include_directories(${extension_name} SYSTEM PUBLIC ${PurCExternalExecutor_SYSTEM_INCLUDE_DIRECTORIES})
    target_link_libraries(${extension_name} ${PurCExternalExecutor_LIBRARIES})
endmacro()

# choose_timer

set(CHOOSETIMER_SOURCES
    choose_timer.c
)

ADD_EXTERNAL_EXECUTOR(CHOOSETIMER ${CHOOSETIMER_SOURCES})

set(FUNC_DEMO_SOURCES
    func_demo.c
)

ADD_EXTERNAL_EXECUTOR(FUNC_DEMO ${FUNC_DEMO_SOURCES})
