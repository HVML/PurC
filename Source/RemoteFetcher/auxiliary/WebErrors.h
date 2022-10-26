/*
 * Copyright (C) 2010-2016 Apple Inc. All rights reserved.
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

#pragma once

#include <wtf/Forward.h>

namespace PurCFetcher {
class ResourceError;
class ResourceRequest;
class ResourceResponse;
}

namespace PurCFetcher {

PurCFetcher::ResourceError cancelledError(const PurCFetcher::ResourceRequest&);
PurCFetcher::ResourceError blockedError(const PurCFetcher::ResourceRequest&);
PurCFetcher::ResourceError blockedByContentBlockerError(const PurCFetcher::ResourceRequest&);
PurCFetcher::ResourceError cannotShowURLError(const PurCFetcher::ResourceRequest&);
PurCFetcher::ResourceError wasBlockedByRestrictionsError(const PurCFetcher::ResourceRequest&);
PurCFetcher::ResourceError interruptedForPolicyChangeError(const PurCFetcher::ResourceRequest&);
PurCFetcher::ResourceError failedCustomProtocolSyncLoad(const PurCFetcher::ResourceRequest&);
PurCFetcher::ResourceError cannotShowMIMETypeError(const PurCFetcher::ResourceResponse&);
PurCFetcher::ResourceError fileDoesNotExistError(const PurCFetcher::ResourceResponse&);
PurCFetcher::ResourceError pluginWillHandleLoadError(const PurCFetcher::ResourceResponse&);
PurCFetcher::ResourceError internalError(const URL&);

#if USE(SOUP)
PurCFetcher::ResourceError downloadNetworkError(const URL&, const PurCWTF::String&);
PurCFetcher::ResourceError downloadCancelledByUserError(const PurCFetcher::ResourceResponse&);
PurCFetcher::ResourceError downloadDestinationError(const PurCFetcher::ResourceResponse&, const PurCWTF::String&);
#endif

} // namespace PurCFetcher
