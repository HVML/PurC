set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-DBUILDING_WITH_CMAKE=1)
add_definitions(-DHAVE_CONFIG_H=1)

option(USE_THIN_ARCHIVES "Produce all static libraries as thin archives" ON)
if (USE_THIN_ARCHIVES)
    execute_process(COMMAND ${CMAKE_AR} -V OUTPUT_VARIABLE AR_VERSION)
    if ("${AR_VERSION}" MATCHES "^GNU ar")
        set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> crT <TARGET> <LINK_FLAGS> <OBJECTS>")
        set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> crT <TARGET> <LINK_FLAGS> <OBJECTS>")
        set(CMAKE_CXX_ARCHIVE_APPEND "<CMAKE_AR> rT <TARGET> <LINK_FLAGS> <OBJECTS>")
        set(CMAKE_C_ARCHIVE_APPEND "<CMAKE_AR> rT <TARGET> <LINK_FLAGS> <OBJECTS>")
    endif ()
endif ()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
define_property(TARGET PROPERTY FOLDER INHERITED BRIEF_DOCS "folder" FULL_DOCS "IDE folder name")

set(ARM_TRADITIONAL_DETECTED FALSE)
if (WTF_CPU_ARM)
    set(ARM_THUMB2_TEST_SOURCE
    "
    #if !defined(thumb2) && !defined(__thumb2__)
    #error \"Thumb2 instruction set isn't available\"
    #endif
    int main() {}
   ")

    CHECK_CXX_SOURCE_COMPILES("${ARM_THUMB2_TEST_SOURCE}" ARM_THUMB2_DETECTED)
    if (NOT ARM_THUMB2_DETECTED)
        set(ARM_TRADITIONAL_DETECTED TRUE)
        # See https://bugs.webkit.org/show_bug.cgi?id=159880#c4 for details.
        message(STATUS "Disabling GNU gold linker, because it doesn't support ARM instruction set properly.")
    endif ()
endif ()

# Use ld.lld when building with LTO
CMAKE_DEPENDENT_OPTION(USE_LD_LLD "Use LLD linker" ON
                       "LTO_MODE;NOT USE_LD_GOLD;NOT WIN32" OFF)
if (USE_LD_LLD)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -fuse-ld=lld -Wl,--version ERROR_QUIET OUTPUT_VARIABLE LD_VERSION)
    if ("${LD_VERSION}" MATCHES "LLD")
        string(APPEND CMAKE_EXE_LINKER_FLAGS " -fuse-ld=lld -Wl,--disable-new-dtags")
        string(APPEND CMAKE_SHARED_LINKER_FLAGS " -fuse-ld=lld -Wl,--disable-new-dtags")
        string(APPEND CMAKE_MODULE_LINKER_FLAGS " -fuse-ld=lld -Wl,--disable-new-dtags")
    else ()
        set(USE_LD_LLD OFF)
    endif ()
endif ()

# Use ld.gold if it is available and isn't disabled explicitly
CMAKE_DEPENDENT_OPTION(USE_LD_GOLD "Use GNU gold linker" ON
                       "NOT CXX_ACCEPTS_MFIX_CORTEX_A53_835769;NOT ARM_TRADITIONAL_DETECTED;NOT WIN32;NOT APPLE;NOT USE_LD_LLD" OFF)
if (USE_LD_GOLD)
    execute_process(COMMAND ${CMAKE_C_COMPILER} -fuse-ld=gold -Wl,--version ERROR_QUIET OUTPUT_VARIABLE LD_VERSION)
    if ("${LD_VERSION}" MATCHES "GNU gold")
        string(APPEND CMAKE_EXE_LINKER_FLAGS " -fuse-ld=gold -Wl,--disable-new-dtags")
        string(APPEND CMAKE_SHARED_LINKER_FLAGS " -fuse-ld=gold -Wl,--disable-new-dtags")
        string(APPEND CMAKE_MODULE_LINKER_FLAGS " -fuse-ld=gold -Wl,--disable-new-dtags")
    else ()
        message(WARNING "GNU gold linker isn't available, using the default system linker.")
        set(USE_LD_GOLD OFF)
    endif ()
