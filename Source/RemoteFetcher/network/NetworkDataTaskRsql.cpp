/* 
 * Copyright (C) 2020 Beijing FMSoft Technologies Co., Ltd.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 * Or,
 * 
 * As this component is a program released under LGPLv3, which claims
 * explicitly that the program could be modified by any end user
 * even if the program is conveyed in non-source form on the system it runs.
 * Generally, if you distribute this program in embedded devices,
 * you might not satisfy this condition. Under this situation or you can
 * not accept any condition of LGPLv3, you need to get a commercial license
 * from FMSoft, along with a patent license for the patents owned by FMSoft.
 * 
 * If you have got a commercial/patent license of this program, please use it
 * under the terms and conditions of the commercial license.
 * 
 * For more information about the commercial license and patent license,
 * please refer to
 * <https://hybridos.fmsoft.cn/blog/hybridos-licensing-policy/>.
 * 
 * Also note that the LGPLv3 license does not apply to any entity in the
 * Exception List published by Beijing FMSoft Technologies Co., Ltd.
 * 
 * If you are or the entity you represent is listed in the Exception List,
 * the above open source or free software license does not apply to you
 * or the entity you represent. Regardless of the purpose, you should not
 * use the software in any way whatsoever, including but not limited to
 * downloading, viewing, copying, distributing, compiling, and running.
 * If you have already downloaded it, you MUST destroy all of its copies.
 * 
 * The Exception List is published by FMSoft and may be updated
 * from time to time. For more information, please see
 * <https://www.fmsoft.cn/exception-list>.
 */ 

#include "config.h"

#if ENABLE(SCHEME_RSQL)

#include <stdio.h>
#include "NetworkDataTaskLsql.h"
#include "NetworkDataTaskRsql.h"
#include "FilterBase.h"

#include "AuthenticationChallengeDisposition.h"
#include "AuthenticationManager.h"
#include "DataReference.h"
#include "Download.h"
#include "NetworkLoad.h"
#include "NetworkProcess.h"
#include "NetworkSession.h"
#include "WebErrors.h"
#include "AuthenticationChallenge.h"
#include "HTTPParsers.h"
#include "MIMETypeRegistry.h"
#include "NetworkStorageSession.h"
#include "SharedBuffer.h"
#include "TextEncoding.h"
#include <wtf/MainThread.h>
#include <wtf/glib/RunLoopSourcePriority.h>
#include <sys/types.h>
#include <unistd.h>
#include "SQLiteStatement.h"


