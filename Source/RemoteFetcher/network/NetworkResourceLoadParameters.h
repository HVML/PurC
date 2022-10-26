/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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

#include "NetworkLoadParameters.h"
#include "PolicyDecision.h"
#include "SandboxExtension.h"
#include "UserContentControllerIdentifier.h"
#include "ContentSecurityPolicyResponseHeaders.h"
#include "CrossOriginAccessControl.h"
#include "FetchOptions.h"
#include <wtf/Seconds.h>

namespace IPC {
class Decoder;
class Encoder;
}

namespace PurCFetcher {

typedef uint64_t ResourceLoadIdentifier;

class NetworkResourceLoadParameters : public NetworkLoadParameters {
public:
    void encode(IPC::Encoder&) const;
    static Optional<NetworkResourceLoadParameters> decode(IPC::Decoder&);

    ResourceLoadIdentifier identifier { 0 };
    Vector<RefPtr<SandboxExtension>> requestBodySandboxExtensions; // Created automatically for the sender.
    RefPtr<SandboxExtension> resourceSandboxExtension; // Created automatically for the sender.
    Seconds maximumBufferingTime;
    RefPtr<PurCFetcher::SecurityOrigin> sourceOrigin;
    PurCFetcher::FetchOptions options;
    Optional<PurCFetcher::ContentSecurityPolicyResponseHeaders> cspResponseHeaders;
    PurCFetcher::HTTPHeaderMap originalRequestHeaders;
    bool shouldRestrictHTTPResponseAccess { false };
    PurCFetcher::PreflightPolicy preflightPolicy { PurCFetcher::PreflightPolicy::Consider };
    bool shouldEnableCrossOriginResourcePolicy { false };
    Vector<RefPtr<PurCFetcher::SecurityOrigin>> frameAncestorOrigins;
    bool isHTTPSUpgradeEnabled { false };
    bool pageHasResourceLoadClient { false };
    Optional<PurCFetcher::FrameIdentifier> parentFrameID;
    bool crossOriginAccessControlCheckEnabled { true };

#if ENABLE(SERVICE_WORKER)
    PurCFetcher::ServiceWorkersMode serviceWorkersMode { PurCFetcher::ServiceWorkersMode::None };
    Optional<PurCFetcher::ServiceWorkerRegistrationIdentifier> serviceWorkerRegistrationIdentifier;
    OptionSet<PurCFetcher::HTTPHeadersToKeepFromCleaning> httpHeadersToKeep;
#endif

#if ENABLE(CONTENT_EXTENSIONS)
    URL mainDocumentURL;
    Optional<UserContentControllerIdentifier> userContentControllerIdentifier;
#endif
    
    Optional<NavigatingToAppBoundDomain> isNavigatingToAppBoundDomain { NavigatingToAppBoundDomain::No };
};

} // namespace PurCFetcher
