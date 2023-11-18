/*
 * Copyright (C) 2009 Joseph Pecoraro. All rights reserved.
 * Copyright (C) 2017-2018 Apple Inc. All rights reserved.
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
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

#include <optional>
#include <wtf/URL.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace PurCFetcher {

struct Cookie {
    Cookie() = default;

    template<class Encoder> void encode(Encoder&) const;
    template<class Decoder> static std::optional<Cookie> decode(Decoder&);

    String name;
    String value;
    String domain;
    String path;
    // Creation and expiration dates are expressed as milliseconds since the UNIX epoch.
    double created { 0 };
    std::optional<double> expires;
    bool httpOnly { false };
    bool secure { false };
    bool session { false };
    String comment;
    URL commentURL;
    Vector<uint16_t> ports;

    enum class SameSitePolicy { None, Lax, Strict };
    SameSitePolicy sameSite { SameSitePolicy::None };
};

template<class Encoder>
void Cookie::encode(Encoder& encoder) const
{
    encoder << name;
    encoder << value;
    encoder << domain;
    encoder << path;
    encoder << created;
    encoder << expires;
    encoder << httpOnly;
    encoder << secure;
    encoder << session;
    encoder << comment;
    encoder << commentURL;
    encoder << ports;
    encoder << sameSite;
}

template<class Decoder>
std::optional<Cookie> Cookie::decode(Decoder& decoder)
{
    Cookie cookie;
    if (!decoder.decode(cookie.name))
        return std::nullopt;
    if (!decoder.decode(cookie.value))
        return std::nullopt;
    if (!decoder.decode(cookie.domain))
        return std::nullopt;
    if (!decoder.decode(cookie.path))
        return std::nullopt;
    if (!decoder.decode(cookie.created))
        return std::nullopt;
    if (!decoder.decode(cookie.expires))
        return std::nullopt;
    if (!decoder.decode(cookie.httpOnly))
        return std::nullopt;
    if (!decoder.decode(cookie.secure))
        return std::nullopt;
    if (!decoder.decode(cookie.session))
        return std::nullopt;
    if (!decoder.decode(cookie.comment))
        return std::nullopt;
    if (!decoder.decode(cookie.commentURL))
        return std::nullopt;
    if (!decoder.decode(cookie.ports))
        return std::nullopt;
    if (!decoder.decode(cookie.sameSite))
        return std::nullopt;
    return cookie;
}

}

namespace PurCWTF {
    template<> struct EnumTraits<PurCFetcher::Cookie::SameSitePolicy> {
    using values = EnumValues<
        PurCFetcher::Cookie::SameSitePolicy,
        PurCFetcher::Cookie::SameSitePolicy::None,
        PurCFetcher::Cookie::SameSitePolicy::Lax,
        PurCFetcher::Cookie::SameSitePolicy::Strict
    >;
};
}
