/*
 * Copyright (C) 2010-2020 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <wtf/EnumTraits.h>

namespace IPC {

enum class ReceiverName : uint8_t {
    GPUConnectionToWebProcess = 1
    , GPUProcess = 2
    , RemoteRenderingBackendProxy = 3
    , RemoteAudioDestinationManager = 4
    , RemoteAudioSessionProxy = 5
    , RemoteCDMFactoryProxy = 6
    , RemoteCDMInstanceProxy = 7
    , RemoteCDMInstanceSessionProxy = 8
    , RemoteCDMProxy = 9
    , RemoteLegacyCDMFactoryProxy = 10
    , RemoteLegacyCDMProxy = 11
    , RemoteLegacyCDMSessionProxy = 12
    , RemoteMediaPlayerManagerProxy = 13
    , RemoteMediaPlayerProxy = 14
    , RemoteMediaResourceManager = 15
    , LibWebRTCCodecsProxy = 16
    , RemoteAudioMediaStreamTrackRenderer = 17
    , RemoteAudioMediaStreamTrackRendererManager = 18
    , RemoteMediaRecorder = 19
    , RemoteMediaRecorderManager = 20
    , RemoteSampleBufferDisplayLayer = 21
    , RemoteSampleBufferDisplayLayerManager = 22
    , WebCookieManager = 23
    , WebIDBServer = 24
    , NetworkConnectionToWebProcess = 25
    , NetworkContentRuleListManager = 26
    , NetworkProcess = 27
    , NetworkResourceLoader = 28
    , NetworkSocketChannel = 29
    , NetworkSocketStream = 30
    , ServiceWorkerFetchTask = 31
    , WebSWServerConnection = 32
    , WebSWServerToContextConnection = 33
    , StorageManagerSet = 34
    , CacheStorageEngineConnection = 35
    , NetworkMDNSRegister = 36
    , NetworkRTCMonitor = 37
    , NetworkRTCProvider = 38
    , NetworkRTCSocket = 39
    , PluginControllerProxy = 40
    , PluginProcess = 41
    , WebProcessConnection = 42
    , AuxiliaryProcess = 43
    , WebConnection = 44
    , AuthenticationManager = 45
    , NPObjectMessageReceiver = 46
    , DrawingAreaProxy = 47
    , VisitedLinkStore = 48
    , WebCookieManagerProxy = 49
    , WebFullScreenManagerProxy = 50
    , WebGeolocationManagerProxy = 51
    , WebPageProxy = 52
    , WebPasteboardProxy = 53
    , WebProcessPool = 54
    , WebProcessProxy = 55
    , WebAutomationSession = 56
    , DownloadProxy = 57
    , GPUProcessProxy = 58
    , RemoteWebInspectorProxy = 59
    , WebInspectorProxy = 60
    , NetworkProcessProxy = 61
    , PluginProcessProxy = 62
    , WebUserContentControllerProxy = 63
    , WebProcess = 64
    , WebAutomationSessionProxy = 65
    , WebIDBConnectionToServer = 66
    , WebFullScreenManager = 67
    , GPUProcessConnection = 68
    , RemoteRenderingBackend = 69
    , MediaPlayerPrivateRemote = 70
    , RemoteAudioDestinationProxy = 71
    , RemoteAudioSession = 72
    , RemoteCDMInstanceSession = 73
    , RemoteLegacyCDMSession = 74
    , RemoteMediaPlayerManager = 75
    , LibWebRTCCodecs = 76
    , SampleBufferDisplayLayer = 77
    , WebGeolocationManager = 78
    , RemoteWebInspectorUI = 79
    , WebInspector = 80
    , WebInspectorInterruptDispatcher = 81
    , WebInspectorUI = 82
    , LibWebRTCNetwork = 83
    , WebMDNSRegister = 84
    , WebRTCMonitor = 85
    , WebRTCResolver = 86
    , NetworkProcessConnection = 87
    , WebResourceLoader = 88
    , WebSocketChannel = 89
    , WebSocketStream = 90
    , WebNotificationManager = 91
    , PluginProcessConnection = 92
    , PluginProcessConnectionManager = 93
    , PluginProxy = 94
    , WebSWClientConnection = 95
    , WebSWContextManagerConnection = 96
    , WebUserContentController = 97
    , DrawingArea = 98
    , EventDispatcher = 99
    , VisitedLinkTableController = 100
    , WebPage = 101
    , StorageAreaMap = 102
    , ViewGestureController = 103
    , ViewGestureGeometryCollector = 104
    , IPC = 105
    , AsyncReply = 106
    , Invalid = 107
};

enum class MessageName : uint16_t {
    GPUConnectionToWebProcess_CreateRenderingBackend = 1
    , GPUConnectionToWebProcess_ReleaseRenderingBackend = 2
    , GPUConnectionToWebProcess_ClearNowPlayingInfo = 3
    , GPUConnectionToWebProcess_SetNowPlayingInfo = 4
#if USE(AUDIO_SESSION)
    , GPUConnectionToWebProcess_EnsureAudioSession = 5
#endif
#if PLATFORM(IOS_FAMILY)
    , GPUConnectionToWebProcess_EnsureMediaSessionHelper = 6
#endif
    , GPUProcess_InitializeGPUProcess = 7
    , GPUProcess_CreateGPUConnectionToWebProcess = 8
    , GPUProcess_CreateGPUConnectionToWebProcessReply = 9
    , GPUProcess_ProcessDidTransitionToForeground = 10
    , GPUProcess_ProcessDidTransitionToBackground = 11
    , GPUProcess_AddSession = 12
    , GPUProcess_RemoveSession = 13
#if ENABLE(MEDIA_STREAM)
    , GPUProcess_SetMockCaptureDevicesEnabled = 14
#endif
#if ENABLE(MEDIA_STREAM)
    , GPUProcess_SetOrientationForMediaCapture = 15
#endif
#if ENABLE(MEDIA_STREAM)
    , GPUProcess_UpdateCaptureAccess = 16
    , GPUProcess_UpdateCaptureAccessReply = 17
#endif
    , RemoteRenderingBackendProxy_CreateImageBuffer = 18
    , RemoteRenderingBackendProxy_ReleaseImageBuffer = 19
    , RemoteRenderingBackendProxy_FlushImageBufferDrawingContext = 20
    , RemoteRenderingBackendProxy_FlushImageBufferDrawingContextAndCommit = 21
    , RemoteRenderingBackendProxy_GetImageData = 22
    , RemoteAudioDestinationManager_CreateAudioDestination = 23
    , RemoteAudioDestinationManager_DeleteAudioDestination = 24
    , RemoteAudioDestinationManager_DeleteAudioDestinationReply = 25
    , RemoteAudioDestinationManager_StartAudioDestination = 26
    , RemoteAudioDestinationManager_StopAudioDestination = 27
    , RemoteAudioSessionProxy_SetCategory = 28
    , RemoteAudioSessionProxy_SetPreferredBufferSize = 29
    , RemoteAudioSessionProxy_TryToSetActive = 30
    , RemoteCDMFactoryProxy_CreateCDM = 31
    , RemoteCDMFactoryProxy_SupportsKeySystem = 32
    , RemoteCDMInstanceProxy_CreateSession = 33
    , RemoteCDMInstanceProxy_InitializeWithConfiguration = 34
    , RemoteCDMInstanceProxy_InitializeWithConfigurationReply = 35
    , RemoteCDMInstanceProxy_SetServerCertificate = 36
    , RemoteCDMInstanceProxy_SetServerCertificateReply = 37
    , RemoteCDMInstanceProxy_SetStorageDirectory = 38
    , RemoteCDMInstanceSessionProxy_RequestLicense = 39
    , RemoteCDMInstanceSessionProxy_RequestLicenseReply = 40
    , RemoteCDMInstanceSessionProxy_UpdateLicense = 41
    , RemoteCDMInstanceSessionProxy_UpdateLicenseReply = 42
    , RemoteCDMInstanceSessionProxy_LoadSession = 43
    , RemoteCDMInstanceSessionProxy_LoadSessionReply = 44
    , RemoteCDMInstanceSessionProxy_CloseSession = 45
    , RemoteCDMInstanceSessionProxy_CloseSessionReply = 46
    , RemoteCDMInstanceSessionProxy_RemoveSessionData = 47
    , RemoteCDMInstanceSessionProxy_RemoveSessionDataReply = 48
    , RemoteCDMInstanceSessionProxy_StoreRecordOfKeyUsage = 49
    , RemoteCDMProxy_GetSupportedConfiguration = 50
    , RemoteCDMProxy_GetSupportedConfigurationReply = 51
    , RemoteCDMProxy_CreateInstance = 52
    , RemoteCDMProxy_LoadAndInitialize = 53
    , RemoteLegacyCDMFactoryProxy_CreateCDM = 54
    , RemoteLegacyCDMFactoryProxy_SupportsKeySystem = 55
    , RemoteLegacyCDMProxy_SupportsMIMEType = 56
    , RemoteLegacyCDMProxy_CreateSession = 57
    , RemoteLegacyCDMProxy_SetPlayerId = 58
    , RemoteLegacyCDMSessionProxy_GenerateKeyRequest = 59
    , RemoteLegacyCDMSessionProxy_ReleaseKeys = 60
    , RemoteLegacyCDMSessionProxy_Update = 61
    , RemoteLegacyCDMSessionProxy_CachedKeyForKeyID = 62
    , RemoteMediaPlayerManagerProxy_CreateMediaPlayer = 63
    , RemoteMediaPlayerManagerProxy_CreateMediaPlayerReply = 64
    , RemoteMediaPlayerManagerProxy_DeleteMediaPlayer = 65
    , RemoteMediaPlayerManagerProxy_GetSupportedTypes = 66
    , RemoteMediaPlayerManagerProxy_SupportsTypeAndCodecs = 67
    , RemoteMediaPlayerManagerProxy_CanDecodeExtendedType = 68
    , RemoteMediaPlayerManagerProxy_OriginsInMediaCache = 69
    , RemoteMediaPlayerManagerProxy_ClearMediaCache = 70
    , RemoteMediaPlayerManagerProxy_ClearMediaCacheForOrigins = 71
    , RemoteMediaPlayerManagerProxy_SupportsKeySystem = 72
    , RemoteMediaPlayerProxy_PrepareForPlayback = 73
    , RemoteMediaPlayerProxy_PrepareForPlaybackReply = 74
    , RemoteMediaPlayerProxy_Load = 75
    , RemoteMediaPlayerProxy_LoadReply = 76
    , RemoteMediaPlayerProxy_CancelLoad = 77
    , RemoteMediaPlayerProxy_PrepareToPlay = 78
    , RemoteMediaPlayerProxy_Play = 79
    , RemoteMediaPlayerProxy_Pause = 80
    , RemoteMediaPlayerProxy_SetVolume = 81
    , RemoteMediaPlayerProxy_SetMuted = 82
    , RemoteMediaPlayerProxy_Seek = 83
    , RemoteMediaPlayerProxy_SeekWithTolerance = 84
    , RemoteMediaPlayerProxy_SetPreload = 85
    , RemoteMediaPlayerProxy_SetPrivateBrowsingMode = 86
    , RemoteMediaPlayerProxy_SetPreservesPitch = 87
    , RemoteMediaPlayerProxy_PrepareForRendering = 88
    , RemoteMediaPlayerProxy_SetVisible = 89
    , RemoteMediaPlayerProxy_SetShouldMaintainAspectRatio = 90
#if ENABLE(VIDEO_PRESENTATION_MODE)
    , RemoteMediaPlayerProxy_SetVideoFullscreenGravity = 91
#endif
    , RemoteMediaPlayerProxy_AcceleratedRenderingStateChanged = 92
    , RemoteMediaPlayerProxy_SetShouldDisableSleep = 93
    , RemoteMediaPlayerProxy_SetRate = 94
#if ENABLE(VIDEO_PRESENTATION_MODE)
    , RemoteMediaPlayerProxy_UpdateVideoFullscreenInlineImage = 95
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    , RemoteMediaPlayerProxy_SetVideoFullscreenMode = 96
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    , RemoteMediaPlayerProxy_VideoFullscreenStandbyChanged = 97
#endif
    , RemoteMediaPlayerProxy_SetBufferingPolicy = 98
    , RemoteMediaPlayerProxy_AudioTrackSetEnabled = 99
    , RemoteMediaPlayerProxy_VideoTrackSetSelected = 100
    , RemoteMediaPlayerProxy_TextTrackSetMode = 101
#if PLATFORM(COCOA)
    , RemoteMediaPlayerProxy_SetVideoInlineSizeFenced = 102
#endif
#if (PLATFORM(COCOA) && ENABLE(VIDEO_PRESENTATION_MODE))
    , RemoteMediaPlayerProxy_SetVideoFullscreenFrameFenced = 103
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    , RemoteMediaPlayerProxy_EnterFullscreen = 104
    , RemoteMediaPlayerProxy_EnterFullscreenReply = 105
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    , RemoteMediaPlayerProxy_ExitFullscreen = 106
    , RemoteMediaPlayerProxy_ExitFullscreenReply = 107
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    , RemoteMediaPlayerProxy_SetWirelessVideoPlaybackDisabled = 108
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    , RemoteMediaPlayerProxy_SetShouldPlayToPlaybackTarget = 109
#endif
#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    , RemoteMediaPlayerProxy_SetLegacyCDMSession = 110
#endif
#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    , RemoteMediaPlayerProxy_KeyAdded = 111
#endif
#if ENABLE(ENCRYPTED_MEDIA)
    , RemoteMediaPlayerProxy_CdmInstanceAttached = 112
#endif
#if ENABLE(ENCRYPTED_MEDIA)
    , RemoteMediaPlayerProxy_CdmInstanceDetached = 113
#endif
#if ENABLE(ENCRYPTED_MEDIA)
    , RemoteMediaPlayerProxy_AttemptToDecryptWithInstance = 114
#endif
#if ENABLE(LEGACY_ENCRYPTED_MEDIA) && ENABLE(ENCRYPTED_MEDIA)
    , RemoteMediaPlayerProxy_SetShouldContinueAfterKeyNeeded = 115
#endif
    , RemoteMediaPlayerProxy_BeginSimulatedHDCPError = 116
    , RemoteMediaPlayerProxy_EndSimulatedHDCPError = 117
    , RemoteMediaPlayerProxy_NotifyActiveSourceBuffersChanged = 118
    , RemoteMediaPlayerProxy_ApplicationWillResignActive = 119
    , RemoteMediaPlayerProxy_ApplicationDidBecomeActive = 120
    , RemoteMediaPlayerProxy_NotifyTrackModeChanged = 121
    , RemoteMediaPlayerProxy_TracksChanged = 122
    , RemoteMediaPlayerProxy_SyncTextTrackBounds = 123
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    , RemoteMediaPlayerProxy_SetWirelessPlaybackTarget = 124
#endif
    , RemoteMediaPlayerProxy_PerformTaskAtMediaTime = 125
    , RemoteMediaPlayerProxy_PerformTaskAtMediaTimeReply = 126
    , RemoteMediaPlayerProxy_WouldTaintOrigin = 127
#if PLATFORM(IOS_FAMILY)
    , RemoteMediaPlayerProxy_ErrorLog = 128
#endif
#if PLATFORM(IOS_FAMILY)
    , RemoteMediaPlayerProxy_AccessLog = 129
#endif
    , RemoteMediaResourceManager_ResponseReceived = 130
    , RemoteMediaResourceManager_ResponseReceivedReply = 131
    , RemoteMediaResourceManager_RedirectReceived = 132
    , RemoteMediaResourceManager_RedirectReceivedReply = 133
    , RemoteMediaResourceManager_DataSent = 134
    , RemoteMediaResourceManager_DataReceived = 135
    , RemoteMediaResourceManager_AccessControlCheckFailed = 136
    , RemoteMediaResourceManager_LoadFailed = 137
    , RemoteMediaResourceManager_LoadFinished = 138
    , LibWebRTCCodecsProxy_CreateH264Decoder = 139
    , LibWebRTCCodecsProxy_CreateH265Decoder = 140
    , LibWebRTCCodecsProxy_ReleaseDecoder = 141
    , LibWebRTCCodecsProxy_DecodeFrame = 142
    , LibWebRTCCodecsProxy_CreateEncoder = 143
    , LibWebRTCCodecsProxy_ReleaseEncoder = 144
    , LibWebRTCCodecsProxy_InitializeEncoder = 145
    , LibWebRTCCodecsProxy_EncodeFrame = 146
    , LibWebRTCCodecsProxy_SetEncodeRates = 147
    , RemoteAudioMediaStreamTrackRenderer_Start = 148
    , RemoteAudioMediaStreamTrackRenderer_Stop = 149
    , RemoteAudioMediaStreamTrackRenderer_Clear = 150
    , RemoteAudioMediaStreamTrackRenderer_SetVolume = 151
    , RemoteAudioMediaStreamTrackRenderer_AudioSamplesStorageChanged = 152
    , RemoteAudioMediaStreamTrackRenderer_AudioSamplesAvailable = 153
    , RemoteAudioMediaStreamTrackRendererManager_CreateRenderer = 154
    , RemoteAudioMediaStreamTrackRendererManager_ReleaseRenderer = 155
    , RemoteMediaRecorder_AudioSamplesStorageChanged = 156
    , RemoteMediaRecorder_AudioSamplesAvailable = 157
    , RemoteMediaRecorder_VideoSampleAvailable = 158
    , RemoteMediaRecorder_FetchData = 159
    , RemoteMediaRecorder_FetchDataReply = 160
    , RemoteMediaRecorder_StopRecording = 161
    , RemoteMediaRecorderManager_CreateRecorder = 162
    , RemoteMediaRecorderManager_CreateRecorderReply = 163
    , RemoteMediaRecorderManager_ReleaseRecorder = 164
    , RemoteSampleBufferDisplayLayer_UpdateDisplayMode = 165
    , RemoteSampleBufferDisplayLayer_UpdateAffineTransform = 166
    , RemoteSampleBufferDisplayLayer_UpdateBoundsAndPosition = 167
    , RemoteSampleBufferDisplayLayer_Flush = 168
    , RemoteSampleBufferDisplayLayer_FlushAndRemoveImage = 169
    , RemoteSampleBufferDisplayLayer_EnqueueSample = 170
    , RemoteSampleBufferDisplayLayer_ClearEnqueuedSamples = 171
    , RemoteSampleBufferDisplayLayerManager_CreateLayer = 172
    , RemoteSampleBufferDisplayLayerManager_CreateLayerReply = 173
    , RemoteSampleBufferDisplayLayerManager_ReleaseLayer = 174
    , WebCookieManager_GetHostnamesWithCookies = 175
    , WebCookieManager_GetHostnamesWithCookiesReply = 176
    , WebCookieManager_DeleteCookiesForHostnames = 177
    , WebCookieManager_DeleteAllCookies = 178
    , WebCookieManager_SetCookie = 179
    , WebCookieManager_SetCookieReply = 180
    , WebCookieManager_SetCookies = 181
    , WebCookieManager_SetCookiesReply = 182
    , WebCookieManager_GetAllCookies = 183
    , WebCookieManager_GetAllCookiesReply = 184
    , WebCookieManager_GetCookies = 185
    , WebCookieManager_GetCookiesReply = 186
    , WebCookieManager_DeleteCookie = 187
    , WebCookieManager_DeleteCookieReply = 188
    , WebCookieManager_DeleteAllCookiesModifiedSince = 189
    , WebCookieManager_DeleteAllCookiesModifiedSinceReply = 190
    , WebCookieManager_SetHTTPCookieAcceptPolicy = 191
    , WebCookieManager_SetHTTPCookieAcceptPolicyReply = 192
    , WebCookieManager_GetHTTPCookieAcceptPolicy = 193
    , WebCookieManager_GetHTTPCookieAcceptPolicyReply = 194
    , WebCookieManager_StartObservingCookieChanges = 195
    , WebCookieManager_StopObservingCookieChanges = 196
#if USE(SOUP)
    , WebCookieManager_SetCookiePersistentStorage = 197
#endif
    , WebIDBServer_DeleteDatabase = 198
    , WebIDBServer_OpenDatabase = 199
    , WebIDBServer_AbortTransaction = 200
    , WebIDBServer_CommitTransaction = 201
    , WebIDBServer_DidFinishHandlingVersionChangeTransaction = 202
    , WebIDBServer_CreateObjectStore = 203
    , WebIDBServer_DeleteObjectStore = 204
    , WebIDBServer_RenameObjectStore = 205
    , WebIDBServer_ClearObjectStore = 206
    , WebIDBServer_CreateIndex = 207
    , WebIDBServer_DeleteIndex = 208
    , WebIDBServer_RenameIndex = 209
    , WebIDBServer_PutOrAdd = 210
    , WebIDBServer_GetRecord = 211
    , WebIDBServer_GetAllRecords = 212
    , WebIDBServer_GetCount = 213
    , WebIDBServer_DeleteRecord = 214
    , WebIDBServer_OpenCursor = 215
    , WebIDBServer_IterateCursor = 216
    , WebIDBServer_EstablishTransaction = 217
    , WebIDBServer_DatabaseConnectionPendingClose = 218
    , WebIDBServer_DatabaseConnectionClosed = 219
    , WebIDBServer_AbortOpenAndUpgradeNeeded = 220
    , WebIDBServer_DidFireVersionChangeEvent = 221
    , WebIDBServer_OpenDBRequestCancelled = 222
    , WebIDBServer_GetAllDatabaseNamesAndVersions = 223
    , NetworkConnectionToWebProcess_ScheduleResourceLoad = 224
    , NetworkConnectionToWebProcess_PerformSynchronousLoad = 225
    , NetworkConnectionToWebProcess_TestProcessIncomingSyncMessagesWhenWaitingForSyncReply = 226
    , NetworkConnectionToWebProcess_LoadPing = 227
    , NetworkConnectionToWebProcess_RemoveLoadIdentifier = 228
    , NetworkConnectionToWebProcess_PageLoadCompleted = 229
    , NetworkConnectionToWebProcess_BrowsingContextRemoved = 230
    , NetworkConnectionToWebProcess_PrefetchDNS = 231
    , NetworkConnectionToWebProcess_PreconnectTo = 232
    , NetworkConnectionToWebProcess_StartDownload = 233
    , NetworkConnectionToWebProcess_ConvertMainResourceLoadToDownload = 234
    , NetworkConnectionToWebProcess_CookiesForDOM = 235
    , NetworkConnectionToWebProcess_SetCookiesFromDOM = 236
    , NetworkConnectionToWebProcess_CookieRequestHeaderFieldValue = 237
    , NetworkConnectionToWebProcess_GetRawCookies = 238
    , NetworkConnectionToWebProcess_SetRawCookie = 239
    , NetworkConnectionToWebProcess_DeleteCookie = 240
    , NetworkConnectionToWebProcess_DomCookiesForHost = 241
#if HAVE(COOKIE_CHANGE_LISTENER_API)
    , NetworkConnectionToWebProcess_UnsubscribeFromCookieChangeNotifications = 242
#endif
    , NetworkConnectionToWebProcess_RegisterFileBlobURL = 243
    , NetworkConnectionToWebProcess_RegisterBlobURL = 244
    , NetworkConnectionToWebProcess_RegisterBlobURLFromURL = 245
    , NetworkConnectionToWebProcess_RegisterBlobURLOptionallyFileBacked = 246
    , NetworkConnectionToWebProcess_RegisterBlobURLForSlice = 247
    , NetworkConnectionToWebProcess_UnregisterBlobURL = 248
    , NetworkConnectionToWebProcess_BlobSize = 249
    , NetworkConnectionToWebProcess_WriteBlobsToTemporaryFiles = 250
    , NetworkConnectionToWebProcess_WriteBlobsToTemporaryFilesReply = 251
    , NetworkConnectionToWebProcess_SetCaptureExtraNetworkLoadMetricsEnabled = 252
    , NetworkConnectionToWebProcess_CreateSocketStream = 253
    , NetworkConnectionToWebProcess_CreateSocketChannel = 254
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkConnectionToWebProcess_RemoveStorageAccessForFrame = 255
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkConnectionToWebProcess_ClearPageSpecificDataForResourceLoadStatistics = 256
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkConnectionToWebProcess_LogUserInteraction = 257
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkConnectionToWebProcess_ResourceLoadStatisticsUpdated = 258
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkConnectionToWebProcess_HasStorageAccess = 259
    , NetworkConnectionToWebProcess_HasStorageAccessReply = 260
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkConnectionToWebProcess_RequestStorageAccess = 261
    , NetworkConnectionToWebProcess_RequestStorageAccessReply = 262
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkConnectionToWebProcess_RequestStorageAccessUnderOpener = 263
#endif
    , NetworkConnectionToWebProcess_AddOriginAccessWhitelistEntry = 264
    , NetworkConnectionToWebProcess_RemoveOriginAccessWhitelistEntry = 265
    , NetworkConnectionToWebProcess_ResetOriginAccessWhitelists = 266
    , NetworkConnectionToWebProcess_GetNetworkLoadInformationResponse = 267
    , NetworkConnectionToWebProcess_GetNetworkLoadIntermediateInformation = 268
    , NetworkConnectionToWebProcess_TakeNetworkLoadInformationMetrics = 269
#if ENABLE(SERVICE_WORKER)
    , NetworkConnectionToWebProcess_EstablishSWContextConnection = 270
    , NetworkConnectionToWebProcess_EstablishSWContextConnectionReply = 271
#endif
#if ENABLE(SERVICE_WORKER)
    , NetworkConnectionToWebProcess_CloseSWContextConnection = 272
#endif
    , NetworkConnectionToWebProcess_UpdateQuotaBasedOnSpaceUsageForTesting = 273
    , NetworkConnectionToWebProcess_CreateNewMessagePortChannel = 274
    , NetworkConnectionToWebProcess_EntangleLocalPortInThisProcessToRemote = 275
    , NetworkConnectionToWebProcess_MessagePortDisentangled = 276
    , NetworkConnectionToWebProcess_MessagePortClosed = 277
    , NetworkConnectionToWebProcess_TakeAllMessagesForPort = 278
    , NetworkConnectionToWebProcess_TakeAllMessagesForPortReply = 279
    , NetworkConnectionToWebProcess_PostMessageToRemote = 280
    , NetworkConnectionToWebProcess_CheckRemotePortForActivity = 281
    , NetworkConnectionToWebProcess_CheckRemotePortForActivityReply = 282
    , NetworkConnectionToWebProcess_DidDeliverMessagePortMessages = 283
    , NetworkConnectionToWebProcess_RegisterURLSchemesAsCORSEnabled = 284
    , NetworkContentRuleListManager_Remove = 285
    , NetworkContentRuleListManager_AddContentRuleLists = 286
    , NetworkContentRuleListManager_RemoveContentRuleList = 287
    , NetworkContentRuleListManager_RemoveAllContentRuleLists = 288
    , NetworkProcess_InitializeNetworkProcess = 289
    , NetworkProcess_CreateNetworkConnectionToWebProcess = 290
    , NetworkProcess_CreateNetworkConnectionToWebProcessReply = 291
#if USE(SOUP)
    , NetworkProcess_SetIgnoreTLSErrors = 292
#endif
#if USE(SOUP)
    , NetworkProcess_UserPreferredLanguagesChanged = 293
#endif
#if USE(SOUP)
    , NetworkProcess_SetNetworkProxySettings = 294
#endif
#if USE(SOUP)
    , NetworkProcess_PrefetchDNS = 295
#endif
#if USE(CURL)
    , NetworkProcess_SetNetworkProxySettings = 296
#endif
    , NetworkProcess_ClearCachedCredentials = 297
    , NetworkProcess_AddWebsiteDataStore = 298
    , NetworkProcess_DestroySession = 299
    , NetworkProcess_FetchWebsiteData = 300
    , NetworkProcess_DeleteWebsiteData = 301
    , NetworkProcess_DeleteWebsiteDataForOrigins = 302
    , NetworkProcess_RenameOriginInWebsiteData = 303
    , NetworkProcess_RenameOriginInWebsiteDataReply = 304
    , NetworkProcess_DownloadRequest = 305
    , NetworkProcess_ResumeDownload = 306
    , NetworkProcess_CancelDownload = 307
#if PLATFORM(COCOA)
    , NetworkProcess_PublishDownloadProgress = 308
#endif
    , NetworkProcess_ApplicationDidEnterBackground = 309
    , NetworkProcess_ApplicationWillEnterForeground = 310
    , NetworkProcess_ContinueWillSendRequest = 311
    , NetworkProcess_ContinueDecidePendingDownloadDestination = 312
#if PLATFORM(COCOA)
    , NetworkProcess_SetQOS = 313
#endif
#if PLATFORM(COCOA)
    , NetworkProcess_SetStorageAccessAPIEnabled = 314
#endif
    , NetworkProcess_SetAllowsAnySSLCertificateForWebSocket = 315
    , NetworkProcess_SyncAllCookies = 316
    , NetworkProcess_AllowSpecificHTTPSCertificateForHost = 317
    , NetworkProcess_SetCacheModel = 318
    , NetworkProcess_SetCacheModelSynchronouslyForTesting = 319
    , NetworkProcess_ProcessDidTransitionToBackground = 320
    , NetworkProcess_ProcessDidTransitionToForeground = 321
    , NetworkProcess_ProcessWillSuspendImminentlyForTestingSync = 322
    , NetworkProcess_PrepareToSuspend = 323
    , NetworkProcess_PrepareToSuspendReply = 324
    , NetworkProcess_ProcessDidResume = 325
    , NetworkProcess_PreconnectTo = 326
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_ClearPrevalentResource = 327
    , NetworkProcess_ClearPrevalentResourceReply = 328
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_ClearUserInteraction = 329
    , NetworkProcess_ClearUserInteractionReply = 330
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_DumpResourceLoadStatistics = 331
    , NetworkProcess_DumpResourceLoadStatisticsReply = 332
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetResourceLoadStatisticsEnabled = 333
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetResourceLoadStatisticsLogTestingEvent = 334
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_UpdatePrevalentDomainsToBlockCookiesFor = 335
    , NetworkProcess_UpdatePrevalentDomainsToBlockCookiesForReply = 336
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_IsGrandfathered = 337
    , NetworkProcess_IsGrandfatheredReply = 338
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_IsPrevalentResource = 339
    , NetworkProcess_IsPrevalentResourceReply = 340
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_IsVeryPrevalentResource = 341
    , NetworkProcess_IsVeryPrevalentResourceReply = 342
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetAgeCapForClientSideCookies = 343
    , NetworkProcess_SetAgeCapForClientSideCookiesReply = 344
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetLastSeen = 345
    , NetworkProcess_SetLastSeenReply = 346
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_MergeStatisticForTesting = 347
    , NetworkProcess_MergeStatisticForTestingReply = 348
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_InsertExpiredStatisticForTesting = 349
    , NetworkProcess_InsertExpiredStatisticForTestingReply = 350
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetPrevalentResource = 351
    , NetworkProcess_SetPrevalentResourceReply = 352
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetPrevalentResourceForDebugMode = 353
    , NetworkProcess_SetPrevalentResourceForDebugModeReply = 354
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_IsResourceLoadStatisticsEphemeral = 355
    , NetworkProcess_IsResourceLoadStatisticsEphemeralReply = 356
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_HadUserInteraction = 357
    , NetworkProcess_HadUserInteractionReply = 358
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_IsRelationshipOnlyInDatabaseOnce = 359
    , NetworkProcess_IsRelationshipOnlyInDatabaseOnceReply = 360
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_HasLocalStorage = 361
    , NetworkProcess_HasLocalStorageReply = 362
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_GetAllStorageAccessEntries = 363
    , NetworkProcess_GetAllStorageAccessEntriesReply = 364
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_IsRegisteredAsRedirectingTo = 365
    , NetworkProcess_IsRegisteredAsRedirectingToReply = 366
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_IsRegisteredAsSubFrameUnder = 367
    , NetworkProcess_IsRegisteredAsSubFrameUnderReply = 368
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_IsRegisteredAsSubresourceUnder = 369
    , NetworkProcess_IsRegisteredAsSubresourceUnderReply = 370
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_DomainIDExistsInDatabase = 371
    , NetworkProcess_DomainIDExistsInDatabaseReply = 372
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_LogFrameNavigation = 373
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_LogUserInteraction = 374
    , NetworkProcess_LogUserInteractionReply = 375
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_ResetParametersToDefaultValues = 376
    , NetworkProcess_ResetParametersToDefaultValuesReply = 377
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_ScheduleClearInMemoryAndPersistent = 378
    , NetworkProcess_ScheduleClearInMemoryAndPersistentReply = 379
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_ScheduleCookieBlockingUpdate = 380
    , NetworkProcess_ScheduleCookieBlockingUpdateReply = 381
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_ScheduleStatisticsAndDataRecordsProcessing = 382
    , NetworkProcess_ScheduleStatisticsAndDataRecordsProcessingReply = 383
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_StatisticsDatabaseHasAllTables = 384
    , NetworkProcess_StatisticsDatabaseHasAllTablesReply = 385
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SubmitTelemetry = 386
    , NetworkProcess_SubmitTelemetryReply = 387
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetCacheMaxAgeCapForPrevalentResources = 388
    , NetworkProcess_SetCacheMaxAgeCapForPrevalentResourcesReply = 389
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetGrandfathered = 390
    , NetworkProcess_SetGrandfatheredReply = 391
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetUseITPDatabase = 392
    , NetworkProcess_SetUseITPDatabaseReply = 393
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_GetResourceLoadStatisticsDataSummary = 394
    , NetworkProcess_GetResourceLoadStatisticsDataSummaryReply = 395
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetGrandfatheringTime = 396
    , NetworkProcess_SetGrandfatheringTimeReply = 397
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetMaxStatisticsEntries = 398
    , NetworkProcess_SetMaxStatisticsEntriesReply = 399
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetMinimumTimeBetweenDataRecordsRemoval = 400
    , NetworkProcess_SetMinimumTimeBetweenDataRecordsRemovalReply = 401
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetPruneEntriesDownTo = 402
    , NetworkProcess_SetPruneEntriesDownToReply = 403
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetShouldClassifyResourcesBeforeDataRecordsRemoval = 404
    , NetworkProcess_SetShouldClassifyResourcesBeforeDataRecordsRemovalReply = 405
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetNotifyPagesWhenDataRecordsWereScanned = 406
    , NetworkProcess_SetNotifyPagesWhenDataRecordsWereScannedReply = 407
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetIsRunningResourceLoadStatisticsTest = 408
    , NetworkProcess_SetIsRunningResourceLoadStatisticsTestReply = 409
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetNotifyPagesWhenTelemetryWasCaptured = 410
    , NetworkProcess_SetNotifyPagesWhenTelemetryWasCapturedReply = 411
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetResourceLoadStatisticsDebugMode = 412
    , NetworkProcess_SetResourceLoadStatisticsDebugModeReply = 413
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetVeryPrevalentResource = 414
    , NetworkProcess_SetVeryPrevalentResourceReply = 415
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetSubframeUnderTopFrameDomain = 416
    , NetworkProcess_SetSubframeUnderTopFrameDomainReply = 417
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetSubresourceUnderTopFrameDomain = 418
    , NetworkProcess_SetSubresourceUnderTopFrameDomainReply = 419
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetSubresourceUniqueRedirectTo = 420
    , NetworkProcess_SetSubresourceUniqueRedirectToReply = 421
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetSubresourceUniqueRedirectFrom = 422
    , NetworkProcess_SetSubresourceUniqueRedirectFromReply = 423
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetTimeToLiveUserInteraction = 424
    , NetworkProcess_SetTimeToLiveUserInteractionReply = 425
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetTopFrameUniqueRedirectTo = 426
    , NetworkProcess_SetTopFrameUniqueRedirectToReply = 427
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetTopFrameUniqueRedirectFrom = 428
    , NetworkProcess_SetTopFrameUniqueRedirectFromReply = 429
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_ResetCacheMaxAgeCapForPrevalentResources = 430
    , NetworkProcess_ResetCacheMaxAgeCapForPrevalentResourcesReply = 431
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_DidCommitCrossSiteLoadWithDataTransfer = 432
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetCrossSiteLoadWithLinkDecorationForTesting = 433
    , NetworkProcess_SetCrossSiteLoadWithLinkDecorationForTestingReply = 434
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_ResetCrossSiteLoadsWithLinkDecorationForTesting = 435
    , NetworkProcess_ResetCrossSiteLoadsWithLinkDecorationForTestingReply = 436
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_DeleteCookiesForTesting = 437
    , NetworkProcess_DeleteCookiesForTestingReply = 438
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_HasIsolatedSession = 439
    , NetworkProcess_HasIsolatedSessionReply = 440
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetAppBoundDomainsForResourceLoadStatistics = 441
    , NetworkProcess_SetAppBoundDomainsForResourceLoadStatisticsReply = 442
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetShouldDowngradeReferrerForTesting = 443
    , NetworkProcess_SetShouldDowngradeReferrerForTestingReply = 444
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetThirdPartyCookieBlockingMode = 445
    , NetworkProcess_SetThirdPartyCookieBlockingModeReply = 446
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetShouldEnbleSameSiteStrictEnforcementForTesting = 447
    , NetworkProcess_SetShouldEnbleSameSiteStrictEnforcementForTestingReply = 448
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetFirstPartyWebsiteDataRemovalModeForTesting = 449
    , NetworkProcess_SetFirstPartyWebsiteDataRemovalModeForTestingReply = 450
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcess_SetToSameSiteStrictCookiesForTesting = 451
    , NetworkProcess_SetToSameSiteStrictCookiesForTestingReply = 452
#endif
    , NetworkProcess_SetAdClickAttributionDebugMode = 453
    , NetworkProcess_SetSessionIsControlledByAutomation = 454
    , NetworkProcess_RegisterURLSchemeAsSecure = 455
    , NetworkProcess_RegisterURLSchemeAsBypassingContentSecurityPolicy = 456
    , NetworkProcess_RegisterURLSchemeAsLocal = 457
    , NetworkProcess_RegisterURLSchemeAsNoAccess = 458
    , NetworkProcess_SetCacheStorageParameters = 459
    , NetworkProcess_SyncLocalStorage = 460
    , NetworkProcess_ClearLegacyPrivateBrowsingLocalStorage = 461
    , NetworkProcess_StoreAdClickAttribution = 462
    , NetworkProcess_DumpAdClickAttribution = 463
    , NetworkProcess_DumpAdClickAttributionReply = 464
    , NetworkProcess_ClearAdClickAttribution = 465
    , NetworkProcess_ClearAdClickAttributionReply = 466
    , NetworkProcess_SetAdClickAttributionOverrideTimerForTesting = 467
    , NetworkProcess_SetAdClickAttributionOverrideTimerForTestingReply = 468
    , NetworkProcess_SetAdClickAttributionConversionURLForTesting = 469
    , NetworkProcess_SetAdClickAttributionConversionURLForTestingReply = 470
    , NetworkProcess_MarkAdClickAttributionsAsExpiredForTesting = 471
    , NetworkProcess_MarkAdClickAttributionsAsExpiredForTestingReply = 472
    , NetworkProcess_GetLocalStorageOriginDetails = 473
    , NetworkProcess_GetLocalStorageOriginDetailsReply = 474
    , NetworkProcess_SetServiceWorkerFetchTimeoutForTesting = 475
    , NetworkProcess_ResetServiceWorkerFetchTimeoutForTesting = 476
    , NetworkProcess_ResetQuota = 477
    , NetworkProcess_ResetQuotaReply = 478
    , NetworkProcess_HasAppBoundSession = 479
    , NetworkProcess_HasAppBoundSessionReply = 480
    , NetworkProcess_ClearAppBoundSession = 481
    , NetworkProcess_ClearAppBoundSessionReply = 482
#if PLATFORM(IOS_FAMILY) && !PLATFORM(MACCATALYST)
    , NetworkProcess_DisableServiceWorkerEntitlement = 483
#endif
#if PLATFORM(IOS_FAMILY) && !PLATFORM(MACCATALYST)
    , NetworkProcess_ClearServiceWorkerEntitlementOverride = 484
    , NetworkProcess_ClearServiceWorkerEntitlementOverrideReply = 485
#endif
    , NetworkProcess_UpdateBundleIdentifier = 486
    , NetworkProcess_UpdateBundleIdentifierReply = 487
    , NetworkProcess_ClearBundleIdentifier = 488
    , NetworkProcess_ClearBundleIdentifierReply = 489
    , NetworkResourceLoader_ContinueWillSendRequest = 490
    , NetworkResourceLoader_ContinueDidReceiveResponse = 491
    , NetworkSocketChannel_SendString = 492
    , NetworkSocketChannel_SendStringReply = 493
    , NetworkSocketChannel_SendData = 494
    , NetworkSocketChannel_SendDataReply = 495
    , NetworkSocketChannel_Close = 496
    , NetworkSocketStream_SendData = 497
    , NetworkSocketStream_SendHandshake = 498
    , NetworkSocketStream_Close = 499
    , ServiceWorkerFetchTask_DidNotHandle = 500
    , ServiceWorkerFetchTask_DidFail = 501
    , ServiceWorkerFetchTask_DidReceiveRedirectResponse = 502
    , ServiceWorkerFetchTask_DidReceiveResponse = 503
    , ServiceWorkerFetchTask_DidReceiveData = 504
    , ServiceWorkerFetchTask_DidReceiveSharedBuffer = 505
    , ServiceWorkerFetchTask_DidReceiveFormData = 506
    , ServiceWorkerFetchTask_DidFinish = 507
    , WebSWServerConnection_ScheduleJobInServer = 508
    , WebSWServerConnection_ScheduleUnregisterJobInServer = 509
    , WebSWServerConnection_ScheduleUnregisterJobInServerReply = 510
    , WebSWServerConnection_FinishFetchingScriptInServer = 511
    , WebSWServerConnection_AddServiceWorkerRegistrationInServer = 512
    , WebSWServerConnection_RemoveServiceWorkerRegistrationInServer = 513
    , WebSWServerConnection_PostMessageToServiceWorker = 514
    , WebSWServerConnection_DidResolveRegistrationPromise = 515
    , WebSWServerConnection_MatchRegistration = 516
    , WebSWServerConnection_WhenRegistrationReady = 517
    , WebSWServerConnection_GetRegistrations = 518
    , WebSWServerConnection_RegisterServiceWorkerClient = 519
    , WebSWServerConnection_UnregisterServiceWorkerClient = 520
    , WebSWServerConnection_TerminateWorkerFromClient = 521
    , WebSWServerConnection_TerminateWorkerFromClientReply = 522
    , WebSWServerConnection_WhenServiceWorkerIsTerminatedForTesting = 523
    , WebSWServerConnection_WhenServiceWorkerIsTerminatedForTestingReply = 524
    , WebSWServerConnection_SetThrottleState = 525
    , WebSWServerConnection_StoreRegistrationsOnDisk = 526
    , WebSWServerConnection_StoreRegistrationsOnDiskReply = 527
    , WebSWServerToContextConnection_ScriptContextFailedToStart = 528
    , WebSWServerToContextConnection_ScriptContextStarted = 529
    , WebSWServerToContextConnection_DidFinishInstall = 530
    , WebSWServerToContextConnection_DidFinishActivation = 531
    , WebSWServerToContextConnection_SetServiceWorkerHasPendingEvents = 532
    , WebSWServerToContextConnection_SkipWaiting = 533
    , WebSWServerToContextConnection_SkipWaitingReply = 534
    , WebSWServerToContextConnection_WorkerTerminated = 535
    , WebSWServerToContextConnection_FindClientByIdentifier = 536
    , WebSWServerToContextConnection_MatchAll = 537
    , WebSWServerToContextConnection_Claim = 538
    , WebSWServerToContextConnection_ClaimReply = 539
    , WebSWServerToContextConnection_SetScriptResource = 540
    , WebSWServerToContextConnection_PostMessageToServiceWorkerClient = 541
    , WebSWServerToContextConnection_DidFailHeartBeatCheck = 542
    , StorageManagerSet_ConnectToLocalStorageArea = 543
    , StorageManagerSet_ConnectToTransientLocalStorageArea = 544
    , StorageManagerSet_ConnectToSessionStorageArea = 545
    , StorageManagerSet_DisconnectFromStorageArea = 546
    , StorageManagerSet_GetValues = 547
    , StorageManagerSet_CloneSessionStorageNamespace = 548
    , StorageManagerSet_SetItem = 549
    , StorageManagerSet_RemoveItem = 550
    , StorageManagerSet_Clear = 551
    , CacheStorageEngineConnection_Reference = 552
    , CacheStorageEngineConnection_Dereference = 553
    , CacheStorageEngineConnection_Open = 554
    , CacheStorageEngineConnection_OpenReply = 555
    , CacheStorageEngineConnection_Remove = 556
    , CacheStorageEngineConnection_RemoveReply = 557
    , CacheStorageEngineConnection_Caches = 558
    , CacheStorageEngineConnection_CachesReply = 559
    , CacheStorageEngineConnection_RetrieveRecords = 560
    , CacheStorageEngineConnection_RetrieveRecordsReply = 561
    , CacheStorageEngineConnection_DeleteMatchingRecords = 562
    , CacheStorageEngineConnection_DeleteMatchingRecordsReply = 563
    , CacheStorageEngineConnection_PutRecords = 564
    , CacheStorageEngineConnection_PutRecordsReply = 565
    , CacheStorageEngineConnection_ClearMemoryRepresentation = 566
    , CacheStorageEngineConnection_ClearMemoryRepresentationReply = 567
    , CacheStorageEngineConnection_EngineRepresentation = 568
    , CacheStorageEngineConnection_EngineRepresentationReply = 569
    , NetworkMDNSRegister_UnregisterMDNSNames = 570
    , NetworkMDNSRegister_RegisterMDNSName = 571
    , NetworkRTCMonitor_StartUpdatingIfNeeded = 572
    , NetworkRTCMonitor_StopUpdating = 573
    , NetworkRTCProvider_CreateUDPSocket = 574
    , NetworkRTCProvider_CreateServerTCPSocket = 575
    , NetworkRTCProvider_CreateClientTCPSocket = 576
    , NetworkRTCProvider_WrapNewTCPConnection = 577
    , NetworkRTCProvider_CreateResolver = 578
    , NetworkRTCProvider_StopResolver = 579
    , NetworkRTCSocket_SendTo = 580
    , NetworkRTCSocket_Close = 581
    , NetworkRTCSocket_SetOption = 582
    , PluginControllerProxy_GeometryDidChange = 583
    , PluginControllerProxy_VisibilityDidChange = 584
    , PluginControllerProxy_FrameDidFinishLoading = 585
    , PluginControllerProxy_FrameDidFail = 586
    , PluginControllerProxy_DidEvaluateJavaScript = 587
    , PluginControllerProxy_StreamWillSendRequest = 588
    , PluginControllerProxy_StreamDidReceiveResponse = 589
    , PluginControllerProxy_StreamDidReceiveData = 590
    , PluginControllerProxy_StreamDidFinishLoading = 591
    , PluginControllerProxy_StreamDidFail = 592
    , PluginControllerProxy_ManualStreamDidReceiveResponse = 593
    , PluginControllerProxy_ManualStreamDidReceiveData = 594
    , PluginControllerProxy_ManualStreamDidFinishLoading = 595
    , PluginControllerProxy_ManualStreamDidFail = 596
    , PluginControllerProxy_HandleMouseEvent = 597
    , PluginControllerProxy_HandleWheelEvent = 598
    , PluginControllerProxy_HandleMouseEnterEvent = 599
    , PluginControllerProxy_HandleMouseLeaveEvent = 600
    , PluginControllerProxy_HandleKeyboardEvent = 601
    , PluginControllerProxy_HandleEditingCommand = 602
    , PluginControllerProxy_IsEditingCommandEnabled = 603
    , PluginControllerProxy_HandlesPageScaleFactor = 604
    , PluginControllerProxy_RequiresUnifiedScaleFactor = 605
    , PluginControllerProxy_SetFocus = 606
    , PluginControllerProxy_DidUpdate = 607
    , PluginControllerProxy_PaintEntirePlugin = 608
    , PluginControllerProxy_GetPluginScriptableNPObject = 609
    , PluginControllerProxy_WindowFocusChanged = 610
    , PluginControllerProxy_WindowVisibilityChanged = 611
#if PLATFORM(COCOA)
    , PluginControllerProxy_SendComplexTextInput = 612
#endif
#if PLATFORM(COCOA)
    , PluginControllerProxy_WindowAndViewFramesChanged = 613
#endif
#if PLATFORM(COCOA)
    , PluginControllerProxy_SetLayerHostingMode = 614
#endif
    , PluginControllerProxy_SupportsSnapshotting = 615
    , PluginControllerProxy_Snapshot = 616
    , PluginControllerProxy_StorageBlockingStateChanged = 617
    , PluginControllerProxy_PrivateBrowsingStateChanged = 618
    , PluginControllerProxy_GetFormValue = 619
    , PluginControllerProxy_MutedStateChanged = 620
    , PluginProcess_InitializePluginProcess = 621
    , PluginProcess_CreateWebProcessConnection = 622
    , PluginProcess_GetSitesWithData = 623
    , PluginProcess_DeleteWebsiteData = 624
    , PluginProcess_DeleteWebsiteDataForHostNames = 625
#if PLATFORM(COCOA)
    , PluginProcess_SetQOS = 626
#endif
    , WebProcessConnection_CreatePlugin = 627
    , WebProcessConnection_CreatePluginAsynchronously = 628
    , WebProcessConnection_DestroyPlugin = 629
    , AuxiliaryProcess_ShutDown = 630
    , AuxiliaryProcess_SetProcessSuppressionEnabled = 631
#if OS(LINUX)
    , AuxiliaryProcess_DidReceiveMemoryPressureEvent = 632
#endif
    , WebConnection_HandleMessage = 633
    , AuthenticationManager_CompleteAuthenticationChallenge = 634
    , NPObjectMessageReceiver_Deallocate = 635
    , NPObjectMessageReceiver_HasMethod = 636
    , NPObjectMessageReceiver_Invoke = 637
    , NPObjectMessageReceiver_InvokeDefault = 638
    , NPObjectMessageReceiver_HasProperty = 639
    , NPObjectMessageReceiver_GetProperty = 640
    , NPObjectMessageReceiver_SetProperty = 641
    , NPObjectMessageReceiver_RemoveProperty = 642
    , NPObjectMessageReceiver_Enumerate = 643
    , NPObjectMessageReceiver_Construct = 644
    , DrawingAreaProxy_EnterAcceleratedCompositingMode = 645
    , DrawingAreaProxy_UpdateAcceleratedCompositingMode = 646
    , DrawingAreaProxy_DidFirstLayerFlush = 647
    , DrawingAreaProxy_DispatchPresentationCallbacksAfterFlushingLayers = 648
#if PLATFORM(COCOA)
    , DrawingAreaProxy_DidUpdateGeometry = 649
#endif
#if USE(COORDINATED_GRAPHICS) || USE(TEXTURE_MAPPER)
    , DrawingAreaProxy_Update = 650
#endif
#if USE(COORDINATED_GRAPHICS) || USE(TEXTURE_MAPPER)
    , DrawingAreaProxy_DidUpdateBackingStoreState = 651
#endif
#if USE(COORDINATED_GRAPHICS) || USE(TEXTURE_MAPPER)
    , DrawingAreaProxy_ExitAcceleratedCompositingMode = 652
#endif
    , VisitedLinkStore_AddVisitedLinkHashFromPage = 653
    , WebCookieManagerProxy_CookiesDidChange = 654
    , WebFullScreenManagerProxy_SupportsFullScreen = 655
    , WebFullScreenManagerProxy_EnterFullScreen = 656
    , WebFullScreenManagerProxy_ExitFullScreen = 657
    , WebFullScreenManagerProxy_BeganEnterFullScreen = 658
    , WebFullScreenManagerProxy_BeganExitFullScreen = 659
    , WebFullScreenManagerProxy_Close = 660
    , WebGeolocationManagerProxy_StartUpdating = 661
    , WebGeolocationManagerProxy_StopUpdating = 662
    , WebGeolocationManagerProxy_SetEnableHighAccuracy = 663
    , WebPageProxy_CreateNewPage = 664
    , WebPageProxy_ShowPage = 665
    , WebPageProxy_ClosePage = 666
    , WebPageProxy_RunJavaScriptAlert = 667
    , WebPageProxy_RunJavaScriptConfirm = 668
    , WebPageProxy_RunJavaScriptPrompt = 669
    , WebPageProxy_MouseDidMoveOverElement = 670
#if ENABLE(NETSCAPE_PLUGIN_API)
    , WebPageProxy_UnavailablePluginButtonClicked = 671
#endif
#if ENABLE(WEBGL)
    , WebPageProxy_WebGLPolicyForURL = 672
#endif
#if ENABLE(WEBGL)
    , WebPageProxy_ResolveWebGLPolicyForURL = 673
#endif
    , WebPageProxy_DidChangeViewportProperties = 674
    , WebPageProxy_DidReceiveEvent = 675
    , WebPageProxy_SetCursor = 676
    , WebPageProxy_SetCursorHiddenUntilMouseMoves = 677
    , WebPageProxy_SetStatusText = 678
    , WebPageProxy_SetFocus = 679
    , WebPageProxy_TakeFocus = 680
    , WebPageProxy_FocusedFrameChanged = 681
    , WebPageProxy_SetRenderTreeSize = 682
    , WebPageProxy_SetToolbarsAreVisible = 683
    , WebPageProxy_GetToolbarsAreVisible = 684
    , WebPageProxy_SetMenuBarIsVisible = 685
    , WebPageProxy_GetMenuBarIsVisible = 686
    , WebPageProxy_SetStatusBarIsVisible = 687
    , WebPageProxy_GetStatusBarIsVisible = 688
    , WebPageProxy_SetIsResizable = 689
    , WebPageProxy_SetWindowFrame = 690
    , WebPageProxy_GetWindowFrame = 691
    , WebPageProxy_ScreenToRootView = 692
    , WebPageProxy_RootViewToScreen = 693
    , WebPageProxy_AccessibilityScreenToRootView = 694
    , WebPageProxy_RootViewToAccessibilityScreen = 695
#if PLATFORM(COCOA)
    , WebPageProxy_ShowValidationMessage = 696
#endif
#if PLATFORM(COCOA)
    , WebPageProxy_HideValidationMessage = 697
#endif
    , WebPageProxy_RunBeforeUnloadConfirmPanel = 698
    , WebPageProxy_PageDidScroll = 699
    , WebPageProxy_RunOpenPanel = 700
    , WebPageProxy_ShowShareSheet = 701
    , WebPageProxy_ShowShareSheetReply = 702
    , WebPageProxy_PrintFrame = 703
    , WebPageProxy_RunModal = 704
    , WebPageProxy_NotifyScrollerThumbIsVisibleInRect = 705
    , WebPageProxy_RecommendedScrollbarStyleDidChange = 706
    , WebPageProxy_DidChangeScrollbarsForMainFrame = 707
    , WebPageProxy_DidChangeScrollOffsetPinningForMainFrame = 708
    , WebPageProxy_DidChangePageCount = 709
    , WebPageProxy_PageExtendedBackgroundColorDidChange = 710
#if ENABLE(NETSCAPE_PLUGIN_API)
    , WebPageProxy_DidFailToInitializePlugin = 711
#endif
#if ENABLE(NETSCAPE_PLUGIN_API)
    , WebPageProxy_DidBlockInsecurePluginVersion = 712
#endif
    , WebPageProxy_SetCanShortCircuitHorizontalWheelEvents = 713
    , WebPageProxy_DidChangeContentSize = 714
    , WebPageProxy_DidChangeIntrinsicContentSize = 715
#if ENABLE(INPUT_TYPE_COLOR)
    , WebPageProxy_ShowColorPicker = 716
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    , WebPageProxy_SetColorPickerColor = 717
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    , WebPageProxy_EndColorPicker = 718
#endif
#if ENABLE(DATALIST_ELEMENT)
    , WebPageProxy_ShowDataListSuggestions = 719
#endif
#if ENABLE(DATALIST_ELEMENT)
    , WebPageProxy_HandleKeydownInDataList = 720
#endif
#if ENABLE(DATALIST_ELEMENT)
    , WebPageProxy_EndDataListSuggestions = 721
#endif
    , WebPageProxy_DecidePolicyForResponse = 722
    , WebPageProxy_DecidePolicyForNavigationActionAsync = 723
    , WebPageProxy_DecidePolicyForNavigationActionSync = 724
    , WebPageProxy_DecidePolicyForNewWindowAction = 725
    , WebPageProxy_UnableToImplementPolicy = 726
    , WebPageProxy_DidChangeProgress = 727
    , WebPageProxy_DidFinishProgress = 728
    , WebPageProxy_DidStartProgress = 729
    , WebPageProxy_SetNetworkRequestsInProgress = 730
    , WebPageProxy_DidCreateMainFrame = 731
    , WebPageProxy_DidCreateSubframe = 732
    , WebPageProxy_DidCreateWindow = 733
    , WebPageProxy_DidStartProvisionalLoadForFrame = 734
    , WebPageProxy_DidReceiveServerRedirectForProvisionalLoadForFrame = 735
    , WebPageProxy_WillPerformClientRedirectForFrame = 736
    , WebPageProxy_DidCancelClientRedirectForFrame = 737
    , WebPageProxy_DidChangeProvisionalURLForFrame = 738
    , WebPageProxy_DidFailProvisionalLoadForFrame = 739
    , WebPageProxy_DidCommitLoadForFrame = 740
    , WebPageProxy_DidFailLoadForFrame = 741
    , WebPageProxy_DidFinishDocumentLoadForFrame = 742
    , WebPageProxy_DidFinishLoadForFrame = 743
    , WebPageProxy_DidFirstLayoutForFrame = 744
    , WebPageProxy_DidFirstVisuallyNonEmptyLayoutForFrame = 745
    , WebPageProxy_DidReachLayoutMilestone = 746
    , WebPageProxy_DidReceiveTitleForFrame = 747
    , WebPageProxy_DidDisplayInsecureContentForFrame = 748
    , WebPageProxy_DidRunInsecureContentForFrame = 749
    , WebPageProxy_DidDetectXSSForFrame = 750
    , WebPageProxy_DidSameDocumentNavigationForFrame = 751
    , WebPageProxy_DidChangeMainDocument = 752
    , WebPageProxy_DidExplicitOpenForFrame = 753
    , WebPageProxy_DidDestroyNavigation = 754
    , WebPageProxy_MainFramePluginHandlesPageScaleGestureDidChange = 755
    , WebPageProxy_DidNavigateWithNavigationData = 756
    , WebPageProxy_DidPerformClientRedirect = 757
    , WebPageProxy_DidPerformServerRedirect = 758
    , WebPageProxy_DidUpdateHistoryTitle = 759
    , WebPageProxy_DidFinishLoadingDataForCustomContentProvider = 760
    , WebPageProxy_WillSubmitForm = 761
    , WebPageProxy_VoidCallback = 762
    , WebPageProxy_DataCallback = 763
    , WebPageProxy_ImageCallback = 764
    , WebPageProxy_StringCallback = 765
    , WebPageProxy_BoolCallback = 766
    , WebPageProxy_InvalidateStringCallback = 767
    , WebPageProxy_ScriptValueCallback = 768
    , WebPageProxy_ComputedPagesCallback = 769
    , WebPageProxy_ValidateCommandCallback = 770
    , WebPageProxy_EditingRangeCallback = 771
    , WebPageProxy_UnsignedCallback = 772
    , WebPageProxy_RectForCharacterRangeCallback = 773
#if ENABLE(APPLICATION_MANIFEST)
    , WebPageProxy_ApplicationManifestCallback = 774
#endif
#if PLATFORM(MAC)
    , WebPageProxy_AttributedStringForCharacterRangeCallback = 775
#endif
#if PLATFORM(MAC)
    , WebPageProxy_FontAtSelectionCallback = 776
#endif
    , WebPageProxy_FontAttributesCallback = 777
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_GestureCallback = 778
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_TouchesCallback = 779
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_SelectionContextCallback = 780
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_InterpretKeyEvent = 781
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_DidReceivePositionInformation = 782
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_SaveImageToLibrary = 783
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_ShowPlaybackTargetPicker = 784
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_CommitPotentialTapFailed = 785
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_DidNotHandleTapAsClick = 786
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_DidCompleteSyntheticClick = 787
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_DisableDoubleTapGesturesDuringTapIfNecessary = 788
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_HandleSmartMagnificationInformationForPotentialTap = 789
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_SelectionRectsCallback = 790
#endif
#if ENABLE(DATA_DETECTION)
    , WebPageProxy_SetDataDetectionResult = 791
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    , WebPageProxy_PrintFinishedCallback = 792
#endif
#if PLATFORM(COCOA)
    , WebPageProxy_DrawToPDFCallback = 793
#endif
#if PLATFORM(COCOA)
    , WebPageProxy_NowPlayingInfoCallback = 794
#endif
    , WebPageProxy_FindStringCallback = 795
    , WebPageProxy_PageScaleFactorDidChange = 796
    , WebPageProxy_PluginScaleFactorDidChange = 797
    , WebPageProxy_PluginZoomFactorDidChange = 798
#if USE(ATK)
    , WebPageProxy_BindAccessibilityTree = 799
#endif
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    , WebPageProxy_SetInputMethodState = 800
#endif
    , WebPageProxy_BackForwardAddItem = 801
    , WebPageProxy_BackForwardGoToItem = 802
    , WebPageProxy_BackForwardItemAtIndex = 803
    , WebPageProxy_BackForwardListCounts = 804
    , WebPageProxy_BackForwardClear = 805
    , WebPageProxy_WillGoToBackForwardListItem = 806
    , WebPageProxy_RegisterEditCommandForUndo = 807
    , WebPageProxy_ClearAllEditCommands = 808
    , WebPageProxy_RegisterInsertionUndoGrouping = 809
    , WebPageProxy_CanUndoRedo = 810
    , WebPageProxy_ExecuteUndoRedo = 811
    , WebPageProxy_LogDiagnosticMessage = 812
    , WebPageProxy_LogDiagnosticMessageWithResult = 813
    , WebPageProxy_LogDiagnosticMessageWithValue = 814
    , WebPageProxy_LogDiagnosticMessageWithEnhancedPrivacy = 815
    , WebPageProxy_LogDiagnosticMessageWithValueDictionary = 816
    , WebPageProxy_LogScrollingEvent = 817
    , WebPageProxy_EditorStateChanged = 818
    , WebPageProxy_CompositionWasCanceled = 819
    , WebPageProxy_SetHasHadSelectionChangesFromUserInteraction = 820
#if HAVE(TOUCH_BAR)
    , WebPageProxy_SetIsTouchBarUpdateSupressedForHiddenContentEditable = 821
#endif
#if HAVE(TOUCH_BAR)
    , WebPageProxy_SetIsNeverRichlyEditableForTouchBar = 822
#endif
    , WebPageProxy_RequestDOMPasteAccess = 823
    , WebPageProxy_DidCountStringMatches = 824
    , WebPageProxy_SetTextIndicator = 825
    , WebPageProxy_ClearTextIndicator = 826
    , WebPageProxy_DidFindString = 827
    , WebPageProxy_DidFailToFindString = 828
    , WebPageProxy_DidFindStringMatches = 829
    , WebPageProxy_DidGetImageForFindMatch = 830
    , WebPageProxy_ShowPopupMenu = 831
    , WebPageProxy_HidePopupMenu = 832
#if ENABLE(CONTEXT_MENUS)
    , WebPageProxy_ShowContextMenu = 833
#endif
    , WebPageProxy_ExceededDatabaseQuota = 834
    , WebPageProxy_ReachedApplicationCacheOriginQuota = 835
    , WebPageProxy_RequestGeolocationPermissionForFrame = 836
    , WebPageProxy_RevokeGeolocationAuthorizationToken = 837
#if ENABLE(MEDIA_STREAM)
    , WebPageProxy_RequestUserMediaPermissionForFrame = 838
#endif
#if ENABLE(MEDIA_STREAM)
    , WebPageProxy_EnumerateMediaDevicesForFrame = 839
    , WebPageProxy_EnumerateMediaDevicesForFrameReply = 840
#endif
#if ENABLE(MEDIA_STREAM)
    , WebPageProxy_BeginMonitoringCaptureDevices = 841
#endif
    , WebPageProxy_RequestNotificationPermission = 842
    , WebPageProxy_ShowNotification = 843
    , WebPageProxy_CancelNotification = 844
    , WebPageProxy_ClearNotifications = 845
    , WebPageProxy_DidDestroyNotification = 846
#if USE(UNIFIED_TEXT_CHECKING)
    , WebPageProxy_CheckTextOfParagraph = 847
#endif
    , WebPageProxy_CheckSpellingOfString = 848
    , WebPageProxy_CheckGrammarOfString = 849
    , WebPageProxy_SpellingUIIsShowing = 850
    , WebPageProxy_UpdateSpellingUIWithMisspelledWord = 851
    , WebPageProxy_UpdateSpellingUIWithGrammarString = 852
    , WebPageProxy_GetGuessesForWord = 853
    , WebPageProxy_LearnWord = 854
    , WebPageProxy_IgnoreWord = 855
    , WebPageProxy_RequestCheckingOfString = 856
#if ENABLE(DRAG_SUPPORT)
    , WebPageProxy_DidPerformDragControllerAction = 857
#endif
#if ENABLE(DRAG_SUPPORT)
    , WebPageProxy_DidEndDragging = 858
#endif
#if PLATFORM(COCOA) && ENABLE(DRAG_SUPPORT)
    , WebPageProxy_StartDrag = 859
#endif
#if PLATFORM(COCOA) && ENABLE(DRAG_SUPPORT)
    , WebPageProxy_SetPromisedDataForImage = 860
#endif
#if (PLATFORM(GTK) || PLATFORM(HBD)) && ENABLE(DRAG_SUPPORT)
    , WebPageProxy_StartDrag = 861
#endif
#if ENABLE(DRAG_SUPPORT)
    , WebPageProxy_DidPerformDragOperation = 862
#endif
#if ENABLE(DATA_INTERACTION)
    , WebPageProxy_DidHandleDragStartRequest = 863
#endif
#if ENABLE(DATA_INTERACTION)
    , WebPageProxy_DidHandleAdditionalDragItemsRequest = 864
#endif
#if ENABLE(DATA_INTERACTION)
    , WebPageProxy_WillReceiveEditDragSnapshot = 865
#endif
#if ENABLE(DATA_INTERACTION)
    , WebPageProxy_DidReceiveEditDragSnapshot = 866
#endif
#if PLATFORM(COCOA)
    , WebPageProxy_DidPerformDictionaryLookup = 867
#endif
#if PLATFORM(COCOA)
    , WebPageProxy_ExecuteSavedCommandBySelector = 868
#endif
#if PLATFORM(COCOA)
    , WebPageProxy_RegisterWebProcessAccessibilityToken = 869
#endif
#if PLATFORM(COCOA)
    , WebPageProxy_PluginFocusOrWindowFocusChanged = 870
#endif
#if PLATFORM(COCOA)
    , WebPageProxy_SetPluginComplexTextInputState = 871
#endif
#if PLATFORM(COCOA)
    , WebPageProxy_GetIsSpeaking = 872
#endif
#if PLATFORM(COCOA)
    , WebPageProxy_Speak = 873
#endif
#if PLATFORM(COCOA)
    , WebPageProxy_StopSpeaking = 874
#endif
#if PLATFORM(COCOA)
    , WebPageProxy_MakeFirstResponder = 875
#endif
#if PLATFORM(COCOA)
    , WebPageProxy_AssistiveTechnologyMakeFirstResponder = 876
#endif
#if PLATFORM(COCOA)
    , WebPageProxy_SearchWithSpotlight = 877
#endif
#if PLATFORM(COCOA)
    , WebPageProxy_SearchTheWeb = 878
#endif
#if HAVE(TOUCH_BAR)
    , WebPageProxy_TouchBarMenuDataChanged = 879
#endif
#if HAVE(TOUCH_BAR)
    , WebPageProxy_TouchBarMenuItemDataAdded = 880
#endif
#if HAVE(TOUCH_BAR)
    , WebPageProxy_TouchBarMenuItemDataRemoved = 881
#endif
#if USE(APPKIT)
    , WebPageProxy_SubstitutionsPanelIsShowing = 882
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    , WebPageProxy_toggleSmartInsertDelete = 883
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    , WebPageProxy_toggleAutomaticQuoteSubstitution = 884
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    , WebPageProxy_toggleAutomaticLinkDetection = 885
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    , WebPageProxy_toggleAutomaticDashSubstitution = 886
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    , WebPageProxy_toggleAutomaticTextReplacement = 887
#endif
#if PLATFORM(MAC)
    , WebPageProxy_ShowCorrectionPanel = 888
#endif
#if PLATFORM(MAC)
    , WebPageProxy_DismissCorrectionPanel = 889
#endif
#if PLATFORM(MAC)
    , WebPageProxy_DismissCorrectionPanelSoon = 890
#endif
#if PLATFORM(MAC)
    , WebPageProxy_RecordAutocorrectionResponse = 891
#endif
#if PLATFORM(MAC)
    , WebPageProxy_SetEditableElementIsFocused = 892
#endif
#if USE(DICTATION_ALTERNATIVES)
    , WebPageProxy_ShowDictationAlternativeUI = 893
#endif
#if USE(DICTATION_ALTERNATIVES)
    , WebPageProxy_RemoveDictationAlternatives = 894
#endif
#if USE(DICTATION_ALTERNATIVES)
    , WebPageProxy_DictationAlternatives = 895
#endif
#if PLATFORM(X11) && ENABLE(NETSCAPE_PLUGIN_API)
    , WebPageProxy_CreatePluginContainer = 896
#endif
#if PLATFORM(X11) && ENABLE(NETSCAPE_PLUGIN_API)
    , WebPageProxy_WindowedPluginGeometryDidChange = 897
#endif
#if PLATFORM(X11) && ENABLE(NETSCAPE_PLUGIN_API)
    , WebPageProxy_WindowedPluginVisibilityDidChange = 898
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_CouldNotRestorePageState = 899
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_RestorePageState = 900
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_RestorePageCenterAndScale = 901
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_DidGetTapHighlightGeometries = 902
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_ElementDidFocus = 903
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_ElementDidBlur = 904
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_UpdateInputContextAfterBlurringAndRefocusingElement = 905
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_FocusedElementDidChangeInputMode = 906
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_ScrollingNodeScrollWillStartScroll = 907
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_ScrollingNodeScrollDidEndScroll = 908
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_ShowInspectorHighlight = 909
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_HideInspectorHighlight = 910
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_FocusedElementInformationCallback = 911
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_ShowInspectorIndication = 912
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_HideInspectorIndication = 913
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_EnableInspectorNodeSearch = 914
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_DisableInspectorNodeSearch = 915
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_UpdateStringForFind = 916
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_HandleAutocorrectionContext = 917
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPageProxy_ShowDataDetectorsUIForPositionInformation = 918
#endif
    , WebPageProxy_DidChangeInspectorFrontendCount = 919
    , WebPageProxy_CreateInspectorTarget = 920
    , WebPageProxy_DestroyInspectorTarget = 921
    , WebPageProxy_SendMessageToInspectorFrontend = 922
    , WebPageProxy_SaveRecentSearches = 923
    , WebPageProxy_LoadRecentSearches = 924
    , WebPageProxy_SavePDFToFileInDownloadsFolder = 925
#if PLATFORM(COCOA)
    , WebPageProxy_SavePDFToTemporaryFolderAndOpenWithNativeApplication = 926
#endif
#if PLATFORM(COCOA)
    , WebPageProxy_OpenPDFFromTemporaryFolderWithNativeApplication = 927
#endif
#if ENABLE(PDFKIT_PLUGIN)
    , WebPageProxy_ShowPDFContextMenu = 928
#endif
#if ENABLE(NETSCAPE_PLUGIN_API)
    , WebPageProxy_FindPlugin = 929
#endif
    , WebPageProxy_DidUpdateActivityState = 930
#if ENABLE(WEB_CRYPTO)
    , WebPageProxy_WrapCryptoKey = 931
#endif
#if ENABLE(WEB_CRYPTO)
    , WebPageProxy_UnwrapCryptoKey = 932
#endif
#if (ENABLE(TELEPHONE_NUMBER_DETECTION) && PLATFORM(MAC))
    , WebPageProxy_ShowTelephoneNumberMenu = 933
#endif
#if USE(QUICK_LOOK)
    , WebPageProxy_DidStartLoadForQuickLookDocumentInMainFrame = 934
#endif
#if USE(QUICK_LOOK)
    , WebPageProxy_DidFinishLoadForQuickLookDocumentInMainFrame = 935
#endif
#if USE(QUICK_LOOK)
    , WebPageProxy_RequestPasswordForQuickLookDocumentInMainFrame = 936
    , WebPageProxy_RequestPasswordForQuickLookDocumentInMainFrameReply = 937
#endif
#if ENABLE(CONTENT_FILTERING)
    , WebPageProxy_ContentFilterDidBlockLoadForFrame = 938
#endif
    , WebPageProxy_IsPlayingMediaDidChange = 939
    , WebPageProxy_HandleAutoplayEvent = 940
#if ENABLE(MEDIA_SESSION)
    , WebPageProxy_HasMediaSessionWithActiveMediaElementsDidChange = 941
#endif
#if ENABLE(MEDIA_SESSION)
    , WebPageProxy_MediaSessionMetadataDidChange = 942
#endif
#if ENABLE(MEDIA_SESSION)
    , WebPageProxy_FocusedContentMediaElementDidChange = 943
#endif
#if PLATFORM(MAC)
    , WebPageProxy_DidPerformImmediateActionHitTest = 944
#endif
    , WebPageProxy_HandleMessage = 945
    , WebPageProxy_HandleSynchronousMessage = 946
    , WebPageProxy_HandleAutoFillButtonClick = 947
    , WebPageProxy_DidResignInputElementStrongPasswordAppearance = 948
    , WebPageProxy_ContentRuleListNotification = 949
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    , WebPageProxy_AddPlaybackTargetPickerClient = 950
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    , WebPageProxy_RemovePlaybackTargetPickerClient = 951
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    , WebPageProxy_ShowPlaybackTargetPicker = 952
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    , WebPageProxy_PlaybackTargetPickerClientStateDidChange = 953
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    , WebPageProxy_SetMockMediaPlaybackTargetPickerEnabled = 954
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    , WebPageProxy_SetMockMediaPlaybackTargetPickerState = 955
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    , WebPageProxy_MockMediaPlaybackTargetPickerDismissPopup = 956
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    , WebPageProxy_SetMockVideoPresentationModeEnabled = 957
#endif
#if ENABLE(POINTER_LOCK)
    , WebPageProxy_RequestPointerLock = 958
#endif
#if ENABLE(POINTER_LOCK)
    , WebPageProxy_RequestPointerUnlock = 959
#endif
    , WebPageProxy_DidFailToSuspendAfterProcessSwap = 960
    , WebPageProxy_DidSuspendAfterProcessSwap = 961
    , WebPageProxy_ImageOrMediaDocumentSizeChanged = 962
    , WebPageProxy_UseFixedLayoutDidChange = 963
    , WebPageProxy_FixedLayoutSizeDidChange = 964
#if ENABLE(VIDEO) && USE(GSTREAMER)
    , WebPageProxy_RequestInstallMissingMediaPlugins = 965
#endif
    , WebPageProxy_DidRestoreScrollPosition = 966
    , WebPageProxy_GetLoadDecisionForIcon = 967
    , WebPageProxy_FinishedLoadingIcon = 968
#if PLATFORM(MAC)
    , WebPageProxy_DidHandleAcceptedCandidate = 969
#endif
    , WebPageProxy_SetIsUsingHighPerformanceWebGL = 970
    , WebPageProxy_StartURLSchemeTask = 971
    , WebPageProxy_StopURLSchemeTask = 972
    , WebPageProxy_LoadSynchronousURLSchemeTask = 973
#if ENABLE(DEVICE_ORIENTATION)
    , WebPageProxy_ShouldAllowDeviceOrientationAndMotionAccess = 974
    , WebPageProxy_ShouldAllowDeviceOrientationAndMotionAccessReply = 975
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    , WebPageProxy_RegisterAttachmentIdentifierFromData = 976
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    , WebPageProxy_RegisterAttachmentIdentifierFromFilePath = 977
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    , WebPageProxy_RegisterAttachmentIdentifier = 978
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    , WebPageProxy_RegisterAttachmentsFromSerializedData = 979
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    , WebPageProxy_CloneAttachmentData = 980
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    , WebPageProxy_DidInsertAttachmentWithIdentifier = 981
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    , WebPageProxy_DidRemoveAttachmentWithIdentifier = 982
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    , WebPageProxy_SerializedAttachmentDataForIdentifiers = 983
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    , WebPageProxy_WritePromisedAttachmentToPasteboard = 984
#endif
    , WebPageProxy_SignedPublicKeyAndChallengeString = 985
#if ENABLE(SPEECH_SYNTHESIS)
    , WebPageProxy_SpeechSynthesisVoiceList = 986
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    , WebPageProxy_SpeechSynthesisSpeak = 987
    , WebPageProxy_SpeechSynthesisSpeakReply = 988
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    , WebPageProxy_SpeechSynthesisSetFinishedCallback = 989
    , WebPageProxy_SpeechSynthesisSetFinishedCallbackReply = 990
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    , WebPageProxy_SpeechSynthesisCancel = 991
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    , WebPageProxy_SpeechSynthesisPause = 992
    , WebPageProxy_SpeechSynthesisPauseReply = 993
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    , WebPageProxy_SpeechSynthesisResume = 994
    , WebPageProxy_SpeechSynthesisResumeReply = 995
#endif
    , WebPageProxy_ConfigureLoggingChannel = 996
#if PLATFORM(GTK) || PLATFORM(HBD)
    , WebPageProxy_ShowEmojiPicker = 997
    , WebPageProxy_ShowEmojiPickerReply = 998
#endif
#if HAVE(VISIBILITY_PROPAGATION_VIEW)
    , WebPageProxy_DidCreateContextForVisibilityPropagation = 999
#endif
#if ENABLE(WEB_AUTHN)
    , WebPageProxy_SetMockWebAuthenticationConfiguration = 1000
#endif
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    , WebPageProxy_SendMessageToWebView = 1001
#endif
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    , WebPageProxy_SendMessageToWebViewWithReply = 1002
    , WebPageProxy_SendMessageToWebViewWithReplyReply = 1003
#endif
    , WebPageProxy_DidFindTextManipulationItems = 1004
#if ENABLE(MEDIA_USAGE)
    , WebPageProxy_AddMediaUsageManagerSession = 1005
#endif
#if ENABLE(MEDIA_USAGE)
    , WebPageProxy_UpdateMediaUsageManagerSessionState = 1006
#endif
#if ENABLE(MEDIA_USAGE)
    , WebPageProxy_RemoveMediaUsageManagerSession = 1007
#endif
    , WebPageProxy_SetHasExecutedAppBoundBehaviorBeforeNavigation = 1008
#if PLATFORM(IOS_FAMILY)
    , WebPasteboardProxy_WriteURLToPasteboard = 1009
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPasteboardProxy_WriteWebContentToPasteboard = 1010
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPasteboardProxy_WriteImageToPasteboard = 1011
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPasteboardProxy_WriteStringToPasteboard = 1012
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPasteboardProxy_UpdateSupportedTypeIdentifiers = 1013
#endif
    , WebPasteboardProxy_WriteCustomData = 1014
    , WebPasteboardProxy_TypesSafeForDOMToReadAndWrite = 1015
    , WebPasteboardProxy_AllPasteboardItemInfo = 1016
    , WebPasteboardProxy_InformationForItemAtIndex = 1017
    , WebPasteboardProxy_GetPasteboardItemsCount = 1018
    , WebPasteboardProxy_ReadStringFromPasteboard = 1019
    , WebPasteboardProxy_ReadURLFromPasteboard = 1020
    , WebPasteboardProxy_ReadBufferFromPasteboard = 1021
    , WebPasteboardProxy_ContainsStringSafeForDOMToReadForType = 1022
#if PLATFORM(COCOA)
    , WebPasteboardProxy_GetNumberOfFiles = 1023
#endif
#if PLATFORM(COCOA)
    , WebPasteboardProxy_GetPasteboardTypes = 1024
#endif
#if PLATFORM(COCOA)
    , WebPasteboardProxy_GetPasteboardPathnamesForType = 1025
#endif
#if PLATFORM(COCOA)
    , WebPasteboardProxy_GetPasteboardStringForType = 1026
#endif
#if PLATFORM(COCOA)
    , WebPasteboardProxy_GetPasteboardStringsForType = 1027
#endif
#if PLATFORM(COCOA)
    , WebPasteboardProxy_GetPasteboardBufferForType = 1028
#endif
#if PLATFORM(COCOA)
    , WebPasteboardProxy_GetPasteboardChangeCount = 1029
#endif
#if PLATFORM(COCOA)
    , WebPasteboardProxy_GetPasteboardColor = 1030
#endif
#if PLATFORM(COCOA)
    , WebPasteboardProxy_GetPasteboardURL = 1031
#endif
#if PLATFORM(COCOA)
    , WebPasteboardProxy_AddPasteboardTypes = 1032
#endif
#if PLATFORM(COCOA)
    , WebPasteboardProxy_SetPasteboardTypes = 1033
#endif
#if PLATFORM(COCOA)
    , WebPasteboardProxy_SetPasteboardURL = 1034
#endif
#if PLATFORM(COCOA)
    , WebPasteboardProxy_SetPasteboardColor = 1035
#endif
#if PLATFORM(COCOA)
    , WebPasteboardProxy_SetPasteboardStringForType = 1036
#endif
#if PLATFORM(COCOA)
    , WebPasteboardProxy_SetPasteboardBufferForType = 1037
#endif
#if PLATFORM(COCOA)
    , WebPasteboardProxy_ContainsURLStringSuitableForLoading = 1038
#endif
#if PLATFORM(COCOA)
    , WebPasteboardProxy_URLStringSuitableForLoading = 1039
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    , WebPasteboardProxy_GetTypes = 1040
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    , WebPasteboardProxy_ReadText = 1041
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    , WebPasteboardProxy_ReadFilePaths = 1042
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    , WebPasteboardProxy_ReadBuffer = 1043
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    , WebPasteboardProxy_WriteToClipboard = 1044
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    , WebPasteboardProxy_ClearClipboard = 1045
#endif
#if USE(LIBWPE)
    , WebPasteboardProxy_GetPasteboardTypes = 1046
#endif
#if USE(LIBWPE)
    , WebPasteboardProxy_WriteWebContentToPasteboard = 1047
#endif
#if USE(LIBWPE)
    , WebPasteboardProxy_WriteStringToPasteboard = 1048
#endif
    , WebProcessPool_HandleMessage = 1049
    , WebProcessPool_HandleSynchronousMessage = 1050
#if ENABLE(GAMEPAD)
    , WebProcessPool_StartedUsingGamepads = 1051
#endif
#if ENABLE(GAMEPAD)
    , WebProcessPool_StoppedUsingGamepads = 1052
#endif
    , WebProcessPool_ReportWebContentCPUTime = 1053
    , WebProcessProxy_UpdateBackForwardItem = 1054
    , WebProcessProxy_DidDestroyFrame = 1055
    , WebProcessProxy_DidDestroyUserGestureToken = 1056
    , WebProcessProxy_ShouldTerminate = 1057
    , WebProcessProxy_EnableSuddenTermination = 1058
    , WebProcessProxy_DisableSuddenTermination = 1059
#if ENABLE(NETSCAPE_PLUGIN_API)
    , WebProcessProxy_GetPlugins = 1060
#endif
#if ENABLE(NETSCAPE_PLUGIN_API)
    , WebProcessProxy_GetPluginProcessConnection = 1061
#endif
    , WebProcessProxy_GetNetworkProcessConnection = 1062
#if ENABLE(GPU_PROCESS)
    , WebProcessProxy_GetGPUProcessConnection = 1063
#endif
    , WebProcessProxy_SetIsHoldingLockedFiles = 1064
    , WebProcessProxy_DidExceedActiveMemoryLimit = 1065
    , WebProcessProxy_DidExceedInactiveMemoryLimit = 1066
    , WebProcessProxy_DidExceedCPULimit = 1067
    , WebProcessProxy_StopResponsivenessTimer = 1068
    , WebProcessProxy_DidReceiveMainThreadPing = 1069
    , WebProcessProxy_DidReceiveBackgroundResponsivenessPing = 1070
    , WebProcessProxy_MemoryPressureStatusChanged = 1071
    , WebProcessProxy_DidExceedInactiveMemoryLimitWhileActive = 1072
    , WebProcessProxy_DidCollectPrewarmInformation = 1073
#if PLATFORM(COCOA)
    , WebProcessProxy_CacheMediaMIMETypes = 1074
#endif
#if PLATFORM(MAC)
    , WebProcessProxy_RequestHighPerformanceGPU = 1075
#endif
#if PLATFORM(MAC)
    , WebProcessProxy_ReleaseHighPerformanceGPU = 1076
#endif
#if PLATFORM(MAC) && ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    , WebProcessProxy_StartDisplayLink = 1077
#endif
#if PLATFORM(MAC) && ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    , WebProcessProxy_StopDisplayLink = 1078
#endif
    , WebProcessProxy_AddPlugInAutoStartOriginHash = 1079
    , WebProcessProxy_PlugInDidReceiveUserInteraction = 1080
#if PLATFORM(GTK) || PLATFORM(WPE)
    , WebProcessProxy_SendMessageToWebContext = 1081
#endif
#if PLATFORM(GTK) || PLATFORM(WPE)
    , WebProcessProxy_SendMessageToWebContextWithReply = 1082
    , WebProcessProxy_SendMessageToWebContextWithReplyReply = 1083
#endif
    , WebProcessProxy_DidCreateSleepDisabler = 1084
    , WebProcessProxy_DidDestroySleepDisabler = 1085
    , WebAutomationSession_DidEvaluateJavaScriptFunction = 1086
    , WebAutomationSession_DidTakeScreenshot = 1087
    , DownloadProxy_DidStart = 1088
    , DownloadProxy_DidReceiveAuthenticationChallenge = 1089
    , DownloadProxy_WillSendRequest = 1090
    , DownloadProxy_DecideDestinationWithSuggestedFilenameAsync = 1091
    , DownloadProxy_DidReceiveResponse = 1092
    , DownloadProxy_DidReceiveData = 1093
    , DownloadProxy_DidCreateDestination = 1094
    , DownloadProxy_DidFinish = 1095
    , DownloadProxy_DidFail = 1096
    , DownloadProxy_DidCancel = 1097
#if HAVE(VISIBILITY_PROPAGATION_VIEW)
    , GPUProcessProxy_DidCreateContextForVisibilityPropagation = 1098
#endif
    , RemoteWebInspectorProxy_FrontendDidClose = 1099
    , RemoteWebInspectorProxy_Reopen = 1100
    , RemoteWebInspectorProxy_ResetState = 1101
    , RemoteWebInspectorProxy_BringToFront = 1102
    , RemoteWebInspectorProxy_Save = 1103
    , RemoteWebInspectorProxy_Append = 1104
    , RemoteWebInspectorProxy_SetForcedAppearance = 1105
    , RemoteWebInspectorProxy_SetSheetRect = 1106
    , RemoteWebInspectorProxy_StartWindowDrag = 1107
    , RemoteWebInspectorProxy_OpenInNewTab = 1108
    , RemoteWebInspectorProxy_ShowCertificate = 1109
    , RemoteWebInspectorProxy_SendMessageToBackend = 1110
    , WebInspectorProxy_OpenLocalInspectorFrontend = 1111
    , WebInspectorProxy_SetFrontendConnection = 1112
    , WebInspectorProxy_SendMessageToBackend = 1113
    , WebInspectorProxy_FrontendLoaded = 1114
    , WebInspectorProxy_DidClose = 1115
    , WebInspectorProxy_BringToFront = 1116
    , WebInspectorProxy_BringInspectedPageToFront = 1117
    , WebInspectorProxy_Reopen = 1118
    , WebInspectorProxy_ResetState = 1119
    , WebInspectorProxy_SetForcedAppearance = 1120
    , WebInspectorProxy_InspectedURLChanged = 1121
    , WebInspectorProxy_ShowCertificate = 1122
    , WebInspectorProxy_ElementSelectionChanged = 1123
    , WebInspectorProxy_TimelineRecordingChanged = 1124
    , WebInspectorProxy_SetDeveloperPreferenceOverride = 1125
    , WebInspectorProxy_Save = 1126
    , WebInspectorProxy_Append = 1127
    , WebInspectorProxy_AttachBottom = 1128
    , WebInspectorProxy_AttachRight = 1129
    , WebInspectorProxy_AttachLeft = 1130
    , WebInspectorProxy_Detach = 1131
    , WebInspectorProxy_AttachAvailabilityChanged = 1132
    , WebInspectorProxy_SetAttachedWindowHeight = 1133
    , WebInspectorProxy_SetAttachedWindowWidth = 1134
    , WebInspectorProxy_SetSheetRect = 1135
    , WebInspectorProxy_StartWindowDrag = 1136
    , NetworkProcessProxy_DidReceiveAuthenticationChallenge = 1137
    , NetworkProcessProxy_NegotiatedLegacyTLS = 1138
    , NetworkProcessProxy_DidNegotiateModernTLS = 1139
    , NetworkProcessProxy_DidFetchWebsiteData = 1140
    , NetworkProcessProxy_DidDeleteWebsiteData = 1141
    , NetworkProcessProxy_DidDeleteWebsiteDataForOrigins = 1142
    , NetworkProcessProxy_DidSyncAllCookies = 1143
    , NetworkProcessProxy_TestProcessIncomingSyncMessagesWhenWaitingForSyncReply = 1144
    , NetworkProcessProxy_TerminateUnresponsiveServiceWorkerProcesses = 1145
    , NetworkProcessProxy_SetIsHoldingLockedFiles = 1146
    , NetworkProcessProxy_LogDiagnosticMessage = 1147
    , NetworkProcessProxy_LogDiagnosticMessageWithResult = 1148
    , NetworkProcessProxy_LogDiagnosticMessageWithValue = 1149
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcessProxy_LogTestingEvent = 1150
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcessProxy_NotifyResourceLoadStatisticsProcessed = 1151
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcessProxy_NotifyWebsiteDataDeletionForRegistrableDomainsFinished = 1152
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcessProxy_NotifyWebsiteDataScanForRegistrableDomainsFinished = 1153
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcessProxy_NotifyResourceLoadStatisticsTelemetryFinished = 1154
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcessProxy_RequestStorageAccessConfirm = 1155
    , NetworkProcessProxy_RequestStorageAccessConfirmReply = 1156
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcessProxy_DeleteWebsiteDataInUIProcessForRegistrableDomains = 1157
    , NetworkProcessProxy_DeleteWebsiteDataInUIProcessForRegistrableDomainsReply = 1158
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcessProxy_DidCommitCrossSiteLoadWithDataTransferFromPrevalentResource = 1159
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , NetworkProcessProxy_SetDomainsWithUserInteraction = 1160
#endif
#if ENABLE(CONTENT_EXTENSIONS)
    , NetworkProcessProxy_ContentExtensionRules = 1161
#endif
    , NetworkProcessProxy_RetrieveCacheStorageParameters = 1162
    , NetworkProcessProxy_TerminateWebProcess = 1163
#if ENABLE(SERVICE_WORKER)
    , NetworkProcessProxy_EstablishWorkerContextConnectionToNetworkProcess = 1164
    , NetworkProcessProxy_EstablishWorkerContextConnectionToNetworkProcessReply = 1165
#endif
#if ENABLE(SERVICE_WORKER)
    , NetworkProcessProxy_WorkerContextConnectionNoLongerNeeded = 1166
#endif
#if ENABLE(SERVICE_WORKER)
    , NetworkProcessProxy_RegisterServiceWorkerClientProcess = 1167
#endif
#if ENABLE(SERVICE_WORKER)
    , NetworkProcessProxy_UnregisterServiceWorkerClientProcess = 1168
#endif
    , NetworkProcessProxy_SetWebProcessHasUploads = 1169
    , NetworkProcessProxy_GetAppBoundDomains = 1170
    , NetworkProcessProxy_GetAppBoundDomainsReply = 1171
    , NetworkProcessProxy_RequestStorageSpace = 1172
    , NetworkProcessProxy_RequestStorageSpaceReply = 1173
    , NetworkProcessProxy_ResourceLoadDidSendRequest = 1174
    , NetworkProcessProxy_ResourceLoadDidPerformHTTPRedirection = 1175
    , NetworkProcessProxy_ResourceLoadDidReceiveChallenge = 1176
    , NetworkProcessProxy_ResourceLoadDidReceiveResponse = 1177
    , NetworkProcessProxy_ResourceLoadDidCompleteWithError = 1178
    , PluginProcessProxy_DidCreateWebProcessConnection = 1179
    , PluginProcessProxy_DidGetSitesWithData = 1180
    , PluginProcessProxy_DidDeleteWebsiteData = 1181
    , PluginProcessProxy_DidDeleteWebsiteDataForHostNames = 1182
#if PLATFORM(COCOA)
    , PluginProcessProxy_SetModalWindowIsShowing = 1183
#endif
#if PLATFORM(COCOA)
    , PluginProcessProxy_SetFullscreenWindowIsShowing = 1184
#endif
#if PLATFORM(COCOA)
    , PluginProcessProxy_LaunchProcess = 1185
#endif
#if PLATFORM(COCOA)
    , PluginProcessProxy_LaunchApplicationAtURL = 1186
#endif
#if PLATFORM(COCOA)
    , PluginProcessProxy_OpenURL = 1187
#endif
#if PLATFORM(COCOA)
    , PluginProcessProxy_OpenFile = 1188
#endif
    , WebUserContentControllerProxy_DidPostMessage = 1189
    , WebUserContentControllerProxy_DidPostMessageReply = 1190
    , WebProcess_InitializeWebProcess = 1191
    , WebProcess_SetWebsiteDataStoreParameters = 1192
    , WebProcess_CreateWebPage = 1193
    , WebProcess_PrewarmGlobally = 1194
    , WebProcess_PrewarmWithDomainInformation = 1195
    , WebProcess_SetCacheModel = 1196
    , WebProcess_RegisterURLSchemeAsEmptyDocument = 1197
    , WebProcess_RegisterURLSchemeAsSecure = 1198
    , WebProcess_RegisterURLSchemeAsBypassingContentSecurityPolicy = 1199
    , WebProcess_SetDomainRelaxationForbiddenForURLScheme = 1200
    , WebProcess_RegisterURLSchemeAsLocal = 1201
    , WebProcess_RegisterURLSchemeAsNoAccess = 1202
    , WebProcess_RegisterURLSchemeAsDisplayIsolated = 1203
    , WebProcess_RegisterURLSchemeAsCORSEnabled = 1204
    , WebProcess_RegisterURLSchemeAsCachePartitioned = 1205
    , WebProcess_RegisterURLSchemeAsCanDisplayOnlyIfCanRequest = 1206
    , WebProcess_SetDefaultRequestTimeoutInterval = 1207
    , WebProcess_SetAlwaysUsesComplexTextCodePath = 1208
    , WebProcess_SetShouldUseFontSmoothing = 1209
    , WebProcess_SetResourceLoadStatisticsEnabled = 1210
    , WebProcess_ClearResourceLoadStatistics = 1211
    , WebProcess_UserPreferredLanguagesChanged = 1212
    , WebProcess_FullKeyboardAccessModeChanged = 1213
    , WebProcess_DidAddPlugInAutoStartOriginHash = 1214
    , WebProcess_ResetPlugInAutoStartOriginHashes = 1215
    , WebProcess_SetPluginLoadClientPolicy = 1216
    , WebProcess_ResetPluginLoadClientPolicies = 1217
    , WebProcess_ClearPluginClientPolicies = 1218
    , WebProcess_RefreshPlugins = 1219
    , WebProcess_StartMemorySampler = 1220
    , WebProcess_StopMemorySampler = 1221
    , WebProcess_SetTextCheckerState = 1222
    , WebProcess_SetEnhancedAccessibility = 1223
    , WebProcess_GarbageCollectJavaScriptObjects = 1224
    , WebProcess_SetJavaScriptGarbageCollectorTimerEnabled = 1225
    , WebProcess_SetInjectedBundleParameter = 1226
    , WebProcess_SetInjectedBundleParameters = 1227
    , WebProcess_HandleInjectedBundleMessage = 1228
    , WebProcess_FetchWebsiteData = 1229
    , WebProcess_FetchWebsiteDataReply = 1230
    , WebProcess_DeleteWebsiteData = 1231
    , WebProcess_DeleteWebsiteDataReply = 1232
    , WebProcess_DeleteWebsiteDataForOrigins = 1233
    , WebProcess_DeleteWebsiteDataForOriginsReply = 1234
    , WebProcess_SetHiddenPageDOMTimerThrottlingIncreaseLimit = 1235
#if PLATFORM(COCOA)
    , WebProcess_SetQOS = 1236
#endif
    , WebProcess_SetMemoryCacheDisabled = 1237
#if ENABLE(SERVICE_CONTROLS)
    , WebProcess_SetEnabledServices = 1238
#endif
    , WebProcess_EnsureAutomationSessionProxy = 1239
    , WebProcess_DestroyAutomationSessionProxy = 1240
    , WebProcess_PrepareToSuspend = 1241
    , WebProcess_PrepareToSuspendReply = 1242
    , WebProcess_ProcessDidResume = 1243
    , WebProcess_MainThreadPing = 1244
    , WebProcess_BackgroundResponsivenessPing = 1245
#if ENABLE(GAMEPAD)
    , WebProcess_SetInitialGamepads = 1246
#endif
#if ENABLE(GAMEPAD)
    , WebProcess_GamepadConnected = 1247
#endif
#if ENABLE(GAMEPAD)
    , WebProcess_GamepadDisconnected = 1248
#endif
#if ENABLE(SERVICE_WORKER)
    , WebProcess_EstablishWorkerContextConnectionToNetworkProcess = 1249
    , WebProcess_EstablishWorkerContextConnectionToNetworkProcessReply = 1250
#endif
    , WebProcess_SetHasSuspendedPageProxy = 1251
    , WebProcess_SetIsInProcessCache = 1252
    , WebProcess_MarkIsNoLongerPrewarmed = 1253
    , WebProcess_GetActivePagesOriginsForTesting = 1254
    , WebProcess_GetActivePagesOriginsForTestingReply = 1255
#if PLATFORM(COCOA)
    , WebProcess_SetScreenProperties = 1256
#endif
#if PLATFORM(MAC) && ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    , WebProcess_ScrollerStylePreferenceChanged = 1257
#endif
#if PLATFORM(MAC) && ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    , WebProcess_DisplayConfigurationChanged = 1258
#endif
#if PLATFORM(IOS_FAMILY) && !PLATFORM(MACCATALYST)
    , WebProcess_BacklightLevelDidChange = 1259
#endif
    , WebProcess_IsJITEnabled = 1260
    , WebProcess_IsJITEnabledReply = 1261
#if PLATFORM(COCOA)
    , WebProcess_SetMediaMIMETypes = 1262
#endif
#if (PLATFORM(COCOA) && ENABLE(REMOTE_INSPECTOR))
    , WebProcess_EnableRemoteWebInspector = 1263
#endif
#if ENABLE(MEDIA_STREAM)
    , WebProcess_AddMockMediaDevice = 1264
#endif
#if ENABLE(MEDIA_STREAM)
    , WebProcess_ClearMockMediaDevices = 1265
#endif
#if ENABLE(MEDIA_STREAM)
    , WebProcess_RemoveMockMediaDevice = 1266
#endif
#if ENABLE(MEDIA_STREAM)
    , WebProcess_ResetMockMediaDevices = 1267
#endif
#if (ENABLE(MEDIA_STREAM) && ENABLE(SANDBOX_EXTENSIONS))
    , WebProcess_GrantUserMediaDeviceSandboxExtensions = 1268
#endif
#if (ENABLE(MEDIA_STREAM) && ENABLE(SANDBOX_EXTENSIONS))
    , WebProcess_RevokeUserMediaDeviceSandboxExtensions = 1269
#endif
    , WebProcess_ClearCurrentModifierStateForTesting = 1270
    , WebProcess_SetBackForwardCacheCapacity = 1271
    , WebProcess_ClearCachedPage = 1272
    , WebProcess_ClearCachedPageReply = 1273
#if PLATFORM(GTK) || PLATFORM(WPE)
    , WebProcess_SendMessageToWebExtension = 1274
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , WebProcess_SeedResourceLoadStatisticsForTesting = 1275
    , WebProcess_SeedResourceLoadStatisticsForTestingReply = 1276
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , WebProcess_SetThirdPartyCookieBlockingMode = 1277
    , WebProcess_SetThirdPartyCookieBlockingModeReply = 1278
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , WebProcess_SetDomainsWithUserInteraction = 1279
#endif
#if PLATFORM(IOS)
    , WebProcess_GrantAccessToAssetServices = 1280
#endif
#if PLATFORM(IOS)
    , WebProcess_RevokeAccessToAssetServices = 1281
#endif
#if PLATFORM(COCOA)
    , WebProcess_UnblockServicesRequiredByAccessibility = 1282
#endif
#if (PLATFORM(COCOA) && ENABLE(CFPREFS_DIRECT_MODE))
    , WebProcess_NotifyPreferencesChanged = 1283
#endif
#if (PLATFORM(COCOA) && ENABLE(CFPREFS_DIRECT_MODE))
    , WebProcess_UnblockPreferenceService = 1284
#endif
#if PLATFORM(GTK) && !USE(GTK4)
    , WebProcess_SetUseSystemAppearanceForScrollbars = 1285
#endif
    , WebAutomationSessionProxy_EvaluateJavaScriptFunction = 1286
    , WebAutomationSessionProxy_ResolveChildFrameWithOrdinal = 1287
    , WebAutomationSessionProxy_ResolveChildFrameWithOrdinalReply = 1288
    , WebAutomationSessionProxy_ResolveChildFrameWithNodeHandle = 1289
    , WebAutomationSessionProxy_ResolveChildFrameWithNodeHandleReply = 1290
    , WebAutomationSessionProxy_ResolveChildFrameWithName = 1291
    , WebAutomationSessionProxy_ResolveChildFrameWithNameReply = 1292
    , WebAutomationSessionProxy_ResolveParentFrame = 1293
    , WebAutomationSessionProxy_ResolveParentFrameReply = 1294
    , WebAutomationSessionProxy_FocusFrame = 1295
    , WebAutomationSessionProxy_ComputeElementLayout = 1296
    , WebAutomationSessionProxy_ComputeElementLayoutReply = 1297
    , WebAutomationSessionProxy_SelectOptionElement = 1298
    , WebAutomationSessionProxy_SelectOptionElementReply = 1299
    , WebAutomationSessionProxy_SetFilesForInputFileUpload = 1300
    , WebAutomationSessionProxy_SetFilesForInputFileUploadReply = 1301
    , WebAutomationSessionProxy_TakeScreenshot = 1302
    , WebAutomationSessionProxy_SnapshotRectForScreenshot = 1303
    , WebAutomationSessionProxy_SnapshotRectForScreenshotReply = 1304
    , WebAutomationSessionProxy_GetCookiesForFrame = 1305
    , WebAutomationSessionProxy_GetCookiesForFrameReply = 1306
    , WebAutomationSessionProxy_DeleteCookie = 1307
    , WebAutomationSessionProxy_DeleteCookieReply = 1308
    , WebIDBConnectionToServer_DidDeleteDatabase = 1309
    , WebIDBConnectionToServer_DidOpenDatabase = 1310
    , WebIDBConnectionToServer_DidAbortTransaction = 1311
    , WebIDBConnectionToServer_DidCommitTransaction = 1312
    , WebIDBConnectionToServer_DidCreateObjectStore = 1313
    , WebIDBConnectionToServer_DidDeleteObjectStore = 1314
    , WebIDBConnectionToServer_DidRenameObjectStore = 1315
    , WebIDBConnectionToServer_DidClearObjectStore = 1316
    , WebIDBConnectionToServer_DidCreateIndex = 1317
    , WebIDBConnectionToServer_DidDeleteIndex = 1318
    , WebIDBConnectionToServer_DidRenameIndex = 1319
    , WebIDBConnectionToServer_DidPutOrAdd = 1320
    , WebIDBConnectionToServer_DidGetRecord = 1321
    , WebIDBConnectionToServer_DidGetAllRecords = 1322
    , WebIDBConnectionToServer_DidGetCount = 1323
    , WebIDBConnectionToServer_DidDeleteRecord = 1324
    , WebIDBConnectionToServer_DidOpenCursor = 1325
    , WebIDBConnectionToServer_DidIterateCursor = 1326
    , WebIDBConnectionToServer_FireVersionChangeEvent = 1327
    , WebIDBConnectionToServer_DidStartTransaction = 1328
    , WebIDBConnectionToServer_DidCloseFromServer = 1329
    , WebIDBConnectionToServer_NotifyOpenDBRequestBlocked = 1330
    , WebIDBConnectionToServer_DidGetAllDatabaseNamesAndVersions = 1331
    , WebFullScreenManager_RequestExitFullScreen = 1332
    , WebFullScreenManager_WillEnterFullScreen = 1333
    , WebFullScreenManager_DidEnterFullScreen = 1334
    , WebFullScreenManager_WillExitFullScreen = 1335
    , WebFullScreenManager_DidExitFullScreen = 1336
    , WebFullScreenManager_SetAnimatingFullScreen = 1337
    , WebFullScreenManager_SaveScrollPosition = 1338
    , WebFullScreenManager_RestoreScrollPosition = 1339
    , WebFullScreenManager_SetFullscreenInsets = 1340
    , WebFullScreenManager_SetFullscreenAutoHideDuration = 1341
    , WebFullScreenManager_SetFullscreenControlsHidden = 1342
    , GPUProcessConnection_DidReceiveRemoteCommand = 1343
    , RemoteRenderingBackend_CreateImageBufferBackend = 1344
    , RemoteRenderingBackend_CommitImageBufferFlushContext = 1345
    , MediaPlayerPrivateRemote_NetworkStateChanged = 1346
    , MediaPlayerPrivateRemote_ReadyStateChanged = 1347
    , MediaPlayerPrivateRemote_FirstVideoFrameAvailable = 1348
    , MediaPlayerPrivateRemote_VolumeChanged = 1349
    , MediaPlayerPrivateRemote_MuteChanged = 1350
    , MediaPlayerPrivateRemote_TimeChanged = 1351
    , MediaPlayerPrivateRemote_DurationChanged = 1352
    , MediaPlayerPrivateRemote_RateChanged = 1353
    , MediaPlayerPrivateRemote_PlaybackStateChanged = 1354
    , MediaPlayerPrivateRemote_EngineFailedToLoad = 1355
    , MediaPlayerPrivateRemote_UpdateCachedState = 1356
    , MediaPlayerPrivateRemote_CharacteristicChanged = 1357
    , MediaPlayerPrivateRemote_SizeChanged = 1358
    , MediaPlayerPrivateRemote_AddRemoteAudioTrack = 1359
    , MediaPlayerPrivateRemote_RemoveRemoteAudioTrack = 1360
    , MediaPlayerPrivateRemote_RemoteAudioTrackConfigurationChanged = 1361
    , MediaPlayerPrivateRemote_AddRemoteTextTrack = 1362
    , MediaPlayerPrivateRemote_RemoveRemoteTextTrack = 1363
    , MediaPlayerPrivateRemote_RemoteTextTrackConfigurationChanged = 1364
    , MediaPlayerPrivateRemote_ParseWebVTTFileHeader = 1365
    , MediaPlayerPrivateRemote_ParseWebVTTCueData = 1366
    , MediaPlayerPrivateRemote_ParseWebVTTCueDataStruct = 1367
    , MediaPlayerPrivateRemote_AddDataCue = 1368
#if ENABLE(DATACUE_VALUE)
    , MediaPlayerPrivateRemote_AddDataCueWithType = 1369
#endif
#if ENABLE(DATACUE_VALUE)
    , MediaPlayerPrivateRemote_UpdateDataCue = 1370
#endif
#if ENABLE(DATACUE_VALUE)
    , MediaPlayerPrivateRemote_RemoveDataCue = 1371
#endif
    , MediaPlayerPrivateRemote_AddGenericCue = 1372
    , MediaPlayerPrivateRemote_UpdateGenericCue = 1373
    , MediaPlayerPrivateRemote_RemoveGenericCue = 1374
    , MediaPlayerPrivateRemote_AddRemoteVideoTrack = 1375
    , MediaPlayerPrivateRemote_RemoveRemoteVideoTrack = 1376
    , MediaPlayerPrivateRemote_RemoteVideoTrackConfigurationChanged = 1377
    , MediaPlayerPrivateRemote_RequestResource = 1378
    , MediaPlayerPrivateRemote_RequestResourceReply = 1379
    , MediaPlayerPrivateRemote_RemoveResource = 1380
    , MediaPlayerPrivateRemote_ResourceNotSupported = 1381
    , MediaPlayerPrivateRemote_EngineUpdated = 1382
    , MediaPlayerPrivateRemote_ActiveSourceBuffersChanged = 1383
#if ENABLE(ENCRYPTED_MEDIA)
    , MediaPlayerPrivateRemote_WaitingForKeyChanged = 1384
#endif
#if ENABLE(ENCRYPTED_MEDIA)
    , MediaPlayerPrivateRemote_InitializationDataEncountered = 1385
#endif
#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    , MediaPlayerPrivateRemote_MediaPlayerKeyNeeded = 1386
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    , MediaPlayerPrivateRemote_CurrentPlaybackTargetIsWirelessChanged = 1387
#endif
    , RemoteAudioDestinationProxy_RenderBuffer = 1388
    , RemoteAudioDestinationProxy_RenderBufferReply = 1389
    , RemoteAudioDestinationProxy_DidChangeIsPlaying = 1390
    , RemoteAudioSession_ConfigurationChanged = 1391
    , RemoteAudioSession_BeginInterruption = 1392
    , RemoteAudioSession_EndInterruption = 1393
    , RemoteCDMInstanceSession_UpdateKeyStatuses = 1394
    , RemoteCDMInstanceSession_SendMessage = 1395
    , RemoteCDMInstanceSession_SessionIdChanged = 1396
    , RemoteLegacyCDMSession_SendMessage = 1397
    , RemoteLegacyCDMSession_SendError = 1398
    , LibWebRTCCodecs_FailedDecoding = 1399
    , LibWebRTCCodecs_CompletedDecoding = 1400
    , LibWebRTCCodecs_CompletedEncoding = 1401
    , SampleBufferDisplayLayer_SetDidFail = 1402
    , WebGeolocationManager_DidChangePosition = 1403
    , WebGeolocationManager_DidFailToDeterminePosition = 1404
#if PLATFORM(IOS_FAMILY)
    , WebGeolocationManager_ResetPermissions = 1405
#endif
    , RemoteWebInspectorUI_Initialize = 1406
    , RemoteWebInspectorUI_UpdateFindString = 1407
#if ENABLE(INSPECTOR_TELEMETRY)
    , RemoteWebInspectorUI_SetDiagnosticLoggingAvailable = 1408
#endif
    , RemoteWebInspectorUI_DidSave = 1409
    , RemoteWebInspectorUI_DidAppend = 1410
    , RemoteWebInspectorUI_SendMessageToFrontend = 1411
    , WebInspector_Show = 1412
    , WebInspector_Close = 1413
    , WebInspector_SetAttached = 1414
    , WebInspector_ShowConsole = 1415
    , WebInspector_ShowResources = 1416
    , WebInspector_ShowMainResourceForFrame = 1417
    , WebInspector_OpenInNewTab = 1418
    , WebInspector_StartPageProfiling = 1419
    , WebInspector_StopPageProfiling = 1420
    , WebInspector_StartElementSelection = 1421
    , WebInspector_StopElementSelection = 1422
    , WebInspector_SetFrontendConnection = 1423
    , WebInspectorInterruptDispatcher_NotifyNeedDebuggerBreak = 1424
    , WebInspectorUI_EstablishConnection = 1425
    , WebInspectorUI_UpdateConnection = 1426
    , WebInspectorUI_AttachedBottom = 1427
    , WebInspectorUI_AttachedRight = 1428
    , WebInspectorUI_AttachedLeft = 1429
    , WebInspectorUI_Detached = 1430
    , WebInspectorUI_SetDockingUnavailable = 1431
    , WebInspectorUI_SetIsVisible = 1432
    , WebInspectorUI_UpdateFindString = 1433
#if ENABLE(INSPECTOR_TELEMETRY)
    , WebInspectorUI_SetDiagnosticLoggingAvailable = 1434
#endif
    , WebInspectorUI_ShowConsole = 1435
    , WebInspectorUI_ShowResources = 1436
    , WebInspectorUI_ShowMainResourceForFrame = 1437
    , WebInspectorUI_StartPageProfiling = 1438
    , WebInspectorUI_StopPageProfiling = 1439
    , WebInspectorUI_StartElementSelection = 1440
    , WebInspectorUI_StopElementSelection = 1441
    , WebInspectorUI_DidSave = 1442
    , WebInspectorUI_DidAppend = 1443
    , WebInspectorUI_SendMessageToFrontend = 1444
    , LibWebRTCNetwork_SignalReadPacket = 1445
    , LibWebRTCNetwork_SignalSentPacket = 1446
    , LibWebRTCNetwork_SignalAddressReady = 1447
    , LibWebRTCNetwork_SignalConnect = 1448
    , LibWebRTCNetwork_SignalClose = 1449
    , LibWebRTCNetwork_SignalNewConnection = 1450
    , WebMDNSRegister_FinishedRegisteringMDNSName = 1451
    , WebRTCMonitor_NetworksChanged = 1452
    , WebRTCResolver_SetResolvedAddress = 1453
    , WebRTCResolver_ResolvedAddressError = 1454
#if ENABLE(SHAREABLE_RESOURCE)
    , NetworkProcessConnection_DidCacheResource = 1455
#endif
    , NetworkProcessConnection_DidFinishPingLoad = 1456
    , NetworkProcessConnection_DidFinishPreconnection = 1457
    , NetworkProcessConnection_SetOnLineState = 1458
    , NetworkProcessConnection_CookieAcceptPolicyChanged = 1459
#if HAVE(COOKIE_CHANGE_LISTENER_API)
    , NetworkProcessConnection_CookiesAdded = 1460
#endif
#if HAVE(COOKIE_CHANGE_LISTENER_API)
    , NetworkProcessConnection_CookiesDeleted = 1461
#endif
#if HAVE(COOKIE_CHANGE_LISTENER_API)
    , NetworkProcessConnection_AllCookiesDeleted = 1462
#endif
    , NetworkProcessConnection_CheckProcessLocalPortForActivity = 1463
    , NetworkProcessConnection_CheckProcessLocalPortForActivityReply = 1464
    , NetworkProcessConnection_MessagesAvailableForPort = 1465
    , NetworkProcessConnection_BroadcastConsoleMessage = 1466
    , WebResourceLoader_WillSendRequest = 1467
    , WebResourceLoader_DidSendData = 1468
    , WebResourceLoader_DidReceiveResponse = 1469
    , WebResourceLoader_DidReceiveData = 1470
    , WebResourceLoader_DidReceiveSharedBuffer = 1471
    , WebResourceLoader_DidFinishResourceLoad = 1472
    , WebResourceLoader_DidFailResourceLoad = 1473
    , WebResourceLoader_DidFailServiceWorkerLoad = 1474
    , WebResourceLoader_ServiceWorkerDidNotHandle = 1475
    , WebResourceLoader_DidBlockAuthenticationChallenge = 1476
    , WebResourceLoader_StopLoadingAfterXFrameOptionsOrContentSecurityPolicyDenied = 1477
#if ENABLE(SHAREABLE_RESOURCE)
    , WebResourceLoader_DidReceiveResource = 1478
#endif
    , WebSocketChannel_DidConnect = 1479
    , WebSocketChannel_DidClose = 1480
    , WebSocketChannel_DidReceiveText = 1481
    , WebSocketChannel_DidReceiveBinaryData = 1482
    , WebSocketChannel_DidReceiveMessageError = 1483
    , WebSocketChannel_DidSendHandshakeRequest = 1484
    , WebSocketChannel_DidReceiveHandshakeResponse = 1485
    , WebSocketStream_DidOpenSocketStream = 1486
    , WebSocketStream_DidCloseSocketStream = 1487
    , WebSocketStream_DidReceiveSocketStreamData = 1488
    , WebSocketStream_DidFailToReceiveSocketStreamData = 1489
    , WebSocketStream_DidUpdateBufferedAmount = 1490
    , WebSocketStream_DidFailSocketStream = 1491
    , WebSocketStream_DidSendData = 1492
    , WebSocketStream_DidSendHandshake = 1493
    , WebNotificationManager_DidShowNotification = 1494
    , WebNotificationManager_DidClickNotification = 1495
    , WebNotificationManager_DidCloseNotifications = 1496
    , WebNotificationManager_DidUpdateNotificationDecision = 1497
    , WebNotificationManager_DidRemoveNotificationDecisions = 1498
    , PluginProcessConnection_SetException = 1499
    , PluginProcessConnectionManager_PluginProcessCrashed = 1500
    , PluginProxy_LoadURL = 1501
    , PluginProxy_Update = 1502
    , PluginProxy_ProxiesForURL = 1503
    , PluginProxy_CookiesForURL = 1504
    , PluginProxy_SetCookiesForURL = 1505
    , PluginProxy_GetAuthenticationInfo = 1506
    , PluginProxy_GetPluginElementNPObject = 1507
    , PluginProxy_Evaluate = 1508
    , PluginProxy_CancelStreamLoad = 1509
    , PluginProxy_ContinueStreamLoad = 1510
    , PluginProxy_CancelManualStreamLoad = 1511
    , PluginProxy_SetStatusbarText = 1512
#if PLATFORM(COCOA)
    , PluginProxy_PluginFocusOrWindowFocusChanged = 1513
#endif
#if PLATFORM(COCOA)
    , PluginProxy_SetComplexTextInputState = 1514
#endif
#if PLATFORM(COCOA)
    , PluginProxy_SetLayerHostingContextID = 1515
#endif
#if PLATFORM(X11)
    , PluginProxy_CreatePluginContainer = 1516
#endif
#if PLATFORM(X11)
    , PluginProxy_WindowedPluginGeometryDidChange = 1517
#endif
#if PLATFORM(X11)
    , PluginProxy_WindowedPluginVisibilityDidChange = 1518
#endif
    , PluginProxy_DidCreatePlugin = 1519
    , PluginProxy_DidFailToCreatePlugin = 1520
    , PluginProxy_SetPluginIsPlayingAudio = 1521
    , WebSWClientConnection_JobRejectedInServer = 1522
    , WebSWClientConnection_RegistrationJobResolvedInServer = 1523
    , WebSWClientConnection_StartScriptFetchForServer = 1524
    , WebSWClientConnection_UpdateRegistrationState = 1525
    , WebSWClientConnection_UpdateWorkerState = 1526
    , WebSWClientConnection_FireUpdateFoundEvent = 1527
    , WebSWClientConnection_SetRegistrationLastUpdateTime = 1528
    , WebSWClientConnection_SetRegistrationUpdateViaCache = 1529
    , WebSWClientConnection_NotifyClientsOfControllerChange = 1530
    , WebSWClientConnection_SetSWOriginTableIsImported = 1531
    , WebSWClientConnection_SetSWOriginTableSharedMemory = 1532
    , WebSWClientConnection_PostMessageToServiceWorkerClient = 1533
    , WebSWClientConnection_DidMatchRegistration = 1534
    , WebSWClientConnection_DidGetRegistrations = 1535
    , WebSWClientConnection_RegistrationReady = 1536
    , WebSWClientConnection_SetDocumentIsControlled = 1537
    , WebSWClientConnection_SetDocumentIsControlledReply = 1538
    , WebSWContextManagerConnection_InstallServiceWorker = 1539
    , WebSWContextManagerConnection_StartFetch = 1540
    , WebSWContextManagerConnection_CancelFetch = 1541
    , WebSWContextManagerConnection_ContinueDidReceiveFetchResponse = 1542
    , WebSWContextManagerConnection_PostMessageToServiceWorker = 1543
    , WebSWContextManagerConnection_FireInstallEvent = 1544
    , WebSWContextManagerConnection_FireActivateEvent = 1545
    , WebSWContextManagerConnection_TerminateWorker = 1546
    , WebSWContextManagerConnection_FindClientByIdentifierCompleted = 1547
    , WebSWContextManagerConnection_MatchAllCompleted = 1548
    , WebSWContextManagerConnection_SetUserAgent = 1549
    , WebSWContextManagerConnection_UpdatePreferencesStore = 1550
    , WebSWContextManagerConnection_Close = 1551
    , WebSWContextManagerConnection_SetThrottleState = 1552
    , WebUserContentController_AddContentWorlds = 1553
    , WebUserContentController_RemoveContentWorlds = 1554
    , WebUserContentController_AddUserScripts = 1555
    , WebUserContentController_RemoveUserScript = 1556
    , WebUserContentController_RemoveAllUserScripts = 1557
    , WebUserContentController_AddUserStyleSheets = 1558
    , WebUserContentController_RemoveUserStyleSheet = 1559
    , WebUserContentController_RemoveAllUserStyleSheets = 1560
    , WebUserContentController_AddUserScriptMessageHandlers = 1561
    , WebUserContentController_RemoveUserScriptMessageHandler = 1562
    , WebUserContentController_RemoveAllUserScriptMessageHandlersForWorlds = 1563
    , WebUserContentController_RemoveAllUserScriptMessageHandlers = 1564
#if ENABLE(CONTENT_EXTENSIONS)
    , WebUserContentController_AddContentRuleLists = 1565
#endif
#if ENABLE(CONTENT_EXTENSIONS)
    , WebUserContentController_RemoveContentRuleList = 1566
#endif
#if ENABLE(CONTENT_EXTENSIONS)
    , WebUserContentController_RemoveAllContentRuleLists = 1567
#endif
#if USE(COORDINATED_GRAPHICS) || USE(TEXTURE_MAPPER)
    , DrawingArea_UpdateBackingStoreState = 1568
#endif
    , DrawingArea_DidUpdate = 1569
#if PLATFORM(COCOA)
    , DrawingArea_UpdateGeometry = 1570
#endif
#if PLATFORM(COCOA)
    , DrawingArea_SetDeviceScaleFactor = 1571
#endif
#if PLATFORM(COCOA)
    , DrawingArea_SetColorSpace = 1572
#endif
#if PLATFORM(COCOA)
    , DrawingArea_SetViewExposedRect = 1573
#endif
#if PLATFORM(COCOA)
    , DrawingArea_AdjustTransientZoom = 1574
#endif
#if PLATFORM(COCOA)
    , DrawingArea_CommitTransientZoom = 1575
#endif
#if PLATFORM(COCOA)
    , DrawingArea_AcceleratedAnimationDidStart = 1576
#endif
#if PLATFORM(COCOA)
    , DrawingArea_AcceleratedAnimationDidEnd = 1577
#endif
#if PLATFORM(COCOA)
    , DrawingArea_AddTransactionCallbackID = 1578
#endif
    , EventDispatcher_WheelEvent = 1579
#if ENABLE(IOS_TOUCH_EVENTS)
    , EventDispatcher_TouchEvent = 1580
#endif
#if ENABLE(MAC_GESTURE_EVENTS)
    , EventDispatcher_GestureEvent = 1581
#endif
#if ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    , EventDispatcher_DisplayWasRefreshed = 1582
#endif
    , VisitedLinkTableController_SetVisitedLinkTable = 1583
    , VisitedLinkTableController_VisitedLinkStateChanged = 1584
    , VisitedLinkTableController_AllVisitedLinkStateChanged = 1585
    , VisitedLinkTableController_RemoveAllVisitedLinks = 1586
    , WebPage_SetInitialFocus = 1587
    , WebPage_SetInitialFocusReply = 1588
    , WebPage_SetActivityState = 1589
    , WebPage_SetLayerHostingMode = 1590
    , WebPage_SetBackgroundColor = 1591
    , WebPage_AddConsoleMessage = 1592
    , WebPage_SendCSPViolationReport = 1593
    , WebPage_EnqueueSecurityPolicyViolationEvent = 1594
    , WebPage_TestProcessIncomingSyncMessagesWhenWaitingForSyncReply = 1595
#if PLATFORM(COCOA)
    , WebPage_SetTopContentInsetFenced = 1596
#endif
    , WebPage_SetTopContentInset = 1597
    , WebPage_SetUnderlayColor = 1598
    , WebPage_ViewWillStartLiveResize = 1599
    , WebPage_ViewWillEndLiveResize = 1600
    , WebPage_ExecuteEditCommandWithCallback = 1601
    , WebPage_ExecuteEditCommandWithCallbackReply = 1602
    , WebPage_KeyEvent = 1603
    , WebPage_MouseEvent = 1604
#if PLATFORM(IOS_FAMILY)
    , WebPage_SetViewportConfigurationViewLayoutSize = 1605
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_SetMaximumUnobscuredSize = 1606
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_SetDeviceOrientation = 1607
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_SetOverrideViewportArguments = 1608
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_DynamicViewportSizeUpdate = 1609
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_SetScreenIsBeingCaptured = 1610
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_HandleTap = 1611
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_PotentialTapAtPosition = 1612
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_CommitPotentialTap = 1613
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_CancelPotentialTap = 1614
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_TapHighlightAtPosition = 1615
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_DidRecognizeLongPress = 1616
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_HandleDoubleTapForDoubleClickAtPoint = 1617
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_InspectorNodeSearchMovedToPosition = 1618
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_InspectorNodeSearchEndedAtPosition = 1619
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_BlurFocusedElement = 1620
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_SelectWithGesture = 1621
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_UpdateSelectionWithTouches = 1622
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_SelectWithTwoTouches = 1623
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_ExtendSelection = 1624
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_SelectWordBackward = 1625
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_MoveSelectionByOffset = 1626
    , WebPage_MoveSelectionByOffsetReply = 1627
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_SelectTextWithGranularityAtPoint = 1628
    , WebPage_SelectTextWithGranularityAtPointReply = 1629
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_SelectPositionAtBoundaryWithDirection = 1630
    , WebPage_SelectPositionAtBoundaryWithDirectionReply = 1631
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_MoveSelectionAtBoundaryWithDirection = 1632
    , WebPage_MoveSelectionAtBoundaryWithDirectionReply = 1633
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_SelectPositionAtPoint = 1634
    , WebPage_SelectPositionAtPointReply = 1635
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_BeginSelectionInDirection = 1636
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_UpdateSelectionWithExtentPoint = 1637
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_UpdateSelectionWithExtentPointAndBoundary = 1638
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_RequestDictationContext = 1639
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_ReplaceDictatedText = 1640
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_ReplaceSelectedText = 1641
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_RequestAutocorrectionData = 1642
    , WebPage_RequestAutocorrectionDataReply = 1643
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_ApplyAutocorrection = 1644
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_SyncApplyAutocorrection = 1645
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_RequestAutocorrectionContext = 1646
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_RequestEvasionRectsAboveSelection = 1647
    , WebPage_RequestEvasionRectsAboveSelectionReply = 1648
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_GetPositionInformation = 1649
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_RequestPositionInformation = 1650
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_StartInteractionWithElementContextOrPosition = 1651
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_StopInteraction = 1652
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_PerformActionOnElement = 1653
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_FocusNextFocusedElement = 1654
    , WebPage_FocusNextFocusedElementReply = 1655
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_SetFocusedElementValue = 1656
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_AutofillLoginCredentials = 1657
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_SetFocusedElementValueAsNumber = 1658
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_SetFocusedElementSelectedIndex = 1659
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_ApplicationWillResignActive = 1660
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_ApplicationDidEnterBackground = 1661
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_ApplicationDidFinishSnapshottingAfterEnteringBackground = 1662
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_ApplicationWillEnterForeground = 1663
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_ApplicationDidBecomeActive = 1664
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_ApplicationDidEnterBackgroundForMedia = 1665
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_ApplicationWillEnterForegroundForMedia = 1666
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_ContentSizeCategoryDidChange = 1667
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_GetSelectionContext = 1668
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_SetAllowsMediaDocumentInlinePlayback = 1669
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_HandleTwoFingerTapAtPoint = 1670
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_HandleStylusSingleTapAtPoint = 1671
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_SetForceAlwaysUserScalable = 1672
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_GetRectsForGranularityWithSelectionOffset = 1673
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_GetRectsAtSelectionOffsetWithText = 1674
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_StoreSelectionForAccessibility = 1675
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_StartAutoscrollAtPosition = 1676
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_CancelAutoscroll = 1677
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_RequestFocusedElementInformation = 1678
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_HardwareKeyboardAvailabilityChanged = 1679
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_SetIsShowingInputViewForFocusedElement = 1680
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_UpdateSelectionWithDelta = 1681
    , WebPage_UpdateSelectionWithDeltaReply = 1682
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_RequestDocumentEditingContext = 1683
    , WebPage_RequestDocumentEditingContextReply = 1684
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_GenerateSyntheticEditingCommand = 1685
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_SetShouldRevealCurrentSelectionAfterInsertion = 1686
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_InsertTextPlaceholder = 1687
    , WebPage_InsertTextPlaceholderReply = 1688
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_RemoveTextPlaceholder = 1689
    , WebPage_RemoveTextPlaceholderReply = 1690
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_TextInputContextsInRect = 1691
    , WebPage_TextInputContextsInRectReply = 1692
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_FocusTextInputContextAndPlaceCaret = 1693
    , WebPage_FocusTextInputContextAndPlaceCaretReply = 1694
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_ClearServiceWorkerEntitlementOverride = 1695
    , WebPage_ClearServiceWorkerEntitlementOverrideReply = 1696
#endif
    , WebPage_SetControlledByAutomation = 1697
    , WebPage_ConnectInspector = 1698
    , WebPage_DisconnectInspector = 1699
    , WebPage_SendMessageToTargetBackend = 1700
#if ENABLE(REMOTE_INSPECTOR)
    , WebPage_SetIndicating = 1701
#endif
#if ENABLE(IOS_TOUCH_EVENTS)
    , WebPage_ResetPotentialTapSecurityOrigin = 1702
#endif
#if ENABLE(IOS_TOUCH_EVENTS)
    , WebPage_TouchEventSync = 1703
#endif
#if !ENABLE(IOS_TOUCH_EVENTS) && ENABLE(TOUCH_EVENTS)
    , WebPage_TouchEvent = 1704
#endif
    , WebPage_CancelPointer = 1705
    , WebPage_TouchWithIdentifierWasRemoved = 1706
#if ENABLE(INPUT_TYPE_COLOR)
    , WebPage_DidEndColorPicker = 1707
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    , WebPage_DidChooseColor = 1708
#endif
#if ENABLE(DATALIST_ELEMENT)
    , WebPage_DidSelectDataListOption = 1709
#endif
#if ENABLE(DATALIST_ELEMENT)
    , WebPage_DidCloseSuggestions = 1710
#endif
#if ENABLE(CONTEXT_MENUS)
    , WebPage_ContextMenuHidden = 1711
#endif
#if ENABLE(CONTEXT_MENUS)
    , WebPage_ContextMenuForKeyEvent = 1712
#endif
    , WebPage_ScrollBy = 1713
    , WebPage_CenterSelectionInVisibleArea = 1714
    , WebPage_GoToBackForwardItem = 1715
    , WebPage_TryRestoreScrollPosition = 1716
    , WebPage_LoadURLInFrame = 1717
    , WebPage_LoadDataInFrame = 1718
    , WebPage_LoadRequest = 1719
    , WebPage_LoadRequestWaitingForProcessLaunch = 1720
    , WebPage_LoadData = 1721
    , WebPage_LoadAlternateHTML = 1722
    , WebPage_NavigateToPDFLinkWithSimulatedClick = 1723
    , WebPage_Reload = 1724
    , WebPage_StopLoading = 1725
    , WebPage_StopLoadingFrame = 1726
    , WebPage_RestoreSession = 1727
    , WebPage_UpdateBackForwardListForReattach = 1728
    , WebPage_SetCurrentHistoryItemForReattach = 1729
    , WebPage_DidRemoveBackForwardItem = 1730
    , WebPage_UpdateWebsitePolicies = 1731
    , WebPage_NotifyUserScripts = 1732
    , WebPage_DidReceivePolicyDecision = 1733
    , WebPage_ContinueWillSubmitForm = 1734
    , WebPage_ClearSelection = 1735
    , WebPage_RestoreSelectionInFocusedEditableElement = 1736
    , WebPage_GetContentsAsString = 1737
    , WebPage_GetAllFrames = 1738
    , WebPage_GetAllFramesReply = 1739
#if PLATFORM(COCOA)
    , WebPage_GetContentsAsAttributedString = 1740
    , WebPage_GetContentsAsAttributedStringReply = 1741
#endif
#if ENABLE(MHTML)
    , WebPage_GetContentsAsMHTMLData = 1742
#endif
    , WebPage_GetMainResourceDataOfFrame = 1743
    , WebPage_GetResourceDataFromFrame = 1744
    , WebPage_GetRenderTreeExternalRepresentation = 1745
    , WebPage_GetSelectionOrContentsAsString = 1746
    , WebPage_GetSelectionAsWebArchiveData = 1747
    , WebPage_GetSourceForFrame = 1748
    , WebPage_GetWebArchiveOfFrame = 1749
    , WebPage_RunJavaScriptInFrameInScriptWorld = 1750
    , WebPage_ForceRepaint = 1751
    , WebPage_SelectAll = 1752
    , WebPage_ScheduleFullEditorStateUpdate = 1753
#if PLATFORM(COCOA)
    , WebPage_PerformDictionaryLookupOfCurrentSelection = 1754
#endif
#if PLATFORM(COCOA)
    , WebPage_PerformDictionaryLookupAtLocation = 1755
#endif
#if ENABLE(DATA_DETECTION)
    , WebPage_DetectDataInAllFrames = 1756
    , WebPage_DetectDataInAllFramesReply = 1757
#endif
#if ENABLE(DATA_DETECTION)
    , WebPage_RemoveDataDetectedLinks = 1758
    , WebPage_RemoveDataDetectedLinksReply = 1759
#endif
    , WebPage_ChangeFont = 1760
    , WebPage_ChangeFontAttributes = 1761
    , WebPage_PreferencesDidChange = 1762
    , WebPage_SetUserAgent = 1763
    , WebPage_SetCustomTextEncodingName = 1764
    , WebPage_SuspendActiveDOMObjectsAndAnimations = 1765
    , WebPage_ResumeActiveDOMObjectsAndAnimations = 1766
    , WebPage_Close = 1767
    , WebPage_TryClose = 1768
    , WebPage_TryCloseReply = 1769
    , WebPage_SetEditable = 1770
    , WebPage_ValidateCommand = 1771
    , WebPage_ExecuteEditCommand = 1772
    , WebPage_IncreaseListLevel = 1773
    , WebPage_DecreaseListLevel = 1774
    , WebPage_ChangeListType = 1775
    , WebPage_SetBaseWritingDirection = 1776
    , WebPage_SetNeedsFontAttributes = 1777
    , WebPage_RequestFontAttributesAtSelectionStart = 1778
    , WebPage_DidRemoveEditCommand = 1779
    , WebPage_ReapplyEditCommand = 1780
    , WebPage_UnapplyEditCommand = 1781
    , WebPage_SetPageAndTextZoomFactors = 1782
    , WebPage_SetPageZoomFactor = 1783
    , WebPage_SetTextZoomFactor = 1784
    , WebPage_WindowScreenDidChange = 1785
    , WebPage_AccessibilitySettingsDidChange = 1786
    , WebPage_ScalePage = 1787
    , WebPage_ScalePageInViewCoordinates = 1788
    , WebPage_ScaleView = 1789
    , WebPage_SetUseFixedLayout = 1790
    , WebPage_SetFixedLayoutSize = 1791
    , WebPage_ListenForLayoutMilestones = 1792
    , WebPage_SetSuppressScrollbarAnimations = 1793
    , WebPage_SetEnableVerticalRubberBanding = 1794
    , WebPage_SetEnableHorizontalRubberBanding = 1795
    , WebPage_SetBackgroundExtendsBeyondPage = 1796
    , WebPage_SetPaginationMode = 1797
    , WebPage_SetPaginationBehavesLikeColumns = 1798
    , WebPage_SetPageLength = 1799
    , WebPage_SetGapBetweenPages = 1800
    , WebPage_SetPaginationLineGridEnabled = 1801
    , WebPage_PostInjectedBundleMessage = 1802
    , WebPage_FindString = 1803
    , WebPage_FindStringMatches = 1804
    , WebPage_GetImageForFindMatch = 1805
    , WebPage_SelectFindMatch = 1806
    , WebPage_IndicateFindMatch = 1807
    , WebPage_HideFindUI = 1808
    , WebPage_CountStringMatches = 1809
    , WebPage_ReplaceMatches = 1810
    , WebPage_AddMIMETypeWithCustomContentProvider = 1811
#if (PLATFORM(GTK) || PLATFORM(HBD)) && ENABLE(DRAG_SUPPORT)
    , WebPage_PerformDragControllerAction = 1812
#endif
#if !PLATFORM(GTK) && !PLATFORM(HBD) && ENABLE(DRAG_SUPPORT)
    , WebPage_PerformDragControllerAction = 1813
#endif
#if ENABLE(DRAG_SUPPORT)
    , WebPage_DidStartDrag = 1814
#endif
#if ENABLE(DRAG_SUPPORT)
    , WebPage_DragEnded = 1815
#endif
#if ENABLE(DRAG_SUPPORT)
    , WebPage_DragCancelled = 1816
#endif
#if PLATFORM(IOS_FAMILY) && ENABLE(DRAG_SUPPORT)
    , WebPage_RequestDragStart = 1817
#endif
#if PLATFORM(IOS_FAMILY) && ENABLE(DRAG_SUPPORT)
    , WebPage_RequestAdditionalItemsForDragSession = 1818
#endif
#if PLATFORM(IOS_FAMILY) && ENABLE(DRAG_SUPPORT)
    , WebPage_InsertDroppedImagePlaceholders = 1819
    , WebPage_InsertDroppedImagePlaceholdersReply = 1820
#endif
#if PLATFORM(IOS_FAMILY) && ENABLE(DRAG_SUPPORT)
    , WebPage_DidConcludeDrop = 1821
#endif
    , WebPage_DidChangeSelectedIndexForActivePopupMenu = 1822
    , WebPage_SetTextForActivePopupMenu = 1823
#if PLATFORM(GTK) || PLATFORM(HBD)
    , WebPage_FailedToShowPopupMenu = 1824
#endif
#if ENABLE(CONTEXT_MENUS)
    , WebPage_DidSelectItemFromActiveContextMenu = 1825
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_DidChooseFilesForOpenPanelWithDisplayStringAndIcon = 1826
#endif
    , WebPage_DidChooseFilesForOpenPanel = 1827
    , WebPage_DidCancelForOpenPanel = 1828
#if ENABLE(SANDBOX_EXTENSIONS)
    , WebPage_ExtendSandboxForFilesFromOpenPanel = 1829
#endif
    , WebPage_AdvanceToNextMisspelling = 1830
    , WebPage_ChangeSpellingToWord = 1831
    , WebPage_DidFinishCheckingText = 1832
    , WebPage_DidCancelCheckingText = 1833
#if USE(APPKIT)
    , WebPage_UppercaseWord = 1834
#endif
#if USE(APPKIT)
    , WebPage_LowercaseWord = 1835
#endif
#if USE(APPKIT)
    , WebPage_CapitalizeWord = 1836
#endif
#if PLATFORM(COCOA)
    , WebPage_SetSmartInsertDeleteEnabled = 1837
#endif
#if ENABLE(GEOLOCATION)
    , WebPage_DidReceiveGeolocationPermissionDecision = 1838
#endif
#if ENABLE(MEDIA_STREAM)
    , WebPage_UserMediaAccessWasGranted = 1839
    , WebPage_UserMediaAccessWasGrantedReply = 1840
#endif
#if ENABLE(MEDIA_STREAM)
    , WebPage_UserMediaAccessWasDenied = 1841
#endif
#if ENABLE(MEDIA_STREAM)
    , WebPage_CaptureDevicesChanged = 1842
#endif
    , WebPage_StopAllMediaPlayback = 1843
    , WebPage_SuspendAllMediaPlayback = 1844
    , WebPage_ResumeAllMediaPlayback = 1845
    , WebPage_DidReceiveNotificationPermissionDecision = 1846
    , WebPage_FreezeLayerTreeDueToSwipeAnimation = 1847
    , WebPage_UnfreezeLayerTreeDueToSwipeAnimation = 1848
    , WebPage_BeginPrinting = 1849
    , WebPage_EndPrinting = 1850
    , WebPage_ComputePagesForPrinting = 1851
#if PLATFORM(COCOA)
    , WebPage_DrawRectToImage = 1852
#endif
#if PLATFORM(COCOA)
    , WebPage_DrawPagesToPDF = 1853
#endif
#if (PLATFORM(COCOA) && PLATFORM(IOS_FAMILY))
    , WebPage_ComputePagesForPrintingAndDrawToPDF = 1854
#endif
#if PLATFORM(COCOA)
    , WebPage_DrawToPDF = 1855
#endif
#if PLATFORM(GTK)
    , WebPage_DrawPagesForPrinting = 1856
#endif
    , WebPage_SetMediaVolume = 1857
    , WebPage_SetMuted = 1858
    , WebPage_SetMayStartMediaWhenInWindow = 1859
    , WebPage_StopMediaCapture = 1860
#if ENABLE(MEDIA_SESSION)
    , WebPage_HandleMediaEvent = 1861
#endif
#if ENABLE(MEDIA_SESSION)
    , WebPage_SetVolumeOfMediaElement = 1862
#endif
    , WebPage_SetCanRunBeforeUnloadConfirmPanel = 1863
    , WebPage_SetCanRunModal = 1864
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    , WebPage_CancelComposition = 1865
#endif
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    , WebPage_DeleteSurrounding = 1866
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    , WebPage_CollapseSelectionInFrame = 1867
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    , WebPage_GetCenterForZoomGesture = 1868
#endif
#if PLATFORM(COCOA)
    , WebPage_SendComplexTextInputToPlugin = 1869
#endif
#if PLATFORM(COCOA)
    , WebPage_WindowAndViewFramesChanged = 1870
#endif
#if PLATFORM(COCOA)
    , WebPage_SetMainFrameIsScrollable = 1871
#endif
#if PLATFORM(COCOA)
    , WebPage_RegisterUIProcessAccessibilityTokens = 1872
#endif
#if PLATFORM(COCOA)
    , WebPage_GetStringSelectionForPasteboard = 1873
#endif
#if PLATFORM(COCOA)
    , WebPage_GetDataSelectionForPasteboard = 1874
#endif
#if PLATFORM(COCOA)
    , WebPage_ReadSelectionFromPasteboard = 1875
#endif
#if (PLATFORM(COCOA) && ENABLE(SERVICE_CONTROLS))
    , WebPage_ReplaceSelectionWithPasteboardData = 1876
#endif
#if PLATFORM(COCOA)
    , WebPage_ShouldDelayWindowOrderingEvent = 1877
#endif
#if PLATFORM(COCOA)
    , WebPage_AcceptsFirstMouse = 1878
#endif
#if PLATFORM(COCOA)
    , WebPage_SetTextAsync = 1879
#endif
#if PLATFORM(COCOA)
    , WebPage_InsertTextAsync = 1880
#endif
#if PLATFORM(COCOA)
    , WebPage_InsertDictatedTextAsync = 1881
#endif
#if PLATFORM(COCOA)
    , WebPage_HasMarkedText = 1882
    , WebPage_HasMarkedTextReply = 1883
#endif
#if PLATFORM(COCOA)
    , WebPage_GetMarkedRangeAsync = 1884
#endif
#if PLATFORM(COCOA)
    , WebPage_GetSelectedRangeAsync = 1885
#endif
#if PLATFORM(COCOA)
    , WebPage_CharacterIndexForPointAsync = 1886
#endif
#if PLATFORM(COCOA)
    , WebPage_FirstRectForCharacterRangeAsync = 1887
#endif
#if PLATFORM(COCOA)
    , WebPage_SetCompositionAsync = 1888
#endif
#if PLATFORM(COCOA)
    , WebPage_ConfirmCompositionAsync = 1889
#endif
#if PLATFORM(MAC)
    , WebPage_AttributedSubstringForCharacterRangeAsync = 1890
#endif
#if PLATFORM(MAC)
    , WebPage_FontAtSelection = 1891
#endif
    , WebPage_SetAlwaysShowsHorizontalScroller = 1892
    , WebPage_SetAlwaysShowsVerticalScroller = 1893
    , WebPage_SetMinimumSizeForAutoLayout = 1894
    , WebPage_SetSizeToContentAutoSizeMaximumSize = 1895
    , WebPage_SetAutoSizingShouldExpandToViewHeight = 1896
    , WebPage_SetViewportSizeForCSSViewportUnits = 1897
#if PLATFORM(COCOA)
    , WebPage_HandleAlternativeTextUIResult = 1898
#endif
#if PLATFORM(IOS_FAMILY)
    , WebPage_WillStartUserTriggeredZooming = 1899
#endif
    , WebPage_SetScrollPinningBehavior = 1900
    , WebPage_SetScrollbarOverlayStyle = 1901
    , WebPage_GetBytecodeProfile = 1902
    , WebPage_GetSamplingProfilerOutput = 1903
    , WebPage_TakeSnapshot = 1904
#if PLATFORM(MAC)
    , WebPage_PerformImmediateActionHitTestAtLocation = 1905
#endif
#if PLATFORM(MAC)
    , WebPage_ImmediateActionDidUpdate = 1906
#endif
#if PLATFORM(MAC)
    , WebPage_ImmediateActionDidCancel = 1907
#endif
#if PLATFORM(MAC)
    , WebPage_ImmediateActionDidComplete = 1908
#endif
#if PLATFORM(MAC)
    , WebPage_DataDetectorsDidPresentUI = 1909
#endif
#if PLATFORM(MAC)
    , WebPage_DataDetectorsDidChangeUI = 1910
#endif
#if PLATFORM(MAC)
    , WebPage_DataDetectorsDidHideUI = 1911
#endif
#if PLATFORM(MAC)
    , WebPage_HandleAcceptedCandidate = 1912
#endif
#if PLATFORM(MAC)
    , WebPage_SetUseSystemAppearance = 1913
#endif
#if PLATFORM(MAC)
    , WebPage_SetHeaderBannerHeightForTesting = 1914
#endif
#if PLATFORM(MAC)
    , WebPage_SetFooterBannerHeightForTesting = 1915
#endif
#if PLATFORM(MAC)
    , WebPage_DidEndMagnificationGesture = 1916
#endif
    , WebPage_EffectiveAppearanceDidChange = 1917
#if PLATFORM(GTK)
    , WebPage_ThemeDidChange = 1918
#endif
#if PLATFORM(COCOA)
    , WebPage_RequestActiveNowPlayingSessionInfo = 1919
#endif
    , WebPage_SetShouldDispatchFakeMouseMoveEvents = 1920
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    , WebPage_PlaybackTargetSelected = 1921
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    , WebPage_PlaybackTargetAvailabilityDidChange = 1922
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    , WebPage_SetShouldPlayToPlaybackTarget = 1923
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    , WebPage_PlaybackTargetPickerWasDismissed = 1924
#endif
#if ENABLE(POINTER_LOCK)
    , WebPage_DidAcquirePointerLock = 1925
#endif
#if ENABLE(POINTER_LOCK)
    , WebPage_DidNotAcquirePointerLock = 1926
#endif
#if ENABLE(POINTER_LOCK)
    , WebPage_DidLosePointerLock = 1927
#endif
    , WebPage_clearWheelEventTestMonitor = 1928
    , WebPage_SetShouldScaleViewToFitDocument = 1929
#if ENABLE(VIDEO) && USE(GSTREAMER)
    , WebPage_DidEndRequestInstallMissingMediaPlugins = 1930
#endif
    , WebPage_SetUserInterfaceLayoutDirection = 1931
    , WebPage_DidGetLoadDecisionForIcon = 1932
    , WebPage_SetUseIconLoadingClient = 1933
#if ENABLE(GAMEPAD)
    , WebPage_GamepadActivity = 1934
#endif
    , WebPage_FrameBecameRemote = 1935
    , WebPage_RegisterURLSchemeHandler = 1936
    , WebPage_URLSchemeTaskDidPerformRedirection = 1937
    , WebPage_URLSchemeTaskDidReceiveResponse = 1938
    , WebPage_URLSchemeTaskDidReceiveData = 1939
    , WebPage_URLSchemeTaskDidComplete = 1940
    , WebPage_SetIsSuspended = 1941
#if ENABLE(ATTACHMENT_ELEMENT)
    , WebPage_InsertAttachment = 1942
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    , WebPage_UpdateAttachmentAttributes = 1943
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    , WebPage_UpdateAttachmentIcon = 1944
#endif
#if ENABLE(APPLICATION_MANIFEST)
    , WebPage_GetApplicationManifest = 1945
#endif
    , WebPage_SetDefersLoading = 1946
    , WebPage_UpdateCurrentModifierState = 1947
    , WebPage_SimulateDeviceOrientationChange = 1948
#if ENABLE(SPEECH_SYNTHESIS)
    , WebPage_SpeakingErrorOccurred = 1949
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    , WebPage_BoundaryEventOccurred = 1950
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    , WebPage_VoicesDidChange = 1951
#endif
    , WebPage_SetCanShowPlaceholder = 1952
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , WebPage_WasLoadedWithDataTransferFromPrevalentResource = 1953
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , WebPage_ClearLoadedThirdPartyDomains = 1954
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    , WebPage_LoadedThirdPartyDomains = 1955
    , WebPage_LoadedThirdPartyDomainsReply = 1956
#endif
#if USE(SYSTEM_PREVIEW)
    , WebPage_SystemPreviewActionTriggered = 1957
#endif
#if PLATFORM(GTK) || PLATFORM(WPE)
    , WebPage_SendMessageToWebExtension = 1958
#endif
#if PLATFORM(GTK) || PLATFORM(WPE)
    , WebPage_SendMessageToWebExtensionWithReply = 1959
    , WebPage_SendMessageToWebExtensionWithReplyReply = 1960
#endif
    , WebPage_StartTextManipulations = 1961
    , WebPage_StartTextManipulationsReply = 1962
    , WebPage_CompleteTextManipulation = 1963
    , WebPage_CompleteTextManipulationReply = 1964
    , WebPage_SetOverriddenMediaType = 1965
    , WebPage_GetProcessDisplayName = 1966
    , WebPage_GetProcessDisplayNameReply = 1967
    , WebPage_UpdateCORSDisablingPatterns = 1968
    , WebPage_SetShouldFireEvents = 1969
    , WebPage_SetNeedsDOMWindowResizeEvent = 1970
    , WebPage_SetHasResourceLoadClient = 1971
    , StorageAreaMap_DidSetItem = 1972
    , StorageAreaMap_DidRemoveItem = 1973
    , StorageAreaMap_DidClear = 1974
    , StorageAreaMap_DispatchStorageEvent = 1975
    , StorageAreaMap_ClearCache = 1976
#if PLATFORM(MAC)
    , ViewGestureController_DidCollectGeometryForMagnificationGesture = 1977
#endif
#if PLATFORM(MAC)
    , ViewGestureController_DidCollectGeometryForSmartMagnificationGesture = 1978
#endif
#if !PLATFORM(IOS_FAMILY)
    , ViewGestureController_DidHitRenderTreeSizeThreshold = 1979
#endif
#if PLATFORM(COCOA)
    , ViewGestureGeometryCollector_CollectGeometryForSmartMagnificationGesture = 1980
#endif
#if PLATFORM(MAC)
    , ViewGestureGeometryCollector_CollectGeometryForMagnificationGesture = 1981
#endif
#if !PLATFORM(IOS_FAMILY)
    , ViewGestureGeometryCollector_SetRenderTreeSizeNotificationThreshold = 1982
#endif
    , WrappedAsyncMessageForTesting = 1983
    , SyncMessageReply = 1984
    , InitializeConnection = 1985
    , LegacySessionState = 1986
};

ReceiverName receiverName(MessageName);
const char* description(MessageName);
bool isValidMessageName(MessageName);

} // namespace IPC

namespace PurCWTF {

template<>
class HasCustomIsValidEnum<IPC::MessageName> : public std::true_type { };
template<typename E, typename T, std::enable_if_t<std::is_same_v<E, IPC::MessageName>>* = nullptr>
bool isValidEnum(T messageName)
{
    static_assert(sizeof(T) == sizeof(E), "isValidEnum<IPC::MessageName> should only be called with 16-bit types");
    return IPC::isValidMessageName(static_cast<E>(messageName));
};

} // namespace PurCWTF
