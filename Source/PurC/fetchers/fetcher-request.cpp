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
#include "Cookie.h"

#include "private/url.h"

#include <wtf/RunLoop.h>

#define DEF_RWS_SIZE 1024


// Always start progress at initialProgressValue. This helps provide feedback as
// soon as a load starts.
static const double initialProgressValue = PCFETCHER_INITIAL_PROGRESS;

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

static const char* transMethod(enum pcfetcher_method method)
{
    switch (method)
    {
        case PCFETCHER_METHOD_GET:
            return "GET";

        case PCFETCHER_METHOD_POST:
            return "POST";

        case PCFETCHER_METHOD_DELETE:
            return "DELETE";

        default:
            return "GET";
    }
}

String pcfetcher_build_uri(const char *base_url,  const char *url);

static inline bool isValidHeaderNameCharacter(const char* character)
{
    // https://tools.ietf.org/html/rfc7230#section-3.2
    // A header name should only contain one or more of
    // alphanumeric or ! # $ % & ' * + - . ^ _ ` | ~
    if (isASCIIAlphanumeric(*character))
        return true;
    switch (*character) {
    case '!':
    case '#':
    case '$':
    case '%':
    case '&':
    case '\'':
    case '*':
    case '+':
    case '-':
    case '.':
    case '^':
    case '_':
    case '`':
    case '|':
    case '~':
        return true;
    default:
        return false;
    }
}

const UChar horizontalEllipsis = 0x2026;
static const size_t maxInputSampleSize = 128;
static String trimInputSample(const char* p, size_t length)
{
    String s = String(p, std::min<size_t>(length, maxInputSampleSize));
    if (length > maxInputSampleSize)
        s.append(horizontalEllipsis);
    return s;
}

static size_t parseHTTPHeader(const char* start, size_t length, String& failureReason,
        StringView& nameStr, String& valueStr, bool strict)
{
    const char* p = start;
    const char* end = start + length;

    Vector<char> name;
    Vector<char> value;

    bool foundFirstNameChar = false;
    const char* namePtr = nullptr;
    size_t nameSize = 0;

    nameStr = StringView();
    valueStr = String();

    for (; p < end; p++) {
        switch (*p) {
        case '\r':
            if (name.isEmpty()) {
                if (p + 1 < end && *(p + 1) == '\n')
                    return (p + 2) - start;
                failureReason = makeString("CR doesn't follow LF in header name at ", trimInputSample(p, end - p));
                return 0;
            }
            failureReason = makeString("Unexpected CR in header name at ", trimInputSample(name.data(), name.size()));
            return 0;
        case '\n':
            failureReason = makeString("Unexpected LF in header name at ", trimInputSample(name.data(), name.size()));
            return 0;
        case ':':
            break;
        default:
            if (!isValidHeaderNameCharacter(p)) {
                if (name.size() < 1)
                    failureReason = "Unexpected start character in header name";
                else
                    failureReason = makeString("Unexpected character in header name at ", trimInputSample(name.data(), name.size()));
                return 0;
            }
            name.append(*p);
            if (!foundFirstNameChar) {
                namePtr = p;
                foundFirstNameChar = true;
            }
            continue;
        }
        if (*p == ':') {
            ++p;
            break;
        }
    }

    nameSize = name.size();
    nameStr = StringView(namePtr, nameSize);

    for (; p < end && *p == 0x20; p++) { }

    for (; p < end; p++) {
        switch (*p) {
        case '\r':
            break;
        case '\n':
            if (strict) {
                failureReason = makeString("Unexpected LF in header value at ", trimInputSample(value.data(), value.size()));
                return 0;
            }
            break;
        default:
            value.append(*p);
        }
        if (*p == '\r' || (!strict && *p == '\n')) {
            ++p;
            break;
        }
    }
    if (p >= end || (strict && *p != '\n')) {
        failureReason = makeString("CR doesn't follow LF after header value at ", trimInputSample(p, end - p));
        return 0;
    }
    valueStr = String::fromUTF8(value.data(), value.size());
    if (valueStr.isNull()) {
        failureReason = "Invalid UTF-8 sequence in header value"_s;
        return 0;
    }
    return p - start;
}


