/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2011, 2012 Apple Inc. All rights reserved.
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

#include "config.h"
#include "DeprecatedGlobalSettings.h"

#include "RuntimeApplicationChecks.h"
#include <wtf/NeverDestroyed.h>

namespace PurCFetcher {

bool DeprecatedGlobalSettings::gMockScrollbarsEnabled = false;
bool DeprecatedGlobalSettings::gUsesOverlayScrollbars = false;
bool DeprecatedGlobalSettings::gMockScrollAnimatorEnabled = false;

bool DeprecatedGlobalSettings::gShouldRespectPriorityInCSSAttributeSetters = false;
bool DeprecatedGlobalSettings::gLowPowerVideoAudioBufferSizeEnabled = false;
bool DeprecatedGlobalSettings::gResourceLoadStatisticsEnabledEnabled = false;
bool DeprecatedGlobalSettings::gAllowsAnySSLCertificate = false;

bool DeprecatedGlobalSettings::gManageAudioSession = false;

// It's very important that this setting doesn't change in the middle of a document's lifetime.
// The Mac port uses this flag when registering and deregistering platform-dependent scrollbar
// objects. Therefore, if this changes at an unexpected time, deregistration may not happen
// correctly, which may cause the platform to follow dangling pointers.
void DeprecatedGlobalSettings::setMockScrollbarsEnabled(bool flag)
{
    gMockScrollbarsEnabled = flag;
    // FIXME: This should update scroll bars in existing pages.
}

bool DeprecatedGlobalSettings::mockScrollbarsEnabled()
{
    return gMockScrollbarsEnabled;
}

void DeprecatedGlobalSettings::setUsesOverlayScrollbars(bool flag)
{
    gUsesOverlayScrollbars = flag;
    // FIXME: This should update scroll bars in existing pages.
}

bool DeprecatedGlobalSettings::usesOverlayScrollbars()
{
    return gUsesOverlayScrollbars;
}

void DeprecatedGlobalSettings::setUsesMockScrollAnimator(bool flag)
{
    gMockScrollAnimatorEnabled = flag;
}

bool DeprecatedGlobalSettings::usesMockScrollAnimator()
{
    return gMockScrollAnimatorEnabled;
}

void DeprecatedGlobalSettings::setShouldRespectPriorityInCSSAttributeSetters(bool flag)
{
    gShouldRespectPriorityInCSSAttributeSetters = flag;
}

bool DeprecatedGlobalSettings::shouldRespectPriorityInCSSAttributeSetters()
{
    return gShouldRespectPriorityInCSSAttributeSetters;
}

void DeprecatedGlobalSettings::setLowPowerVideoAudioBufferSizeEnabled(bool flag)
{
    gLowPowerVideoAudioBufferSizeEnabled = flag;
}

void DeprecatedGlobalSettings::setResourceLoadStatisticsEnabled(bool flag)
{
    gResourceLoadStatisticsEnabledEnabled = flag;
}

bool DeprecatedGlobalSettings::globalConstRedeclarationShouldThrow()
{
    return true;
}

void DeprecatedGlobalSettings::setAllowsAnySSLCertificate(bool allowAnySSLCertificate)
{
    gAllowsAnySSLCertificate = allowAnySSLCertificate;
}

bool DeprecatedGlobalSettings::allowsAnySSLCertificate()
{
    return gAllowsAnySSLCertificate;
}

} // namespace PurCFetcher
