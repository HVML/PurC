/*
 * Copyright (C) 2003-2016 Apple Inc. All rights reserved.
 *           (C) 2006 Graham Dennis (graham.dennis@gmail.com)
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

#include <wtf/Forward.h>

namespace PurCFetcher {

class DeprecatedGlobalSettings {
public:
    DeprecatedGlobalSettings() = delete;

    static bool isPostLoadCPUUsageMeasurementEnabled();
    static bool isPostBackgroundingCPUUsageMeasurementEnabled();
    static bool isPerActivityStateCPUUsageMeasurementEnabled();

    static bool isPostLoadMemoryUsageMeasurementEnabled();
    static bool isPostBackgroundingMemoryUsageMeasurementEnabled();

    static bool globalConstRedeclarationShouldThrow();

    PURCFETCHER_EXPORT static void setMockScrollbarsEnabled(bool flag);
    PURCFETCHER_EXPORT static bool mockScrollbarsEnabled();

    PURCFETCHER_EXPORT static void setUsesOverlayScrollbars(bool flag);
    static bool usesOverlayScrollbars();

    PURCFETCHER_EXPORT static void setUsesMockScrollAnimator(bool);
    static bool usesMockScrollAnimator();

    PURCFETCHER_EXPORT static void setShouldRespectPriorityInCSSAttributeSetters(bool);
    static bool shouldRespectPriorityInCSSAttributeSetters();

    static bool lowPowerVideoAudioBufferSizeEnabled() { return gLowPowerVideoAudioBufferSizeEnabled; }
    PURCFETCHER_EXPORT static void setLowPowerVideoAudioBufferSizeEnabled(bool);

    static bool resourceLoadStatisticsEnabled() { return gResourceLoadStatisticsEnabledEnabled; }
    PURCFETCHER_EXPORT static void setResourceLoadStatisticsEnabled(bool);

    PURCFETCHER_EXPORT static void setAllowsAnySSLCertificate(bool);
    static bool allowsAnySSLCertificate();

private:
    static bool gMockScrollbarsEnabled;
    static bool gUsesOverlayScrollbars;
    static bool gMockScrollAnimatorEnabled;

    static bool gShouldRespectPriorityInCSSAttributeSetters;
    PURCFETCHER_EXPORT static bool gManageAudioSession;

    static bool gLowPowerVideoAudioBufferSizeEnabled;
    PURCFETCHER_EXPORT static bool gResourceLoadStatisticsEnabledEnabled;
    static bool gAllowsAnySSLCertificate;
};

inline bool DeprecatedGlobalSettings::isPostLoadCPUUsageMeasurementEnabled()
{
    return false;
}

inline bool DeprecatedGlobalSettings::isPostBackgroundingCPUUsageMeasurementEnabled()
{
    return false;
}

inline bool DeprecatedGlobalSettings::isPerActivityStateCPUUsageMeasurementEnabled()
{
    return false;
}

inline bool DeprecatedGlobalSettings::isPostLoadMemoryUsageMeasurementEnabled()
{
    return false;
}

inline bool DeprecatedGlobalSettings::isPostBackgroundingMemoryUsageMeasurementEnabled()
{
    return false;
}

} // namespace PurCFetcher