static int fill_raw_header(
        ResourceRequest* request,
        purc_variant_t params)
{
    int ret = -1;
    size_t nr_buf = 0;
    const char *buf = NULL;
    const char *p;
    const char *end;
    // size_t nr_consumed = 0;
    String value;
    StringView name;

    if (!purc_variant_is_string(params)) {
        purc_set_error(PURC_ERROR_INVALID_VALUE);
        goto out;
    }

    buf = purc_variant_get_string_const_ex(params, &nr_buf);
    if (!buf || buf[0] == '\0') {
        ret = 0;
        goto out;
    }

    p = buf;
    end = buf + nr_buf;
    for (; p < end && (*p == 0x20 || *p == '\n' || *p == '\r'); p++) { }

    for (; p < end; ++p) {
        String failureReason;
        size_t consumedLength = parseHTTPHeader(p, end - p, failureReason,
                name, value, false);
        if (!consumedLength)
            break; // No more header to parse.

        p += consumedLength;
        // nr_consumed += consumedLength;

        // The name should not be empty, but the value could be empty.
        if (name.isEmpty()) {
            break;
        }

        request->setHTTPHeaderField(name.toString(), value);
    }

    if (end - p) {
        request->setHTTPBody(FormData::create(p, end - p));
    }

    ret = 0;
out:
    return ret;
}

static int fill_normal_params(
        std::unique_ptr<PurCWTF::URL> &url,
        enum pcfetcher_method method,
        ResourceRequest* request,
        purc_variant_t params)
{
    int ret = -1;
    const char *encode_p = NULL;

    purc_variant_t encode_val = pcutils_url_build_query(params, NULL, '&', 0);
    if (!encode_val) {
        goto out;
    }

    encode_p = purc_variant_get_string_const(encode_val);

    if (encode_p && encode_p[0]) {
        if (method == PCFETCHER_METHOD_GET) {
            url->setQuery(encode_p);
        }
        else {
            request->setHTTPBody(FormData::create(encode_p, strlen(encode_p)));
            request->setHTTPContentType("application/x-www-form-urlencoded");
        }
    }

    purc_variant_unref(encode_val);

    ret = 0;
out:
    return ret;
}

static int fill_request_param(
        std::unique_ptr<PurCWTF::URL> &url,
        enum pcfetcher_method method,
        ResourceRequest* request,
        purc_variant_t params)
{
    int ret = -1;
    bool raw_header = false;

    if (!params) {
        ret = 0;
        goto out;
    }

    if (purc_variant_is_object(params)) {
        purc_variant_t raw = purc_variant_object_get_by_ckey(params,
                FETCHER_PARAM_RAW_HEADER);
        if (raw && purc_variant_booleanize(raw)) {
            raw_header = true;
            params = purc_variant_object_get_by_ckey(params,
                    FETCHER_PARAM_DATA);
        }
        purc_clr_error();
    }

    if (!params) {
        ret = 0;
        goto out;
    }

    if (raw_header) {
        ret = fill_raw_header(request, params);
    }
    else {
        ret = fill_normal_params(url, method, request, params);
    }

out:
    return ret;
}

void PcFetcherRequest::setCookie(struct pcfetcher_session *session,
        const char *domain, const char *path)
{
    (void) path;
    struct list_head *cookies = &session->cookies;
    struct pcfetcher_cookie *p;
    struct pcfetcher_cookie *n;
    list_for_each_entry_safe(p, n, cookies, node) {
        /* purc-fetcher: soup do the actual matching operation */
        if (strcmp(p->domain, domain) == 0) {
            PurCFetcher::Cookie cookie;
            cookie.name = p->name;
            cookie.value = p->content;
            cookie.domain = p->domain;
            cookie.path = p->path;
            cookie.secure = p->secure;
            if (p->expire_time > 0) {
                cookie.expires = p->expire_time;
            }
            m_connection->send(
                    Messages::NetworkConnectionToWebProcess::SetRawCookie(cookie), 0);
        }
    }
}

