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

#if ENABLE(LCMD)

#include <stdio.h>
#include "NetworkDataTaskLcmd.h"
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


namespace PurCFetcher {
using namespace PurCFetcher;

#define  DEFAULT_READBUFFER_SIZE 8192

const char* KEY_STATUS_CODE = "statusCode";
const char* KEY_ERROR_MSG = "errorMsg";
const char* KEY_EXIT_CODE = "exitCode";
const char* KEY_LINES = "lines";

const char* CMD_FILTER = "cmdfilter";
const char* CMD_LINE = "cmdline";

String decodeEscapeSequencesFromParsedURL(StringView input)
{
    auto inputLength = input.length();
    if (!inputLength)
        return emptyString();
    Vector<LChar> percentDecoded;
    percentDecoded.reserveInitialCapacity(inputLength);
    for (unsigned i = 0; i < inputLength; ++i) {
        if (input[i] == '%'
            && inputLength > 2
            && i < inputLength - 2
            && isASCIIHexDigit(input[i + 1])
            && isASCIIHexDigit(input[i + 2])) {
            percentDecoded.uncheckedAppend(toASCIIHexValue(input[i + 1], input[i + 2]));
            i += 2;
        } else
            percentDecoded.uncheckedAppend(input[i]);
    }
    return String::fromUTF8(percentDecoded.data(), percentDecoded.size());
}

NetworkDataTaskLcmd::NetworkDataTaskLcmd(NetworkSession& session, NetworkDataTaskClient& client, const ResourceRequest& requestWithCredentials, StoredCredentialsPolicy storedCredentialsPolicy, ContentSniffingPolicy shouldContentSniff, PurCFetcher::ContentEncodingSniffingPolicy, bool shouldClearReferrerOnHTTPSToHTTPRedirect, bool dataTaskIsForMainFrameNavigation)
    : NetworkDataTask(session, client, requestWithCredentials, storedCredentialsPolicy, shouldClearReferrerOnHTTPSToHTTPRedirect, dataTaskIsForMainFrameNavigation)
    , m_filterManager(adoptRef(*new CmdFilterManager()))
{
    UNUSED_PARAM(shouldContentSniff);
    m_session->registerNetworkDataTask(*this);
    if (m_scheduledFailureType != NoFailure)
        return;

    auto request = requestWithCredentials;
    createRequest(WTFMove(request));
}

NetworkDataTaskLcmd::~NetworkDataTaskLcmd()
{
    m_session->unregisterNetworkDataTask(*this);
}

String NetworkDataTaskLcmd::suggestedFilename() const
{
    String suggestedFilename = m_response.suggestedFilename();
    if (!suggestedFilename.isEmpty())
        return suggestedFilename;

    return decodeURLEscapeSequences(m_response.url().lastPathComponent());
}

void NetworkDataTaskLcmd::setPendingDownloadLocation(const String& filename, SandboxExtension::Handle&& sandboxExtensionHandle, bool allowOverwrite)
{
    NetworkDataTask::setPendingDownloadLocation(filename, WTFMove(sandboxExtensionHandle), allowOverwrite);
}

void NetworkDataTaskLcmd::cancel()
{
    if (m_state == State::Canceling || m_state == State::Completed)
        return;

    m_state = State::Canceling;
}

void NetworkDataTaskLcmd::resume()
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

void NetworkDataTaskLcmd::invalidateAndCancel()
{
}

NetworkDataTask::State NetworkDataTaskLcmd::state() const
{
    return m_state;
}

void NetworkDataTaskLcmd::dispatchDidCompleteWithError(const ResourceError& error)
{
    m_networkLoadMetrics.responseEnd = MonotonicTime::now() - m_startTime;
    m_networkLoadMetrics.markComplete();

    m_client->didCompleteWithError(error, m_networkLoadMetrics);
}

void NetworkDataTaskLcmd::dispatchDidReceiveResponse()
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

void NetworkDataTaskLcmd::createRequest(PurCFetcher::ResourceRequest&& request)
{
    m_currentRequest = WTFMove(request);
    m_startTime = MonotonicTime::now();
}

void NetworkDataTaskLcmd::sendRequest()
{
    runCmdInner();
    buildResponse();
    dispatchDidReceiveResponse();
}

void NetworkDataTaskLcmd::runCmdInner()
{
    m_readBuffer.clear();
	char data[DEFAULT_READBUFFER_SIZE] = {'0'};

    String cmdLine;
    if (m_currentRequest.url().hasQuery())
    {
        parseQueryString(m_currentRequest.url().query().toString());
        if (!m_cmdFilter.isEmpty())
        {
            parseCmdFilter(m_cmdFilter);
        }

        if (!m_cmdLine.isEmpty())
        {
            cmdLine = parseCmdLine(m_cmdLine);
        }

    }

#if 0
    if (m_currentRequest.url().hasFragment())
        printf("......................Fragment=%s\n", m_currentRequest.url().fragmentIdentifier().utf8().data());
#endif


    String path = m_currentRequest.url().path().toString().stripWhiteSpace();
    const char* command;
    if (cmdLine.isEmpty())
    {
        command = (const char*)path.characters8();
    }
    else
    {
        StringBuilder sb;
        sb.append(path);
        if (path.endsWith("sh"))
        {
            sb.append(" -c \"");
            sb.append(cmdLine);
            sb.append(" \"");
        }
        else
        {
            sb.append(" ");

            size_t rfindResult = path.reverseFind("/");
            String prefix = (rfindResult == notFound) ? path + " ": path.substring(rfindResult + 1) + " ";
            if (cmdLine.startsWith(prefix))
            {
                sb.append(cmdLine.substring(prefix.length()));
            }
            else
            {
                sb.append(cmdLine);
            }
        }
        const CString& tmp = sb.toString().utf8();
        command = (const char*)tmp.data();
    }

	FILE* fp = popen(command, "r");
	if (fp == NULL)
	{
        m_statusCode = 500;
        m_errorMsg = String(strerror(errno));
        return;
	}
	while (fgets(data, sizeof(data), fp) != NULL)
	{
        m_readBuffer.append(data, strlen(data));
        if (m_state == State::Canceling)
        {
            m_statusCode = 503;
            m_errorMsg = String(strerror(errno));
            return;
        }
	}

    m_readLines = String(m_readBuffer.data(),m_readBuffer.size()).split("\n");

	int status = pclose(fp);
    m_exitCode = WEXITSTATUS(status);
    if (m_exitCode == 127)
    {
        m_statusCode = 404;
        m_errorMsg = "Not Found";
    }
    else
    {
        m_statusCode = 200;
    }
}

void NetworkDataTaskLcmd::runCmdOuter()
{
}

void NetworkDataTaskLcmd::buildResponse()
{
    m_responseBuffer.clear();
    auto result = JSON::Object::create();
    result->setInteger(KEY_STATUS_CODE, m_statusCode);
    if (m_errorMsg.isEmpty())
        result->setValue(KEY_ERROR_MSG, JSON::Value::null());
    else
        result->setString(KEY_ERROR_MSG, m_errorMsg);
    if (m_statusCode == 200 || m_statusCode == 404)
        result->setInteger(KEY_EXIT_CODE, m_exitCode);
    else
        result->setValue(KEY_EXIT_CODE, JSON::Value::null());

    if (m_readLines.size())
    {
        auto array = JSON::Array::create();
        Vector<Ref<JSON::Value>>  lines = m_filterManager->doFilter(m_readLines);
        int lineSize = lines.size();
        for (int i = 0; i < lineSize; i++)
        {
            array->pushValue(WTFMove(lines[i]));
        }
        result->setArray(KEY_LINES, WTFMove(array));
    }
    else
    {
        auto array = JSON::Array::create();
        result->setArray(KEY_LINES, WTFMove(array));
    }

    String json = result->toJSONString();
    m_responseBuffer.append(json.characters8(), json.length());
}

void NetworkDataTaskLcmd::parseQueryString(String query)
{
    if (query.isEmpty())
        return;

    Vector<String> params = query.split("&");
    int size = params.size();
    for (int i = 0; i < size; i++)
    {
        String decode = PurCWTF::decodeEscapeSequencesFromParsedURL(
                StringView(params[i]));
        size_t index = decode.find("=");
        if (index == notFound)
            index = decode.length();

        String name = decode.substring(0, index).stripWhiteSpace();
        String value = decode.substring(index + 1).stripWhiteSpace();
        if (equalIgnoringASCIICase(name, CMD_FILTER))
        {
            m_cmdFilter = value;
        }
        else if (equalIgnoringASCIICase(name, CMD_LINE))
        {
            m_cmdLine = value;
        }
        else
        {
            m_paramMap.set(name, value);
        }
    }
}

void NetworkDataTaskLcmd::parseCmdFilter(String cmdFilter)
{
    if (cmdFilter.isEmpty())
        return;
    Vector<String> params = cmdFilter.split(";");
    int size = params.size();
    for (int i = 0; i < size; i++)
    {
        size_t index = params[i].find("(");
        if (index == notFound)
            index = params[i].length();

        String name = params[i].substring(0, index);
        String value = params[i].substring(index + 1);
        if (!value.isEmpty())
        {
            size_t idx = value.reverseFind(")");
            if (idx != notFound)
                value = value.substring(0, idx);
        }
        m_filterManager->addFilter(name, value.stripLeadingAndTrailingCharacters(isSingleQuotes));
    }
}

String NetworkDataTaskLcmd::parseCmdLine(String cmdLine)
{
    if (cmdLine.isEmpty())
        return cmdLine;

    StringBuilder resultSb;
    size_t start = cmdLine.find('$');
    if (start == notFound)
    {
        return cmdLine;
    }

    size_t cmdLineLength = cmdLine.length();
    size_t end = start + 1;
    resultSb.append(cmdLine.substring(0, start - 0));
    while (start != notFound && end < cmdLineLength) 
    {
        if (cmdLine[end] == '$' )
        {
            resultSb.append("$");

            start = cmdLine.find('$', end + 1);
            if (start != notFound)
            {
                resultSb.append(cmdLine.substring(end+1, start - end - 1));
            }
            else
            {
                resultSb.append(cmdLine.substring(end+1));
            }

            end = start + 1;
            continue;
        }

        if (!((cmdLine[end] >= 'a' && cmdLine[end] <= 'z') ||
                    (cmdLine[end] >= 'A' && cmdLine[end] <= 'Z')))
        {
            resultSb.append(cmdLine.substring(start, 2));
            start = cmdLine.find('$', end + 1);
            if (start != notFound)
            {
                resultSb.append(cmdLine.substring(end + 1, start - end - 1));
            }
            else
            {
                resultSb.append(cmdLine.substring(end + 1));
            }
            end = start + 1;
            continue;
        }

#if 0
        if (cmdLine[end] >= '0' and cmdLine[end] <= '9')
        {
        }
#endif

        StringBuilder sb;
        for (;end < cmdLineLength; end++)
        {
            if ((cmdLine[end] >= '0' && cmdLine[end] <= '9')
                    || (cmdLine[end] >= 'a' && cmdLine[end] <= 'z')
                    || (cmdLine[end] >= 'A' && cmdLine[end] <= 'Z')
                    || (cmdLine[end] == '_')
               )
            {
                sb.append(cmdLine[end]);
            }
            else
            {
                break;
            }
        }

        auto findResult = m_paramMap.find(sb.toString());
        if(findResult != m_paramMap.end())
        {
            resultSb.append(findResult->value);
        }

        if (end < cmdLineLength)
        {
            start = cmdLine.find('$', end);
            if (start != notFound)
            {
                resultSb.append(cmdLine.substring(end, start - end));
            }
            else
            {
                resultSb.append(cmdLine.substring(end));
            }
            end = start + 1;
        }
    }
    return resultSb.toString();
}

} // namespace PurCFetcher

#endif // ENABLE(LCMD)
