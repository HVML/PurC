/*
 * @file fetcher-request.cpp
 * @author XueShuming
 * @date 2021/11/17
 * @brief The impl for fetcher session.
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

#include "config.h"

#if ENABLE(REMOTE_FETCHER)

#include "fetcher-request.h"
#include "fetcher-process.h"
#include "fetcher-messages.h"

#include "NetworkResourceLoadParameters.h"
#include "ResourceError.h"
#include "ResourceResponse.h"

#include <wtf/RunLoop.h>

#define DEF_RWS_SIZE 1024


// Always start progress at initialProgressValue. This helps provide feedback as
// soon as a load starts.
static const double initialProgressValue = 0.1;

// Similarly, always leave space at the end. This helps show the user that we're not done
// until we're done.
static const double finalProgressValue = 0.9; // 1.0 - initialProgressValue
static const int progressItemDefaultEstimatedLength = 1024 * 16;

using namespace PurCFetcher;

extern "C"  struct pcinst* pcinst_current(void);

PcFetcherRequest::PcFetcherRequest(uint64_t sessionId,
        IPC::Connection::Identifier identifier,
        WorkQueue *queue, PcFetcherProcess *process)
    : m_sessionId(sessionId)
    , m_req_id(0)
    , m_is_async(false)
    , m_connection(IPC::Connection::createClientConnection(identifier, *this, queue))
    , m_workQueue(queue)
    , m_fetcherProcess(process)
{
    auto locker = holdLock(m_callbackLock);
    m_callback = pcfetcher_create_callback_info();
    if (m_callback == NULL) {
        return;
    }

    m_connection->open();
    m_runloop = &RunLoop::current();
}

PcFetcherRequest::~PcFetcherRequest()
{
    close();
    auto locker = holdLock(m_callbackLock);
    if (m_callback) {
        pcfetcher_destroy_callback_info(m_callback);
    }
}

void PcFetcherRequest::close()
{
    if (m_connection) {
        m_connection->invalidate();
        m_connection = nullptr;
    }
}

static const char* transMethod(enum pcfetcher_request_method method)
{
    switch (method)
    {
        case PCFETCHER_REQUEST_METHOD_GET:
            return "GET";

        case PCFETCHER_REQUEST_METHOD_POST:
            return "POST";

        case PCFETCHER_REQUEST_METHOD_DELETE:
            return "DELETE";

        default:
            return "GET";
    }
}

purc_variant_t PcFetcherRequest::requestAsync(
        const char* base_uri,
        const char* url,
        enum pcfetcher_request_method method,
        purc_variant_t params,
        uint32_t timeout,
        pcfetcher_response_handler handler,
        void* ctxt,
        pcfetcher_progress_tracker tracker,
        void* tracker_ctxt)
{
    // TODO send params with http request
    UNUSED_PARAM(params);

    auto locker = holdLock(m_callbackLock);
    m_callback->handler = handler;
    m_callback->ctxt = ctxt;
    m_callback->tracker = tracker;
    m_callback->tracker_ctxt = tracker_ctxt;
    m_is_async = true;

    String uri;
    if (base_uri &&
            strncmp(url, base_uri, strlen(base_uri)) != 0) {
        uri.append(base_uri);
    }
    uri.append(url);
    std::unique_ptr<PurCWTF::URL> wurl = makeUnique<URL>(URL(), uri);

    ResourceRequest request;
    request.setURL(*wurl);
    request.setHTTPMethod(transMethod(method));
    request.setTimeoutInterval(timeout);

    m_req_id = ProcessIdentifier::generate().toUInt64();
    NetworkResourceLoadParameters loadParameters;
    loadParameters.identifier = m_req_id;
    loadParameters.request = request;
    loadParameters.webPageProxyID = WebPageProxyIdentifier::generate();
    loadParameters.webPageID = PageIdentifier::generate();
    loadParameters.webFrameID = FrameIdentifier::generate();
    loadParameters.parentPID = getpid();

    m_connection->send(
            Messages::NetworkConnectionToWebProcess::ScheduleResourceLoad(
                loadParameters), 0);

    m_callback->req_id = purc_variant_make_native(this, NULL);
    return m_callback->req_id;
}

purc_rwstream_t PcFetcherRequest::requestSync(
        const char* base_uri,
        const char* url,
        enum pcfetcher_request_method method,
        purc_variant_t params,
        uint32_t timeout,
        struct pcfetcher_resp_header *resp_header)
{
    // TODO send params with http request
    UNUSED_PARAM(params);

    m_is_async = false;

    String uri;
    if (base_uri &&
            strncmp(url, base_uri, strlen(base_uri)) != 0) {
        uri.append(base_uri);
    }
    uri.append(url);
    std::unique_ptr<PurCWTF::URL> wurl = makeUnique<URL>(URL(), uri);

    ResourceRequest request;
    request.setURL(*wurl);
    request.setHTTPMethod(transMethod(method));
    request.setTimeoutInterval(timeout);

    m_req_id = ProcessIdentifier::generate().toUInt64();
    NetworkResourceLoadParameters loadParameters;
    loadParameters.identifier = m_req_id;
    loadParameters.request = request;
    loadParameters.webPageProxyID = WebPageProxyIdentifier::generate();
    loadParameters.webPageID = PageIdentifier::generate();
    loadParameters.webFrameID = FrameIdentifier::generate();
    loadParameters.parentPID = getpid();

    m_connection->send(
            Messages::NetworkConnectionToWebProcess::ScheduleResourceLoad(
                loadParameters), 0);

    wait(timeout);

    purc_rwstream_t rws = NULL;
    {
        auto locker = holdLock(m_callbackLock);
        if (m_callback == NULL) {
            return NULL;
        }

        if (!m_callback->header.sz_resp && m_callback->rws) {
            size_t sz_content = 0;
            size_t sz_buffer = 0;
            purc_rwstream_get_mem_buffer_ex(m_callback->rws, &sz_content,
                    &sz_buffer, false);
            m_callback->header.sz_resp = sz_content;
        }

        if (resp_header) {
            resp_header->ret_code = m_callback->header.ret_code;
            if (m_callback->header.mime_type) {
                resp_header->mime_type = strdup(m_callback->header.mime_type);
            }
            else {
                resp_header->mime_type = NULL;
            }
            resp_header->sz_resp = m_callback->header.sz_resp;
        }

        if (m_callback->rws) {
            purc_rwstream_seek(m_callback->rws, 0, SEEK_SET);
        }

        rws = m_callback->rws;
        m_callback->rws = NULL;
    }
    m_fetcherProcess->requestFinished(this);
    return rws;
}

void PcFetcherRequest::stop()
{
    auto locker = holdLock(m_callbackLock);
    if (!m_is_async || m_callback == NULL) {
        return;
    }
    struct pcfetcher_callback_info *info = m_callback;
    m_callback->cancelled = true;
    m_callback = nullptr;

    info->header.ret_code = RESP_CODE_USER_STOP;
    m_runloop->dispatch([info, request=this] {
            info->handler(info->req_id, info->ctxt, &info->header, NULL);
            info->handler = nullptr;
            pcfetcher_destroy_callback_info(info);
            request->m_fetcherProcess->requestFinished(request);
            }
        );
}

void PcFetcherRequest::cancel()
{
    auto locker = holdLock(m_callbackLock);
    if (!m_is_async || m_callback == NULL) {
        return;
    }

    struct pcfetcher_callback_info *info = m_callback;
    m_callback->cancelled = true;
    m_callback = nullptr;

    info->header.ret_code = RESP_CODE_USER_CANCEL;
    m_runloop->dispatch([info, request=this] {
            info->handler(info->req_id, info->ctxt, &info->header, NULL);
            info->handler = nullptr;
            pcfetcher_destroy_callback_info(info);
            request->m_fetcherProcess->requestFinished(request);
            }
        );
}

void PcFetcherRequest::wait(uint32_t timeout)
{
    m_waitForSyncReplySemaphore.waitFor(Seconds(timeout));
}

void PcFetcherRequest::wakeUp(void)
{
    m_waitForSyncReplySemaphore.signal();
}

void PcFetcherRequest::didClose(IPC::Connection&)
{
}

void PcFetcherRequest::didReceiveInvalidMessage(IPC::Connection&,
        IPC::MessageName)
{
}

void PcFetcherRequest::didReceiveMessage(IPC::Connection&,
        IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::WebResourceLoader::DidReceiveResponse::name()) {
        IPC::handleMessage<Messages::WebResourceLoader::DidReceiveResponse>(
                decoder, this, &PcFetcherRequest::didReceiveResponse);
        return;
    }
    if (decoder.messageName() == Messages::WebResourceLoader::DidReceiveSharedBuffer::name()) {
        IPC::handleMessage<Messages::WebResourceLoader::DidReceiveSharedBuffer>(
                decoder, this, &PcFetcherRequest::didReceiveSharedBuffer);
        return;
    }
    if (decoder.messageName() == Messages::WebResourceLoader::DidFinishResourceLoad::name()) {
        IPC::handleMessage<Messages::WebResourceLoader::DidFinishResourceLoad>(
                decoder, this, &PcFetcherRequest::didFinishResourceLoad);
        return;
    }
    if (decoder.messageName() == Messages::WebResourceLoader::DidFailResourceLoad::name()) {
        IPC::handleMessage<Messages::WebResourceLoader::DidFailResourceLoad>(
                decoder, this, &PcFetcherRequest::didFailResourceLoad);
        return;
    }
    if (decoder.messageName() == Messages::WebResourceLoader::WillSendRequest::name()) {
        IPC::handleMessage<Messages::WebResourceLoader::WillSendRequest>(
                decoder, this, &PcFetcherRequest::willSendRequest);
        return;
    }
}

void PcFetcherRequest::didReceiveSyncMessage(IPC::Connection& connection,
        IPC::Decoder& decoder, std::unique_ptr<IPC::Encoder>& replyEncoder)
{
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    UNUSED_PARAM(replyEncoder);
}

void PcFetcherRequest::didReceiveResponse(
        const PurCFetcher::ResourceResponse& response,
        bool needsContinueDidReceiveResponseMessage)
{
    UNUSED_PARAM(needsContinueDidReceiveResponseMessage);

    auto locker = holdLock(m_callbackLock);
    if (m_callback == NULL) {
        return;
    }

    m_callback->header.ret_code = response.httpStatusCode();
    /* files:// */
    if (m_callback->header.ret_code == 0) {
        m_callback->header.ret_code = 200;
    }
    if (m_callback->header.mime_type) {
        free(m_callback->header.mime_type);
    }
    const CString &utf8 = response.mimeType().utf8();
    m_callback->header.mime_type = strdup((const char*)utf8.data());
    m_callback->header.sz_resp = response.expectedContentLength();
    if (m_callback->rws) {
        purc_rwstream_destroy(m_callback->rws);
    }

    size_t init;
    if (m_callback->header.sz_resp <= 0) {
        init = DEF_RWS_SIZE;
        m_estimatedLength = progressItemDefaultEstimatedLength;
    }
    else {
        init = m_callback->header.sz_resp;
        m_estimatedLength = m_callback->header.sz_resp;
    }
    m_bytesReceived = 0;
    m_progressValue = initialProgressValue;
    if (m_callback->tracker) {
        struct pcfetcher_callback_info *info = m_callback;
        m_runloop->dispatch([info, request=this] {
                info->tracker(info->req_id, info->tracker_ctxt,
                        request->m_progressValue);
            }
        );
    }
    m_callback->rws = purc_rwstream_new_buffer(init, INT_MAX);
}

