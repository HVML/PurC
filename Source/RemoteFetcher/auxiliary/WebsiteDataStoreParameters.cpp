/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#include "config.h"
#include "WebsiteDataStoreParameters.h"

#include "WebCoreArgumentCoders.h"

namespace PurCFetcher {

WebsiteDataStoreParameters::~WebsiteDataStoreParameters()
{
}

void WebsiteDataStoreParameters::encode(IPC::Encoder& encoder) const
{
    encoder << networkSessionParameters;
    encoder << uiProcessCookieStorageIdentifier;
    encoder << cookieStoragePathExtensionHandle;
    encoder << pendingCookies;

    encoder << localStorageDirectory << localStorageDirectoryExtensionHandle;

    encoder << cacheStorageDirectory << cacheStorageDirectoryExtensionHandle;

    encoder << perOriginStorageQuota;
    encoder << perThirdPartyOriginStorageQuota;
}

Optional<WebsiteDataStoreParameters> WebsiteDataStoreParameters::decode(IPC::Decoder& decoder)
{
    WebsiteDataStoreParameters parameters;

    Optional<NetworkSessionCreationParameters> networkSessionParameters;
    decoder >> networkSessionParameters;
    if (!networkSessionParameters)
        return PurCWTF::nullopt;
    parameters.networkSessionParameters = WTFMove(*networkSessionParameters);

    Optional<Vector<uint8_t>> uiProcessCookieStorageIdentifier;
    decoder >> uiProcessCookieStorageIdentifier;
    if (!uiProcessCookieStorageIdentifier)
        return PurCWTF::nullopt;
    parameters.uiProcessCookieStorageIdentifier = WTFMove(*uiProcessCookieStorageIdentifier);

    Optional<SandboxExtension::Handle> cookieStoragePathExtensionHandle;
    decoder >> cookieStoragePathExtensionHandle;
    if (!cookieStoragePathExtensionHandle)
        return PurCWTF::nullopt;
    parameters.cookieStoragePathExtensionHandle = WTFMove(*cookieStoragePathExtensionHandle);

    Optional<Vector<PurCFetcher::Cookie>> pendingCookies;
    decoder >> pendingCookies;
    if (!pendingCookies)
        return PurCWTF::nullopt;
    parameters.pendingCookies = WTFMove(*pendingCookies);

    Optional<String> localStorageDirectory;
    decoder >> localStorageDirectory;
    if (!localStorageDirectory)
        return PurCWTF::nullopt;
    parameters.localStorageDirectory = WTFMove(*localStorageDirectory);

    Optional<SandboxExtension::Handle> localStorageDirectoryExtensionHandle;
    decoder >> localStorageDirectoryExtensionHandle;
    if (!localStorageDirectoryExtensionHandle)
        return PurCWTF::nullopt;
    parameters.localStorageDirectoryExtensionHandle = WTFMove(*localStorageDirectoryExtensionHandle);

    Optional<String> cacheStorageDirectory;
    decoder >> cacheStorageDirectory;
    if (!cacheStorageDirectory)
        return PurCWTF::nullopt;
    parameters.cacheStorageDirectory = WTFMove(*cacheStorageDirectory);

    Optional<SandboxExtension::Handle> cacheStorageDirectoryExtensionHandle;
    decoder >> cacheStorageDirectoryExtensionHandle;
    if (!cacheStorageDirectoryExtensionHandle)
        return PurCWTF::nullopt;
    parameters.cacheStorageDirectoryExtensionHandle = WTFMove(*cacheStorageDirectoryExtensionHandle);

    Optional<uint64_t> perOriginStorageQuota;
    decoder >> perOriginStorageQuota;
    if (!perOriginStorageQuota)
        return PurCWTF::nullopt;
    parameters.perOriginStorageQuota = *perOriginStorageQuota;

    Optional<uint64_t> perThirdPartyOriginStorageQuota;
    decoder >> perThirdPartyOriginStorageQuota;
    if (!perThirdPartyOriginStorageQuota)
        return PurCWTF::nullopt;
    parameters.perThirdPartyOriginStorageQuota = *perThirdPartyOriginStorageQuota;
    
    return parameters;
}

} // namespace PurCFetcher
