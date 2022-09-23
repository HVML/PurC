include(PurCCommon)

# node_test
PURC_EXECUTABLE_DECLARE(node_test)

list(APPEND node_test_PRIVATE_INCLUDE_DIRECTORIES
    "${DOMRULER_DIR}/include"
    "${FORWARDING_HEADERS_DIR}/domruler"
)

list(APPEND node_test_SYSTEM_INCLUDE_DIRECTORIES
    "${CSSEng_INCLUDE_DIRS}"
)

list(APPEND node_test_SOURCES
    node_test.c
)

set(node_test_LIBRARIES
    PurC::DOMRuler
    PurC::CSSEng
    ${GLIB_LIBRARIES}
)

PURC_EXECUTABLE(node_test)
PURC_COMPUTE_SOURCES(node_test)

# css_select_test
PURC_EXECUTABLE_DECLARE(css_select_test)

list(APPEND css_select_test_PRIVATE_INCLUDE_DIRECTORIES
    "${DOMRULER_DIR}/include"
    "${DOMRULER_DIR}/src"
    "${FORWARDING_HEADERS_DIR}/domruler"
)

list(APPEND css_select_test_SYSTEM_INCLUDE_DIRECTORIES
    "${CSSEng_INCLUDE_DIRS}"
    "${GLIB_INCLUDE_DIRS}"
)

list(APPEND css_select_test_SOURCES
    css_select_test.c
)

set(css_select_test_LIBRARIES
    PurC::PurC
    PurC::DOMRuler
    PurC::CSSEng
    ${GLIB_LIBRARIES}
)

PURC_EXECUTABLE(css_select_test)
PURC_COMPUTE_SOURCES(css_select_test)

# css_select_test2
PURC_EXECUTABLE_DECLARE(css_select_test2)

list(APPEND css_select_test2_PRIVATE_INCLUDE_DIRECTORIES
    "${DOMRULER_DIR}/include"
    "${DOMRULER_DIR}/src"
    "${FORWARDING_HEADERS_DIR}/domruler"
)

list(APPEND css_select_test2_SYSTEM_INCLUDE_DIRECTORIES
    "${CSSEng_INCLUDE_DIRS}"
    "${GLIB_INCLUDE_DIRS}"
)

list(APPEND css_select_test2_SOURCES
    css_select_test2.c
)

set(css_select_test2_LIBRARIES
    PurC::PurC
    PurC::DOMRuler
    PurC::CSSEng
    ${GLIB_LIBRARIES}
)

PURC_EXECUTABLE(css_select_test2)
PURC_COMPUTE_SOURCES(css_select_test2)

# layout_test
PURC_EXECUTABLE_DECLARE(layout_test)

list(APPEND layout_test_PRIVATE_INCLUDE_DIRECTORIES
    "${DOMRULER_DIR}/include"
    "${DOMRULER_DIR}/src"
    "${FORWARDING_HEADERS_DIR}/domruler"
)

list(APPEND layout_test_SYSTEM_INCLUDE_DIRECTORIES
    "${CSSEng_INCLUDE_DIRS}"
    "${GLIB_INCLUDE_DIRS}"
)

list(APPEND layout_test_SOURCES
    layout_test.c
)

set(layout_test_LIBRARIES
    PurC::PurC
    PurC::DOMRuler
    PurC::CSSEng
    ${GLIB_LIBRARIES}
)

PURC_EXECUTABLE(layout_test)
PURC_COMPUTE_SOURCES(layout_test)

# layout_pcdom_test
PURC_EXECUTABLE_DECLARE(layout_pcdom_test)

list(APPEND layout_pcdom_test_PRIVATE_INCLUDE_DIRECTORIES
    "${DOMRULER_DIR}/include"
    "${DOMRULER_DIR}/src"
    "${FORWARDING_HEADERS_DIR}/domruler"
)

list(APPEND layout_pcdom_test_SYSTEM_INCLUDE_DIRECTORIES
    "${CSSEng_INCLUDE_DIRS}"
    "${GLIB_INCLUDE_DIRS}"
)

list(APPEND layout_pcdom_test_SOURCES
    layout_pcdom_test.c
)

set(layout_pcdom_test_LIBRARIES
    PurC::PurC
    PurC::DOMRuler
    PurC::CSSEng
    ${GLIB_LIBRARIES}
)

PURC_EXECUTABLE(layout_pcdom_test)
PURC_COMPUTE_SOURCES(layout_pcdom_test)

