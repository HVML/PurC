FIND_PACKAGE(PkgConfig)

PKG_CHECK_MODULES(PCAT2 REQUIRED purringcat2)
INCLUDE_DIRECTORIES(${PCAT2_INCLUDE_DIRS})
LINK_DIRECTORIES(${PCAT2_LIBRARY_DIRS})

FIND_LIBRARY(libpcat2 NAMES purringcat2 PATHS ${PCAT2_LIBRARY_DIRS})

ADD_EXECUTABLE(attributes_high_level attributes_high_level.c)
TARGET_LINK_LIBRARIES(attributes_high_level ${libpcat2})

ADD_EXECUTABLE(callback_tree_node_high_level callback_tree_node_high_level.c)
TARGET_LINK_LIBRARIES(callback_tree_node_high_level ${libpcat2})

ADD_EXECUTABLE(chunks_high_level chunks_high_level.c)
TARGET_LINK_LIBRARIES(chunks_high_level ${libpcat2})

ADD_EXECUTABLE(custom_tree_high_level custom_tree_high_level.c)
TARGET_LINK_LIBRARIES(custom_tree_high_level ${libpcat2})

ADD_EXECUTABLE(fragment_high_level fragment_high_level.c)
TARGET_LINK_LIBRARIES(fragment_high_level ${libpcat2})

ADD_EXECUTABLE(get_title_high_level get_title_high_level.c)
TARGET_LINK_LIBRARIES(get_title_high_level ${libpcat2})

ADD_EXECUTABLE(html2sexpr html2sexpr.c)
TARGET_LINK_LIBRARIES(html2sexpr ${libpcat2})

ADD_EXECUTABLE(insert_in_appropriate_place_high_level insert_in_appropriate_place_high_level.c)
TARGET_LINK_LIBRARIES(insert_in_appropriate_place_high_level ${libpcat2})

ADD_EXECUTABLE(modify_and_serialize modify_and_serialize.c)
TARGET_LINK_LIBRARIES(modify_and_serialize ${libpcat2})

ADD_EXECUTABLE(nodes_by_attr_key_high_level nodes_by_attr_key_high_level.c)
TARGET_LINK_LIBRARIES(nodes_by_attr_key_high_level ${libpcat2})

ADD_EXECUTABLE(nodes_by_attr_value_high_level nodes_by_attr_value_high_level.c)
TARGET_LINK_LIBRARIES(nodes_by_attr_value_high_level ${libpcat2})

ADD_EXECUTABLE(parse_without_whitespace parse_without_whitespace.c)
TARGET_LINK_LIBRARIES(parse_without_whitespace ${libpcat2})

ADD_EXECUTABLE(print_tree_high_level print_tree_high_level.c)
TARGET_LINK_LIBRARIES(print_tree_high_level ${libpcat2})

ADD_EXECUTABLE(serialization_high_level serialization_high_level.c)
TARGET_LINK_LIBRARIES(serialization_high_level ${libpcat2})

ADD_EXECUTABLE(string_manipulate_high_level string_manipulate_high_level.c)
TARGET_LINK_LIBRARIES(string_manipulate_high_level ${libpcat2})

ADD_EXECUTABLE(tokenizer_colorize_high_level tokenizer_colorize_high_level.c)
TARGET_LINK_LIBRARIES(tokenizer_colorize_high_level ${libpcat2})

#ADD_EXECUTABLE(tokenizer_colorize_low_level tokenizer_colorize_low_level.c)
#TARGET_LINK_LIBRARIES(tokenizer_colorize_low_level ${libpcat2})

#ADD_EXECUTABLE(get_title_low_level get_title_low_level.c)
#TARGET_LINK_LIBRARIES(get_title_low_level ${libpcat2})

