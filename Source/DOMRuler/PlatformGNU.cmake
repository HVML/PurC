set(DOMRuler_OUTPUT_NAME domruler)

list(APPEND DOMRuler_LIBRARIES
    DOMRuler::WTF
    PurC::PurC
    CSSEng::CSSEng
    ${GLIB_LIBRARIES}
)

configure_file(domruler.pc.in ${DOMRuler_PKGCONFIG_FILE} @ONLY)
install(FILES "${DOMRuler_PKGCONFIG_FILE}"
        DESTINATION "${LIB_INSTALL_DIR}/pkgconfig"
)

