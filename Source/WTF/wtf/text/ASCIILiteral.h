/*
 * Copyright (C) 2018 Yusuke Suzuki <utatane.tea@gmail.com>
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

#include <wtf/ASCIICType.h>

namespace PurCWTF {

class PrintStream;

class ASCIILiteral final {
public:
    operator const char*() const { return m_characters; }

    static constexpr ASCIILiteral fromLiteralUnsafe(const char* string)
    {
        return ASCIILiteral { string };
    }

    WTF_EXPORT_PRIVATE void dump(PrintStream& out) const;

    static constexpr ASCIILiteral null()
    {
        return ASCIILiteral { nullptr };
    }

    const char* characters() const { return m_characters; }

private:
    constexpr explicit ASCIILiteral(const char* characters) : m_characters(characters) { }

    const char* m_characters;
};

inline namespace StringLiterals {

constexpr ASCIILiteral operator"" _s(const char* characters, size_t n)
{
#if ENABLE_ASSERTS
    for (size_t i = 0; i < n; ++i)
        ASSERT_UNDER_CONSTEXPR_CONTEXT(isASCII(characters[i]));
#else
    UNUSED_PARAM(n);
#endif
    return ASCIILiteral::fromLiteralUnsafe(characters);
}

} // inline StringLiterals

} // namespace PurCWTF

using namespace PurCWTF::StringLiterals;
using PurCWTF::ASCIILiteral;
