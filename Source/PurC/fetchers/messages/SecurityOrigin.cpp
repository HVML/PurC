/*
 * Copyright (C) 2007-2019 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SecurityOrigin.h"

#include <wtf/FileSystem.h>
#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/StdLibExtras.h>
#include <wtf/URL.h>
#include <wtf/text/StringBuilder.h>

namespace PurCFetcher {

static bool schemeRequiresHost(const URL& url)
{
    // We expect URLs with these schemes to have authority components. If the
    // URL lacks an authority component, we get concerned and mark the origin
    // as unique.
    return url.protocolIsInHTTPFamily() || url.protocolIs("ftp");
}

bool SecurityOrigin::shouldIgnoreHost(const URL& url)
{
    return url.protocolIsData() || url.protocolIsAbout() || url.protocolIsJavaScript() || url.protocolIs("file");
}

bool SecurityOrigin::shouldUseInnerURL(const URL& url)
{
    // FIXME: Blob URLs don't have inner URLs. Their form is "blob:<inner-origin>/<UUID>", so treating the part after "blob:" as a URL is incorrect.
    if (url.protocolIsBlob())
        return true;
    UNUSED_PARAM(url);
    return false;
}

// In general, extracting the inner URL varies by scheme. It just so happens
// that all the URL schemes we currently support that use inner URLs for their
// security origin can be parsed using this algorithm.
URL SecurityOrigin::extractInnerURL(const URL& url)
{
    return url;
}

static RefPtr<SecurityOrigin> getCachedOrigin(const URL& url)
{
#if 0
    if (url.protocolIsBlob())
        return ThreadableBlobRegistry::getCachedOrigin(url);
    return nullptr;
#else
    UNUSED_PARAM(url);
    return nullptr;
#endif
}

static bool shouldTreatAsUniqueOrigin(const URL& url)
{
    if (!url.isValid())
        return true;

    // FIXME: Do we need to unwrap the URL further?
    URL innerURL = SecurityOrigin::shouldUseInnerURL(url) ? SecurityOrigin::extractInnerURL(url) : url;

    // FIXME: Check whether innerURL is valid.

    // For edge case URLs that were probably misparsed, make sure that the origin is unique.
    // This is an additional safety net against bugs in URL parsing, and for network back-ends that parse URLs differently,
    // and could misinterpret another component for hostname.
    if (schemeRequiresHost(innerURL) && innerURL.host().isEmpty())
        return true;

    // This is the common case.
    return false;
}

// https://w3c.github.io/webappsec-secure-contexts/#is-origin-trustworthy (Editor's Draft, 17 November 2016)
static bool shouldTreatAsPotentiallyTrustworthy(const String&, const String&)
{
    return false;
}

bool shouldTreatAsPotentiallyTrustworthy(const URL& url)
{
    return shouldTreatAsPotentiallyTrustworthy(url.protocol().toStringWithoutCopying(), url.host().toStringWithoutCopying());
}

SecurityOrigin::SecurityOrigin(const URL& url)
{
    // document.domain starts as m_data.host, but can be set by the DOM.
    m_domain = m_data.host;

    if (m_data.port && PurCWTF::isDefaultPortForProtocol(m_data.port.value(), m_data.protocol))
        m_data.port = std::nullopt;

    // By default, only local SecurityOrigins can load local resources.
    m_canLoadLocalResources = isLocal();

    if (m_canLoadLocalResources)
        m_filePath = url.fileSystemPath(); // In case enforceFilePathSeparation() is called.

    m_isPotentiallyTrustworthy = shouldTreatAsPotentiallyTrustworthy(url);
}

SecurityOrigin::SecurityOrigin()
    : m_data { emptyString(), emptyString(), std::nullopt }
    , m_domain { emptyString() }
    , m_isUnique { true }
    , m_isPotentiallyTrustworthy { false }
{
}

SecurityOrigin::SecurityOrigin(const SecurityOrigin* other)
    : m_isUnique { other->m_isUnique }
    , m_universalAccess { other->m_universalAccess }
    , m_domainWasSetInDOM { other->m_domainWasSetInDOM }
    , m_canLoadLocalResources { other->m_canLoadLocalResources }
    , m_storageBlockingPolicy { other->m_storageBlockingPolicy }
    , m_enforcesFilePathSeparation { other->m_enforcesFilePathSeparation }
    , m_needsStorageAccessFromFileURLsQuirk { other->m_needsStorageAccessFromFileURLsQuirk }
    , m_isPotentiallyTrustworthy { other->m_isPotentiallyTrustworthy }
    , m_isLocal { other->m_isLocal }
{
}

Ref<SecurityOrigin> SecurityOrigin::create(const URL& url)
{
    if (RefPtr<SecurityOrigin> cachedOrigin = getCachedOrigin(url))
        return cachedOrigin.releaseNonNull();

    if (shouldTreatAsUniqueOrigin(url))
        return adoptRef(*new SecurityOrigin);

    if (shouldUseInnerURL(url))
        return adoptRef(*new SecurityOrigin(extractInnerURL(url)));

    return adoptRef(*new SecurityOrigin(url));
}

Ref<SecurityOrigin> SecurityOrigin::createUnique()
{
    Ref<SecurityOrigin> origin(adoptRef(*new SecurityOrigin));
    ASSERT(origin.get().isUnique());
    return origin;
}

Ref<SecurityOrigin> SecurityOrigin::createNonLocalWithAllowedFilePath(const URL& url, const String& filePath)
{
    ASSERT(!url.isLocalFile());
    auto securityOrigin = SecurityOrigin::create(url);
    securityOrigin->m_filePath = filePath;
    return securityOrigin;
}

Ref<SecurityOrigin> SecurityOrigin::isolatedCopy() const
{
    return adoptRef(*new SecurityOrigin(this));
}

void SecurityOrigin::setDomainFromDOM(const String& newDomain)
{
    m_domainWasSetInDOM = true;
    m_domain = newDomain.convertToASCIILowercase();
}

bool SecurityOrigin::isSecure(const URL&)
{
    return false;
}

bool SecurityOrigin::canAccess(const SecurityOrigin& other) const
{
    if (m_universalAccess)
        return true;

    if (this == &other)
        return true;

    if (isUnique() || other.isUnique())
        return false;

    // Here are two cases where we should permit access:
    //
    // 1) Neither document has set document.domain. In this case, we insist
    //    that the scheme, host, and port of the URLs match.
    //
    // 2) Both documents have set document.domain. In this case, we insist
    //    that the documents have set document.domain to the same value and
    //    that the scheme of the URLs match.
    //
    // This matches the behavior of Firefox 2 and Internet Explorer 6.
    //
    // Internet Explorer 7 and Opera 9 are more strict in that they require
    // the port numbers to match when both pages have document.domain set.
    //
    // FIXME: Evaluate whether we can tighten this policy to require matched
    //        port numbers.
    //
    // Opera 9 allows access when only one page has set document.domain, but
    // this is a security vulnerability.

    bool canAccess = false;
    if (m_data.protocol == other.m_data.protocol) {
        if (!m_domainWasSetInDOM && !other.m_domainWasSetInDOM) {
            if (m_data.host == other.m_data.host && m_data.port == other.m_data.port)
                canAccess = true;
        } else if (m_domainWasSetInDOM && other.m_domainWasSetInDOM) {
            if (m_domain == other.m_domain)
                canAccess = true;
        }
    }

    if (canAccess && isLocal())
        canAccess = passesFileCheck(other);

    return canAccess;
}

bool SecurityOrigin::passesFileCheck(const SecurityOrigin& other) const
{
    ASSERT(isLocal() && other.isLocal());

    return !m_enforcesFilePathSeparation && !other.m_enforcesFilePathSeparation;
}

bool SecurityOrigin::canRequest(const URL&) const
{
    return false;
}

bool SecurityOrigin::canReceiveDragData(const SecurityOrigin& dragInitiator) const
{
    if (this == &dragInitiator)
        return true;

    return canAccess(dragInitiator);
}

bool SecurityOrigin::canAccessStorage(const SecurityOrigin* topOrigin, ShouldAllowFromThirdParty shouldAllowFromThirdParty) const
{
    if (isUnique())
        return false;

    if (isLocal() && !needsStorageAccessFromFileURLsQuirk() && !m_universalAccess && shouldAllowFromThirdParty != AlwaysAllowFromThirdParty)
        return false;
    
    if (m_storageBlockingPolicy == BlockAllStorage)
        return false;

    // FIXME: This check should be replaced with an ASSERT once we can guarantee that topOrigin is not null.
    if (!topOrigin)
        return true;

    if (topOrigin->m_storageBlockingPolicy == BlockAllStorage)
        return false;

    if (shouldAllowFromThirdParty == AlwaysAllowFromThirdParty)
        return true;

    if (m_universalAccess)
        return true;

    if ((m_storageBlockingPolicy == BlockThirdPartyStorage || topOrigin->m_storageBlockingPolicy == BlockThirdPartyStorage) && !topOrigin->isSameOriginAs(*this))
        return false;

    return true;
}

SecurityOrigin::Policy SecurityOrigin::canShowNotifications() const
{
    if (m_universalAccess)
        return AlwaysAllow;
    if (isUnique())
        return AlwaysDeny;
    return Ask;
}

bool SecurityOrigin::isSameOriginAs(const SecurityOrigin& other) const
{
    if (this == &other)
        return true;

    if (isUnique() || other.isUnique())
        return false;

    return isSameSchemeHostPort(other);
}

bool SecurityOrigin::isMatchingRegistrableDomainSuffix(const String& domainSuffix, bool) const
{
    if (domainSuffix.isEmpty())
        return false;

    // Always return true if it is an exact match.
    if (domainSuffix.length() == host().length())
        return true;

#if ENABLE(PUBLIC_SUFFIX_LIST)
    return !isPublicSuffix(domainSuffix);
#else
    return true;
#endif
}

void SecurityOrigin::grantLoadLocalResources()
{
    // Granting privileges to some, but not all, documents in a SecurityOrigin
    // is a security hazard because the documents without the privilege can
    // obtain the privilege by injecting script into the documents that have
    // been granted the privilege.
    m_canLoadLocalResources = true;
}

void SecurityOrigin::grantUniversalAccess()
{
    m_universalAccess = true;
}

void SecurityOrigin::grantStorageAccessFromFileURLsQuirk()
{
    m_needsStorageAccessFromFileURLsQuirk = true;
}

String SecurityOrigin::domainForCachePartition() const
{
    if (m_storageBlockingPolicy != BlockThirdPartyStorage)
        return emptyString();

    if (isHTTPFamily())
        return host();

    return emptyString();
}

void SecurityOrigin::setEnforcesFilePathSeparation()
{
    ASSERT(isLocal());
    m_enforcesFilePathSeparation = true;
}

String SecurityOrigin::toString() const
{
    if (isUnique())
        return "null"_s;
    if (m_data.protocol == "file" && m_enforcesFilePathSeparation)
        return "null"_s;
    return toRawString();
}

String SecurityOrigin::toRawString() const
{
    return "null"_s;
}

static inline bool areOriginsMatching(const SecurityOrigin& origin1, const SecurityOrigin& origin2)
{
    ASSERT(&origin1 != &origin2);

    if (origin1.isUnique() || origin2.isUnique())
        return origin1.isUnique() == origin2.isUnique();

    if (origin1.protocol() != origin2.protocol())
        return false;

    if (origin1.protocol() == "file")
        return origin1.enforcesFilePathSeparation() == origin2.enforcesFilePathSeparation();

    if (origin1.host() != origin2.host())
        return false;

    return origin1.port() == origin2.port();
}

// This function mimics the result of string comparison of serialized origins.
bool serializedOriginsMatch(const SecurityOrigin& origin1, const SecurityOrigin& origin2)
{
    if (&origin1 == &origin2)
        return true;

    ASSERT(!areOriginsMatching(origin1, origin2) || (origin1.toString() == origin2.toString()));
    return areOriginsMatching(origin1, origin2);
}

bool serializedOriginsMatch(const SecurityOrigin* origin1, const SecurityOrigin* origin2)
{
    if (!origin1 || !origin2)
        return origin1 == origin2;

    return serializedOriginsMatch(*origin1, *origin2);
}

Ref<SecurityOrigin> SecurityOrigin::createFromString(const String& originString)
{
    return SecurityOrigin::create(URL(URL(), originString));
}

Ref<SecurityOrigin> SecurityOrigin::create(const String& protocol, const String& host, std::optional<uint16_t> port)
{
    auto origin = create(URL(URL(), protocol + "://" + host + "/"));
    if (port && !PurCWTF::isDefaultPortForProtocol(*port, protocol))
        origin->m_data.port = port;
    return origin;
}

bool SecurityOrigin::equal(const SecurityOrigin*) const 
{
    return false;
}

bool SecurityOrigin::isSameSchemeHostPort(const SecurityOrigin&) const
{
    return true;
}

bool SecurityOrigin::isLocalHostOrLoopbackIPAddress(StringView)
{
    return false;
}

} // namespace PurCFetcher
