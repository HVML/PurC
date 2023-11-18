/*
 * Copyright (C) 2011-2020 Apple Inc. All rights reserved.
 * Copyright (C) 2020 Beijing FMSoft Technologies Co., Ltd.
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
#include "WebCoreArgumentCoders.h"

#include "DataReference.h"
#include "SharedBufferDataReference.h"
#include "SharedBuffer.h"
#include "ResourceError.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "SecurityOrigin.h"
#include "CertificateInfo.h"
#include "NetworkProxySettings.h"

#include <wtf/URL.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringHash.h>



// FIXME: Seems like we could use std::tuple to cut down the code below a lot!

namespace IPC {
using namespace PurCFetcher;

static void encodeSharedBuffer(Encoder& encoder, const SharedBuffer* buffer)
{
    uint64_t bufferSize = buffer ? buffer->size() : 0;
    encoder << bufferSize;
    if (!bufferSize)
        return;

#if USE(UNIX_DOMAIN_SOCKETS)
    // Do not use shared memory for SharedBuffer encoding in Unix, because it's easy to reach the
    // maximum number of file descriptors open per process when sending large data in small chunks
    // over the IPC. ConnectionUnix.cpp already uses shared memory to send any IPC message that is
    // too large. See https://bugs.webkit.org/show_bug.cgi?id=208571.
    for (const auto& element : *buffer)
        encoder.encodeFixedLengthData(reinterpret_cast<const uint8_t*>(element.segment->data()), element.segment->size(), 1);
#else
    SharedMemory::Handle handle;
    auto sharedMemoryBuffer = SharedMemory::allocate(buffer->size());
    memcpy(sharedMemoryBuffer->data(), buffer->data(), buffer->size());
    sharedMemoryBuffer->createHandle(handle, SharedMemory::Protection::ReadOnly);
    encoder << handle;
#endif
}

static WARN_UNUSED_RETURN bool decodeSharedBuffer(Decoder& decoder, RefPtr<SharedBuffer>& buffer)
{
    uint64_t bufferSize = 0;
    if (!decoder.decode(bufferSize))
        return false;

    if (!bufferSize)
        return true;

#if USE(UNIX_DOMAIN_SOCKETS)
    if (!decoder.bufferIsLargeEnoughToContain<uint8_t>(bufferSize))
        return false;

    Vector<uint8_t> data;
    data.grow(bufferSize);
    if (!decoder.decodeFixedLengthData(data.data(), data.size(), 1))
        return false;

    buffer = SharedBuffer::create(WTFMove(data));
#else
    SharedMemory::Handle handle;
    if (!decoder.decode(handle))
        return false;

    // SharedMemory::Handle::size() is rounded up to the nearest page.
    if (bufferSize > handle.size())
        return false;

    auto sharedMemoryBuffer = SharedMemory::map(handle, SharedMemory::Protection::ReadOnly);
    if (!sharedMemoryBuffer)
        return false;

    buffer = SharedBuffer::create(static_cast<unsigned char*>(sharedMemoryBuffer->data()), bufferSize);
#endif

    return true;
}



void ArgumentCoder<RefPtr<PurCFetcher::SharedBuffer>>::encode(Encoder& encoder, const RefPtr<PurCFetcher::SharedBuffer>& buffer)
{
    encodeSharedBuffer(encoder, buffer.get());
}

std::optional<RefPtr<SharedBuffer>> ArgumentCoder<RefPtr<PurCFetcher::SharedBuffer>>::decode(Decoder& decoder)
{
    RefPtr<SharedBuffer> buffer;
    if (!decodeSharedBuffer(decoder, buffer))
        return std::nullopt;

    return buffer;
}

void ArgumentCoder<Ref<PurCFetcher::SharedBuffer>>::encode(Encoder& encoder, const Ref<PurCFetcher::SharedBuffer>& buffer)
{
    encodeSharedBuffer(encoder, buffer.ptr());
}

std::optional<Ref<SharedBuffer>> ArgumentCoder<Ref<PurCFetcher::SharedBuffer>>::decode(Decoder& decoder)
{
    RefPtr<SharedBuffer> buffer;
    if (!decodeSharedBuffer(decoder, buffer) || !buffer)
        return std::nullopt;

    return buffer.releaseNonNull();
}

void ArgumentCoder<ResourceError>::encode(Encoder& encoder, const ResourceError& resourceError)
{
    encoder << resourceError.type();
    if (resourceError.type() == ResourceError::Type::Null)
        return;
    encodePlatformData(encoder, resourceError);
}

bool ArgumentCoder<ResourceError>::decode(Decoder& decoder, ResourceError& resourceError)
{
    ResourceError::Type type;
    if (!decoder.decode(type))
        return false;

    if (type == ResourceError::Type::Null) {
        resourceError = { };
        return true;
    }

    if (!decodePlatformData(decoder, resourceError))
        return false;

    resourceError.setType(type);
    return true;
}

void ArgumentCoder<ResourceRequest>::encode(Encoder& encoder, const ResourceRequest& resourceRequest)
{
    encoder << resourceRequest.cachePartition();
    encoder << resourceRequest.hiddenFromInspector();

    if (resourceRequest.encodingRequiresPlatformData()) {
        encoder << true;
        encodePlatformData(encoder, resourceRequest);
        return;
    }
    encoder << false;
    resourceRequest.encodeWithoutPlatformData(encoder);
}

bool ArgumentCoder<ResourceRequest>::decode(Decoder& decoder, ResourceRequest& resourceRequest)
{
    String cachePartition;
    if (!decoder.decode(cachePartition))
        return false;
    resourceRequest.setCachePartition(cachePartition);

    bool isHiddenFromInspector;
    if (!decoder.decode(isHiddenFromInspector))
        return false;
    resourceRequest.setHiddenFromInspector(isHiddenFromInspector);

    bool hasPlatformData;
    if (!decoder.decode(hasPlatformData))
        return false;
    if (hasPlatformData)
        return decodePlatformData(decoder, resourceRequest);

    return resourceRequest.decodeWithoutPlatformData(decoder);
}

void ArgumentCoder<Vector<RefPtr<SecurityOrigin>>>::encode(Encoder& encoder, const Vector<RefPtr<SecurityOrigin>>& origins)
{
    encoder << static_cast<uint64_t>(origins.size());
    for (auto& origin : origins)
        encoder << *origin;
}
    
bool ArgumentCoder<Vector<RefPtr<SecurityOrigin>>>::decode(Decoder& decoder, Vector<RefPtr<SecurityOrigin>>& origins)
{
    uint64_t dataSize;
    if (!decoder.decode(dataSize))
        return false;

    for (uint64_t i = 0; i < dataSize; ++i) {
        auto decodedOriginRefPtr = SecurityOrigin::decode(decoder);
        if (!decodedOriginRefPtr)
            return false;
        origins.append(decodedOriginRefPtr.releaseNonNull());
    }
    origins.shrinkToFit();

    return true;
}

void ArgumentCoder<ResourceRequest>::encodePlatformData(Encoder& encoder, const ResourceRequest& resourceRequest)
{
    resourceRequest.encodeWithPlatformData(encoder);
}

bool ArgumentCoder<ResourceRequest>::decodePlatformData(Decoder& decoder, ResourceRequest& resourceRequest)
{
    return resourceRequest.decodeWithPlatformData(decoder);
}

void ArgumentCoder<CertificateInfo>::encode(Encoder& encoder, const CertificateInfo& certificateInfo)
{
    auto* certificate = certificateInfo.certificate();
    if (!certificate) {
        encoder << 0;
        return;
    }

    Vector<GRefPtr<GByteArray>> certificatesDataList;
    for (; certificate; certificate = g_tls_certificate_get_issuer(certificate)) {
        GByteArray* certificateData = nullptr;
        g_object_get(G_OBJECT(certificate), "certificate", &certificateData, nullptr);

        if (!certificateData) {
            certificatesDataList.clear();
            break;
        }

        certificatesDataList.append(adoptGRef(certificateData));
    }

    encoder << static_cast<uint32_t>(certificatesDataList.size());

    if (certificatesDataList.isEmpty())
        return;

    // Encode starting from the root certificate.
    for (size_t i = certificatesDataList.size(); i > 0; --i) {
        GByteArray* certificate = certificatesDataList[i - 1].get();
        encoder.encodeVariableLengthByteArray(IPC::DataReference(certificate->data, certificate->len));
    }

    encoder << static_cast<uint32_t>(certificateInfo.tlsErrors());
}

bool ArgumentCoder<CertificateInfo>::decode(Decoder& decoder, CertificateInfo& certificateInfo)
{
    uint32_t chainLength;
    if (!decoder.decode(chainLength))
        return false;

    if (!chainLength)
        return true;

    GType certificateType = g_tls_backend_get_certificate_type(g_tls_backend_get_default());
    GRefPtr<GTlsCertificate> certificate;
    for (uint32_t i = 0; i < chainLength; i++) {
        IPC::DataReference certificateDataReference;
        if (!decoder.decodeVariableLengthByteArray(certificateDataReference))
            return false;

        GByteArray* certificateData = g_byte_array_sized_new(certificateDataReference.size());
        GRefPtr<GByteArray> certificateBytes = adoptGRef(g_byte_array_append(certificateData, certificateDataReference.data(), certificateDataReference.size()));

        certificate = adoptGRef(G_TLS_CERTIFICATE(g_initable_new(
            certificateType, nullptr, nullptr, "certificate", certificateBytes.get(), "issuer", certificate.get(), nullptr)));
    }

    uint32_t tlsErrors;
    if (!decoder.decode(tlsErrors))
        return false;

    certificateInfo.setCertificate(certificate.get());
    certificateInfo.setTLSErrors(static_cast<GTlsCertificateFlags>(tlsErrors));

    return true;
}

void ArgumentCoder<ResourceError>::encodePlatformData(Encoder& encoder, const ResourceError& resourceError)
{
    encoder << resourceError.domain();
    encoder << resourceError.errorCode();
    encoder << resourceError.failingURL().string();
    encoder << resourceError.localizedDescription();

    encoder << CertificateInfo(resourceError);
}

bool ArgumentCoder<ResourceError>::decodePlatformData(Decoder& decoder, ResourceError& resourceError)
{
    String domain;
    if (!decoder.decode(domain))
        return false;

    int errorCode;
    if (!decoder.decode(errorCode))
        return false;

    String failingURL;
    if (!decoder.decode(failingURL))
        return false;

    String localizedDescription;
    if (!decoder.decode(localizedDescription))
        return false;

    resourceError = ResourceError(domain, errorCode, URL(URL(), failingURL), localizedDescription);

    CertificateInfo certificateInfo;
    if (!decoder.decode(certificateInfo))
        return false;

    resourceError.setCertificate(certificateInfo.certificate());
    resourceError.setTLSErrors(certificateInfo.tlsErrors());
    return true;
}

void ArgumentCoder<NetworkProxySettings>::encode(Encoder& encoder, const NetworkProxySettings& settings)
{
    ASSERT(!settings.isEmpty());
    encoder << settings.mode;
    if (settings.mode != NetworkProxySettings::Mode::Custom)
        return;

    encoder << settings.defaultProxyURL;
    uint32_t ignoreHostsCount = settings.ignoreHosts ? g_strv_length(settings.ignoreHosts.get()) : 0;
    encoder << ignoreHostsCount;
    if (ignoreHostsCount) {
        for (uint32_t i = 0; settings.ignoreHosts.get()[i]; ++i)
            encoder << CString(settings.ignoreHosts.get()[i]);
    }
    encoder << settings.proxyMap;
}

bool ArgumentCoder<NetworkProxySettings>::decode(Decoder& decoder, NetworkProxySettings& settings)
{
    if (!decoder.decode(settings.mode))
        return false;

    if (settings.mode != NetworkProxySettings::Mode::Custom)
        return true;

    if (!decoder.decode(settings.defaultProxyURL))
        return false;

    uint32_t ignoreHostsCount;
    if (!decoder.decode(ignoreHostsCount))
        return false;

    if (ignoreHostsCount) {
        settings.ignoreHosts.reset(g_new0(char*, ignoreHostsCount + 1));
        for (uint32_t i = 0; i < ignoreHostsCount; ++i) {
            CString host;
            if (!decoder.decode(host))
                return false;

            settings.ignoreHosts.get()[i] = g_strdup(host.data());
        }
    }

    if (!decoder.decode(settings.proxyMap))
        return false;

    return !settings.isEmpty();
}

void ArgumentCoder<ProtectionSpace>::encodePlatformData(Encoder&, const ProtectionSpace&)
{
    ASSERT_NOT_REACHED();
}

bool ArgumentCoder<ProtectionSpace>::decodePlatformData(Decoder&, ProtectionSpace&)
{
    ASSERT_NOT_REACHED();
    return false;
}

void ArgumentCoder<Credential>::encodePlatformData(Encoder&, const Credential&)
{
    ASSERT_NOT_REACHED();
}

bool ArgumentCoder<Credential>::decodePlatformData(Decoder&, Credential&)
{
    ASSERT_NOT_REACHED();
    return false;
}

} // namespace IPC
