/*
 * Copyright (C) 2010-2019 Apple Inc. All rights reserved.
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
#include "NetworkProcessConnection.h"

namespace PurCFetcher {

NetworkProcessConnection::NetworkProcessConnection(IPC::Connection::Identifier, HTTPCookieAcceptPolicy)
{
}

NetworkProcessConnection::~NetworkProcessConnection()
{
}

void NetworkProcessConnection::didReceiveMessage(IPC::Connection&, IPC::Decoder&)
{
}

void NetworkProcessConnection::didReceiveSyncMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&)
{
}

void NetworkProcessConnection::didClose(IPC::Connection&)
{
}

void NetworkProcessConnection::didReceiveInvalidMessage(IPC::Connection&, IPC::MessageName)
{
}


void NetworkProcessConnection::didFinishPingLoad(uint64_t, PurCFetcher::ResourceError&&, PurCFetcher::ResourceResponse&&)
{
}

void NetworkProcessConnection::didFinishPreconnection(uint64_t, PurCFetcher::ResourceError&&)
{
}

void NetworkProcessConnection::setOnLineState(bool)
{
}

void NetworkProcessConnection::cookieAcceptPolicyChanged(PurCFetcher::HTTPCookieAcceptPolicy)
{
}


void NetworkProcessConnection::checkProcessLocalPortForActivity(const PurCFetcher::MessagePortIdentifier&, CompletionHandler<void(PurCFetcher::MessagePortChannelProvider::HasActivity)>&&)
{
}

void NetworkProcessConnection::messagesAvailableForPort(const PurCFetcher::MessagePortIdentifier&)
{
}


#if ENABLE(SHAREABLE_RESOURCE)
// Message handlers.
void NetworkProcessConnection::didCacheResource(const PurCFetcher::ResourceRequest&, const ShareableResource::Handle&)
{
}

#endif


} // namespace PurCFetcher
