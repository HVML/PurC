/*
 * Copyright (C) 2014-2017 Apple Inc. All rights reserved.
 * Copyright (C) 2019 ~ 2020, Beijing FMSoft Technologies Co., Ltd.
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
#include "NetworkCache.h"

#include "AsyncRevalidation.h"
#include "Logging.h"
#include "NetworkCacheSpeculativeLoad.h"
#include "NetworkCacheSpeculativeLoadManager.h"
#include "NetworkCacheStorage.h"
#include "NetworkProcess.h"
#include "NetworkSession.h"
#include "CacheValidation.h"
#include "HTTPHeaderNames.h"
#include "NetworkStorageSession.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "SharedBuffer.h"
#include <wtf/FileSystem.h>
#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/RunLoop.h>
#include <wtf/text/StringBuilder.h>

namespace PurCFetcher {
namespace NetworkCache {

using namespace FileSystem;

static const AtomString& resourceType()
{
    ASSERT(PurCWTF::RunLoop::isMain());
    static NeverDestroyed<const AtomString> resource("Resource", AtomString::ConstructFromLiteral);
    return resource;
}

static size_t computeCapacity(CacheModel cacheModel, const String& cachePath)
{
    unsigned urlCacheMemoryCapacity = 0;
    uint64_t urlCacheDiskCapacity = 0;
    if (auto diskFreeSize = FileSystem::volumeFreeSpace(cachePath)) {
        // As a fudge factor, use 1000 instead of 1024, in case the reported byte
        // count doesn't align exactly to a megabyte boundary.
        *diskFreeSize /= KB * 1000;
        calculateURLCacheSizes(cacheModel, *diskFreeSize, urlCacheMemoryCapacity, urlCacheDiskCapacity);
    }
    return urlCacheDiskCapacity;
}

RefPtr<Cache> Cache::open(NetworkProcess& networkProcess, const String& cachePath, OptionSet<CacheOption> options, PAL::SessionID sessionID)
{
    if (!FileSystem::makeAllDirectories(cachePath))
        return nullptr;

    auto capacity = computeCapacity(networkProcess.cacheModel(), cachePath);
    auto storage = Storage::open(cachePath, options.contains(CacheOption::TestingMode) ? Storage::Mode::AvoidRandomness : Storage::Mode::Normal, capacity);

    LOG(NetworkCache, "(NetworkProcess) opened cache storage, success %d", !!storage);

    if (!storage)
        return nullptr;

    return adoptRef(*new Cache(networkProcess, cachePath, storage.releaseNonNull(), options, sessionID));
}

static void dumpFileChanged(Cache* cache)
{
    cache->dumpContentsToFile();
}

Cache::Cache(NetworkProcess& networkProcess, const String& storageDirectory, Ref<Storage>&& storage, OptionSet<CacheOption> options, PAL::SessionID sessionID)
    : m_storage(WTFMove(storage))
    , m_networkProcess(networkProcess)
    , m_sessionID(sessionID)
    , m_storageDirectory(storageDirectory)
{
    if (options.contains(CacheOption::RegisterNotify)) {
        // Triggers with "touch $cachePath/dump".
        CString dumpFilePath = fileSystemRepresentation(pathByAppendingComponent(m_storage->basePathIsolatedCopy(), "dump"));
        GRefPtr<GFile> dumpFile = adoptGRef(g_file_new_for_path(dumpFilePath.data()));
        GFileMonitor* monitor = g_file_monitor_file(dumpFile.get(), G_FILE_MONITOR_NONE, nullptr, nullptr);
        g_signal_connect_swapped(monitor, "changed", G_CALLBACK(dumpFileChanged), this);
    }
}

Cache::~Cache()
{
}

size_t Cache::capacity() const
{
    return m_storage->capacity();
}

void Cache::updateCapacity()
{
    auto newCapacity = computeCapacity(m_networkProcess->cacheModel(), m_storage->basePathIsolatedCopy());
    m_storage->setCapacity(newCapacity);
}

Key Cache::makeCacheKey(const PurCFetcher::ResourceRequest& request)
{
    // FIXME: This implements minimal Range header disk cache support. We don't parse
    // ranges so only the same exact range request will be served from the cache.
    String range = request.httpHeaderField(PurCFetcher::HTTPHeaderName::Range);
    return { request.cachePartition(), resourceType(), range, request.url().string(), m_storage->salt() };
}

static bool cachePolicyAllowsExpired(PurCFetcher::ResourceRequestCachePolicy policy)
{
    switch (policy) {
    case PurCFetcher::ResourceRequestCachePolicy::ReturnCacheDataElseLoad:
    case PurCFetcher::ResourceRequestCachePolicy::ReturnCacheDataDontLoad:
        return true;
    case PurCFetcher::ResourceRequestCachePolicy::UseProtocolCachePolicy:
    case PurCFetcher::ResourceRequestCachePolicy::ReloadIgnoringCacheData:
    case PurCFetcher::ResourceRequestCachePolicy::RefreshAnyCacheData:
        return false;
    case PurCFetcher::ResourceRequestCachePolicy::DoNotUseAnyCache:
        ASSERT_NOT_REACHED();
        return false;
    }
    return false;
}

static UseDecision responseNeedsRevalidation(NetworkSession& networkSession, const PurCFetcher::ResourceResponse& response, WallTime timestamp, std::optional<Seconds> maxStale)
{
    UNUSED_PARAM(networkSession);
    if (response.cacheControlContainsNoCache())
        return UseDecision::Validate;

    auto age = PurCFetcher::computeCurrentAge(response, timestamp);
    auto lifetime = PurCFetcher::computeFreshnessLifetimeForHTTPFamily(response, timestamp);

    auto maximumStaleness = maxStale ? maxStale.value() : 0_ms;
    bool hasExpired = age - lifetime > maximumStaleness;
#if ENABLE(NETWORK_CACHE_STALE_WHILE_REVALIDATE)
    if (hasExpired && !maxStale && networkSession.isStaleWhileRevalidateEnabled()) {
        auto responseMaxStaleness = response.cacheControlStaleWhileRevalidate();
        maximumStaleness += responseMaxStaleness ? responseMaxStaleness.value() : 0_ms;
        bool inResponseStaleness = age - lifetime < maximumStaleness;
        if (inResponseStaleness)
            return UseDecision::AsyncRevalidate;
    }
#endif

    if (hasExpired) {
#ifndef LOG_DISABLED
        LOG(NetworkCache, "(NetworkProcess) needsRevalidation hasExpired age=%f lifetime=%f max-staleness=%f", age, lifetime, maximumStaleness);
#endif
        return UseDecision::Validate;
    }

    return UseDecision::Use;
}

static UseDecision responseNeedsRevalidation(NetworkSession& networkSession, const PurCFetcher::ResourceResponse& response, const PurCFetcher::ResourceRequest& request, WallTime timestamp)
{
    auto requestDirectives = PurCFetcher::parseCacheControlDirectives(request.httpHeaderFields());
    if (requestDirectives.noCache)
        return UseDecision::Validate;
    // For requests we ignore max-age values other than zero.
    if (requestDirectives.maxAge && requestDirectives.maxAge.value() == 0_ms)
        return UseDecision::Validate;

    return responseNeedsRevalidation(networkSession, response, timestamp, requestDirectives.maxStale);
}

static UseDecision makeUseDecision(NetworkProcess& networkProcess, const PAL::SessionID& sessionID, const Entry& entry, const PurCFetcher::ResourceRequest& request)
{
    // The request is conditional so we force revalidation from the network. We merely check the disk cache
    // so we can update the cache entry.
    if (request.isConditional() && !entry.redirectRequest())
        return UseDecision::Validate;

    if (!PurCFetcher::verifyVaryingRequestHeaders(networkProcess.storageSession(sessionID), entry.varyingRequestHeaders(), request))
        return UseDecision::NoDueToVaryingHeaderMismatch;

    // We never revalidate in the case of a history navigation.
    if (cachePolicyAllowsExpired(request.cachePolicy()))
        return UseDecision::Use;

    auto decision = responseNeedsRevalidation(*networkProcess.networkSession(sessionID), entry.response(), request, entry.timeStamp());
    if (decision != UseDecision::Validate)
        return decision;

    if (!entry.response().hasCacheValidatorFields())
        return UseDecision::NoDueToMissingValidatorFields;

    return entry.redirectRequest() ? UseDecision::NoDueToExpiredRedirect : UseDecision::Validate;
}

static RetrieveDecision makeRetrieveDecision(const PurCFetcher::ResourceRequest& request)
{
    ASSERT(request.cachePolicy() != PurCFetcher::ResourceRequestCachePolicy::DoNotUseAnyCache);

    // FIXME: Support HEAD requests.
    if (request.httpMethod() != "GET")
        return RetrieveDecision::NoDueToHTTPMethod;
    if (request.cachePolicy() == PurCFetcher::ResourceRequestCachePolicy::ReloadIgnoringCacheData && !request.isConditional())
        return RetrieveDecision::NoDueToReloadIgnoringCache;

    return RetrieveDecision::Yes;
}

static bool isMediaMIMEType(const String& type)
{
    return startsWithLettersIgnoringASCIICase(type, "video/") || startsWithLettersIgnoringASCIICase(type, "audio/");
}

static StoreDecision makeStoreDecision(const PurCFetcher::ResourceRequest& originalRequest, const PurCFetcher::ResourceResponse& response, size_t bodySize)
{
    UNUSED_PARAM(bodySize);
    if (!originalRequest.url().protocolIsInHTTPFamily() || !response.isInHTTPFamily())
        return StoreDecision::NoDueToProtocol;

    if (originalRequest.httpMethod() != "GET")
        return StoreDecision::NoDueToHTTPMethod;

    auto requestDirectives = PurCFetcher::parseCacheControlDirectives(originalRequest.httpHeaderFields());
    if (requestDirectives.noStore)
        return StoreDecision::NoDueToNoStoreRequest;

    if (response.cacheControlContainsNoStore())
        return StoreDecision::NoDueToNoStoreResponse;

    if (!PurCFetcher::isStatusCodeCacheableByDefault(response.httpStatusCode())) {
        // http://tools.ietf.org/html/rfc7234#section-4.3.2
        bool hasExpirationHeaders = response.expires() || response.cacheControlMaxAge();
        bool expirationHeadersAllowCaching = PurCFetcher::isStatusCodePotentiallyCacheable(response.httpStatusCode()) && hasExpirationHeaders;
        if (!expirationHeadersAllowCaching)
            return StoreDecision::NoDueToHTTPStatusCode;
    }

    bool isMainResource = originalRequest.requester() == PurCFetcher::ResourceRequest::Requester::Main;
    bool storeUnconditionallyForHistoryNavigation = isMainResource || originalRequest.priority() == PurCFetcher::ResourceLoadPriority::VeryHigh;
    if (!storeUnconditionallyForHistoryNavigation) {
        auto now = WallTime::now();
        Seconds allowedStale { 0_ms };
#if ENABLE(NETWORK_CACHE_STALE_WHILE_REVALIDATE)
        if (auto value = response.cacheControlStaleWhileRevalidate())
            allowedStale = value.value();
#endif
        bool hasNonZeroLifetime = !response.cacheControlContainsNoCache() && (PurCFetcher::computeFreshnessLifetimeForHTTPFamily(response, now) > 0_ms || allowedStale > 0_ms);
        bool possiblyReusable = response.hasCacheValidatorFields() || hasNonZeroLifetime;
        if (!possiblyReusable)
            return StoreDecision::NoDueToUnlikelyToReuse;
    }

    // Media loaded via XHR is likely being used for MSE streaming (YouTube and Netflix for example).
    // Streaming media fills the cache quickly and is unlikely to be reused.
    // FIXME: We should introduce a separate media cache partition that doesn't affect other resources.
    // FIXME: We should also make sure make the MSE paths are copy-free so we can use mapped buffers from disk effectively.
    auto requester = originalRequest.requester();
    bool isDefinitelyStreamingMedia = requester == PurCFetcher::ResourceRequest::Requester::Media;
    bool isLikelyStreamingMedia = requester == PurCFetcher::ResourceRequest::Requester::XHR && isMediaMIMEType(response.mimeType());
    if (isLikelyStreamingMedia || isDefinitelyStreamingMedia)
        return StoreDecision::NoDueToStreamingMedia;

    return StoreDecision::Yes;
}

#if ENABLE(NETWORK_CACHE_STALE_WHILE_REVALIDATE)
void Cache::startAsyncRevalidationIfNeeded(const PurCFetcher::ResourceRequest& request, const NetworkCache::Key& key, std::unique_ptr<Entry>&& entry, const GlobalFrameID& frameID, std::optional<NavigatingToAppBoundDomain> isNavigatingToAppBoundDomain)
{
    m_pendingAsyncRevalidations.ensure(key, [&] {
        auto addResult = m_pendingAsyncRevalidationByPage.ensure(frameID, [] {
            return WeakHashSet<AsyncRevalidation>();
        });
        auto revalidation = makeUnique<AsyncRevalidation>(*this, frameID, request, WTFMove(entry), isNavigatingToAppBoundDomain, [this, key](auto result) {
            ASSERT(m_pendingAsyncRevalidations.contains(key));
            m_pendingAsyncRevalidations.remove(key);
            LOG(NetworkCache, "(NetworkProcess) Async revalidation completed for '%s' with result %d", key.identifier().utf8().data(), static_cast<int>(result));
        });
        addResult.iterator->value.add(*revalidation);
        return revalidation;
    });
}
#endif

void Cache::browsingContextRemoved(WebPageProxyIdentifier webPageProxyID, PurCFetcher::PageIdentifier webPageID, PurCFetcher::FrameIdentifier webFrameID)
{
    UNUSED_PARAM(webPageProxyID);
    UNUSED_PARAM(webPageID);
    UNUSED_PARAM(webFrameID);
#if ENABLE(NETWORK_CACHE_STALE_WHILE_REVALIDATE)
    auto loaders = m_pendingAsyncRevalidationByPage.take({ webPageProxyID, webPageID, webFrameID });
    for (auto& loader : loaders)
        loader.cancel();
#endif
}

void Cache::retrieve(const PurCFetcher::ResourceRequest& request, const GlobalFrameID& frameID, std::optional<NavigatingToAppBoundDomain> isNavigatingToAppBoundDomain, RetrieveCompletionHandler&& completionHandler)
{
    ASSERT(request.url().protocolIsInHTTPFamily());

    LOG(NetworkCache, "(NetworkProcess) retrieving %s priority %d", request.url().string().ascii().data(), static_cast<int>(request.priority()));

    Key storageKey = makeCacheKey(request);
    auto priority = static_cast<unsigned>(request.priority());

    RetrieveInfo info;
    info.startTime = MonotonicTime::now();
    info.priority = priority;

    auto retrieveDecision = makeRetrieveDecision(request);
    if (retrieveDecision != RetrieveDecision::Yes) {
        completeRetrieve(WTFMove(completionHandler), nullptr, info);
        return;
    }

    m_storage->retrieve(storageKey, priority, [this, protectedThis = makeRef(*this), request, completionHandler = WTFMove(completionHandler), info = WTFMove(info), storageKey, networkProcess = makeRef(networkProcess()), sessionID = m_sessionID, frameID, isNavigatingToAppBoundDomain](auto record, auto timings) mutable {
        info.storageTimings = timings;

        if (!record) {
            LOG(NetworkCache, "(NetworkProcess) not found in storage");
            completeRetrieve(WTFMove(completionHandler), nullptr, info);
            return false;
        }

        ASSERT(record->key == storageKey);

        auto entry = Entry::decodeStorageRecord(*record);

        auto useDecision = entry ? makeUseDecision(networkProcess, sessionID, *entry, request) : UseDecision::NoDueToDecodeFailure;
        switch (useDecision) {
        case UseDecision::AsyncRevalidate: {
#if ENABLE(NETWORK_CACHE_STALE_WHILE_REVALIDATE)
            auto entryCopy = makeUnique<Entry>(*entry);
            entryCopy->setNeedsValidation(true);
            startAsyncRevalidationIfNeeded(request, storageKey, WTFMove(entryCopy), frameID, isNavigatingToAppBoundDomain);
#else
            UNUSED_PARAM(frameID);
            UNUSED_PARAM(this);
#endif
            FALLTHROUGH;
        }
        case UseDecision::Use:
            break;
        case UseDecision::Validate:
            entry->setNeedsValidation(true);
            break;
        default:
            entry = nullptr;
        };

#if !LOG_DISABLED
        auto elapsed = MonotonicTime::now() - info.startTime;
        LOG(NetworkCache, "(NetworkProcess) retrieve complete useDecision=%d priority=%d time=%" PRIi64 "ms", static_cast<int>(useDecision), static_cast<int>(request.priority()), elapsed.millisecondsAs<int64_t>());
#endif
        completeRetrieve(WTFMove(completionHandler), WTFMove(entry), info);

        return useDecision != UseDecision::NoDueToDecodeFailure;
    });
}

void Cache::completeRetrieve(RetrieveCompletionHandler&& handler, std::unique_ptr<Entry> entry, RetrieveInfo& info)
{
    info.completionTime = MonotonicTime::now();
    handler(WTFMove(entry), info);
}
    
std::unique_ptr<Entry> Cache::makeEntry(const PurCFetcher::ResourceRequest& request, const PurCFetcher::ResourceResponse& response, RefPtr<PurCFetcher::SharedBuffer>&& responseData)
{
    return makeUnique<Entry>(makeCacheKey(request), response, WTFMove(responseData), PurCFetcher::collectVaryingRequestHeaders(networkProcess().storageSession(m_sessionID), request, response));
}

std::unique_ptr<Entry> Cache::makeRedirectEntry(const PurCFetcher::ResourceRequest& request, const PurCFetcher::ResourceResponse& response, const PurCFetcher::ResourceRequest& redirectRequest)
{
    return makeUnique<Entry>(makeCacheKey(request), response, redirectRequest, PurCFetcher::collectVaryingRequestHeaders(networkProcess().storageSession(m_sessionID), request, response));
}

std::unique_ptr<Entry> Cache::store(const PurCFetcher::ResourceRequest& request, const PurCFetcher::ResourceResponse& response, RefPtr<PurCFetcher::SharedBuffer>&& responseData, Function<void(MappedBody&)>&& completionHandler)
{
    ASSERT(responseData);

    LOG(NetworkCache, "(NetworkProcess) storing %s, partition %s", request.url().string().latin1().data(), makeCacheKey(request).partition().latin1().data());
    //printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ (NetworkProcess cache) storing %s, partition %s\n", request.url().string().latin1().data(), makeCacheKey(request).partition().latin1().data());

    StoreDecision storeDecision = makeStoreDecision(request, response, responseData ? responseData->size() : 0);
    if (storeDecision != StoreDecision::Yes) {
        LOG(NetworkCache, "(NetworkProcess) didn't store, storeDecision=%d", static_cast<int>(storeDecision));
        auto key = makeCacheKey(request);

        auto isSuccessfulRevalidation = response.httpStatusCode() == 304;
        if (!isSuccessfulRevalidation) {
            // Make sure we don't keep a stale entry in the cache.
            remove(key);
        }

        return nullptr;
    }

    auto cacheEntry = makeEntry(request, response, WTFMove(responseData));
    auto record = cacheEntry->encodeAsStorageRecord();

    m_storage->store(record, [protectedThis = makeRef(*this), completionHandler = WTFMove(completionHandler)](const Data& bodyData) mutable {
        UNUSED_PARAM(bodyData);
        MappedBody mappedBody;
#if ENABLE(SHAREABLE_RESOURCE)
        if (auto sharedMemory = bodyData.tryCreateSharedMemory()) {
            mappedBody.shareableResource = ShareableResource::create(sharedMemory.releaseNonNull(), 0, bodyData.size());
            if (!mappedBody.shareableResource) {
                if (completionHandler)
                    completionHandler(mappedBody);
                return;
            }
            mappedBody.shareableResource->createHandle(mappedBody.shareableResourceHandle);
        }
#endif
        if (completionHandler)
            completionHandler(mappedBody);
        LOG(NetworkCache, "(NetworkProcess) stored");
    });

    return cacheEntry;
}

std::unique_ptr<Entry> Cache::storeRedirect(const PurCFetcher::ResourceRequest& request, const PurCFetcher::ResourceResponse& response, const PurCFetcher::ResourceRequest& redirectRequest, std::optional<Seconds> maxAgeCap)
{
    LOG(NetworkCache, "(NetworkProcess) storing redirect %s -> %s", request.url().string().latin1().data(), redirectRequest.url().string().latin1().data());

    StoreDecision storeDecision = makeStoreDecision(request, response, 0);
    if (storeDecision != StoreDecision::Yes) {
        LOG(NetworkCache, "(NetworkProcess) didn't store redirect, storeDecision=%d", static_cast<int>(storeDecision));
        return nullptr;
    }

    auto cacheEntry = makeRedirectEntry(request, response, redirectRequest);

#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (maxAgeCap) {
        LOG(NetworkCache, "(NetworkProcess) capping max age for redirect %s -> %s", request.url().string().latin1().data(), redirectRequest.url().string().latin1().data());
        cacheEntry->capMaxAge(maxAgeCap.value());
    }
#else
    UNUSED_PARAM(maxAgeCap);
#endif

    auto record = cacheEntry->encodeAsStorageRecord();

    m_storage->store(record, nullptr);
    
    return cacheEntry;
}

std::unique_ptr<Entry> Cache::update(const PurCFetcher::ResourceRequest& originalRequest, const Entry& existingEntry, const PurCFetcher::ResourceResponse& validatingResponse)
{
    LOG(NetworkCache, "(NetworkProcess) updating %s", originalRequest.url().string().latin1().data());

    PurCFetcher::ResourceResponse response = existingEntry.response();
    PurCFetcher::updateResponseHeadersAfterRevalidation(response, validatingResponse);

    auto updateEntry = makeUnique<Entry>(existingEntry.key(), response, existingEntry.buffer(), PurCFetcher::collectVaryingRequestHeaders(networkProcess().storageSession(m_sessionID), originalRequest, response));
    auto updateRecord = updateEntry->encodeAsStorageRecord();

    m_storage->store(updateRecord, { });

    return updateEntry;
}

void Cache::remove(const Key& key)
{
    m_storage->remove(key);
}

void Cache::remove(const PurCFetcher::ResourceRequest& request)
{
    remove(makeCacheKey(request));
}

void Cache::remove(const Vector<Key>& keys, Function<void()>&& completionHandler)
{
    m_storage->remove(keys, WTFMove(completionHandler));
}

void Cache::traverse(Function<void(const TraversalEntry*)>&& traverseHandler)
{
    // Protect against clients making excessive traversal requests.
    const unsigned maximumTraverseCount = 3;
    if (m_traverseCount >= maximumTraverseCount) {
        WTFLogAlways("Maximum parallel cache traverse count exceeded. Ignoring traversal request.");

        RunLoop::main().dispatch([traverseHandler = WTFMove(traverseHandler)] () mutable {
            traverseHandler(nullptr);
        });
        return;
    }

    ++m_traverseCount;

    m_storage->traverse(resourceType(), { }, [this, protectedThis = makeRef(*this), traverseHandler = WTFMove(traverseHandler)] (const Storage::Record* record, const Storage::RecordInfo& recordInfo) mutable {
        if (!record) {
            --m_traverseCount;
            traverseHandler(nullptr);
            return;
        }

        auto entry = Entry::decodeStorageRecord(*record);
        if (!entry)
            return;

        TraversalEntry traversalEntry { *entry, recordInfo };
        traverseHandler(&traversalEntry);
    });
}

String Cache::dumpFilePath() const
{
    return pathByAppendingComponent(m_storage->versionPath(), "dump.json");
}

void Cache::dumpContentsToFile()
{
    auto fd = openFile(dumpFilePath(), FileOpenMode::Write);
    if (!isHandleValid(fd))
        return;
    auto prologue = String("{\n\"entries\": [\n").utf8();
    writeToFile(fd, prologue.data(), prologue.length());

    struct Totals {
        unsigned count { 0 };
        double worth { 0 };
        size_t bodySize { 0 };
    };
    Totals totals;
    auto flags = { Storage::TraverseFlag::ComputeWorth, Storage::TraverseFlag::ShareCount };
    size_t capacity = m_storage->capacity();
    m_storage->traverse(resourceType(), flags, [fd, totals, capacity](const Storage::Record* record, const Storage::RecordInfo& info) mutable {
        if (!record) {
            CString writeData = makeString(
                "{}\n"
                "],\n"
                "\"totals\": {\n"
                "\"capacity\": ", capacity, ",\n"
                "\"count\": ", totals.count, ",\n"
                "\"bodySize\": ", totals.bodySize, ",\n"
                "\"averageWorth\": ", totals.count ? totals.worth / totals.count : 0, "\n"
                "}\n}\n"
            ).utf8();
            writeToFile(fd, writeData.data(), writeData.length());
            closeFile(fd);
            return;
        }
        auto entry = Entry::decodeStorageRecord(*record);
        if (!entry)
            return;
        ++totals.count;
        totals.worth += info.worth;
        totals.bodySize += info.bodySize;

        StringBuilder json;
        entry->asJSON(json, info);
        json.appendLiteral(",\n");
        auto writeData = json.toString().utf8();
        writeToFile(fd, writeData.data(), writeData.length());
    });
}

void Cache::deleteDumpFile()
{
    WorkQueue::create("com.apple.PurCFetcher.Cache.delete")->dispatch([path = dumpFilePath().isolatedCopy()] {
        deleteFile(path);
    });
}

void Cache::clear(WallTime modifiedSince, Function<void()>&& completionHandler)
{
    LOG(NetworkCache, "(NetworkProcess) clearing cache");

    String anyType;
    m_storage->clear(anyType, modifiedSince, WTFMove(completionHandler));

    deleteDumpFile();
}

void Cache::clear()
{
    clear(-WallTime::infinity(), nullptr);
}

String Cache::recordsPathIsolatedCopy() const
{
    return m_storage->recordsPathIsolatedCopy();
}

void Cache::retrieveData(const DataKey& dataKey, Function<void(const uint8_t*, size_t)> completionHandler)
{
    //printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ (NetworkProcess cache) retrieveData\n");
    Key key { dataKey, m_storage->salt() };
    m_storage->retrieve(key, 4, [completionHandler = WTFMove(completionHandler)] (auto record, auto) mutable {
        if (!record || !record->body.size()) {
            completionHandler(nullptr, 0);
            return true;
        }
        completionHandler(record->body.data(), record->body.size());
        return true;
    });
}

void Cache::storeData(const DataKey& dataKey, const uint8_t* data, size_t size)
{
    Key key { dataKey, m_storage->salt() };
    Storage::Record record { key, WallTime::now(), { }, Data { data, size }, { } };
    m_storage->store(record, { });
}

}
}
