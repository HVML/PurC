/*
 * @file fetcher-messages.h
 * @author XueShuming
 * @date 2021/11/28
 * @brief The fetcher messages class.
 *
 * Copyright (C) 2021 FMSoft <https://www.fmsoft.cn>
 *
 * This file is a part of PurC (short for Purring Cat), an HVML interpreter.
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
 */

#ifndef PURC_FETCHER_MESSAGES_H
#define PURC_FETCHER_MESSAGES_H

#include "fetcher-messages-basic.h"

#include "ArgumentCoders.h"
#include "Attachment.h"
#include "Connection.h"
#include "MessageNames.h"

#include <optional>
#include <wtf/Forward.h>
#include <wtf/ThreadSafeRefCounted.h>

namespace PAL {
class SessionID;
}

namespace IPC {
class DataReference;
class FormDataReference;
class SharedBufferDataReference;
}

namespace PurCFetcher {

class NetworkResourceLoadParameters;
enum class HTTPCookieAcceptPolicy : uint8_t;
struct NetworkProcessCreationParameters;
class NetworkLoadMetrics;
class ResourceError;
class ResourceRequest;
class ResourceResponse;

}

namespace Messages {

namespace NetworkConnectionToWebProcess {

static inline IPC::ReceiverName messageReceiverName()
{
    return IPC::ReceiverName::NetworkConnectionToWebProcess;
}

class ScheduleResourceLoad {
public:
    using Arguments = std::tuple<const PurCFetcher::NetworkResourceLoadParameters&>;

    static IPC::MessageName name() { return IPC::MessageName::NetworkConnectionToWebProcess_ScheduleResourceLoad; }
    static const bool isSync = false;

    explicit ScheduleResourceLoad(const PurCFetcher::NetworkResourceLoadParameters& resourceLoadParameters)
        : m_arguments(resourceLoadParameters)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

} // namespace NetworkConnectionToWebProcess

namespace NetworkProcess {

using CreateNetworkConnectionToWebProcessDelayedReply = CompletionHandler<void(const std::optional<IPC::Attachment>& connectionIdentifier, PurCFetcher::HTTPCookieAcceptPolicy cookieAcceptPolicy)>;

static inline IPC::ReceiverName messageReceiverName()
{
    return IPC::ReceiverName::NetworkProcess;
}

class InitializeNetworkProcess {
public:
    using Arguments = std::tuple<const PurCFetcher::NetworkProcessCreationParameters&>;

    static IPC::MessageName name() { return IPC::MessageName::NetworkProcess_InitializeNetworkProcess; }
    static const bool isSync = false;

