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
    std::optional<PurCFetcher::FrameIdentifier> frameID;
    std::optional<PurCFetcher::FrameIdentifier> parentFrameID;
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

    static std::optional<ResourceLoadInfo> decode(IPC::Decoder& decoder)
    {
        std::optional<NetworkResourceLoadIdentifier> resourceLoadID;
        decoder >> resourceLoadID;
        if (!resourceLoadID)
            return std::nullopt;

        std::optional<std::optional<PurCFetcher::FrameIdentifier>> frameID;
        decoder >> frameID;
        if (!frameID)
            return std::nullopt;

        std::optional<std::optional<PurCFetcher::FrameIdentifier>> parentFrameID;
        decoder >> parentFrameID;
        if (!parentFrameID)
            return std::nullopt;

        std::optional<URL> originalURL;
        decoder >> originalURL;
        if (!originalURL)
            return std::nullopt;

        std::optional<String> originalHTTPMethod;
        decoder >> originalHTTPMethod;
        if (!originalHTTPMethod)
            return std::nullopt;

        std::optional<WallTime> eventTimestamp;
        decoder >> eventTimestamp;
        if (!eventTimestamp)
            return std::nullopt;

        std::optional<bool> loadedFromCache;
        decoder >> loadedFromCache;
        if (!loadedFromCache)
            return std::nullopt;

        std::optional<Type> type;
        decoder >> type;
        if (!type)
            return std::nullopt;

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
