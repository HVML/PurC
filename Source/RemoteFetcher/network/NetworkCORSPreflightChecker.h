/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#include "NetworkDataTask.h"
#include "WebPageProxyIdentifier.h"
#include "FrameIdentifier.h"
#include "NetworkLoadInformation.h"
#include "PageIdentifier.h"
#include "StoredCredentialsPolicy.h"
#include "SessionID.h"
#include <wtf/CompletionHandler.h>

namespace PurCFetcher {
class ResourceError;
class SecurityOrigin;
}

namespace PurCFetcher {

class NetworkProcess;
class NetworkResourceLoader;

class NetworkCORSPreflightChecker final : private NetworkDataTaskClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    struct Parameters {
        PurCFetcher::ResourceRequest originalRequest;
        Ref<PurCFetcher::SecurityOrigin> sourceOrigin;
        RefPtr<PurCFetcher::SecurityOrigin> topOrigin;
        String referrer;
        String userAgent;
        PAL::SessionID sessionID;
        WebPageProxyIdentifier webPageProxyID;
        PurCFetcher::StoredCredentialsPolicy storedCredentialsPolicy;
    };
    using CompletionCallback = CompletionHandler<void(PurCFetcher::ResourceError&&)>;

    NetworkCORSPreflightChecker(NetworkProcess&, NetworkResourceLoader*, Parameters&&, bool shouldCaptureExtraNetworkLoadMetrics, CompletionCallback&&);
    ~NetworkCORSPreflightChecker();
    const PurCFetcher::ResourceRequest& originalRequest() const { return m_parameters.originalRequest; }

    void startPreflight();

    PurCFetcher::NetworkTransactionInformation takeInformation();

private:
    void willPerformHTTPRedirection(PurCFetcher::ResourceResponse&&, PurCFetcher::ResourceRequest&&, RedirectCompletionHandler&&) final;
    void didReceiveChallenge(PurCFetcher::AuthenticationChallenge&&, NegotiatedLegacyTLS, ChallengeCompletionHandler&&) final;
    void didReceiveResponse(PurCFetcher::ResourceResponse&&, NegotiatedLegacyTLS, ResponseCompletionHandler&&) final;
    void didReceiveData(Ref<PurCFetcher::SharedBuffer>&&) final;
    void didCompleteWithError(const PurCFetcher::ResourceError&, const PurCFetcher::NetworkLoadMetrics&) final;
    void didSendData(uint64_t totalBytesSent, uint64_t totalBytesExpectedToSend) final;
    void wasBlocked() final;
    void cannotShowURL() final;
    void wasBlockedByRestrictions() final;

    Parameters m_parameters;
    Ref<NetworkProcess> m_networkProcess;
    PurCFetcher::ResourceResponse m_response;
    CompletionCallback m_completionCallback;
    RefPtr<NetworkDataTask> m_task;
    bool m_shouldCaptureExtraNetworkLoadMetrics { false };
    PurCFetcher::NetworkTransactionInformation m_loadInformation;
    WeakPtr<NetworkResourceLoader> m_networkResourceLoader;
};

} // namespace PurCFetcher
