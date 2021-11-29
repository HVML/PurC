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
#include "SecurityOrigin.h"

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

Optional<RefPtr<SharedBuffer>> ArgumentCoder<RefPtr<PurCFetcher::SharedBuffer>>::decode(Decoder& decoder)
{
    RefPtr<SharedBuffer> buffer;
    if (!decodeSharedBuffer(decoder, buffer))
        return WTF::nullopt;

    return buffer;
}

void ArgumentCoder<Ref<PurCFetcher::SharedBuffer>>::encode(Encoder& encoder, const Ref<PurCFetcher::SharedBuffer>& buffer)
{
    encodeSharedBuffer(encoder, buffer.ptr());
}

Optional<Ref<SharedBuffer>> ArgumentCoder<Ref<PurCFetcher::SharedBuffer>>::decode(Decoder& decoder)
{
    RefPtr<SharedBuffer> buffer;
    if (!decodeSharedBuffer(decoder, buffer) || !buffer)
        return WTF::nullopt;

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

#if USE(SYSTEM_PREVIEW)
    if (resourceRequest.isSystemPreview()) {
        encoder << true;
        encoder << resourceRequest.systemPreviewInfo();
    } else
        encoder << false;
#endif

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

#if USE(SYSTEM_PREVIEW)
    bool isSystemPreview;
    if (!decoder.decode(isSystemPreview))
        return false;

    if (isSystemPreview) {
        SystemPreviewInfo systemPreviewInfo;
        if (!decoder.decode(systemPreviewInfo))
            return false;
        resourceRequest.setSystemPreviewInfo(systemPreviewInfo);
    }
#endif

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

#if 0
void ArgumentCoder<CacheQueryOptions>::encode(Encoder& encoder, const CacheQueryOptions& options)
{
    encoder << options.ignoreSearch;
    encoder << options.ignoreMethod;
    encoder << options.ignoreVary;
    encoder << options.cacheName;
}

bool ArgumentCoder<CacheQueryOptions>::decode(Decoder& decoder, CacheQueryOptions& options)
{
    bool ignoreSearch;
    if (!decoder.decode(ignoreSearch))
        return false;
    bool ignoreMethod;
    if (!decoder.decode(ignoreMethod))
        return false;
    bool ignoreVary;
    if (!decoder.decode(ignoreVary))
        return false;
    String cacheName;
    if (!decoder.decode(cacheName))
        return false;

    options.ignoreSearch = ignoreSearch;
    options.ignoreMethod = ignoreMethod;
    options.ignoreVary = ignoreVary;
    options.cacheName = WTFMove(cacheName);
    return true;
}


void ArgumentCoder<DOMCacheEngine::CacheInfo>::encode(Encoder& encoder, const DOMCacheEngine::CacheInfo& info)
{
    encoder << info.identifier;
    encoder << info.name;
}

auto ArgumentCoder<DOMCacheEngine::CacheInfo>::decode(Decoder& decoder) -> Optional<DOMCacheEngine::CacheInfo>
{
    Optional<uint64_t> identifier;
    decoder >> identifier;
    if (!identifier)
        return WTF::nullopt;
    
    Optional<String> name;
    decoder >> name;
    if (!name)
        return WTF::nullopt;
    
    return {{ WTFMove(*identifier), WTFMove(*name) }};
}

void ArgumentCoder<DOMCacheEngine::Record>::encode(Encoder& encoder, const DOMCacheEngine::Record& record)
{
    encoder << record.identifier;

    encoder << record.requestHeadersGuard;
    encoder << record.request;
    encoder << record.options;
    encoder << record.referrer;

    encoder << record.responseHeadersGuard;
    encoder << record.response;
    encoder << record.updateResponseCounter;
    encoder << record.responseBodySize;

    WTF::switchOn(record.responseBody, [&](const Ref<SharedBuffer>& buffer) {
        encoder << true;
        encodeSharedBuffer(encoder, buffer.ptr());
    }, [&](const Ref<FormData>& formData) {
        encoder << false;
        encoder << true;
        formData->encode(encoder);
    }, [&](const std::nullptr_t&) {
        encoder << false;
        encoder << false;
    });
}

Optional<DOMCacheEngine::Record> ArgumentCoder<DOMCacheEngine::Record>::decode(Decoder& decoder)
{
    uint64_t identifier;
    if (!decoder.decode(identifier))
        return WTF::nullopt;

    FetchHeaders::Guard requestHeadersGuard;
    if (!decoder.decode(requestHeadersGuard))
        return WTF::nullopt;

    PurCFetcher::ResourceRequest request;
    if (!decoder.decode(request))
        return WTF::nullopt;

    Optional<PurCFetcher::FetchOptions> options;
    decoder >> options;
    if (!options)
        return WTF::nullopt;

    String referrer;
    if (!decoder.decode(referrer))
        return WTF::nullopt;

    FetchHeaders::Guard responseHeadersGuard;
    if (!decoder.decode(responseHeadersGuard))
        return WTF::nullopt;

    PurCFetcher::ResourceResponse response;
    if (!decoder.decode(response))
        return WTF::nullopt;

    uint64_t updateResponseCounter;
    if (!decoder.decode(updateResponseCounter))
        return WTF::nullopt;

    uint64_t responseBodySize;
    if (!decoder.decode(responseBodySize))
        return WTF::nullopt;

    PurCFetcher::DOMCacheEngine::ResponseBody responseBody;
    bool hasSharedBufferBody;
    if (!decoder.decode(hasSharedBufferBody))
        return WTF::nullopt;

    if (hasSharedBufferBody) {
        RefPtr<SharedBuffer> buffer;
        if (!decodeSharedBuffer(decoder, buffer))
            return WTF::nullopt;
        if (buffer)
            responseBody = buffer.releaseNonNull();
    } else {
        bool hasFormDataBody;
        if (!decoder.decode(hasFormDataBody))
            return WTF::nullopt;
        if (hasFormDataBody) {
            auto formData = FormData::decode(decoder);
            if (!formData)
                return WTF::nullopt;
            responseBody = formData.releaseNonNull();
        }
    }

    return {{ WTFMove(identifier), WTFMove(updateResponseCounter), WTFMove(requestHeadersGuard), WTFMove(request), WTFMove(options.value()), WTFMove(referrer), WTFMove(responseHeadersGuard), WTFMove(response), WTFMove(responseBody), responseBodySize }};
}


#endif

} // namespace IPC
