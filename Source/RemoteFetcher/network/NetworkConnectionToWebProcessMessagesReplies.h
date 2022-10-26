/*
 * Copyright (C) 2010-2020 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "MessageNames.h"
#include "Cookie.h"
//#include "MessageWithMessagePorts.h"
#include "NetworkLoadInformation.h"
#include <wtf/Forward.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace PurCFetcher {
class NetworkLoadMetrics;
class ResourceError;
class ResourceResponse;
struct RequestStorageAccessResult;
}

namespace Messages {
namespace NetworkConnectionToWebProcess {

using PerformSynchronousLoadDelayedReply = CompletionHandler<void(const PurCFetcher::ResourceError& error, const PurCFetcher::ResourceResponse& response, const Vector<char>& data)>;

using TestProcessIncomingSyncMessagesWhenWaitingForSyncReplyDelayedReply = CompletionHandler<void(bool handled)>;

using CookiesForDOMDelayedReply = CompletionHandler<void(const String& cookieString, bool didAccessSecureCookies)>;

using CookieRequestHeaderFieldValueDelayedReply = CompletionHandler<void(const String& cookieString, bool didAccessSecureCookies)>;

using GetRawCookiesDelayedReply = CompletionHandler<void(const Vector<PurCFetcher::Cookie>& cookies)>;

using DomCookiesForHostDelayedReply = CompletionHandler<void(const Vector<PurCFetcher::Cookie>& cookies)>;

using BlobSizeDelayedReply = CompletionHandler<void(uint64_t resultSize)>;

using WriteBlobsToTemporaryFilesAsyncReply = CompletionHandler<void(const Vector<String>& fileNames)>;

#if ENABLE(RESOURCE_LOAD_STATISTICS)
using HasStorageAccessAsyncReply = CompletionHandler<void(bool hasStorageAccess)>;
#endif

#if ENABLE(RESOURCE_LOAD_STATISTICS)
using RequestStorageAccessAsyncReply = CompletionHandler<void(const PurCFetcher::RequestStorageAccessResult& result)>;
#endif

using GetNetworkLoadInformationResponseDelayedReply = CompletionHandler<void(const PurCFetcher::ResourceResponse& response)>;

using GetNetworkLoadIntermediateInformationDelayedReply = CompletionHandler<void(const Vector<PurCFetcher::NetworkTransactionInformation>& transactions)>;

using TakeNetworkLoadInformationMetricsDelayedReply = CompletionHandler<void(const PurCFetcher::NetworkLoadMetrics& networkMetrics)>;

#if ENABLE(SERVICE_WORKER)
using EstablishSWContextConnectionAsyncReply = CompletionHandler<void()>;
#endif

//using TakeAllMessagesForPortAsyncReply = CompletionHandler<void(const Vector<PurCFetcher::MessageWithMessagePorts>& messages, uint64_t messageBatchIdentifier)>;

using CheckRemotePortForActivityAsyncReply = CompletionHandler<void(bool hasActivity)>;

} // namespace NetworkConnectionToWebProcess
} // namespace Messages
