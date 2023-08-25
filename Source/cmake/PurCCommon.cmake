# -----------------------------------------------------------------------------
# This file is included individually from various subdirectories (JSC, WTF,
# XCore, XKit) in order to allow scripts to build only part of PURC.
# We want to run this file only once.
# -----------------------------------------------------------------------------
if (NOT HAS_RUN_PURC_COMMON)
    set(HAS_RUN_PURC_COMMON TRUE)

    # -----------------------------------------------------------------------------
    # Find common packages (used by all ports)
    # -----------------------------------------------------------------------------
    if (WIN32)
        list(APPEND CMAKE_PROGRAM_PATH $ENV{SystemDrive}/cygwin/bin)
    endif ()

    set(Python_ADDITIONAL_VERSIONS 3)
    find_package(Python2 2.7.0 REQUIRED)

    # -----------------------------------------------------------------------------
    # Helper macros and feature defines
    # -----------------------------------------------------------------------------

    # To prevent multiple inclusion, most modules should be included once here.
    include(CheckCCompilerFlag)
    include(CheckCXXCompilerFlag)
    include(CheckCXXSourceCompiles)
    include(CheckFunctionExists)
    include(CheckIncludeFile)
    include(CheckSymbolExists)
    include(CheckStructHasMember)
    include(CheckTypeSize)
    include(CMakeDependentOption)
    include(CMakeParseArguments)
    include(ProcessorCount)
    include(VersioningUtils)

    include(PurCPackaging)
    include(PurCMacros)
    include(PurCFS)
    include(PurCCCache)
    include(PurCFeatures)
    include(PurCCompilerFlags)
    include(PurCStaticAnalysis)
    include(PurCFindPackage)

    include(OptionsCommon)
    include(Options${PORT})

    # -----------------------------------------------------------------------------
    # Job pool to avoid running too many memory hungry linker processes
    # -----------------------------------------------------------------------------
    if (${CMAKE_BUILD_TYPE} STREQUAL "Release" OR ${CMAKE_BUILD_TYPE} STREQUAL "MinSizeRel")
        set_property(GLOBAL PROPERTY JOB_POOLS link_pool_jobs=4)
    else ()
        set_property(GLOBAL PROPERTY JOB_POOLS link_pool_jobs=2)
    endif ()
    set(CMAKE_JOB_POOL_LINK link_pool_jobs)

    # -----------------------------------------------------------------------------
    # config.h
    # -----------------------------------------------------------------------------
    CREATE_CONFIGURATION_HEADER()
endif ()
