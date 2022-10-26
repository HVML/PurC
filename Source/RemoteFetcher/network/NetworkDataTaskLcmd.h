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

#pragma once

#if ENABLE(LCMD)

#include "NetworkDataTask.h"
#include "NetworkLoadMetrics.h"
#include "ProtectionSpace.h"
#include "ResourceResponse.h"
#include <wtf/RunLoop.h>
#include <wtf/glib/GRefPtr.h>
#include "CmdFilterManager.h"

namespace PurCFetcher {

class NetworkDataTaskLcmd final : public NetworkDataTask {
public:
    static Ref<NetworkDataTask> create(NetworkSession& session, NetworkDataTaskClient& client, const PurCFetcher::ResourceRequest& request, PurCFetcher::StoredCredentialsPolicy storedCredentialsPolicy, PurCFetcher::ContentSniffingPolicy shouldContentSniff, PurCFetcher::ContentEncodingSniffingPolicy shouldContentEncodingSniff, bool shouldClearReferrerOnHTTPSToHTTPRedirect, bool dataTaskIsForMainFrameNavigation)
    {
        return adoptRef(*new NetworkDataTaskLcmd(session, client, request, storedCredentialsPolicy, shouldContentSniff, shouldContentEncodingSniff, shouldClearReferrerOnHTTPSToHTTPRedirect, dataTaskIsForMainFrameNavigation));
    }

    ~NetworkDataTaskLcmd();

private:
    NetworkDataTaskLcmd(NetworkSession&, NetworkDataTaskClient&, const PurCFetcher::ResourceRequest&, PurCFetcher::StoredCredentialsPolicy, PurCFetcher::ContentSniffingPolicy, PurCFetcher::ContentEncodingSniffingPolicy, bool shouldClearReferrerOnHTTPSToHTTPRedirect, bool dataTaskIsForMainFrameNavigation);

    String suggestedFilename() const override;
    void setPendingDownloadLocation(const String&, SandboxExtension::Handle&&, bool /*allowOverwrite*/) override;

    void cancel() override;
    void resume() override;
    void invalidateAndCancel() override;
    NetworkDataTask::State state() const override;

    void dispatchDidCompleteWithError(const PurCFetcher::ResourceError&);
    void dispatchDidReceiveResponse();
    void createRequest(PurCFetcher::ResourceRequest&&);
    void sendRequest();

    void runCmdInner();
    void runCmdOuter();
    void buildResponse();

    void parseQueryString(String query);
    void parseCmdFilter(String cmdFilter);
    String parseCmdLine(String cmdLine);
private:
    State m_state { State::Suspended };
    PurCFetcher::ResourceRequest m_currentRequest;
    PurCFetcher::ResourceResponse m_response;

    MonotonicTime m_startTime;
    PurCFetcher::NetworkLoadMetrics m_networkLoadMetrics;
    Vector<char> m_readBuffer;
    Vector<char> m_responseBuffer;
    Vector<String> m_readLines;

    String m_errorMsg;
    int m_statusCode;
    int m_exitCode;

    RefPtr<CmdFilterManager> m_filterManager;

    HashMap<String, String> m_paramMap;

    String m_cmdFilter;
    String m_cmdLine;
};

} // namespace PurCFetcher

#endif // ENABLE(LCMD)
