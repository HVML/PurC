/*
 * Copyright (C) 2010, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2020~2021 Beijing FMSoft Technologies Co., Ltd.
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ProcessLauncher.h"

#include <glib.h>

#include <wtf/FileSystem.h>
#include <wtf/StdLibExtras.h>
#include <wtf/UniStdExtras.h>
#include <wtf/SystemTracing.h>
#include <wtf/WorkQueue.h>
#include <wtf/glib/GLibUtilities.h>
#include <wtf/glib/GUniquePtr.h>


#define FETCHER_NAME   "purc_fetcher"

namespace PurCFetcher {

static void childSetupFunction(gpointer userData)
{
    int socket = GPOINTER_TO_INT(userData);
    close(socket);
}

static String getExecutablePath()
{
    CString executablePath = getCurrentExecutablePath();
    if (!executablePath.isNull())
        return FileSystem::directoryName(FileSystem::stringFromFileSystemRepresentation(executablePath.data()));
    return { };
}

#define PURC_ENVV_FETCHER_EXEC_PATH "PURC_FETCHER_EXEC_PATH"

static String findPurCProcess(const char* processName)
{
    static const char* execDirectory = g_getenv(PURC_ENVV_FETCHER_EXEC_PATH);
    if (execDirectory) {
        String processPath = FileSystem::pathByAppendingComponent(FileSystem::stringFromFileSystemRepresentation(execDirectory), processName);
        if (FileSystem::fileExists(processPath))
            return processPath;
    }

    static String executablePath = getExecutablePath();
    if (!executablePath.isNull()) {
        String processPath = FileSystem::pathByAppendingComponent(executablePath, processName);
        if (FileSystem::fileExists(processPath))
            return processPath;
    }

    return FileSystem::pathByAppendingComponent(FileSystem::stringFromFileSystemRepresentation(PURC_LIBEXEC_DIR), processName);
}

String executablePathOfFetcherProcess()
{
    return findPurCProcess(FETCHER_NAME);
}

ProcessLauncher::ProcessLauncher(Client* client, LaunchOptions&& launchOptions)
    : m_client(client)
    , m_launchOptions(WTFMove(launchOptions))
{
    tracePoint(ProcessLaunchStart);
    launchProcess();
}

void ProcessLauncher::didFinishLaunchingProcess(ProcessID processIdentifier, IPC::Connection::Identifier identifier)
{
    tracePoint(ProcessLaunchEnd);
    m_processIdentifier = processIdentifier;
    m_isLaunching = false;

    if (m_client) {
        m_client->didFinishLaunching(this, identifier);
    }
}

void ProcessLauncher::invalidate()
{
    m_client = 0;
    platformInvalidate();
}

void ProcessLauncher::launchProcess()
{
    IPC::Connection::SocketPair socketPair = IPC::Connection::createPlatformConnection(IPC::Connection::ConnectionOptions::SetCloexecOnServer);

    String executablePath;
    CString realExecutablePath;

    switch (m_launchOptions.processType) {
    case ProcessLauncher::ProcessType::Fetcher:
        executablePath = executablePathOfFetcherProcess();
        break;
    default:
        ASSERT_NOT_REACHED();
        return;
    }

    realExecutablePath = FileSystem::fileSystemRepresentation(executablePath);
    GUniquePtr<gchar> processIdentifier(g_strdup_printf("%" PRIu64, m_launchOptions.processIdentifier.toUInt64()));
    GUniquePtr<gchar> webkitSocket(g_strdup_printf("%d", socketPair.client));
    unsigned nargs = 5; // size of the argv array for g_spawn_async()

    char** argv = g_newa(char*, nargs);
    unsigned i = 0;
    argv[i++] = const_cast<char*>(realExecutablePath.data());
    argv[i++] = processIdentifier.get();
    argv[i++] = webkitSocket.get();
    argv[i++] = nullptr;
    argv[i++] = nullptr;

    GRefPtr<GSubprocessLauncher> launcher = adoptGRef(g_subprocess_launcher_new(G_SUBPROCESS_FLAGS_INHERIT_FDS));
    g_subprocess_launcher_set_child_setup(launcher.get(), childSetupFunction, GINT_TO_POINTER(socketPair.server), nullptr);
    g_subprocess_launcher_take_fd(launcher.get(), socketPair.client, socketPair.client);

    GUniqueOutPtr<GError> error;
    GRefPtr<GSubprocess> process;

    process = adoptGRef(g_subprocess_launcher_spawnv(launcher.get(), argv, &error.outPtr()));

    if (!process.get())
        g_error("Unable to fork a new child process: %s", error->message);

    const char* processIdStr = g_subprocess_get_identifier(process.get());
    m_processIdentifier = g_ascii_strtoll(processIdStr, nullptr, 0);
    RELEASE_ASSERT(m_processIdentifier);

    // Don't expose the parent socket to potential future children.
    if (!setCloseOnExec(socketPair.client))
        RELEASE_ASSERT_NOT_REACHED();

    didFinishLaunchingProcess(m_processIdentifier, socketPair.server);
}

void ProcessLauncher::terminateProcess()
{
    if (m_isLaunching) {
        invalidate();
        return;
    }

    if (!m_processIdentifier)
        return;

    kill(m_processIdentifier, SIGKILL);
    m_processIdentifier = 0;
}

void ProcessLauncher::platformInvalidate()
{
}


} // namespace PurCFetcher
