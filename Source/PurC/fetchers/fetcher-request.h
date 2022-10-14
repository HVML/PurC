/*
 * @file fetcher-request.h
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

#ifndef PURC_FETCHER_REQUEST_H
#define PURC_FETCHER_REQUEST_H

#if ENABLE(REMOTE_FETCHER)

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

class PcFetcherProcess;
class PcFetcherRequest : public IPC::Connection::Client {
    WTF_MAKE_NONCOPYABLE(PcFetcherRequest);

public:
    PcFetcherRequest(uint64_t sessionId,
            IPC::Connection::Identifier connectionIdentifier, WorkQueue *queue,
            PcFetcherProcess *process);

    ~PcFetcherRequest();

    IPC::Connection* connection() const
    {
        ASSERT(m_connection);
        return m_connection.get();
    }

    void close();

    purc_variant_t requestAsync(
        const char* base_uri,
        const char* url,
        enum pcfetcher_request_method method,
        purc_variant_t params,
        uint32_t timeout,
        pcfetcher_response_handler handler,
        void* ctxt);

    purc_rwstream_t requestSync(
        const char* base_uri,
        const char* url,
        enum pcfetcher_request_method method,
        purc_variant_t params,
        uint32_t timeout,
        struct pcfetcher_resp_header *resp_header);

    void stop();
    void cancel();
    purc_variant_t getRequestId()
    {
        return m_callback ? m_callback->req_id : PURC_VARIANT_INVALID;
    }

    void wait(uint32_t timeout);
    void wakeUp(void);

    RunLoop *getRunLoop() { return m_runloop; }

protected:
    bool dispatchMessage(IPC::Connection&, IPC::Decoder&);
    bool dispatchSyncMessage(IPC::Connection&, IPC::Decoder&,
            std::unique_ptr<IPC::Encoder>&);

    void didClose(IPC::Connection&);
    void didReceiveInvalidMessage(IPC::Connection&, IPC::MessageName);
    const char* connectionName(void) { return "PcFetcherRequest"; }

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
    BinarySemaphore m_waitForSyncReplySemaphore;

    RunLoop* m_runloop;
    WorkQueue* m_workQueue;

    Lock m_callbackLock;
    struct pcfetcher_callback_info *m_callback;

    PcFetcherProcess *m_fetcherProcess;

    long long m_estimatedLength {0};
    long long m_bytesReceived {0};
    double m_progressValue;
};


#endif // ENABLE(REMOTE_FETCHER)

#endif /* not defined PURC_FETCHER_REQUEST_H */
