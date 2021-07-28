# add_custom_target(DOM_PY python3 attr.py "${CMAKE_CURRENT_BINARY_DIR}"
#     DEPENDS attr.py tmp/const.h tmp/res.h
#     COMMENT "generating attr_res.h and attr_const.h"
#     WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
#     BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/attr_res.h"
#                "${CMAKE_CURRENT_BINARY_DIR}/attr_const.h"
#     VERBATIM)
# 
# add_dependencies(PurC DOM_PY)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/attr_res.h"
                          "${CMAKE_CURRENT_BINARY_DIR}/attr_const.h"
                          "${CMAKE_CURRENT_BINARY_DIR}/foo.c"
    COMMAND python3 attr.py "${CMAKE_CURRENT_BINARY_DIR}"
    COMMAND touch "${CMAKE_CURRENT_BINARY_DIR}/foo.c"
    DEPENDS attr.py tmp/const.h tmp/res.h
    COMMENT "generating attr_res.h and attr_const.h"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM)

add_library(DOM_PY_DUMMY foo.c)

add_dependencies(PurC DOM_PY_DUMMY)

