/*
 * @file fetcher-session.cpp
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

#include "fetcher-session.h"
#include "fetcher-messages.h"

#include "NetworkResourceLoadParameters.h"
#include "ResourceError.h"
#include "ResourceResponse.h"

#include <wtf/RunLoop.h>

#define DEF_RWS_SIZE 1024

using namespace PurCFetcher;

PcFetcherSession::PcFetcherSession(uint64_t sessionId,
        IPC::Connection::Identifier identifier)
    : m_sessionId(sessionId)
    , m_req_id(0)
    , m_is_async(false)
    , m_connection(IPC::Connection::createClientConnection(identifier, *this))
    , m_req_handler(NULL)
    , m_req_ctxt(NULL)
    , m_resp_rwstream(NULL)
{
    memset(&m_resp_header, 0, sizeof(m_resp_header));
    m_connection->open();
}

PcFetcherSession::~PcFetcherSession()
{
    close();
    if (m_resp_header.mime_type) {
        free(m_resp_header.mime_type);
    }
    if (m_resp_rwstream) {
        purc_rwstream_destroy(m_resp_rwstream);
    }
}

void PcFetcherSession::close()
{
    m_connection->invalidate();
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

purc_variant_t PcFetcherSession::requestAsync(
        const char* base_uri,
        const char* url,
        enum pcfetcher_request_method method,
        purc_variant_t params,
        uint32_t timeout,
        response_handler handler,
        void* ctxt)
{
    // TODO send params with http request
    UNUSED_PARAM(params);

    m_req_handler = handler;
    m_req_ctxt = ctxt;
    m_is_async = true;

    String uri;
    if (base_uri) {
        uri.append(base_uri);
    }
    uri.append(url);
    std::unique_ptr<WTF::URL> wurl = makeUnique<URL>(URL(), uri);

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

    m_req_vid = purc_variant_make_ulongint(m_req_id);
    return m_req_vid;
}

purc_rwstream_t PcFetcherSession::requestSync(
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
    if (base_uri) {
        uri.append(base_uri);
    }
    uri.append(url);
    std::unique_ptr<WTF::URL> wurl = makeUnique<URL>(URL(), uri);

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

    if (resp_header) {
        resp_header->ret_code = m_resp_header.ret_code;
        if (m_resp_header.mime_type) {
            resp_header->mime_type = strdup(m_resp_header.mime_type);
        }
        else {
            resp_header->mime_type = NULL;
        }
        resp_header->sz_resp = m_resp_header.sz_resp;
    }

    purc_rwstream_t rws = m_resp_rwstream;
    m_resp_rwstream = NULL;
    return rws;
}

void PcFetcherSession::wait(uint32_t timeout)
{
    m_waitForSyncReplySemaphore.waitFor(Seconds(timeout));
}

void PcFetcherSession::wakeUp(void)
{
    m_waitForSyncReplySemaphore.signal();
}

void PcFetcherSession::didClose(IPC::Connection&)
{
}

void PcFetcherSession::didReceiveInvalidMessage(IPC::Connection&,
        IPC::MessageName)
{
}

void PcFetcherSession::didReceiveMessage(IPC::Connection&,
        IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::WebResourceLoader::DidReceiveResponse::name()) {
        IPC::handleMessage<Messages::WebResourceLoader::DidReceiveResponse>(
                decoder, this, &PcFetcherSession::didReceiveResponse);
        return;
    }
    if (decoder.messageName() == Messages::WebResourceLoader::DidReceiveSharedBuffer::name()) {
        IPC::handleMessage<Messages::WebResourceLoader::DidReceiveSharedBuffer>(
                decoder, this, &PcFetcherSession::didReceiveSharedBuffer);
        return;
    }
    if (decoder.messageName() == Messages::WebResourceLoader::DidFinishResourceLoad::name()) {
        IPC::handleMessage<Messages::WebResourceLoader::DidFinishResourceLoad>(
                decoder, this, &PcFetcherSession::didFinishResourceLoad);
        return;
    }
    if (decoder.messageName() == Messages::WebResourceLoader::DidFailResourceLoad::name()) {
        IPC::handleMessage<Messages::WebResourceLoader::DidFailResourceLoad>(
                decoder, this, &PcFetcherSession::didFailResourceLoad);
        return;
    }
    if (decoder.messageName() == Messages::WebResourceLoader::WillSendRequest::name()) {
        IPC::handleMessage<Messages::WebResourceLoader::WillSendRequest>(
                decoder, this, &PcFetcherSession::willSendRequest);
        return;
    }
}

void PcFetcherSession::didReceiveSyncMessage(IPC::Connection& connection,
        IPC::Decoder& decoder, std::unique_ptr<IPC::Encoder>& replyEncoder)
{
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    UNUSED_PARAM(replyEncoder);
}

void PcFetcherSession::didReceiveResponse(
        const PurCFetcher::ResourceResponse& response,
        bool needsContinueDidReceiveResponseMessage)
{
    UNUSED_PARAM(needsContinueDidReceiveResponseMessage);
    m_resp_header.ret_code = response.httpStatusCode();
    if (m_resp_header.mime_type) {
        free(m_resp_header.mime_type);
    }
    const CString &utf8 = response.mimeType().utf8();
    m_resp_header.mime_type = strdup((const char*)utf8.data());
    m_resp_header.sz_resp = response.expectedContentLength();
    if (m_resp_rwstream) {
        purc_rwstream_destroy(m_resp_rwstream);
    }
    size_t init = m_resp_header.sz_resp ? m_resp_header.sz_resp : DEF_RWS_SIZE;
    m_resp_rwstream = purc_rwstream_new_buffer(init, INT_MAX);
}

void PcFetcherSession::didReceiveSharedBuffer(
        IPC::SharedBufferDataReference&& data, int64_t encodedDataLength)
{
    UNUSED_PARAM(encodedDataLength);
    purc_rwstream_write(m_resp_rwstream, data.data(), data.size());
}

void PcFetcherSession::didFinishResourceLoad(
        const NetworkLoadMetrics& networkLoadMetrics)
{
    UNUSED_PARAM(networkLoadMetrics);

    if (m_is_async) {
        if (m_req_handler) {
            if (!m_resp_header.sz_resp && m_resp_rwstream) {
                size_t sz_content = 0;
                size_t sz_buffer = 0;
                purc_rwstream_get_mem_buffer_ex(m_resp_rwstream, &sz_content,
                        &sz_buffer, false);
                m_resp_header.sz_resp = sz_content;
            }
            if (m_resp_rwstream) {
                purc_rwstream_seek(m_resp_rwstream, 0, SEEK_SET);
            }
            RunLoop::main().dispatch([session=this] {
                session->m_req_handler(session->m_req_vid,
                        session->m_req_ctxt,
                        &session->m_resp_header,
                       session->m_resp_rwstream);
                session->m_resp_rwstream = NULL;
                delete session;
            });
        }
    }
    else {
        wakeUp();
    }
}

void PcFetcherSession::didFailResourceLoad(const ResourceError& error)
{
    UNUSED_PARAM(error);
    // TODO : trans error code
    m_resp_header.ret_code = 408;

    if (m_is_async) {
        if (m_req_handler) {
            if (!m_resp_header.sz_resp && m_resp_rwstream) {
                size_t sz_content = 0;
                size_t sz_buffer = 0;
                purc_rwstream_get_mem_buffer_ex(m_resp_rwstream, &sz_content,
                        &sz_buffer, false);
                m_resp_header.sz_resp = sz_content;
            }
            if (m_resp_rwstream) {
                purc_rwstream_seek(m_resp_rwstream, 0, SEEK_SET);
            }
            RunLoop::main().dispatch([session=this] {
                session->m_req_handler(session->m_req_vid,
                        session->m_req_ctxt,
                        &session->m_resp_header,
                       session->m_resp_rwstream);
                session->m_resp_rwstream = NULL;
                delete session;
            });
        }
    }
    else {
        wakeUp();
    }
}

void PcFetcherSession::willSendRequest(ResourceRequest&& proposedRequest,
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

