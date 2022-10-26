/*
 * Copyright (C) 2010-2016 Apple Inc. All rights reserved.
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
#include "DownloadProxy.h"

#include <wtf/FileSystem.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace PurCFetcher {

DownloadProxy::DownloadProxy()
{
}

DownloadProxy::~DownloadProxy()
{
}

void DownloadProxy::didStart(const ResourceRequest&, const String&)
{
}

void DownloadProxy::didReceiveAuthenticationChallenge(AuthenticationChallenge&&, uint64_t)
{
}

void DownloadProxy::willSendRequest(ResourceRequest&&, const ResourceResponse&)
{
}

void DownloadProxy::didReceiveResponse(const ResourceResponse&)
{
}

void DownloadProxy::didReceiveData(uint64_t, uint64_t, uint64_t)
{
}

void DownloadProxy::decideDestinationWithSuggestedFilenameAsync(DownloadID, const String&)
{
}

void DownloadProxy::didCreateDestination(const String&)
{
}

void DownloadProxy::didFinish()
{
}

void DownloadProxy::didFail(const ResourceError&, const IPC::DataReference&)
{
}

void DownloadProxy::didCancel(const IPC::DataReference&)
{
}

} // namespace PurCFetcher