namespace PurCFetcher {
using namespace PurCFetcher;

#define  DEFAULT_READBUFFER_SIZE 8192

extern const char* KEY_STATUS_CODE;
extern const char* KEY_ERROR_MSG;
extern const char* KEY_EXIT_CODE;
extern const char* KEY_LINES;
extern const char* KEY_RESULT;
extern const char* KEY_ROWSAFFECTED;
extern const char* KEY_ROWS;

extern const char* CMD_SQL_QUERY;
extern const char* CMD_SQL_ROWFORMAT;

extern const char* FORMAT_DICT;
extern const char* FORMAT_ARRAY;

extern const char* SELECT;
extern const char* INSERT;
extern const char* UPDATE;
extern const char* DELETE;

NetworkDataTaskRsql::NetworkDataTaskRsql(NetworkSession& session, NetworkDataTaskClient& client, const ResourceRequest& requestWithCredentials, StoredCredentialsPolicy storedCredentialsPolicy, ContentSniffingPolicy shouldContentSniff, PurCFetcher::ContentEncodingSniffingPolicy, bool shouldClearReferrerOnHTTPSToHTTPRedirect, bool dataTaskIsForMainFrameNavigation)
    : NetworkDataTask(session, client, requestWithCredentials, storedCredentialsPolicy, shouldClearReferrerOnHTTPSToHTTPRedirect, dataTaskIsForMainFrameNavigation)
    , m_formatArray(false)
{
    UNUSED_PARAM(shouldContentSniff);
    m_session->registerNetworkDataTask(*this);
    if (m_scheduledFailureType != NoFailure)
        return;

    auto request = requestWithCredentials;
    createRequest(WTFMove(request));
}

NetworkDataTaskRsql::~NetworkDataTaskRsql()
{
    m_session->unregisterNetworkDataTask(*this);
}

String NetworkDataTaskRsql::suggestedFilename() const
{
    String suggestedFilename = m_response.suggestedFilename();
    if (!suggestedFilename.isEmpty())
        return suggestedFilename;

    return decodeURLEscapeSequences(m_response.url().lastPathComponent());
}

void NetworkDataTaskRsql::setPendingDownloadLocation(const String& filename, SandboxExtension::Handle&& sandboxExtensionHandle, bool allowOverwrite)
{
    NetworkDataTask::setPendingDownloadLocation(filename, WTFMove(sandboxExtensionHandle), allowOverwrite);
}

void NetworkDataTaskRsql::cancel()
{
    if (m_state == State::Canceling || m_state == State::Completed)
        return;

    m_state = State::Canceling;
}

void NetworkDataTaskRsql::resume()
{
    ASSERT(m_state != State::Running);
    if (m_state == State::Canceling || m_state == State::Completed)
        return;

    m_state = State::Running;

    if (m_scheduledFailureType != NoFailure) {
        ASSERT(m_failureTimer.isActive());
        return;
    }

    sendRequest();
}

void NetworkDataTaskRsql::invalidateAndCancel()
{
}

NetworkDataTask::State NetworkDataTaskRsql::state() const
{
    return m_state;
}

void NetworkDataTaskRsql::dispatchDidCompleteWithError(const ResourceError& error)
{
    m_networkLoadMetrics.responseEnd = MonotonicTime::now() - m_startTime;
    m_networkLoadMetrics.markComplete();

    m_client->didCompleteWithError(error, m_networkLoadMetrics);
}

void NetworkDataTaskRsql::dispatchDidReceiveResponse()
{
    m_networkLoadMetrics.responseStart = MonotonicTime::now() - m_startTime;
    m_response.setURL(m_currentRequest.url());
    const char* contentType = "application/json";
    m_response.setMimeType(extractMIMETypeFromMediaType(contentType));
    m_response.setTextEncodingName(extractCharsetFromMediaType(contentType));
    m_response.setExpectedContentLength(m_responseBuffer.size());
    m_response.setHTTPHeaderField(HTTPHeaderName::AccessControlAllowOrigin, "*");
    m_response.setHTTPHeaderField(HTTPHeaderName::Expires, "-1");
    m_response.setHTTPHeaderField(HTTPHeaderName::CacheControl, "no-cache");
    m_response.setHTTPHeaderField(HTTPHeaderName::Pragma, "no-cache");
    m_response.setHTTPStatusCode(m_statusCode);

    didReceiveResponse(ResourceResponse(m_response), NegotiatedLegacyTLS::No, [this, protectedThis = makeRef(*this)](PolicyAction policyAction) {
        if (m_state == State::Canceling || m_state == State::Completed) {
            return;
        }

        switch (policyAction) {
        case PolicyAction::Use:
            {
                m_client->didReceiveData(SharedBuffer::create(WTFMove(m_responseBuffer)));
                dispatchDidCompleteWithError({ });
            }
            break;

        case PolicyAction::Ignore:
        case PolicyAction::Download:
        case PolicyAction::StopAllLoads:
            break;
        }
    });
}

void NetworkDataTaskRsql::createRequest(PurCFetcher::ResourceRequest&& request)
{
    m_currentRequest = WTFMove(request);
    m_startTime = MonotonicTime::now();
}

void NetworkDataTaskRsql::sendRequest()
{
    runCmdInner();
    buildResponse();
    dispatchDidReceiveResponse();
}

void NetworkDataTaskRsql::runCmdInner()
{
    String path = m_currentRequest.url().path().toString().stripWhiteSpace();

    if (m_currentRequest.url().hasQuery())
    {
        parseQueryString(m_currentRequest.url().query().toString());
        if (!m_sqlQuery.isEmpty())
        {
            parseSqlQuery(m_sqlQuery);
        }
    }

#if 0
    if (m_currentRequest.url().hasFragment())
        printf("......................Fragment=%s\n", m_currentRequest.url().fragmentIdentifier().utf8().data());
#endif

    String user =  m_currentRequest.url().user();
    String pass =  m_currentRequest.url().password();
    String server = m_currentRequest.url().host().toString();
    std::optional<uint16_t> portO = m_currentRequest.url().port();
    int port = portO ? portO.value() : -1;
    String database;
    String dbName;

    Vector<String> pathVec = path.split('/');
    if (pathVec.size() >= 2)
    {
        database = pathVec[0];
        dbName = pathVec[pathVec.size() - 1];
    }
    else
    {
        database = pathVec[0];
        dbName = pathVec[0];
    }

#if 0
    printf("......................user=%s\n", user.characters8());
    printf("......................pass=%s\n", pass.characters8());
    printf(".......................svr=%s\n", server.characters8());
    printf("......................port=%d\n", port);
    printf("......................path=%s\n", path.characters8());
    printf("......................database=%s\n", database.characters8());
    printf("......................dbName=%s\n", dbName.characters8());
#endif

    mysql_init(&m_mysql);

    if (!mysql_real_connect(&m_mysql, server.utf8().data(), user.utf8().data(), pass.utf8().data(), dbName.utf8().data(), port, NULL, 0))
    {
        m_exitCode = 127;
        m_statusCode = 404;
        StringBuilder sb;
        sb.append("Failed to connect to database:");
        sb.append(mysql_error(&m_mysql));
        m_errorMsg = sb.toString();
//        printf("Failed to connect to database: %s\n", m_errorMsg.characters8());
        return;
    }

    m_statusCode = 200;
    int size = m_sqlVec.size();
    for (int i = 0; i < size; i++)
    {
        String& sql = m_sqlVec[i];
        if (sql.startsWithIgnoringASCIICase(SELECT))
        {
            runSqlSelect(sql);
        }
        else if (sql.startsWithIgnoringASCIICase(INSERT))
        {
            runSqlInsert(sql);
        }
        else if (sql.startsWithIgnoringASCIICase(UPDATE))
        {
            runSqlUpdate(sql);
        }
        else if (sql.startsWithIgnoringASCIICase(DELETE))
        {
            runSqlDelete(sql);
        }
    }
}

void NetworkDataTaskRsql::runSqlSelect(String sql)
{
    if (sql.isEmpty())
        return;

    SqlResult sr;
    CString cmd = sql.utf8();
    if (mysql_real_query(&m_mysql, cmd.data(), cmd.length()))
    {
        StringBuilder sb;
        sb.append("Failed to query : ");
        sb.append(sql);
        sb.append(". Error : ");
        sb.append(mysql_error(&m_mysql));

        sr.statusCode = 500;
        sr.errorMsg = sb.toString();
        m_sqlResults.append(sr);
        return;
    }

    MYSQL_RES* res = mysql_store_result(&m_mysql);
    if (!res) {
        sr.statusCode = 500;
        sr.errorMsg = "Failed to get result : " + sql;
        m_sqlResults.append(sr);
        return;
    }

    sr.statusCode = 200;

    int num_fields = mysql_num_fields(res);
    MYSQL_FIELD* fields = mysql_fetch_fields(res);
    for (int i = 0; i < num_fields; ++i)
    {
        m_sqlResultColumnNames.append(fields[i].name);
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)))
    {
        Vector<SQLValueH> columns;
        unsigned long *lengths = mysql_fetch_lengths(res);
        for (int i = 0; i < num_fields; ++i)
        {
            String value(row[i], lengths[i]);
            switch (fields[i].type)
            {
                case MYSQL_TYPE_TINY:
                case MYSQL_TYPE_SHORT:
                case MYSQL_TYPE_LONG:
                    columns.append(value.toInt());
                    break;

                case MYSQL_TYPE_FLOAT:
                case MYSQL_TYPE_DOUBLE: 
                    columns.append(value.toDouble());
                    break;

                case MYSQL_TYPE_NULL:
                    columns.append(nullptr);
                    break;

                default:
                    columns.append(value);
                    break;
            }
        }
        sr.rowsVec.append(columns);

        if (m_state == State::Canceling)
        {
            sr.statusCode = 503;
            sr.errorMsg = "Canceling";
            m_sqlResults.append(sr);
            break;
        }
    }
    mysql_free_result(res);

    sr.rowsAffected = sr.rowsVec.size();
    m_sqlResults.append(sr);
}

