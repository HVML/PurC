/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
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

#include "ProcessIdentifier.h"
#include <wtf/Hasher.h>
#include <wtf/text/StringConcatenateNumbers.h>

namespace PurCFetcher {

struct MessagePortIdentifier {
    ProcessIdentifier processIdentifier;
    enum PortIdentifierType { };
    ObjectIdentifier<PortIdentifierType> portIdentifier;

    unsigned hash() const;

    template<class Encoder> void encode(Encoder&) const;
    template<class Decoder> static std::optional<MessagePortIdentifier> decode(Decoder&);

#if !LOG_DISABLED
    String logString() const;
#endif
};

inline bool operator==(const MessagePortIdentifier& a, const MessagePortIdentifier& b)
{
    return a.processIdentifier == b.processIdentifier &&  a.portIdentifier == b.portIdentifier;
}

template<class Encoder>
void MessagePortIdentifier::encode(Encoder& encoder) const
{
    encoder << processIdentifier << portIdentifier;
}

template<class Decoder>
std::optional<MessagePortIdentifier> MessagePortIdentifier::decode(Decoder& decoder)
{
    std::optional<ProcessIdentifier> processIdentifier;
    decoder >> processIdentifier;
    if (!processIdentifier)
        return std::nullopt;

    std::optional<ObjectIdentifier<PortIdentifierType>> portIdentifier;
    decoder >> portIdentifier;
    if (!portIdentifier)
        return std::nullopt;

    return { { WTFMove(*processIdentifier), WTFMove(*portIdentifier) } };
}

inline unsigned MessagePortIdentifier::hash() const
{
    return computeHash(processIdentifier.toUInt64(), portIdentifier.toUInt64());
}

#if !LOG_DISABLED

inline String MessagePortIdentifier::logString() const
{
    return makeString(processIdentifier.toUInt64(), '-', portIdentifier.toUInt64());
}

#endif

} // namespace PurCFetcher

namespace PurCWTF {

struct MessagePortIdentifierHash {
    static unsigned hash(const PurCFetcher::MessagePortIdentifier& key) { return key.hash(); }
    static bool equal(const PurCFetcher::MessagePortIdentifier& a, const PurCFetcher::MessagePortIdentifier& b) { return a == b; }
    static const bool safeToCompareToEmptyOrDeleted = true;
};

template<> struct HashTraits<PurCFetcher::MessagePortIdentifier> : GenericHashTraits<PurCFetcher::MessagePortIdentifier> {
    static PurCFetcher::MessagePortIdentifier emptyValue() { return { }; }

    static void constructDeletedValue(PurCFetcher::MessagePortIdentifier& slot) { slot.processIdentifier = makeObjectIdentifier<PurCFetcher::ProcessIdentifierType>(std::numeric_limits<uint64_t>::max()); }

    static bool isDeletedValue(const PurCFetcher::MessagePortIdentifier& slot) { return slot.processIdentifier.toUInt64() == std::numeric_limits<uint64_t>::max(); }
};

template<> struct DefaultHash<PurCFetcher::MessagePortIdentifier> {
    typedef MessagePortIdentifierHash Hash;
};

} // namespace PurCWTF
