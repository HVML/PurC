/*
 * Copyright (C) 2006 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
 * Copyright (C) 2013, 2016 Apple Inc. All rights reserved.
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

#include <wtf/Vector.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace PurCWTF {

enum Base64EncodePolicy {
    Base64DoNotInsertLFs,
    Base64InsertLFs,
    Base64URLPolicy // No padding, no LFs.
};

enum Base64DecodeOptions {
    Base64Default = 0,
    Base64ValidatePadding = 1 << 0,
    Base64IgnoreSpacesAndNewLines = 1 << 1,
    Base64DiscardVerticalTab = 1 << 2,
};

class SignedOrUnsignedCharVectorAdapter {
public:
    SignedOrUnsignedCharVectorAdapter(Vector<char>& vector)
        : m_isSigned(true)
    {
        m_vector.c = &vector;
    }
    SignedOrUnsignedCharVectorAdapter(Vector<uint8_t>& vector)
        : m_isSigned(false)
    {
        m_vector.u = &vector;
    }

    uint8_t* data()
    {
        if (m_isSigned)
            return reinterpret_cast<uint8_t*>(m_vector.c->data());
        return m_vector.u->data();
    }
    
    size_t size() const
    {
        if (m_isSigned)
            return m_vector.c->size();
        return m_vector.u->size();
    }
    
    void clear()
    {
        if (m_isSigned) {
            m_vector.c->clear();
            return;
        }
        m_vector.u->clear();
    }
    
    void grow(size_t size)
    {
        if (m_isSigned) {
            m_vector.c->grow(size);
            return;
        }
        m_vector.u->grow(size);
    }
    
    void shrink(size_t size)
    {
        if (m_isSigned) {
            m_vector.c->shrink(size);
            return;
        }
        m_vector.u->shrink(size);
    }
    
    uint8_t& operator[](size_t position) { return data()[position]; }

private:
    bool m_isSigned;
    union {
        Vector<char>* c;
        Vector<uint8_t>* u;
    } m_vector;
};

class ConstSignedOrUnsignedCharVectorAdapter {
public:
    ConstSignedOrUnsignedCharVectorAdapter(const Vector<char>& vector)
        : m_isSigned(false)
    {
        m_vector.c = &vector;
    }
    ConstSignedOrUnsignedCharVectorAdapter(const Vector<uint8_t>& vector)
        : m_isSigned(true)
    {
        m_vector.u = &vector;
    }

    const uint8_t* data() const
    {
        if (m_isSigned)
            return reinterpret_cast<const uint8_t*>(m_vector.c->data());
        return m_vector.u->data();
    }
    size_t size() const
    {
        if (m_isSigned)
            return m_vector.c->size();
        return m_vector.u->size();
    }

private:
    bool m_isSigned;
    union {
        const Vector<char>* c;
        const Vector<uint8_t>* u;
    } m_vector;
};

WTF_EXPORT_PRIVATE void base64Encode(const void*, unsigned, Vector<char>&, Base64EncodePolicy = Base64DoNotInsertLFs);
void base64Encode(ConstSignedOrUnsignedCharVectorAdapter, Vector<char>&, Base64EncodePolicy = Base64DoNotInsertLFs);
void base64Encode(const CString&, Vector<char>&, Base64EncodePolicy = Base64DoNotInsertLFs);
WTF_EXPORT_PRIVATE String base64Encode(const void*, unsigned, Base64EncodePolicy = Base64DoNotInsertLFs);
String base64Encode(ConstSignedOrUnsignedCharVectorAdapter, Base64EncodePolicy = Base64DoNotInsertLFs);
String base64Encode(const CString&, Base64EncodePolicy = Base64DoNotInsertLFs);

WTF_EXPORT_PRIVATE bool base64Decode(const String&, SignedOrUnsignedCharVectorAdapter, unsigned options = Base64Default);
WTF_EXPORT_PRIVATE bool base64Decode(StringView, SignedOrUnsignedCharVectorAdapter, unsigned options = Base64Default);
WTF_EXPORT_PRIVATE bool base64Decode(const Vector<char>&, SignedOrUnsignedCharVectorAdapter, unsigned options = Base64Default);
WTF_EXPORT_PRIVATE bool base64Decode(const char*, unsigned, SignedOrUnsignedCharVectorAdapter, unsigned options = Base64Default);

inline void base64Encode(ConstSignedOrUnsignedCharVectorAdapter in, Vector<char>& out, Base64EncodePolicy policy)
{
    base64Encode(in.data(), in.size(), out, policy);
}

inline void base64Encode(const CString& in, Vector<char>& out, Base64EncodePolicy policy)
{
    base64Encode(in.data(), in.length(), out, policy);
}

inline String base64Encode(ConstSignedOrUnsignedCharVectorAdapter in, Base64EncodePolicy policy)
{
    return base64Encode(in.data(), in.size(), policy);
}

inline String base64Encode(const CString& in, Base64EncodePolicy policy)
{
    return base64Encode(in.data(), in.length(), policy);
}

// ======================================================================================
// All the same functions modified for base64url, as defined in RFC 4648.
// This format uses '-' and '_' instead of '+' and '/' respectively.
// ======================================================================================

WTF_EXPORT_PRIVATE void base64URLEncode(const void*, unsigned, Vector<char>&);
void base64URLEncode(ConstSignedOrUnsignedCharVectorAdapter, Vector<char>&);
void base64URLEncode(const CString&, Vector<char>&);

WTF_EXPORT_PRIVATE String base64URLEncode(const void*, unsigned);
String base64URLEncode(ConstSignedOrUnsignedCharVectorAdapter);
String base64URLEncode(const CString&);

WTF_EXPORT_PRIVATE bool base64URLDecode(const String&, SignedOrUnsignedCharVectorAdapter);
WTF_EXPORT_PRIVATE bool base64URLDecode(StringView, SignedOrUnsignedCharVectorAdapter);
WTF_EXPORT_PRIVATE bool base64URLDecode(const Vector<char>&, SignedOrUnsignedCharVectorAdapter);
WTF_EXPORT_PRIVATE bool base64URLDecode(const char*, unsigned, SignedOrUnsignedCharVectorAdapter);

inline void base64URLEncode(ConstSignedOrUnsignedCharVectorAdapter in, Vector<char>& out)
{
    base64URLEncode(in.data(), in.size(), out);
}

inline void base64URLEncode(const CString& in, Vector<char>& out)
{
    base64URLEncode(in.data(), in.length(), out);
}

inline String base64URLEncode(ConstSignedOrUnsignedCharVectorAdapter in)
{
    return base64URLEncode(in.data(), in.size());
}

inline String base64URLEncode(const CString& in)
{
    return base64URLEncode(in.data(), in.length());
}

template<typename CharacterType> static inline bool isBase64OrBase64URLCharacter(CharacterType c)
{
    return isASCIIAlphanumeric(c) || c == '+' || c == '/' || c == '-' || c == '_';
}

} // namespace PurCWTF

using PurCWTF::Base64EncodePolicy;
using PurCWTF::Base64DoNotInsertLFs;
using PurCWTF::Base64InsertLFs;
using PurCWTF::Base64ValidatePadding;
using PurCWTF::Base64IgnoreSpacesAndNewLines;
using PurCWTF::Base64DiscardVerticalTab;
using PurCWTF::base64Encode;
using PurCWTF::base64Decode;
using PurCWTF::base64URLDecode;
using PurCWTF::isBase64OrBase64URLCharacter;
