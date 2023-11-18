/*
 * Copyright (C) 2004-2017 Apple Inc. All rights reserved.
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

#include "AuthenticationClient.h"
#include "StoredCredentialsPolicy.h"
#include <wtf/Box.h>
#include <wtf/MonotonicTime.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/AtomString.h>

#if USE(CURL)
#include "CurlResourceHandleDelegate.h"
#endif

namespace PurCWTF {
class SchedulePair;
template<typename T> class MessageQueue;
}

namespace PurCFetcher {

class AuthenticationChallenge;
class Credential;
class Frame;
class NetworkingContext;
class ProtectionSpace;
class ResourceError;
class ResourceHandleClient;
class ResourceHandleInternal;
class NetworkLoadMetrics;
class ResourceRequest;
class ResourceResponse;
class SharedBuffer;
class SynchronousLoaderMessageQueue;
class Timer;

#if USE(CURL)
class CurlRequest;
class CurlResourceHandleDelegate;
#endif

class ResourceHandle : public RefCounted<ResourceHandle>, public AuthenticationClient {
public:
    PURCFETCHER_EXPORT static RefPtr<ResourceHandle> create(NetworkingContext*, const ResourceRequest&, ResourceHandleClient*, bool defersLoading, bool shouldContentSniff, bool shouldContentEncodingSniff);
    PURCFETCHER_EXPORT static void loadResourceSynchronously(NetworkingContext*, const ResourceRequest&, StoredCredentialsPolicy, ResourceError&, ResourceResponse&, Vector<char>& data);
    PURCFETCHER_EXPORT virtual ~ResourceHandle();

    void didReceiveResponse(ResourceResponse&&, CompletionHandler<void()>&&);

    bool shouldUseCredentialStorage();
    void didReceiveAuthenticationChallenge(const AuthenticationChallenge&);
    void receivedCredential(const AuthenticationChallenge&, const Credential&) override;
    void receivedRequestToContinueWithoutCredential(const AuthenticationChallenge&) override;
    void receivedCancellation(const AuthenticationChallenge&) override;
    void receivedRequestToPerformDefaultHandling(const AuthenticationChallenge&) override;
    void receivedChallengeRejection(const AuthenticationChallenge&) override;

    bool shouldContentSniff() const;
    static bool shouldContentSniffURL(const URL&);

    bool shouldContentEncodingSniff() const;

    PURCFETCHER_EXPORT static void forceContentSniffing();

#if USE(CURL)
    ResourceHandleInternal* getInternal() { return d.get(); }
#endif

#if USE(CURL)
    bool cancelledOrClientless();
    CurlResourceHandleDelegate* delegate();

    void continueAfterDidReceiveResponse();
    void willSendRequest();
    void continueAfterWillSendRequest(ResourceRequest&&);
#endif

    bool hasAuthenticationChallenge() const;
    void clearAuthentication();
    PURCFETCHER_EXPORT virtual void cancel();

    // The client may be 0, in which case no callbacks will be made.
    PURCFETCHER_EXPORT ResourceHandleClient* client() const;
    PURCFETCHER_EXPORT void clearClient();

    PURCFETCHER_EXPORT void setDefersLoading(bool);

    PURCFETCHER_EXPORT ResourceRequest& firstRequest();
    const String& lastHTTPMethod() const;

    void failureTimerFired();

    NetworkingContext* context() const;

    using RefCounted<ResourceHandle>::ref;
    using RefCounted<ResourceHandle>::deref;

    typedef Ref<ResourceHandle> (*BuiltinConstructor)(const ResourceRequest& request, ResourceHandleClient* client);
    static void registerBuiltinConstructor(const AtomString& protocol, BuiltinConstructor);

    typedef void (*BuiltinSynchronousLoader)(NetworkingContext*, const ResourceRequest&, StoredCredentialsPolicy, ResourceError&, ResourceResponse&, Vector<char>& data);
    static void registerBuiltinSynchronousLoader(const AtomString& protocol, BuiltinSynchronousLoader);

protected:
    ResourceHandle(NetworkingContext*, const ResourceRequest&, ResourceHandleClient*, bool defersLoading, bool shouldContentSniff, bool shouldContentEncodingSniff);

private:
    enum FailureType {
        NoFailure,
        BlockedFailure,
        InvalidURLFailure
    };

    void platformSetDefersLoading(bool);

    void platformContinueSynchronousDidReceiveResponse();

    void scheduleFailure(FailureType);

    bool start();
    static void platformLoadResourceSynchronously(NetworkingContext*, const ResourceRequest&, StoredCredentialsPolicy, ResourceError&, ResourceResponse&, Vector<char>& data);

    void refAuthenticationClient() override { ref(); }
    void derefAuthenticationClient() override { deref(); }


#if USE(CURL)
    enum class RequestStatus {
        NewRequest,
        ReusedRequest
    };

    void addCacheValidationHeaders(ResourceRequest&);
    Ref<CurlRequest> createCurlRequest(ResourceRequest&&, RequestStatus = RequestStatus::NewRequest);

    bool shouldRedirectAsGET(const ResourceRequest&, bool crossOrigin);

    std::optional<Credential> getCredential(const ResourceRequest&, bool);
    void restartRequestWithCredential(const ProtectionSpace&, const Credential&);

    void handleDataURL();
#endif

    friend class ResourceHandleInternal;
    std::unique_ptr<ResourceHandleInternal> d;
};

}
