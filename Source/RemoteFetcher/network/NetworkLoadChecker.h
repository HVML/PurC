/*
 * Copyright (C) 2018-2019 Apple Inc. All rights reserved.
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

#include "UserContentControllerIdentifier.h"
#include "WebPageProxyIdentifier.h"
#include "ContentExtensionActions.h"
#include "ContentSecurityPolicyResponseHeaders.h"
#include "FetchOptions.h"
#include "NetworkLoadInformation.h"
#include "ResourceError.h"
#include "SessionID.h"
#include <wtf/CompletionHandler.h>
#include <wtf/Expected.h>
#include <wtf/Variant.h>
#include <wtf/WeakPtr.h>

namespace PurCFetcher {
//class ContentSecurityPolicy;
struct ContentSecurityPolicyClient;
class SecurityOrigin;
enum class PreflightPolicy : uint8_t;
enum class StoredCredentialsPolicy : uint8_t;
}

namespace PurCFetcher {

class NetworkCORSPreflightChecker;
class NetworkProcess;
class NetworkResourceLoader;
class NetworkSchemeRegistry;

class NetworkLoadChecker : public CanMakeWeakPtr<NetworkLoadChecker> {
    WTF_MAKE_FAST_ALLOCATED;
public:
    enum class LoadType : bool { MainFrame, Other };

    NetworkLoadChecker(NetworkProcess&, NetworkResourceLoader*, NetworkSchemeRegistry*, PurCFetcher::FetchOptions&&, PAL::SessionID, WebPageProxyIdentifier, PurCFetcher::HTTPHeaderMap&&, URL&&, RefPtr<PurCFetcher::SecurityOrigin>&&, RefPtr<PurCFetcher::SecurityOrigin>&& topOrigin, PurCFetcher::PreflightPolicy, String&& referrer, bool isHTTPSUpgradeEnabled = false, bool shouldCaptureExtraNetworkLoadMetrics = false, LoadType requestLoadType = LoadType::Other);
    ~NetworkLoadChecker();

    struct RedirectionTriplet {
        PurCFetcher::ResourceRequest request;
        PurCFetcher::ResourceRequest redirectRequest;
        PurCFetcher::ResourceResponse redirectResponse;
    };

    using RequestOrRedirectionTripletOrError = Variant<PurCFetcher::ResourceRequest, RedirectionTriplet, PurCFetcher::ResourceError>;
    using ValidationHandler = CompletionHandler<void(RequestOrRedirectionTripletOrError&&)>;
    void check(PurCFetcher::ResourceRequest&&, PurCFetcher::ContentSecurityPolicyClient*, ValidationHandler&&);

    using RedirectionRequestOrError = Expected<RedirectionTriplet, PurCFetcher::ResourceError>;
    using RedirectionValidationHandler = CompletionHandler<void(RedirectionRequestOrError&&)>;
    void checkRedirection(PurCFetcher::ResourceRequest&& request, PurCFetcher::ResourceRequest&& redirectRequest, PurCFetcher::ResourceResponse&& redirectResponse, PurCFetcher::ContentSecurityPolicyClient*, RedirectionValidationHandler&&);

    PurCFetcher::ResourceError validateResponse(const PurCFetcher::ResourceRequest&, PurCFetcher::ResourceResponse&);

    void setCSPResponseHeaders(PurCFetcher::ContentSecurityPolicyResponseHeaders&& headers) { m_cspResponseHeaders = WTFMove(headers); }
#if ENABLE(CONTENT_EXTENSIONS)
    void setContentExtensionController(URL&& mainDocumentURL, std::optional<UserContentControllerIdentifier> identifier)
    {
        m_mainDocumentURL = WTFMove(mainDocumentURL);
        m_userContentControllerIdentifier = identifier;
    }
#endif

    NetworkProcess& networkProcess() { return m_networkProcess; }

    const URL& url() const { return m_url; }
    PurCFetcher::StoredCredentialsPolicy storedCredentialsPolicy() const { return m_storedCredentialsPolicy; }

    PurCFetcher::NetworkLoadInformation takeNetworkLoadInformation() { return WTFMove(m_loadInformation); }
    void storeRedirectionIfNeeded(const PurCFetcher::ResourceRequest&, const PurCFetcher::ResourceResponse&);

    void enableContentExtensionsCheck() { m_checkContentExtensions = true; }

private:
    PurCFetcher::ContentSecurityPolicy* contentSecurityPolicy();
    bool isChecking() const { return !!m_corsPreflightChecker; }
    bool isRedirected() const { return m_redirectCount; }

    void checkRequest(PurCFetcher::ResourceRequest&&, PurCFetcher::ContentSecurityPolicyClient*, ValidationHandler&&);

    bool isAllowedByContentSecurityPolicy(const PurCFetcher::ResourceRequest&, PurCFetcher::ContentSecurityPolicyClient*);

    void continueCheckingRequest(PurCFetcher::ResourceRequest&&, ValidationHandler&&);
    void continueCheckingRequestOrDoSyntheticRedirect(PurCFetcher::ResourceRequest&& originalRequest, PurCFetcher::ResourceRequest&& currentRequest, ValidationHandler&&);

    bool doesNotNeedCORSCheck(const URL&) const;
    void checkCORSRequest(PurCFetcher::ResourceRequest&&, ValidationHandler&&);
    void checkCORSRedirectedRequest(PurCFetcher::ResourceRequest&&, ValidationHandler&&);
    void checkCORSRequestWithPreflight(PurCFetcher::ResourceRequest&&, ValidationHandler&&);

    RequestOrRedirectionTripletOrError accessControlErrorForValidationHandler(String&&);

#if ENABLE(CONTENT_EXTENSIONS)
    struct ContentExtensionResult {
        PurCFetcher::ResourceRequest request;
        const PurCFetcher::ContentRuleListResults& results;
    };
    using ContentExtensionResultOrError = Expected<ContentExtensionResult, PurCFetcher::ResourceError>;
    using ContentExtensionCallback = CompletionHandler<void(ContentExtensionResultOrError&&)>;
    void processContentRuleListsForLoad(PurCFetcher::ResourceRequest&&, ContentExtensionCallback&&);
#endif

    void applyHTTPSUpgradeIfNeeded(PurCFetcher::ResourceRequest&&, CompletionHandler<void(PurCFetcher::ResourceRequest&&)>&&) const;

    PurCFetcher::FetchOptions m_options;
    PurCFetcher::StoredCredentialsPolicy m_storedCredentialsPolicy;
    PAL::SessionID m_sessionID;
    Ref<NetworkProcess> m_networkProcess;
    WebPageProxyIdentifier m_webPageProxyID;
    PurCFetcher::HTTPHeaderMap m_originalRequestHeaders; // Needed for CORS checks.
    PurCFetcher::HTTPHeaderMap m_firstRequestHeaders; // Needed for CORS checks.
    URL m_url;
    RefPtr<PurCFetcher::SecurityOrigin> m_origin;
    RefPtr<PurCFetcher::SecurityOrigin> m_topOrigin;
    std::optional<PurCFetcher::ContentSecurityPolicyResponseHeaders> m_cspResponseHeaders;
#if ENABLE(CONTENT_EXTENSIONS)
    URL m_mainDocumentURL;
    std::optional<UserContentControllerIdentifier> m_userContentControllerIdentifier;
#endif

    std::unique_ptr<NetworkCORSPreflightChecker> m_corsPreflightChecker;
    bool m_isSameOriginRequest { true };
    bool m_isSimpleRequest { true };
//    std::unique_ptr<PurCFetcher::ContentSecurityPolicy> m_contentSecurityPolicy;
    size_t m_redirectCount { 0 };
    URL m_previousURL;
    PurCFetcher::PreflightPolicy m_preflightPolicy;
    String m_referrer;
    bool m_checkContentExtensions { false };
    bool m_shouldCaptureExtraNetworkLoadMetrics { false };

    PurCFetcher::NetworkLoadInformation m_loadInformation;

    LoadType m_requestLoadType;
    RefPtr<NetworkSchemeRegistry> m_schemeRegistry;
    WeakPtr<NetworkResourceLoader> m_networkResourceLoader;
};

}