endif ()

set(ENABLE_DEBUG_FISSION_DEFAULT OFF)
if (USE_LD_GOLD AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    check_cxx_compiler_flag(-gsplit-dwarf CXX_COMPILER_SUPPORTS_GSPLIT_DWARF)
    if (CXX_COMPILER_SUPPORTS_GSPLIT_DWARF)
        set(ENABLE_DEBUG_FISSION_DEFAULT ON)
    endif ()
endif ()

option(DEBUG_FISSION "Use Debug Fission support" ${ENABLE_DEBUG_FISSION_DEFAULT})

if (DEBUG_FISSION)
    if (NOT USE_LD_GOLD)
        message(FATAL_ERROR "Need GNU gold linker for Debug Fission support")
    endif ()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -gsplit-dwarf")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gsplit-dwarf")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gdb-index")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gdb-index")
endif ()

set(GCC_OFFLINEASM_SOURCE_MAP_DEFAULT OFF)
if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(GCC_OFFLINEASM_SOURCE_MAP_DEFAULT ON)
endif ()

option(GCC_OFFLINEASM_SOURCE_MAP
  "Produce debug line information for offlineasm-generated code"
  ${GCC_OFFLINEASM_SOURCE_MAP_DEFAULT})

# Enable the usage of OpenMP.
#  - At this moment, OpenMP is only used as an alternative implementation
#    to native threads for the parallelization of the SVG filters.
if (USE_OPENMP)
    find_package(OpenMP REQUIRED)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif ()