    explicit InitializeNetworkProcess(const PurCFetcher::NetworkProcessCreationParameters& processCreationParameters)
        : m_arguments(processCreationParameters)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class CreateNetworkConnectionToWebProcess {
public:
    using Arguments = std::tuple<const PurCFetcher::ProcessIdentifier&, const PAL::SessionID&>;

    static IPC::MessageName name() { return IPC::MessageName::NetworkProcess_CreateNetworkConnectionToWebProcess; }
    static const bool isSync = true;

    using DelayedReply = CreateNetworkConnectionToWebProcessDelayedReply;
    static void send(std::unique_ptr<IPC::Encoder>&&, IPC::Connection&, const std::optional<IPC::Attachment>& connectionIdentifier, PurCFetcher::HTTPCookieAcceptPolicy cookieAcceptPolicy);
    using Reply = std::tuple<std::optional<IPC::Attachment>&, PurCFetcher::HTTPCookieAcceptPolicy&>;
    using ReplyArguments = std::tuple<std::optional<IPC::Attachment>, PurCFetcher::HTTPCookieAcceptPolicy>;
    CreateNetworkConnectionToWebProcess(const PurCFetcher::ProcessIdentifier& processIdentifier, const PAL::SessionID& sessionID)
        : m_arguments(processIdentifier, sessionID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

} // namespace NetworkProcess

namespace NetworkResourceLoader {

static inline IPC::ReceiverName messageReceiverName()
{
    return IPC::ReceiverName::NetworkResourceLoader;
}

class ContinueWillSendRequest {
public:
    using Arguments = std::tuple<const PurCFetcher::ResourceRequest&, bool>;

    static IPC::MessageName name() { return IPC::MessageName::NetworkResourceLoader_ContinueWillSendRequest; }
    static const bool isSync = false;

    ContinueWillSendRequest(const PurCFetcher::ResourceRequest& request, bool isAllowedToAskUserForCredentials)
        : m_arguments(request, isAllowedToAskUserForCredentials)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ContinueDidReceiveResponse {
public:
    using Arguments = std::tuple<>;

    static IPC::MessageName name() { return IPC::MessageName::NetworkResourceLoader_ContinueDidReceiveResponse; }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

} // namespace NetworkResourceLoader

namespace WebResourceLoader {

static inline IPC::ReceiverName messageReceiverName()
{
    return IPC::ReceiverName::WebResourceLoader;
}

class WillSendRequest {
public:
    using Arguments = std::tuple<const PurCFetcher::ResourceRequest&, const IPC::FormDataReference&, const PurCFetcher::ResourceResponse&>;

    static IPC::MessageName name() { return IPC::MessageName::WebResourceLoader_WillSendRequest; }
    static const bool isSync = false;

    WillSendRequest(const PurCFetcher::ResourceRequest& request, const IPC::FormDataReference& requestBody, const PurCFetcher::ResourceResponse& redirectResponse)
        : m_arguments(request, requestBody, redirectResponse)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidSendData {
public:
    using Arguments = std::tuple<uint64_t, uint64_t>;

    static IPC::MessageName name() { return IPC::MessageName::WebResourceLoader_DidSendData; }
    static const bool isSync = false;

    DidSendData(uint64_t bytesSent, uint64_t totalBytesToBeSent)
        : m_arguments(bytesSent, totalBytesToBeSent)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidReceiveResponse {
public:
    using Arguments = std::tuple<const PurCFetcher::ResourceResponse&, bool>;

    static IPC::MessageName name() { return IPC::MessageName::WebResourceLoader_DidReceiveResponse; }
    static const bool isSync = false;

    DidReceiveResponse(const PurCFetcher::ResourceResponse& response, bool needsContinueDidReceiveResponseMessage)
        : m_arguments(response, needsContinueDidReceiveResponseMessage)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidReceiveData {
public:
    using Arguments = std::tuple<const IPC::DataReference&, int64_t>;

    static IPC::MessageName name() { return IPC::MessageName::WebResourceLoader_DidReceiveData; }
    static const bool isSync = false;

    DidReceiveData(const IPC::DataReference& data, int64_t encodedDataLength)
        : m_arguments(data, encodedDataLength)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidReceiveSharedBuffer {
public:
    using Arguments = std::tuple<const IPC::SharedBufferDataReference&, int64_t>;

    static IPC::MessageName name() { return IPC::MessageName::WebResourceLoader_DidReceiveSharedBuffer; }
    static const bool isSync = false;

    DidReceiveSharedBuffer(const IPC::SharedBufferDataReference& data, int64_t encodedDataLength)
        : m_arguments(data, encodedDataLength)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFinishResourceLoad {
public:
    using Arguments = std::tuple<const PurCFetcher::NetworkLoadMetrics&>;

    static IPC::MessageName name() { return IPC::MessageName::WebResourceLoader_DidFinishResourceLoad; }
    static const bool isSync = false;

    explicit DidFinishResourceLoad(const PurCFetcher::NetworkLoadMetrics& networkLoadMetrics)
        : m_arguments(networkLoadMetrics)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFailResourceLoad {
public:
    using Arguments = std::tuple<const PurCFetcher::ResourceError&>;

    static IPC::MessageName name() { return IPC::MessageName::WebResourceLoader_DidFailResourceLoad; }
    static const bool isSync = false;

    explicit DidFailResourceLoad(const PurCFetcher::ResourceError& error)
        : m_arguments(error)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFailServiceWorkerLoad {
public:
    using Arguments = std::tuple<const PurCFetcher::ResourceError&>;

    static IPC::MessageName name() { return IPC::MessageName::WebResourceLoader_DidFailServiceWorkerLoad; }
    static const bool isSync = false;

    explicit DidFailServiceWorkerLoad(const PurCFetcher::ResourceError& error)
        : m_arguments(error)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ServiceWorkerDidNotHandle {
public:
    using Arguments = std::tuple<>;

    static IPC::MessageName name() { return IPC::MessageName::WebResourceLoader_ServiceWorkerDidNotHandle; }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidBlockAuthenticationChallenge {
public:
    using Arguments = std::tuple<>;

    static IPC::MessageName name() { return IPC::MessageName::WebResourceLoader_DidBlockAuthenticationChallenge; }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class StopLoadingAfterXFrameOptionsOrContentSecurityPolicyDenied {
public:
    using Arguments = std::tuple<const PurCFetcher::ResourceResponse&>;

    static IPC::MessageName name() { return IPC::MessageName::WebResourceLoader_StopLoadingAfterXFrameOptionsOrContentSecurityPolicyDenied; }
    static const bool isSync = false;

    explicit StopLoadingAfterXFrameOptionsOrContentSecurityPolicyDenied(const PurCFetcher::ResourceResponse& response)
        : m_arguments(response)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

} // namespace WebResourceLoader


} // namespace Messages

#endif /* not defined PURC_FETCHER_MESSAGES_H */


