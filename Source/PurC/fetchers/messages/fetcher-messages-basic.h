/*
 * @file fetcher-messages-basic.h
 * @author XueShuming
 * @date 2021/11/28
 * @brief The basic type for fetcher messages.
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

#ifndef PURC_FETCHER_MESSAGES_BASIC_H
#define PURC_FETCHER_MESSAGES_BASIC_H

#include <stdint.h>
#include <wtf/EnumTraits.h>
#include <wtf/Forward.h>
#include <wtf/Markable.h>
#include <wtf/Seconds.h>
#include <wtf/ObjectIdentifier.h>

namespace PurCFetcher {

enum class CacheModel : uint8_t {
    DocumentViewer,
    DocumentBrowser,
    PrimaryWebBrowser
};

struct CacheControlDirectives {
    constexpr CacheControlDirectives()
        : noCache(false)
        , noStore(false)
        , mustRevalidate(false)
        , immutable(false)
        { }

    Markable<Seconds, Seconds::MarkableTraits> maxAge;
    Markable<Seconds, Seconds::MarkableTraits> maxStale;
    Markable<Seconds, Seconds::MarkableTraits> staleWhileRevalidate;
    bool noCache : 1;
    bool noStore : 1;
    bool mustRevalidate : 1;
    bool immutable : 1;
};

enum DocumentIdentifierType { };
using DocumentIdentifier = ObjectIdentifier<DocumentIdentifierType>;

enum FrameIdentifierType { };
using FrameIdentifier = ObjectIdentifier<FrameIdentifierType>;

enum WebPageProxyIdentifierType { };
using WebPageProxyIdentifier = ObjectIdentifier<WebPageProxyIdentifierType>;

enum PageIdentifierType { };
using PageIdentifier = ObjectIdentifier<PageIdentifierType>;

enum ProcessIdentifierType { };
using ProcessIdentifier = ObjectIdentifier<ProcessIdentifierType>;

enum class HTTPCookieAcceptPolicy : uint8_t {
    AlwaysAccept = 0,
    Never = 1,
    OnlyFromMainDocumentDomain = 2,
    ExclusivelyFromMainDocumentDomain = 3,
};

enum class NavigatingToAppBoundDomain : bool { Yes, No };

enum class ThirdPartyCookieBlockingMode : uint8_t { All, AllExceptBetweenAppBoundDomains, AllOnSitesWithoutUserInteraction, OnlyAccordingToPerDomainPolicy };
enum class FirstPartyWebsiteDataRemovalMode : uint8_t { AllButCookies, None, AllButCookiesLiveOnTestingTimeout, AllButCookiesReproTestingTimeout };

enum class StoredCredentialsPolicy : uint8_t {
    DoNotUse,
    Use,
    EphemeralStateless
};
static constexpr unsigned bitWidthOfStoredCredentialsPolicy = 2;

enum class ShouldRelaxThirdPartyCookieBlocking : bool { No, Yes };

enum class ReferrerPolicy : uint8_t {
    EmptyString,
    NoReferrer,
    NoReferrerWhenDowngrade,
    SameOrigin,
    Origin,
    StrictOrigin,
    OriginWhenCrossOrigin,
    StrictOriginWhenCrossOrigin,
    UnsafeUrl
};

enum class ResourceLoadPriority : uint8_t {
    VeryLow,
    Low,
    Medium,
    High,
    VeryHigh,
    Lowest = VeryLow,
    Highest = VeryHigh,
};

} // namespace PurCFetcher

namespace WTF {

template<> struct EnumTraits<PurCFetcher::CacheModel> {
    using values = EnumValues<
    PurCFetcher::CacheModel,
    PurCFetcher::CacheModel::DocumentViewer,
    PurCFetcher::CacheModel::DocumentBrowser,
    PurCFetcher::CacheModel::PrimaryWebBrowser
    >;
};

template<> struct EnumTraits<PurCFetcher::HTTPCookieAcceptPolicy> {
    using values = EnumValues<
        PurCFetcher::HTTPCookieAcceptPolicy,
        PurCFetcher::HTTPCookieAcceptPolicy::AlwaysAccept,
        PurCFetcher::HTTPCookieAcceptPolicy::Never,
        PurCFetcher::HTTPCookieAcceptPolicy::OnlyFromMainDocumentDomain,
        PurCFetcher::HTTPCookieAcceptPolicy::ExclusivelyFromMainDocumentDomain
    >;
};

template<> struct EnumTraits<PurCFetcher::ThirdPartyCookieBlockingMode> {
    using values = EnumValues<
        PurCFetcher::ThirdPartyCookieBlockingMode,
        PurCFetcher::ThirdPartyCookieBlockingMode::All,
        PurCFetcher::ThirdPartyCookieBlockingMode::AllExceptBetweenAppBoundDomains,
        PurCFetcher::ThirdPartyCookieBlockingMode::AllOnSitesWithoutUserInteraction,
        PurCFetcher::ThirdPartyCookieBlockingMode::OnlyAccordingToPerDomainPolicy
    >;
};

template<> struct EnumTraits<PurCFetcher::FirstPartyWebsiteDataRemovalMode> {
    using values = EnumValues<
        PurCFetcher::FirstPartyWebsiteDataRemovalMode,
        PurCFetcher::FirstPartyWebsiteDataRemovalMode::AllButCookies,
        PurCFetcher::FirstPartyWebsiteDataRemovalMode::None,
        PurCFetcher::FirstPartyWebsiteDataRemovalMode::AllButCookiesLiveOnTestingTimeout,
        PurCFetcher::FirstPartyWebsiteDataRemovalMode::AllButCookiesReproTestingTimeout
    >;
};

template<> struct EnumTraits<PurCFetcher::ReferrerPolicy> {
    using values = EnumValues<
        PurCFetcher::ReferrerPolicy,
        PurCFetcher::ReferrerPolicy::EmptyString,
        PurCFetcher::ReferrerPolicy::NoReferrer,
        PurCFetcher::ReferrerPolicy::NoReferrerWhenDowngrade,
        PurCFetcher::ReferrerPolicy::SameOrigin,
        PurCFetcher::ReferrerPolicy::Origin,
        PurCFetcher::ReferrerPolicy::StrictOrigin,
        PurCFetcher::ReferrerPolicy::OriginWhenCrossOrigin,
        PurCFetcher::ReferrerPolicy::StrictOriginWhenCrossOrigin,
        PurCFetcher::ReferrerPolicy::UnsafeUrl
    >;
};

template<> struct EnumTraits<PurCFetcher::ResourceLoadPriority> {
    using values = EnumValues<
        PurCFetcher::ResourceLoadPriority,
        PurCFetcher::ResourceLoadPriority::VeryLow,
        PurCFetcher::ResourceLoadPriority::Low,
        PurCFetcher::ResourceLoadPriority::Medium,
        PurCFetcher::ResourceLoadPriority::High,
        PurCFetcher::ResourceLoadPriority::VeryHigh
    >;
};

} // namespace WTF

#endif /* not defined PURC_FETCHER_MESSAGES_BASIC_H */


