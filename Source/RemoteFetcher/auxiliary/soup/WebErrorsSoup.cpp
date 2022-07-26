/*
 * Copyright (C) 2017 Igalia S.L.
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
#include "WebErrors.h"

#include "APIError.h"
#include "ResourceError.h"
#include "ResourceResponse.h"

#define WEB_UI_STRING(string, description)  string

namespace PurCFetcher {
using namespace PurCFetcher;

ResourceError downloadNetworkError(const URL& failingURL, const String& localizedDescription)
{
    return ResourceError(API::Error::webKitDownloadErrorDomain(), API::Error::Download::Transport, failingURL, localizedDescription);
}

ResourceError downloadCancelledByUserError(const ResourceResponse& response)
{
    return ResourceError(API::Error::webKitDownloadErrorDomain(), API::Error::Download::CancelledByUser, response.url(), WEB_UI_STRING("User cancelled the download", "The download was cancelled by the user"));
}

ResourceError downloadDestinationError(const ResourceResponse& response, const String& localizedDescription)
{
    return ResourceError(API::Error::webKitDownloadErrorDomain(), API::Error::Download::Destination, response.url(), localizedDescription);
}

} // namespace PurCFetcher
