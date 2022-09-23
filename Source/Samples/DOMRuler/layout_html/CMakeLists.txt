include(PurCCommon)
include(target/CSSEng)
include(target/DOMRuler)

# layout_html
PURC_EXECUTABLE_DECLARE(layout_html)

list(APPEND layout_html_PRIVATE_INCLUDE_DIRECTORIES
    "${DOMRuler_FRAMEWORK_HEADERS_DIR}"
)

list(APPEND layout_html_SYSTEM_INCLUDE_DIRECTORIES
    "${CSSEng_FRAMEWORK_HEADERS_DIR}"
)

list(APPEND layout_html_SOURCES
    layout_html.c
)

set(layout_html_LIBRARIES
    PurC::PurC
    PurC::DOMRuler
    PurC::CSSEng
)

PURC_EXECUTABLE(layout_html)
PURC_COMPUTE_SOURCES(layout_html)