# GTK and WPE use the GNU installation directories as defaults.
if (NOT PORT STREQUAL "GTK" AND NOT PORT STREQUAL "WPE")
    set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib" CACHE PATH "Absolute path to library installation directory")
    set(EXEC_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "Absolute path to executable installation directory")
    set(LIBEXEC_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin" CACHE PATH "Absolute path to install executables executed by the library")
endif ()

# Check whether features.h header exists.
# Including glibc's one defines __GLIBC__, that is used in Platform.h
PURC_CHECK_HAVE_INCLUDE(HAVE_FEATURES_H features.h)

# Check for headers
PURC_CHECK_HAVE_INCLUDE(HAVE_STDATOMIC_H stdatomic.h)
PURC_CHECK_HAVE_INCLUDE(HAVE_ERRNO_H errno.h)
PURC_CHECK_HAVE_INCLUDE(HAVE_STRING_H string.h)
PURC_CHECK_HAVE_INCLUDE(HAVE_MEMORY_H memory.h)
PURC_CHECK_HAVE_INCLUDE(HAVE_UTIME_H utime.h)
PURC_CHECK_HAVE_INCLUDE(HAVE_LANGINFO_H langinfo.h)
PURC_CHECK_HAVE_INCLUDE(HAVE_SYS_IOCTL_H sys/ioctl.h)
PURC_CHECK_HAVE_INCLUDE(HAVE_SYS_SELECT_H sys/select.h)
PURC_CHECK_HAVE_INCLUDE(HAVE_SYS_PARAM_H sys/param.h)
PURC_CHECK_HAVE_INCLUDE(HAVE_MMAP sys/mman.h)
PURC_CHECK_HAVE_INCLUDE(HAVE_PTHREAD_NP_H pthread_np.h)
PURC_CHECK_HAVE_INCLUDE(HAVE_SYS_TIME_H sys/time.h)
PURC_CHECK_HAVE_INCLUDE(HAVE_SYS_TIMEB_H sys/timeb.h)
PURC_CHECK_HAVE_INCLUDE(HAVE_SYS_SYSMACROS_H sys/sysmacros.h)
PURC_CHECK_HAVE_INCLUDE(HAVE_LINUX_MEMFD_H linux/memfd.h)
PURC_CHECK_HAVE_INCLUDE(HAVE_LINUX_FS_H linux/fs.h)
PURC_CHECK_HAVE_INCLUDE(HAVE_SYSLOG_H syslog.h)
PURC_CHECK_HAVE_INCLUDE(HAVE_FCNTL_H fcntl.h)
PURC_CHECK_HAVE_INCLUDE(HAVE_STROPTS_H stropts.h)

# Check for functions
PURC_CHECK_HAVE_FUNCTION(HAVE_ALIGNED_MALLOC _aligned_malloc)
PURC_CHECK_HAVE_FUNCTION(HAVE_ISDEBUGGERPRESENT IsDebuggerPresent)
PURC_CHECK_HAVE_FUNCTION(HAVE_LOCALTIME_R localtime_r)
PURC_CHECK_HAVE_FUNCTION(HAVE_MALLOC_TRIM malloc_trim)
PURC_CHECK_HAVE_FUNCTION(HAVE_STRNSTR strnstr)
PURC_CHECK_HAVE_FUNCTION(HAVE_TIMEGM timegm)
PURC_CHECK_HAVE_FUNCTION(HAVE_VASPRINTF vasprintf)
PURC_CHECK_HAVE_FUNCTION(HAVE_VSYSLOG vsyslog)
PURC_CHECK_HAVE_FUNCTION(HAVE_ALLOCA alloca)
PURC_CHECK_HAVE_FUNCTION(HAVE_REALPATH realpath)
PURC_CHECK_HAVE_FUNCTION(HAVE_STRCASECMP strcasecmp)
PURC_CHECK_HAVE_FUNCTION(HAVE_STRNCASECMP strncasecmp)
PURC_CHECK_HAVE_FUNCTION(HAVE_STRVERSCMP strverscmp)
PURC_CHECK_HAVE_FUNCTION(HAVE_UTIMENSAT utimensat)
PURC_CHECK_HAVE_FUNCTION(HAVE_RANDOM_R random_r)
PURC_CHECK_HAVE_FUNCTION(HAVE_STRERRORNAME_NP strerrorname_np)
PURC_CHECK_HAVE_FUNCTION(HAVE_GET_PROCESS_STATS get_process_stats)
PURC_CHECK_HAVE_FUNCTION(HAVE_POSIX_FALLOCATE posix_fallocate)

# Check for symbols
PURC_CHECK_HAVE_SYMBOL(HAVE_REGEX_H regexec regex.h)
if (NOT (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin"))
PURC_CHECK_HAVE_SYMBOL(HAVE_PTHREAD_MAIN_NP pthread_main_np pthread_np.h)
endif ()
# Windows has signal.h but is missing symbols that are used in calls to signal.
PURC_CHECK_HAVE_SYMBOL(HAVE_SIGNAL_H SIGTRAP signal.h)

# Check for struct members
PURC_CHECK_HAVE_STRUCT(HAVE_STAT_BIRTHTIME "struct stat" st_birthtime sys/stat.h)
PURC_CHECK_HAVE_STRUCT(HAVE_STRUCT_STAT_ST_BLOCKS "struct stat" st_blocks sys/stat.h)
PURC_CHECK_HAVE_STRUCT(HAVE_STRUCT_STAT_ST_BLKSIZE "struct stat" st_blksize sys/stat.h)
PURC_CHECK_HAVE_STRUCT(HAVE_STRUCT_STAT_ST_RDEV "struct stat" st_rdev sys/stat.h)
PURC_CHECK_HAVE_STRUCT(HAVE_STRUCT_STAT_ST_MTIM "struct stat" st_mtim sys/stat.h)
PURC_CHECK_HAVE_STRUCT(HAVE_TM_GMTOFF "struct tm" tm_gmtoff time.h)
PURC_CHECK_HAVE_STRUCT(HAVE_TM_ZONE "struct tm" tm_zone time.h)

# Check for int types
check_type_size("__int128_t" INT128_VALUE)

if (HAVE_INT128_VALUE)
  SET_AND_EXPOSE_TO_BUILD(HAVE_INT128_T INT128_VALUE)
endif ()

check_type_size("uintmax_t" SIZEOF_UINTMAX_T)
set(SIZEOF_UINTMAX_T "${SIZEOF_UINTMAX_T}")

check_type_size("long" SIZEOF_LONG)
math(EXPR BITS_PER_LONG "${SIZEOF_LONG} * 8" OUTPUT_FORMAT DECIMAL)

