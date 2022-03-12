add_subdirectory(utils)
add_subdirectory(rwstream)
add_subdirectory(variant)
add_subdirectory(instance)
add_subdirectory(misc)
add_subdirectory(tree)
add_subdirectory(ejson)
add_subdirectory(html)
add_subdirectory(dvobjs)
add_subdirectory(hvml)
add_subdirectory(vcm)
add_subdirectory(vdom)
add_subdirectory(executors)
add_subdirectory(interpreter)
add_subdirectory(fetcher)

PURC_COPY_FILES(TEST_Script
    DESTINATION ${CMAKE_BINARY_DIR}/
    FILES run_all_tests.sh
)
