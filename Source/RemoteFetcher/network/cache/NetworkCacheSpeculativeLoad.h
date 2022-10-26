/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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

#if ENABLE(NETWORK_CACHE_SPECULATIVE_REVALIDATION) || ENABLE(NETWORK_CACHE_STALE_WHILE_REVALIDATE)

#include "NetworkCache.h"
#include "NetworkCacheEntry.h"
#include "NetworkLoadClient.h"
#include "PolicyDecision.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "SharedBuffer.h"
#include <wtf/CompletionHandler.h>

namespace PurCFetcher {

class NetworkLoad;

namespace NetworkCache {

class SpeculativeLoad final : public NetworkLoadClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    using RevalidationCompletionHandler = CompletionHandler<void(std::unique_ptr<NetworkCache::Entry>)>;
    SpeculativeLoad(Cache&, const GlobalFrameID&, const PurCFetcher::ResourceRequest&, std::unique_ptr<NetworkCache::Entry>, Optional<NavigatingToAppBoundDomain>, RevalidationCompletionHandler&&);

    virtual ~SpeculativeLoad();

    const PurCFetcher::ResourceRequest& originalRequest() const { return m_originalRequest; }

    void cancel();

private:
    // NetworkLoadClient.
    void didSendData(unsigned long long, unsigned long long) override { }
    bool isSynchronous() const override { return false; }
    bool isAllowedToAskUserForCredentials() const final { return false; }
    void willSendRedirectedRequest(PurCFetcher::ResourceRequest&&, PurCFetcher::ResourceRequest&& redirectRequest, PurCFetcher::ResourceResponse&& redirectResponse) override;
    void didReceiveResponse(PurCFetcher::ResourceResponse&&, ResponseCompletionHandler&&) override;
    void didReceiveBuffer(Ref<PurCFetcher::SharedBuffer>&&, int reportedEncodedDataLength) override;
    void didFinishLoading(const PurCFetcher::NetworkLoadMetrics&) override;
    void didFailLoading(const PurCFetcher::ResourceError&) override;

    void didComplete();

    Ref<Cache> m_cache;
    RevalidationCompletionHandler m_completionHandler;
    PurCFetcher::ResourceRequest m_originalRequest;

    std::unique_ptr<NetworkLoad> m_networkLoad;

    PurCFetcher::ResourceResponse m_response;

    RefPtr<PurCFetcher::SharedBuffer> m_bufferedDataForCache;
    std::unique_ptr<NetworkCache::Entry> m_cacheEntry;
    bool m_didComplete { false };
};

bool requestsHeadersMatch(const PurCFetcher::ResourceRequest& speculativeValidationRequest, const PurCFetcher::ResourceRequest& actualRequest);

} // namespace NetworkCache
} // namespace PurCFetcher

#endif // ENABLE(NETWORK_CACHE_SPECULATIVE_REVALIDATION) || ENABLE(NETWORK_CACHE_STALE_WHILE_REVALIDATE)
