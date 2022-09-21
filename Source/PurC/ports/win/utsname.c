/*
 * @file utsname.c
 * @author Vincent Wei (https://github.com/VincentWei)
 * @date 2022/09/07
 * @brief Equivalent implementation of uname() for Windows.
 *
 * Copyright (C) 2022 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "private/ports.h"

#include <windows.h>
#include <stdio.h>

int uname(struct utsname *buf);
{
    SYSTEM_INFO systemInfo;
    OSVERSIONINFO versionInfo;
    DWORD computerNameLength = sizeof(buf->nodename);

    GetSystemInfo(&systemInfo);
    versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&versionInfo);

    /* sysname */
    switch (versionInfo.dwPlatformId) {
    case VER_PLATFORM_WIN32s:
        strcpy(buf->sysname, "WIN32S");
        break;
    case VER_PLATFORM_WIN32_WINDOWS:
        strcpy(buf->sysname, "WIN95");
        break;
    case VER_PLATFORM_WIN32_NT:
        strcpy(buf->sysname, "WINNT");
        break;
    default:
        strcpy(buf->sysname, "UNKNOWN_OS");
        break;
    }

    /* nodename */
    GetComputerName((LPTSTR)&buf->nodename, &computerNameLength);

    /* release */
    sprintf(buf->release, "%d.%d",
            versionInfo.dwMajorVersion, versionInfo.dwMinorVersion);

    /* version */
    sprintf(buf->version, "%d", versionInfo.dwBuildNumber);

    /* machine */
    switch (systemInfo.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_INTEL:
        sprintf(buf->machine, "x86");
        break;

    case PROCESSOR_ARCHITECTURE_AMD64:
        sprintf(buf->machine, "x86_64");
        break;

    case PROCESSOR_ARCHITECTURE_IA64:
        sprintf(buf->machine, "IA64");
        break;

    case PROCESSOR_ARCHITECTURE_ARM:
        sprintf(buf->machine, "ARM");
        break;

    case PROCESSOR_ARCHITECTURE_ARM64:
        sprintf(buf->machine, "ARM64");
        break;

    case PROCESSOR_ARCHITECTURE_MIPS:
        sprintf(buf->machine, "MIPS");
        break;

    case PROCESSOR_ARCHITECTURE_ALPHA:
        sprintf(buf->machine, "ALPHA");
        break;

    case PROCESSOR_ARCHITECTURE_PPC:
        sprintf(buf->machine, "PowerPC");
        break;

    default:
        printf("UNKNOWN_PROCESSOR");
        break;
    }

    return 0;
}