void NetworkDataTaskRsql::runSqlInsert(String sql)
{
    if (sql.isEmpty())
        return;

    SqlResult sr;
    CString cmd = sql.utf8();
    if (mysql_real_query(&m_mysql, cmd.data(), cmd.length()))
    {
        StringBuilder sb;
        sb.append("Failed to : ");
        sb.append(sql);
        sb.append(". Error : ");
        sb.append(mysql_error(&m_mysql));

        sr.statusCode = 500;
        sr.errorMsg = sb.toString();
        m_sqlResults.append(sr);
        return;
    }

    sr.statusCode = 200;
    sr.rowsAffected = mysql_affected_rows(&m_mysql);
    m_sqlResults.append(sr);
}

void NetworkDataTaskRsql::runSqlUpdate(String sql)
{
    runSqlInsert(sql);
}

void NetworkDataTaskRsql::runSqlDelete(String sql)
{
    runSqlInsert(sql);
}

void NetworkDataTaskRsql::buildResponse()
{
    auto result = JSON::Object::create();

    int resultSize = m_sqlResults.size();
    switch (resultSize)
    {
    case 0:
        {
            result->setInteger(KEY_STATUS_CODE, m_statusCode);

            if (m_errorMsg.isEmpty())
                result->setValue(KEY_ERROR_MSG, JSON::Value::null());
            else
                result->setString(KEY_ERROR_MSG, m_errorMsg);

            result->setInteger(KEY_ROWSAFFECTED, m_readLines.size());
            auto array = JSON::Array::create();
            result->setArray(KEY_ROWS, WTFMove(array));
        }
        break;

    case 1:
        {
            SqlResult& sqlResult = m_sqlResults[0];
            result->setInteger(KEY_STATUS_CODE, sqlResult.statusCode);
            if (sqlResult.errorMsg.isEmpty())
                result->setValue(KEY_ERROR_MSG, JSON::Value::null());
            else
                result->setString(KEY_ERROR_MSG, sqlResult.errorMsg);
            result->setInteger(KEY_ROWSAFFECTED, sqlResult.rowsAffected);

            int rowSize = sqlResult.rowsVec.size();
            auto array = JSON::Array::create();
            for (int i = 0; i < rowSize; i++)
            {
                if (m_formatArray)
                {
                    array->pushValue(formatAsArray(sqlResult.rowsVec[i]));
                }
                else
                {
                    array->pushValue(formatAsDict(sqlResult.rowsVec[i]));
                }
            }
            result->setArray(KEY_ROWS, WTFMove(array));
        }
        break;

    default:
        {
            result->setInteger(KEY_STATUS_CODE, 200);
            auto resultArray = JSON::Array::create();
            for (int j = 0; j < resultSize; j++)
            {
                SqlResult& sqlResult = m_sqlResults[j];
                auto res = JSON::Object::create();

                if (sqlResult.errorMsg.isEmpty())
                    res->setValue(KEY_ERROR_MSG, JSON::Value::null());
                else
                    res->setString(KEY_ERROR_MSG, sqlResult.errorMsg);
                res->setInteger(KEY_ROWSAFFECTED, sqlResult.rowsAffected);

                int rowSize = sqlResult.rowsVec.size();
                auto array = JSON::Array::create();
                for (int i = 0; i < rowSize; i++)
                {
                    if (m_formatArray)
                    {
                        array->pushValue(formatAsArray(sqlResult.rowsVec[i]));
                    }
                    else
                    {
                        array->pushValue(formatAsDict(sqlResult.rowsVec[i]));
                    }
                }
                res->setArray(KEY_ROWS, WTFMove(array));
                resultArray->pushObject(WTFMove(res));
            }
            result->setArray(KEY_RESULT, WTFMove(resultArray));
        }
        break;
    }

    String json = result->toJSONString();

    m_responseBuffer.clear();
    m_responseBuffer.append(json.characters8(), json.length());
}

