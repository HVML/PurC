CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(purring-cat-2-examples C)

SET(PROJECT_NAME "purring-cat-2-examples")
SET(PROJECT_VERSION "0.0.1")

ADD_DEFINITIONS(-O2 -g --std=gnu99 -Wall -fmax-errors=10)

SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")

ADD_SUBDIRECTORY(myhtml)

# Packing stuff
SET(CPACK_PACKAGE_VENDOR "FMSoft")
SET(CPACK_PACKAGE_NAME ${PROJECT_NAME})
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Examples of Purring Cat 2")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}")
SET(CPACK_SOURCE_GENERATOR "TGZ")
SET(CPACK_SOURCE_IGNORE_FILES
      "/auto/;/cmake/;/.svn/;/.git/;/.bzr/;/CMakeFiles/;/_CPack_Packages/;CMakeCache.txt$;
      ~$;.swp$;.cmake$;
      ${CPACK_SOURCE_IGNORE_FILES}")

INCLUDE(CPack)

