include(PurCCommon)

# test_node
PURC_EXECUTABLE_DECLARE(test_node)

list(APPEND test_node_PRIVATE_INCLUDE_DIRECTORIES
    "${DOMRULER_DIR}/include"
    "${FORWARDING_HEADERS_DIR}/domruler"
)

list(APPEND test_node_SYSTEM_INCLUDE_DIRECTORIES
    "${CSSEng_INCLUDE_DIRS}"
)

list(APPEND test_node_SOURCES
    test_node.c
)

set(test_node_LIBRARIES
    PurC::DOMRuler
    PurC::CSSEng
    ${GLIB_LIBRARIES}
)

PURC_EXECUTABLE(test_node)
PURC_COMPUTE_SOURCES(test_node)

# test_css_select
PURC_EXECUTABLE_DECLARE(test_css_select)

list(APPEND test_css_select_PRIVATE_INCLUDE_DIRECTORIES
    "${DOMRULER_DIR}/include"
    "${DOMRULER_DIR}/src"
    "${FORWARDING_HEADERS_DIR}/domruler"
)

list(APPEND test_css_select_SYSTEM_INCLUDE_DIRECTORIES
    "${CSSEng_INCLUDE_DIRS}"
    "${GLIB_INCLUDE_DIRS}"
)

list(APPEND test_css_select_SOURCES
    test_css_select.c
)

set(test_css_select_LIBRARIES
    PurC::PurC
    PurC::DOMRuler
    PurC::CSSEng
    ${GLIB_LIBRARIES}
)

PURC_EXECUTABLE(test_css_select)
PURC_COMPUTE_SOURCES(test_css_select)

# test_css_select2
PURC_EXECUTABLE_DECLARE(test_css_select2)

list(APPEND test_css_select2_PRIVATE_INCLUDE_DIRECTORIES
    "${DOMRULER_DIR}/include"
    "${DOMRULER_DIR}/src"
    "${FORWARDING_HEADERS_DIR}/domruler"
)

list(APPEND test_css_select2_SYSTEM_INCLUDE_DIRECTORIES
    "${CSSEng_INCLUDE_DIRS}"
    "${GLIB_INCLUDE_DIRS}"
)

list(APPEND test_css_select2_SOURCES
    test_css_select2.c
)

set(test_css_select2_LIBRARIES
    PurC::PurC
    PurC::DOMRuler
    PurC::CSSEng
    ${GLIB_LIBRARIES}
)

PURC_EXECUTABLE(test_css_select2)
PURC_COMPUTE_SOURCES(test_css_select2)

# test_layout
PURC_EXECUTABLE_DECLARE(test_layout)

list(APPEND test_layout_PRIVATE_INCLUDE_DIRECTORIES
    "${DOMRULER_DIR}/include"
    "${DOMRULER_DIR}/src"
    "${FORWARDING_HEADERS_DIR}/domruler"
)

list(APPEND test_layout_SYSTEM_INCLUDE_DIRECTORIES
    "${CSSEng_INCLUDE_DIRS}"
    "${GLIB_INCLUDE_DIRS}"
)

list(APPEND test_layout_SOURCES
    test_layout.c
)

set(test_layout_LIBRARIES
    PurC::PurC
    PurC::DOMRuler
    PurC::CSSEng
    ${GLIB_LIBRARIES}
)

PURC_EXECUTABLE(test_layout)
PURC_COMPUTE_SOURCES(test_layout)

# test_layout_pcdom
PURC_EXECUTABLE_DECLARE(test_layout_pcdom)

list(APPEND test_layout_pcdom_PRIVATE_INCLUDE_DIRECTORIES
    "${DOMRULER_DIR}/include"
    "${DOMRULER_DIR}/src"
    "${FORWARDING_HEADERS_DIR}/domruler"
)

list(APPEND test_layout_pcdom_SYSTEM_INCLUDE_DIRECTORIES
    "${CSSEng_INCLUDE_DIRS}"
    "${GLIB_INCLUDE_DIRS}"
)

list(APPEND test_layout_pcdom_SOURCES
    test_layout_pcdom.c
)

set(test_layout_pcdom_LIBRARIES
    PurC::PurC
    PurC::DOMRuler
    PurC::CSSEng
    ${GLIB_LIBRARIES}
)

PURC_EXECUTABLE(test_layout_pcdom)
PURC_COMPUTE_SOURCES(test_layout_pcdom)

