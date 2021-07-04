# Append the all C files in the specified directory list to the source list
MACRO(CHECK_THREAD_LOCAL_STORAGE_KEYWORD)
    set(CMAKE_TRY_COMPILE_TARGET_TYPE EXECUTABLE)
    set(WTF_TLS_KEYWORD_TYPE "TLS_KEYWORD_TYPE_NONE")
    file(WRITE "${CMAKE_BINARY_DIR}/check_tls_storage_class.c"
    "
#if defined(__MINGW32__) && !(__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#error This MinGW version has broken __thread support
#endif
#ifdef __OpenBSD__
#error OpenBSD has broken __thread support
#endif
__thread int test;
int main (void) { return 0; }
    ")
    try_compile(_TLS_STORAGE_CLASS
            "${CMAKE_BINARY_DIR}/check_tls_storage_class"
            "${CMAKE_BINARY_DIR}/check_tls_storage_class.c"
            OUTPUT_VARIABLE __output)
    if (NOT _TLS_STORAGE_CLASS)
        message(STATUS "Compile check_tls_storage_class.c: ${__output}")
        file(WRITE "${CMAKE_BINARY_DIR}/check_tls_declaration_specifier.c"
                "
int __declspec(thread) test;
int main (void) { return 0; }
                ")
        try_compile(_TLS_DECL_SPEC
                "${CMAKE_BINARY_DIR}/check_tls_declaration_specifier"
                "${CMAKE_BINARY_DIR}/check_tls_declaration_specifier.c"
                OUTPUT_VARIABLE __output)
        if (_TLS_DECL_SPEC)
            set(WTF_TLS_KEYWORD_TYPE "TLS_KEYWORD_TYPE_DECL_SPEC")
        else ()
            message(STATUS "compile check_tls_declaration_specifier.c: ${__output}")
        endif ()
        unset(_TLS_DECL_SPEC)
    else ()
        set(WTF_TLS_KEYWORD_TYPE "TLS_KEYWORD_TYPE_STORAGE_CLASS")
    endif ()
    unset(_TLS_STORAGE_CLASS)
    unset (__output)
ENDMACRO()