void PcFetcherRequest::didReceiveSharedBuffer(
        IPC::SharedBufferDataReference&& data, int64_t encodedDataLength)
{
    UNUSED_PARAM(encodedDataLength);
    auto locker = holdLock(m_callbackLock);
    if (m_callback == NULL) {
        return;
    }
    m_bytesReceived += data.size();
    if (m_bytesReceived > m_estimatedLength) {
        m_estimatedLength = m_bytesReceived * 2;
    }
    double increment, percentOfRemainingBytes;
    long long remainingBytes = m_estimatedLength - m_bytesReceived;
    if (remainingBytes > 0)  // Prevent divide by 0.
         percentOfRemainingBytes = (double)data.size() / (double)remainingBytes;
    else
        percentOfRemainingBytes = 1.0;

    double maxProgressValue = finalProgressValue;
    increment = (maxProgressValue - m_progressValue) * percentOfRemainingBytes;
    m_progressValue += increment;
    m_progressValue = std::min(m_progressValue, maxProgressValue);
    if (m_callback->tracker) {
        struct pcfetcher_callback_info *info = m_callback;
        m_runloop->dispatch([info, request=this] {
                info->tracker(info->req_id, info->tracker_ctxt,
                        request->m_progressValue);
            }
        );
    }

    purc_rwstream_write(m_callback->rws, data.data(), data.size());
}

