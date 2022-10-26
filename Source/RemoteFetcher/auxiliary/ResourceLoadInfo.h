/*
 * Copyright (C) 2020 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "ArgumentCoders.h"
#include "NetworkResourceLoadIdentifier.h"
#include "FrameIdentifier.h"
#include <wtf/URL.h>
#include <wtf/WallTime.h>

namespace PurCFetcher {

struct ResourceLoadInfo {

    enum class Type : uint8_t {
        ApplicationManifest,
        Beacon,
        CSPReport,
        Document,
        Fetch,
        Font,
        Image,
        Media,
        Object,
        Other,
        Ping,
        Script,
        Stylesheet,
        XMLHTTPRequest,
        XSLT
    };
    
    NetworkResourceLoadIdentifier resourceLoadID;
    Optional<PurCFetcher::FrameIdentifier> frameID;
    Optional<PurCFetcher::FrameIdentifier> parentFrameID;
    URL originalURL;
    String originalHTTPMethod;
    WallTime eventTimestamp;
    bool loadedFromCache { false };
    Type type { Type::Other };

    void encode(IPC::Encoder& encoder) const
    {
        encoder << resourceLoadID;
        encoder << frameID;
        encoder << parentFrameID;
        encoder << originalURL;
        encoder << originalHTTPMethod;
        encoder << eventTimestamp;
        encoder << loadedFromCache;
        encoder << type;
    }

    static Optional<ResourceLoadInfo> decode(IPC::Decoder& decoder)
    {
        Optional<NetworkResourceLoadIdentifier> resourceLoadID;
        decoder >> resourceLoadID;
        if (!resourceLoadID)
            return PurCWTF::nullopt;

        Optional<Optional<PurCFetcher::FrameIdentifier>> frameID;
        decoder >> frameID;
        if (!frameID)
            return PurCWTF::nullopt;

        Optional<Optional<PurCFetcher::FrameIdentifier>> parentFrameID;
        decoder >> parentFrameID;
        if (!parentFrameID)
            return PurCWTF::nullopt;

        Optional<URL> originalURL;
        decoder >> originalURL;
        if (!originalURL)
            return PurCWTF::nullopt;

        Optional<String> originalHTTPMethod;
        decoder >> originalHTTPMethod;
        if (!originalHTTPMethod)
            return PurCWTF::nullopt;

        Optional<WallTime> eventTimestamp;
        decoder >> eventTimestamp;
        if (!eventTimestamp)
            return PurCWTF::nullopt;

        Optional<bool> loadedFromCache;
        decoder >> loadedFromCache;
        if (!loadedFromCache)
            return PurCWTF::nullopt;

        Optional<Type> type;
        decoder >> type;
        if (!type)
            return PurCWTF::nullopt;

        return {{
            WTFMove(*resourceLoadID),
            WTFMove(*frameID),
            WTFMove(*parentFrameID),
            WTFMove(*originalURL),
            WTFMove(*originalHTTPMethod),
            WTFMove(*eventTimestamp),
            WTFMove(*loadedFromCache),
            WTFMove(*type),
        }};
    }
};

} // namespace PurCFetcher

namespace PurCWTF {

template<> struct EnumTraits<PurCFetcher::ResourceLoadInfo::Type> {
    using values = EnumValues<
        PurCFetcher::ResourceLoadInfo::Type,
        PurCFetcher::ResourceLoadInfo::Type::ApplicationManifest,
        PurCFetcher::ResourceLoadInfo::Type::Beacon,
        PurCFetcher::ResourceLoadInfo::Type::CSPReport,
        PurCFetcher::ResourceLoadInfo::Type::Document,
        PurCFetcher::ResourceLoadInfo::Type::Fetch,
        PurCFetcher::ResourceLoadInfo::Type::Font,
        PurCFetcher::ResourceLoadInfo::Type::Image,
        PurCFetcher::ResourceLoadInfo::Type::Media,
        PurCFetcher::ResourceLoadInfo::Type::Object,
        PurCFetcher::ResourceLoadInfo::Type::Other,
        PurCFetcher::ResourceLoadInfo::Type::Ping,
        PurCFetcher::ResourceLoadInfo::Type::Script,
        PurCFetcher::ResourceLoadInfo::Type::Stylesheet,
        PurCFetcher::ResourceLoadInfo::Type::XMLHTTPRequest,
        PurCFetcher::ResourceLoadInfo::Type::XSLT
    >;
};

} // namespace PurCWTF
