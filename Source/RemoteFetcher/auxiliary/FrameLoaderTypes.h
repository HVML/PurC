/*
 * Copyright (C) 2006-2020 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "ProcessIdentifier.h"
#include <wtf/EnumTraits.h>

namespace PurCFetcher {

enum FrameState {
    FrameStateProvisional,
    // This state indicates we are ready to commit to a page,
    // which means the view will transition to use the new data source.
    FrameStateCommittedPage,
    FrameStateComplete
};

enum class PolicyAction : uint8_t {
    Use,
    Download,
    Ignore,
    StopAllLoads
};

enum class ReloadOption : uint8_t {
    ExpiredOnly = 1 << 0,
    FromOrigin  = 1 << 1,
    DisableContentBlockers = 1 << 2,
};

enum class FrameLoadType : uint8_t {
    Standard,
    Back,
    Forward,
    IndexedBackForward, // a multi-item hop in the backforward list
    Reload,
    Same, // user loads same URL again (but not reload button)
    RedirectWithLockedBackForwardList, // FIXME: Merge "lockBackForwardList", "lockHistory", "quickRedirect" and "clientRedirect" into a single concept of redirect.
    Replace,
    ReloadFromOrigin,
    ReloadExpiredOnly
};

enum class WillContinueLoading : bool { No, Yes };

class PolicyCheckIdentifier {
public:
    PolicyCheckIdentifier() = default;

    static PolicyCheckIdentifier create();

    bool isValidFor(PolicyCheckIdentifier);
    bool operator==(const PolicyCheckIdentifier& other) const { return m_process == other.m_process && m_policyCheck == other.m_policyCheck; }

    template<class Encoder> void encode(Encoder&) const;
    template<class Decoder> static std::optional<PolicyCheckIdentifier> decode(Decoder&);

private:
    PolicyCheckIdentifier(ProcessIdentifier process, uint64_t policyCheck)
        : m_process(process)
        , m_policyCheck(policyCheck)
    { }

    ProcessIdentifier m_process;
    uint64_t m_policyCheck { 0 };
};

template<class Encoder>
void PolicyCheckIdentifier::encode(Encoder& encoder) const
{
    encoder << m_process << m_policyCheck;
}

template<class Decoder>
std::optional<PolicyCheckIdentifier> PolicyCheckIdentifier::decode(Decoder& decoder)
{
    auto process = ProcessIdentifier::decode(decoder);
    if (!process)
        return std::nullopt;

    uint64_t policyCheck;
    if (!decoder.decode(policyCheck))
        return std::nullopt;

    return PolicyCheckIdentifier { *process, policyCheck };
}

enum class ShouldContinuePolicyCheck : bool {
    Yes,
    No
};

enum class NewFrameOpenerPolicy : uint8_t {
    Suppress,
    Allow
};

enum class NavigationType : uint8_t {
    LinkClicked,
    FormSubmitted,
    BackForward,
    Reload,
    FormResubmitted,
    Other
};

enum class ShouldOpenExternalURLsPolicy : uint8_t {
    ShouldNotAllow,
    ShouldAllowExternalSchemes,
    ShouldAllow,
};

enum class InitiatedByMainFrame : uint8_t {
    Yes,
    Unknown,
};

enum ClearProvisionalItemPolicy {
    ShouldClearProvisionalItem,
    ShouldNotClearProvisionalItem
};

enum class StopLoadingPolicy {
    PreventDuringUnloadEvents,
    AlwaysStopLoading
};

enum class ObjectContentType : uint8_t {
    None,
    Image,
    Frame,
    PlugIn,
};

enum UnloadEventPolicy {
    UnloadEventPolicyNone,
    UnloadEventPolicyUnloadOnly,
    UnloadEventPolicyUnloadAndPageHide
};

// Passed to FrameLoader::urlSelected() and ScriptController::executeIfJavaScriptURL()
// to control whether, in the case of a JavaScript URL, executeIfJavaScriptURL() should
// replace the document. It is a FIXME to eliminate this extra parameter from
// executeIfJavaScriptURL(), in which case this enum can go away.
enum ShouldReplaceDocumentIfJavaScriptURL {
    ReplaceDocumentIfJavaScriptURL,
    DoNotReplaceDocumentIfJavaScriptURL
};

enum class WebGLLoadPolicy : uint8_t {
    WebGLBlockCreation,
    WebGLAllowCreation,
    WebGLPendingCreation
};

enum class LockHistory : bool { No, Yes };
enum class LockBackForwardList : bool { No, Yes };
enum class AllowNavigationToInvalidURL : bool { No, Yes };
enum class HasInsecureContent : bool { No, Yes };

enum class LoadCompletionType : bool {
    Finish,
    Cancel
};

enum class AllowsContentJavaScript : bool {
    No,
    Yes,
};

} // namespace PurCFetcher

namespace PurCWTF {

template<> struct EnumTraits<PurCFetcher::FrameLoadType> {
    using values = EnumValues<
        PurCFetcher::FrameLoadType,
        PurCFetcher::FrameLoadType::Standard,
        PurCFetcher::FrameLoadType::Back,
        PurCFetcher::FrameLoadType::Forward,
        PurCFetcher::FrameLoadType::IndexedBackForward,
        PurCFetcher::FrameLoadType::Reload,
        PurCFetcher::FrameLoadType::Same,
        PurCFetcher::FrameLoadType::RedirectWithLockedBackForwardList,
        PurCFetcher::FrameLoadType::Replace,
        PurCFetcher::FrameLoadType::ReloadFromOrigin,
        PurCFetcher::FrameLoadType::ReloadExpiredOnly
    >;
};

template<> struct EnumTraits<PurCFetcher::NavigationType> {
    using values = EnumValues<
        PurCFetcher::NavigationType,
        PurCFetcher::NavigationType::LinkClicked,
        PurCFetcher::NavigationType::FormSubmitted,
        PurCFetcher::NavigationType::BackForward,
        PurCFetcher::NavigationType::Reload,
        PurCFetcher::NavigationType::FormResubmitted,
        PurCFetcher::NavigationType::Other
    >;
};

template<> struct EnumTraits<PurCFetcher::PolicyAction> {
    using values = EnumValues<
        PurCFetcher::PolicyAction,
        PurCFetcher::PolicyAction::Use,
        PurCFetcher::PolicyAction::Download,
        PurCFetcher::PolicyAction::Ignore,
        PurCFetcher::PolicyAction::StopAllLoads
    >;
};

template<> struct EnumTraits<PurCFetcher::ShouldOpenExternalURLsPolicy> {
    using values = EnumValues<
        PurCFetcher::ShouldOpenExternalURLsPolicy,
        PurCFetcher::ShouldOpenExternalURLsPolicy::ShouldNotAllow,
        PurCFetcher::ShouldOpenExternalURLsPolicy::ShouldAllowExternalSchemes,
        PurCFetcher::ShouldOpenExternalURLsPolicy::ShouldAllow
    >;
};

template<> struct EnumTraits<PurCFetcher::WebGLLoadPolicy> {
    using values = EnumValues<
        PurCFetcher::WebGLLoadPolicy,
        PurCFetcher::WebGLLoadPolicy::WebGLBlockCreation,
        PurCFetcher::WebGLLoadPolicy::WebGLAllowCreation,
        PurCFetcher::WebGLLoadPolicy::WebGLPendingCreation
    >;
};

} // namespace PurCWTF
