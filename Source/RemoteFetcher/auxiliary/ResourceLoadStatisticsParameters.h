/*
 * Copyright (C) 2020 Apple Inc. All rights reserved.
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

#include "ArgumentCoders.h"
#include "SandboxExtension.h"
#include "NetworkStorageSession.h"
#include "RegistrableDomain.h"
#include <wtf/text/WTFString.h>

namespace PurCFetcher {

struct ResourceLoadStatisticsParameters {

    String directory;
    SandboxExtension::Handle directoryExtensionHandle;
    bool enabled { false };
    bool isItpStateExplicitlySet { false };
    bool enableLogTestingEvent { false };
    bool shouldIncludeLocalhost { true };
    bool enableDebugMode { false };
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    PurCFetcher::ThirdPartyCookieBlockingMode thirdPartyCookieBlockingMode { PurCFetcher::ThirdPartyCookieBlockingMode::All };
    PurCFetcher::SameSiteStrictEnforcementEnabled sameSiteStrictEnforcementEnabled { PurCFetcher::SameSiteStrictEnforcementEnabled::No };
#endif
    PurCFetcher::FirstPartyWebsiteDataRemovalMode firstPartyWebsiteDataRemovalMode { PurCFetcher::FirstPartyWebsiteDataRemovalMode::AllButCookies };
    PurCFetcher::RegistrableDomain standaloneApplicationDomain;
    HashSet<PurCFetcher::RegistrableDomain> appBoundDomains;
    PurCFetcher::RegistrableDomain manualPrevalentResource;
    
    void encode(IPC::Encoder& encoder) const
    {
        encoder << directory;
        encoder << directoryExtensionHandle;
        encoder << enabled;
        encoder << isItpStateExplicitlySet;
        encoder << enableLogTestingEvent;
        encoder << shouldIncludeLocalhost;
        encoder << enableDebugMode;
#if ENABLE(RESOURCE_LOAD_STATISTICS)
        encoder << thirdPartyCookieBlockingMode;
        encoder << sameSiteStrictEnforcementEnabled;
#endif
        encoder << firstPartyWebsiteDataRemovalMode;
        encoder << standaloneApplicationDomain;
        encoder << appBoundDomains;
        encoder << manualPrevalentResource;
    }

    static Optional<ResourceLoadStatisticsParameters> decode(IPC::Decoder& decoder)
    {
        Optional<String> directory;
        decoder >> directory;
        if (!directory)
            return PurCWTF::nullopt;
        
        Optional<SandboxExtension::Handle> directoryExtensionHandle;
        decoder >> directoryExtensionHandle;
        if (!directoryExtensionHandle)
            return PurCWTF::nullopt;
        
        Optional<bool> enabled;
        decoder >> enabled;
        if (!enabled)
            return PurCWTF::nullopt;

        Optional<bool> isItpStateExplicitlySet;
        decoder >> isItpStateExplicitlySet;
        if (!isItpStateExplicitlySet)
            return PurCWTF::nullopt;

        Optional<bool> enableLogTestingEvent;
        decoder >> enableLogTestingEvent;
        if (!enableLogTestingEvent)
            return PurCWTF::nullopt;

        Optional<bool> shouldIncludeLocalhost;
        decoder >> shouldIncludeLocalhost;
        if (!shouldIncludeLocalhost)
            return PurCWTF::nullopt;

        Optional<bool> enableDebugMode;
        decoder >> enableDebugMode;
        if (!enableDebugMode)
            return PurCWTF::nullopt;

#if ENABLE(RESOURCE_LOAD_STATISTICS)
        Optional<PurCFetcher::ThirdPartyCookieBlockingMode> thirdPartyCookieBlockingMode;
        decoder >> thirdPartyCookieBlockingMode;
        if (!thirdPartyCookieBlockingMode)
            return PurCWTF::nullopt;

        Optional<PurCFetcher::SameSiteStrictEnforcementEnabled> sameSiteStrictEnforcementEnabled;
        decoder >> sameSiteStrictEnforcementEnabled;
        if (!sameSiteStrictEnforcementEnabled)
            return PurCWTF::nullopt;
#endif

        Optional<PurCFetcher::FirstPartyWebsiteDataRemovalMode> firstPartyWebsiteDataRemovalMode;
        decoder >> firstPartyWebsiteDataRemovalMode;
        if (!firstPartyWebsiteDataRemovalMode)
            return PurCWTF::nullopt;

        Optional<PurCFetcher::RegistrableDomain> standaloneApplicationDomain;
        decoder >> standaloneApplicationDomain;
        if (!standaloneApplicationDomain)
            return PurCWTF::nullopt;

        Optional<HashSet<PurCFetcher::RegistrableDomain>> appBoundDomains;
        decoder >> appBoundDomains;
        if (!appBoundDomains)
            return PurCWTF::nullopt;

        Optional<PurCFetcher::RegistrableDomain> manualPrevalentResource;
        decoder >> manualPrevalentResource;
        if (!manualPrevalentResource)
            return PurCWTF::nullopt;

        return {{
            WTFMove(*directory),
            WTFMove(*directoryExtensionHandle),
            WTFMove(*enabled),
            WTFMove(*isItpStateExplicitlySet),
            WTFMove(*enableLogTestingEvent),
            WTFMove(*shouldIncludeLocalhost),
            WTFMove(*enableDebugMode),
#if ENABLE(RESOURCE_LOAD_STATISTICS)
            WTFMove(*thirdPartyCookieBlockingMode),
            WTFMove(*sameSiteStrictEnforcementEnabled),
#endif
            WTFMove(*firstPartyWebsiteDataRemovalMode),
            WTFMove(*standaloneApplicationDomain),
            WTFMove(*appBoundDomains),
            WTFMove(*manualPrevalentResource),
        }};
    }
};

} // namespace PurCFetcher
