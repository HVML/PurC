/*
 * Copyright (C) 2012-2019 Apple Inc. All rights reserved.
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

#include "CacheStorageEngineConnection.h"
#include "Connection.h"
#include "DownloadID.h"
#include "NetworkActivityTracker.h"
#include "NetworkConnectionToWebProcessMessagesReplies.h"
#include "NetworkResourceLoadMap.h"
#include "PolicyDecision.h"
#include "SandboxExtension.h"
#include "WebPageProxyIdentifier.h"
#include "WebSocketIdentifier.h"
#include "FrameIdentifier.h"
#include "MessagePortChannelProvider.h"
#include "MessagePortIdentifier.h"
#include "NetworkLoadInformation.h"
#include "NetworkStorageSession.h"
#include "PageIdentifier.h"
#include "ProcessIdentifier.h"
#include "RegistrableDomain.h"
#include <wtf/RefCounted.h>

namespace PAL {
class SessionID;
}

namespace PurCFetcher {
class BlobDataFileReference;
class BlobPart;
class BlobRegistryImpl;
class ResourceError;
class ResourceRequest;
enum class StorageAccessScope : bool;
enum class ShouldAskITP : bool;
struct RequestStorageAccessResult;
struct SameSiteInfo;

enum class HTTPCookieAcceptPolicy : uint8_t;
enum class IncludeSecureCookies : bool;
}

namespace PurCFetcher {

class NetworkSchemeRegistry;
class NetworkProcess;
class NetworkResourceLoader;
class NetworkResourceLoadParameters;
class NetworkSession;
class ServiceWorkerFetchTask;
class WebSWServerConnection;
class WebSWServerToContextConnection;
typedef uint64_t ResourceLoadIdentifier;

namespace NetworkCache {
struct DataKey;
}

class NetworkConnectionToWebProcess
    : public RefCounted<NetworkConnectionToWebProcess>
#if HAVE(COOKIE_CHANGE_LISTENER_API)
    , public PurCFetcher::CookieChangeObserver
#endif
    , IPC::Connection::Client {
public:
    using RegistrableDomain = PurCFetcher::RegistrableDomain;

    static Ref<NetworkConnectionToWebProcess> create(NetworkProcess&, PurCFetcher::ProcessIdentifier, PAL::SessionID, IPC::Connection::Identifier);
    virtual ~NetworkConnectionToWebProcess();
    
    PAL::SessionID sessionID() const { return m_sessionID; }
    NetworkSession* networkSession();

    IPC::Connection& connection() { return m_connection.get(); }
    NetworkProcess& networkProcess() { return m_networkProcess.get(); }

    void didCleanupResourceLoader(NetworkResourceLoader&);
    void transferKeptAliveLoad(NetworkResourceLoader&);
    void setOnLineState(bool);

    bool captureExtraNetworkLoadMetricsEnabled() const { return m_captureExtraNetworkLoadMetricsEnabled; }

    RefPtr<PurCFetcher::BlobDataFileReference> getBlobDataFileReferenceForPath(const String& path);

    void cleanupForSuspension(Function<void()>&&);
    void endSuspension();

    void getNetworkLoadInformationResponse(ResourceLoadIdentifier identifier, CompletionHandler<void(const PurCFetcher::ResourceResponse&)>&& completionHandler)
    {
        completionHandler(m_networkLoadInformationByID.get(identifier).response);
    }

    void getNetworkLoadIntermediateInformation(ResourceLoadIdentifier identifier, CompletionHandler<void(const Vector<PurCFetcher::NetworkTransactionInformation>&)>&& completionHandler)
    {
        completionHandler(m_networkLoadInformationByID.get(identifier).transactions);
    }

    void takeNetworkLoadInformationMetrics(ResourceLoadIdentifier identifier, CompletionHandler<void(const PurCFetcher::NetworkLoadMetrics&)>&& completionHandler)
    {
        completionHandler(m_networkLoadInformationByID.take(identifier).metrics);
    }

    void addNetworkLoadInformation(ResourceLoadIdentifier identifier, PurCFetcher::NetworkLoadInformation&& information)
    {
        ASSERT(!m_networkLoadInformationByID.contains(identifier));
        m_networkLoadInformationByID.add(identifier, WTFMove(information));
    }

    void addNetworkLoadInformationMetrics(ResourceLoadIdentifier identifier, const PurCFetcher::NetworkLoadMetrics& metrics)
    {
        ASSERT(m_networkLoadInformationByID.contains(identifier));
        m_networkLoadInformationByID.ensure(identifier, [] {
            return PurCFetcher::NetworkLoadInformation { };
        }).iterator->value.metrics = metrics;
    }

    void removeNetworkLoadInformation(ResourceLoadIdentifier identifier)
    {
        m_networkLoadInformationByID.remove(identifier);
    }

    Optional<NetworkActivityTracker> startTrackingResourceLoad(PurCFetcher::PageIdentifier, ResourceLoadIdentifier resourceID, bool isTopResource);
    void stopTrackingResourceLoad(ResourceLoadIdentifier resourceID, NetworkActivityTracker::CompletionCode);

    void removeSocketChannel(WebSocketIdentifier);

    PurCFetcher::ProcessIdentifier webProcessIdentifier() const { return m_webProcessIdentifier; }

    void checkProcessLocalPortForActivity(const PurCFetcher::MessagePortIdentifier&, CompletionHandler<void(PurCFetcher::MessagePortChannelProvider::HasActivity)>&&);

    NetworkSchemeRegistry& schemeRegistry() { return m_schemeRegistry.get(); }

    void cookieAcceptPolicyChanged(PurCFetcher::HTTPCookieAcceptPolicy);

private:
    NetworkConnectionToWebProcess(NetworkProcess&, PurCFetcher::ProcessIdentifier, PAL::SessionID, IPC::Connection::Identifier);

    void didFinishPreconnection(uint64_t preconnectionIdentifier, const PurCFetcher::ResourceError&);
    PurCFetcher::NetworkStorageSession* storageSession();

    // IPC::Connection::Client
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) override;
    void didReceiveSyncMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&) override;
    void didClose(IPC::Connection&) override;
    void didReceiveInvalidMessage(IPC::Connection&, IPC::MessageName) override;
    const char* connectionName(void) override { return "NetworkConnectionToWebProcess"; };

    // Message handlers.
    void didReceiveNetworkConnectionToWebProcessMessage(IPC::Connection&, IPC::Decoder&);
    void didReceiveSyncNetworkConnectionToWebProcessMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&);

    void scheduleResourceLoad(NetworkResourceLoadParameters&&);
    void performSynchronousLoad(NetworkResourceLoadParameters&&, Messages::NetworkConnectionToWebProcess::PerformSynchronousLoadDelayedReply&&);
    void testProcessIncomingSyncMessagesWhenWaitingForSyncReply(WebPageProxyIdentifier, Messages::NetworkConnectionToWebProcess::TestProcessIncomingSyncMessagesWhenWaitingForSyncReplyDelayedReply&&);
    void loadPing(NetworkResourceLoadParameters&&);
    void prefetchDNS(const String&);
    void preconnectTo(Optional<uint64_t> preconnectionIdentifier, NetworkResourceLoadParameters&&);

    void removeLoadIdentifier(ResourceLoadIdentifier);
    void pageLoadCompleted(PurCFetcher::PageIdentifier);
    void browsingContextRemoved(WebPageProxyIdentifier, PurCFetcher::PageIdentifier, PurCFetcher::FrameIdentifier);
    void crossOriginRedirectReceived(ResourceLoadIdentifier, const URL& redirectURL);
    void startDownload(DownloadID, const PurCFetcher::ResourceRequest&, Optional<NavigatingToAppBoundDomain>, const String& suggestedName = { });
    void convertMainResourceLoadToDownload(uint64_t mainResourceLoadIdentifier, DownloadID, const PurCFetcher::ResourceRequest&, const PurCFetcher::ResourceResponse&, Optional<NavigatingToAppBoundDomain>);

    void registerURLSchemesAsCORSEnabled(Vector<String>&& schemes);

    void cookiesForDOM(const URL& firstParty, const PurCFetcher::SameSiteInfo&, const URL&, PurCFetcher::FrameIdentifier, PurCFetcher::PageIdentifier, PurCFetcher::IncludeSecureCookies, PurCFetcher::ShouldAskITP, PurCFetcher::ShouldRelaxThirdPartyCookieBlocking, CompletionHandler<void(String cookieString, bool secureCookiesAccessed)>&&);
    void setCookiesFromDOM(const URL& firstParty, const PurCFetcher::SameSiteInfo&, const URL&, PurCFetcher::FrameIdentifier, PurCFetcher::PageIdentifier, PurCFetcher::ShouldAskITP, const String&, PurCFetcher::ShouldRelaxThirdPartyCookieBlocking);
    void cookieRequestHeaderFieldValue(const URL& firstParty, const PurCFetcher::SameSiteInfo&, const URL&, Optional<PurCFetcher::FrameIdentifier>, Optional<PurCFetcher::PageIdentifier>, PurCFetcher::IncludeSecureCookies, PurCFetcher::ShouldAskITP, PurCFetcher::ShouldRelaxThirdPartyCookieBlocking, CompletionHandler<void(String cookieString, bool secureCookiesAccessed)>&&);
    void getRawCookies(const URL& firstParty, const PurCFetcher::SameSiteInfo&, const URL&, Optional<PurCFetcher::FrameIdentifier>, Optional<PurCFetcher::PageIdentifier>, PurCFetcher::ShouldAskITP, PurCFetcher::ShouldRelaxThirdPartyCookieBlocking, CompletionHandler<void(Vector<PurCFetcher::Cookie>&&)>&&);
    void setRawCookie(const PurCFetcher::Cookie&);
    void deleteCookie(const URL&, const String& cookieName);

    void setCaptureExtraNetworkLoadMetricsEnabled(bool);

    void createSocketStream(URL&&, String cachePartition, WebSocketIdentifier);

    void createSocketChannel(const PurCFetcher::ResourceRequest&, const String& protocol, WebSocketIdentifier);
    void updateQuotaBasedOnSpaceUsageForTesting(const PurCFetcher::ClientOrigin&);

    void createNewMessagePortChannel(const PurCFetcher::MessagePortIdentifier& port1, const PurCFetcher::MessagePortIdentifier& port2);
    void entangleLocalPortInThisProcessToRemote(const PurCFetcher::MessagePortIdentifier& local, const PurCFetcher::MessagePortIdentifier& remote);
    void messagePortDisentangled(const PurCFetcher::MessagePortIdentifier&);
    void messagePortClosed(const PurCFetcher::MessagePortIdentifier&);
    void takeAllMessagesForPort(const PurCFetcher::MessagePortIdentifier&, CompletionHandler<void(Vector<PurCFetcher::MessageWithMessagePorts>&&, uint64_t)>&&);
    void postMessageToRemote(PurCFetcher::MessageWithMessagePorts&&, const PurCFetcher::MessagePortIdentifier&);
    void checkRemotePortForActivity(const PurCFetcher::MessagePortIdentifier, CompletionHandler<void(bool)>&&);
    void didDeliverMessagePortMessages(uint64_t messageBatchIdentifier);

#if USE(LIBWEBRTC)
    NetworkRTCProvider& rtcProvider();
#endif
#if ENABLE(WEB_RTC)
    NetworkMDNSRegister& mdnsRegister() { return m_mdnsRegister; }
#endif

    CacheStorageEngineConnection& cacheStorageConnection();

#if ENABLE(RESOURCE_LOAD_STATISTICS)
    void removeStorageAccessForFrame(PurCFetcher::FrameIdentifier, PurCFetcher::PageIdentifier);
    void clearPageSpecificDataForResourceLoadStatistics(PurCFetcher::PageIdentifier);

    void logUserInteraction(const RegistrableDomain&);
    void resourceLoadStatisticsUpdated(Vector<PurCFetcher::ResourceLoadStatistics>&&);
    void hasStorageAccess(const RegistrableDomain& subFrameDomain, const RegistrableDomain& topFrameDomain, PurCFetcher::FrameIdentifier, PurCFetcher::PageIdentifier, CompletionHandler<void(bool)>&&);
    void requestStorageAccess(const RegistrableDomain& subFrameDomain, const RegistrableDomain& topFrameDomain, PurCFetcher::FrameIdentifier, PurCFetcher::PageIdentifier, WebPageProxyIdentifier, PurCFetcher::StorageAccessScope, CompletionHandler<void(PurCFetcher::RequestStorageAccessResult)>&&);
    void requestStorageAccessUnderOpener(PurCFetcher::RegistrableDomain&& domainInNeedOfStorageAccess, PurCFetcher::PageIdentifier openerPageID, PurCFetcher::RegistrableDomain&& openerDomain);
#endif

    void addOriginAccessWhitelistEntry(const String& sourceOrigin, const String& destinationProtocol, const String& destinationHost, bool allowDestinationSubdomains);
    void removeOriginAccessWhitelistEntry(const String& sourceOrigin, const String& destinationProtocol, const String& destinationHost, bool allowDestinationSubdomains);
    void resetOriginAccessWhitelists();

    uint64_t nextMessageBatchIdentifier(Function<void()>&&);

    void domCookiesForHost(const String& host, bool subscribeToCookieChangeNotifications, CompletionHandler<void(const Vector<PurCFetcher::Cookie>&)>&&);

#if HAVE(COOKIE_CHANGE_LISTENER_API)
    void unsubscribeFromCookieChangeNotifications(const HashSet<String>& hosts);

    // PurCFetcher::CookieChangeObserver.
    void cookiesAdded(const String& host, const Vector<PurCFetcher::Cookie>&) final;
    void cookiesDeleted(const String& host, const Vector<PurCFetcher::Cookie>&) final;
    void allCookiesDeleted() final;
#endif

    struct ResourceNetworkActivityTracker {
        ResourceNetworkActivityTracker() = default;
        ResourceNetworkActivityTracker(const ResourceNetworkActivityTracker&) = default;
        ResourceNetworkActivityTracker(ResourceNetworkActivityTracker&&) = default;
        ResourceNetworkActivityTracker(PurCFetcher::PageIdentifier pageID)
            : pageID { pageID }
            , isRootActivity { true }
            , networkActivity { NetworkActivityTracker::Label::LoadPage }
        {
        }

        ResourceNetworkActivityTracker(PurCFetcher::PageIdentifier pageID, ResourceLoadIdentifier resourceID)
            : pageID { pageID }
            , resourceID { resourceID }
            , networkActivity { NetworkActivityTracker::Label::LoadResource }
        {
        }

        PurCFetcher::PageIdentifier pageID;
        ResourceLoadIdentifier resourceID { 0 };
        bool isRootActivity { false };
        NetworkActivityTracker networkActivity;
    };

    void stopAllNetworkActivityTracking();
    void stopAllNetworkActivityTrackingForPage(PurCFetcher::PageIdentifier);
    size_t findRootNetworkActivity(PurCFetcher::PageIdentifier);
    size_t findNetworkActivityTracker(ResourceLoadIdentifier resourceID);

    void hasUploadStateChanged(bool);

    Ref<IPC::Connection> m_connection;
    Ref<NetworkProcess> m_networkProcess;
    PAL::SessionID m_sessionID;

    NetworkResourceLoadMap m_networkResourceLoaders;
    Vector<ResourceNetworkActivityTracker> m_networkActivityTrackers;

    HashMap<ResourceLoadIdentifier, PurCFetcher::NetworkLoadInformation> m_networkLoadInformationByID;


#if USE(LIBWEBRTC)
    RefPtr<NetworkRTCProvider> m_rtcProvider;
#endif
#if ENABLE(WEB_RTC)
    NetworkMDNSRegister m_mdnsRegister;
#endif
#if HAVE(COOKIE_CHANGE_LISTENER_API)
    HashSet<String> m_hostsWithCookieListeners;
#endif

    bool m_captureExtraNetworkLoadMetricsEnabled { false };

    RefPtr<CacheStorageEngineConnection> m_cacheStorageConnection;

    const PurCFetcher::ProcessIdentifier m_webProcessIdentifier;

    HashSet<PurCFetcher::MessagePortIdentifier> m_processEntangledPorts;
    HashMap<uint64_t, Function<void()>> m_messageBatchDeliveryCompletionHandlers;
    Ref<NetworkSchemeRegistry> m_schemeRegistry;
};

} // namespace PurCFetcher
