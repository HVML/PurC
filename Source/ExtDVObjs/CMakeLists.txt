set_property(DIRECTORY . PROPERTY FOLDER "ExtDVObjs")

set(EXT_DVOBJ_VERSION 0)
set(EXT_DVOBJ_LIB_INSTALL_DIR "/usr/lib" CACHE PATH "Absolute path to library installation directory")

include(PurCCommon)

add_subdirectory(fs)
add_subdirectory(math)

