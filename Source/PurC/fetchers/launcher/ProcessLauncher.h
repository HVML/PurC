/*
 * Copyright (C) 2010-2019 Apple Inc. All rights reserved.
 * Copyright (C) 2020-2021 Beijing FMSoft Technologies Co., Ltd.
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

#pragma once

#include "fetcher-messages-basic.h"

#include "Connection.h"
#include <wtf/HashMap.h>
#include <wtf/ProcessID.h>
#include <wtf/RefPtr.h>
#include <wtf/Threading.h>
#include <wtf/WeakPtr.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace PurCFetcher {

class ProcessLauncher : public ThreadSafeRefCounted<ProcessLauncher>, public CanMakeWeakPtr<ProcessLauncher> {
public:
    class Client {
    public:
        virtual ~Client() { }

        virtual void didFinishLaunching(ProcessLauncher*, IPC::Connection::Identifier) = 0;
    };

    enum class ProcessType {
        Fetcher,
    };

    struct LaunchOptions {
        ProcessType processType;
        PurCFetcher::ProcessIdentifier processIdentifier;
        HashMap<String, String> extraInitializationData;
        bool nonValidInjectedCodeAllowed { false };
        bool shouldMakeProcessLaunchFailForTesting { false };
        CString customWebContentServiceBundleIdentifier;
    };

    static Ref<ProcessLauncher> create(Client* client, LaunchOptions&& launchOptions)
    {
        return adoptRef(*new ProcessLauncher(client, WTFMove(launchOptions)));
    }

    bool isLaunching() const { return m_isLaunching; }
    ProcessID processIdentifier() const { return m_processIdentifier; }

    void terminateProcess();
    void invalidate();

private:
    ProcessLauncher(Client*, LaunchOptions&&);

    void launchProcess();
    void didFinishLaunchingProcess(ProcessID, IPC::Connection::Identifier);

    void platformInvalidate();

    Client* m_client;

    const LaunchOptions m_launchOptions;
    bool m_isLaunching { true };
    ProcessID m_processIdentifier { 0 };
};

} // namespace PurCFetcher
