/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
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

#include "fetcher-messages-basic.h"
#include "ResourceLoaderOptions.h"
#include "ResourceRequest.h"
#include "SecurityOrigin.h"
#include <wtf/EnumTraits.h>
#include <wtf/ProcessID.h>

namespace PurCFetcher {

enum class PreconnectOnly : bool { No, Yes };

class NetworkLoadParameters {
public:
    WebPageProxyIdentifier webPageProxyID;
    PurCFetcher::PageIdentifier webPageID;
    PurCFetcher::FrameIdentifier webFrameID;
    RefPtr<PurCFetcher::SecurityOrigin> topOrigin;
    PurCWTF::ProcessID parentPID { 0 };
#if HAVE(AUDIT_TOKEN)
    std::optional<audit_token_t> networkProcessAuditToken;
#endif
    PurCFetcher::ResourceRequest request;
    PurCFetcher::ContentSniffingPolicy contentSniffingPolicy { PurCFetcher::ContentSniffingPolicy::SniffContent };
    PurCFetcher::ContentEncodingSniffingPolicy contentEncodingSniffingPolicy { PurCFetcher::ContentEncodingSniffingPolicy::Sniff };
    PurCFetcher::StoredCredentialsPolicy storedCredentialsPolicy { PurCFetcher::StoredCredentialsPolicy::DoNotUse };
    PurCFetcher::ClientCredentialPolicy clientCredentialPolicy { PurCFetcher::ClientCredentialPolicy::CannotAskClientForCredentials };
    bool shouldClearReferrerOnHTTPSToHTTPRedirect { true };
    bool needsCertificateInfo { false };
    bool isMainFrameNavigation { false };
    bool isMainResourceNavigationForAnyFrame { false };
    PurCFetcher::ShouldRelaxThirdPartyCookieBlocking shouldRelaxThirdPartyCookieBlocking { PurCFetcher::ShouldRelaxThirdPartyCookieBlocking::No };
    PreconnectOnly shouldPreconnectOnly { PreconnectOnly::No };
    std::optional<NavigatingToAppBoundDomain> isNavigatingToAppBoundDomain { NavigatingToAppBoundDomain::No };
};

} // namespace PurCFetcher
