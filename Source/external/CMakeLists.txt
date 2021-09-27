set_property(DIRECTORY . PROPERTY FOLDER "EXTERNAL")

include(PurCCommon)

set(dvobjs_PARSERS math)
PURC_FLEX_BISON(external math "${dvobjs_PARSERS}")
unset(dvobjs_PARSERS)

add_subdirectory(file)
add_subdirectory(fs)
add_subdirectory(math)
