/*
 * Copyright (C) 2012-2018 Apple Inc. All rights reserved.
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

#include <wtf/text/WTFString.h>

namespace PurCFetcher {

class DiagnosticLoggingKeys {
public:
    PURCFETCHER_EXPORT static String activeInForegroundTabKey();
    PURCFETCHER_EXPORT static String activeInBackgroundTabOnlyKey();
    static String applicationCacheKey();
#if ENABLE(APPLICATION_MANIFEST)
    static String applicationManifestKey();
#endif
    static String audioKey();
    PURCFETCHER_EXPORT static String backNavigationDeltaKey();
    PURCFETCHER_EXPORT static String cacheControlNoStoreKey();
    static String cachedResourceRevalidationKey();
    static String cachedResourceRevalidationReasonKey();
    static String canCacheKey();
    PURCFETCHER_EXPORT static String canceledLessThan2SecondsKey();
    PURCFETCHER_EXPORT static String canceledLessThan5SecondsKey();
    PURCFETCHER_EXPORT static String canceledLessThan20SecondsKey();
    PURCFETCHER_EXPORT static String canceledMoreThan20SecondsKey();
    static String cannotSuspendActiveDOMObjectsKey();
    PURCFETCHER_EXPORT static String cpuUsageKey();
    PURCFETCHER_EXPORT static String createSharedBufferFailedKey();
    static String deniedByClientKey();
    static String deviceMotionKey();
    static String deviceOrientationKey();
    static String diskCacheKey();
    static String diskCacheAfterValidationKey();
    static String documentLoaderStoppingKey();
    PURCFETCHER_EXPORT static String domainCausingCrashKey();
    static String domainCausingEnergyDrainKey();
    PURCFETCHER_EXPORT static String domainCausingJetsamKey();
    PURCFETCHER_EXPORT static String simulatedPageCrashKey();
    PURCFETCHER_EXPORT static String exceededActiveMemoryLimitKey();
    PURCFETCHER_EXPORT static String exceededInactiveMemoryLimitKey();
    PURCFETCHER_EXPORT static String exceededBackgroundCPULimitKey();
    static String domainVisitedKey();
    static String engineFailedToLoadKey();
    PURCFETCHER_EXPORT static String entryRightlyNotWarmedUpKey();
    PURCFETCHER_EXPORT static String entryWronglyNotWarmedUpKey();
    static String expiredKey();
    PURCFETCHER_EXPORT static String failedLessThan2SecondsKey();
    PURCFETCHER_EXPORT static String failedLessThan5SecondsKey();
    PURCFETCHER_EXPORT static String failedLessThan20SecondsKey();
    PURCFETCHER_EXPORT static String failedMoreThan20SecondsKey();
    static String fontKey();
    static String hasPluginsKey();
    static String httpsNoStoreKey();
    static String imageKey();
    static String inMemoryCacheKey();
    PURCFETCHER_EXPORT static String inactiveKey();
    PURCFETCHER_EXPORT static String internalErrorKey();
    PURCFETCHER_EXPORT static String invalidSessionIDKey();
    PURCFETCHER_EXPORT static String isAttachmentKey();
    PURCFETCHER_EXPORT static String isConditionalRequestKey();
    static String isDisabledKey();
    static String isErrorPageKey();
    static String isExpiredKey();
    PURCFETCHER_EXPORT static String isReloadIgnoringCacheDataKey();
    static String loadingKey();
    static String isLoadingKey();
    static String mainResourceKey();
    static String mediaLoadedKey();
    static String mediaLoadingFailedKey();
    static String memoryCacheEntryDecisionKey();
    static String memoryCacheUsageKey();
    PURCFETCHER_EXPORT static String missingValidatorFieldsKey();
    static String navigationKey();
    PURCFETCHER_EXPORT static String needsRevalidationKey();
    PURCFETCHER_EXPORT static String networkCacheKey();
    PURCFETCHER_EXPORT static String networkCacheFailureReasonKey();
    PURCFETCHER_EXPORT static String networkCacheUnusedReasonKey();
    PURCFETCHER_EXPORT static String networkCacheReuseFailureKey();
    static String networkKey();
    PURCFETCHER_EXPORT static String networkProcessCrashedKey();
    PURCFETCHER_EXPORT static String neverSeenBeforeKey();
    static String noKey();
    static String noCacheKey();
    static String noCurrentHistoryItemKey();
    static String noDocumentLoaderKey();
    PURCFETCHER_EXPORT static String noLongerInCacheKey();
    static String noStoreKey();
    PURCFETCHER_EXPORT static String nonVisibleStateKey();
    PURCFETCHER_EXPORT static String notHTTPFamilyKey();
    static String notInMemoryCacheKey();
    PURCFETCHER_EXPORT static String occurredKey();
    PURCFETCHER_EXPORT static String otherKey();
    static String backForwardCacheKey();
    static String backForwardCacheFailureKey();
    static String visuallyEmptyKey();
    static String pageContainsAtLeastOneMediaEngineKey();
    static String pageContainsAtLeastOnePluginKey();
    static String pageContainsMediaEngineKey();
    static String pageContainsPluginKey();
    static String pageHandlesWebGLContextLossKey();
    static String pageLoadedKey();
    static String playedKey();
    static String pluginLoadedKey();
    static String pluginLoadingFailedKey();
    static String postPageBackgroundingCPUUsageKey();
    static String postPageBackgroundingMemoryUsageKey();
    static String postPageLoadCPUUsageKey();
    static String postPageLoadMemoryUsageKey();
    static String provisionalLoadKey();
    static String prunedDueToMaxSizeReached();
    static String prunedDueToMemoryPressureKey();
    static String prunedDueToProcessSuspended();
    static String quirkRedirectComingKey();
    static String rawKey();
    static String redirectKey();
    static String reloadFromOriginKey();
    static String reloadKey();
    static String reloadRevalidatingExpiredKey();
    static String replaceKey();
    static String resourceLoadedKey();
    static String resourceResponseSourceKey();
    PURCFETCHER_EXPORT static String retrievalKey();
    PURCFETCHER_EXPORT static String retrievalRequestKey();
    PURCFETCHER_EXPORT static String revalidatingKey();
    static String sameLoadKey();
    static String scriptKey();
    static String serviceWorkerKey();
    static String siteSpecificQuirkKey();
    PURCFETCHER_EXPORT static String streamingMedia();
    static String styleSheetKey();
    PURCFETCHER_EXPORT static String succeededLessThan2SecondsKey();
    PURCFETCHER_EXPORT static String succeededLessThan5SecondsKey();
    PURCFETCHER_EXPORT static String succeededLessThan20SecondsKey();
    PURCFETCHER_EXPORT static String succeededMoreThan20SecondsKey();
    PURCFETCHER_EXPORT static String successfulSpeculativeWarmupWithRevalidationKey();
    PURCFETCHER_EXPORT static String successfulSpeculativeWarmupWithoutRevalidationKey();
    static String svgDocumentKey();
    PURCFETCHER_EXPORT static String synchronousMessageFailedKey();
    PURCFETCHER_EXPORT static String telemetryPageLoadKey();
    PURCFETCHER_EXPORT static String timedOutKey();
    PURCFETCHER_EXPORT static String uncacheableStatusCodeKey();
    static String underMemoryPressureKey();
    PURCFETCHER_EXPORT static String unknownEntryRequestKey();
    PURCFETCHER_EXPORT static String unlikelyToReuseKey();
    PURCFETCHER_EXPORT static String unsupportedHTTPMethodKey();
    static String unsuspendableDOMObjectKey();
    PURCFETCHER_EXPORT static String unusedKey();
    static String unusedReasonCredentialSettingsKey();
    static String unusedReasonErrorKey();
    static String unusedReasonMustRevalidateNoValidatorKey();
    static String unusedReasonNoStoreKey();
    static String unusedReasonRedirectChainKey();
    static String unusedReasonReloadKey();
    static String unusedReasonTypeMismatchKey();
    static String usedKey();
    PURCFETCHER_EXPORT static String userZoomActionKey();
    PURCFETCHER_EXPORT static String varyingHeaderMismatchKey();
    static String videoKey();
    PURCFETCHER_EXPORT static String visibleNonActiveStateKey();
    PURCFETCHER_EXPORT static String visibleAndActiveStateKey();
    PURCFETCHER_EXPORT static String wastedSpeculativeWarmupWithRevalidationKey();
    PURCFETCHER_EXPORT static String wastedSpeculativeWarmupWithoutRevalidationKey();
    PURCFETCHER_EXPORT static String webGLStateKey();
    PURCFETCHER_EXPORT static String webViewKey();
    static String yesKey();

    PURCFETCHER_EXPORT static String memoryUsageToDiagnosticLoggingKey(uint64_t memoryUsage);
    PURCFETCHER_EXPORT static String foregroundCPUUsageToDiagnosticLoggingKey(double cpuUsage);
    PURCFETCHER_EXPORT static String backgroundCPUUsageToDiagnosticLoggingKey(double cpuUsage);
    
    PURCFETCHER_EXPORT static String resourceLoadStatisticsTelemetryKey();
};

} // namespace PurCFetcher