purc_variant_t PcFetcherRequest::requestAsync(
        struct pcfetcher_session *session,
        const char* base_uri,
        const char* url,
        enum pcfetcher_method method,
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
    m_callback->session = session;
    m_callback->handler = handler;
    m_callback->ctxt = ctxt;
    m_callback->tracker = tracker;
    m_callback->tracker_ctxt = tracker_ctxt;
    m_is_async = true;
    m_session = session;

    String uri;
    if (base_uri) {
        uri = pcfetcher_build_uri(base_uri, url);
    }
    else {
        uri.append(url);
    }
    std::unique_ptr<PurCWTF::URL> wurl = makeUnique<URL>(URL(), uri);

    ResourceRequest request;
    if (fill_request_param(wurl, method, &request, params)) {
        return NULL;
    }

    setCookie(session, wurl->host().toString().utf8().data(),
            wurl->path().toString().utf8().data());

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
        struct pcfetcher_session *session,
        const char* base_uri,
        const char* url,
        enum pcfetcher_method method,
        purc_variant_t params,
        uint32_t timeout,
        struct pcfetcher_resp_header *resp_header)
{
    // TODO send params with http request
    UNUSED_PARAM(params);

    m_session = session;
    m_is_async = false;

    String uri;
    URL tmp(URL(), url);
    if (base_uri && !tmp.isValid() &&
            strncmp(url, base_uri, strlen(base_uri)) != 0) {
        uri.append(base_uri);
    }
    uri.append(url);
    std::unique_ptr<PurCWTF::URL> wurl = makeUnique<URL>(URL(), uri);

    ResourceRequest request;
    if (fill_request_param(wurl, method, &request, params)) {
        return NULL;
    }

    setCookie(session, wurl->host().toString().utf8().data(),
            wurl->path().toString().utf8().data());

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
            info->handler(info->session, info->req_id, info->ctxt,
                    PCFETCHER_RESP_TYPE_ERROR,
                    (const char *)&info->header, 0);
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
            info->handler(info->session, info->req_id, info->ctxt,
                    PCFETCHER_RESP_TYPE_ERROR,
                    (const char *)&info->header, 0);
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

    if (m_is_async) {
        struct pcfetcher_callback_info *info = m_callback;
        m_runloop->dispatch([info] {
                    info->handler(info->session, info->req_id, info->ctxt,
                            PCFETCHER_RESP_TYPE_HEADER,
                            (const char *)&info->header, 0);
                }
            );

        if (m_callback->tracker) {
            m_runloop->dispatch([info, progress=m_progressValue] {
                    info->tracker(info->session, info->req_id, info->tracker_ctxt,
                            progress);
                }
            );
        }
    }
    else {
        m_callback->rws = purc_rwstream_new_buffer(init, INT_MAX);
    }
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

    if (m_is_async) {
        struct pcfetcher_callback_info *info = m_callback;
        size_t nr_bytes = data.size();
        char *buf = strndup(data.data(), nr_bytes);
        m_runloop->dispatch([info, progress=m_progressValue, buf, nr_bytes] {
                info->handler(info->session, info->req_id, info->ctxt,
                        PCFETCHER_RESP_TYPE_DATA,
                        (const char *)buf, nr_bytes);
                free(buf);
                }
            );

        if (m_callback->tracker) {
            struct pcfetcher_callback_info *info = m_callback;
            const char *bytes = data.data();
            size_t nr_bytes = data.size();
            m_runloop->dispatch([info, progress=m_progressValue, bytes, nr_bytes] {
                    info->tracker(info->session, info->req_id, info->tracker_ctxt,
                            progress);
                }
            );
        }
    }
    else {
        purc_rwstream_write(m_callback->rws, data.data(), data.size());
    }
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
        m_runloop->dispatch([info, progress=m_progressValue] {
                info->tracker(info->session, info->req_id, info->tracker_ctxt,
                        progress);
            }
        );
    }

    if (!m_callback->handler) {
        return;
    }

    struct pcfetcher_callback_info *info = m_callback;
    m_callback = NULL;
    m_runloop->dispatch([info, request=this] {
            info->handler(info->session, info->req_id, info->ctxt,
                    PCFETCHER_RESP_TYPE_FINISH,
                    NULL, 0);
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

    struct pcfetcher_callback_info *info = m_callback;
    m_callback = NULL;
    m_runloop->dispatch([info, request=this] {
            info->handler(info->session, info->req_id, info->ctxt,
                    PCFETCHER_RESP_TYPE_ERROR,
                    (const char *)&info->header, 0);
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

