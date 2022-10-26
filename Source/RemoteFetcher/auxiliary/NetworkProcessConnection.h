/*
 * Copyright (C) 2010, 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef NetworkProcessConnection_h
#define NetworkProcessConnection_h

#include "Connection.h"
#include "ShareableResource.h"
#include "MessagePortChannelProvider.h"
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace IPC {
class DataReference;
}

namespace PurCFetcher {
class ResourceError;
class ResourceRequest;
class ResourceResponse;
struct Cookie;
struct MessagePortIdentifier;
struct MessageWithMessagePorts;
enum class HTTPCookieAcceptPolicy : uint8_t;
}

namespace PurCFetcher {

class WebIDBConnectionToServer;
class WebSWClientConnection;

typedef uint64_t ResourceLoadIdentifier;

class NetworkProcessConnection : public RefCounted<NetworkProcessConnection>, IPC::Connection::Client {
public:
    static Ref<NetworkProcessConnection> create(IPC::Connection::Identifier connectionIdentifier, PurCFetcher::HTTPCookieAcceptPolicy httpCookieAcceptPolicy)
    {
        return adoptRef(*new NetworkProcessConnection(connectionIdentifier, httpCookieAcceptPolicy));
    }
    ~NetworkProcessConnection();

private:
    NetworkProcessConnection(IPC::Connection::Identifier, PurCFetcher::HTTPCookieAcceptPolicy);


    void didReceiveNetworkProcessConnectionMessage(IPC::Connection&, IPC::Decoder&);
    // IPC::Connection::Client
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) override;
    void didReceiveSyncMessage(IPC::Connection&, IPC::Decoder&, std::unique_ptr<IPC::Encoder>&) override;
    void didClose(IPC::Connection&) override;
    void didReceiveInvalidMessage(IPC::Connection&, IPC::MessageName) override;
    const char* connectionName(void) override { return "NetworkProcessConnection"; };

    void didFinishPingLoad(uint64_t, PurCFetcher::ResourceError&&, PurCFetcher::ResourceResponse&&);
    void didFinishPreconnection(uint64_t, PurCFetcher::ResourceError&&);
    void setOnLineState(bool);
    void cookieAcceptPolicyChanged(PurCFetcher::HTTPCookieAcceptPolicy);

    void checkProcessLocalPortForActivity(const PurCFetcher::MessagePortIdentifier&, CompletionHandler<void(PurCFetcher::MessagePortChannelProvider::HasActivity)>&&);
    void messagesAvailableForPort(const PurCFetcher::MessagePortIdentifier&);

#if ENABLE(SHAREABLE_RESOURCE)
    // Message handlers.
    void didCacheResource(const PurCFetcher::ResourceRequest&, const ShareableResource::Handle&);
#endif

};

} // namespace PurCFetcher

#endif // NetworkProcessConnection_h
