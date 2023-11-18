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

#pragma once

#include "CacheStorageEngineCaches.h"
#include "NetworkCacheData.h"
#include "WebsiteData.h"
#include "ClientOrigin.h"
#include "StorageQuotaManager.h"
#include "SessionID.h"
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/WeakPtr.h>
#include <wtf/WorkQueue.h>

namespace IPC {
class Connection;
}

namespace PurCWTF {
class CallbackAggregator;
};

namespace PurCFetcher {
struct RetrieveRecordsOptions;
}

namespace PurCFetcher {

class NetworkProcess;

namespace CacheStorage {

using CacheIdentifier = uint64_t;
using LockCount = uint64_t;

class Engine : public RefCounted<Engine>, public CanMakeWeakPtr<Engine> {
public:
    ~Engine();

    static void from(NetworkProcess&, PAL::SessionID, Function<void(Engine&)>&&);
    static void destroyEngine(NetworkProcess&, PAL::SessionID);
    static void fetchEntries(NetworkProcess&, PAL::SessionID, bool shouldComputeSize, CompletionHandler<void(Vector<WebsiteData::Entry>)>&&);

    static void open(NetworkProcess&, PAL::SessionID, PurCFetcher::ClientOrigin&&, String&& cacheName, PurCFetcher::DOMCacheEngine::CacheIdentifierCallback&&);
    static void remove(NetworkProcess&, PAL::SessionID, uint64_t cacheIdentifier, PurCFetcher::DOMCacheEngine::CacheIdentifierCallback&&);
    static void retrieveCaches(NetworkProcess&, PAL::SessionID, PurCFetcher::ClientOrigin&&, uint64_t updateCounter, PurCFetcher::DOMCacheEngine::CacheInfosCallback&&);

    static void retrieveRecords(NetworkProcess&, PAL::SessionID, uint64_t cacheIdentifier, PurCFetcher::RetrieveRecordsOptions&&, PurCFetcher::DOMCacheEngine::RecordsCallback&&);
    static void putRecords(NetworkProcess&, PAL::SessionID, uint64_t cacheIdentifier, Vector<PurCFetcher::DOMCacheEngine::Record>&&, PurCFetcher::DOMCacheEngine::RecordIdentifiersCallback&&);
    static void deleteMatchingRecords(NetworkProcess&, PAL::SessionID, uint64_t cacheIdentifier, PurCFetcher::ResourceRequest&&, PurCFetcher::CacheQueryOptions&&, PurCFetcher::DOMCacheEngine::RecordIdentifiersCallback&&);

    static void lock(NetworkProcess&, PAL::SessionID, uint64_t cacheIdentifier);
    static void unlock(NetworkProcess&, PAL::SessionID, uint64_t cacheIdentifier);

    static void clearMemoryRepresentation(NetworkProcess&, PAL::SessionID, PurCFetcher::ClientOrigin&&, PurCFetcher::DOMCacheEngine::CompletionCallback&&);
    static void representation(NetworkProcess&, PAL::SessionID, CompletionHandler<void(String&&)>&&);

    static void clearAllCaches(NetworkProcess&, PAL::SessionID, CompletionHandler<void()>&&);
    static void clearCachesForOrigin(NetworkProcess&, PAL::SessionID, PurCFetcher::SecurityOriginData&&, CompletionHandler<void()>&&);

    static void initializeQuotaUser(NetworkProcess&, PAL::SessionID, const PurCFetcher::ClientOrigin&, CompletionHandler<void()>&&);

    static uint64_t diskUsage(const String& rootPath, const PurCFetcher::ClientOrigin&);
    void requestSpace(const PurCFetcher::ClientOrigin&, uint64_t spaceRequested, CompletionHandler<void(PurCFetcher::StorageQuotaManager::Decision)>&&);

    bool shouldPersist() const { return !!m_ioQueue;}

    void writeFile(const String& filename, NetworkCache::Data&&, PurCFetcher::DOMCacheEngine::CompletionCallback&&);
    void readFile(const String& filename, CompletionHandler<void(const NetworkCache::Data&, int error)>&&);
    void removeFile(const String& filename);
    void writeSizeFile(const String&, uint64_t size, CompletionHandler<void()>&&);
    static std::optional<uint64_t> readSizeFile(const String&);

    const String& rootPath() const { return m_rootPath; }
    const NetworkCache::Salt& salt() const { return m_salt.value(); }
    uint64_t nextCacheIdentifier() { return ++m_nextCacheIdentifier; }

private:
    Engine(PAL::SessionID, NetworkProcess&, String&& rootPath);

