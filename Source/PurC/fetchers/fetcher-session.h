/*
 * @file fetcher-session.h
 * @author XueShuming
 * @date 2021/11/17
 * @brief The fetcher session class.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PURC_FETCHER_SESSION_H
#define PURC_FETCHER_SESSION_H

#if ENABLE(LINK_PURC_FETCHER)

#include "fetcher-internal.h"
#include "fetcher-messages-basic.h"

#include "WebCoreArgumentCoders.h"
#include "SharedBufferDataReference.h"
#include "Connection.h"
#include "MessageReceiverMap.h"
#include "ProcessLauncher.h"
#include "FormDataReference.h"

#include <wtf/ProcessID.h>
#include <wtf/SystemTracing.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/threads/BinarySemaphore.h>

using namespace PurCFetcher;

class PcFetcherSession : public IPC::Connection::Client {
    WTF_MAKE_NONCOPYABLE(PcFetcherSession);

public:
    PcFetcherSession(uint64_t sessionId,
            IPC::Connection::Identifier connectionIdentifier);

    ~PcFetcherSession();

    IPC::Connection* connection() const
    {
        ASSERT(m_connection);
        return m_connection.get();
    }

    void close();

    purc_variant_t requestAsync(
        const char* url,
        enum pcfetcher_request_method method,
        purc_variant_t params,
        uint32_t timeout,
        response_handler handler,
        void* ctxt);

    purc_rwstream_t requestSync(
        const char* url,
        enum pcfetcher_request_method method,
        purc_variant_t params,
        uint32_t timeout,
        struct pcfetcher_resp_header *resp_header);

    void wait(uint32_t timeout);
    void wakeUp(void);

protected:
    bool dispatchMessage(IPC::Connection&, IPC::Decoder&);
    bool dispatchSyncMessage(IPC::Connection&, IPC::Decoder&,
            std::unique_ptr<IPC::Encoder>&);

    void didClose(IPC::Connection&);
    void didReceiveInvalidMessage(IPC::Connection&, IPC::MessageName);
    const char* connectionName(void) { return "PcFetcherSession"; }

    void didReceiveMessage(IPC::Connection&, IPC::Decoder&);
    void didReceiveSyncMessage(IPC::Connection&, IPC::Decoder&,
            std::unique_ptr<IPC::Encoder>&);

    void didReceiveResponse(const PurCFetcher::ResourceResponse&, bool);
    void didReceiveSharedBuffer(IPC::SharedBufferDataReference&&,
            int64_t encodedDataLength);
    void didFinishResourceLoad(const PurCFetcher::NetworkLoadMetrics&);
    void didFailResourceLoad(const ResourceError& error);
    void willSendRequest(ResourceRequest&&,
            IPC::FormDataReference&& requestBody, ResourceResponse&&);

private:
    uint64_t m_sessionId;
    uint64_t m_req_id;
    bool m_is_async;

    RefPtr<IPC::Connection> m_connection;
    IPC::MessageReceiverMap m_messageReceiverMap;
    BinarySemaphore m_waitForSyncReplySemaphore;
    struct pcfetcher_resp_header m_resp_header;

    response_handler m_req_handler;
    void* m_req_ctxt;

    purc_rwstream_t m_resp_rwstream;
    purc_variant_t m_req_vid;
};


#endif // ENABLE(LINK_PURC_FETCHER)

#endif /* not defined PURC_FETCHER_SESSION_H */
