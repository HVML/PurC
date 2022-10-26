/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#include "SQLValue.h"
#include "SQLiteDatabase.h"

struct sqlite3_stmt;

namespace PurCFetcher {

class SQLiteStatement {
    WTF_MAKE_NONCOPYABLE(SQLiteStatement); WTF_MAKE_FAST_ALLOCATED;
public:
    PURCFETCHER_EXPORT SQLiteStatement(SQLiteDatabase&, const String&);
    PURCFETCHER_EXPORT ~SQLiteStatement();
    
    PURCFETCHER_EXPORT int prepare();
    PURCFETCHER_EXPORT int bindBlob(int index, const void* blob, int size);
    PURCFETCHER_EXPORT int bindBlob(int index, const String&);
    PURCFETCHER_EXPORT int bindText(int index, const String&);
    PURCFETCHER_EXPORT int bindInt(int index, int);
    PURCFETCHER_EXPORT int bindInt64(int index, int64_t);
    PURCFETCHER_EXPORT int bindDouble(int index, double);
    PURCFETCHER_EXPORT int bindNull(int index);
    PURCFETCHER_EXPORT int bindValue(int index, const SQLValue&);
    PURCFETCHER_EXPORT unsigned bindParameterCount() const;

    PURCFETCHER_EXPORT int step();
    PURCFETCHER_EXPORT int finalize();
    PURCFETCHER_EXPORT int reset();
    
    int prepareAndStep() { if (int error = prepare()) return error; return step(); }
    
    // prepares, steps, and finalizes the query.
    // returns true if all 3 steps succeed with step() returning SQLITE_DONE
    // returns false otherwise  
    PURCFETCHER_EXPORT bool executeCommand();
    
    // prepares, steps, and finalizes.  
    // returns true is step() returns SQLITE_ROW
    // returns false otherwise
    bool returnsAtLeastOneResult();

    bool isExpired();

    // Returns -1 on last-step failing.  Otherwise, returns number of rows
    // returned in the last step()
    int columnCount();
    
    PURCFETCHER_EXPORT bool isColumnNull(int col);
    PURCFETCHER_EXPORT bool isColumnDeclaredAsBlob(int col);
    String getColumnName(int col);
    SQLValue getColumnValue(int col);
    SQLValueH getColumnValueH(int col);
    PURCFETCHER_EXPORT String getColumnText(int col);
    PURCFETCHER_EXPORT double getColumnDouble(int col);
    PURCFETCHER_EXPORT int getColumnInt(int col);
    PURCFETCHER_EXPORT int64_t getColumnInt64(int col);
    PURCFETCHER_EXPORT String getColumnBlobAsString(int col);
    PURCFETCHER_EXPORT void getColumnBlobAsVector(int col, Vector<char>&);
    PURCFETCHER_EXPORT void getColumnBlobAsVector(int col, Vector<uint8_t>&);

    bool returnTextResults(int col, Vector<String>&);
    bool returnIntResults(int col, Vector<int>&);
    bool returnInt64Results(int col, Vector<int64_t>&);
    bool returnDoubleResults(int col, Vector<double>&);

    SQLiteDatabase& database() { return m_database; }
    
    const String& query() const { return m_query; }
    
private:
    SQLiteDatabase& m_database;
    String m_query;
    sqlite3_stmt* m_statement;
#if ENABLE_ASSERTS
    bool m_isPrepared { false };
#endif
};

} // namespace PurCFetcher
