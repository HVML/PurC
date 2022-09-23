set(CSSEng_OUTPUT_NAME csseng)

configure_file(csseng.pc.in ${CSSEng_PKGCONFIG_FILE} @ONLY)
install(FILES "${CSSEng_PKGCONFIG_FILE}"
        DESTINATION "${LIB_INSTALL_DIR}/pkgconfig"
)

