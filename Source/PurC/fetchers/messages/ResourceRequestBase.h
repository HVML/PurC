/*
 * Copyright (C) 2003, 2006 Apple Inc.  All rights reserved.
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
 * Copyright (C) 2009, 2012 Google Inc. All rights reserved.
 * Copyright (C) 2019 ~ 2020, Beijing FMSoft Technologies Co., Ltd.
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

#pragma once

#include "fetcher-messages-basic.h"
#include "FormData.h"
#include "HTTPHeaderMap.h"
#include <wtf/EnumTraits.h>
#include <wtf/URL.h>

namespace PurCFetcher {

enum class ResourceRequestCachePolicy : uint8_t {
    UseProtocolCachePolicy, // normal load, equivalent to fetch "default" cache mode.
    ReloadIgnoringCacheData, // reload, equivalent to fetch "reload"cache mode.
    ReturnCacheDataElseLoad, // back/forward or encoding change - allow stale data, equivalent to fetch "force-cache" cache mode.
    ReturnCacheDataDontLoad, // results of a post - allow stale data and only use cache, equivalent to fetch "only-if-cached" cache mode.
    DoNotUseAnyCache, // Bypass the cache entirely, equivalent to fetch "no-store" cache mode.
    RefreshAnyCacheData, // Serve cache data only if revalidated, equivalent to fetch "no-cache" mode.
};

enum HTTPBodyUpdatePolicy : uint8_t {
    DoNotUpdateHTTPBody,
    UpdateHTTPBody
};

class ResourceRequest;
class ResourceResponse;

// Do not use this type directly.  Use ResourceRequest instead.
class ResourceRequestBase {
    WTF_MAKE_FAST_ALLOCATED;
public:
    PURC_EXPORT ResourceRequest isolatedCopy() const;
    PURC_EXPORT void setAsIsolatedCopy(const ResourceRequest&);

    PURC_EXPORT bool isNull() const;
    PURC_EXPORT bool isEmpty() const;

    PURC_EXPORT const URL& url() const;
    PURC_EXPORT void setURL(const URL& url);

    void redirectAsGETIfNeeded(const ResourceRequestBase &, const ResourceResponse&);
    PURC_EXPORT ResourceRequest redirectedRequest(const ResourceResponse&, bool shouldClearReferrerOnHTTPSToHTTPRedirect) const;

    PURC_EXPORT void removeCredentials();

    PURC_EXPORT ResourceRequestCachePolicy cachePolicy() const;
    PURC_EXPORT void setCachePolicy(ResourceRequestCachePolicy cachePolicy);

    PURC_EXPORT double timeoutInterval() const; // May return 0 when using platform default.
    void setTimeoutInterval(double timeoutInterval);

    PURC_EXPORT const URL& firstPartyForCookies() const;
    PURC_EXPORT void setFirstPartyForCookies(const URL&);

    // Same-Site cookies; see <https://tools.ietf.org/html/draft-ietf-httpbis-cookie-same-site-00#section-2.1>
    // and <https://tools.ietf.org/html/draft-ietf-httpbis-cookie-same-site-00#section-5.2>.
    // FIXME: For some reason the main resource request may be updated more than once. We start off as Unspecified
    // to detect if we need to compute the same-site and top-site state or not. See FIXME in DocumentLoader::startLoadingMainResource().
    enum class SameSiteDisposition : uint8_t { Unspecified, SameSite, CrossSite };
    bool isSameSiteUnspecified() const { return m_sameSiteDisposition == SameSiteDisposition::Unspecified; }
    PURC_EXPORT bool isSameSite() const; // Whether this request's registrable domain matches the request's initiator's "site for cookies".
    PURC_EXPORT void setIsSameSite(bool);
    PURC_EXPORT bool isTopSite() const; // Whether this request is for a top-level navigation.
    PURC_EXPORT void setIsTopSite(bool);

    PURC_EXPORT const String& httpMethod() const;
    PURC_EXPORT void setHTTPMethod(const String& httpMethod);

    PURC_EXPORT const HTTPHeaderMap& httpHeaderFields() const;
    PURC_EXPORT void setHTTPHeaderFields(HTTPHeaderMap);

    PURC_EXPORT String httpHeaderField(const String& name) const;
    PURC_EXPORT String httpHeaderField(HTTPHeaderName) const;
    PURC_EXPORT void setHTTPHeaderField(const String& name, const String& value);
    PURC_EXPORT void setHTTPHeaderField(HTTPHeaderName, const String& value);
    PURC_EXPORT void addHTTPHeaderField(HTTPHeaderName, const String& value);
    PURC_EXPORT void addHTTPHeaderField(const String& name, const String& value);
    PURC_EXPORT void addHTTPHeaderFieldIfNotPresent(HTTPHeaderName, const String&);

    PURC_EXPORT bool hasHTTPHeaderField(HTTPHeaderName) const;

    // Instead of passing a string literal to any of these functions, just use a HTTPHeaderName instead.
    template<size_t length> String httpHeaderField(const char (&)[length]) const = delete;
    template<size_t length> void setHTTPHeaderField(const char (&)[length], const String&) = delete;
    template<size_t length> void addHTTPHeaderField(const char (&)[length], const String&) = delete;

    PURC_EXPORT void clearHTTPAuthorization();

    PURC_EXPORT String httpContentType() const;
    PURC_EXPORT void setHTTPContentType(const String&);
    PURC_EXPORT void clearHTTPContentType();

    bool hasHTTPHeader(HTTPHeaderName) const;

    PURC_EXPORT String httpReferrer() const;
    bool hasHTTPReferrer() const;
    PURC_EXPORT void setHTTPReferrer(const String&);
    PURC_EXPORT void setExistingHTTPReferrerToOriginString();
    PURC_EXPORT void clearHTTPReferrer();

    PURC_EXPORT String httpOrigin() const;
    bool hasHTTPOrigin() const;
    void setHTTPOrigin(const String&);
    PURC_EXPORT void clearHTTPOrigin();

    PURC_EXPORT String httpUserAgent() const;
    PURC_EXPORT void setHTTPUserAgent(const String&);
    void clearHTTPUserAgent();

    void clearHTTPAcceptEncoding();

    PURC_EXPORT void clearPurpose();

    const Vector<String>& responseContentDispositionEncodingFallbackArray() const { return m_responseContentDispositionEncodingFallbackArray; }
    PURC_EXPORT void setResponseContentDispositionEncodingFallbackArray(const String& encoding1, const String& encoding2 = String(), const String& encoding3 = String());

    PURC_EXPORT FormData* httpBody() const;
    PURC_EXPORT bool hasUpload() const;
    PURC_EXPORT void setHTTPBody(RefPtr<FormData>&&);

    bool allowCookies() const;
    void setAllowCookies(bool allowCookies);

    PURC_EXPORT ResourceLoadPriority priority() const;
    PURC_EXPORT void setPriority(ResourceLoadPriority);

    PURC_EXPORT static String partitionName(const String& domain);
    const String& cachePartition() const { return m_cachePartition; }
    PURC_EXPORT void setCachePartition(const String&);
    void setDomainForCachePartition(const String& domain) { setCachePartition(partitionName(domain)); }

    PURC_EXPORT bool isConditional() const;
    PURC_EXPORT void makeUnconditional();

    // Whether this request should be hidden from the Inspector.
    bool hiddenFromInspector() const { return m_hiddenFromInspector; }
    void setHiddenFromInspector(bool hiddenFromInspector) { m_hiddenFromInspector = hiddenFromInspector; }

    enum class Requester : uint8_t { Unspecified, Main, XHR, Fetch, Media, ImportScripts, Ping, Beacon };
    Requester requester() const { return m_requester; }
    void setRequester(Requester requester) { m_requester = requester; }

    // Who initiated the request so the Inspector can associate it with a context. E.g. a Web Worker.
    String initiatorIdentifier() const { return m_initiatorIdentifier; }
    void setInitiatorIdentifier(const String& identifier) { m_initiatorIdentifier = identifier; }

    // Additional information for the Inspector to be able to identify the node that initiated this request.
    const std::optional<int>& inspectorInitiatorNodeIdentifier() const { return m_inspectorInitiatorNodeIdentifier; }
    void setInspectorInitiatorNodeIdentifier(int inspectorInitiatorNodeIdentifier) { m_inspectorInitiatorNodeIdentifier = inspectorInitiatorNodeIdentifier; }

    bool encodingRequiresPlatformData() const { return true; }

    template<class Encoder> void encodeWithoutPlatformData(Encoder&) const;
    template<class Decoder> WARN_UNUSED_RETURN bool decodeWithoutPlatformData(Decoder&);

    PURC_EXPORT static double defaultTimeoutInterval(); // May return 0 when using platform default.
    PURC_EXPORT static void setDefaultTimeoutInterval(double);

    PURC_EXPORT static bool equal(const ResourceRequest&, const ResourceRequest&);

    // gengyue
    PURC_EXPORT void setJsonType(bool json) {m_isJson = json;}
    PURC_EXPORT bool getJsonType(void) const {return m_isJson; }

protected:
    // Used when ResourceRequest is initialized from a platform representation of the request
    ResourceRequestBase()
        : m_allowCookies(false)
        , m_resourceRequestUpdated(false)
        , m_platformRequestUpdated(true)
        , m_resourceRequestBodyUpdated(false)
        , m_platformRequestBodyUpdated(true)
        , m_hiddenFromInspector(false)
        , m_isTopSite(false)
    {
    }

    ResourceRequestBase(const URL& url, ResourceRequestCachePolicy policy)
        : m_url(url)
        , m_timeoutInterval(s_defaultTimeoutInterval)
        , m_httpMethod("GET"_s)
        , m_cachePolicy(policy)
        , m_allowCookies(true)
        , m_resourceRequestUpdated(true)
        , m_platformRequestUpdated(false)
        , m_resourceRequestBodyUpdated(true)
        , m_platformRequestBodyUpdated(false)
        , m_hiddenFromInspector(false)
        , m_isTopSite(false)
    {
    }

    void updatePlatformRequest(HTTPBodyUpdatePolicy = HTTPBodyUpdatePolicy::DoNotUpdateHTTPBody) const;
    void updateResourceRequest(HTTPBodyUpdatePolicy = HTTPBodyUpdatePolicy::DoNotUpdateHTTPBody) const;

    template<class Encoder> void encodeBase(Encoder&) const;
    template<class Decoder> WARN_UNUSED_RETURN bool decodeBase(Decoder&);

    // The ResourceRequest subclass may "shadow" this method to compare platform specific fields
    static bool platformCompare(const ResourceRequest&, const ResourceRequest&) { return true; }

    URL m_url;
    double m_timeoutInterval; // 0 is a magic value for platform default on platforms that have one.
    URL m_firstPartyForCookies;
    String m_httpMethod;
    String m_initiatorIdentifier;
    String m_cachePartition { emptyString() };
    HTTPHeaderMap m_httpHeaderFields;
    Vector<String> m_responseContentDispositionEncodingFallbackArray;
    RefPtr<FormData> m_httpBody;
    ResourceRequestCachePolicy m_cachePolicy { ResourceRequestCachePolicy::UseProtocolCachePolicy };
    SameSiteDisposition m_sameSiteDisposition { SameSiteDisposition::Unspecified };
    ResourceLoadPriority m_priority { ResourceLoadPriority::Low };
    Requester m_requester { Requester::Unspecified };
    std::optional<int> m_inspectorInitiatorNodeIdentifier;
    bool m_allowCookies : 1;
    mutable bool m_resourceRequestUpdated : 1;
    mutable bool m_platformRequestUpdated : 1;
    mutable bool m_resourceRequestBodyUpdated : 1;
    mutable bool m_platformRequestBodyUpdated : 1;
    bool m_hiddenFromInspector : 1;
    bool m_isTopSite : 1;

    // gengyue
    bool m_isJson {false};

private:
    const ResourceRequest& asResourceRequest() const;

    PURC_EXPORT static double s_defaultTimeoutInterval;
};

bool equalIgnoringHeaderFields(const ResourceRequestBase&, const ResourceRequestBase&);

inline bool operator==(const ResourceRequest& a, const ResourceRequest& b) { return ResourceRequestBase::equal(a, b); }
inline bool operator!=(ResourceRequest& a, const ResourceRequest& b) { return !(a == b); }

template<class Encoder>
ALWAYS_INLINE void ResourceRequestBase::encodeBase(Encoder& encoder) const
{
    encoder << m_url;
    encoder << m_timeoutInterval;
    encoder << m_firstPartyForCookies.string();
    encoder << m_httpMethod;
    encoder << m_httpHeaderFields;
    encoder << m_responseContentDispositionEncodingFallbackArray;
    encoder << m_cachePolicy;
    encoder << m_allowCookies;
    encoder << m_sameSiteDisposition;
    encoder << m_isTopSite;
    encoder << m_priority;
    encoder << m_requester;

    // gengyue
    encoder << m_isJson;
}

template<class Decoder>
ALWAYS_INLINE bool ResourceRequestBase::decodeBase(Decoder& decoder)
{
    std::optional<URL> url;
    decoder >> url;
    if (!url)
        return false;
    m_url = WTFMove(*url);

    std::optional<double> timeoutInterval;
    decoder >> timeoutInterval;
    if (!timeoutInterval)
        return false;
    m_timeoutInterval = WTFMove(*timeoutInterval);

    std::optional<String> firstPartyForCookies;
    decoder >> firstPartyForCookies;
    if (!firstPartyForCookies)
        return false;
    m_firstPartyForCookies = URL({ }, *firstPartyForCookies);

    std::optional<String> httpMethod;
    decoder >> httpMethod;
    if (!httpMethod)
        return false;
    m_httpMethod = WTFMove(*httpMethod);

    std::optional<HTTPHeaderMap> fields;
    decoder >> fields;
    if (!fields)
        return false;
    m_httpHeaderFields = WTFMove(*fields);

    std::optional<Vector<String>> array;
    decoder >> array;
    if (!array)
        return false;
    m_responseContentDispositionEncodingFallbackArray = WTFMove(*array);

    std::optional<ResourceRequestCachePolicy> cachePolicy;
    decoder >> cachePolicy;
    if (!cachePolicy)
        return false;
    m_cachePolicy = *cachePolicy;

    std::optional<bool> allowCookies;
    decoder >> allowCookies;
    if (!allowCookies)
        return false;
    m_allowCookies = *allowCookies;

    std::optional<SameSiteDisposition> sameSiteDisposition;
    decoder >> sameSiteDisposition;
    if (!sameSiteDisposition)
        return false;
    m_sameSiteDisposition = *sameSiteDisposition;

    std::optional<bool> isTopSite;
    decoder >> isTopSite;
    if (!isTopSite)
        return false;
    m_isTopSite = *isTopSite;

    std::optional<ResourceLoadPriority> priority;
    decoder >> priority;
    if (!priority)
        return false;
    m_priority = *priority;

    std::optional<Requester> requester;
    decoder >> requester;
    if (!requester)
        return false;
    m_requester = *requester;

    // gengyue
    std::optional<bool> isJson;
    decoder >> isJson;
    if (!isJson)
        return false;
    m_isJson = *isJson;

    return true;
}

template<class Encoder>
void ResourceRequestBase::encodeWithoutPlatformData(Encoder& encoder) const
{
    ASSERT(!m_httpBody);
    ASSERT(!m_platformRequestUpdated);
    encodeBase(encoder);
}

template<class Decoder>
bool ResourceRequestBase::decodeWithoutPlatformData(Decoder& decoder)
{
    return decodeBase(decoder);
}

} // namespace PurCFetcher

namespace PurCWTF {

template<> struct EnumTraits<PurCFetcher::ResourceRequestCachePolicy> {
    using values = EnumValues<
        PurCFetcher::ResourceRequestCachePolicy,
        PurCFetcher::ResourceRequestCachePolicy::UseProtocolCachePolicy,
        PurCFetcher::ResourceRequestCachePolicy::ReloadIgnoringCacheData,
        PurCFetcher::ResourceRequestCachePolicy::ReturnCacheDataElseLoad,
        PurCFetcher::ResourceRequestCachePolicy::ReturnCacheDataDontLoad,
        PurCFetcher::ResourceRequestCachePolicy::DoNotUseAnyCache,
        PurCFetcher::ResourceRequestCachePolicy::RefreshAnyCacheData
    >;
};

template<> struct EnumTraits<PurCFetcher::ResourceRequestBase::SameSiteDisposition> {
    using values = EnumValues<
        PurCFetcher::ResourceRequestBase::SameSiteDisposition,
        PurCFetcher::ResourceRequestBase::SameSiteDisposition::Unspecified,
        PurCFetcher::ResourceRequestBase::SameSiteDisposition::SameSite,
        PurCFetcher::ResourceRequestBase::SameSiteDisposition::CrossSite
    >;
};

template<> struct EnumTraits<PurCFetcher::ResourceRequestBase::Requester> {
    using values = EnumValues<
        PurCFetcher::ResourceRequestBase::Requester,
        PurCFetcher::ResourceRequestBase::Requester::Unspecified,
        PurCFetcher::ResourceRequestBase::Requester::Main,
        PurCFetcher::ResourceRequestBase::Requester::XHR,
        PurCFetcher::ResourceRequestBase::Requester::Fetch,
        PurCFetcher::ResourceRequestBase::Requester::Media,
        PurCFetcher::ResourceRequestBase::Requester::ImportScripts,
        PurCFetcher::ResourceRequestBase::Requester::Ping,
        PurCFetcher::ResourceRequestBase::Requester::Beacon
    >;
};

} // namespace PurCWTF
