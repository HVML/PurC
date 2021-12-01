find_library(COCOA_LIBRARY Cocoa)
find_library(COREFOUNDATION_LIBRARY CoreFoundation)
find_library(READLINE_LIBRARY Readline)
list(APPEND WTF_LIBRARIES
    ${GLIB_GIO_LIBRARIES}
    ${GLIB_GOBJECT_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${COREFOUNDATION_LIBRARY}
    ${COCOA_LIBRARY}
    ${READLINE_LIBRARY}
)

list(APPEND WTF_SYSTEM_INCLUDE_DIRECTORIES
    ${GIO_UNIX_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
)

list(APPEND WTF_PUBLIC_HEADERS
    glib/GLibUtilities.h
    glib/GMutexLocker.h
    glib/GRefPtr.h
    glib/GSocketMonitor.h
    glib/GTypedefs.h
    glib/GUniquePtr.h
    glib/RunLoopSourcePriority.h
    glib/SocketConnection.h
    glib/WTFGType.h

    WeakObjCPtr.h

    cf/CFURLExtras.h
    cf/TypeCastsCF.h

    cocoa/CrashReporter.h
    cocoa/Entitlements.h
    cocoa/NSURLExtras.h
    cocoa/RuntimeApplicationChecksCocoa.h
    cocoa/SoftLinking.h
    cocoa/VectorCocoa.h

    darwin/WeakLinking.h

    spi/cf/CFBundleSPI.h
    spi/cf/CFStringSPI.h

    spi/cocoa/CFXPCBridgeSPI.h
    spi/cocoa/CrashReporterClientSPI.h
    spi/cocoa/MachVMSPI.h
    spi/cocoa/NSLocaleSPI.h
    spi/cocoa/SecuritySPI.h
    spi/cocoa/objcSPI.h

    spi/darwin/DataVaultSPI.h
    spi/darwin/OSVariantSPI.h
    spi/darwin/ProcessMemoryFootprint.h
    spi/darwin/SandboxSPI.h
    spi/darwin/XPCSPI.h
    spi/darwin/dyldSPI.h

    spi/mac/MetadataSPI.h

)

list(APPEND WTF_SOURCES
    generic/MainThreadGeneric.cpp
    generic/WorkQueueGeneric.cpp

    glib/FileSystemGlib.cpp
    glib/GLibUtilities.cpp
    glib/GRefPtr.cpp
    glib/GSocketMonitor.cpp
    glib/RunLoopGLib.cpp
    glib/SocketConnection.cpp
    glib/URLGLib.cpp

    posix/OSAllocatorPOSIX.cpp
    posix/ThreadingPOSIX.cpp

    text/unix/TextBreakIteratorInternalICUUnix.cpp

    unix/CPUTimeUnix.cpp
    unix/LanguageUnix.cpp
    unix/UniStdExtrasUnix.cpp

#    BlockObjCExceptions.mm
#
#    cf/CFURLExtras.cpp
#    cf/FileSystemCF.cpp
#    cf/LanguageCF.cpp
#    cf/RunLoopCF.cpp
#    cf/RunLoopTimerCF.cpp
#    cf/SchedulePairCF.cpp
#    cf/URLCF.cpp
#
#    cocoa/AutodrainedPool.cpp
#    cocoa/CPUTimeCocoa.cpp
#    cocoa/CrashReporter.cpp
#    cocoa/Entitlements.mm
#    cocoa/FileSystemCocoa.mm
#    cocoa/LanguageCocoa.mm
#    cocoa/MachSendRight.cpp
#    cocoa/MainThreadCocoa.mm
#    cocoa/MemoryFootprintCocoa.cpp
#    cocoa/MemoryPressureHandlerCocoa.mm
#    cocoa/NSURLExtras.mm
#    cocoa/ResourceUsageCocoa.cpp
#    cocoa/RuntimeApplicationChecksCocoa.cpp
#    cocoa/SystemTracingCocoa.cpp
#    cocoa/URLCocoa.mm
#    cocoa/WorkQueueCocoa.cpp
#
#    mac/FileSystemMac.mm
#    mac/SchedulePairMac.mm
#
#    posix/FileSystemPOSIX.cpp
#    posix/OSAllocatorPOSIX.cpp
#    posix/ThreadingPOSIX.cpp
#
#    text/cf/AtomStringImplCF.cpp
#    text/cf/StringCF.cpp
#    text/cf/StringImplCF.cpp
#    text/cf/StringViewCF.cpp
#
#    text/cocoa/StringCocoa.mm
#    text/cocoa/StringImplCocoa.mm
#    text/cocoa/StringViewCocoa.mm
)

file(COPY mac/MachExceptions.defs DESTINATION ${WTF_DERIVED_SOURCES_DIR})

add_custom_command(
    OUTPUT
        ${WTF_DERIVED_SOURCES_DIR}/MachExceptionsServer.h
        ${WTF_DERIVED_SOURCES_DIR}/mach_exc.h
        ${WTF_DERIVED_SOURCES_DIR}/mach_excServer.c
        ${WTF_DERIVED_SOURCES_DIR}/mach_excUser.c
    MAIN_DEPENDENCY mac/MachExceptions.defs
    WORKING_DIRECTORY ${WTF_DERIVED_SOURCES_DIR}
    COMMAND mig -sheader MachExceptionsServer.h MachExceptions.defs
    VERBATIM)
list(APPEND WTF_SOURCES
    ${WTF_DERIVED_SOURCES_DIR}/mach_excServer.c
    ${WTF_DERIVED_SOURCES_DIR}/mach_excUser.c
)

PURC_CREATE_FORWARDING_HEADERS(WebKitLegacy DIRECTORIES ${WebKitLegacy_FORWARDING_HEADERS_DIRECTORIES} FILES ${WebKitLegacy_FORWARDING_HEADERS_FILES})
PURC_CREATE_FORWARDING_HEADERS(WebKit DIRECTORIES ${FORWARDING_HEADERS_DIR}/WebKitLegacy)
