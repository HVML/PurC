/*
 * Copyright (C) 2006, 2008, 2016 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "ResourceResponseBase.h"

#include "HTTPHeaderNames.h"
#include "ResourceResponse.h"
#include <wtf/MathExtras.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringView.h>

namespace PurCFetcher {


ResourceResponseBase::ResourceResponseBase()
    : m_haveParsedCacheControlHeader(false)
    , m_haveParsedAgeHeader(false)
    , m_haveParsedDateHeader(false)
    , m_haveParsedExpiresHeader(false)
    , m_haveParsedLastModifiedHeader(false)
    , m_haveParsedContentRangeHeader(false)
    , m_isRedirected(false)
    , m_isRangeRequested(false)
    , m_isNull(true)
    , m_usedLegacyTLS(UsedLegacyTLS::No)
    , m_tainting(Tainting::Basic)
    , m_source(Source::Unknown)
    , m_type(Type::Default)
{
}

ResourceResponseBase::ResourceResponseBase(const URL& url, const String& mimeType, long long expectedLength, const String& textEncodingName)
    : m_url(url)
    , m_mimeType(mimeType)
    , m_expectedContentLength(expectedLength)
    , m_textEncodingName(textEncodingName)
    , m_certificateInfo(CertificateInfo()) // Empty but valid for synthetic responses.
    , m_haveParsedCacheControlHeader(false)
    , m_haveParsedAgeHeader(false)
    , m_haveParsedDateHeader(false)
    , m_haveParsedExpiresHeader(false)
    , m_haveParsedLastModifiedHeader(false)
    , m_haveParsedContentRangeHeader(false)
    , m_isRedirected(false)
    , m_isRangeRequested(false)
    , m_isNull(false)
    , m_usedLegacyTLS(UsedLegacyTLS::No)
    , m_tainting(Tainting::Basic)
    , m_source(Source::Unknown)
    , m_type(Type::Default)
{
}

ResourceResponseBase::CrossThreadData ResourceResponseBase::crossThreadData() const
{
    CrossThreadData data;

    data.url = url().isolatedCopy();
    data.mimeType = mimeType().isolatedCopy();
    data.expectedContentLength = expectedContentLength();
    data.textEncodingName = textEncodingName().isolatedCopy();

    data.httpStatusCode = httpStatusCode();
    data.httpStatusText = httpStatusText().isolatedCopy();
    data.httpVersion = httpVersion().isolatedCopy();

    data.httpHeaderFields = httpHeaderFields().isolatedCopy();
    if (m_networkLoadMetrics)
        data.networkLoadMetrics = m_networkLoadMetrics->isolatedCopy();
    data.type = m_type;
    data.tainting = m_tainting;
    data.isRedirected = m_isRedirected;
    data.isRangeRequested = m_isRangeRequested;

    return data;
}

ResourceResponse ResourceResponseBase::fromCrossThreadData(CrossThreadData&& data)
{
    ResourceResponse response;

    response.setURL(data.url);
    response.setMimeType(data.mimeType);
    response.setExpectedContentLength(data.expectedContentLength);
    response.setTextEncodingName(data.textEncodingName);

    response.setHTTPStatusCode(data.httpStatusCode);
    response.setHTTPStatusText(data.httpStatusText);
    response.setHTTPVersion(data.httpVersion);

    response.m_httpHeaderFields = WTFMove(data.httpHeaderFields);
    if (data.networkLoadMetrics)
        response.m_networkLoadMetrics = Box<NetworkLoadMetrics>::create(WTFMove(data.networkLoadMetrics.value()));
    else
        response.m_networkLoadMetrics = nullptr;
    response.m_type = data.type;
    response.m_tainting = data.tainting;
    response.m_isRedirected = data.isRedirected;
    response.m_isRangeRequested = data.isRangeRequested;

    return response;
}

ResourceResponse ResourceResponseBase::syntheticRedirectResponse(const URL& fromURL, const URL& toURL)
{
    ResourceResponse redirectResponse;
    redirectResponse.setURL(fromURL);
    redirectResponse.setHTTPStatusCode(302);
    redirectResponse.setHTTPVersion("HTTP/1.1"_s);
    redirectResponse.setHTTPHeaderField(HTTPHeaderName::Location, toURL.string());
    redirectResponse.setHTTPHeaderField(HTTPHeaderName::CacheControl, "no-store"_s);

    return redirectResponse;
}

ResourceResponse ResourceResponseBase::filter(const ResourceResponse& response, PerformExposeAllHeadersCheck)
{
    if (response.tainting() == Tainting::Opaque) {
        ResourceResponse opaqueResponse;
        opaqueResponse.setTainting(Tainting::Opaque);
        opaqueResponse.setType(Type::Opaque);
        return opaqueResponse;
    }

    if (response.tainting() == Tainting::Opaqueredirect) {
        ResourceResponse opaqueResponse;
        opaqueResponse.setTainting(Tainting::Opaqueredirect);
        opaqueResponse.setType(Type::Opaqueredirect);
        opaqueResponse.setURL(response.url());
        return opaqueResponse;
    }


    return response;
}

bool ResourceResponseBase::isInHTTPFamily() const
{
    lazyInit(CommonFieldsOnly);

    return m_url.protocolIsInHTTPFamily();
}

const URL& ResourceResponseBase::url() const
{
    lazyInit(CommonFieldsOnly);

    return m_url;
}

void ResourceResponseBase::setURL(const URL& url)
{
    lazyInit(CommonFieldsOnly);
    m_isNull = false;

    m_url = url;

    // FIXME: Should invalidate or update platform response if present.
}

const String& ResourceResponseBase::mimeType() const
{
    lazyInit(CommonFieldsOnly);

    return m_mimeType; 
}

void ResourceResponseBase::setMimeType(const String& mimeType)
{
    lazyInit(CommonFieldsOnly);
    m_isNull = false;

    // FIXME: MIME type is determined by HTTP Content-Type header. We should update the header, so that it doesn't disagree with m_mimeType.
    m_mimeType = mimeType;

    // FIXME: Should invalidate or update platform response if present.
}

long long ResourceResponseBase::expectedContentLength() const 
{
    lazyInit(CommonFieldsOnly);

    return m_expectedContentLength;
}

void ResourceResponseBase::setExpectedContentLength(long long expectedContentLength)
{
    lazyInit(CommonFieldsOnly);
    m_isNull = false;

    // FIXME: Content length is determined by HTTP Content-Length header. We should update the header, so that it doesn't disagree with m_expectedContentLength.
    m_expectedContentLength = expectedContentLength; 

    // FIXME: Should invalidate or update platform response if present.
}

const String& ResourceResponseBase::textEncodingName() const
{
    lazyInit(CommonFieldsOnly);

    return m_textEncodingName;
}

void ResourceResponseBase::setTextEncodingName(const String& encodingName)
{
    lazyInit(CommonFieldsOnly);
    m_isNull = false;

    // FIXME: Text encoding is determined by HTTP Content-Type header. We should update the header, so that it doesn't disagree with m_textEncodingName.
    m_textEncodingName = encodingName;

    // FIXME: Should invalidate or update platform response if present.
}

void ResourceResponseBase::setType(Type type)
{
    m_isNull = false;
    m_type = type;
}

void ResourceResponseBase::includeCertificateInfo() const
{
}

String ResourceResponseBase::suggestedFilename() const
{
    return "";
}

String ResourceResponseBase::sanitizeSuggestedFilename(const String& suggestedFilename)
{
    if (suggestedFilename.isEmpty())
        return suggestedFilename;

    ResourceResponse response(URL({ }, "http://example.com/"), String(), -1, String());
    response.setHTTPStatusCode(200);
    String escapedSuggestedFilename = String(suggestedFilename).replace('\\', "\\\\").replace('"', "\\\"");
    String value = makeString("attachment; filename=\"", escapedSuggestedFilename, '"');
    response.setHTTPHeaderField(HTTPHeaderName::ContentDisposition, value);
    return response.suggestedFilename();
}

bool ResourceResponseBase::isSuccessful() const
{
    int code = httpStatusCode();
    return code >= 200 && code < 300;
}

int ResourceResponseBase::httpStatusCode() const
{
    lazyInit(CommonFieldsOnly);

    return m_httpStatusCode;
}

void ResourceResponseBase::setHTTPStatusCode(int statusCode)
{
    lazyInit(CommonFieldsOnly);

    m_httpStatusCode = statusCode;
    m_isNull = false;

    // FIXME: Should invalidate or update platform response if present.
}

bool ResourceResponseBase::isRedirection() const
{
    return isRedirectionStatusCode(m_httpStatusCode);
}

const String& ResourceResponseBase::httpStatusText() const 
{
    lazyInit(AllFields);

    return m_httpStatusText; 
}

void ResourceResponseBase::setHTTPStatusText(const String& statusText) 
{
    lazyInit(AllFields);

    m_httpStatusText = statusText; 

    // FIXME: Should invalidate or update platform response if present.
}

const String& ResourceResponseBase::httpVersion() const
{
    lazyInit(AllFields);
    
    return m_httpVersion;
}

void ResourceResponseBase::setHTTPVersion(const String& versionText)
{
    lazyInit(AllFields);
    
    m_httpVersion = versionText;
    
    // FIXME: Should invalidate or update platform response if present.
}

static bool isSafeRedirectionResponseHeader(HTTPHeaderName name)
{
    // PurCFetcher needs to keep location and cache related headers as it does caching.
    // We also keep CORS/ReferrerPolicy headers until CORS checks/Referrer computation are done in NetworkProcess.
    return name == HTTPHeaderName::Location
        || name == HTTPHeaderName::ReferrerPolicy
        || name == HTTPHeaderName::CacheControl
        || name == HTTPHeaderName::Date
        || name == HTTPHeaderName::Expires
        || name == HTTPHeaderName::ETag
        || name == HTTPHeaderName::LastModified
        || name == HTTPHeaderName::Age
        || name == HTTPHeaderName::Pragma
        || name == HTTPHeaderName::ReferrerPolicy
        || name == HTTPHeaderName::Refresh
        || name == HTTPHeaderName::Vary
        || name == HTTPHeaderName::AccessControlAllowCredentials
        || name == HTTPHeaderName::AccessControlAllowHeaders
        || name == HTTPHeaderName::AccessControlAllowMethods
        || name == HTTPHeaderName::AccessControlAllowOrigin
        || name == HTTPHeaderName::AccessControlExposeHeaders
        || name == HTTPHeaderName::AccessControlMaxAge
        || name == HTTPHeaderName::CrossOriginResourcePolicy
        || name == HTTPHeaderName::TimingAllowOrigin;
}

void ResourceResponseBase::sanitizeHTTPHeaderFieldsAccordingToTainting()
{
}

void ResourceResponseBase::sanitizeHTTPHeaderFields(SanitizationType type)
{
    lazyInit(AllFields);

    m_httpHeaderFields.remove(HTTPHeaderName::SetCookie);
    m_httpHeaderFields.remove(HTTPHeaderName::SetCookie2);

    switch (type) {
    case SanitizationType::RemoveCookies:
        return;
    case SanitizationType::Redirection: {
        auto commonHeaders = WTFMove(m_httpHeaderFields.commonHeaders());
        for (auto& header : commonHeaders) {
            if (isSafeRedirectionResponseHeader(header.key))
                m_httpHeaderFields.add(header.key, WTFMove(header.value));
        }
        m_httpHeaderFields.uncommonHeaders().clear();
        return;
    }
    case SanitizationType::CrossOriginSafe:
        sanitizeHTTPHeaderFieldsAccordingToTainting();
    }
}

bool ResourceResponseBase::isHTTP09() const
{
    lazyInit(AllFields);

    return m_httpVersion.startsWith("HTTP/0.9");
}

String ResourceResponseBase::httpHeaderField(const String& name) const
{
    lazyInit(CommonFieldsOnly);

    // If we already have the header, just return it instead of consuming memory by grabing all headers.
    String value = m_httpHeaderFields.get(name);
    if (!value.isEmpty())        
        return value;

    lazyInit(AllFields);

    return m_httpHeaderFields.get(name); 
}

String ResourceResponseBase::httpHeaderField(HTTPHeaderName name) const
{
    lazyInit(CommonFieldsOnly);

    // If we already have the header, just return it instead of consuming memory by grabing all headers.
    String value = m_httpHeaderFields.get(name);
    if (!value.isEmpty())
        return value;

    lazyInit(AllFields);

    return m_httpHeaderFields.get(name); 
}

void ResourceResponseBase::updateHeaderParsedState(HTTPHeaderName name)
{
    switch (name) {
    case HTTPHeaderName::Age:
        m_haveParsedAgeHeader = false;
        break;

    case HTTPHeaderName::CacheControl:
    case HTTPHeaderName::Pragma:
        m_haveParsedCacheControlHeader = false;
        break;

    case HTTPHeaderName::Date:
        m_haveParsedDateHeader = false;
        break;

    case HTTPHeaderName::Expires:
        m_haveParsedExpiresHeader = false;
        break;

    case HTTPHeaderName::LastModified:
        m_haveParsedLastModifiedHeader = false;
        break;

    case HTTPHeaderName::ContentRange:
        m_haveParsedContentRangeHeader = false;
        break;

    default:
        break;
    }
}

void ResourceResponseBase::setHTTPHeaderField(const String& name, const String& value)
{
    lazyInit(AllFields);

    HTTPHeaderName headerName;
    if (findHTTPHeaderName(name, headerName))
        updateHeaderParsedState(headerName);

    m_httpHeaderFields.set(name, value);

    // FIXME: Should invalidate or update platform response if present.
}

void ResourceResponseBase::setHTTPHeaderFields(HTTPHeaderMap&& headerFields)
{
    lazyInit(AllFields);

    m_httpHeaderFields = WTFMove(headerFields);
}

void ResourceResponseBase::setHTTPHeaderField(HTTPHeaderName name, const String& value)
{
    lazyInit(AllFields);

    updateHeaderParsedState(name);

    m_httpHeaderFields.set(name, value);

    // FIXME: Should invalidate or update platform response if present.
}

void ResourceResponseBase::addHTTPHeaderField(HTTPHeaderName name, const String& value)
{
    lazyInit(AllFields);
    updateHeaderParsedState(name);
    m_httpHeaderFields.add(name, value);
}

void ResourceResponseBase::addHTTPHeaderField(const String& name, const String& value)
{
    HTTPHeaderName headerName;
    if (findHTTPHeaderName(name, headerName))
        addHTTPHeaderField(headerName, value);
    else {
        lazyInit(AllFields);
        m_httpHeaderFields.add(name, value);
    }
}

const HTTPHeaderMap& ResourceResponseBase::httpHeaderFields() const
{
    lazyInit(AllFields);

    return m_httpHeaderFields;
}

void ResourceResponseBase::parseCacheControlDirectives() const
{
}
    
bool ResourceResponseBase::cacheControlContainsNoCache() const
{
    if (!m_haveParsedCacheControlHeader)
        parseCacheControlDirectives();
    return m_cacheControlDirectives.noCache;
}

bool ResourceResponseBase::cacheControlContainsNoStore() const
{
    if (!m_haveParsedCacheControlHeader)
        parseCacheControlDirectives();
    return m_cacheControlDirectives.noStore;
}

bool ResourceResponseBase::cacheControlContainsMustRevalidate() const
{
    if (!m_haveParsedCacheControlHeader)
        parseCacheControlDirectives();
    return m_cacheControlDirectives.mustRevalidate;
}
    
bool ResourceResponseBase::cacheControlContainsImmutable() const
{
    if (!m_haveParsedCacheControlHeader)
        parseCacheControlDirectives();
    return m_cacheControlDirectives.immutable;
}

bool ResourceResponseBase::hasCacheValidatorFields() const
{
    lazyInit(CommonFieldsOnly);

    return !m_httpHeaderFields.get(HTTPHeaderName::LastModified).isEmpty() || !m_httpHeaderFields.get(HTTPHeaderName::ETag).isEmpty();
}

std::optional<Seconds> ResourceResponseBase::cacheControlMaxAge() const
{
    if (!m_haveParsedCacheControlHeader)
        parseCacheControlDirectives();
    return m_cacheControlDirectives.maxAge;
}

std::optional<Seconds> ResourceResponseBase::cacheControlStaleWhileRevalidate() const
{
    if (!m_haveParsedCacheControlHeader)
        parseCacheControlDirectives();
    return m_cacheControlDirectives.staleWhileRevalidate;
}

static std::optional<WallTime> parseDateValueInHeader(const HTTPHeaderMap& headers, HTTPHeaderName headerName)
{
    String headerValue = headers.get(headerName);
    if (headerValue.isEmpty())
        return std::nullopt;
    return std::nullopt;
}

std::optional<WallTime> ResourceResponseBase::date() const
{
    lazyInit(CommonFieldsOnly);

    if (!m_haveParsedDateHeader) {
        m_date = parseDateValueInHeader(m_httpHeaderFields, HTTPHeaderName::Date);
        m_haveParsedDateHeader = true;
    }
    return m_date;
}

std::optional<Seconds> ResourceResponseBase::age() const
{
    lazyInit(CommonFieldsOnly);

    if (!m_haveParsedAgeHeader) {
        String headerValue = m_httpHeaderFields.get(HTTPHeaderName::Age);
        bool ok;
        double ageDouble = headerValue.toDouble(&ok);
        if (ok)
            m_age = Seconds { ageDouble };
        m_haveParsedAgeHeader = true;
    }
    return m_age;
}

std::optional<WallTime> ResourceResponseBase::expires() const
{
    lazyInit(CommonFieldsOnly);

    if (!m_haveParsedExpiresHeader) {
        m_expires = parseDateValueInHeader(m_httpHeaderFields, HTTPHeaderName::Expires);
        m_haveParsedExpiresHeader = true;
    }
    return m_expires;
}

std::optional<WallTime> ResourceResponseBase::lastModified() const
{
    lazyInit(CommonFieldsOnly);

    if (!m_haveParsedLastModifiedHeader) {
        m_lastModified = parseDateValueInHeader(m_httpHeaderFields, HTTPHeaderName::LastModified);
        m_haveParsedLastModifiedHeader = true;
    }
    return m_lastModified;
}

bool ResourceResponseBase::isAttachment() const
{
    lazyInit(AllFields);

    auto value = m_httpHeaderFields.get(HTTPHeaderName::ContentDisposition);
    return equalLettersIgnoringASCIICase(value.left(value.find(';')).stripWhiteSpace(), "attachment");
}

bool ResourceResponseBase::isAttachmentWithFilename() const
{
    return false;
}

ResourceResponseBase::Source ResourceResponseBase::source() const
{
    lazyInit(AllFields);

    return m_source;
}

void ResourceResponseBase::lazyInit(InitLevel initLevel) const
{
    const_cast<ResourceResponse*>(static_cast<const ResourceResponse*>(this))->platformLazyInit(initLevel);
}

bool ResourceResponseBase::compare(const ResourceResponse& a, const ResourceResponse& b)
{
    if (a.isNull() != b.isNull())
        return false;  
    if (a.url() != b.url())
        return false;
    if (a.mimeType() != b.mimeType())
        return false;
    if (a.expectedContentLength() != b.expectedContentLength())
        return false;
    if (a.textEncodingName() != b.textEncodingName())
        return false;
    if (a.suggestedFilename() != b.suggestedFilename())
        return false;
    if (a.httpStatusCode() != b.httpStatusCode())
        return false;
    if (a.httpStatusText() != b.httpStatusText())
        return false;
    if (a.httpHeaderFields() != b.httpHeaderFields())
        return false;
    if (a.m_networkLoadMetrics.get() != b.m_networkLoadMetrics.get()) {
        if (!a.m_networkLoadMetrics) {
            if (NetworkLoadMetrics() != *b.m_networkLoadMetrics.get())
                return false;
        } else if (!b.m_networkLoadMetrics) {
            if (NetworkLoadMetrics() != *a.m_networkLoadMetrics.get())
                return false;
        } else if (*a.m_networkLoadMetrics.get() != *b.m_networkLoadMetrics.get())
            return false;
    }
    return ResourceResponse::platformCompare(a, b);
}

bool ResourceResponseBase::containsInvalidHTTPHeaders() const
{
    return false;
}

}
