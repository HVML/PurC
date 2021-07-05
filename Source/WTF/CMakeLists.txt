set_property(DIRECTORY . PROPERTY FOLDER "WTF")

include(PurCCommon)
include(MyMacros.cmake)

CHECK_THREAD_LOCAL_STORAGE_KEYWORD()
configure_file(wtf/TLSKeyword.h.in ${WTF_FRAMEWORK_HEADERS_DIR}/wtf/TLSKeyword.h)

add_subdirectory(wtf)
