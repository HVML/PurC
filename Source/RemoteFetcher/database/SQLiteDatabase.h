/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Justin Haygood (jhaygood@reaktix.com)
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

#include <functional>
#include <sqlite3.h>
#include <wtf/Lock.h>
#include <wtf/Threading.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#if COMPILER(MSVC)
#pragma warning(disable: 4800)
#endif

struct sqlite3;

namespace PurCFetcher {

class DatabaseAuthorizer;
class SQLiteStatement;
class SQLiteTransaction;

class SQLiteDatabase {
    WTF_MAKE_FAST_ALLOCATED;
    WTF_MAKE_NONCOPYABLE(SQLiteDatabase);
    friend class SQLiteTransaction;
public:
    PURCFETCHER_EXPORT SQLiteDatabase();
    PURCFETCHER_EXPORT ~SQLiteDatabase();

    enum class OpenMode { ReadOnly, ReadWrite, ReadWriteCreate };
    PURCFETCHER_EXPORT bool open(const String& filename, OpenMode = OpenMode::ReadWriteCreate);
    bool isOpen() const { return m_db; }
    PURCFETCHER_EXPORT void close();

    void updateLastChangesCount();

    PURCFETCHER_EXPORT bool executeCommand(const String&);
    bool returnsAtLeastOneResult(const String&);

    PURCFETCHER_EXPORT bool tableExists(const String&);
    PURCFETCHER_EXPORT void clearAllTables();
    PURCFETCHER_EXPORT int runVacuumCommand();
    PURCFETCHER_EXPORT int runIncrementalVacuumCommand();

    bool transactionInProgress() const { return m_transactionInProgress; }

    // Aborts the current database operation. This is thread safe.
    PURCFETCHER_EXPORT void interrupt();

    int64_t lastInsertRowID();
    int lastChanges();

    void setBusyTimeout(int ms);
    void setBusyHandler(int(*)(void*, int));

    void setFullsync(bool);

    // This enables automatic WAL truncation via a commit hook that uses SQLITE_CHECKPOINT_TRUNCATE.
    // However, it shouldn't be used if you use a custom busy handler or timeout. This is because
    // SQLITE_CHECKPOINT_TRUNCATE will invoke the busy handler if it can't acquire the necessary
    // locks, which can lead to unintended delays.
    void enableAutomaticWALTruncation();

    // Gets/sets the maximum size in bytes
    // Depending on per-database attributes, the size will only be settable in units that are the page size of the database, which is established at creation
    // These chunks will never be anything other than 512, 1024, 2048, 4096, 8192, 16384, or 32768 bytes in size.
    // setMaximumSize() will round the size down to the next smallest chunk if the passed size doesn't align.
    int64_t maximumSize();
    void setMaximumSize(int64_t);

    // Gets the number of unused bytes in the database file.
    int64_t freeSpaceSize();
    int64_t totalSize();

    // The SQLite SYNCHRONOUS pragma can be either FULL, NORMAL, or OFF
    // FULL - Any writing calls to the DB block until the data is actually on the disk surface
    // NORMAL - SQLite pauses at some critical moments when writing, but much less than FULL
    // OFF - Calls return immediately after the data has been passed to disk
    enum SynchronousPragma { SyncOff = 0, SyncNormal = 1, SyncFull = 2 };
    void setSynchronous(SynchronousPragma);

    PURCFETCHER_EXPORT int lastError();
    PURCFETCHER_EXPORT const char* lastErrorMsg();

    sqlite3* sqlite3Handle() const
    {
        ASSERT(m_sharable || m_openingThread == &Thread::current() || !m_db);
        return m_db;
    }

    void setAuthorizer(DatabaseAuthorizer&);

    Lock& databaseMutex() { return m_lockingMutex; }
    bool isAutoCommitOn() const;

    // The SQLite AUTO_VACUUM pragma can be either NONE, FULL, or INCREMENTAL.
    // NONE - SQLite does not do any vacuuming
    // FULL - SQLite moves all empty pages to the end of the DB file and truncates
    //        the file to remove those pages after every transaction. This option
    //        requires SQLite to store additional information about each page in
    //        the database file.
    // INCREMENTAL - SQLite stores extra information for each page in the database
    //               file, but removes the empty pages only when PRAGMA INCREMANTAL_VACUUM
    //               is called.
    enum AutoVacuumPragma { AutoVacuumNone = 0, AutoVacuumFull = 1, AutoVacuumIncremental = 2 };
    PURCFETCHER_EXPORT bool turnOnIncrementalAutoVacuum();

    PURCFETCHER_EXPORT void setCollationFunction(const String& collationName, PurCWTF::Function<int(int, const void*, int, const void*)>&&);
    void removeCollationFunction(const String& collationName);

    // Set this flag to allow access from multiple threads.  Not all multi-threaded accesses are safe!
    // See http://www.sqlite.org/cvstrac/wiki?p=MultiThreading for more info.
#if ENABLE_ASSERTS
    PURCFETCHER_EXPORT void disableThreadingChecks();
#else
    void disableThreadingChecks() { }
#endif

    PURCFETCHER_EXPORT static void setIsDatabaseOpeningForbidden(bool);

private:
    static int authorizerFunction(void*, int, const char*, const char*, const char*, const char*);

    void enableAuthorizer(bool enable);
    void useWALJournalMode();

    int pageSize();

    void overrideUnauthorizedFunctions();

    sqlite3* m_db { nullptr };
    int m_pageSize { -1 };

    bool m_transactionInProgress { false };
#if ENABLE_ASSERTS
    bool m_sharable { false };
#endif

    bool m_useWAL { false };

    Lock m_authorizerLock;
    RefPtr<DatabaseAuthorizer> m_authorizer;

    Lock m_lockingMutex;
    RefPtr<Thread> m_openingThread { nullptr };

    Lock m_databaseClosingMutex;

    int m_openError { SQLITE_ERROR };
    CString m_openErrorMessage;

    int m_lastChangesCount { 0 };
};

} // namespace PurCFetcher
