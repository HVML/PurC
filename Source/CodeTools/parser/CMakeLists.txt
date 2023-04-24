cmake_minimum_required(VERSION 3.15)
if (CMAKE_HOST_UNIX)
    find_program(HOST_CC_FOUND cc)
elseif (CMAKE_HOST_WIN32)
    find_program(HOST_CC_FOUND cl)
endif ()

if (HOST_CC_FOUND)
    message(STATUS "Found host C compiler: ${HOST_CC_FOUND}")
    set(CMAKE_C_COMPILER, ${HOST_CC_FOUND})
    add_executable(csspropgen css_property_parser_gen.c)
else ()
    message(FATAL_ERROR "Could NOT find host C compiler")
endif ()

#include(PurCCommon)
#
#PURC_EXECUTABLE_DECLARE(csspropgen)
#
#list(APPEND csspropgen_PRIVATE_INCLUDE_DIRECTORIES
#)
#
#list(APPEND csspropgen_SYSTEM_INCLUDE_DIRECTORIES
#)
#
#list(APPEND csspropgen_SOURCES
#    css_property_parser_gen.c
#)
#
#set(csspropgen_LIBRARIES
#)
#
#PURC_EXECUTABLE(csspropgen)
#PURC_COMPUTE_SOURCES(csspropgen)

