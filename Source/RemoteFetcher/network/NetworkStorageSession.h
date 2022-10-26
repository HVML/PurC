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

#include "CredentialStorage.h"
#include "FrameIdentifier.h"
#include "PageIdentifier.h"
#include "RegistrableDomain.h"
#include "ShouldRelaxThirdPartyCookieBlocking.h"
#include "SessionID.h"
#include <wtf/CompletionHandler.h>
#include <wtf/Function.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/WallTime.h>
#include <wtf/text/WTFString.h>

#if USE(SOUP)
#include <wtf/Function.h>
#include <wtf/glib/GRefPtr.h>
typedef struct _SoupCookieJar SoupCookieJar;
#endif

#if USE(CURL)
#include "CookieJarDB.h"
#include <wtf/UniqueRef.h>
#endif

#ifdef __OBJC__
#include <objc/objc.h>
#endif

namespace PurCFetcher {

class CurlProxySettings;
class NetworkingContext;
class ResourceRequest;

struct Cookie;
struct CookieRequestHeaderFieldProxy;
struct SameSiteInfo;

enum class HTTPCookieAcceptPolicy : uint8_t;
enum class IncludeSecureCookies : bool;
enum class IncludeHttpOnlyCookies : bool;
enum class ThirdPartyCookieBlockingMode : uint8_t { All, AllExceptBetweenAppBoundDomains, AllOnSitesWithoutUserInteraction, OnlyAccordingToPerDomainPolicy };
enum class SameSiteStrictEnforcementEnabled : bool { Yes, No };
enum class FirstPartyWebsiteDataRemovalMode : uint8_t { AllButCookies, None, AllButCookiesLiveOnTestingTimeout, AllButCookiesReproTestingTimeout };
enum class ShouldAskITP : bool { No, Yes };

#if HAVE(COOKIE_CHANGE_LISTENER_API)
class CookieChangeObserver {
public:
    virtual ~CookieChangeObserver() { }
    virtual void cookiesAdded(const String& host, const Vector<PurCFetcher::Cookie>&) = 0;
    virtual void cookiesDeleted(const String& host, const Vector<PurCFetcher::Cookie>&) = 0;
    virtual void allCookiesDeleted() = 0;
};
#endif

class NetworkStorageSession {
    WTF_MAKE_NONCOPYABLE(NetworkStorageSession); WTF_MAKE_FAST_ALLOCATED;
public:
    using TopFrameDomain = PurCFetcher::RegistrableDomain;
    using SubResourceDomain = PurCFetcher::RegistrableDomain;

    PURCFETCHER_EXPORT static void permitProcessToUseCookieAPI(bool);
    PURCFETCHER_EXPORT static bool processMayUseCookieAPI();

    PAL::SessionID sessionID() const { return m_sessionID; }
    CredentialStorage& credentialStorage() { return m_credentialStorage; }

#ifdef __OBJC__
    PURCFETCHER_EXPORT NSHTTPCookieStorage *nsCookieStorage() const;
#endif

#if USE(SOUP)
    PURCFETCHER_EXPORT explicit NetworkStorageSession(PAL::SessionID);
    ~NetworkStorageSession();

    SoupCookieJar* cookieStorage() const { return m_cookieStorage.get(); }
    void setCookieStorage(GRefPtr<SoupCookieJar>&&);
    void setCookieObserverHandler(Function<void ()>&&);
    void getCredentialFromPersistentStorage(const ProtectionSpace&, GCancellable*, Function<void (Credential&&)>&& completionHandler);
    void saveCredentialToPersistentStorage(const ProtectionSpace&, const Credential&);
#elif USE(CURL)
    PURCFETCHER_EXPORT NetworkStorageSession(PAL::SessionID);
    ~NetworkStorageSession();

    CookieJarDB& cookieDatabase() const;
    PURCFETCHER_EXPORT void setCookieDatabase(UniqueRef<CookieJarDB>&&);
    PURCFETCHER_EXPORT void setCookiesFromHTTPResponse(const URL& firstParty, const URL&, const String&) const;
    PURCFETCHER_EXPORT void setCookieAcceptPolicy(CookieAcceptPolicy) const;
    PURCFETCHER_EXPORT void setProxySettings(CurlProxySettings&&);
#else
    PURCFETCHER_EXPORT NetworkStorageSession(PAL::SessionID, NetworkingContext*);
    ~NetworkStorageSession();

    NetworkingContext* context() const;
#endif

