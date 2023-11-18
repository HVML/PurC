/*
 * Copyright (C) 2010-2020 Apple Inc. All rights reserved.
 * Copyright (C) 2020 Beijing FMSoft Technologies Co., Ltd.
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

#include "fetcher-messages-basic.h"
#include "ArgumentCoders.h"
#include "NetworkLoadMetrics.h"
#include <wtf/EnumTraits.h>

namespace PurCFetcher {

class AuthenticationChallenge;
class CertificateInfo;
class ProtectionSpace;
class Credential;
class SecurityOrigin;
class SharedBuffer;
class ResourceError;
class ResourceRequest;
class ResourceResponse;
struct CacheQueryOptions;
struct NetworkProxySettings;

namespace DOMCacheEngine {
struct CacheInfo;
struct Record;
}


} // namespace PurCFetcher

namespace IPC {

template<> struct ArgumentCoder<PurCFetcher::AuthenticationChallenge> {
    static void encode(Encoder&, const PurCFetcher::AuthenticationChallenge&);
    static WARN_UNUSED_RETURN bool decode(Decoder&, PurCFetcher::AuthenticationChallenge&);
};

template<> struct ArgumentCoder<PurCFetcher::ProtectionSpace> {
    static void encode(Encoder&, const PurCFetcher::ProtectionSpace&);
    static WARN_UNUSED_RETURN bool decode(Decoder&, PurCFetcher::ProtectionSpace&);
    static void encodePlatformData(Encoder&, const PurCFetcher::ProtectionSpace&);
    static WARN_UNUSED_RETURN bool decodePlatformData(Decoder&, PurCFetcher::ProtectionSpace&);
};

template<> struct ArgumentCoder<PurCFetcher::Credential> {
    static void encode(Encoder&, const PurCFetcher::Credential&);
    static WARN_UNUSED_RETURN bool decode(Decoder&, PurCFetcher::Credential&);
    static void encodePlatformData(Encoder&, const PurCFetcher::Credential&);
    static WARN_UNUSED_RETURN bool decodePlatformData(Decoder&, PurCFetcher::Credential&);
};

template<> struct ArgumentCoder<PurCFetcher::CertificateInfo> {
    static void encode(Encoder&, const PurCFetcher::CertificateInfo&);
    static WARN_UNUSED_RETURN bool decode(Decoder&, PurCFetcher::CertificateInfo&);
};

template<> struct ArgumentCoder<RefPtr<PurCFetcher::SharedBuffer>> {
    static void encode(Encoder&, const RefPtr<PurCFetcher::SharedBuffer>&);
    static std::optional<RefPtr<PurCFetcher::SharedBuffer>> decode(Decoder&);
};

template<> struct ArgumentCoder<Ref<PurCFetcher::SharedBuffer>> {
    static void encode(Encoder&, const Ref<PurCFetcher::SharedBuffer>&);
    static std::optional<Ref<PurCFetcher::SharedBuffer>> decode(Decoder&);
};


template<> struct ArgumentCoder<PurCFetcher::ResourceError> {
    static void encode(Encoder&, const PurCFetcher::ResourceError&);
    static WARN_UNUSED_RETURN bool decode(Decoder&, PurCFetcher::ResourceError&);
    static void encodePlatformData(Encoder&, const PurCFetcher::ResourceError&);
    static WARN_UNUSED_RETURN bool decodePlatformData(Decoder&, PurCFetcher::ResourceError&);
};

template<> struct ArgumentCoder<PurCFetcher::ResourceRequest> {
    static void encode(Encoder&, const PurCFetcher::ResourceRequest&);
    static WARN_UNUSED_RETURN bool decode(Decoder&, PurCFetcher::ResourceRequest&);
    static void encodePlatformData(Encoder&, const PurCFetcher::ResourceRequest&);
    static WARN_UNUSED_RETURN bool decodePlatformData(Decoder&, PurCFetcher::ResourceRequest&);
};

template<> struct ArgumentCoder<PurCFetcher::CacheQueryOptions> {
    static void encode(Encoder&, const PurCFetcher::CacheQueryOptions&);
    static WARN_UNUSED_RETURN bool decode(Decoder&, PurCFetcher::CacheQueryOptions&);
};

template<> struct ArgumentCoder<PurCFetcher::NetworkProxySettings> {
    static void encode(Encoder&, const PurCFetcher::NetworkProxySettings&);
    static WARN_UNUSED_RETURN bool decode(Decoder&, PurCFetcher::NetworkProxySettings&);
};

template<> struct ArgumentCoder<Vector<RefPtr<PurCFetcher::SecurityOrigin>>> {
    static void encode(Encoder&, const Vector<RefPtr<PurCFetcher::SecurityOrigin>>&);
    static WARN_UNUSED_RETURN bool decode(Decoder&, Vector<RefPtr<PurCFetcher::SecurityOrigin>>&);
};

template<> struct ArgumentCoder<PurCFetcher::DOMCacheEngine::CacheInfo> {
    static void encode(Encoder&, const PurCFetcher::DOMCacheEngine::CacheInfo&);
    static std::optional<PurCFetcher::DOMCacheEngine::CacheInfo> decode(Decoder&);
};

template<> struct ArgumentCoder<PurCFetcher::DOMCacheEngine::Record> {
    static void encode(Encoder&, const PurCFetcher::DOMCacheEngine::Record&);
    static std::optional<PurCFetcher::DOMCacheEngine::Record> decode(Decoder&);
};

} // namespace IPC

namespace PurCWTF {

template<> struct EnumTraits<PurCFetcher::NetworkLoadPriority> {
    using values = EnumValues<
        PurCFetcher::NetworkLoadPriority,
        PurCFetcher::NetworkLoadPriority::Low,
        PurCFetcher::NetworkLoadPriority::Medium,
        PurCFetcher::NetworkLoadPriority::High,
        PurCFetcher::NetworkLoadPriority::Unknown
    >;
};

template<> struct EnumTraits<PurCFetcher::StoredCredentialsPolicy> {
    using values = EnumValues<
        PurCFetcher::StoredCredentialsPolicy,
        PurCFetcher::StoredCredentialsPolicy::DoNotUse,
        PurCFetcher::StoredCredentialsPolicy::Use,
        PurCFetcher::StoredCredentialsPolicy::EphemeralStateless
    >;
};

} // namespace PurCWTF