void PcFetcherRequest::didFinishResourceLoad(
        const NetworkLoadMetrics& networkLoadMetrics)
{
    UNUSED_PARAM(networkLoadMetrics);
    auto locker = holdLock(m_callbackLock);
    m_progressValue = 1.0;
    if (m_callback == NULL) {
        return;
    }

    if (!m_is_async) {
        wakeUp();
        return;
    }

    if (m_callback->tracker) {
        struct pcfetcher_callback_info *info = m_callback;
        m_runloop->dispatch([info, request=this] {
                info->tracker(info->req_id, info->tracker_ctxt,
                        request->m_progressValue);
            }
        );
    }

    if (!m_callback->handler) {
        return;
    }
    if (!m_callback->header.sz_resp && m_callback->rws) {
        size_t sz_content = 0;
        size_t sz_buffer = 0;
        purc_rwstream_get_mem_buffer_ex(m_callback->rws, &sz_content,
                &sz_buffer, false);
        m_callback->header.sz_resp = sz_content;
    }
    if (m_callback->rws) {
        purc_rwstream_seek(m_callback->rws, 0, SEEK_SET);
    }
    struct pcfetcher_callback_info *info = m_callback;
    m_callback = NULL;
    m_runloop->dispatch([info, request=this] {
            info->handler(info->req_id, info->ctxt, &info->header,
                    info->rws);
            info->rws = NULL;
            pcfetcher_destroy_callback_info(info);
            request->m_fetcherProcess->requestFinished(request);
            }
        );
}

