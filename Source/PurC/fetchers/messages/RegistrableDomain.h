/*
 * Copyright (C) 2019 Apple Inc. All rights reserved.
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

#include <wtf/HashTraits.h>
#include <wtf/URL.h>
#include <wtf/text/WTFString.h>

namespace PurCFetcher {

class RegistrableDomain {
    WTF_MAKE_FAST_ALLOCATED;
public:
    RegistrableDomain() = default;

    bool operator!=(const RegistrableDomain& other) const { return m_registrableDomain != other.m_registrableDomain; }
    bool operator==(const RegistrableDomain& other) const { return m_registrableDomain == other.m_registrableDomain; }
    bool operator==(const char* other) const { return m_registrableDomain == other; }

    RegistrableDomain(PurCWTF::HashTableDeletedValueType)
        : m_registrableDomain(PurCWTF::HashTableDeletedValue) { }
    bool isHashTableDeletedValue() const { return m_registrableDomain.isHashTableDeletedValue(); }
    unsigned hash() const { return m_registrableDomain.hash(); }

    struct RegistrableDomainHash {
        static unsigned hash(const RegistrableDomain& registrableDomain) { return registrableDomain.m_registrableDomain.hash(); }
        static bool equal(const RegistrableDomain& a, const RegistrableDomain& b) { return a == b; }
        static const bool safeToCompareToEmptyOrDeleted = false;
    };

    template<class Encoder> void encode(Encoder&) const;
    template<class Decoder> static std::optional<RegistrableDomain> decode(Decoder&);

private:
    String m_registrableDomain;
};

template<class Encoder>
void RegistrableDomain::encode(Encoder& encoder) const
{
    encoder << m_registrableDomain;
}

template<class Decoder>
std::optional<RegistrableDomain> RegistrableDomain::decode(Decoder& decoder)
{
    std::optional<String> domain;
    decoder >> domain;
    if (!domain)
        return std::nullopt;

    RegistrableDomain registrableDomain;
    registrableDomain.m_registrableDomain = WTFMove(*domain);
    return registrableDomain;
}

} // namespace PurCFetcher

namespace PurCWTF {
template<> struct DefaultHash<PurCFetcher::RegistrableDomain> {
    using Hash = PurCFetcher::RegistrableDomain::RegistrableDomainHash;
};
template<> struct HashTraits<PurCFetcher::RegistrableDomain> : SimpleClassHashTraits<PurCFetcher::RegistrableDomain> { };
}
