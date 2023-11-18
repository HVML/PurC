/*
 * Copyright (C) 2016 Canon Inc.
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions
 * are required to be met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Canon Inc. nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CANON INC. AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CANON INC. AND ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "fetcher-messages-basic.h"
#include <wtf/Markable.h>
#include <wtf/text/WTFString.h>

namespace PurCFetcher {

struct FetchOptions {
    enum class Destination : uint8_t { EmptyString, Audio, Document, Embed, Font, Image, Manifest, Object, Report, Script, Serviceworker, Sharedworker, Style, Track, Video, Worker, Xslt };
    enum class Mode : uint8_t { Navigate, SameOrigin, NoCors, Cors };
    enum class Credentials : uint8_t { Omit, SameOrigin, Include };
    enum class Cache : uint8_t { Default, NoStore, Reload, NoCache, ForceCache, OnlyIfCached };
    enum class Redirect : uint8_t { Follow, Error, Manual };

    FetchOptions() = default;
    FetchOptions(Destination, Mode, Credentials, Cache, Redirect, ReferrerPolicy, String&&, bool);
    FetchOptions isolatedCopy() const { return { destination, mode, credentials, cache, redirect, referrerPolicy, integrity.isolatedCopy(), keepAlive }; }

    template<class Encoder> void encodePersistent(Encoder&) const;
    template<class Decoder> static WARN_UNUSED_RETURN bool decodePersistent(Decoder&, FetchOptions&);
    template<class Encoder> void encode(Encoder&) const;
    template<class Decoder> static std::optional<FetchOptions> decode(Decoder&);

    Destination destination { Destination::EmptyString };
    Mode mode { Mode::NoCors };
    Credentials credentials { Credentials::Omit };
    Cache cache { Cache::Default };
    Redirect redirect { Redirect::Follow };
    ReferrerPolicy referrerPolicy { ReferrerPolicy::EmptyString };
    bool keepAlive { false };
    String integrity;
    Markable<DocumentIdentifier, DocumentIdentifier::MarkableTraits> clientIdentifier;
};

inline FetchOptions::FetchOptions(Destination destination, Mode mode, Credentials credentials, Cache cache, Redirect redirect, ReferrerPolicy referrerPolicy, String&& integrity, bool keepAlive)
    : destination(destination)
    , mode(mode)
    , credentials(credentials)
    , cache(cache)
    , redirect(redirect)
    , referrerPolicy(referrerPolicy)
    , keepAlive(keepAlive)
    , integrity(WTFMove(integrity))
{
}

inline bool isPotentialNavigationOrSubresourceRequest(FetchOptions::Destination destination)
{
    return destination == FetchOptions::Destination::Object
        || destination == FetchOptions::Destination::Embed;
}

inline bool isNonSubresourceRequest(FetchOptions::Destination destination)
{
    return destination == FetchOptions::Destination::Document
        || destination == FetchOptions::Destination::Report
        || destination == FetchOptions::Destination::Serviceworker
        || destination == FetchOptions::Destination::Sharedworker
        || destination == FetchOptions::Destination::Worker;
}

inline bool isScriptLikeDestination(FetchOptions::Destination destination)
{
    return destination == FetchOptions::Destination::Script
        || destination == FetchOptions::Destination::Serviceworker
        || destination == FetchOptions::Destination::Worker;
}

}

namespace PurCWTF {

template<> struct EnumTraits<PurCFetcher::FetchOptions::Destination> {
    using values = EnumValues<
        PurCFetcher::FetchOptions::Destination,
        PurCFetcher::FetchOptions::Destination::EmptyString,
        PurCFetcher::FetchOptions::Destination::Audio,
        PurCFetcher::FetchOptions::Destination::Document,
        PurCFetcher::FetchOptions::Destination::Embed,
        PurCFetcher::FetchOptions::Destination::Font,
        PurCFetcher::FetchOptions::Destination::Image,
        PurCFetcher::FetchOptions::Destination::Manifest,
        PurCFetcher::FetchOptions::Destination::Object,
        PurCFetcher::FetchOptions::Destination::Report,
        PurCFetcher::FetchOptions::Destination::Script,
        PurCFetcher::FetchOptions::Destination::Serviceworker,
        PurCFetcher::FetchOptions::Destination::Sharedworker,
        PurCFetcher::FetchOptions::Destination::Style,
        PurCFetcher::FetchOptions::Destination::Track,
        PurCFetcher::FetchOptions::Destination::Video,
        PurCFetcher::FetchOptions::Destination::Worker,
        PurCFetcher::FetchOptions::Destination::Xslt
    >;
};

template<> struct EnumTraits<PurCFetcher::FetchOptions::Mode> {
    using values = EnumValues<
        PurCFetcher::FetchOptions::Mode,
        PurCFetcher::FetchOptions::Mode::Navigate,
        PurCFetcher::FetchOptions::Mode::SameOrigin,
        PurCFetcher::FetchOptions::Mode::NoCors,
        PurCFetcher::FetchOptions::Mode::Cors
    >;
};

template<> struct EnumTraits<PurCFetcher::FetchOptions::Credentials> {
    using values = EnumValues<
        PurCFetcher::FetchOptions::Credentials,
        PurCFetcher::FetchOptions::Credentials::Omit,
        PurCFetcher::FetchOptions::Credentials::SameOrigin,
        PurCFetcher::FetchOptions::Credentials::Include
    >;
};

template<> struct EnumTraits<PurCFetcher::FetchOptions::Cache> {
    using values = EnumValues<
        PurCFetcher::FetchOptions::Cache,
        PurCFetcher::FetchOptions::Cache::Default,
        PurCFetcher::FetchOptions::Cache::NoStore,
        PurCFetcher::FetchOptions::Cache::Reload,
        PurCFetcher::FetchOptions::Cache::NoCache,
        PurCFetcher::FetchOptions::Cache::ForceCache,
        PurCFetcher::FetchOptions::Cache::OnlyIfCached
    >;
};

template<> struct EnumTraits<PurCFetcher::FetchOptions::Redirect> {
    using values = EnumValues<
        PurCFetcher::FetchOptions::Redirect,
        PurCFetcher::FetchOptions::Redirect::Follow,
        PurCFetcher::FetchOptions::Redirect::Error,
        PurCFetcher::FetchOptions::Redirect::Manual
    >;
};

}

namespace PurCFetcher {

template<class Encoder>
inline void FetchOptions::encodePersistent(Encoder& encoder) const
{
    // Changes to encoding here should bump NetworkCache Storage format version.
    encoder << destination;
    encoder << mode;
    encoder << credentials;
    encoder << cache;
    encoder << redirect;
    encoder << referrerPolicy;
    encoder << integrity;
    encoder << keepAlive;
}

template<class Decoder>
inline bool FetchOptions::decodePersistent(Decoder& decoder, FetchOptions& options)
{
    std::optional<FetchOptions::Destination> destination;
    decoder >> destination;
    if (!destination)
        return false;

    std::optional<FetchOptions::Mode> mode;
    decoder >> mode;
    if (!mode)
        return false;

    std::optional<FetchOptions::Credentials> credentials;
    decoder >> credentials;
    if (!credentials)
        return false;

    std::optional<FetchOptions::Cache> cache;
    decoder >> cache;
    if (!cache)
        return false;

    std::optional<FetchOptions::Redirect> redirect;
    decoder >> redirect;
    if (!redirect)
        return false;

    std::optional<ReferrerPolicy> referrerPolicy;
    decoder >> referrerPolicy;
    if (!referrerPolicy)
        return false;

    std::optional<String> integrity;
    decoder >> integrity;
    if (!integrity)
        return false;

    std::optional<bool> keepAlive;
    decoder >> keepAlive;
    if (!keepAlive)
        return false;

    options.destination = *destination;
    options.mode = *mode;
    options.credentials = *credentials;
    options.cache = *cache;
    options.redirect = *redirect;
    options.referrerPolicy = *referrerPolicy;
    options.integrity = WTFMove(*integrity);
    options.keepAlive = *keepAlive;

    return true;
}

template<class Encoder>
inline void FetchOptions::encode(Encoder& encoder) const
{
    encodePersistent(encoder);
    encoder << clientIdentifier.asOptional();
}

template<class Decoder>
inline std::optional<FetchOptions> FetchOptions::decode(Decoder& decoder)
{
    FetchOptions options;
    if (!decodePersistent(decoder, options))
        return std::nullopt;

    std::optional<std::optional<DocumentIdentifier>> clientIdentifier;
    decoder >> clientIdentifier;
    if (!clientIdentifier)
        return std::nullopt;
    options.clientIdentifier = WTFMove(clientIdentifier.value());

    return options;
}

} // namespace PurCFetcher
