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

#include "config.h"
#include "NetworkProcessProxy.h"

#include "AuthenticationManager.h"
#include "DownloadProxyMessages.h"
#if ENABLE(LEGACY_CUSTOM_PROTOCOL_MANAGER)
#include "LegacyCustomProtocolManagerMessages.h"
#include "LegacyCustomProtocolManagerProxyMessages.h"
#endif
#include "Logging.h"
#include "NetworkProcessCreationParameters.h"
#include "NetworkProcessMessages.h"
#include "NetworkProcessProxyMessages.h"
#include "SandboxExtension.h"
#if HAVE(SEC_KEY_PROXY)
#include "SecKeyProxyStore.h"
#endif
#include <wtf/CompletionHandler.h>

#define MESSAGE_CHECK(assertion) MESSAGE_CHECK_BASE(assertion, connection())

namespace PurCFetcher {
using namespace PurCFetcher;

NetworkProcessProxy::NetworkProcessProxy()
{
}

NetworkProcessProxy::~NetworkProcessProxy()
{
}

void NetworkProcessProxy::didReceiveMessage(IPC::Connection&, IPC::Decoder&)
{
}

void NetworkProcessProxy::didReceiveSyncMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&)
{
}

void NetworkProcessProxy::didClose(IPC::Connection&)
{
}

void NetworkProcessProxy::didReceiveInvalidMessage(IPC::Connection&, IPC::MessageName)
{
}

void NetworkProcessProxy::didReceiveAuthenticationChallenge(PAL::SessionID, WebPageProxyIdentifier, const Optional<SecurityOriginData>&, PurCFetcher::AuthenticationChallenge&&, bool, uint64_t)
{
}

void NetworkProcessProxy::negotiatedLegacyTLS(WebPageProxyIdentifier)
{
}

void NetworkProcessProxy::didNegotiateModernTLS(WebPageProxyIdentifier, const PurCFetcher::AuthenticationChallenge&)
{
}

void NetworkProcessProxy::didFetchWebsiteData(CallbackID, const WebsiteData&)
{
}

void NetworkProcessProxy::didDeleteWebsiteData(CallbackID)
{
}

void NetworkProcessProxy::didDeleteWebsiteDataForOrigins(CallbackID)
{
}

void NetworkProcessProxy::setWebProcessHasUploads(PurCFetcher::ProcessIdentifier, bool)
{
}

void NetworkProcessProxy::logDiagnosticMessage(WebPageProxyIdentifier, const String&, const String&, PurCFetcher::ShouldSample)
{
}

void NetworkProcessProxy::logDiagnosticMessageWithResult(WebPageProxyIdentifier, const String&, const String&, uint32_t, PurCFetcher::ShouldSample)
{
}

void NetworkProcessProxy::logDiagnosticMessageWithValue(WebPageProxyIdentifier, const String&, const String&, double, unsigned, PurCFetcher::ShouldSample)
{
}

void NetworkProcessProxy::retrieveCacheStorageParameters(PAL::SessionID)
{
}


void NetworkProcessProxy::terminateWebProcess(PurCFetcher::ProcessIdentifier)
{
}

void NetworkProcessProxy::requestStorageSpace(PAL::SessionID, const PurCFetcher::ClientOrigin&, uint64_t, uint64_t, uint64_t, CompletionHandler<void(Optional<uint64_t> quota)>&&)
{
}

void NetworkProcessProxy::syncAllCookies()
{
}

void NetworkProcessProxy::didSyncAllCookies()
{
}

void NetworkProcessProxy::terminateUnresponsiveServiceWorkerProcesses(PurCFetcher::ProcessIdentifier)
{
}

void NetworkProcessProxy::setIsHoldingLockedFiles(bool)
{
}

void NetworkProcessProxy::getAppBoundDomains(PAL::SessionID, CompletionHandler<void(HashSet<PurCFetcher::RegistrableDomain>&&)>&&)
{
}

void NetworkProcessProxy::resourceLoadDidSendRequest(WebPageProxyIdentifier, ResourceLoadInfo&&, PurCFetcher::ResourceRequest&&, Optional<IPC::FormDataReference>&&)
{
}

void NetworkProcessProxy::resourceLoadDidPerformHTTPRedirection(WebPageProxyIdentifier, ResourceLoadInfo&&, PurCFetcher::ResourceResponse&&, PurCFetcher::ResourceRequest&&)
{
}

void NetworkProcessProxy::resourceLoadDidReceiveChallenge(WebPageProxyIdentifier, ResourceLoadInfo&&, PurCFetcher::AuthenticationChallenge&&)
{
}

void NetworkProcessProxy::resourceLoadDidReceiveResponse(WebPageProxyIdentifier, ResourceLoadInfo&&, PurCFetcher::ResourceResponse&&)
{
}

void NetworkProcessProxy::resourceLoadDidCompleteWithError(WebPageProxyIdentifier, ResourceLoadInfo&&, PurCFetcher::ResourceResponse&&, PurCFetcher::ResourceError&&)
{
}

void NetworkProcessProxy::testProcessIncomingSyncMessagesWhenWaitingForSyncReply(WebPageProxyIdentifier, Messages::NetworkProcessProxy::TestProcessIncomingSyncMessagesWhenWaitingForSyncReplyDelayedReply&&)
{
}

} // namespace PurCFetcher

#undef MESSAGE_CHECK
