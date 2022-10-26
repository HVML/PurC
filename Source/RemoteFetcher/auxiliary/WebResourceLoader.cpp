/*
 * Copyright (C) 2012-2018 Apple Inc. All rights reserved.
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
#include "WebResourceLoader.h"

namespace PurCFetcher {

Ref<WebResourceLoader> WebResourceLoader::create(Ref<ResourceLoader>&& coreLoader, const TrackingParameters& trackingParameters)
{
    return adoptRef(*new WebResourceLoader(WTFMove(coreLoader), trackingParameters));
}

WebResourceLoader::WebResourceLoader(Ref<PurCFetcher::ResourceLoader>&&, const TrackingParameters&)
{
}

WebResourceLoader::~WebResourceLoader()
{
}

IPC::Connection* WebResourceLoader::messageSenderConnection() const
{
    return nullptr;
}

uint64_t WebResourceLoader::messageSenderDestinationID() const
{
    return 0;
}

void WebResourceLoader::willSendRequest(PurCFetcher::ResourceRequest&&, IPC::FormDataReference&&, PurCFetcher::ResourceResponse&&)
{
}

void WebResourceLoader::didSendData(uint64_t, uint64_t)
{
}

void WebResourceLoader::didReceiveResponse(const PurCFetcher::ResourceResponse&, bool)
{
}

void WebResourceLoader::didReceiveData(IPC::DataReference&&, int64_t)
{
}

void WebResourceLoader::didReceiveSharedBuffer(IPC::SharedBufferDataReference&&, int64_t)
{
}

void WebResourceLoader::didFinishResourceLoad(const PurCFetcher::NetworkLoadMetrics&)
{
}

void WebResourceLoader::didFailResourceLoad(const PurCFetcher::ResourceError&)
{
}

void WebResourceLoader::didFailServiceWorkerLoad(const PurCFetcher::ResourceError&)
{
}

void WebResourceLoader::serviceWorkerDidNotHandle()
{
}

void WebResourceLoader::didBlockAuthenticationChallenge()
{
}


void WebResourceLoader::stopLoadingAfterXFrameOptionsOrContentSecurityPolicyDenied(const PurCFetcher::ResourceResponse&)
{
}


void WebResourceLoader::deferReceivingSharedBuffer(IPC::SharedBufferDataReference&&, int64_t)
{
}

void WebResourceLoader::processReceivedData(const char*, size_t, int64_t)
{
}


#if ENABLE(SHAREABLE_RESOURCE)
void WebResourceLoader::didReceiveResource(const ShareableResource::Handle&)
{
}
#endif

} // namespace PurCFetcher

#undef RELEASE_LOG_IF_ALLOWED
