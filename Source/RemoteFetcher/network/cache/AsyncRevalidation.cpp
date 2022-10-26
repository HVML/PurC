/*
 * Copyright (C) 2019 Igalia S.L.
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
#include "AsyncRevalidation.h"

#if ENABLE(NETWORK_CACHE_STALE_WHILE_REVALIDATE)
#include "CacheValidation.h"
#include "ResourceRequest.h"

namespace PurCFetcher {
namespace NetworkCache {

static inline PurCFetcher::ResourceRequest constructRevalidationRequest(const Key& key, const PurCFetcher::ResourceRequest& request, const Entry& entry)
{
    PurCFetcher::ResourceRequest revalidationRequest = request;
    if (!key.partition().isEmpty())
        revalidationRequest.setCachePartition(key.partition());
    ASSERT_WITH_MESSAGE(key.range().isEmpty(), "range is not supported");

    revalidationRequest.makeUnconditional();
    auto eTag = entry.response().httpHeaderField(PurCFetcher::HTTPHeaderName::ETag);
    if (!eTag.isEmpty())
        revalidationRequest.setHTTPHeaderField(PurCFetcher::HTTPHeaderName::IfNoneMatch, eTag);

    auto lastModified = entry.response().httpHeaderField(PurCFetcher::HTTPHeaderName::LastModified);
    if (!lastModified.isEmpty())
        revalidationRequest.setHTTPHeaderField(PurCFetcher::HTTPHeaderName::IfModifiedSince, lastModified);

    revalidationRequest.setPriority(PurCFetcher::ResourceLoadPriority::Low);

    return revalidationRequest;
}

void AsyncRevalidation::cancel()
{
    if (m_load)
        m_load->cancel();
}

void AsyncRevalidation::staleWhileRevalidateEnding()
{
    if (m_completionHandler)
        m_completionHandler(Result::Timeout);
}

AsyncRevalidation::AsyncRevalidation(Cache& cache, const GlobalFrameID& frameID, const PurCFetcher::ResourceRequest& request, std::unique_ptr<NetworkCache::Entry>&& entry, Optional<NavigatingToAppBoundDomain> isNavigatingToAppBoundDomain, CompletionHandler<void(Result)>&& handler)
    : m_timer(*this, &AsyncRevalidation::staleWhileRevalidateEnding)
    , m_completionHandler(WTFMove(handler))
{
    auto key = entry->key();
    auto revalidationRequest = constructRevalidationRequest(key, request, *entry.get());
    auto age = PurCFetcher::computeCurrentAge(entry->response(), entry->timeStamp());
    auto lifetime = PurCFetcher::computeFreshnessLifetimeForHTTPFamily(entry->response(), entry->timeStamp());
    auto responseMaxStaleness = entry->response().cacheControlStaleWhileRevalidate();
    ASSERT(responseMaxStaleness);
    m_timer.startOneShot(*responseMaxStaleness + (lifetime - age));
    m_load = makeUnique<SpeculativeLoad>(cache, frameID, WTFMove(revalidationRequest), WTFMove(entry), isNavigatingToAppBoundDomain, [this, key, revalidationRequest](auto&& revalidatedEntry) {
        ASSERT(!revalidatedEntry || !revalidatedEntry->needsValidation());
        ASSERT(!revalidatedEntry || revalidatedEntry->key() == key);
        if (m_completionHandler)
            m_completionHandler(revalidatedEntry ? Result::Success : Result::Failure);
    });
}

} // namespace NetworkCache
} // namespace PurCFetcher

#endif // ENABLE(NETWORK_CACHE_STALE_WHILE_REVALIDATE)
