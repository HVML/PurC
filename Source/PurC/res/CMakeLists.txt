find_package(Python3 COMPONENTS Interpreter)

if (NOT Python3_FOUND)
    message(FATAL_ERROR "python3 is required")
endif ()

add_subdirectory(dom)

