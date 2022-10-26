/*
 * Copyright (C) 2012-2020 Apple Inc. All rights reserved.
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

#include "AuxiliaryProcess.h"
#include "CacheModel.h"
#include "CallbackID.h"
#include "DownloadManager.h"
#include "LocalStorageDatabaseTracker.h"
#include "NetworkContentRuleListManager.h"
#include "NetworkHTTPSUpgradeChecker.h"
#include "SandboxExtension.h"
#if ENABLE(INDEXED_DATABASE)
#include "WebIDBServer.h"
#endif
#include "WebPageProxyIdentifier.h"
#include "WebsiteData.h"
#include "ClientOrigin.h"
#include "CrossSiteNavigationDataTransfer.h"
#include "DiagnosticLoggingClient.h"
#include "FetchIdentifier.h"
//#include "IDBKeyData.h"
#include "MessagePortChannelRegistry.h"
#include "StorageQuotaManager.h"
#include "PageIdentifier.h"
#include "RegistrableDomain.h"
#include <memory>
#include <wtf/CrossThreadTask.h>
#include <wtf/Function.h>
#include <wtf/HashSet.h>
#include <wtf/MemoryPressureHandler.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/RetainPtr.h>
#include <wtf/WeakPtr.h>

namespace IPC {
class FormDataReference;
}

namespace PAL {
class SessionID;
}

namespace PurCFetcher {
class CertificateInfo;
class CurlProxySettings;
class DownloadID;
class ProtectionSpace;
class StorageQuotaManager;
class NetworkStorageSession;
class ResourceError;
class SWServer;
enum class HTTPCookieAcceptPolicy : uint8_t;
enum class IncludeHttpOnlyCookies : bool;
enum class StoredCredentialsPolicy : uint8_t;
enum class StorageAccessPromptWasShown : bool;
enum class StorageAccessWasGranted : bool;
struct ClientOrigin;
struct MessageWithMessagePorts;
struct SecurityOriginData;
struct SoupNetworkProxySettings;
struct ServiceWorkerClientIdentifier;
}

namespace PurCFetcher {

class AuthenticationManager;
class NetworkConnectionToWebProcess;
class NetworkProcessSupplement;
class NetworkProximityManager;
class NetworkResourceLoader;
class StorageManagerSet;
class WebSWServerConnection;
class WebSWServerToContextConnection;
enum class ShouldGrandfatherStatistics : bool;
enum class StorageAccessStatus : uint8_t;
enum class WebsiteDataFetchOption : uint8_t;
enum class WebsiteDataType : uint32_t;
struct NetworkProcessCreationParameters;
struct WebsiteDataStoreParameters;

namespace NetworkCache {
enum class CacheOption : uint8_t;
}

namespace CacheStorage {
class Engine;
}

class NetworkProcess : public AuxiliaryProcess, private DownloadManager::Client, public ThreadSafeRefCounted<NetworkProcess>, public CanMakeWeakPtr<NetworkProcess>
{
    WTF_MAKE_NONCOPYABLE(NetworkProcess);
public:
    using RegistrableDomain = PurCFetcher::RegistrableDomain;
    using TopFrameDomain = PurCFetcher::RegistrableDomain;
    using SubFrameDomain = PurCFetcher::RegistrableDomain;
    using SubResourceDomain = PurCFetcher::RegistrableDomain;
    using RedirectDomain = PurCFetcher::RegistrableDomain;
    using RedirectedFromDomain = PurCFetcher::RegistrableDomain;
    using RedirectedToDomain = PurCFetcher::RegistrableDomain;
    using NavigatedFromDomain = PurCFetcher::RegistrableDomain;
    using NavigatedToDomain = PurCFetcher::RegistrableDomain;
    using DomainInNeedOfStorageAccess = PurCFetcher::RegistrableDomain;
    using OpenerDomain = PurCFetcher::RegistrableDomain;

    NetworkProcess(AuxiliaryProcessInitializationParameters&&);
    ~NetworkProcess();
    static constexpr ProcessType processType = ProcessType::Network;

    template <typename T>
    T* supplement()
    {
        return static_cast<T*>(m_supplements.get(T::supplementName()));
    }

    template <typename T>
    void addSupplement()
    {
        m_supplements.add(T::supplementName(), makeUnique<T>(*this));
    }

    void removeNetworkConnectionToWebProcess(NetworkConnectionToWebProcess&);

    AuthenticationManager& authenticationManager();
    DownloadManager& downloadManager();

    void setSession(PAL::SessionID, std::unique_ptr<NetworkSession>&&);
    NetworkSession* networkSession(PAL::SessionID) const final;
    void destroySession(PAL::SessionID);

    void forEachNetworkSession(const Function<void(NetworkSession&)>&);

    void forEachNetworkStorageSession(const Function<void(PurCFetcher::NetworkStorageSession&)>&);
    PurCFetcher::NetworkStorageSession* storageSession(const PAL::SessionID&) const;
    PurCFetcher::NetworkStorageSession& defaultStorageSession() const;
    std::unique_ptr<PurCFetcher::NetworkStorageSession> newTestingSession(const PAL::SessionID&);
    void ensureSession(const PAL::SessionID&, bool shouldUseTestingNetworkSession, const String& identifier);

    void processWillSuspendImminentlyForTestingSync(CompletionHandler<void()>&&);
    void prepareToSuspend(bool isSuspensionImminent, CompletionHandler<void()>&&);
    void processDidResume();
    void resume();

    CacheModel cacheModel() const { return m_cacheModel; }

    // Diagnostic messages logging.
    void logDiagnosticMessage(WebPageProxyIdentifier, const String& message, const String& description, PurCFetcher::ShouldSample);
    void logDiagnosticMessageWithResult(WebPageProxyIdentifier, const String& message, const String& description, PurCFetcher::DiagnosticLoggingResultType, PurCFetcher::ShouldSample);
    void logDiagnosticMessageWithValue(WebPageProxyIdentifier, const String& message, const String& description, double value, unsigned significantFigures, PurCFetcher::ShouldSample);

#if USE(SOUP)
    void getHostNamesWithHSTSCache(PurCFetcher::NetworkStorageSession&, HashSet<String>&);
    void deleteHSTSCacheForHostNames(PurCFetcher::NetworkStorageSession&, const Vector<String>&);
    void clearHSTSCache(PurCFetcher::NetworkStorageSession&, WallTime modifiedSince);
#endif

    void findPendingDownloadLocation(NetworkDataTask&, ResponseCompletionHandler&&, const PurCFetcher::ResourceResponse&);

    void prefetchDNS(const String&);

    void addWebsiteDataStore(WebsiteDataStoreParameters&&);

#if ENABLE(RESOURCE_LOAD_STATISTICS)
    void clearPrevalentResource(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void()>&&);
    void clearUserInteraction(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void()>&&);
    void deleteAndRestrictWebsiteDataForRegistrableDomains(PAL::SessionID, OptionSet<WebsiteDataType>, RegistrableDomainsToDeleteOrRestrictWebsiteDataFor&&, bool shouldNotifyPage, CompletionHandler<void(const HashSet<RegistrableDomain>&)>&&);
    void deleteCookiesForTesting(PAL::SessionID, RegistrableDomain, bool includeHttpOnlyCookies, CompletionHandler<void()>&&);
    void dumpResourceLoadStatistics(PAL::SessionID, CompletionHandler<void(String)>&&);
    void updatePrevalentDomainsToBlockCookiesFor(PAL::SessionID, const Vector<RegistrableDomain>& domainsToBlock, CompletionHandler<void()>&&);
    void isGrandfathered(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void(bool)>&&);
    void isPrevalentResource(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void(bool)>&&);
    void isVeryPrevalentResource(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void(bool)>&&);
    void setAgeCapForClientSideCookies(PAL::SessionID, Optional<Seconds>, CompletionHandler<void()>&&);
    void isRegisteredAsRedirectingTo(PAL::SessionID, const RedirectedFromDomain&, const RedirectedToDomain&, CompletionHandler<void(bool)>&&);
    void isRegisteredAsSubFrameUnder(PAL::SessionID, const SubFrameDomain&, const TopFrameDomain&, CompletionHandler<void(bool)>&&);
    void isRegisteredAsSubresourceUnder(PAL::SessionID, const SubResourceDomain&, const TopFrameDomain&, CompletionHandler<void(bool)>&&);
    void setGrandfathered(PAL::SessionID, const RegistrableDomain&, bool isGrandfathered, CompletionHandler<void()>&&);
    void setUseITPDatabase(PAL::SessionID, bool value, CompletionHandler<void()>&&);
    void setMaxStatisticsEntries(PAL::SessionID, uint64_t maximumEntryCount, CompletionHandler<void()>&&);
    void setPrevalentResource(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void()>&&);
    void setPrevalentResourceForDebugMode(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void()>&&);
    void setVeryPrevalentResource(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void()>&&);
    void setPruneEntriesDownTo(PAL::SessionID, uint64_t pruneTargetCount, CompletionHandler<void()>&&);
    void hadUserInteraction(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void(bool)>&&);
    void isRelationshipOnlyInDatabaseOnce(PAL::SessionID, const RegistrableDomain& subDomain, const RegistrableDomain& topDomain, CompletionHandler<void(bool)>&&);
    void hasLocalStorage(PAL::SessionID, const RegistrableDomain&, CompletionHandler<void(bool)>&&);
    void getAllStorageAccessEntries(PAL::SessionID, CompletionHandler<void(Vector<String> domains)>&&);
    void logFrameNavigation(PAL::SessionID, const NavigatedToDomain&, const TopFrameDomain&, const NavigatedFromDomain&, bool isRedirect, bool isMainFrame, Seconds delayAfterMainFrameDocumentLoad, bool wasPotentiallyInitiatedByUser);
    void logUserInteraction(PAL::SessionID, const TopFrameDomain&, CompletionHandler<void()>&&);
    void resetCacheMaxAgeCapForPrevalentResources(PAL::SessionID, CompletionHandler<void()>&&);
    void resetParametersToDefaultValues(PAL::SessionID, CompletionHandler<void()>&&);
    void scheduleClearInMemoryAndPersistent(PAL::SessionID, Optional<WallTime> modifiedSince, ShouldGrandfatherStatistics, CompletionHandler<void()>&&);
    void scheduleCookieBlockingUpdate(PAL::SessionID, CompletionHandler<void()>&&);
    void scheduleStatisticsAndDataRecordsProcessing(PAL::SessionID, CompletionHandler<void()>&&);
    void statisticsDatabaseHasAllTables(PAL::SessionID, CompletionHandler<void(bool)>&&);
    void submitTelemetry(PAL::SessionID, CompletionHandler<void()>&&);
    void setCacheMaxAgeCapForPrevalentResources(PAL::SessionID, Seconds, CompletionHandler<void()>&&);
    void setGrandfatheringTime(PAL::SessionID, Seconds, CompletionHandler<void()>&&);
    void setLastSeen(PAL::SessionID, const RegistrableDomain&, Seconds, CompletionHandler<void()>&&);
    void domainIDExistsInDatabase(PAL::SessionID, int domainID, CompletionHandler<void(bool)>&&);
    void mergeStatisticForTesting(PAL::SessionID, const RegistrableDomain&, const TopFrameDomain& topFrameDomain1, const TopFrameDomain& topFrameDomain2, Seconds lastSeen, bool hadUserInteraction, Seconds mostRecentUserInteraction, bool isGrandfathered, bool isPrevalent, bool isVeryPrevalent, unsigned dataRecordsRemoved, CompletionHandler<void()>&&);
    void insertExpiredStatisticForTesting(PAL::SessionID, const RegistrableDomain&, bool hadUserInteraction, bool isScheduledForAllButCookieDataRemoval, bool isPrevalent, CompletionHandler<void()>&&);
    void setMinimumTimeBetweenDataRecordsRemoval(PAL::SessionID, Seconds, CompletionHandler<void()>&&);
    void setNotifyPagesWhenDataRecordsWereScanned(PAL::SessionID, bool value, CompletionHandler<void()>&&);
    void setIsRunningResourceLoadStatisticsTest(PAL::SessionID, bool value, CompletionHandler<void()>&&);
    void setNotifyPagesWhenTelemetryWasCaptured(PAL::SessionID, bool value, CompletionHandler<void()>&&);
    void setResourceLoadStatisticsEnabled(PAL::SessionID, bool);
    void setResourceLoadStatisticsLogTestingEvent(bool);
    void setResourceLoadStatisticsDebugMode(PAL::SessionID, bool debugMode, CompletionHandler<void()>&&d);
    void isResourceLoadStatisticsEphemeral(PAL::SessionID, CompletionHandler<void(bool)>&&) const;
    void setShouldClassifyResourcesBeforeDataRecordsRemoval(PAL::SessionID, bool value, CompletionHandler<void()>&&);
    void setSubframeUnderTopFrameDomain(PAL::SessionID, const SubFrameDomain&, const TopFrameDomain&, CompletionHandler<void()>&&);
    void setSubresourceUnderTopFrameDomain(PAL::SessionID, const SubResourceDomain&, const TopFrameDomain&, CompletionHandler<void()>&&);
    void setSubresourceUniqueRedirectTo(PAL::SessionID, const SubResourceDomain&, const RedirectedToDomain&, CompletionHandler<void()>&&);
    void setSubresourceUniqueRedirectFrom(PAL::SessionID, const SubResourceDomain&, const RedirectedFromDomain&, CompletionHandler<void()>&&);
    void setTimeToLiveUserInteraction(PAL::SessionID, Seconds, CompletionHandler<void()>&&);
    void setTopFrameUniqueRedirectTo(PAL::SessionID, const TopFrameDomain&, const RedirectedToDomain&, CompletionHandler<void()>&&);
    void setTopFrameUniqueRedirectFrom(PAL::SessionID, const TopFrameDomain&, const RedirectedFromDomain&, CompletionHandler<void()>&&);
    void registrableDomainsWithWebsiteData(PAL::SessionID, OptionSet<WebsiteDataType>, bool shouldNotifyPage, CompletionHandler<void(HashSet<RegistrableDomain>&&)>&&);
    void didCommitCrossSiteLoadWithDataTransfer(PAL::SessionID, const RegistrableDomain& fromDomain, const RegistrableDomain& toDomain, OptionSet<PurCFetcher::CrossSiteNavigationDataTransfer::Flag>, WebPageProxyIdentifier, PurCFetcher::PageIdentifier);
    void setCrossSiteLoadWithLinkDecorationForTesting(PAL::SessionID, const RegistrableDomain& fromDomain, const RegistrableDomain& toDomain, CompletionHandler<void()>&&);
    void resetCrossSiteLoadsWithLinkDecorationForTesting(PAL::SessionID, CompletionHandler<void()>&&);
    void hasIsolatedSession(PAL::SessionID, const PurCFetcher::RegistrableDomain&, CompletionHandler<void(bool)>&&) const;
    void setAppBoundDomainsForResourceLoadStatistics(PAL::SessionID, HashSet<PurCFetcher::RegistrableDomain>&&, CompletionHandler<void()>&&);
    bool isITPDatabaseEnabled() const { return m_isITPDatabaseEnabled; }
    void setShouldDowngradeReferrerForTesting(bool, CompletionHandler<void()>&&);
    void setThirdPartyCookieBlockingMode(PAL::SessionID, PurCFetcher::ThirdPartyCookieBlockingMode, CompletionHandler<void()>&&);
    void setShouldEnbleSameSiteStrictEnforcementForTesting(PAL::SessionID, PurCFetcher::SameSiteStrictEnforcementEnabled, CompletionHandler<void()>&&);
    void setFirstPartyWebsiteDataRemovalModeForTesting(PAL::SessionID, PurCFetcher::FirstPartyWebsiteDataRemovalMode, CompletionHandler<void()>&&);
    void setToSameSiteStrictCookiesForTesting(PAL::SessionID, const PurCFetcher::RegistrableDomain&, CompletionHandler<void()>&&);
#endif

    using CacheStorageRootPathCallback = CompletionHandler<void(String&&)>;
    void cacheStorageRootPath(PAL::SessionID, CacheStorageRootPathCallback&&);

    void preconnectTo(PAL::SessionID, WebPageProxyIdentifier, PurCFetcher::PageIdentifier, const URL&, const String&, PurCFetcher::StoredCredentialsPolicy, Optional<NavigatingToAppBoundDomain>);

    void setSessionIsControlledByAutomation(PAL::SessionID, bool);
    bool sessionIsControlledByAutomation(PAL::SessionID) const;

    void connectionToWebProcessClosed(IPC::Connection&, PAL::SessionID);
    void getLocalStorageOriginDetails(PAL::SessionID, CompletionHandler<void(Vector<LocalStorageDatabaseTracker::OriginDetails>&&)>&&);

#if ENABLE(CONTENT_EXTENSIONS)
    NetworkContentRuleListManager& networkContentRuleListManager() { return m_networkContentRuleListManager; }
#endif

#if ENABLE(INDEXED_DATABASE)
    WebIDBServer& webIDBServer(PAL::SessionID);
#endif

    void syncLocalStorage(CompletionHandler<void()>&&);
    void clearLegacyPrivateBrowsingLocalStorage();

    void resetQuota(PAL::SessionID, CompletionHandler<void()>&&);
    void renameOriginInWebsiteData(PAL::SessionID, const URL&, const URL&, OptionSet<WebsiteDataType>, CompletionHandler<void()>&&);

    bool parentProcessHasServiceWorkerEntitlement() const { return true; }

    const String& uiProcessBundleIdentifier() const { return m_uiProcessBundleIdentifier; }

    void ref() const override { ThreadSafeRefCounted<NetworkProcess>::ref(); }
    void deref() const override { ThreadSafeRefCounted<NetworkProcess>::deref(); }

    CacheStorage::Engine* findCacheEngine(const PAL::SessionID&);
    CacheStorage::Engine& ensureCacheEngine(const PAL::SessionID&, Function<Ref<CacheStorage::Engine>()>&&);
    void removeCacheEngine(const PAL::SessionID&);
    void requestStorageSpace(PAL::SessionID, const PurCFetcher::ClientOrigin&, uint64_t quota, uint64_t currentSize, uint64_t spaceRequired, CompletionHandler<void(Optional<uint64_t>)>&&);

    void dumpAdClickAttribution(PAL::SessionID, CompletionHandler<void(String)>&&);
    void clearAdClickAttribution(PAL::SessionID, CompletionHandler<void()>&&);
    void setAdClickAttributionOverrideTimerForTesting(PAL::SessionID, bool value, CompletionHandler<void()>&&);
    void setAdClickAttributionConversionURLForTesting(PAL::SessionID, URL&&, CompletionHandler<void()>&&);
    void markAdClickAttributionsAsExpiredForTesting(PAL::SessionID, CompletionHandler<void()>&&);

    RefPtr<PurCFetcher::StorageQuotaManager> storageQuotaManager(PAL::SessionID, const PurCFetcher::ClientOrigin&);

    void addKeptAliveLoad(Ref<NetworkResourceLoader>&&);
    void removeKeptAliveLoad(NetworkResourceLoader&);

    const OptionSet<NetworkCache::CacheOption>& cacheOptions() const { return m_cacheOptions; }

    NetworkConnectionToWebProcess* webProcessConnection(PurCFetcher::ProcessIdentifier) const;
    PurCFetcher::MessagePortChannelRegistry& messagePortChannelRegistry() { return m_messagePortChannelRegistry; }

    void setServiceWorkerFetchTimeoutForTesting(Seconds, CompletionHandler<void()>&&);
    void resetServiceWorkerFetchTimeoutForTesting(CompletionHandler<void()>&&);
    Seconds serviceWorkerFetchTimeout() const { return m_serviceWorkerFetchTimeout; }

    void cookieAcceptPolicyChanged(PurCFetcher::HTTPCookieAcceptPolicy);
    void hasAppBoundSession(PAL::SessionID, CompletionHandler<void(bool)>&&) const;
    void clearAppBoundSession(PAL::SessionID, CompletionHandler<void()>&&);

//    void broadcastConsoleMessage(PAL::SessionID, JSC::MessageSource, JSC::MessageLevel, const String& message);
    void updateBundleIdentifier(String&&, CompletionHandler<void()>&&);
    void clearBundleIdentifier(CompletionHandler<void()>&&);

private:
    void platformInitializeNetworkProcess(const NetworkProcessCreationParameters&);
    std::unique_ptr<PurCFetcher::NetworkStorageSession> platformCreateDefaultStorageSession() const;

    void didReceiveNetworkProcessMessage(IPC::Connection&, IPC::Decoder&);

    void terminate() override;
    void platformTerminate();

    void lowMemoryHandler(Critical);

    void processDidTransitionToForeground();
    void processDidTransitionToBackground();
    void platformProcessDidTransitionToForeground();
    void platformProcessDidTransitionToBackground();

    // AuxiliaryProcess
    void initializeProcess(const AuxiliaryProcessInitializationParameters&) override;
    void initializeProcessName(const AuxiliaryProcessInitializationParameters&) override;
    void initializeSandbox(const AuxiliaryProcessInitializationParameters&, SandboxInitializationParameters&) override;
    void initializeConnection(IPC::Connection*) override;
    bool shouldTerminate() override;

    // IPC::Connection::Client
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) override;
    void didReceiveSyncMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&) override;
    void didClose(IPC::Connection&) override;
    const char* connectionName(void) override { return "NetworkProcess"; };

    // DownloadManager::Client
    void didCreateDownload() override;
    void didDestroyDownload() override;
    IPC::Connection* downloadProxyConnection() override;
    IPC::Connection* parentProcessConnectionForDownloads() override { return parentProcessConnection(); }
    AuthenticationManager& downloadsAuthenticationManager() override;
    void pendingDownloadCanceled(DownloadID) override;

    // Message Handlers
    void didReceiveSyncNetworkProcessMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&);
    void initializeNetworkProcess(NetworkProcessCreationParameters&&);
    void createNetworkConnectionToWebProcess(PurCFetcher::ProcessIdentifier, PAL::SessionID, CompletionHandler<void(Optional<IPC::Attachment>&&, PurCFetcher::HTTPCookieAcceptPolicy)>&&);

    void fetchWebsiteData(PAL::SessionID, OptionSet<WebsiteDataType>, OptionSet<WebsiteDataFetchOption>, CallbackID);
    void deleteWebsiteData(PAL::SessionID, OptionSet<WebsiteDataType>, WallTime modifiedSince, CallbackID);
    void deleteWebsiteDataForOrigins(PAL::SessionID, OptionSet<WebsiteDataType>, const Vector<PurCFetcher::SecurityOriginData>& origins, const Vector<String>& cookieHostNames, const Vector<String>& HSTSCacheHostnames, const Vector<RegistrableDomain>&, CallbackID);

    void clearCachedCredentials();

    void setCacheStorageParameters(PAL::SessionID, String&& cacheStorageDirectory, SandboxExtension::Handle&&);
    void initializeQuotaUsers(PurCFetcher::StorageQuotaManager&, PAL::SessionID, const PurCFetcher::ClientOrigin&);

    // FIXME: This should take a session ID so we can identify which disk cache to delete.
    void clearDiskCache(WallTime modifiedSince, CompletionHandler<void()>&&);

    void downloadRequest(PAL::SessionID, DownloadID, const PurCFetcher::ResourceRequest&, Optional<NavigatingToAppBoundDomain>, const String& suggestedFilename);
    void resumeDownload(PAL::SessionID, DownloadID, const IPC::DataReference& resumeData, const String& path, SandboxExtension::Handle&&);
    void cancelDownload(DownloadID);
    void continueWillSendRequest(DownloadID, PurCFetcher::ResourceRequest&&);
    void continueDecidePendingDownloadDestination(DownloadID, String destination, SandboxExtension::Handle&&, bool allowOverwrite);
    void applicationDidEnterBackground();
    void applicationWillEnterForeground();

    void setCacheModel(CacheModel);
    void setCacheModelSynchronouslyForTesting(CacheModel, CompletionHandler<void()>&&);
    void allowSpecificHTTPSCertificateForHost(const PurCFetcher::CertificateInfo&, const String& host);
    void setAllowsAnySSLCertificateForWebSocket(bool, CompletionHandler<void()>&&);

    void syncAllCookies();
    void didSyncAllCookies();

#if USE(SOUP)
    void setIgnoreTLSErrors(bool);
    void userPreferredLanguagesChanged(const Vector<String>&);
    void setNetworkProxySettings(const PurCFetcher::SoupNetworkProxySettings&);
#endif

#if USE(CURL)
    void setNetworkProxySettings(PAL::SessionID, PurCFetcher::CurlProxySettings&&);
#endif

#if PLATFORM(MAC) || PLATFORM(MACCATALYST)
    static void setSharedHTTPCookieStorage(const Vector<uint8_t>& identifier);
#endif

    void platformSyncAllCookies(CompletionHandler<void()>&&);

    void registerURLSchemeAsSecure(const String&) const;
    void registerURLSchemeAsBypassingContentSecurityPolicy(const String&) const;
    void registerURLSchemeAsLocal(const String&) const;
    void registerURLSchemeAsNoAccess(const String&) const;
    void registerURLSchemeAsCORSEnabled(const String&) const;

#if ENABLE(INDEXED_DATABASE)
    void addIndexedDatabaseSession(PAL::SessionID, String&, SandboxExtension::Handle&);
    void collectIndexedDatabaseOriginsForVersion(const String&, HashSet<PurCFetcher::SecurityOriginData>&);
    HashSet<PurCFetcher::SecurityOriginData> indexedDatabaseOrigins(const String& path);
    Ref<WebIDBServer> createWebIDBServer(PAL::SessionID);
    void setSessionStorageQuotaManagerIDBRootPath(PAL::SessionID, const String& idbRootPath);
#endif

    void postStorageTask(CrossThreadTask&&);
    // For execution on work queue thread only.
    void performNextStorageTask();
    void ensurePathExists(const String& path);

    class SessionStorageQuotaManager {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        SessionStorageQuotaManager(const String& cacheRootPath, uint64_t defaultQuota, uint64_t defaultThirdPartyQuota)
            : m_cacheRootPath(cacheRootPath)
            , m_defaultQuota(defaultQuota)
            , m_defaultThirdPartyQuota(defaultThirdPartyQuota)
        {
        }
        uint64_t defaultQuota(const PurCFetcher::ClientOrigin& origin) const { return origin.topOrigin == origin.clientOrigin ? m_defaultQuota : m_defaultThirdPartyQuota; }

        Ref<PurCFetcher::StorageQuotaManager> ensureOriginStorageQuotaManager(PurCFetcher::ClientOrigin origin, uint64_t quota, PurCFetcher::StorageQuotaManager::UsageGetter&& usageGetter, PurCFetcher::StorageQuotaManager::QuotaIncreaseRequester&& quotaIncreaseRequester)
        {
            auto iter = m_storageQuotaManagers.ensure(origin, [quota, usageGetter = WTFMove(usageGetter), quotaIncreaseRequester = WTFMove(quotaIncreaseRequester)]() mutable {
                return PurCFetcher::StorageQuotaManager::create(quota, WTFMove(usageGetter), WTFMove(quotaIncreaseRequester));
            }).iterator;
            return makeRef(*iter->value);
        }

        auto existingStorageQuotaManagers() { return m_storageQuotaManagers.values(); }

        const String& cacheRootPath() const { return m_cacheRootPath; }
#if ENABLE(INDEXED_DATABASE)
        void setIDBRootPath(const String& idbRootPath) { m_idbRootPath = idbRootPath; }
        const String& idbRootPath() const { return m_idbRootPath; }
#endif

    private:
        String m_cacheRootPath;
#if ENABLE(INDEXED_DATABASE)
        String m_idbRootPath;
#endif
        uint64_t m_defaultQuota { PurCFetcher::StorageQuotaManager::defaultQuota() };
        uint64_t m_defaultThirdPartyQuota { PurCFetcher::StorageQuotaManager::defaultThirdPartyQuota() };
        HashMap<PurCFetcher::ClientOrigin, RefPtr<PurCFetcher::StorageQuotaManager>> m_storageQuotaManagers;
    };
    void addSessionStorageQuotaManager(PAL::SessionID, uint64_t defaultQuota, uint64_t defaultThirdPartyQuota, const String& cacheRootPath, SandboxExtension::Handle&);
    void removeSessionStorageQuotaManager(PAL::SessionID);

    // Connections to WebProcesses.
    HashMap<PurCFetcher::ProcessIdentifier, Ref<NetworkConnectionToWebProcess>> m_webProcessConnections;

    bool m_hasSetCacheModel { false };
    CacheModel m_cacheModel { CacheModel::DocumentViewer };
    bool m_suppressMemoryPressureHandler { false };
    String m_uiProcessBundleIdentifier;
    DownloadManager m_downloadManager;

    HashMap<PAL::SessionID, Ref<CacheStorage::Engine>> m_cacheEngines;

    typedef HashMap<const char*, std::unique_ptr<NetworkProcessSupplement>, PtrHash<const char*>> NetworkProcessSupplementMap;
    NetworkProcessSupplementMap m_supplements;

    HashSet<PAL::SessionID> m_sessionsControlledByAutomation;
    HashMap<PAL::SessionID, Vector<CacheStorageRootPathCallback>> m_cacheStorageParametersCallbacks;

    HashMap<PAL::SessionID, std::unique_ptr<NetworkSession>> m_networkSessions;
    HashMap<PAL::SessionID, std::unique_ptr<PurCFetcher::NetworkStorageSession>> m_networkStorageSessions;
    mutable std::unique_ptr<PurCFetcher::NetworkStorageSession> m_defaultNetworkStorageSession;

    RefPtr<StorageManagerSet> m_storageManagerSet;

#if ENABLE(CONTENT_EXTENSIONS)
    NetworkContentRuleListManager m_networkContentRuleListManager;
#endif

    Ref<WorkQueue> m_storageTaskQueue { WorkQueue::create("com.apple.PurCFetcher.StorageTask") };

#if ENABLE(INDEXED_DATABASE)
    HashMap<PAL::SessionID, String> m_idbDatabasePaths;
    HashMap<PAL::SessionID, RefPtr<WebIDBServer>> m_webIDBServers;
#endif

    Deque<CrossThreadTask> m_storageTasks;
    Lock m_storageTaskMutex;

#if ENABLE(RESOURCE_LOAD_STATISTICS)
    bool m_isITPDatabaseEnabled { false };
#endif

    Lock m_sessionStorageQuotaManagersLock;
    HashMap<PAL::SessionID, std::unique_ptr<SessionStorageQuotaManager>> m_sessionStorageQuotaManagers;

    OptionSet<NetworkCache::CacheOption> m_cacheOptions;
    PurCFetcher::MessagePortChannelRegistry m_messagePortChannelRegistry;

    static const Seconds defaultServiceWorkerFetchTimeout;
    Seconds m_serviceWorkerFetchTimeout { defaultServiceWorkerFetchTimeout };
};

} // namespace PurCFetcher