void PcFetcherRequest::didFailResourceLoad(const ResourceError& error)
{
    UNUSED_PARAM(error);
    auto locker = holdLock(m_callbackLock);
    if (m_callback == NULL) {
        return;
    }
    // TODO : trans error code
    m_callback->header.ret_code = 408;

    if (!m_is_async) {
        wakeUp();
        return;
    }

    if (!m_callback->handler) {
        return;
    }

    if (!m_callback->header.sz_resp && m_callback->rws) {
        size_t sz_content = 0;
        size_t sz_buffer = 0;
        purc_rwstream_get_mem_buffer_ex(m_callback->rws, &sz_content,
                &sz_buffer, false);
        m_callback->header.sz_resp = sz_content;
    }
    if (m_callback->rws) {
        purc_rwstream_seek(m_callback->rws, 0, SEEK_SET);
    }
    struct pcfetcher_callback_info *info = m_callback;
    m_callback = NULL;
    m_runloop->dispatch([info, request=this] {
            info->handler(info->req_id, info->ctxt, &info->header,
                    info->rws);
            info->rws = NULL;
            pcfetcher_destroy_callback_info(info);
            request->m_fetcherProcess->requestFinished(request);
            }
        );
}

void PcFetcherRequest::willSendRequest(ResourceRequest&& proposedRequest,
        IPC::FormDataReference&& proposedRequestBody,
        ResourceResponse&& redirectResponse)
{
    UNUSED_PARAM(proposedRequestBody);
    UNUSED_PARAM(redirectResponse);
    proposedRequest.setHTTPBody(proposedRequestBody.takeData());
    m_connection->send(
            Messages::NetworkResourceLoader::ContinueWillSendRequest(
                proposedRequest, true), m_req_id);
}

#endif // ENABLE(REMOTE_FETCHER)

