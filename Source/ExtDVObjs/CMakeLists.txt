set_property(DIRECTORY . PROPERTY FOLDER "ExtDVObjs")

include(PurCCommon)

find_package(Python3 COMPONENTS Development)
if (Python3_Development_FOUND)
    add_subdirectory(py)
endif()

add_subdirectory(fs)
add_subdirectory(math)

