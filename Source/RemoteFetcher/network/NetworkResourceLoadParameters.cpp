/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "NetworkResourceLoadParameters.h"

#include "WebCoreArgumentCoders.h"

namespace PurCFetcher {
using namespace PurCFetcher;

void NetworkResourceLoadParameters::encode(IPC::Encoder& encoder) const
{
    encoder << identifier;
    encoder << webPageProxyID;
    encoder << webPageID;
    encoder << webFrameID;
    encoder << parentPID;
    encoder << request;

    encoder << static_cast<bool>(request.httpBody());
    if (request.httpBody()) {
        request.httpBody()->encode(encoder);

        const Vector<FormDataElement>& elements = request.httpBody()->elements();
        size_t fileCount = 0;
        for (size_t i = 0, count = elements.size(); i < count; ++i) {
            if (PurCWTF::holds_alternative<FormDataElement::EncodedFileData>(elements[i].data))
                ++fileCount;
        }

        SandboxExtension::HandleArray requestBodySandboxExtensions;
        requestBodySandboxExtensions.allocate(fileCount);
        size_t extensionIndex = 0;
        for (size_t i = 0, count = elements.size(); i < count; ++i) {
            const FormDataElement& element = elements[i];
            if (auto* fileData = PurCWTF::get_if<FormDataElement::EncodedFileData>(element.data)) {
                const String& path = fileData->filename;
                SandboxExtension::createHandle(path, SandboxExtension::Type::ReadOnly, requestBodySandboxExtensions[extensionIndex++]);
            }
        }
        encoder << requestBodySandboxExtensions;
    }

    if (request.url().isLocalFile()) {
        SandboxExtension::Handle requestSandboxExtension;
#if HAVE(SANDBOX_ISSUE_READ_EXTENSION_TO_PROCESS_BY_AUDIT_TOKEN)
        if (networkProcessAuditToken)
            SandboxExtension::createHandleForReadByAuditToken(request.url().fileSystemPath(), *networkProcessAuditToken, requestSandboxExtension);
        else
            SandboxExtension::createHandle(request.url().fileSystemPath(), SandboxExtension::Type::ReadOnly, requestSandboxExtension);
#else
        SandboxExtension::createHandle(request.url().fileSystemPath(), SandboxExtension::Type::ReadOnly, requestSandboxExtension);
#endif
        encoder << requestSandboxExtension;
    }

    encoder << contentSniffingPolicy;
    encoder << contentEncodingSniffingPolicy;
    encoder << storedCredentialsPolicy;
    encoder << clientCredentialPolicy;
    encoder << shouldPreconnectOnly;
    encoder << shouldClearReferrerOnHTTPSToHTTPRedirect;
    encoder << needsCertificateInfo;
    encoder << isMainFrameNavigation;
    encoder << isMainResourceNavigationForAnyFrame;
    encoder << shouldRelaxThirdPartyCookieBlocking;
    encoder << maximumBufferingTime;

    encoder << static_cast<bool>(sourceOrigin);
    if (sourceOrigin)
        encoder << *sourceOrigin;
    encoder << static_cast<bool>(topOrigin);
    if (sourceOrigin)
        encoder << *topOrigin;
    encoder << options;
    encoder << cspResponseHeaders;
    encoder << originalRequestHeaders;

    encoder << shouldRestrictHTTPResponseAccess;

    encoder << preflightPolicy;

    encoder << shouldEnableCrossOriginResourcePolicy;

    encoder << frameAncestorOrigins;
    encoder << isHTTPSUpgradeEnabled;
    encoder << pageHasResourceLoadClient;
    encoder << parentFrameID;
    encoder << crossOriginAccessControlCheckEnabled;

#if ENABLE(SERVICE_WORKER)
    encoder << serviceWorkersMode;
    encoder << serviceWorkerRegistrationIdentifier;
    encoder << httpHeadersToKeep;
#endif

#if ENABLE(CONTENT_EXTENSIONS)
    encoder << mainDocumentURL;
    encoder << userContentControllerIdentifier;
#endif
    
    encoder << isNavigatingToAppBoundDomain;
}

Optional<NetworkResourceLoadParameters> NetworkResourceLoadParameters::decode(IPC::Decoder& decoder)
{
    NetworkResourceLoadParameters result;

    if (!decoder.decode(result.identifier))
        return PurCWTF::nullopt;
        
    Optional<WebPageProxyIdentifier> webPageProxyID;
    decoder >> webPageProxyID;
    if (!webPageProxyID)
        return PurCWTF::nullopt;
    result.webPageProxyID = *webPageProxyID;

    Optional<PageIdentifier> webPageID;
    decoder >> webPageID;
    if (!webPageID)
        return PurCWTF::nullopt;
    result.webPageID = *webPageID;

    if (!decoder.decode(result.webFrameID))
        return PurCWTF::nullopt;

    if (!decoder.decode(result.parentPID))
        return PurCWTF::nullopt;

    if (!decoder.decode(result.request))
        return PurCWTF::nullopt;

    bool hasHTTPBody;
    if (!decoder.decode(hasHTTPBody))
        return PurCWTF::nullopt;

    if (hasHTTPBody) {
        RefPtr<FormData> formData = FormData::decode(decoder);
        if (!formData)
            return PurCWTF::nullopt;
        result.request.setHTTPBody(WTFMove(formData));

        Optional<SandboxExtension::HandleArray> requestBodySandboxExtensionHandles;
        decoder >> requestBodySandboxExtensionHandles;
        if (!requestBodySandboxExtensionHandles)
            return PurCWTF::nullopt;
        for (size_t i = 0; i < requestBodySandboxExtensionHandles->size(); ++i) {
            if (auto extension = SandboxExtension::create(WTFMove(requestBodySandboxExtensionHandles->at(i))))
                result.requestBodySandboxExtensions.append(WTFMove(extension));
        }
    }

    if (result.request.url().isLocalFile()) {
        Optional<SandboxExtension::Handle> resourceSandboxExtensionHandle;
        decoder >> resourceSandboxExtensionHandle;
        if (!resourceSandboxExtensionHandle)
            return PurCWTF::nullopt;
        result.resourceSandboxExtension = SandboxExtension::create(WTFMove(*resourceSandboxExtensionHandle));
    }

    if (!decoder.decode(result.contentSniffingPolicy))
        return PurCWTF::nullopt;
    if (!decoder.decode(result.contentEncodingSniffingPolicy))
        return PurCWTF::nullopt;
    if (!decoder.decode(result.storedCredentialsPolicy))
        return PurCWTF::nullopt;
    if (!decoder.decode(result.clientCredentialPolicy))
        return PurCWTF::nullopt;
    if (!decoder.decode(result.shouldPreconnectOnly))
        return PurCWTF::nullopt;
    if (!decoder.decode(result.shouldClearReferrerOnHTTPSToHTTPRedirect))
        return PurCWTF::nullopt;
    if (!decoder.decode(result.needsCertificateInfo))
        return PurCWTF::nullopt;
    if (!decoder.decode(result.isMainFrameNavigation))
        return PurCWTF::nullopt;
    if (!decoder.decode(result.isMainResourceNavigationForAnyFrame))
        return PurCWTF::nullopt;
    if (!decoder.decode(result.shouldRelaxThirdPartyCookieBlocking))
        return PurCWTF::nullopt;
    if (!decoder.decode(result.maximumBufferingTime))
        return PurCWTF::nullopt;

    bool hasSourceOrigin;
    if (!decoder.decode(hasSourceOrigin))
        return PurCWTF::nullopt;
    if (hasSourceOrigin) {
        result.sourceOrigin = SecurityOrigin::decode(decoder);
        if (!result.sourceOrigin)
            return PurCWTF::nullopt;
    }

    bool hasTopOrigin;
    if (!decoder.decode(hasTopOrigin))
        return PurCWTF::nullopt;
    if (hasTopOrigin) {
        result.topOrigin = SecurityOrigin::decode(decoder);
        if (!result.topOrigin)
            return PurCWTF::nullopt;
    }

    Optional<FetchOptions> options;
    decoder >> options;
    if (!options)
        return PurCWTF::nullopt;
    result.options = *options;

    if (!decoder.decode(result.cspResponseHeaders))
        return PurCWTF::nullopt;
    if (!decoder.decode(result.originalRequestHeaders))
        return PurCWTF::nullopt;

    Optional<bool> shouldRestrictHTTPResponseAccess;
    decoder >> shouldRestrictHTTPResponseAccess;
    if (!shouldRestrictHTTPResponseAccess)
        return PurCWTF::nullopt;
    result.shouldRestrictHTTPResponseAccess = *shouldRestrictHTTPResponseAccess;

    if (!decoder.decode(result.preflightPolicy))
        return PurCWTF::nullopt;

    Optional<bool> shouldEnableCrossOriginResourcePolicy;
    decoder >> shouldEnableCrossOriginResourcePolicy;
    if (!shouldEnableCrossOriginResourcePolicy)
        return PurCWTF::nullopt;
    result.shouldEnableCrossOriginResourcePolicy = *shouldEnableCrossOriginResourcePolicy;

    if (!decoder.decode(result.frameAncestorOrigins))
        return PurCWTF::nullopt;

    Optional<bool> isHTTPSUpgradeEnabled;
    decoder >> isHTTPSUpgradeEnabled;
    if (!isHTTPSUpgradeEnabled)
        return PurCWTF::nullopt;
    result.isHTTPSUpgradeEnabled = *isHTTPSUpgradeEnabled;

    Optional<bool> pageHasResourceLoadClient;
    decoder >> pageHasResourceLoadClient;
    if (!pageHasResourceLoadClient)
        return PurCWTF::nullopt;
    result.pageHasResourceLoadClient = *pageHasResourceLoadClient;
    
    Optional<Optional<FrameIdentifier>> parentFrameID;
    decoder >> parentFrameID;
    if (!parentFrameID)
        return PurCWTF::nullopt;
    result.parentFrameID = WTFMove(*parentFrameID);

    Optional<bool> crossOriginAccessControlCheckEnabled;
    decoder >> crossOriginAccessControlCheckEnabled;
    if (!crossOriginAccessControlCheckEnabled)
        return PurCWTF::nullopt;
    result.crossOriginAccessControlCheckEnabled = *crossOriginAccessControlCheckEnabled;
    
#if ENABLE(SERVICE_WORKER)
    Optional<ServiceWorkersMode> serviceWorkersMode;
    decoder >> serviceWorkersMode;
    if (!serviceWorkersMode)
        return PurCWTF::nullopt;
    result.serviceWorkersMode = *serviceWorkersMode;

    Optional<Optional<ServiceWorkerRegistrationIdentifier>> serviceWorkerRegistrationIdentifier;
    decoder >> serviceWorkerRegistrationIdentifier;
    if (!serviceWorkerRegistrationIdentifier)
        return PurCWTF::nullopt;
    result.serviceWorkerRegistrationIdentifier = *serviceWorkerRegistrationIdentifier;

    Optional<OptionSet<HTTPHeadersToKeepFromCleaning>> httpHeadersToKeep;
    decoder >> httpHeadersToKeep;
    if (!httpHeadersToKeep)
        return PurCWTF::nullopt;
    result.httpHeadersToKeep = WTFMove(*httpHeadersToKeep);
#endif

#if ENABLE(CONTENT_EXTENSIONS)
    if (!decoder.decode(result.mainDocumentURL))
        return PurCWTF::nullopt;

    Optional<Optional<UserContentControllerIdentifier>> userContentControllerIdentifier;
    decoder >> userContentControllerIdentifier;
    if (!userContentControllerIdentifier)
        return PurCWTF::nullopt;
    result.userContentControllerIdentifier = *userContentControllerIdentifier;
#endif

    Optional<Optional<NavigatingToAppBoundDomain>> isNavigatingToAppBoundDomain;
    decoder >> isNavigatingToAppBoundDomain;
    if (!isNavigatingToAppBoundDomain)
        return PurCWTF::nullopt;
    result.isNavigatingToAppBoundDomain = *isNavigatingToAppBoundDomain;

    return result;
}
    
} // namespace PurCFetcher
