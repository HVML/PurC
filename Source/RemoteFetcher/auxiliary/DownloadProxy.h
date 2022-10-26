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

#pragma once

#include "APIObject.h"
#include "Connection.h"
#include "DownloadID.h"
#include "SandboxExtension.h"
#include "ResourceRequest.h"
#include <wtf/Forward.h>
#include <wtf/Ref.h>
#include <wtf/WeakPtr.h>

namespace API {
class Data;
class FrameInfo;
}

namespace PurCFetcher {
class AuthenticationChallenge;
class IntRect;
class ProtectionSpace;
class ResourceError;
class ResourceResponse;
}

namespace PurCFetcher {

class DownloadID;

class DownloadProxy : public API::ObjectImpl<API::Object::Type::Download>, public IPC::MessageReceiver {
public:

    template<typename... Args> static Ref<DownloadProxy> create(Args&&... args)
    {
        return adoptRef(*new DownloadProxy(std::forward<Args>(args)...));
    }
    ~DownloadProxy();

private:
    explicit DownloadProxy();

    // IPC::MessageReceiver
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) override;

    // Message handlers.
    void didStart(const PurCFetcher::ResourceRequest&, const String& suggestedFilename);
    void didReceiveAuthenticationChallenge(PurCFetcher::AuthenticationChallenge&&, uint64_t challengeID);
    void didReceiveResponse(const PurCFetcher::ResourceResponse&);
    void didReceiveData(uint64_t bytesWritten, uint64_t totalBytesWritten, uint64_t totalBytesExpectedToWrite);
    void shouldDecodeSourceDataOfMIMEType(const String& mimeType, bool& result);
    void didCreateDestination(const String& path);
    void didFinish();
    void didFail(const PurCFetcher::ResourceError&, const IPC::DataReference& resumeData);
    void didCancel(const IPC::DataReference& resumeData);
    void willSendRequest(PurCFetcher::ResourceRequest&& redirectRequest, const PurCFetcher::ResourceResponse& redirectResponse);
    void decideDestinationWithSuggestedFilenameAsync(DownloadID, const String& suggestedFilename);
};

} // namespace PurCFetcher