    PURCFETCHER_EXPORT HTTPCookieAcceptPolicy cookieAcceptPolicy() const;
    PURCFETCHER_EXPORT void setCookie(const Cookie&);
    PURCFETCHER_EXPORT void setCookies(const Vector<Cookie>&, const URL&, const URL& mainDocumentURL);
    PURCFETCHER_EXPORT void setCookiesFromDOM(const URL& firstParty, const SameSiteInfo&, const URL&, Optional<FrameIdentifier>, Optional<PageIdentifier>, ShouldAskITP, const String&, ShouldRelaxThirdPartyCookieBlocking) const;
    PURCFETCHER_EXPORT void deleteCookie(const Cookie&);
    PURCFETCHER_EXPORT void deleteCookie(const URL&, const String&) const;
    PURCFETCHER_EXPORT void deleteAllCookies();
    PURCFETCHER_EXPORT void deleteAllCookiesModifiedSince(WallTime);
    PURCFETCHER_EXPORT void deleteCookiesForHostnames(const Vector<String>& cookieHostNames);
    PURCFETCHER_EXPORT void deleteCookiesForHostnames(const Vector<String>& cookieHostNames, IncludeHttpOnlyCookies);
    PURCFETCHER_EXPORT Vector<Cookie> getAllCookies();
    PURCFETCHER_EXPORT Vector<Cookie> getCookies(const URL&);
    PURCFETCHER_EXPORT void hasCookies(const RegistrableDomain&, CompletionHandler<void(bool)>&&) const;
    PURCFETCHER_EXPORT bool getRawCookies(const URL& firstParty, const SameSiteInfo&, const URL&, Optional<FrameIdentifier>, Optional<PageIdentifier>, ShouldAskITP, ShouldRelaxThirdPartyCookieBlocking, Vector<Cookie>&) const;
    PURCFETCHER_EXPORT void flushCookieStore();
    PURCFETCHER_EXPORT void getHostnamesWithCookies(HashSet<String>& hostnames);
    PURCFETCHER_EXPORT std::pair<String, bool> cookiesForDOM(const URL& firstParty, const SameSiteInfo&, const URL&, Optional<FrameIdentifier>, Optional<PageIdentifier>, IncludeSecureCookies, ShouldAskITP, ShouldRelaxThirdPartyCookieBlocking) const;
    PURCFETCHER_EXPORT std::pair<String, bool> cookieRequestHeaderFieldValue(const URL& firstParty, const SameSiteInfo&, const URL&, Optional<FrameIdentifier>, Optional<PageIdentifier>, IncludeSecureCookies, ShouldAskITP, ShouldRelaxThirdPartyCookieBlocking) const;
    PURCFETCHER_EXPORT std::pair<String, bool> cookieRequestHeaderFieldValue(const CookieRequestHeaderFieldProxy&) const;

    PURCFETCHER_EXPORT Vector<Cookie> domCookiesForHost(const String& host);

#if HAVE(COOKIE_CHANGE_LISTENER_API)
    PURCFETCHER_EXPORT void startListeningForCookieChangeNotifications(CookieChangeObserver&, const String& host);
    PURCFETCHER_EXPORT void stopListeningForCookieChangeNotifications(CookieChangeObserver&, const HashSet<String>& hosts);
    PURCFETCHER_EXPORT bool supportsCookieChangeListenerAPI() const;
#endif

#if ENABLE(RESOURCE_LOAD_STATISTICS)
    void setResourceLoadStatisticsEnabled(bool enabled) { m_isResourceLoadStatisticsEnabled = enabled; }
    PURCFETCHER_EXPORT bool shouldBlockCookies(const ResourceRequest&, Optional<FrameIdentifier>, Optional<PageIdentifier>, ShouldRelaxThirdPartyCookieBlocking) const;
    PURCFETCHER_EXPORT bool shouldBlockCookies(const URL& firstPartyForCookies, const URL& resource, Optional<FrameIdentifier>, Optional<PageIdentifier>, ShouldRelaxThirdPartyCookieBlocking) const;
    PURCFETCHER_EXPORT bool shouldBlockThirdPartyCookies(const RegistrableDomain&) const;
    PURCFETCHER_EXPORT bool shouldBlockThirdPartyCookiesButKeepFirstPartyCookiesFor(const RegistrableDomain&) const;
    PURCFETCHER_EXPORT void setAllCookiesToSameSiteStrict(const RegistrableDomain&, CompletionHandler<void()>&&);
    PURCFETCHER_EXPORT bool hasHadUserInteractionAsFirstParty(const RegistrableDomain&) const;
    PURCFETCHER_EXPORT void setPrevalentDomainsToBlockAndDeleteCookiesFor(const Vector<RegistrableDomain>&);
    PURCFETCHER_EXPORT void setPrevalentDomainsToBlockButKeepCookiesFor(const Vector<RegistrableDomain>&);
    PURCFETCHER_EXPORT void setDomainsWithUserInteractionAsFirstParty(const Vector<RegistrableDomain>&);
    PURCFETCHER_EXPORT void setAgeCapForClientSideCookies(Optional<Seconds>);
    PURCFETCHER_EXPORT bool hasStorageAccess(const RegistrableDomain& resourceDomain, const RegistrableDomain& firstPartyDomain, Optional<FrameIdentifier>, PageIdentifier) const;
    PURCFETCHER_EXPORT Vector<String> getAllStorageAccessEntries() const;
    PURCFETCHER_EXPORT void grantStorageAccess(const RegistrableDomain& resourceDomain, const RegistrableDomain& firstPartyDomain, Optional<FrameIdentifier>, PageIdentifier);
    PURCFETCHER_EXPORT void removeStorageAccessForFrame(FrameIdentifier, PageIdentifier);
    PURCFETCHER_EXPORT void clearPageSpecificDataForResourceLoadStatistics(PageIdentifier);
    PURCFETCHER_EXPORT void removeAllStorageAccess();
    PURCFETCHER_EXPORT void setCacheMaxAgeCapForPrevalentResources(Seconds);
    PURCFETCHER_EXPORT void resetCacheMaxAgeCapForPrevalentResources();
    PURCFETCHER_EXPORT Optional<Seconds> maxAgeCacheCap(const ResourceRequest&);
    PURCFETCHER_EXPORT void didCommitCrossSiteLoadWithDataTransferFromPrevalentResource(const RegistrableDomain& toDomain, PageIdentifier);
    PURCFETCHER_EXPORT void resetCrossSiteLoadsWithLinkDecorationForTesting();
    PURCFETCHER_EXPORT void setThirdPartyCookieBlockingMode(ThirdPartyCookieBlockingMode);
    PURCFETCHER_EXPORT void setAppBoundDomains(HashSet<RegistrableDomain>&&);
    PURCFETCHER_EXPORT void resetAppBoundDomains();
#endif

private:
#if HAVE(COOKIE_CHANGE_LISTENER_API)
    void registerCookieChangeListenersIfNecessary();
    void unregisterCookieChangeListenersIfNecessary();
#endif

