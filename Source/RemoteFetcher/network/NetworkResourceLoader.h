/*
 * Copyright (C) 2012-2015 Apple Inc. All rights reserved.
 * Copyright (C) 2019 ~ 2020, Beijing FMSoft Technologies Co., Ltd.
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

#include "DownloadID.h"
#include "MessageSender.h"
#include "NetworkCache.h"
#include "NetworkConnectionToWebProcess.h"
#include "NetworkConnectionToWebProcessMessagesReplies.h"
#include "NetworkLoadClient.h"
#include "NetworkResourceLoadIdentifier.h"
#include "NetworkResourceLoadParameters.h"
#include "AdClickAttribution.h"
#include "ContentSecurityPolicyClient.h"
#include "CrossOriginAccessControl.h"
#include "ResourceResponse.h"
//#include "SecurityPolicyViolationEvent.h"
#include "Timer.h"
#include <wtf/WeakPtr.h>

namespace PurCFetcher {
class FormData;
class NetworkStorageSession;
class ResourceRequest;
}

namespace PurCFetcher {

class NetworkConnectionToWebProcess;
class NetworkLoad;
class NetworkLoadChecker;
class ServiceWorkerFetchTask;
class WebSWServerConnection;

enum class NegotiatedLegacyTLS : bool;

struct ResourceLoadInfo;

namespace NetworkCache {
class Entry;
}

class NetworkResourceLoader final
    : public RefCounted<NetworkResourceLoader>
    , public NetworkLoadClient
    , public IPC::MessageSender
    , public PurCFetcher::ContentSecurityPolicyClient
    , public PurCFetcher::CrossOriginAccessControlCheckDisabler
    , public CanMakeWeakPtr<NetworkResourceLoader> {
public:
    static Ref<NetworkResourceLoader> create(NetworkResourceLoadParameters&& parameters, NetworkConnectionToWebProcess& connection, Messages::NetworkConnectionToWebProcess::PerformSynchronousLoadDelayedReply&& reply = nullptr)
    {
        return adoptRef(*new NetworkResourceLoader(WTFMove(parameters), connection, WTFMove(reply)));
    }
    virtual ~NetworkResourceLoader();

    const PurCFetcher::ResourceRequest& originalRequest() const { return m_parameters.request; }

    NetworkLoad* networkLoad() const { return m_networkLoad.get(); }

    void start();
    void abort();

    // Message handlers.
    void didReceiveNetworkResourceLoaderMessage(IPC::Connection&, IPC::Decoder&);

    void continueWillSendRequest(PurCFetcher::ResourceRequest&& newRequest, bool isAllowedToAskUserForCredentials);

    const PurCFetcher::ResourceResponse& response() const { return m_response; }

    NetworkConnectionToWebProcess& connectionToWebProcess() const { return m_connection; }
    PAL::SessionID sessionID() const { return m_connection->sessionID(); }
    ResourceLoadIdentifier identifier() const { return m_parameters.identifier; }
    PurCFetcher::FrameIdentifier frameID() const { return m_parameters.webFrameID; }
    PurCFetcher::PageIdentifier pageID() const { return m_parameters.webPageID; }
    const NetworkResourceLoadParameters& parameters() const { return m_parameters; }

    NetworkCache::GlobalFrameID globalFrameID() { return { m_parameters.webPageProxyID, pageID(), frameID() }; }

    struct SynchronousLoadData;

    // NetworkLoadClient.
    void didSendData(unsigned long long bytesSent, unsigned long long totalBytesToBeSent) final;
    bool isSynchronous() const final;
    bool isAllowedToAskUserForCredentials() const final { return m_isAllowedToAskUserForCredentials; }
    void willSendRedirectedRequest(PurCFetcher::ResourceRequest&&, PurCFetcher::ResourceRequest&& redirectRequest, PurCFetcher::ResourceResponse&&) final;
    void didReceiveResponse(PurCFetcher::ResourceResponse&&, ResponseCompletionHandler&&) final;
    void didReceiveBuffer(Ref<PurCFetcher::SharedBuffer>&&, int reportedEncodedDataLength) final;
    void didFinishLoading(const PurCFetcher::NetworkLoadMetrics&) final;
    void didFailLoading(const PurCFetcher::ResourceError&) final;
    void didBlockAuthenticationChallenge() final;
    void didReceiveChallenge(const PurCFetcher::AuthenticationChallenge&) final;
    bool shouldCaptureExtraNetworkLoadMetrics() const final;

    // CrossOriginAccessControlCheckDisabler
    bool crossOriginAccessControlCheckEnabled() const override;
        
    void convertToDownload(DownloadID, const PurCFetcher::ResourceRequest&, const PurCFetcher::ResourceResponse&);

    bool isMainResource() const { return m_parameters.request.requester() == PurCFetcher::ResourceRequest::Requester::Main; }
    bool isMainFrameLoad() const { return isMainResource() && m_parameters.frameAncestorOrigins.isEmpty(); }
    bool isCrossOriginPrefetch() const;

    bool isAlwaysOnLoggingAllowed() const;

#if ENABLE(RESOURCE_LOAD_STATISTICS) && !RELEASE_LOG_DISABLED
    static bool shouldLogCookieInformation(NetworkConnectionToWebProcess&, const PAL::SessionID&);
    static void logCookieInformation(NetworkConnectionToWebProcess&, const String& label, const void* loggedObject, const PurCFetcher::NetworkStorageSession&, const URL& firstParty, const PurCFetcher::SameSiteInfo&, const URL&, const String& referrer, Optional<PurCFetcher::FrameIdentifier>, Optional<PurCFetcher::PageIdentifier>, Optional<uint64_t> identifier);
#endif

    void disableExtraNetworkLoadMetricsCapture() { m_shouldCaptureExtraNetworkLoadMetrics = false; }

    bool isKeptAlive() const { return m_isKeptAlive; }

    void consumeSandboxExtensionsIfNeeded();

#if ENABLE(SERVICE_WORKER)
    void startWithServiceWorker();
    void serviceWorkerDidNotHandle(ServiceWorkerFetchTask*);
#endif

private:
    NetworkResourceLoader(NetworkResourceLoadParameters&&, NetworkConnectionToWebProcess&, Messages::NetworkConnectionToWebProcess::PerformSynchronousLoadDelayedReply&&);

    // IPC::MessageSender
    IPC::Connection* messageSenderConnection() const override;
    uint64_t messageSenderDestinationID() const override { return m_parameters.identifier; }

    bool canUseCache(const PurCFetcher::ResourceRequest&) const;
    bool canUseCachedRedirect(const PurCFetcher::ResourceRequest&) const;

    void tryStoreAsCacheEntry();
    void retrieveCacheEntry(const PurCFetcher::ResourceRequest&);
    void retrieveCacheEntryInternal(std::unique_ptr<NetworkCache::Entry>&&, PurCFetcher::ResourceRequest&&);
    void didRetrieveCacheEntry(std::unique_ptr<NetworkCache::Entry>);
    void sendResultForCacheEntry(std::unique_ptr<NetworkCache::Entry>);
    void validateCacheEntry(std::unique_ptr<NetworkCache::Entry>);
    void dispatchWillSendRequestForCacheEntry(PurCFetcher::ResourceRequest&&, std::unique_ptr<NetworkCache::Entry>&&);

    bool shouldInterruptLoadForXFrameOptions(const String&, const URL&);
    bool shouldInterruptLoadForCSPFrameAncestorsOrXFrameOptions(const PurCFetcher::ResourceResponse&);

    enum class FirstLoad { No, Yes };
    void startNetworkLoad(PurCFetcher::ResourceRequest&&, FirstLoad);
    void restartNetworkLoad(PurCFetcher::ResourceRequest&&);
    void continueDidReceiveResponse();
    void didReceiveMainResourceResponse(const PurCFetcher::ResourceResponse&);

    enum class LoadResult {
        Unknown,
        Success,
        Failure,
        Cancel
    };
    void cleanup(LoadResult);
    
    void platformDidReceiveResponse(const PurCFetcher::ResourceResponse&);

    void startBufferingTimerIfNeeded();
    void bufferingTimerFired();
    void sendBuffer(PurCFetcher::SharedBuffer&, size_t encodedDataLength);

    void consumeSandboxExtensions();
    void invalidateSandboxExtensions();

#if ENABLE(RESOURCE_LOAD_STATISTICS) && !RELEASE_LOG_DISABLED
    void logCookieInformation() const;
#endif

    void continueWillSendRedirectedRequest(PurCFetcher::ResourceRequest&&, PurCFetcher::ResourceRequest&& redirectRequest, PurCFetcher::ResourceResponse&&, Optional<PurCFetcher::AdClickAttribution::Conversion>&&);
    void didFinishWithRedirectResponse(PurCFetcher::ResourceRequest&&, PurCFetcher::ResourceRequest&& redirectRequest, PurCFetcher::ResourceResponse&&);
    PurCFetcher::ResourceResponse sanitizeResponseIfPossible(PurCFetcher::ResourceResponse&&, PurCFetcher::ResourceResponse::SanitizationType);

    // ContentSecurityPolicyClient
    void sendCSPViolationReport(URL&&, Ref<PurCFetcher::FormData>&&) final;
//    void enqueueSecurityPolicyViolationEvent(PurCFetcher::SecurityPolicyViolationEvent::Init&&) final;

    void logSlowCacheRetrieveIfNeeded(const NetworkCache::Cache::RetrieveInfo&);

    void handleAdClickAttributionConversion(PurCFetcher::AdClickAttribution::Conversion&&, const URL&, const PurCFetcher::ResourceRequest&);

    Optional<Seconds> validateCacheEntryForMaxAgeCapValidation(const PurCFetcher::ResourceRequest&, const PurCFetcher::ResourceRequest& redirectRequest, const PurCFetcher::ResourceResponse&);

    ResourceLoadInfo resourceLoadInfo();

    const NetworkResourceLoadParameters m_parameters;

    Ref<NetworkConnectionToWebProcess> m_connection;

    std::unique_ptr<NetworkLoad> m_networkLoad;

    PurCFetcher::ResourceResponse m_response;

    size_t m_bufferedDataEncodedDataLength { 0 };
    RefPtr<PurCFetcher::SharedBuffer> m_bufferedData;
    unsigned m_redirectCount { 0 };

    std::unique_ptr<SynchronousLoadData> m_synchronousLoadData;

    bool m_wasStarted { false };
    bool m_didConsumeSandboxExtensions { false };
    bool m_isAllowedToAskUserForCredentials { false };
    size_t m_numBytesReceived { 0 };

    unsigned m_retrievedDerivedDataCount { 0 };

    PurCFetcher::Timer m_bufferingTimer;
    RefPtr<NetworkCache::Cache> m_cache;
    RefPtr<PurCFetcher::SharedBuffer> m_bufferedDataForCache;
    std::unique_ptr<NetworkCache::Entry> m_cacheEntryForValidation;
    std::unique_ptr<NetworkCache::Entry> m_cacheEntryForMaxAgeCapValidation;
    bool m_isWaitingContinueWillSendRequestForCachedRedirect { false };
    std::unique_ptr<NetworkCache::Entry> m_cacheEntryWaitingForContinueDidReceiveResponse;
    std::unique_ptr<NetworkLoadChecker> m_networkLoadChecker;
    bool m_shouldRestartLoad { false };
    ResponseCompletionHandler m_responseCompletionHandler;
    bool m_shouldCaptureExtraNetworkLoadMetrics { false };
    bool m_isKeptAlive { false };

    Optional<NetworkActivityTracker> m_networkActivityTracker;

    // gengyue
    int m_httpresponsecode;

#if ENABLE(SERVICE_WORKER)
    std::unique_ptr<ServiceWorkerFetchTask> m_serviceWorkerFetchTask;
#endif
    NetworkResourceLoadIdentifier m_resourceLoadID;
    PurCFetcher::ResourceResponse m_redirectResponse;
};

} // namespace PurCFetcher
