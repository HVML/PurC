/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
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

#include <type_traits>
#include <wtf/StdLibExtras.h>

// Based on http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0052r2.pdf

namespace PurCWTF {

template<typename ExitFunction>
class ScopeExit final {
public:
    template<typename ExitFunctionParameter>
    explicit ScopeExit(ExitFunctionParameter&& exitFunction)
        : m_exitFunction(WTFMove(exitFunction))
    {
    }

    ScopeExit(ScopeExit&& other)
        : m_exitFunction(WTFMove(other.m_exitFunction))
        , m_executeOnDestruction(std::exchange(other.m_executeOnDestruction, false))
    {
    }

    ~ScopeExit()
    {
        if (m_executeOnDestruction)
            m_exitFunction();
    }

    void release()
    {
        m_executeOnDestruction = false;
    }

    ScopeExit(const ScopeExit&) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;
    ScopeExit& operator=(ScopeExit&&) = delete;

private:
    ExitFunction m_exitFunction;
    bool m_executeOnDestruction { true };
};


template<typename ExitFunction> ScopeExit<ExitFunction> makeScopeExit(ExitFunction&&) WARN_UNUSED_RETURN;
template<typename ExitFunction>
ScopeExit<ExitFunction> makeScopeExit(ExitFunction&& exitFunction)
{
    return ScopeExit<ExitFunction>(std::forward<ExitFunction>(exitFunction));
}

}

using PurCWTF::ScopeExit;
using PurCWTF::makeScopeExit;
