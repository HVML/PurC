/*
 * Copyright (C) 2009 Gustavo Noronha Silva
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if USE(SOUP)
#include "ResourceRequest.h"

#include "GUniquePtrSoup.h"
#include "HTTPParsers.h"
#include "MIMETypeRegistry.h"
#include "RegistrableDomain.h"
#include "SharedBuffer.h"
#include "URLSoup.h"
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace PurCFetcher {

void ResourceRequest::updateSoupMessageBody(SoupMessage* soupMessage) const
{
    auto* formData = httpBody();
    if (!formData || formData->isEmpty())
        return;

#if USE(SOUP2)
    soup_message_body_set_accumulate(soupMessage->request_body, FALSE);
    uint64_t bodySize = 0;
    for (const auto& element : formData->elements()) {
        switchOn(element.data,
            [&] (const Vector<uint8_t>& bytes) {
                bodySize += bytes.size();
                soup_message_body_append(soupMessage->request_body, SOUP_MEMORY_TEMPORARY, bytes.data(), bytes.size());
            }, [&] (const FormDataElement::EncodedFileData& fileData) {
                if (auto buffer = SharedBuffer::createWithContentsOfFile(fileData.filename)) {
                    if (buffer->isEmpty())
                        return;

                    GUniquePtr<SoupBuffer> soupBuffer(soup_buffer_new_with_owner(buffer->data(), buffer->size(), buffer.get(), [](void* data) {
                                static_cast<SharedBuffer*>(data)->deref();
                                }));

                    bodySize += buffer->size();
                    if (soupBuffer->length)
                        soup_message_body_append_buffer(soupMessage->request_body, soupBuffer.get());
                }
            }, [&] (const FormDataElement::EncodedBlobData& blob) {
                (void) blob;
            }
        );
    }

    ASSERT(bodySize == static_cast<uint64_t>(soupMessage->request_body->length));
#else
    uint64_t bodySize = 0;
    GRefPtr<GInputStream> stream = adoptGRef(g_memory_input_stream_new());
    for (const auto& element : formData->elements()) {
        switchOn(element.data,
            [&] (const Vector<uint8_t>& bytes) {
                g_memory_input_stream_add_data(G_MEMORY_INPUT_STREAM(stream.get()), bytes.data(), bytes.size(), NULL);
                bodySize += bytes.size();
            }, [&] (const FormDataElement::EncodedFileData& fileData) {
                if (auto buffer = SharedBuffer::createWithContentsOfFile(fileData.filename)) {
                    if (buffer->isEmpty())
                        return;

                    g_memory_input_stream_add_data(G_MEMORY_INPUT_STREAM(stream.get()), buffer->data(), buffer->size(), NULL);
                    bodySize += buffer->size();
                }
            }, [&] (const FormDataElement::EncodedBlobData& blob) {
                (void) blob;
            }
        );
    }
    soup_message_set_request_body(soupMessage, nullptr, stream.get(), bodySize);
#endif
}

void ResourceRequest::updateSoupMessageMembers(SoupMessage* soupMessage) const
{
#if USE(SOUP2)
    updateSoupMessageHeaders(soupMessage->request_headers);
#else
    updateSoupMessageHeaders(soup_message_get_request_headers(soupMessage));
#endif

    auto firstParty = urlToSoupURI(firstPartyForCookies());
    if (firstParty)
        soup_message_set_first_party(soupMessage, firstParty.get());

#if SOUP_CHECK_VERSION(2, 69, 90)
    if (!isSameSiteUnspecified()) {
        if (isSameSite()) {
            auto siteForCookies = urlToSoupURI(m_url);
            soup_message_set_site_for_cookies(soupMessage, siteForCookies.get());
        }
        soup_message_set_is_top_level_navigation(soupMessage, isTopSite());
    }
#endif

    soup_message_set_flags(soupMessage, m_soupFlags);

    if (!acceptEncoding())
        soup_message_disable_feature(soupMessage, SOUP_TYPE_CONTENT_DECODER);
    if (!allowCookies())
        soup_message_disable_feature(soupMessage, SOUP_TYPE_COOKIE_JAR);
}

void ResourceRequest::updateSoupMessageHeaders(SoupMessageHeaders* soupHeaders) const
{
    const HTTPHeaderMap& headers = httpHeaderFields();
    if (!headers.isEmpty()) {
        HTTPHeaderMap::const_iterator end = headers.end();
        for (HTTPHeaderMap::const_iterator it = headers.begin(); it != end; ++it)
            soup_message_headers_append(soupHeaders, it->key.utf8().data(), it->value.utf8().data());
    }
}

void ResourceRequest::updateFromSoupMessageHeaders(SoupMessageHeaders* soupHeaders)
{
    m_httpHeaderFields.clear();
    SoupMessageHeadersIter headersIter;
    soup_message_headers_iter_init(&headersIter, soupHeaders);
    const char* headerName;
    const char* headerValue;
    while (soup_message_headers_iter_next(&headersIter, &headerName, &headerValue))
        m_httpHeaderFields.set(String(headerName), String(headerValue));
}

void ResourceRequest::updateSoupMessage(SoupMessage* soupMessage) const
{
    g_object_set(soupMessage, "method", httpMethod().ascii().data(), NULL);

    auto uri = createSoupURI();
    soup_message_set_uri(soupMessage, uri.get());

    updateSoupMessageMembers(soupMessage);
    updateSoupMessageBody(soupMessage);
}

void ResourceRequest::updateFromSoupMessage(SoupMessage* soupMessage)
{
    bool shouldPortBeResetToZero = m_url.port() && !m_url.port().value();
    m_url = soupURIToURL(soup_message_get_uri(soupMessage));

    // SoupURI cannot differeniate between an explicitly specified port 0 and
    // no port specified.
    if (shouldPortBeResetToZero)
        m_url.setPort(0);

#if USE(SOUP2)
    m_httpMethod = String(soupMessage->method);
#else
    m_httpMethod = String(soup_message_get_method(soupMessage));
#endif

#if USE(SOUP2)
    updateFromSoupMessageHeaders(soupMessage->request_headers);
#else
    updateFromSoupMessageHeaders(soup_message_get_request_headers(soupMessage));
#endif

#if USE(SOUP2)
    // FIXME: by xue
    if (soupMessage->request_body->data)
        m_httpBody = FormData::create(soupMessage->request_body->data, soupMessage->request_body->length);
#endif

    if (auto firstParty = soup_message_get_first_party(soupMessage))
        m_firstPartyForCookies = soupURIToURL(firstParty);

#if SOUP_CHECK_VERSION(2, 69, 90)
    setIsTopSite(soup_message_get_is_top_level_navigation(soupMessage));

    if (auto siteForCookies = soup_message_get_site_for_cookies(soupMessage))
        setIsSameSite(areRegistrableDomainsEqual(soupURIToURL(siteForCookies), m_url));
    else
        m_sameSiteDisposition = SameSiteDisposition::Unspecified;
#else
    m_sameSiteDisposition = SameSiteDisposition::Unspecified;
#endif

    m_soupFlags = soup_message_get_flags(soupMessage);

#if SOUP_CHECK_VERSION(2, 71, 0)
    m_acceptEncoding = !soup_message_is_feature_disabled(soupMessage, SOUP_TYPE_CONTENT_DECODER);
    m_allowCookies = !soup_message_is_feature_disabled(soupMessage, SOUP_TYPE_COOKIE_JAR);
#endif
}

unsigned initializeMaximumHTTPConnectionCountPerHost()
{
    // Soup has its own queue control; it wants to have all requests
    // given to it, so that it is able to look ahead, and schedule
    // them in a good way.
    return 10000;
}

#if USE(SOUP2)
GUniquePtr<SoupURI> ResourceRequest::createSoupURI() const
#else
GRefPtr<GUri> ResourceRequest::createSoupURI() const
#endif
{
    // WebKit does not support fragment identifiers in data URLs, but soup does.
    // Before passing the URL to soup, we should make sure to urlencode any '#'
    // characters, so that soup does not interpret them as fragment identifiers.
    // See http://wkbug.com/68089
    if (m_url.protocolIsData()) {
        String urlString = m_url.string();
        urlString.replace("#", "%23");
        return urlToSoupURI(URL(URL(), urlString));
    }

    auto soupURI = urlToSoupURI(m_url);

#if USE(SOUP2)
    // Versions of libsoup prior to 2.42 have a soup_uri_new that will convert empty passwords that are not
    // prefixed by a colon into null. Some parts of soup like the SoupAuthenticationManager will only be active
    // when both the username and password are non-null. When we have credentials, empty usernames and passwords
    // should be empty strings instead of null.
    String urlUser = m_url.user();
    String urlPass = m_url.password();
    if (!urlUser.isEmpty() || !urlPass.isEmpty()) {
        soup_uri_set_user(soupURI.get(), urlUser.utf8().data());
        soup_uri_set_password(soupURI.get(), urlPass.utf8().data());
    }
#endif

    return soupURI;
}

}

#endif