    PAL::SessionID m_sessionID;

#if USE(SOUP)
    static void cookiesDidChange(NetworkStorageSession*);

    GRefPtr<SoupCookieJar> m_cookieStorage;
    Function<void ()> m_cookieObserverHandler;
#elif USE(CURL)
    mutable UniqueRef<CookieJarDB> m_cookieDatabase;
#else
    RefPtr<NetworkingContext> m_context;
#endif

#if HAVE(COOKIE_CHANGE_LISTENER_API)
    bool m_didRegisterCookieListeners { false };
    RetainPtr<NSMutableSet> m_subscribedDomainsForCookieChanges;
    HashMap<String, HashSet<CookieChangeObserver*>> m_cookieChangeObservers;
#endif

    CredentialStorage m_credentialStorage;

#if ENABLE(RESOURCE_LOAD_STATISTICS)
    bool m_isResourceLoadStatisticsEnabled = false;
    Optional<Seconds> clientSideCookieCap(const TopFrameDomain&, Optional<PageIdentifier>) const;
    bool shouldExemptDomainPairFromThirdPartyCookieBlocking(const TopFrameDomain&, const SubResourceDomain&) const;
    HashSet<RegistrableDomain> m_registrableDomainsToBlockAndDeleteCookiesFor;
    HashSet<RegistrableDomain> m_registrableDomainsToBlockButKeepCookiesFor;
    HashSet<RegistrableDomain> m_registrableDomainsWithUserInteractionAsFirstParty;
    HashMap<PageIdentifier, HashMap<FrameIdentifier, RegistrableDomain>> m_framesGrantedStorageAccess;
    HashMap<PageIdentifier, HashMap<RegistrableDomain, RegistrableDomain>> m_pagesGrantedStorageAccess;
    Optional<Seconds> m_cacheMaxAgeCapForPrevalentResources { };
    Optional<Seconds> m_ageCapForClientSideCookies { };
    Optional<Seconds> m_ageCapForClientSideCookiesShort { };
    HashMap<PurCFetcher::PageIdentifier, RegistrableDomain> m_navigatedToWithLinkDecorationByPrevalentResource;
    bool m_navigationWithLinkDecorationTestMode = false;
    ThirdPartyCookieBlockingMode m_thirdPartyCookieBlockingMode { ThirdPartyCookieBlockingMode::All };
    HashSet<RegistrableDomain> m_appBoundDomains;
#endif

    static bool m_processMayUseCookieAPI;
};

}

namespace PurCWTF {

template<> struct EnumTraits<PurCFetcher::ThirdPartyCookieBlockingMode> {
    using values = EnumValues<
        PurCFetcher::ThirdPartyCookieBlockingMode,
        PurCFetcher::ThirdPartyCookieBlockingMode::All,
        PurCFetcher::ThirdPartyCookieBlockingMode::AllExceptBetweenAppBoundDomains,
        PurCFetcher::ThirdPartyCookieBlockingMode::AllOnSitesWithoutUserInteraction,
        PurCFetcher::ThirdPartyCookieBlockingMode::OnlyAccordingToPerDomainPolicy
    >;
};

template<> struct EnumTraits<PurCFetcher::FirstPartyWebsiteDataRemovalMode> {
    using values = EnumValues<
        PurCFetcher::FirstPartyWebsiteDataRemovalMode,
        PurCFetcher::FirstPartyWebsiteDataRemovalMode::AllButCookies,
        PurCFetcher::FirstPartyWebsiteDataRemovalMode::None,
        PurCFetcher::FirstPartyWebsiteDataRemovalMode::AllButCookiesLiveOnTestingTimeout,
        PurCFetcher::FirstPartyWebsiteDataRemovalMode::AllButCookiesReproTestingTimeout
    >;
};

}
