/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
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


#include <wtf/HashMap.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/ThreadSpecific.h>
#include <wtf/text/AtomString.h>
#include <wtf/text/TextBreakIterator.h>
#if ENABLE(ICU)
#include <unicode/uloc.h>
#include <wtf/unicode/icu/ICUHelpers.h>
#endif
#include <wtf/text/UChar.h>

namespace PurCWTF {

class LineBreakIteratorPool {
    WTF_MAKE_NONCOPYABLE(LineBreakIteratorPool);
    WTF_MAKE_FAST_ALLOCATED;
public:
    LineBreakIteratorPool() = default;

    static LineBreakIteratorPool& sharedPool()
    {
        static NeverDestroyed<PurCWTF::ThreadSpecific<LineBreakIteratorPool>> pool;
        return *pool.get();
    }

    static AtomString makeLocaleWithBreakKeyword(const AtomString& locale, LineBreakIteratorMode mode)
    {
        // The uloc functions model locales as char*, so we have to downconvert our AtomString.
        auto utf8Locale = locale.string().utf8();
        if (!utf8Locale.length())
            return locale;
        Vector<char> scratchBuffer(utf8Locale.length() + 11, 0);
        memcpy(scratchBuffer.data(), utf8Locale.data(), utf8Locale.length());

        const char* keywordValue = nullptr;
        switch (mode) {
        case LineBreakIteratorMode::Default:
            // nullptr will cause any existing values to be removed.
            break;
        case LineBreakIteratorMode::Loose:
            keywordValue = "loose";
            break;
        case LineBreakIteratorMode::Normal:
            keywordValue = "normal";
            break;
        case LineBreakIteratorMode::Strict:
            keywordValue = "strict";
            break;
        }

        UErrorCode status = U_ZERO_ERROR;
        int32_t lengthNeeded = uloc_setKeywordValue("lb", keywordValue, scratchBuffer.data(), scratchBuffer.size(), &status);
        if (U_SUCCESS(status))
            return AtomString::fromUTF8(scratchBuffer.data(), lengthNeeded);
        if (needsToGrowToProduceBuffer(status)) {
            scratchBuffer.grow(lengthNeeded + 1);
            memset(scratchBuffer.data() + utf8Locale.length(), 0, scratchBuffer.size() - utf8Locale.length());
            status = U_ZERO_ERROR;
            int32_t lengthNeeded2 = uloc_setKeywordValue("lb", keywordValue, scratchBuffer.data(), scratchBuffer.size(), &status);
            if (!U_SUCCESS(status) || lengthNeeded != lengthNeeded2)
                return locale;
            return AtomString::fromUTF8(scratchBuffer.data(), lengthNeeded);
        }
        return locale;
    }

    UBreakIterator* take(const AtomString& locale, LineBreakIteratorMode mode)
    {
        auto localeWithOptionalBreakKeyword = makeLocaleWithBreakKeyword(locale, mode);

        UBreakIterator* iterator = nullptr;
        for (size_t i = 0; i < m_pool.size(); ++i) {
            if (m_pool[i].first == localeWithOptionalBreakKeyword) {
                iterator = m_pool[i].second;
                m_pool.remove(i);
                break;
            }
        }

        if (!iterator) {
            iterator = openLineBreakIterator(localeWithOptionalBreakKeyword);
            if (!iterator)
                return nullptr;
        }

        ASSERT(!m_vendedIterators.contains(iterator));
        m_vendedIterators.add(iterator, localeWithOptionalBreakKeyword);
        return iterator;
    }

    void put(UBreakIterator* iterator)
    {
        ASSERT(m_vendedIterators.contains(iterator));
        if (m_pool.size() == capacity) {
            closeLineBreakIterator(m_pool[0].second);
            m_pool.remove(0);
        }
        m_pool.uncheckedAppend({ m_vendedIterators.take(iterator), iterator });
    }

private:
    static constexpr size_t capacity = 4;

    Vector<std::pair<AtomString, UBreakIterator*>, capacity> m_pool;
    HashMap<UBreakIterator*, AtomString> m_vendedIterators;

    friend PurCWTF::ThreadSpecific<LineBreakIteratorPool>::operator LineBreakIteratorPool*();
};

}
