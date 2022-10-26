/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#include "WebCoreArgumentCoders.h"
#include "Connection.h"
#include "MessageSender.h"
#include "ShareableResource.h"
#include "WebPageProxyIdentifier.h"
#include "FrameIdentifier.h"
#include "PageIdentifier.h"
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace IPC {
class SharedBufferDataReference;
class FormDataReference;
}

namespace PurCFetcher {
class NetworkLoadMetrics;
class ResourceError;
class ResourceLoader;
class ResourceRequest;
class ResourceResponse;
}

namespace PurCFetcher {

typedef uint64_t ResourceLoadIdentifier;

class WebResourceLoader : public RefCounted<WebResourceLoader>, public IPC::MessageSender {
public:
    struct TrackingParameters {
        WebPageProxyIdentifier webPageProxyID;
        PurCFetcher::PageIdentifier pageID;
        PurCFetcher::FrameIdentifier frameID;
        ResourceLoadIdentifier resourceID { 0 };
    };

    static Ref<WebResourceLoader> create(Ref<PurCFetcher::ResourceLoader>&&, const TrackingParameters&);

    ~WebResourceLoader();

private:
    WebResourceLoader(Ref<PurCFetcher::ResourceLoader>&&, const TrackingParameters&);

    void didReceiveWebResourceLoaderMessage(IPC::Connection&, IPC::Decoder&);

    // IPC::MessageSender
    IPC::Connection* messageSenderConnection() const override;
    uint64_t messageSenderDestinationID() const override;

    void willSendRequest(PurCFetcher::ResourceRequest&&, IPC::FormDataReference&&, PurCFetcher::ResourceResponse&&);
    void didSendData(uint64_t, uint64_t);
    void didReceiveResponse(const PurCFetcher::ResourceResponse&, bool);
    void didReceiveData(IPC::DataReference&&, int64_t);
    void didReceiveSharedBuffer(IPC::SharedBufferDataReference&&, int64_t);
    void didFinishResourceLoad(const PurCFetcher::NetworkLoadMetrics&);
    void didFailResourceLoad(const PurCFetcher::ResourceError&);
    void didFailServiceWorkerLoad(const PurCFetcher::ResourceError&);
    void serviceWorkerDidNotHandle();
    void didBlockAuthenticationChallenge();

    void stopLoadingAfterXFrameOptionsOrContentSecurityPolicyDenied(const PurCFetcher::ResourceResponse&);

    void deferReceivingSharedBuffer(IPC::SharedBufferDataReference&&, int64_t);
    void processReceivedData(const char*, size_t, int64_t);

#if ENABLE(SHAREABLE_RESOURCE)
    void didReceiveResource(const ShareableResource::Handle&);
#endif
};

} // namespace PurCFetcher