    void open(const PurCFetcher::ClientOrigin&, const String& cacheName, PurCFetcher::DOMCacheEngine::CacheIdentifierCallback&&);
    void remove(uint64_t cacheIdentifier, PurCFetcher::DOMCacheEngine::CacheIdentifierCallback&&);
    void retrieveCaches(const PurCFetcher::ClientOrigin&, uint64_t updateCounter, PurCFetcher::DOMCacheEngine::CacheInfosCallback&&);

    void clearAllCaches(CompletionHandler<void()>&&);
    void clearAllCachesFromDisk(CompletionHandler<void()>&&);
    void clearCachesForOrigin(const PurCFetcher::SecurityOriginData&, CompletionHandler<void()>&&);
    void clearCachesForOriginFromDisk(const PurCFetcher::SecurityOriginData&, CompletionHandler<void()>&&);
    void deleteDirectoryRecursivelyOnBackgroundThread(const String& path, CompletionHandler<void()>&&);

    void clearMemoryRepresentation(const PurCFetcher::ClientOrigin&, PurCFetcher::DOMCacheEngine::CompletionCallback&&);
    String representation();

    void retrieveRecords(uint64_t cacheIdentifier, PurCFetcher::RetrieveRecordsOptions&&, PurCFetcher::DOMCacheEngine::RecordsCallback&&);
    void putRecords(uint64_t cacheIdentifier, Vector<PurCFetcher::DOMCacheEngine::Record>&&, PurCFetcher::DOMCacheEngine::RecordIdentifiersCallback&&);
    void deleteMatchingRecords(uint64_t cacheIdentifier, PurCFetcher::ResourceRequest&&, PurCFetcher::CacheQueryOptions&&, PurCFetcher::DOMCacheEngine::RecordIdentifiersCallback&&);

    void lock(uint64_t cacheIdentifier);
    void unlock(uint64_t cacheIdentifier);

    String cachesRootPath(const PurCFetcher::ClientOrigin&);

    void fetchEntries(bool /* shouldComputeSize */, CompletionHandler<void(Vector<WebsiteData::Entry>)>&&);

    void getDirectories(CompletionHandler<void(const Vector<String>&)>&&);
    void fetchDirectoryEntries(bool shouldComputeSize, const Vector<String>& folderPaths, CompletionHandler<void(Vector<WebsiteData::Entry>)>&&);
    void clearCachesForOriginFromDirectories(const Vector<String>&, const PurCFetcher::SecurityOriginData&, CompletionHandler<void()>&&);

    void initialize(PurCFetcher::DOMCacheEngine::CompletionCallback&&);

    using CachesOrError = Expected<std::reference_wrapper<Caches>, PurCFetcher::DOMCacheEngine::Error>;
    using CachesCallback = Function<void(CachesOrError&&)>;
    void readCachesFromDisk(const PurCFetcher::ClientOrigin&, CachesCallback&&);

    using CacheOrError = Expected<std::reference_wrapper<Cache>, PurCFetcher::DOMCacheEngine::Error>;
    using CacheCallback = Function<void(CacheOrError&&)>;
    void readCache(uint64_t cacheIdentifier, CacheCallback&&);

    CompletionHandler<void()> createClearTask(CompletionHandler<void()>&&);

    Cache* cache(uint64_t cacheIdentifier);

    PAL::SessionID m_sessionID;
    WeakPtr<NetworkProcess> m_networkProcess;
    HashMap<PurCFetcher::ClientOrigin, RefPtr<Caches>> m_caches;
    uint64_t m_nextCacheIdentifier { 0 };
    String m_rootPath;
    RefPtr<WorkQueue> m_ioQueue;
    std::optional<NetworkCache::Salt> m_salt;
    HashMap<CacheIdentifier, LockCount> m_cacheLocks;
    Vector<PurCFetcher::DOMCacheEngine::CompletionCallback> m_initializationCallbacks;
    HashMap<uint64_t, PurCFetcher::DOMCacheEngine::CompletionCallback> m_pendingWriteCallbacks;
    HashMap<uint64_t, CompletionHandler<void(const NetworkCache::Data&, int error)>> m_pendingReadCallbacks;
    uint64_t m_pendingCallbacksCounter { 0 };
    Vector<PurCFetcher::DOMCacheEngine::CompletionCallback> m_pendingClearCallbacks;
    uint64_t m_clearTaskCounter { 0 };
};

} // namespace CacheStorage

} // namespace PurCFetcher
