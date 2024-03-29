/*
 * Copyright (C) 2003-2018 Apple Inc. All rights reserved.
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

#include "FrameIdentifier.h"
#include "PageIdentifier.h"
#include <wtf/Forward.h>
#include <wtf/text/WTFString.h>

namespace PurCFetcher {

enum class IncludeSecureCookies : bool { No, Yes };
enum class IncludeHttpOnlyCookies : bool { No, Yes };
enum class SecureCookiesAccessed : bool { No, Yes };

class StorageSessionProvider;
struct SameSiteInfo;

class PURCFETCHER_EXPORT CookieJar : public RefCounted<CookieJar> {
public:
    static Ref<CookieJar> create(Ref<StorageSessionProvider>&&);
    

    virtual std::pair<String, SecureCookiesAccessed> cookieRequestHeaderFieldValue(const URL& firstParty, const SameSiteInfo&, const URL&, std::optional<FrameIdentifier>, std::optional<PageIdentifier>, IncludeSecureCookies) const;

    // Cookie Cache.
    virtual void clearCache() { }
    virtual void clearCacheForHost(const String&) { }

    virtual ~CookieJar();
protected:
    CookieJar(Ref<StorageSessionProvider>&&);

private:
    Ref<StorageSessionProvider> m_storageSessionProvider;
};

} // namespace PurCFetcher