Ref<JSON::Value> NetworkDataTaskRsql::formatAsArray(Vector<SQLValueH>& lineColumns)
{
    auto array = JSON::Array::create();
    if (lineColumns.size() == 0)
        return array;

    int size = lineColumns.size();
    for (int i = 0; i < size; i++)
    {
        SQLValueH value = lineColumns[i];
        if (PurCWTF::holds_alternative<String>(value))
        {
            array->pushString(PurCWTF::get<String>(value));
        }
        else if (PurCWTF::holds_alternative<double>(value))
        {
            array->pushDouble(PurCWTF::get<double>(value));
        }
        else if (PurCWTF::holds_alternative<int>(value))
        {
            array->pushInteger(PurCWTF::get<int>(value));
        }
        else
            array->pushValue(JSON::Value::null());
    }
    return array;
}

Ref<JSON::Value> NetworkDataTaskRsql::formatAsDict(Vector<SQLValueH>& lineColumns)
{
    auto result = JSON::Object::create();
    if (lineColumns.size() == 0)
        return result;

    int size = lineColumns.size();
    for (int i = 0; i < size; i++)
    {
        SQLValueH value = lineColumns[i];
        if (PurCWTF::holds_alternative<String>(value))
        {
            result->setString(m_sqlResultColumnNames[i], PurCWTF::get<String>(value));
        }
        else if (PurCWTF::holds_alternative<double>(value))
        {
            result->setDouble(m_sqlResultColumnNames[i], PurCWTF::get<double>(value));
        }
        else if (PurCWTF::holds_alternative<int>(value))
        {
            result->setInteger(m_sqlResultColumnNames[i], PurCWTF::get<int>(value));
        }
        else
            result->setValue(m_sqlResultColumnNames[i], JSON::Value::null());
    }
    return result;
}

