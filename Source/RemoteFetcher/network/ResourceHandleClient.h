/*
 * Copyright (C) 2006-2017 Apple Inc. All rights reserved.
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

#include <wtf/CompletionHandler.h>
#include <wtf/Forward.h>
#include <wtf/Ref.h>

namespace PurCFetcher {
class AuthenticationChallenge;
class Credential;
class ProtectionSpace;
class ResourceHandle;
class ResourceError;
class ResourceRequest;
class ResourceResponse;
class SharedBuffer;

enum CacheStoragePolicy {
    StorageAllowed,
    StorageAllowedInMemoryOnly,
    StorageNotAllowed
};

class ResourceHandleClient {
public:
    PURCFETCHER_EXPORT ResourceHandleClient();
    PURCFETCHER_EXPORT virtual ~ResourceHandleClient();

    virtual void didSendData(ResourceHandle*, unsigned long long /*bytesSent*/, unsigned long long /*totalBytesToBeSent*/) { }

    virtual void didReceiveData(ResourceHandle*, const uint8_t*, unsigned, int /*encodedDataLength*/) { }
    PURCFETCHER_EXPORT virtual void didReceiveBuffer(ResourceHandle*, Ref<SharedBuffer>&&, int encodedDataLength);

    virtual void didFinishLoading(ResourceHandle*) { }
    virtual void didFail(ResourceHandle*, const ResourceError&) { }
    virtual void wasBlocked(ResourceHandle*) { }
    virtual void cannotShowURL(ResourceHandle*) { }

    virtual bool loadingSynchronousXHR() { return false; }

    PURCFETCHER_EXPORT virtual void willSendRequestAsync(ResourceHandle*, ResourceRequest&&, ResourceResponse&&, CompletionHandler<void(ResourceRequest&&)>&&) = 0;

    PURCFETCHER_EXPORT virtual void didReceiveResponseAsync(ResourceHandle*, ResourceResponse&&, CompletionHandler<void()>&&) = 0;

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    PURCFETCHER_EXPORT virtual void canAuthenticateAgainstProtectionSpaceAsync(ResourceHandle*, const ProtectionSpace&, CompletionHandler<void(bool)>&&) = 0;
#endif

    virtual bool shouldUseCredentialStorage(ResourceHandle*) { return false; }
    virtual void didReceiveAuthenticationChallenge(ResourceHandle*, const AuthenticationChallenge&) { }
    virtual void receivedCancellation(ResourceHandle*, const AuthenticationChallenge&) { }
};

}
