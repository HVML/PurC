set_property(DIRECTORY . PROPERTY FOLDER "ExtDVObjs")

include(PurCCommon)

if (ENABLE_EXTDVOBJ_MATH)
    add_subdirectory(math)
endif ()

if (ENABLE_EXTDVOBJ_FS)
    add_subdirectory(fs)
endif ()

if (ENABLE_EXTDVOBJ_PY)
    add_subdirectory(py)
endif ()

if (ENABLE_EXTDVOBJ_SQLITE)
    add_subdirectory(sqlite)
endif ()