void NetworkDataTaskRsql::parseQueryString(String query)
{
    if (query.isEmpty())
        return;

    Vector<String> params = query.split("&");
    int size = params.size();
    for (int i = 0; i < size; i++)
    {
        String param = PurCWTF::decodeEscapeSequencesFromParsedURL(
                StringView(params[i]));
        size_t index = param.find("=");
        if (index == notFound)
            index = param.length();

        String name = param.substring(0, index).stripWhiteSpace();
        String value = param.substring(index + 1).stripWhiteSpace();

        if (equalIgnoringASCIICase(name, CMD_SQL_QUERY))
        {
            m_sqlQuery = value;
        }
        else if (equalIgnoringASCIICase(name, CMD_SQL_ROWFORMAT))
        {
            m_formatArray = equalIgnoringASCIICase(value, FORMAT_ARRAY);
        }
        else
        {
            m_paramMap.set(name, value);
        }
    }
}

void NetworkDataTaskRsql::parseSqlQuery(String sqlQuery)
{
    if (sqlQuery.isEmpty())
        return;

    Vector<String> params = sqlQuery.split(";");
    int size = params.size();
    for (int i = 0; i < size; i++)
    {
        StringBuilder sqlSb;
        String param = params[i].stripWhiteSpace();

        size_t start = param.find('$');
        if (start == notFound)
        {
            m_sqlVec.append(param);
            continue;
        }

        size_t paramLength = param.length();
        size_t end = start + 1;
        sqlSb.append(param.substring(0, start - 0));
        while ((start != notFound) && (end < paramLength))
        {
            if (param[end] == '$' )
            {
                sqlSb.append("$");

                start = param.find('$', end + 1);
                if (start != notFound)
                {
                    sqlSb.append(param.substring(end+1, start - end - 1));
                }
                else
                {
                    sqlSb.append(param.substring(end+1));
                }

                end = start + 1;
                continue;
            }

            if (!((param[end] >= 'a' && param[end] <= 'z')
                        || (param[end] >= 'A' && param[end] <= 'Z')
                        ))
            {
                sqlSb.append(param.substring(start, 2));
                start = param.find('$', end + 1);
                if (start != notFound)
                {
                    sqlSb.append(param.substring(end + 1, start - end - 1));
                }
                else
                {
                    sqlSb.append(param.substring(end + 1));
                }
                end = start + 1;
                continue;
            }

#if 0
            if (param[end] >= '0' and param[end] <= '9')
            {
            }
#endif

            StringBuilder sb;
            for (;end < paramLength; end++)
            {
                if ((param[end] >= '0' && param[end] <= '9')
                    || (param[end] >= 'a' && param[end] <= 'z')
                    || (param[end] >= 'A' && param[end] <= 'Z')
                    || (param[end] == '_'))
                {
                    sb.append(param[end]);
                }
                else
                {
                    break;
                }
            }

            auto findResult = m_paramMap.find(sb.toString());
            if(findResult != m_paramMap.end())
            {
                sqlSb.append(findResult->value);
            }

            if (end < paramLength)
            {
                start = param.find('$', end);
                if (start != notFound)
                {
                    sqlSb.append(param.substring(end, start - end));
                }
                else
                {
                    sqlSb.append(param.substring(end));
                }
                end = start + 1;
            }
        }

        if (!sqlSb.isEmpty())
        {
            m_sqlVec.append(sqlSb.toString());
        }
    }
}

} // namespace PurCFetcher

#endif // ENABLE(SCHEME_RSQL)
