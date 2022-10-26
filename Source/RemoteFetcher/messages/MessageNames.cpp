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

#include "config.h"
#include "MessageNames.h"

namespace IPC {

const char* description(MessageName name)
{
    switch (name) {
    case MessageName::GPUConnectionToWebProcess_CreateRenderingBackend:
        return "GPUConnectionToWebProcess::CreateRenderingBackend";
    case MessageName::GPUConnectionToWebProcess_ReleaseRenderingBackend:
        return "GPUConnectionToWebProcess::ReleaseRenderingBackend";
    case MessageName::GPUConnectionToWebProcess_ClearNowPlayingInfo:
        return "GPUConnectionToWebProcess::ClearNowPlayingInfo";
    case MessageName::GPUConnectionToWebProcess_SetNowPlayingInfo:
        return "GPUConnectionToWebProcess::SetNowPlayingInfo";
#if USE(AUDIO_SESSION)
    case MessageName::GPUConnectionToWebProcess_EnsureAudioSession:
        return "GPUConnectionToWebProcess::EnsureAudioSession";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::GPUConnectionToWebProcess_EnsureMediaSessionHelper:
        return "GPUConnectionToWebProcess::EnsureMediaSessionHelper";
#endif
    case MessageName::GPUProcess_InitializeGPUProcess:
        return "GPUProcess::InitializeGPUProcess";
    case MessageName::GPUProcess_CreateGPUConnectionToWebProcess:
        return "GPUProcess::CreateGPUConnectionToWebProcess";
    case MessageName::GPUProcess_CreateGPUConnectionToWebProcessReply:
        return "GPUProcess::CreateGPUConnectionToWebProcessReply";
    case MessageName::GPUProcess_ProcessDidTransitionToForeground:
        return "GPUProcess::ProcessDidTransitionToForeground";
    case MessageName::GPUProcess_ProcessDidTransitionToBackground:
        return "GPUProcess::ProcessDidTransitionToBackground";
    case MessageName::GPUProcess_AddSession:
        return "GPUProcess::AddSession";
    case MessageName::GPUProcess_RemoveSession:
        return "GPUProcess::RemoveSession";
#if ENABLE(MEDIA_STREAM)
    case MessageName::GPUProcess_SetMockCaptureDevicesEnabled:
        return "GPUProcess::SetMockCaptureDevicesEnabled";
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::GPUProcess_SetOrientationForMediaCapture:
        return "GPUProcess::SetOrientationForMediaCapture";
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::GPUProcess_UpdateCaptureAccess:
        return "GPUProcess::UpdateCaptureAccess";
    case MessageName::GPUProcess_UpdateCaptureAccessReply:
        return "GPUProcess::UpdateCaptureAccessReply";
#endif
    case MessageName::RemoteRenderingBackendProxy_CreateImageBuffer:
        return "RemoteRenderingBackendProxy::CreateImageBuffer";
    case MessageName::RemoteRenderingBackendProxy_ReleaseImageBuffer:
        return "RemoteRenderingBackendProxy::ReleaseImageBuffer";
    case MessageName::RemoteRenderingBackendProxy_FlushImageBufferDrawingContext:
        return "RemoteRenderingBackendProxy::FlushImageBufferDrawingContext";
    case MessageName::RemoteRenderingBackendProxy_FlushImageBufferDrawingContextAndCommit:
        return "RemoteRenderingBackendProxy::FlushImageBufferDrawingContextAndCommit";
    case MessageName::RemoteRenderingBackendProxy_GetImageData:
        return "RemoteRenderingBackendProxy::GetImageData";
    case MessageName::RemoteAudioDestinationManager_CreateAudioDestination:
        return "RemoteAudioDestinationManager::CreateAudioDestination";
    case MessageName::RemoteAudioDestinationManager_DeleteAudioDestination:
        return "RemoteAudioDestinationManager::DeleteAudioDestination";
    case MessageName::RemoteAudioDestinationManager_DeleteAudioDestinationReply:
        return "RemoteAudioDestinationManager::DeleteAudioDestinationReply";
    case MessageName::RemoteAudioDestinationManager_StartAudioDestination:
        return "RemoteAudioDestinationManager::StartAudioDestination";
    case MessageName::RemoteAudioDestinationManager_StopAudioDestination:
        return "RemoteAudioDestinationManager::StopAudioDestination";
    case MessageName::RemoteAudioSessionProxy_SetCategory:
        return "RemoteAudioSessionProxy::SetCategory";
    case MessageName::RemoteAudioSessionProxy_SetPreferredBufferSize:
        return "RemoteAudioSessionProxy::SetPreferredBufferSize";
    case MessageName::RemoteAudioSessionProxy_TryToSetActive:
        return "RemoteAudioSessionProxy::TryToSetActive";
    case MessageName::RemoteCDMFactoryProxy_CreateCDM:
        return "RemoteCDMFactoryProxy::CreateCDM";
    case MessageName::RemoteCDMFactoryProxy_SupportsKeySystem:
        return "RemoteCDMFactoryProxy::SupportsKeySystem";
    case MessageName::RemoteCDMInstanceProxy_CreateSession:
        return "RemoteCDMInstanceProxy::CreateSession";
    case MessageName::RemoteCDMInstanceProxy_InitializeWithConfiguration:
        return "RemoteCDMInstanceProxy::InitializeWithConfiguration";
    case MessageName::RemoteCDMInstanceProxy_InitializeWithConfigurationReply:
        return "RemoteCDMInstanceProxy::InitializeWithConfigurationReply";
    case MessageName::RemoteCDMInstanceProxy_SetServerCertificate:
        return "RemoteCDMInstanceProxy::SetServerCertificate";
    case MessageName::RemoteCDMInstanceProxy_SetServerCertificateReply:
        return "RemoteCDMInstanceProxy::SetServerCertificateReply";
    case MessageName::RemoteCDMInstanceProxy_SetStorageDirectory:
        return "RemoteCDMInstanceProxy::SetStorageDirectory";
    case MessageName::RemoteCDMInstanceSessionProxy_RequestLicense:
        return "RemoteCDMInstanceSessionProxy::RequestLicense";
    case MessageName::RemoteCDMInstanceSessionProxy_RequestLicenseReply:
        return "RemoteCDMInstanceSessionProxy::RequestLicenseReply";
    case MessageName::RemoteCDMInstanceSessionProxy_UpdateLicense:
        return "RemoteCDMInstanceSessionProxy::UpdateLicense";
    case MessageName::RemoteCDMInstanceSessionProxy_UpdateLicenseReply:
        return "RemoteCDMInstanceSessionProxy::UpdateLicenseReply";
    case MessageName::RemoteCDMInstanceSessionProxy_LoadSession:
        return "RemoteCDMInstanceSessionProxy::LoadSession";
    case MessageName::RemoteCDMInstanceSessionProxy_LoadSessionReply:
        return "RemoteCDMInstanceSessionProxy::LoadSessionReply";
    case MessageName::RemoteCDMInstanceSessionProxy_CloseSession:
        return "RemoteCDMInstanceSessionProxy::CloseSession";
    case MessageName::RemoteCDMInstanceSessionProxy_CloseSessionReply:
        return "RemoteCDMInstanceSessionProxy::CloseSessionReply";
    case MessageName::RemoteCDMInstanceSessionProxy_RemoveSessionData:
        return "RemoteCDMInstanceSessionProxy::RemoveSessionData";
    case MessageName::RemoteCDMInstanceSessionProxy_RemoveSessionDataReply:
        return "RemoteCDMInstanceSessionProxy::RemoveSessionDataReply";
    case MessageName::RemoteCDMInstanceSessionProxy_StoreRecordOfKeyUsage:
        return "RemoteCDMInstanceSessionProxy::StoreRecordOfKeyUsage";
    case MessageName::RemoteCDMProxy_GetSupportedConfiguration:
        return "RemoteCDMProxy::GetSupportedConfiguration";
    case MessageName::RemoteCDMProxy_GetSupportedConfigurationReply:
        return "RemoteCDMProxy::GetSupportedConfigurationReply";
    case MessageName::RemoteCDMProxy_CreateInstance:
        return "RemoteCDMProxy::CreateInstance";
    case MessageName::RemoteCDMProxy_LoadAndInitialize:
        return "RemoteCDMProxy::LoadAndInitialize";
    case MessageName::RemoteLegacyCDMFactoryProxy_CreateCDM:
        return "RemoteLegacyCDMFactoryProxy::CreateCDM";
    case MessageName::RemoteLegacyCDMFactoryProxy_SupportsKeySystem:
        return "RemoteLegacyCDMFactoryProxy::SupportsKeySystem";
    case MessageName::RemoteLegacyCDMProxy_SupportsMIMEType:
        return "RemoteLegacyCDMProxy::SupportsMIMEType";
    case MessageName::RemoteLegacyCDMProxy_CreateSession:
        return "RemoteLegacyCDMProxy::CreateSession";
    case MessageName::RemoteLegacyCDMProxy_SetPlayerId:
        return "RemoteLegacyCDMProxy::SetPlayerId";
    case MessageName::RemoteLegacyCDMSessionProxy_GenerateKeyRequest:
        return "RemoteLegacyCDMSessionProxy::GenerateKeyRequest";
    case MessageName::RemoteLegacyCDMSessionProxy_ReleaseKeys:
        return "RemoteLegacyCDMSessionProxy::ReleaseKeys";
    case MessageName::RemoteLegacyCDMSessionProxy_Update:
        return "RemoteLegacyCDMSessionProxy::Update";
    case MessageName::RemoteLegacyCDMSessionProxy_CachedKeyForKeyID:
        return "RemoteLegacyCDMSessionProxy::CachedKeyForKeyID";
    case MessageName::RemoteMediaPlayerManagerProxy_CreateMediaPlayer:
        return "RemoteMediaPlayerManagerProxy::CreateMediaPlayer";
    case MessageName::RemoteMediaPlayerManagerProxy_CreateMediaPlayerReply:
        return "RemoteMediaPlayerManagerProxy::CreateMediaPlayerReply";
    case MessageName::RemoteMediaPlayerManagerProxy_DeleteMediaPlayer:
        return "RemoteMediaPlayerManagerProxy::DeleteMediaPlayer";
    case MessageName::RemoteMediaPlayerManagerProxy_GetSupportedTypes:
        return "RemoteMediaPlayerManagerProxy::GetSupportedTypes";
    case MessageName::RemoteMediaPlayerManagerProxy_SupportsTypeAndCodecs:
        return "RemoteMediaPlayerManagerProxy::SupportsTypeAndCodecs";
    case MessageName::RemoteMediaPlayerManagerProxy_CanDecodeExtendedType:
        return "RemoteMediaPlayerManagerProxy::CanDecodeExtendedType";
    case MessageName::RemoteMediaPlayerManagerProxy_OriginsInMediaCache:
        return "RemoteMediaPlayerManagerProxy::OriginsInMediaCache";
    case MessageName::RemoteMediaPlayerManagerProxy_ClearMediaCache:
        return "RemoteMediaPlayerManagerProxy::ClearMediaCache";
    case MessageName::RemoteMediaPlayerManagerProxy_ClearMediaCacheForOrigins:
        return "RemoteMediaPlayerManagerProxy::ClearMediaCacheForOrigins";
    case MessageName::RemoteMediaPlayerManagerProxy_SupportsKeySystem:
        return "RemoteMediaPlayerManagerProxy::SupportsKeySystem";
    case MessageName::RemoteMediaPlayerProxy_PrepareForPlayback:
        return "RemoteMediaPlayerProxy::PrepareForPlayback";
    case MessageName::RemoteMediaPlayerProxy_PrepareForPlaybackReply:
        return "RemoteMediaPlayerProxy::PrepareForPlaybackReply";
    case MessageName::RemoteMediaPlayerProxy_Load:
        return "RemoteMediaPlayerProxy::Load";
    case MessageName::RemoteMediaPlayerProxy_LoadReply:
        return "RemoteMediaPlayerProxy::LoadReply";
    case MessageName::RemoteMediaPlayerProxy_CancelLoad:
        return "RemoteMediaPlayerProxy::CancelLoad";
    case MessageName::RemoteMediaPlayerProxy_PrepareToPlay:
        return "RemoteMediaPlayerProxy::PrepareToPlay";
    case MessageName::RemoteMediaPlayerProxy_Play:
        return "RemoteMediaPlayerProxy::Play";
    case MessageName::RemoteMediaPlayerProxy_Pause:
        return "RemoteMediaPlayerProxy::Pause";
    case MessageName::RemoteMediaPlayerProxy_SetVolume:
        return "RemoteMediaPlayerProxy::SetVolume";
    case MessageName::RemoteMediaPlayerProxy_SetMuted:
        return "RemoteMediaPlayerProxy::SetMuted";
    case MessageName::RemoteMediaPlayerProxy_Seek:
        return "RemoteMediaPlayerProxy::Seek";
    case MessageName::RemoteMediaPlayerProxy_SeekWithTolerance:
        return "RemoteMediaPlayerProxy::SeekWithTolerance";
    case MessageName::RemoteMediaPlayerProxy_SetPreload:
        return "RemoteMediaPlayerProxy::SetPreload";
    case MessageName::RemoteMediaPlayerProxy_SetPrivateBrowsingMode:
        return "RemoteMediaPlayerProxy::SetPrivateBrowsingMode";
    case MessageName::RemoteMediaPlayerProxy_SetPreservesPitch:
        return "RemoteMediaPlayerProxy::SetPreservesPitch";
    case MessageName::RemoteMediaPlayerProxy_PrepareForRendering:
        return "RemoteMediaPlayerProxy::PrepareForRendering";
    case MessageName::RemoteMediaPlayerProxy_SetVisible:
        return "RemoteMediaPlayerProxy::SetVisible";
    case MessageName::RemoteMediaPlayerProxy_SetShouldMaintainAspectRatio:
        return "RemoteMediaPlayerProxy::SetShouldMaintainAspectRatio";
#if ENABLE(VIDEO_PRESENTATION_MODE)
    case MessageName::RemoteMediaPlayerProxy_SetVideoFullscreenGravity:
        return "RemoteMediaPlayerProxy::SetVideoFullscreenGravity";
#endif
    case MessageName::RemoteMediaPlayerProxy_AcceleratedRenderingStateChanged:
        return "RemoteMediaPlayerProxy::AcceleratedRenderingStateChanged";
    case MessageName::RemoteMediaPlayerProxy_SetShouldDisableSleep:
        return "RemoteMediaPlayerProxy::SetShouldDisableSleep";
    case MessageName::RemoteMediaPlayerProxy_SetRate:
        return "RemoteMediaPlayerProxy::SetRate";
#if ENABLE(VIDEO_PRESENTATION_MODE)
    case MessageName::RemoteMediaPlayerProxy_UpdateVideoFullscreenInlineImage:
        return "RemoteMediaPlayerProxy::UpdateVideoFullscreenInlineImage";
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    case MessageName::RemoteMediaPlayerProxy_SetVideoFullscreenMode:
        return "RemoteMediaPlayerProxy::SetVideoFullscreenMode";
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    case MessageName::RemoteMediaPlayerProxy_VideoFullscreenStandbyChanged:
        return "RemoteMediaPlayerProxy::VideoFullscreenStandbyChanged";
#endif
    case MessageName::RemoteMediaPlayerProxy_SetBufferingPolicy:
        return "RemoteMediaPlayerProxy::SetBufferingPolicy";
    case MessageName::RemoteMediaPlayerProxy_AudioTrackSetEnabled:
        return "RemoteMediaPlayerProxy::AudioTrackSetEnabled";
    case MessageName::RemoteMediaPlayerProxy_VideoTrackSetSelected:
        return "RemoteMediaPlayerProxy::VideoTrackSetSelected";
    case MessageName::RemoteMediaPlayerProxy_TextTrackSetMode:
        return "RemoteMediaPlayerProxy::TextTrackSetMode";
#if PLATFORM(COCOA)
    case MessageName::RemoteMediaPlayerProxy_SetVideoInlineSizeFenced:
        return "RemoteMediaPlayerProxy::SetVideoInlineSizeFenced";
#endif
#if (PLATFORM(COCOA) && ENABLE(VIDEO_PRESENTATION_MODE))
    case MessageName::RemoteMediaPlayerProxy_SetVideoFullscreenFrameFenced:
        return "RemoteMediaPlayerProxy::SetVideoFullscreenFrameFenced";
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    case MessageName::RemoteMediaPlayerProxy_EnterFullscreen:
        return "RemoteMediaPlayerProxy::EnterFullscreen";
    case MessageName::RemoteMediaPlayerProxy_EnterFullscreenReply:
        return "RemoteMediaPlayerProxy::EnterFullscreenReply";
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    case MessageName::RemoteMediaPlayerProxy_ExitFullscreen:
        return "RemoteMediaPlayerProxy::ExitFullscreen";
    case MessageName::RemoteMediaPlayerProxy_ExitFullscreenReply:
        return "RemoteMediaPlayerProxy::ExitFullscreenReply";
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    case MessageName::RemoteMediaPlayerProxy_SetWirelessVideoPlaybackDisabled:
        return "RemoteMediaPlayerProxy::SetWirelessVideoPlaybackDisabled";
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    case MessageName::RemoteMediaPlayerProxy_SetShouldPlayToPlaybackTarget:
        return "RemoteMediaPlayerProxy::SetShouldPlayToPlaybackTarget";
#endif
#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    case MessageName::RemoteMediaPlayerProxy_SetLegacyCDMSession:
        return "RemoteMediaPlayerProxy::SetLegacyCDMSession";
#endif
#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    case MessageName::RemoteMediaPlayerProxy_KeyAdded:
        return "RemoteMediaPlayerProxy::KeyAdded";
#endif
#if ENABLE(ENCRYPTED_MEDIA)
    case MessageName::RemoteMediaPlayerProxy_CdmInstanceAttached:
        return "RemoteMediaPlayerProxy::CdmInstanceAttached";
#endif
#if ENABLE(ENCRYPTED_MEDIA)
    case MessageName::RemoteMediaPlayerProxy_CdmInstanceDetached:
        return "RemoteMediaPlayerProxy::CdmInstanceDetached";
#endif
#if ENABLE(ENCRYPTED_MEDIA)
    case MessageName::RemoteMediaPlayerProxy_AttemptToDecryptWithInstance:
        return "RemoteMediaPlayerProxy::AttemptToDecryptWithInstance";
#endif
#if ENABLE(LEGACY_ENCRYPTED_MEDIA) && ENABLE(ENCRYPTED_MEDIA)
    case MessageName::RemoteMediaPlayerProxy_SetShouldContinueAfterKeyNeeded:
        return "RemoteMediaPlayerProxy::SetShouldContinueAfterKeyNeeded";
#endif
    case MessageName::RemoteMediaPlayerProxy_BeginSimulatedHDCPError:
        return "RemoteMediaPlayerProxy::BeginSimulatedHDCPError";
    case MessageName::RemoteMediaPlayerProxy_EndSimulatedHDCPError:
        return "RemoteMediaPlayerProxy::EndSimulatedHDCPError";
    case MessageName::RemoteMediaPlayerProxy_NotifyActiveSourceBuffersChanged:
        return "RemoteMediaPlayerProxy::NotifyActiveSourceBuffersChanged";
    case MessageName::RemoteMediaPlayerProxy_ApplicationWillResignActive:
        return "RemoteMediaPlayerProxy::ApplicationWillResignActive";
    case MessageName::RemoteMediaPlayerProxy_ApplicationDidBecomeActive:
        return "RemoteMediaPlayerProxy::ApplicationDidBecomeActive";
    case MessageName::RemoteMediaPlayerProxy_NotifyTrackModeChanged:
        return "RemoteMediaPlayerProxy::NotifyTrackModeChanged";
    case MessageName::RemoteMediaPlayerProxy_TracksChanged:
        return "RemoteMediaPlayerProxy::TracksChanged";
    case MessageName::RemoteMediaPlayerProxy_SyncTextTrackBounds:
        return "RemoteMediaPlayerProxy::SyncTextTrackBounds";
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    case MessageName::RemoteMediaPlayerProxy_SetWirelessPlaybackTarget:
        return "RemoteMediaPlayerProxy::SetWirelessPlaybackTarget";
#endif
    case MessageName::RemoteMediaPlayerProxy_PerformTaskAtMediaTime:
        return "RemoteMediaPlayerProxy::PerformTaskAtMediaTime";
    case MessageName::RemoteMediaPlayerProxy_PerformTaskAtMediaTimeReply:
        return "RemoteMediaPlayerProxy::PerformTaskAtMediaTimeReply";
    case MessageName::RemoteMediaPlayerProxy_WouldTaintOrigin:
        return "RemoteMediaPlayerProxy::WouldTaintOrigin";
#if PLATFORM(IOS_FAMILY)
    case MessageName::RemoteMediaPlayerProxy_ErrorLog:
        return "RemoteMediaPlayerProxy::ErrorLog";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::RemoteMediaPlayerProxy_AccessLog:
        return "RemoteMediaPlayerProxy::AccessLog";
#endif
    case MessageName::RemoteMediaResourceManager_ResponseReceived:
        return "RemoteMediaResourceManager::ResponseReceived";
    case MessageName::RemoteMediaResourceManager_ResponseReceivedReply:
        return "RemoteMediaResourceManager::ResponseReceivedReply";
    case MessageName::RemoteMediaResourceManager_RedirectReceived:
        return "RemoteMediaResourceManager::RedirectReceived";
    case MessageName::RemoteMediaResourceManager_RedirectReceivedReply:
        return "RemoteMediaResourceManager::RedirectReceivedReply";
    case MessageName::RemoteMediaResourceManager_DataSent:
        return "RemoteMediaResourceManager::DataSent";
    case MessageName::RemoteMediaResourceManager_DataReceived:
        return "RemoteMediaResourceManager::DataReceived";
    case MessageName::RemoteMediaResourceManager_AccessControlCheckFailed:
        return "RemoteMediaResourceManager::AccessControlCheckFailed";
    case MessageName::RemoteMediaResourceManager_LoadFailed:
        return "RemoteMediaResourceManager::LoadFailed";
    case MessageName::RemoteMediaResourceManager_LoadFinished:
        return "RemoteMediaResourceManager::LoadFinished";
    case MessageName::LibWebRTCCodecsProxy_CreateH264Decoder:
        return "LibWebRTCCodecsProxy::CreateH264Decoder";
    case MessageName::LibWebRTCCodecsProxy_CreateH265Decoder:
        return "LibWebRTCCodecsProxy::CreateH265Decoder";
    case MessageName::LibWebRTCCodecsProxy_ReleaseDecoder:
        return "LibWebRTCCodecsProxy::ReleaseDecoder";
    case MessageName::LibWebRTCCodecsProxy_DecodeFrame:
        return "LibWebRTCCodecsProxy::DecodeFrame";
    case MessageName::LibWebRTCCodecsProxy_CreateEncoder:
        return "LibWebRTCCodecsProxy::CreateEncoder";
    case MessageName::LibWebRTCCodecsProxy_ReleaseEncoder:
        return "LibWebRTCCodecsProxy::ReleaseEncoder";
    case MessageName::LibWebRTCCodecsProxy_InitializeEncoder:
        return "LibWebRTCCodecsProxy::InitializeEncoder";
    case MessageName::LibWebRTCCodecsProxy_EncodeFrame:
        return "LibWebRTCCodecsProxy::EncodeFrame";
    case MessageName::LibWebRTCCodecsProxy_SetEncodeRates:
        return "LibWebRTCCodecsProxy::SetEncodeRates";
    case MessageName::RemoteAudioMediaStreamTrackRenderer_Start:
        return "RemoteAudioMediaStreamTrackRenderer::Start";
    case MessageName::RemoteAudioMediaStreamTrackRenderer_Stop:
        return "RemoteAudioMediaStreamTrackRenderer::Stop";
    case MessageName::RemoteAudioMediaStreamTrackRenderer_Clear:
        return "RemoteAudioMediaStreamTrackRenderer::Clear";
    case MessageName::RemoteAudioMediaStreamTrackRenderer_SetVolume:
        return "RemoteAudioMediaStreamTrackRenderer::SetVolume";
    case MessageName::RemoteAudioMediaStreamTrackRenderer_AudioSamplesStorageChanged:
        return "RemoteAudioMediaStreamTrackRenderer::AudioSamplesStorageChanged";
    case MessageName::RemoteAudioMediaStreamTrackRenderer_AudioSamplesAvailable:
        return "RemoteAudioMediaStreamTrackRenderer::AudioSamplesAvailable";
    case MessageName::RemoteAudioMediaStreamTrackRendererManager_CreateRenderer:
        return "RemoteAudioMediaStreamTrackRendererManager::CreateRenderer";
    case MessageName::RemoteAudioMediaStreamTrackRendererManager_ReleaseRenderer:
        return "RemoteAudioMediaStreamTrackRendererManager::ReleaseRenderer";
    case MessageName::RemoteMediaRecorder_AudioSamplesStorageChanged:
        return "RemoteMediaRecorder::AudioSamplesStorageChanged";
    case MessageName::RemoteMediaRecorder_AudioSamplesAvailable:
        return "RemoteMediaRecorder::AudioSamplesAvailable";
    case MessageName::RemoteMediaRecorder_VideoSampleAvailable:
        return "RemoteMediaRecorder::VideoSampleAvailable";
    case MessageName::RemoteMediaRecorder_FetchData:
        return "RemoteMediaRecorder::FetchData";
    case MessageName::RemoteMediaRecorder_FetchDataReply:
        return "RemoteMediaRecorder::FetchDataReply";
    case MessageName::RemoteMediaRecorder_StopRecording:
        return "RemoteMediaRecorder::StopRecording";
    case MessageName::RemoteMediaRecorderManager_CreateRecorder:
        return "RemoteMediaRecorderManager::CreateRecorder";
    case MessageName::RemoteMediaRecorderManager_CreateRecorderReply:
        return "RemoteMediaRecorderManager::CreateRecorderReply";
    case MessageName::RemoteMediaRecorderManager_ReleaseRecorder:
        return "RemoteMediaRecorderManager::ReleaseRecorder";
    case MessageName::RemoteSampleBufferDisplayLayer_UpdateDisplayMode:
        return "RemoteSampleBufferDisplayLayer::UpdateDisplayMode";
    case MessageName::RemoteSampleBufferDisplayLayer_UpdateAffineTransform:
        return "RemoteSampleBufferDisplayLayer::UpdateAffineTransform";
    case MessageName::RemoteSampleBufferDisplayLayer_UpdateBoundsAndPosition:
        return "RemoteSampleBufferDisplayLayer::UpdateBoundsAndPosition";
    case MessageName::RemoteSampleBufferDisplayLayer_Flush:
        return "RemoteSampleBufferDisplayLayer::Flush";
    case MessageName::RemoteSampleBufferDisplayLayer_FlushAndRemoveImage:
        return "RemoteSampleBufferDisplayLayer::FlushAndRemoveImage";
    case MessageName::RemoteSampleBufferDisplayLayer_EnqueueSample:
        return "RemoteSampleBufferDisplayLayer::EnqueueSample";
    case MessageName::RemoteSampleBufferDisplayLayer_ClearEnqueuedSamples:
        return "RemoteSampleBufferDisplayLayer::ClearEnqueuedSamples";
    case MessageName::RemoteSampleBufferDisplayLayerManager_CreateLayer:
        return "RemoteSampleBufferDisplayLayerManager::CreateLayer";
    case MessageName::RemoteSampleBufferDisplayLayerManager_CreateLayerReply:
        return "RemoteSampleBufferDisplayLayerManager::CreateLayerReply";
    case MessageName::RemoteSampleBufferDisplayLayerManager_ReleaseLayer:
        return "RemoteSampleBufferDisplayLayerManager::ReleaseLayer";
    case MessageName::WebCookieManager_GetHostnamesWithCookies:
        return "WebCookieManager::GetHostnamesWithCookies";
    case MessageName::WebCookieManager_GetHostnamesWithCookiesReply:
        return "WebCookieManager::GetHostnamesWithCookiesReply";
    case MessageName::WebCookieManager_DeleteCookiesForHostnames:
        return "WebCookieManager::DeleteCookiesForHostnames";
    case MessageName::WebCookieManager_DeleteAllCookies:
        return "WebCookieManager::DeleteAllCookies";
    case MessageName::WebCookieManager_SetCookie:
        return "WebCookieManager::SetCookie";
    case MessageName::WebCookieManager_SetCookieReply:
        return "WebCookieManager::SetCookieReply";
    case MessageName::WebCookieManager_SetCookies:
        return "WebCookieManager::SetCookies";
    case MessageName::WebCookieManager_SetCookiesReply:
        return "WebCookieManager::SetCookiesReply";
    case MessageName::WebCookieManager_GetAllCookies:
        return "WebCookieManager::GetAllCookies";
    case MessageName::WebCookieManager_GetAllCookiesReply:
        return "WebCookieManager::GetAllCookiesReply";
    case MessageName::WebCookieManager_GetCookies:
        return "WebCookieManager::GetCookies";
    case MessageName::WebCookieManager_GetCookiesReply:
        return "WebCookieManager::GetCookiesReply";
    case MessageName::WebCookieManager_DeleteCookie:
        return "WebCookieManager::DeleteCookie";
    case MessageName::WebCookieManager_DeleteCookieReply:
        return "WebCookieManager::DeleteCookieReply";
    case MessageName::WebCookieManager_DeleteAllCookiesModifiedSince:
        return "WebCookieManager::DeleteAllCookiesModifiedSince";
    case MessageName::WebCookieManager_DeleteAllCookiesModifiedSinceReply:
        return "WebCookieManager::DeleteAllCookiesModifiedSinceReply";
    case MessageName::WebCookieManager_SetHTTPCookieAcceptPolicy:
        return "WebCookieManager::SetHTTPCookieAcceptPolicy";
    case MessageName::WebCookieManager_SetHTTPCookieAcceptPolicyReply:
        return "WebCookieManager::SetHTTPCookieAcceptPolicyReply";
    case MessageName::WebCookieManager_GetHTTPCookieAcceptPolicy:
        return "WebCookieManager::GetHTTPCookieAcceptPolicy";
    case MessageName::WebCookieManager_GetHTTPCookieAcceptPolicyReply:
        return "WebCookieManager::GetHTTPCookieAcceptPolicyReply";
    case MessageName::WebCookieManager_StartObservingCookieChanges:
        return "WebCookieManager::StartObservingCookieChanges";
    case MessageName::WebCookieManager_StopObservingCookieChanges:
        return "WebCookieManager::StopObservingCookieChanges";
#if USE(SOUP)
    case MessageName::WebCookieManager_SetCookiePersistentStorage:
        return "WebCookieManager::SetCookiePersistentStorage";
#endif
    case MessageName::WebIDBServer_DeleteDatabase:
        return "WebIDBServer::DeleteDatabase";
    case MessageName::WebIDBServer_OpenDatabase:
        return "WebIDBServer::OpenDatabase";
    case MessageName::WebIDBServer_AbortTransaction:
        return "WebIDBServer::AbortTransaction";
    case MessageName::WebIDBServer_CommitTransaction:
        return "WebIDBServer::CommitTransaction";
    case MessageName::WebIDBServer_DidFinishHandlingVersionChangeTransaction:
        return "WebIDBServer::DidFinishHandlingVersionChangeTransaction";
    case MessageName::WebIDBServer_CreateObjectStore:
        return "WebIDBServer::CreateObjectStore";
    case MessageName::WebIDBServer_DeleteObjectStore:
        return "WebIDBServer::DeleteObjectStore";
    case MessageName::WebIDBServer_RenameObjectStore:
        return "WebIDBServer::RenameObjectStore";
    case MessageName::WebIDBServer_ClearObjectStore:
        return "WebIDBServer::ClearObjectStore";
    case MessageName::WebIDBServer_CreateIndex:
        return "WebIDBServer::CreateIndex";
    case MessageName::WebIDBServer_DeleteIndex:
        return "WebIDBServer::DeleteIndex";
    case MessageName::WebIDBServer_RenameIndex:
        return "WebIDBServer::RenameIndex";
    case MessageName::WebIDBServer_PutOrAdd:
        return "WebIDBServer::PutOrAdd";
    case MessageName::WebIDBServer_GetRecord:
        return "WebIDBServer::GetRecord";
    case MessageName::WebIDBServer_GetAllRecords:
        return "WebIDBServer::GetAllRecords";
    case MessageName::WebIDBServer_GetCount:
        return "WebIDBServer::GetCount";
    case MessageName::WebIDBServer_DeleteRecord:
        return "WebIDBServer::DeleteRecord";
    case MessageName::WebIDBServer_OpenCursor:
        return "WebIDBServer::OpenCursor";
    case MessageName::WebIDBServer_IterateCursor:
        return "WebIDBServer::IterateCursor";
    case MessageName::WebIDBServer_EstablishTransaction:
        return "WebIDBServer::EstablishTransaction";
    case MessageName::WebIDBServer_DatabaseConnectionPendingClose:
        return "WebIDBServer::DatabaseConnectionPendingClose";
    case MessageName::WebIDBServer_DatabaseConnectionClosed:
        return "WebIDBServer::DatabaseConnectionClosed";
    case MessageName::WebIDBServer_AbortOpenAndUpgradeNeeded:
        return "WebIDBServer::AbortOpenAndUpgradeNeeded";
    case MessageName::WebIDBServer_DidFireVersionChangeEvent:
        return "WebIDBServer::DidFireVersionChangeEvent";
    case MessageName::WebIDBServer_OpenDBRequestCancelled:
        return "WebIDBServer::OpenDBRequestCancelled";
    case MessageName::WebIDBServer_GetAllDatabaseNamesAndVersions:
        return "WebIDBServer::GetAllDatabaseNamesAndVersions";
    case MessageName::NetworkConnectionToWebProcess_ScheduleResourceLoad:
        return "NetworkConnectionToWebProcess::ScheduleResourceLoad";
    case MessageName::NetworkConnectionToWebProcess_PerformSynchronousLoad:
        return "NetworkConnectionToWebProcess::PerformSynchronousLoad";
    case MessageName::NetworkConnectionToWebProcess_TestProcessIncomingSyncMessagesWhenWaitingForSyncReply:
        return "NetworkConnectionToWebProcess::TestProcessIncomingSyncMessagesWhenWaitingForSyncReply";
    case MessageName::NetworkConnectionToWebProcess_LoadPing:
        return "NetworkConnectionToWebProcess::LoadPing";
    case MessageName::NetworkConnectionToWebProcess_RemoveLoadIdentifier:
        return "NetworkConnectionToWebProcess::RemoveLoadIdentifier";
    case MessageName::NetworkConnectionToWebProcess_PageLoadCompleted:
        return "NetworkConnectionToWebProcess::PageLoadCompleted";
    case MessageName::NetworkConnectionToWebProcess_BrowsingContextRemoved:
        return "NetworkConnectionToWebProcess::BrowsingContextRemoved";
    case MessageName::NetworkConnectionToWebProcess_PrefetchDNS:
        return "NetworkConnectionToWebProcess::PrefetchDNS";
    case MessageName::NetworkConnectionToWebProcess_PreconnectTo:
        return "NetworkConnectionToWebProcess::PreconnectTo";
    case MessageName::NetworkConnectionToWebProcess_StartDownload:
        return "NetworkConnectionToWebProcess::StartDownload";
    case MessageName::NetworkConnectionToWebProcess_ConvertMainResourceLoadToDownload:
        return "NetworkConnectionToWebProcess::ConvertMainResourceLoadToDownload";
    case MessageName::NetworkConnectionToWebProcess_CookiesForDOM:
        return "NetworkConnectionToWebProcess::CookiesForDOM";
    case MessageName::NetworkConnectionToWebProcess_SetCookiesFromDOM:
        return "NetworkConnectionToWebProcess::SetCookiesFromDOM";
    case MessageName::NetworkConnectionToWebProcess_CookieRequestHeaderFieldValue:
        return "NetworkConnectionToWebProcess::CookieRequestHeaderFieldValue";
    case MessageName::NetworkConnectionToWebProcess_GetRawCookies:
        return "NetworkConnectionToWebProcess::GetRawCookies";
    case MessageName::NetworkConnectionToWebProcess_SetRawCookie:
        return "NetworkConnectionToWebProcess::SetRawCookie";
    case MessageName::NetworkConnectionToWebProcess_DeleteCookie:
        return "NetworkConnectionToWebProcess::DeleteCookie";
    case MessageName::NetworkConnectionToWebProcess_DomCookiesForHost:
        return "NetworkConnectionToWebProcess::DomCookiesForHost";
#if HAVE(COOKIE_CHANGE_LISTENER_API)
    case MessageName::NetworkConnectionToWebProcess_UnsubscribeFromCookieChangeNotifications:
        return "NetworkConnectionToWebProcess::UnsubscribeFromCookieChangeNotifications";
#endif
    case MessageName::NetworkConnectionToWebProcess_RegisterFileBlobURL:
        return "NetworkConnectionToWebProcess::RegisterFileBlobURL";
    case MessageName::NetworkConnectionToWebProcess_RegisterBlobURL:
        return "NetworkConnectionToWebProcess::RegisterBlobURL";
    case MessageName::NetworkConnectionToWebProcess_RegisterBlobURLFromURL:
        return "NetworkConnectionToWebProcess::RegisterBlobURLFromURL";
    case MessageName::NetworkConnectionToWebProcess_RegisterBlobURLOptionallyFileBacked:
        return "NetworkConnectionToWebProcess::RegisterBlobURLOptionallyFileBacked";
    case MessageName::NetworkConnectionToWebProcess_RegisterBlobURLForSlice:
        return "NetworkConnectionToWebProcess::RegisterBlobURLForSlice";
    case MessageName::NetworkConnectionToWebProcess_UnregisterBlobURL:
        return "NetworkConnectionToWebProcess::UnregisterBlobURL";
    case MessageName::NetworkConnectionToWebProcess_BlobSize:
        return "NetworkConnectionToWebProcess::BlobSize";
    case MessageName::NetworkConnectionToWebProcess_WriteBlobsToTemporaryFiles:
        return "NetworkConnectionToWebProcess::WriteBlobsToTemporaryFiles";
    case MessageName::NetworkConnectionToWebProcess_WriteBlobsToTemporaryFilesReply:
        return "NetworkConnectionToWebProcess::WriteBlobsToTemporaryFilesReply";
    case MessageName::NetworkConnectionToWebProcess_SetCaptureExtraNetworkLoadMetricsEnabled:
        return "NetworkConnectionToWebProcess::SetCaptureExtraNetworkLoadMetricsEnabled";
    case MessageName::NetworkConnectionToWebProcess_CreateSocketStream:
        return "NetworkConnectionToWebProcess::CreateSocketStream";
    case MessageName::NetworkConnectionToWebProcess_CreateSocketChannel:
        return "NetworkConnectionToWebProcess::CreateSocketChannel";
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkConnectionToWebProcess_RemoveStorageAccessForFrame:
        return "NetworkConnectionToWebProcess::RemoveStorageAccessForFrame";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkConnectionToWebProcess_ClearPageSpecificDataForResourceLoadStatistics:
        return "NetworkConnectionToWebProcess::ClearPageSpecificDataForResourceLoadStatistics";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkConnectionToWebProcess_LogUserInteraction:
        return "NetworkConnectionToWebProcess::LogUserInteraction";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkConnectionToWebProcess_ResourceLoadStatisticsUpdated:
        return "NetworkConnectionToWebProcess::ResourceLoadStatisticsUpdated";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkConnectionToWebProcess_HasStorageAccess:
        return "NetworkConnectionToWebProcess::HasStorageAccess";
    case MessageName::NetworkConnectionToWebProcess_HasStorageAccessReply:
        return "NetworkConnectionToWebProcess::HasStorageAccessReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkConnectionToWebProcess_RequestStorageAccess:
        return "NetworkConnectionToWebProcess::RequestStorageAccess";
    case MessageName::NetworkConnectionToWebProcess_RequestStorageAccessReply:
        return "NetworkConnectionToWebProcess::RequestStorageAccessReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkConnectionToWebProcess_RequestStorageAccessUnderOpener:
        return "NetworkConnectionToWebProcess::RequestStorageAccessUnderOpener";
#endif
    case MessageName::NetworkConnectionToWebProcess_AddOriginAccessWhitelistEntry:
        return "NetworkConnectionToWebProcess::AddOriginAccessWhitelistEntry";
    case MessageName::NetworkConnectionToWebProcess_RemoveOriginAccessWhitelistEntry:
        return "NetworkConnectionToWebProcess::RemoveOriginAccessWhitelistEntry";
    case MessageName::NetworkConnectionToWebProcess_ResetOriginAccessWhitelists:
        return "NetworkConnectionToWebProcess::ResetOriginAccessWhitelists";
    case MessageName::NetworkConnectionToWebProcess_GetNetworkLoadInformationResponse:
        return "NetworkConnectionToWebProcess::GetNetworkLoadInformationResponse";
    case MessageName::NetworkConnectionToWebProcess_GetNetworkLoadIntermediateInformation:
        return "NetworkConnectionToWebProcess::GetNetworkLoadIntermediateInformation";
    case MessageName::NetworkConnectionToWebProcess_TakeNetworkLoadInformationMetrics:
        return "NetworkConnectionToWebProcess::TakeNetworkLoadInformationMetrics";
#if ENABLE(SERVICE_WORKER)
    case MessageName::NetworkConnectionToWebProcess_EstablishSWContextConnection:
        return "NetworkConnectionToWebProcess::EstablishSWContextConnection";
    case MessageName::NetworkConnectionToWebProcess_EstablishSWContextConnectionReply:
        return "NetworkConnectionToWebProcess::EstablishSWContextConnectionReply";
#endif
#if ENABLE(SERVICE_WORKER)
    case MessageName::NetworkConnectionToWebProcess_CloseSWContextConnection:
        return "NetworkConnectionToWebProcess::CloseSWContextConnection";
#endif
    case MessageName::NetworkConnectionToWebProcess_UpdateQuotaBasedOnSpaceUsageForTesting:
        return "NetworkConnectionToWebProcess::UpdateQuotaBasedOnSpaceUsageForTesting";
    case MessageName::NetworkConnectionToWebProcess_CreateNewMessagePortChannel:
        return "NetworkConnectionToWebProcess::CreateNewMessagePortChannel";
    case MessageName::NetworkConnectionToWebProcess_EntangleLocalPortInThisProcessToRemote:
        return "NetworkConnectionToWebProcess::EntangleLocalPortInThisProcessToRemote";
    case MessageName::NetworkConnectionToWebProcess_MessagePortDisentangled:
        return "NetworkConnectionToWebProcess::MessagePortDisentangled";
    case MessageName::NetworkConnectionToWebProcess_MessagePortClosed:
        return "NetworkConnectionToWebProcess::MessagePortClosed";
    case MessageName::NetworkConnectionToWebProcess_TakeAllMessagesForPort:
        return "NetworkConnectionToWebProcess::TakeAllMessagesForPort";
    case MessageName::NetworkConnectionToWebProcess_TakeAllMessagesForPortReply:
        return "NetworkConnectionToWebProcess::TakeAllMessagesForPortReply";
    case MessageName::NetworkConnectionToWebProcess_PostMessageToRemote:
        return "NetworkConnectionToWebProcess::PostMessageToRemote";
    case MessageName::NetworkConnectionToWebProcess_CheckRemotePortForActivity:
        return "NetworkConnectionToWebProcess::CheckRemotePortForActivity";
    case MessageName::NetworkConnectionToWebProcess_CheckRemotePortForActivityReply:
        return "NetworkConnectionToWebProcess::CheckRemotePortForActivityReply";
    case MessageName::NetworkConnectionToWebProcess_DidDeliverMessagePortMessages:
        return "NetworkConnectionToWebProcess::DidDeliverMessagePortMessages";
    case MessageName::NetworkConnectionToWebProcess_RegisterURLSchemesAsCORSEnabled:
        return "NetworkConnectionToWebProcess::RegisterURLSchemesAsCORSEnabled";
    case MessageName::NetworkContentRuleListManager_Remove:
        return "NetworkContentRuleListManager::Remove";
    case MessageName::NetworkContentRuleListManager_AddContentRuleLists:
        return "NetworkContentRuleListManager::AddContentRuleLists";
    case MessageName::NetworkContentRuleListManager_RemoveContentRuleList:
        return "NetworkContentRuleListManager::RemoveContentRuleList";
    case MessageName::NetworkContentRuleListManager_RemoveAllContentRuleLists:
        return "NetworkContentRuleListManager::RemoveAllContentRuleLists";
    case MessageName::NetworkProcess_InitializeNetworkProcess:
        return "NetworkProcess::InitializeNetworkProcess";
    case MessageName::NetworkProcess_CreateNetworkConnectionToWebProcess:
        return "NetworkProcess::CreateNetworkConnectionToWebProcess";
    case MessageName::NetworkProcess_CreateNetworkConnectionToWebProcessReply:
        return "NetworkProcess::CreateNetworkConnectionToWebProcessReply";
#if USE(SOUP)
    case MessageName::NetworkProcess_SetIgnoreTLSErrors:
        return "NetworkProcess::SetIgnoreTLSErrors";
#endif
#if USE(SOUP)
    case MessageName::NetworkProcess_UserPreferredLanguagesChanged:
        return "NetworkProcess::UserPreferredLanguagesChanged";
#endif
#if USE(SOUP)
    case MessageName::NetworkProcess_SetNetworkProxySettings:
        return "NetworkProcess::SetNetworkProxySettings";
#endif
#if USE(SOUP)
    case MessageName::NetworkProcess_PrefetchDNS:
        return "NetworkProcess::PrefetchDNS";
#endif
#if USE(CURL)
    case MessageName::NetworkProcess_SetNetworkProxySettings:
        return "NetworkProcess::SetNetworkProxySettings";
#endif
    case MessageName::NetworkProcess_ClearCachedCredentials:
        return "NetworkProcess::ClearCachedCredentials";
    case MessageName::NetworkProcess_AddWebsiteDataStore:
        return "NetworkProcess::AddWebsiteDataStore";
    case MessageName::NetworkProcess_DestroySession:
        return "NetworkProcess::DestroySession";
    case MessageName::NetworkProcess_FetchWebsiteData:
        return "NetworkProcess::FetchWebsiteData";
    case MessageName::NetworkProcess_DeleteWebsiteData:
        return "NetworkProcess::DeleteWebsiteData";
    case MessageName::NetworkProcess_DeleteWebsiteDataForOrigins:
        return "NetworkProcess::DeleteWebsiteDataForOrigins";
    case MessageName::NetworkProcess_RenameOriginInWebsiteData:
        return "NetworkProcess::RenameOriginInWebsiteData";
    case MessageName::NetworkProcess_RenameOriginInWebsiteDataReply:
        return "NetworkProcess::RenameOriginInWebsiteDataReply";
    case MessageName::NetworkProcess_DownloadRequest:
        return "NetworkProcess::DownloadRequest";
    case MessageName::NetworkProcess_ResumeDownload:
        return "NetworkProcess::ResumeDownload";
    case MessageName::NetworkProcess_CancelDownload:
        return "NetworkProcess::CancelDownload";
#if PLATFORM(COCOA)
    case MessageName::NetworkProcess_PublishDownloadProgress:
        return "NetworkProcess::PublishDownloadProgress";
#endif
    case MessageName::NetworkProcess_ApplicationDidEnterBackground:
        return "NetworkProcess::ApplicationDidEnterBackground";
    case MessageName::NetworkProcess_ApplicationWillEnterForeground:
        return "NetworkProcess::ApplicationWillEnterForeground";
    case MessageName::NetworkProcess_ContinueWillSendRequest:
        return "NetworkProcess::ContinueWillSendRequest";
    case MessageName::NetworkProcess_ContinueDecidePendingDownloadDestination:
        return "NetworkProcess::ContinueDecidePendingDownloadDestination";
#if PLATFORM(COCOA)
    case MessageName::NetworkProcess_SetQOS:
        return "NetworkProcess::SetQOS";
#endif
#if PLATFORM(COCOA)
    case MessageName::NetworkProcess_SetStorageAccessAPIEnabled:
        return "NetworkProcess::SetStorageAccessAPIEnabled";
#endif
    case MessageName::NetworkProcess_SetAllowsAnySSLCertificateForWebSocket:
        return "NetworkProcess::SetAllowsAnySSLCertificateForWebSocket";
    case MessageName::NetworkProcess_SyncAllCookies:
        return "NetworkProcess::SyncAllCookies";
    case MessageName::NetworkProcess_AllowSpecificHTTPSCertificateForHost:
        return "NetworkProcess::AllowSpecificHTTPSCertificateForHost";
    case MessageName::NetworkProcess_SetCacheModel:
        return "NetworkProcess::SetCacheModel";
    case MessageName::NetworkProcess_SetCacheModelSynchronouslyForTesting:
        return "NetworkProcess::SetCacheModelSynchronouslyForTesting";
    case MessageName::NetworkProcess_ProcessDidTransitionToBackground:
        return "NetworkProcess::ProcessDidTransitionToBackground";
    case MessageName::NetworkProcess_ProcessDidTransitionToForeground:
        return "NetworkProcess::ProcessDidTransitionToForeground";
    case MessageName::NetworkProcess_ProcessWillSuspendImminentlyForTestingSync:
        return "NetworkProcess::ProcessWillSuspendImminentlyForTestingSync";
    case MessageName::NetworkProcess_PrepareToSuspend:
        return "NetworkProcess::PrepareToSuspend";
    case MessageName::NetworkProcess_PrepareToSuspendReply:
        return "NetworkProcess::PrepareToSuspendReply";
    case MessageName::NetworkProcess_ProcessDidResume:
        return "NetworkProcess::ProcessDidResume";
    case MessageName::NetworkProcess_PreconnectTo:
        return "NetworkProcess::PreconnectTo";
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ClearPrevalentResource:
        return "NetworkProcess::ClearPrevalentResource";
    case MessageName::NetworkProcess_ClearPrevalentResourceReply:
        return "NetworkProcess::ClearPrevalentResourceReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ClearUserInteraction:
        return "NetworkProcess::ClearUserInteraction";
    case MessageName::NetworkProcess_ClearUserInteractionReply:
        return "NetworkProcess::ClearUserInteractionReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_DumpResourceLoadStatistics:
        return "NetworkProcess::DumpResourceLoadStatistics";
    case MessageName::NetworkProcess_DumpResourceLoadStatisticsReply:
        return "NetworkProcess::DumpResourceLoadStatisticsReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetResourceLoadStatisticsEnabled:
        return "NetworkProcess::SetResourceLoadStatisticsEnabled";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetResourceLoadStatisticsLogTestingEvent:
        return "NetworkProcess::SetResourceLoadStatisticsLogTestingEvent";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_UpdatePrevalentDomainsToBlockCookiesFor:
        return "NetworkProcess::UpdatePrevalentDomainsToBlockCookiesFor";
    case MessageName::NetworkProcess_UpdatePrevalentDomainsToBlockCookiesForReply:
        return "NetworkProcess::UpdatePrevalentDomainsToBlockCookiesForReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsGrandfathered:
        return "NetworkProcess::IsGrandfathered";
    case MessageName::NetworkProcess_IsGrandfatheredReply:
        return "NetworkProcess::IsGrandfatheredReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsPrevalentResource:
        return "NetworkProcess::IsPrevalentResource";
    case MessageName::NetworkProcess_IsPrevalentResourceReply:
        return "NetworkProcess::IsPrevalentResourceReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsVeryPrevalentResource:
        return "NetworkProcess::IsVeryPrevalentResource";
    case MessageName::NetworkProcess_IsVeryPrevalentResourceReply:
        return "NetworkProcess::IsVeryPrevalentResourceReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetAgeCapForClientSideCookies:
        return "NetworkProcess::SetAgeCapForClientSideCookies";
    case MessageName::NetworkProcess_SetAgeCapForClientSideCookiesReply:
        return "NetworkProcess::SetAgeCapForClientSideCookiesReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetLastSeen:
        return "NetworkProcess::SetLastSeen";
    case MessageName::NetworkProcess_SetLastSeenReply:
        return "NetworkProcess::SetLastSeenReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_MergeStatisticForTesting:
        return "NetworkProcess::MergeStatisticForTesting";
    case MessageName::NetworkProcess_MergeStatisticForTestingReply:
        return "NetworkProcess::MergeStatisticForTestingReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_InsertExpiredStatisticForTesting:
        return "NetworkProcess::InsertExpiredStatisticForTesting";
    case MessageName::NetworkProcess_InsertExpiredStatisticForTestingReply:
        return "NetworkProcess::InsertExpiredStatisticForTestingReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetPrevalentResource:
        return "NetworkProcess::SetPrevalentResource";
    case MessageName::NetworkProcess_SetPrevalentResourceReply:
        return "NetworkProcess::SetPrevalentResourceReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetPrevalentResourceForDebugMode:
        return "NetworkProcess::SetPrevalentResourceForDebugMode";
    case MessageName::NetworkProcess_SetPrevalentResourceForDebugModeReply:
        return "NetworkProcess::SetPrevalentResourceForDebugModeReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsResourceLoadStatisticsEphemeral:
        return "NetworkProcess::IsResourceLoadStatisticsEphemeral";
    case MessageName::NetworkProcess_IsResourceLoadStatisticsEphemeralReply:
        return "NetworkProcess::IsResourceLoadStatisticsEphemeralReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_HadUserInteraction:
        return "NetworkProcess::HadUserInteraction";
    case MessageName::NetworkProcess_HadUserInteractionReply:
        return "NetworkProcess::HadUserInteractionReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsRelationshipOnlyInDatabaseOnce:
        return "NetworkProcess::IsRelationshipOnlyInDatabaseOnce";
    case MessageName::NetworkProcess_IsRelationshipOnlyInDatabaseOnceReply:
        return "NetworkProcess::IsRelationshipOnlyInDatabaseOnceReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_HasLocalStorage:
        return "NetworkProcess::HasLocalStorage";
    case MessageName::NetworkProcess_HasLocalStorageReply:
        return "NetworkProcess::HasLocalStorageReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_GetAllStorageAccessEntries:
        return "NetworkProcess::GetAllStorageAccessEntries";
    case MessageName::NetworkProcess_GetAllStorageAccessEntriesReply:
        return "NetworkProcess::GetAllStorageAccessEntriesReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsRegisteredAsRedirectingTo:
        return "NetworkProcess::IsRegisteredAsRedirectingTo";
    case MessageName::NetworkProcess_IsRegisteredAsRedirectingToReply:
        return "NetworkProcess::IsRegisteredAsRedirectingToReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsRegisteredAsSubFrameUnder:
        return "NetworkProcess::IsRegisteredAsSubFrameUnder";
    case MessageName::NetworkProcess_IsRegisteredAsSubFrameUnderReply:
        return "NetworkProcess::IsRegisteredAsSubFrameUnderReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsRegisteredAsSubresourceUnder:
        return "NetworkProcess::IsRegisteredAsSubresourceUnder";
    case MessageName::NetworkProcess_IsRegisteredAsSubresourceUnderReply:
        return "NetworkProcess::IsRegisteredAsSubresourceUnderReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_DomainIDExistsInDatabase:
        return "NetworkProcess::DomainIDExistsInDatabase";
    case MessageName::NetworkProcess_DomainIDExistsInDatabaseReply:
        return "NetworkProcess::DomainIDExistsInDatabaseReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_LogFrameNavigation:
        return "NetworkProcess::LogFrameNavigation";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_LogUserInteraction:
        return "NetworkProcess::LogUserInteraction";
    case MessageName::NetworkProcess_LogUserInteractionReply:
        return "NetworkProcess::LogUserInteractionReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ResetParametersToDefaultValues:
        return "NetworkProcess::ResetParametersToDefaultValues";
    case MessageName::NetworkProcess_ResetParametersToDefaultValuesReply:
        return "NetworkProcess::ResetParametersToDefaultValuesReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ScheduleClearInMemoryAndPersistent:
        return "NetworkProcess::ScheduleClearInMemoryAndPersistent";
    case MessageName::NetworkProcess_ScheduleClearInMemoryAndPersistentReply:
        return "NetworkProcess::ScheduleClearInMemoryAndPersistentReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ScheduleCookieBlockingUpdate:
        return "NetworkProcess::ScheduleCookieBlockingUpdate";
    case MessageName::NetworkProcess_ScheduleCookieBlockingUpdateReply:
        return "NetworkProcess::ScheduleCookieBlockingUpdateReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ScheduleStatisticsAndDataRecordsProcessing:
        return "NetworkProcess::ScheduleStatisticsAndDataRecordsProcessing";
    case MessageName::NetworkProcess_ScheduleStatisticsAndDataRecordsProcessingReply:
        return "NetworkProcess::ScheduleStatisticsAndDataRecordsProcessingReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_StatisticsDatabaseHasAllTables:
        return "NetworkProcess::StatisticsDatabaseHasAllTables";
    case MessageName::NetworkProcess_StatisticsDatabaseHasAllTablesReply:
        return "NetworkProcess::StatisticsDatabaseHasAllTablesReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SubmitTelemetry:
        return "NetworkProcess::SubmitTelemetry";
    case MessageName::NetworkProcess_SubmitTelemetryReply:
        return "NetworkProcess::SubmitTelemetryReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetCacheMaxAgeCapForPrevalentResources:
        return "NetworkProcess::SetCacheMaxAgeCapForPrevalentResources";
    case MessageName::NetworkProcess_SetCacheMaxAgeCapForPrevalentResourcesReply:
        return "NetworkProcess::SetCacheMaxAgeCapForPrevalentResourcesReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetGrandfathered:
        return "NetworkProcess::SetGrandfathered";
    case MessageName::NetworkProcess_SetGrandfatheredReply:
        return "NetworkProcess::SetGrandfatheredReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetUseITPDatabase:
        return "NetworkProcess::SetUseITPDatabase";
    case MessageName::NetworkProcess_SetUseITPDatabaseReply:
        return "NetworkProcess::SetUseITPDatabaseReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_GetResourceLoadStatisticsDataSummary:
        return "NetworkProcess::GetResourceLoadStatisticsDataSummary";
    case MessageName::NetworkProcess_GetResourceLoadStatisticsDataSummaryReply:
        return "NetworkProcess::GetResourceLoadStatisticsDataSummaryReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetGrandfatheringTime:
        return "NetworkProcess::SetGrandfatheringTime";
    case MessageName::NetworkProcess_SetGrandfatheringTimeReply:
        return "NetworkProcess::SetGrandfatheringTimeReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetMaxStatisticsEntries:
        return "NetworkProcess::SetMaxStatisticsEntries";
    case MessageName::NetworkProcess_SetMaxStatisticsEntriesReply:
        return "NetworkProcess::SetMaxStatisticsEntriesReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetMinimumTimeBetweenDataRecordsRemoval:
        return "NetworkProcess::SetMinimumTimeBetweenDataRecordsRemoval";
    case MessageName::NetworkProcess_SetMinimumTimeBetweenDataRecordsRemovalReply:
        return "NetworkProcess::SetMinimumTimeBetweenDataRecordsRemovalReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetPruneEntriesDownTo:
        return "NetworkProcess::SetPruneEntriesDownTo";
    case MessageName::NetworkProcess_SetPruneEntriesDownToReply:
        return "NetworkProcess::SetPruneEntriesDownToReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetShouldClassifyResourcesBeforeDataRecordsRemoval:
        return "NetworkProcess::SetShouldClassifyResourcesBeforeDataRecordsRemoval";
    case MessageName::NetworkProcess_SetShouldClassifyResourcesBeforeDataRecordsRemovalReply:
        return "NetworkProcess::SetShouldClassifyResourcesBeforeDataRecordsRemovalReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetNotifyPagesWhenDataRecordsWereScanned:
        return "NetworkProcess::SetNotifyPagesWhenDataRecordsWereScanned";
    case MessageName::NetworkProcess_SetNotifyPagesWhenDataRecordsWereScannedReply:
        return "NetworkProcess::SetNotifyPagesWhenDataRecordsWereScannedReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetIsRunningResourceLoadStatisticsTest:
        return "NetworkProcess::SetIsRunningResourceLoadStatisticsTest";
    case MessageName::NetworkProcess_SetIsRunningResourceLoadStatisticsTestReply:
        return "NetworkProcess::SetIsRunningResourceLoadStatisticsTestReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetNotifyPagesWhenTelemetryWasCaptured:
        return "NetworkProcess::SetNotifyPagesWhenTelemetryWasCaptured";
    case MessageName::NetworkProcess_SetNotifyPagesWhenTelemetryWasCapturedReply:
        return "NetworkProcess::SetNotifyPagesWhenTelemetryWasCapturedReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetResourceLoadStatisticsDebugMode:
        return "NetworkProcess::SetResourceLoadStatisticsDebugMode";
    case MessageName::NetworkProcess_SetResourceLoadStatisticsDebugModeReply:
        return "NetworkProcess::SetResourceLoadStatisticsDebugModeReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetVeryPrevalentResource:
        return "NetworkProcess::SetVeryPrevalentResource";
    case MessageName::NetworkProcess_SetVeryPrevalentResourceReply:
        return "NetworkProcess::SetVeryPrevalentResourceReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetSubframeUnderTopFrameDomain:
        return "NetworkProcess::SetSubframeUnderTopFrameDomain";
    case MessageName::NetworkProcess_SetSubframeUnderTopFrameDomainReply:
        return "NetworkProcess::SetSubframeUnderTopFrameDomainReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetSubresourceUnderTopFrameDomain:
        return "NetworkProcess::SetSubresourceUnderTopFrameDomain";
    case MessageName::NetworkProcess_SetSubresourceUnderTopFrameDomainReply:
        return "NetworkProcess::SetSubresourceUnderTopFrameDomainReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetSubresourceUniqueRedirectTo:
        return "NetworkProcess::SetSubresourceUniqueRedirectTo";
    case MessageName::NetworkProcess_SetSubresourceUniqueRedirectToReply:
        return "NetworkProcess::SetSubresourceUniqueRedirectToReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetSubresourceUniqueRedirectFrom:
        return "NetworkProcess::SetSubresourceUniqueRedirectFrom";
    case MessageName::NetworkProcess_SetSubresourceUniqueRedirectFromReply:
        return "NetworkProcess::SetSubresourceUniqueRedirectFromReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetTimeToLiveUserInteraction:
        return "NetworkProcess::SetTimeToLiveUserInteraction";
    case MessageName::NetworkProcess_SetTimeToLiveUserInteractionReply:
        return "NetworkProcess::SetTimeToLiveUserInteractionReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetTopFrameUniqueRedirectTo:
        return "NetworkProcess::SetTopFrameUniqueRedirectTo";
    case MessageName::NetworkProcess_SetTopFrameUniqueRedirectToReply:
        return "NetworkProcess::SetTopFrameUniqueRedirectToReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetTopFrameUniqueRedirectFrom:
        return "NetworkProcess::SetTopFrameUniqueRedirectFrom";
    case MessageName::NetworkProcess_SetTopFrameUniqueRedirectFromReply:
        return "NetworkProcess::SetTopFrameUniqueRedirectFromReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ResetCacheMaxAgeCapForPrevalentResources:
        return "NetworkProcess::ResetCacheMaxAgeCapForPrevalentResources";
    case MessageName::NetworkProcess_ResetCacheMaxAgeCapForPrevalentResourcesReply:
        return "NetworkProcess::ResetCacheMaxAgeCapForPrevalentResourcesReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_DidCommitCrossSiteLoadWithDataTransfer:
        return "NetworkProcess::DidCommitCrossSiteLoadWithDataTransfer";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetCrossSiteLoadWithLinkDecorationForTesting:
        return "NetworkProcess::SetCrossSiteLoadWithLinkDecorationForTesting";
    case MessageName::NetworkProcess_SetCrossSiteLoadWithLinkDecorationForTestingReply:
        return "NetworkProcess::SetCrossSiteLoadWithLinkDecorationForTestingReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ResetCrossSiteLoadsWithLinkDecorationForTesting:
        return "NetworkProcess::ResetCrossSiteLoadsWithLinkDecorationForTesting";
    case MessageName::NetworkProcess_ResetCrossSiteLoadsWithLinkDecorationForTestingReply:
        return "NetworkProcess::ResetCrossSiteLoadsWithLinkDecorationForTestingReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_DeleteCookiesForTesting:
        return "NetworkProcess::DeleteCookiesForTesting";
    case MessageName::NetworkProcess_DeleteCookiesForTestingReply:
        return "NetworkProcess::DeleteCookiesForTestingReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_HasIsolatedSession:
        return "NetworkProcess::HasIsolatedSession";
    case MessageName::NetworkProcess_HasIsolatedSessionReply:
        return "NetworkProcess::HasIsolatedSessionReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetAppBoundDomainsForResourceLoadStatistics:
        return "NetworkProcess::SetAppBoundDomainsForResourceLoadStatistics";
    case MessageName::NetworkProcess_SetAppBoundDomainsForResourceLoadStatisticsReply:
        return "NetworkProcess::SetAppBoundDomainsForResourceLoadStatisticsReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetShouldDowngradeReferrerForTesting:
        return "NetworkProcess::SetShouldDowngradeReferrerForTesting";
    case MessageName::NetworkProcess_SetShouldDowngradeReferrerForTestingReply:
        return "NetworkProcess::SetShouldDowngradeReferrerForTestingReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetThirdPartyCookieBlockingMode:
        return "NetworkProcess::SetThirdPartyCookieBlockingMode";
    case MessageName::NetworkProcess_SetThirdPartyCookieBlockingModeReply:
        return "NetworkProcess::SetThirdPartyCookieBlockingModeReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetShouldEnbleSameSiteStrictEnforcementForTesting:
        return "NetworkProcess::SetShouldEnbleSameSiteStrictEnforcementForTesting";
    case MessageName::NetworkProcess_SetShouldEnbleSameSiteStrictEnforcementForTestingReply:
        return "NetworkProcess::SetShouldEnbleSameSiteStrictEnforcementForTestingReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetFirstPartyWebsiteDataRemovalModeForTesting:
        return "NetworkProcess::SetFirstPartyWebsiteDataRemovalModeForTesting";
    case MessageName::NetworkProcess_SetFirstPartyWebsiteDataRemovalModeForTestingReply:
        return "NetworkProcess::SetFirstPartyWebsiteDataRemovalModeForTestingReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetToSameSiteStrictCookiesForTesting:
        return "NetworkProcess::SetToSameSiteStrictCookiesForTesting";
    case MessageName::NetworkProcess_SetToSameSiteStrictCookiesForTestingReply:
        return "NetworkProcess::SetToSameSiteStrictCookiesForTestingReply";
#endif
    case MessageName::NetworkProcess_SetAdClickAttributionDebugMode:
        return "NetworkProcess::SetAdClickAttributionDebugMode";
    case MessageName::NetworkProcess_SetSessionIsControlledByAutomation:
        return "NetworkProcess::SetSessionIsControlledByAutomation";
    case MessageName::NetworkProcess_RegisterURLSchemeAsSecure:
        return "NetworkProcess::RegisterURLSchemeAsSecure";
    case MessageName::NetworkProcess_RegisterURLSchemeAsBypassingContentSecurityPolicy:
        return "NetworkProcess::RegisterURLSchemeAsBypassingContentSecurityPolicy";
    case MessageName::NetworkProcess_RegisterURLSchemeAsLocal:
        return "NetworkProcess::RegisterURLSchemeAsLocal";
    case MessageName::NetworkProcess_RegisterURLSchemeAsNoAccess:
        return "NetworkProcess::RegisterURLSchemeAsNoAccess";
    case MessageName::NetworkProcess_SetCacheStorageParameters:
        return "NetworkProcess::SetCacheStorageParameters";
    case MessageName::NetworkProcess_SyncLocalStorage:
        return "NetworkProcess::SyncLocalStorage";
    case MessageName::NetworkProcess_ClearLegacyPrivateBrowsingLocalStorage:
        return "NetworkProcess::ClearLegacyPrivateBrowsingLocalStorage";
    case MessageName::NetworkProcess_StoreAdClickAttribution:
        return "NetworkProcess::StoreAdClickAttribution";
    case MessageName::NetworkProcess_DumpAdClickAttribution:
        return "NetworkProcess::DumpAdClickAttribution";
    case MessageName::NetworkProcess_DumpAdClickAttributionReply:
        return "NetworkProcess::DumpAdClickAttributionReply";
    case MessageName::NetworkProcess_ClearAdClickAttribution:
        return "NetworkProcess::ClearAdClickAttribution";
    case MessageName::NetworkProcess_ClearAdClickAttributionReply:
        return "NetworkProcess::ClearAdClickAttributionReply";
    case MessageName::NetworkProcess_SetAdClickAttributionOverrideTimerForTesting:
        return "NetworkProcess::SetAdClickAttributionOverrideTimerForTesting";
    case MessageName::NetworkProcess_SetAdClickAttributionOverrideTimerForTestingReply:
        return "NetworkProcess::SetAdClickAttributionOverrideTimerForTestingReply";
    case MessageName::NetworkProcess_SetAdClickAttributionConversionURLForTesting:
        return "NetworkProcess::SetAdClickAttributionConversionURLForTesting";
    case MessageName::NetworkProcess_SetAdClickAttributionConversionURLForTestingReply:
        return "NetworkProcess::SetAdClickAttributionConversionURLForTestingReply";
    case MessageName::NetworkProcess_MarkAdClickAttributionsAsExpiredForTesting:
        return "NetworkProcess::MarkAdClickAttributionsAsExpiredForTesting";
    case MessageName::NetworkProcess_MarkAdClickAttributionsAsExpiredForTestingReply:
        return "NetworkProcess::MarkAdClickAttributionsAsExpiredForTestingReply";
    case MessageName::NetworkProcess_GetLocalStorageOriginDetails:
        return "NetworkProcess::GetLocalStorageOriginDetails";
    case MessageName::NetworkProcess_GetLocalStorageOriginDetailsReply:
        return "NetworkProcess::GetLocalStorageOriginDetailsReply";
    case MessageName::NetworkProcess_SetServiceWorkerFetchTimeoutForTesting:
        return "NetworkProcess::SetServiceWorkerFetchTimeoutForTesting";
    case MessageName::NetworkProcess_ResetServiceWorkerFetchTimeoutForTesting:
        return "NetworkProcess::ResetServiceWorkerFetchTimeoutForTesting";
    case MessageName::NetworkProcess_ResetQuota:
        return "NetworkProcess::ResetQuota";
    case MessageName::NetworkProcess_ResetQuotaReply:
        return "NetworkProcess::ResetQuotaReply";
    case MessageName::NetworkProcess_HasAppBoundSession:
        return "NetworkProcess::HasAppBoundSession";
    case MessageName::NetworkProcess_HasAppBoundSessionReply:
        return "NetworkProcess::HasAppBoundSessionReply";
    case MessageName::NetworkProcess_ClearAppBoundSession:
        return "NetworkProcess::ClearAppBoundSession";
    case MessageName::NetworkProcess_ClearAppBoundSessionReply:
        return "NetworkProcess::ClearAppBoundSessionReply";
#if PLATFORM(IOS_FAMILY) && !PLATFORM(MACCATALYST)
    case MessageName::NetworkProcess_DisableServiceWorkerEntitlement:
        return "NetworkProcess::DisableServiceWorkerEntitlement";
#endif
#if PLATFORM(IOS_FAMILY) && !PLATFORM(MACCATALYST)
    case MessageName::NetworkProcess_ClearServiceWorkerEntitlementOverride:
        return "NetworkProcess::ClearServiceWorkerEntitlementOverride";
    case MessageName::NetworkProcess_ClearServiceWorkerEntitlementOverrideReply:
        return "NetworkProcess::ClearServiceWorkerEntitlementOverrideReply";
#endif
    case MessageName::NetworkProcess_UpdateBundleIdentifier:
        return "NetworkProcess::UpdateBundleIdentifier";
    case MessageName::NetworkProcess_UpdateBundleIdentifierReply:
        return "NetworkProcess::UpdateBundleIdentifierReply";
    case MessageName::NetworkProcess_ClearBundleIdentifier:
        return "NetworkProcess::ClearBundleIdentifier";
    case MessageName::NetworkProcess_ClearBundleIdentifierReply:
        return "NetworkProcess::ClearBundleIdentifierReply";
    case MessageName::NetworkResourceLoader_ContinueWillSendRequest:
        return "NetworkResourceLoader::ContinueWillSendRequest";
    case MessageName::NetworkResourceLoader_ContinueDidReceiveResponse:
        return "NetworkResourceLoader::ContinueDidReceiveResponse";
    case MessageName::NetworkSocketChannel_SendString:
        return "NetworkSocketChannel::SendString";
    case MessageName::NetworkSocketChannel_SendStringReply:
        return "NetworkSocketChannel::SendStringReply";
    case MessageName::NetworkSocketChannel_SendData:
        return "NetworkSocketChannel::SendData";
    case MessageName::NetworkSocketChannel_SendDataReply:
        return "NetworkSocketChannel::SendDataReply";
    case MessageName::NetworkSocketChannel_Close:
        return "NetworkSocketChannel::Close";
    case MessageName::NetworkSocketStream_SendData:
        return "NetworkSocketStream::SendData";
    case MessageName::NetworkSocketStream_SendHandshake:
        return "NetworkSocketStream::SendHandshake";
    case MessageName::NetworkSocketStream_Close:
        return "NetworkSocketStream::Close";
    case MessageName::ServiceWorkerFetchTask_DidNotHandle:
        return "ServiceWorkerFetchTask::DidNotHandle";
    case MessageName::ServiceWorkerFetchTask_DidFail:
        return "ServiceWorkerFetchTask::DidFail";
    case MessageName::ServiceWorkerFetchTask_DidReceiveRedirectResponse:
        return "ServiceWorkerFetchTask::DidReceiveRedirectResponse";
    case MessageName::ServiceWorkerFetchTask_DidReceiveResponse:
        return "ServiceWorkerFetchTask::DidReceiveResponse";
    case MessageName::ServiceWorkerFetchTask_DidReceiveData:
        return "ServiceWorkerFetchTask::DidReceiveData";
    case MessageName::ServiceWorkerFetchTask_DidReceiveSharedBuffer:
        return "ServiceWorkerFetchTask::DidReceiveSharedBuffer";
    case MessageName::ServiceWorkerFetchTask_DidReceiveFormData:
        return "ServiceWorkerFetchTask::DidReceiveFormData";
    case MessageName::ServiceWorkerFetchTask_DidFinish:
        return "ServiceWorkerFetchTask::DidFinish";
    case MessageName::WebSWServerConnection_ScheduleJobInServer:
        return "WebSWServerConnection::ScheduleJobInServer";
    case MessageName::WebSWServerConnection_ScheduleUnregisterJobInServer:
        return "WebSWServerConnection::ScheduleUnregisterJobInServer";
    case MessageName::WebSWServerConnection_ScheduleUnregisterJobInServerReply:
        return "WebSWServerConnection::ScheduleUnregisterJobInServerReply";
    case MessageName::WebSWServerConnection_FinishFetchingScriptInServer:
        return "WebSWServerConnection::FinishFetchingScriptInServer";
    case MessageName::WebSWServerConnection_AddServiceWorkerRegistrationInServer:
        return "WebSWServerConnection::AddServiceWorkerRegistrationInServer";
    case MessageName::WebSWServerConnection_RemoveServiceWorkerRegistrationInServer:
        return "WebSWServerConnection::RemoveServiceWorkerRegistrationInServer";
    case MessageName::WebSWServerConnection_PostMessageToServiceWorker:
        return "WebSWServerConnection::PostMessageToServiceWorker";
    case MessageName::WebSWServerConnection_DidResolveRegistrationPromise:
        return "WebSWServerConnection::DidResolveRegistrationPromise";
    case MessageName::WebSWServerConnection_MatchRegistration:
        return "WebSWServerConnection::MatchRegistration";
    case MessageName::WebSWServerConnection_WhenRegistrationReady:
        return "WebSWServerConnection::WhenRegistrationReady";
    case MessageName::WebSWServerConnection_GetRegistrations:
        return "WebSWServerConnection::GetRegistrations";
    case MessageName::WebSWServerConnection_RegisterServiceWorkerClient:
        return "WebSWServerConnection::RegisterServiceWorkerClient";
    case MessageName::WebSWServerConnection_UnregisterServiceWorkerClient:
        return "WebSWServerConnection::UnregisterServiceWorkerClient";
    case MessageName::WebSWServerConnection_TerminateWorkerFromClient:
        return "WebSWServerConnection::TerminateWorkerFromClient";
    case MessageName::WebSWServerConnection_TerminateWorkerFromClientReply:
        return "WebSWServerConnection::TerminateWorkerFromClientReply";
    case MessageName::WebSWServerConnection_WhenServiceWorkerIsTerminatedForTesting:
        return "WebSWServerConnection::WhenServiceWorkerIsTerminatedForTesting";
    case MessageName::WebSWServerConnection_WhenServiceWorkerIsTerminatedForTestingReply:
        return "WebSWServerConnection::WhenServiceWorkerIsTerminatedForTestingReply";
    case MessageName::WebSWServerConnection_SetThrottleState:
        return "WebSWServerConnection::SetThrottleState";
    case MessageName::WebSWServerConnection_StoreRegistrationsOnDisk:
        return "WebSWServerConnection::StoreRegistrationsOnDisk";
    case MessageName::WebSWServerConnection_StoreRegistrationsOnDiskReply:
        return "WebSWServerConnection::StoreRegistrationsOnDiskReply";
    case MessageName::WebSWServerToContextConnection_ScriptContextFailedToStart:
        return "WebSWServerToContextConnection::ScriptContextFailedToStart";
    case MessageName::WebSWServerToContextConnection_ScriptContextStarted:
        return "WebSWServerToContextConnection::ScriptContextStarted";
    case MessageName::WebSWServerToContextConnection_DidFinishInstall:
        return "WebSWServerToContextConnection::DidFinishInstall";
    case MessageName::WebSWServerToContextConnection_DidFinishActivation:
        return "WebSWServerToContextConnection::DidFinishActivation";
    case MessageName::WebSWServerToContextConnection_SetServiceWorkerHasPendingEvents:
        return "WebSWServerToContextConnection::SetServiceWorkerHasPendingEvents";
    case MessageName::WebSWServerToContextConnection_SkipWaiting:
        return "WebSWServerToContextConnection::SkipWaiting";
    case MessageName::WebSWServerToContextConnection_SkipWaitingReply:
        return "WebSWServerToContextConnection::SkipWaitingReply";
    case MessageName::WebSWServerToContextConnection_WorkerTerminated:
        return "WebSWServerToContextConnection::WorkerTerminated";
    case MessageName::WebSWServerToContextConnection_FindClientByIdentifier:
        return "WebSWServerToContextConnection::FindClientByIdentifier";
    case MessageName::WebSWServerToContextConnection_MatchAll:
        return "WebSWServerToContextConnection::MatchAll";
    case MessageName::WebSWServerToContextConnection_Claim:
        return "WebSWServerToContextConnection::Claim";
    case MessageName::WebSWServerToContextConnection_ClaimReply:
        return "WebSWServerToContextConnection::ClaimReply";
    case MessageName::WebSWServerToContextConnection_SetScriptResource:
        return "WebSWServerToContextConnection::SetScriptResource";
    case MessageName::WebSWServerToContextConnection_PostMessageToServiceWorkerClient:
        return "WebSWServerToContextConnection::PostMessageToServiceWorkerClient";
    case MessageName::WebSWServerToContextConnection_DidFailHeartBeatCheck:
        return "WebSWServerToContextConnection::DidFailHeartBeatCheck";
    case MessageName::StorageManagerSet_ConnectToLocalStorageArea:
        return "StorageManagerSet::ConnectToLocalStorageArea";
    case MessageName::StorageManagerSet_ConnectToTransientLocalStorageArea:
        return "StorageManagerSet::ConnectToTransientLocalStorageArea";
    case MessageName::StorageManagerSet_ConnectToSessionStorageArea:
        return "StorageManagerSet::ConnectToSessionStorageArea";
    case MessageName::StorageManagerSet_DisconnectFromStorageArea:
        return "StorageManagerSet::DisconnectFromStorageArea";
    case MessageName::StorageManagerSet_GetValues:
        return "StorageManagerSet::GetValues";
    case MessageName::StorageManagerSet_CloneSessionStorageNamespace:
        return "StorageManagerSet::CloneSessionStorageNamespace";
    case MessageName::StorageManagerSet_SetItem:
        return "StorageManagerSet::SetItem";
    case MessageName::StorageManagerSet_RemoveItem:
        return "StorageManagerSet::RemoveItem";
    case MessageName::StorageManagerSet_Clear:
        return "StorageManagerSet::Clear";
    case MessageName::CacheStorageEngineConnection_Reference:
        return "CacheStorageEngineConnection::Reference";
    case MessageName::CacheStorageEngineConnection_Dereference:
        return "CacheStorageEngineConnection::Dereference";
    case MessageName::CacheStorageEngineConnection_Open:
        return "CacheStorageEngineConnection::Open";
    case MessageName::CacheStorageEngineConnection_OpenReply:
        return "CacheStorageEngineConnection::OpenReply";
    case MessageName::CacheStorageEngineConnection_Remove:
        return "CacheStorageEngineConnection::Remove";
    case MessageName::CacheStorageEngineConnection_RemoveReply:
        return "CacheStorageEngineConnection::RemoveReply";
    case MessageName::CacheStorageEngineConnection_Caches:
        return "CacheStorageEngineConnection::Caches";
    case MessageName::CacheStorageEngineConnection_CachesReply:
        return "CacheStorageEngineConnection::CachesReply";
    case MessageName::CacheStorageEngineConnection_RetrieveRecords:
        return "CacheStorageEngineConnection::RetrieveRecords";
    case MessageName::CacheStorageEngineConnection_RetrieveRecordsReply:
        return "CacheStorageEngineConnection::RetrieveRecordsReply";
    case MessageName::CacheStorageEngineConnection_DeleteMatchingRecords:
        return "CacheStorageEngineConnection::DeleteMatchingRecords";
    case MessageName::CacheStorageEngineConnection_DeleteMatchingRecordsReply:
        return "CacheStorageEngineConnection::DeleteMatchingRecordsReply";
    case MessageName::CacheStorageEngineConnection_PutRecords:
        return "CacheStorageEngineConnection::PutRecords";
    case MessageName::CacheStorageEngineConnection_PutRecordsReply:
        return "CacheStorageEngineConnection::PutRecordsReply";
    case MessageName::CacheStorageEngineConnection_ClearMemoryRepresentation:
        return "CacheStorageEngineConnection::ClearMemoryRepresentation";
    case MessageName::CacheStorageEngineConnection_ClearMemoryRepresentationReply:
        return "CacheStorageEngineConnection::ClearMemoryRepresentationReply";
    case MessageName::CacheStorageEngineConnection_EngineRepresentation:
        return "CacheStorageEngineConnection::EngineRepresentation";
    case MessageName::CacheStorageEngineConnection_EngineRepresentationReply:
        return "CacheStorageEngineConnection::EngineRepresentationReply";
    case MessageName::NetworkMDNSRegister_UnregisterMDNSNames:
        return "NetworkMDNSRegister::UnregisterMDNSNames";
    case MessageName::NetworkMDNSRegister_RegisterMDNSName:
        return "NetworkMDNSRegister::RegisterMDNSName";
    case MessageName::NetworkRTCMonitor_StartUpdatingIfNeeded:
        return "NetworkRTCMonitor::StartUpdatingIfNeeded";
    case MessageName::NetworkRTCMonitor_StopUpdating:
        return "NetworkRTCMonitor::StopUpdating";
    case MessageName::NetworkRTCProvider_CreateUDPSocket:
        return "NetworkRTCProvider::CreateUDPSocket";
    case MessageName::NetworkRTCProvider_CreateServerTCPSocket:
        return "NetworkRTCProvider::CreateServerTCPSocket";
    case MessageName::NetworkRTCProvider_CreateClientTCPSocket:
        return "NetworkRTCProvider::CreateClientTCPSocket";
    case MessageName::NetworkRTCProvider_WrapNewTCPConnection:
        return "NetworkRTCProvider::WrapNewTCPConnection";
    case MessageName::NetworkRTCProvider_CreateResolver:
        return "NetworkRTCProvider::CreateResolver";
    case MessageName::NetworkRTCProvider_StopResolver:
        return "NetworkRTCProvider::StopResolver";
    case MessageName::NetworkRTCSocket_SendTo:
        return "NetworkRTCSocket::SendTo";
    case MessageName::NetworkRTCSocket_Close:
        return "NetworkRTCSocket::Close";
    case MessageName::NetworkRTCSocket_SetOption:
        return "NetworkRTCSocket::SetOption";
    case MessageName::PluginControllerProxy_GeometryDidChange:
        return "PluginControllerProxy::GeometryDidChange";
    case MessageName::PluginControllerProxy_VisibilityDidChange:
        return "PluginControllerProxy::VisibilityDidChange";
    case MessageName::PluginControllerProxy_FrameDidFinishLoading:
        return "PluginControllerProxy::FrameDidFinishLoading";
    case MessageName::PluginControllerProxy_FrameDidFail:
        return "PluginControllerProxy::FrameDidFail";
    case MessageName::PluginControllerProxy_DidEvaluateJavaScript:
        return "PluginControllerProxy::DidEvaluateJavaScript";
    case MessageName::PluginControllerProxy_StreamWillSendRequest:
        return "PluginControllerProxy::StreamWillSendRequest";
    case MessageName::PluginControllerProxy_StreamDidReceiveResponse:
        return "PluginControllerProxy::StreamDidReceiveResponse";
    case MessageName::PluginControllerProxy_StreamDidReceiveData:
        return "PluginControllerProxy::StreamDidReceiveData";
    case MessageName::PluginControllerProxy_StreamDidFinishLoading:
        return "PluginControllerProxy::StreamDidFinishLoading";
    case MessageName::PluginControllerProxy_StreamDidFail:
        return "PluginControllerProxy::StreamDidFail";
    case MessageName::PluginControllerProxy_ManualStreamDidReceiveResponse:
        return "PluginControllerProxy::ManualStreamDidReceiveResponse";
    case MessageName::PluginControllerProxy_ManualStreamDidReceiveData:
        return "PluginControllerProxy::ManualStreamDidReceiveData";
    case MessageName::PluginControllerProxy_ManualStreamDidFinishLoading:
        return "PluginControllerProxy::ManualStreamDidFinishLoading";
    case MessageName::PluginControllerProxy_ManualStreamDidFail:
        return "PluginControllerProxy::ManualStreamDidFail";
    case MessageName::PluginControllerProxy_HandleMouseEvent:
        return "PluginControllerProxy::HandleMouseEvent";
    case MessageName::PluginControllerProxy_HandleWheelEvent:
        return "PluginControllerProxy::HandleWheelEvent";
    case MessageName::PluginControllerProxy_HandleMouseEnterEvent:
        return "PluginControllerProxy::HandleMouseEnterEvent";
    case MessageName::PluginControllerProxy_HandleMouseLeaveEvent:
        return "PluginControllerProxy::HandleMouseLeaveEvent";
    case MessageName::PluginControllerProxy_HandleKeyboardEvent:
        return "PluginControllerProxy::HandleKeyboardEvent";
    case MessageName::PluginControllerProxy_HandleEditingCommand:
        return "PluginControllerProxy::HandleEditingCommand";
    case MessageName::PluginControllerProxy_IsEditingCommandEnabled:
        return "PluginControllerProxy::IsEditingCommandEnabled";
    case MessageName::PluginControllerProxy_HandlesPageScaleFactor:
        return "PluginControllerProxy::HandlesPageScaleFactor";
    case MessageName::PluginControllerProxy_RequiresUnifiedScaleFactor:
        return "PluginControllerProxy::RequiresUnifiedScaleFactor";
    case MessageName::PluginControllerProxy_SetFocus:
        return "PluginControllerProxy::SetFocus";
    case MessageName::PluginControllerProxy_DidUpdate:
        return "PluginControllerProxy::DidUpdate";
    case MessageName::PluginControllerProxy_PaintEntirePlugin:
        return "PluginControllerProxy::PaintEntirePlugin";
    case MessageName::PluginControllerProxy_GetPluginScriptableNPObject:
        return "PluginControllerProxy::GetPluginScriptableNPObject";
    case MessageName::PluginControllerProxy_WindowFocusChanged:
        return "PluginControllerProxy::WindowFocusChanged";
    case MessageName::PluginControllerProxy_WindowVisibilityChanged:
        return "PluginControllerProxy::WindowVisibilityChanged";
#if PLATFORM(COCOA)
    case MessageName::PluginControllerProxy_SendComplexTextInput:
        return "PluginControllerProxy::SendComplexTextInput";
#endif
#if PLATFORM(COCOA)
    case MessageName::PluginControllerProxy_WindowAndViewFramesChanged:
        return "PluginControllerProxy::WindowAndViewFramesChanged";
#endif
#if PLATFORM(COCOA)
    case MessageName::PluginControllerProxy_SetLayerHostingMode:
        return "PluginControllerProxy::SetLayerHostingMode";
#endif
    case MessageName::PluginControllerProxy_SupportsSnapshotting:
        return "PluginControllerProxy::SupportsSnapshotting";
    case MessageName::PluginControllerProxy_Snapshot:
        return "PluginControllerProxy::Snapshot";
    case MessageName::PluginControllerProxy_StorageBlockingStateChanged:
        return "PluginControllerProxy::StorageBlockingStateChanged";
    case MessageName::PluginControllerProxy_PrivateBrowsingStateChanged:
        return "PluginControllerProxy::PrivateBrowsingStateChanged";
    case MessageName::PluginControllerProxy_GetFormValue:
        return "PluginControllerProxy::GetFormValue";
    case MessageName::PluginControllerProxy_MutedStateChanged:
        return "PluginControllerProxy::MutedStateChanged";
    case MessageName::PluginProcess_InitializePluginProcess:
        return "PluginProcess::InitializePluginProcess";
    case MessageName::PluginProcess_CreateWebProcessConnection:
        return "PluginProcess::CreateWebProcessConnection";
    case MessageName::PluginProcess_GetSitesWithData:
        return "PluginProcess::GetSitesWithData";
    case MessageName::PluginProcess_DeleteWebsiteData:
        return "PluginProcess::DeleteWebsiteData";
    case MessageName::PluginProcess_DeleteWebsiteDataForHostNames:
        return "PluginProcess::DeleteWebsiteDataForHostNames";
#if PLATFORM(COCOA)
    case MessageName::PluginProcess_SetQOS:
        return "PluginProcess::SetQOS";
#endif
    case MessageName::WebProcessConnection_CreatePlugin:
        return "WebProcessConnection::CreatePlugin";
    case MessageName::WebProcessConnection_CreatePluginAsynchronously:
        return "WebProcessConnection::CreatePluginAsynchronously";
    case MessageName::WebProcessConnection_DestroyPlugin:
        return "WebProcessConnection::DestroyPlugin";
    case MessageName::AuxiliaryProcess_ShutDown:
        return "AuxiliaryProcess::ShutDown";
    case MessageName::AuxiliaryProcess_SetProcessSuppressionEnabled:
        return "AuxiliaryProcess::SetProcessSuppressionEnabled";
#if OS(LINUX)
    case MessageName::AuxiliaryProcess_DidReceiveMemoryPressureEvent:
        return "AuxiliaryProcess::DidReceiveMemoryPressureEvent";
#endif
    case MessageName::WebConnection_HandleMessage:
        return "WebConnection::HandleMessage";
    case MessageName::AuthenticationManager_CompleteAuthenticationChallenge:
        return "AuthenticationManager::CompleteAuthenticationChallenge";
    case MessageName::NPObjectMessageReceiver_Deallocate:
        return "NPObjectMessageReceiver::Deallocate";
    case MessageName::NPObjectMessageReceiver_HasMethod:
        return "NPObjectMessageReceiver::HasMethod";
    case MessageName::NPObjectMessageReceiver_Invoke:
        return "NPObjectMessageReceiver::Invoke";
    case MessageName::NPObjectMessageReceiver_InvokeDefault:
        return "NPObjectMessageReceiver::InvokeDefault";
    case MessageName::NPObjectMessageReceiver_HasProperty:
        return "NPObjectMessageReceiver::HasProperty";
    case MessageName::NPObjectMessageReceiver_GetProperty:
        return "NPObjectMessageReceiver::GetProperty";
    case MessageName::NPObjectMessageReceiver_SetProperty:
        return "NPObjectMessageReceiver::SetProperty";
    case MessageName::NPObjectMessageReceiver_RemoveProperty:
        return "NPObjectMessageReceiver::RemoveProperty";
    case MessageName::NPObjectMessageReceiver_Enumerate:
        return "NPObjectMessageReceiver::Enumerate";
    case MessageName::NPObjectMessageReceiver_Construct:
        return "NPObjectMessageReceiver::Construct";
    case MessageName::DrawingAreaProxy_EnterAcceleratedCompositingMode:
        return "DrawingAreaProxy::EnterAcceleratedCompositingMode";
    case MessageName::DrawingAreaProxy_UpdateAcceleratedCompositingMode:
        return "DrawingAreaProxy::UpdateAcceleratedCompositingMode";
    case MessageName::DrawingAreaProxy_DidFirstLayerFlush:
        return "DrawingAreaProxy::DidFirstLayerFlush";
    case MessageName::DrawingAreaProxy_DispatchPresentationCallbacksAfterFlushingLayers:
        return "DrawingAreaProxy::DispatchPresentationCallbacksAfterFlushingLayers";
#if PLATFORM(COCOA)
    case MessageName::DrawingAreaProxy_DidUpdateGeometry:
        return "DrawingAreaProxy::DidUpdateGeometry";
#endif
#if USE(COORDINATED_GRAPHICS) || USE(TEXTURE_MAPPER)
    case MessageName::DrawingAreaProxy_Update:
        return "DrawingAreaProxy::Update";
#endif
#if USE(COORDINATED_GRAPHICS) || USE(TEXTURE_MAPPER)
    case MessageName::DrawingAreaProxy_DidUpdateBackingStoreState:
        return "DrawingAreaProxy::DidUpdateBackingStoreState";
#endif
#if USE(COORDINATED_GRAPHICS) || USE(TEXTURE_MAPPER)
    case MessageName::DrawingAreaProxy_ExitAcceleratedCompositingMode:
        return "DrawingAreaProxy::ExitAcceleratedCompositingMode";
#endif
    case MessageName::VisitedLinkStore_AddVisitedLinkHashFromPage:
        return "VisitedLinkStore::AddVisitedLinkHashFromPage";
    case MessageName::WebCookieManagerProxy_CookiesDidChange:
        return "WebCookieManagerProxy::CookiesDidChange";
    case MessageName::WebFullScreenManagerProxy_SupportsFullScreen:
        return "WebFullScreenManagerProxy::SupportsFullScreen";
    case MessageName::WebFullScreenManagerProxy_EnterFullScreen:
        return "WebFullScreenManagerProxy::EnterFullScreen";
    case MessageName::WebFullScreenManagerProxy_ExitFullScreen:
        return "WebFullScreenManagerProxy::ExitFullScreen";
    case MessageName::WebFullScreenManagerProxy_BeganEnterFullScreen:
        return "WebFullScreenManagerProxy::BeganEnterFullScreen";
    case MessageName::WebFullScreenManagerProxy_BeganExitFullScreen:
        return "WebFullScreenManagerProxy::BeganExitFullScreen";
    case MessageName::WebFullScreenManagerProxy_Close:
        return "WebFullScreenManagerProxy::Close";
    case MessageName::WebGeolocationManagerProxy_StartUpdating:
        return "WebGeolocationManagerProxy::StartUpdating";
    case MessageName::WebGeolocationManagerProxy_StopUpdating:
        return "WebGeolocationManagerProxy::StopUpdating";
    case MessageName::WebGeolocationManagerProxy_SetEnableHighAccuracy:
        return "WebGeolocationManagerProxy::SetEnableHighAccuracy";
    case MessageName::WebPageProxy_CreateNewPage:
        return "WebPageProxy::CreateNewPage";
    case MessageName::WebPageProxy_ShowPage:
        return "WebPageProxy::ShowPage";
    case MessageName::WebPageProxy_ClosePage:
        return "WebPageProxy::ClosePage";
    case MessageName::WebPageProxy_RunJavaScriptAlert:
        return "WebPageProxy::RunJavaScriptAlert";
    case MessageName::WebPageProxy_RunJavaScriptConfirm:
        return "WebPageProxy::RunJavaScriptConfirm";
    case MessageName::WebPageProxy_RunJavaScriptPrompt:
        return "WebPageProxy::RunJavaScriptPrompt";
    case MessageName::WebPageProxy_MouseDidMoveOverElement:
        return "WebPageProxy::MouseDidMoveOverElement";
#if ENABLE(NETSCAPE_PLUGIN_API)
    case MessageName::WebPageProxy_UnavailablePluginButtonClicked:
        return "WebPageProxy::UnavailablePluginButtonClicked";
#endif
#if ENABLE(WEBGL)
    case MessageName::WebPageProxy_WebGLPolicyForURL:
        return "WebPageProxy::WebGLPolicyForURL";
#endif
#if ENABLE(WEBGL)
    case MessageName::WebPageProxy_ResolveWebGLPolicyForURL:
        return "WebPageProxy::ResolveWebGLPolicyForURL";
#endif
    case MessageName::WebPageProxy_DidChangeViewportProperties:
        return "WebPageProxy::DidChangeViewportProperties";
    case MessageName::WebPageProxy_DidReceiveEvent:
        return "WebPageProxy::DidReceiveEvent";
    case MessageName::WebPageProxy_SetCursor:
        return "WebPageProxy::SetCursor";
    case MessageName::WebPageProxy_SetCursorHiddenUntilMouseMoves:
        return "WebPageProxy::SetCursorHiddenUntilMouseMoves";
    case MessageName::WebPageProxy_SetStatusText:
        return "WebPageProxy::SetStatusText";
    case MessageName::WebPageProxy_SetFocus:
        return "WebPageProxy::SetFocus";
    case MessageName::WebPageProxy_TakeFocus:
        return "WebPageProxy::TakeFocus";
    case MessageName::WebPageProxy_FocusedFrameChanged:
        return "WebPageProxy::FocusedFrameChanged";
    case MessageName::WebPageProxy_SetRenderTreeSize:
        return "WebPageProxy::SetRenderTreeSize";
    case MessageName::WebPageProxy_SetToolbarsAreVisible:
        return "WebPageProxy::SetToolbarsAreVisible";
    case MessageName::WebPageProxy_GetToolbarsAreVisible:
        return "WebPageProxy::GetToolbarsAreVisible";
    case MessageName::WebPageProxy_SetMenuBarIsVisible:
        return "WebPageProxy::SetMenuBarIsVisible";
    case MessageName::WebPageProxy_GetMenuBarIsVisible:
        return "WebPageProxy::GetMenuBarIsVisible";
    case MessageName::WebPageProxy_SetStatusBarIsVisible:
        return "WebPageProxy::SetStatusBarIsVisible";
    case MessageName::WebPageProxy_GetStatusBarIsVisible:
        return "WebPageProxy::GetStatusBarIsVisible";
    case MessageName::WebPageProxy_SetIsResizable:
        return "WebPageProxy::SetIsResizable";
    case MessageName::WebPageProxy_SetWindowFrame:
        return "WebPageProxy::SetWindowFrame";
    case MessageName::WebPageProxy_GetWindowFrame:
        return "WebPageProxy::GetWindowFrame";
    case MessageName::WebPageProxy_ScreenToRootView:
        return "WebPageProxy::ScreenToRootView";
    case MessageName::WebPageProxy_RootViewToScreen:
        return "WebPageProxy::RootViewToScreen";
    case MessageName::WebPageProxy_AccessibilityScreenToRootView:
        return "WebPageProxy::AccessibilityScreenToRootView";
    case MessageName::WebPageProxy_RootViewToAccessibilityScreen:
        return "WebPageProxy::RootViewToAccessibilityScreen";
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_ShowValidationMessage:
        return "WebPageProxy::ShowValidationMessage";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_HideValidationMessage:
        return "WebPageProxy::HideValidationMessage";
#endif
    case MessageName::WebPageProxy_RunBeforeUnloadConfirmPanel:
        return "WebPageProxy::RunBeforeUnloadConfirmPanel";
    case MessageName::WebPageProxy_PageDidScroll:
        return "WebPageProxy::PageDidScroll";
    case MessageName::WebPageProxy_RunOpenPanel:
        return "WebPageProxy::RunOpenPanel";
    case MessageName::WebPageProxy_ShowShareSheet:
        return "WebPageProxy::ShowShareSheet";
    case MessageName::WebPageProxy_ShowShareSheetReply:
        return "WebPageProxy::ShowShareSheetReply";
    case MessageName::WebPageProxy_PrintFrame:
        return "WebPageProxy::PrintFrame";
    case MessageName::WebPageProxy_RunModal:
        return "WebPageProxy::RunModal";
    case MessageName::WebPageProxy_NotifyScrollerThumbIsVisibleInRect:
        return "WebPageProxy::NotifyScrollerThumbIsVisibleInRect";
    case MessageName::WebPageProxy_RecommendedScrollbarStyleDidChange:
        return "WebPageProxy::RecommendedScrollbarStyleDidChange";
    case MessageName::WebPageProxy_DidChangeScrollbarsForMainFrame:
        return "WebPageProxy::DidChangeScrollbarsForMainFrame";
    case MessageName::WebPageProxy_DidChangeScrollOffsetPinningForMainFrame:
        return "WebPageProxy::DidChangeScrollOffsetPinningForMainFrame";
    case MessageName::WebPageProxy_DidChangePageCount:
        return "WebPageProxy::DidChangePageCount";
    case MessageName::WebPageProxy_PageExtendedBackgroundColorDidChange:
        return "WebPageProxy::PageExtendedBackgroundColorDidChange";
#if ENABLE(NETSCAPE_PLUGIN_API)
    case MessageName::WebPageProxy_DidFailToInitializePlugin:
        return "WebPageProxy::DidFailToInitializePlugin";
#endif
#if ENABLE(NETSCAPE_PLUGIN_API)
    case MessageName::WebPageProxy_DidBlockInsecurePluginVersion:
        return "WebPageProxy::DidBlockInsecurePluginVersion";
#endif
    case MessageName::WebPageProxy_SetCanShortCircuitHorizontalWheelEvents:
        return "WebPageProxy::SetCanShortCircuitHorizontalWheelEvents";
    case MessageName::WebPageProxy_DidChangeContentSize:
        return "WebPageProxy::DidChangeContentSize";
    case MessageName::WebPageProxy_DidChangeIntrinsicContentSize:
        return "WebPageProxy::DidChangeIntrinsicContentSize";
#if ENABLE(INPUT_TYPE_COLOR)
    case MessageName::WebPageProxy_ShowColorPicker:
        return "WebPageProxy::ShowColorPicker";
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    case MessageName::WebPageProxy_SetColorPickerColor:
        return "WebPageProxy::SetColorPickerColor";
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    case MessageName::WebPageProxy_EndColorPicker:
        return "WebPageProxy::EndColorPicker";
#endif
#if ENABLE(DATALIST_ELEMENT)
    case MessageName::WebPageProxy_ShowDataListSuggestions:
        return "WebPageProxy::ShowDataListSuggestions";
#endif
#if ENABLE(DATALIST_ELEMENT)
    case MessageName::WebPageProxy_HandleKeydownInDataList:
        return "WebPageProxy::HandleKeydownInDataList";
#endif
#if ENABLE(DATALIST_ELEMENT)
    case MessageName::WebPageProxy_EndDataListSuggestions:
        return "WebPageProxy::EndDataListSuggestions";
#endif
    case MessageName::WebPageProxy_DecidePolicyForResponse:
        return "WebPageProxy::DecidePolicyForResponse";
    case MessageName::WebPageProxy_DecidePolicyForNavigationActionAsync:
        return "WebPageProxy::DecidePolicyForNavigationActionAsync";
    case MessageName::WebPageProxy_DecidePolicyForNavigationActionSync:
        return "WebPageProxy::DecidePolicyForNavigationActionSync";
    case MessageName::WebPageProxy_DecidePolicyForNewWindowAction:
        return "WebPageProxy::DecidePolicyForNewWindowAction";
    case MessageName::WebPageProxy_UnableToImplementPolicy:
        return "WebPageProxy::UnableToImplementPolicy";
    case MessageName::WebPageProxy_DidChangeProgress:
        return "WebPageProxy::DidChangeProgress";
    case MessageName::WebPageProxy_DidFinishProgress:
        return "WebPageProxy::DidFinishProgress";
    case MessageName::WebPageProxy_DidStartProgress:
        return "WebPageProxy::DidStartProgress";
    case MessageName::WebPageProxy_SetNetworkRequestsInProgress:
        return "WebPageProxy::SetNetworkRequestsInProgress";
    case MessageName::WebPageProxy_DidCreateMainFrame:
        return "WebPageProxy::DidCreateMainFrame";
    case MessageName::WebPageProxy_DidCreateSubframe:
        return "WebPageProxy::DidCreateSubframe";
    case MessageName::WebPageProxy_DidCreateWindow:
        return "WebPageProxy::DidCreateWindow";
    case MessageName::WebPageProxy_DidStartProvisionalLoadForFrame:
        return "WebPageProxy::DidStartProvisionalLoadForFrame";
    case MessageName::WebPageProxy_DidReceiveServerRedirectForProvisionalLoadForFrame:
        return "WebPageProxy::DidReceiveServerRedirectForProvisionalLoadForFrame";
    case MessageName::WebPageProxy_WillPerformClientRedirectForFrame:
        return "WebPageProxy::WillPerformClientRedirectForFrame";
    case MessageName::WebPageProxy_DidCancelClientRedirectForFrame:
        return "WebPageProxy::DidCancelClientRedirectForFrame";
    case MessageName::WebPageProxy_DidChangeProvisionalURLForFrame:
        return "WebPageProxy::DidChangeProvisionalURLForFrame";
    case MessageName::WebPageProxy_DidFailProvisionalLoadForFrame:
        return "WebPageProxy::DidFailProvisionalLoadForFrame";
    case MessageName::WebPageProxy_DidCommitLoadForFrame:
        return "WebPageProxy::DidCommitLoadForFrame";
    case MessageName::WebPageProxy_DidFailLoadForFrame:
        return "WebPageProxy::DidFailLoadForFrame";
    case MessageName::WebPageProxy_DidFinishDocumentLoadForFrame:
        return "WebPageProxy::DidFinishDocumentLoadForFrame";
    case MessageName::WebPageProxy_DidFinishLoadForFrame:
        return "WebPageProxy::DidFinishLoadForFrame";
    case MessageName::WebPageProxy_DidFirstLayoutForFrame:
        return "WebPageProxy::DidFirstLayoutForFrame";
    case MessageName::WebPageProxy_DidFirstVisuallyNonEmptyLayoutForFrame:
        return "WebPageProxy::DidFirstVisuallyNonEmptyLayoutForFrame";
    case MessageName::WebPageProxy_DidReachLayoutMilestone:
        return "WebPageProxy::DidReachLayoutMilestone";
    case MessageName::WebPageProxy_DidReceiveTitleForFrame:
        return "WebPageProxy::DidReceiveTitleForFrame";
    case MessageName::WebPageProxy_DidDisplayInsecureContentForFrame:
        return "WebPageProxy::DidDisplayInsecureContentForFrame";
    case MessageName::WebPageProxy_DidRunInsecureContentForFrame:
        return "WebPageProxy::DidRunInsecureContentForFrame";
    case MessageName::WebPageProxy_DidDetectXSSForFrame:
        return "WebPageProxy::DidDetectXSSForFrame";
    case MessageName::WebPageProxy_DidSameDocumentNavigationForFrame:
        return "WebPageProxy::DidSameDocumentNavigationForFrame";
    case MessageName::WebPageProxy_DidChangeMainDocument:
        return "WebPageProxy::DidChangeMainDocument";
    case MessageName::WebPageProxy_DidExplicitOpenForFrame:
        return "WebPageProxy::DidExplicitOpenForFrame";
    case MessageName::WebPageProxy_DidDestroyNavigation:
        return "WebPageProxy::DidDestroyNavigation";
    case MessageName::WebPageProxy_MainFramePluginHandlesPageScaleGestureDidChange:
        return "WebPageProxy::MainFramePluginHandlesPageScaleGestureDidChange";
    case MessageName::WebPageProxy_DidNavigateWithNavigationData:
        return "WebPageProxy::DidNavigateWithNavigationData";
    case MessageName::WebPageProxy_DidPerformClientRedirect:
        return "WebPageProxy::DidPerformClientRedirect";
    case MessageName::WebPageProxy_DidPerformServerRedirect:
        return "WebPageProxy::DidPerformServerRedirect";
    case MessageName::WebPageProxy_DidUpdateHistoryTitle:
        return "WebPageProxy::DidUpdateHistoryTitle";
    case MessageName::WebPageProxy_DidFinishLoadingDataForCustomContentProvider:
        return "WebPageProxy::DidFinishLoadingDataForCustomContentProvider";
    case MessageName::WebPageProxy_WillSubmitForm:
        return "WebPageProxy::WillSubmitForm";
    case MessageName::WebPageProxy_VoidCallback:
        return "WebPageProxy::VoidCallback";
    case MessageName::WebPageProxy_DataCallback:
        return "WebPageProxy::DataCallback";
    case MessageName::WebPageProxy_ImageCallback:
        return "WebPageProxy::ImageCallback";
    case MessageName::WebPageProxy_StringCallback:
        return "WebPageProxy::StringCallback";
    case MessageName::WebPageProxy_BoolCallback:
        return "WebPageProxy::BoolCallback";
    case MessageName::WebPageProxy_InvalidateStringCallback:
        return "WebPageProxy::InvalidateStringCallback";
    case MessageName::WebPageProxy_ScriptValueCallback:
        return "WebPageProxy::ScriptValueCallback";
    case MessageName::WebPageProxy_ComputedPagesCallback:
        return "WebPageProxy::ComputedPagesCallback";
    case MessageName::WebPageProxy_ValidateCommandCallback:
        return "WebPageProxy::ValidateCommandCallback";
    case MessageName::WebPageProxy_EditingRangeCallback:
        return "WebPageProxy::EditingRangeCallback";
    case MessageName::WebPageProxy_UnsignedCallback:
        return "WebPageProxy::UnsignedCallback";
    case MessageName::WebPageProxy_RectForCharacterRangeCallback:
        return "WebPageProxy::RectForCharacterRangeCallback";
#if ENABLE(APPLICATION_MANIFEST)
    case MessageName::WebPageProxy_ApplicationManifestCallback:
        return "WebPageProxy::ApplicationManifestCallback";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPageProxy_AttributedStringForCharacterRangeCallback:
        return "WebPageProxy::AttributedStringForCharacterRangeCallback";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPageProxy_FontAtSelectionCallback:
        return "WebPageProxy::FontAtSelectionCallback";
#endif
    case MessageName::WebPageProxy_FontAttributesCallback:
        return "WebPageProxy::FontAttributesCallback";
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_GestureCallback:
        return "WebPageProxy::GestureCallback";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_TouchesCallback:
        return "WebPageProxy::TouchesCallback";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_SelectionContextCallback:
        return "WebPageProxy::SelectionContextCallback";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_InterpretKeyEvent:
        return "WebPageProxy::InterpretKeyEvent";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_DidReceivePositionInformation:
        return "WebPageProxy::DidReceivePositionInformation";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_SaveImageToLibrary:
        return "WebPageProxy::SaveImageToLibrary";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_ShowPlaybackTargetPicker:
        return "WebPageProxy::ShowPlaybackTargetPicker";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_CommitPotentialTapFailed:
        return "WebPageProxy::CommitPotentialTapFailed";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_DidNotHandleTapAsClick:
        return "WebPageProxy::DidNotHandleTapAsClick";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_DidCompleteSyntheticClick:
        return "WebPageProxy::DidCompleteSyntheticClick";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_DisableDoubleTapGesturesDuringTapIfNecessary:
        return "WebPageProxy::DisableDoubleTapGesturesDuringTapIfNecessary";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_HandleSmartMagnificationInformationForPotentialTap:
        return "WebPageProxy::HandleSmartMagnificationInformationForPotentialTap";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_SelectionRectsCallback:
        return "WebPageProxy::SelectionRectsCallback";
#endif
#if ENABLE(DATA_DETECTION)
    case MessageName::WebPageProxy_SetDataDetectionResult:
        return "WebPageProxy::SetDataDetectionResult";
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPageProxy_PrintFinishedCallback:
        return "WebPageProxy::PrintFinishedCallback";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_DrawToPDFCallback:
        return "WebPageProxy::DrawToPDFCallback";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_NowPlayingInfoCallback:
        return "WebPageProxy::NowPlayingInfoCallback";
#endif
    case MessageName::WebPageProxy_FindStringCallback:
        return "WebPageProxy::FindStringCallback";
    case MessageName::WebPageProxy_PageScaleFactorDidChange:
        return "WebPageProxy::PageScaleFactorDidChange";
    case MessageName::WebPageProxy_PluginScaleFactorDidChange:
        return "WebPageProxy::PluginScaleFactorDidChange";
    case MessageName::WebPageProxy_PluginZoomFactorDidChange:
        return "WebPageProxy::PluginZoomFactorDidChange";
#if USE(ATK)
    case MessageName::WebPageProxy_BindAccessibilityTree:
        return "WebPageProxy::BindAccessibilityTree";
#endif
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    case MessageName::WebPageProxy_SetInputMethodState:
        return "WebPageProxy::SetInputMethodState";
#endif
    case MessageName::WebPageProxy_BackForwardAddItem:
        return "WebPageProxy::BackForwardAddItem";
    case MessageName::WebPageProxy_BackForwardGoToItem:
        return "WebPageProxy::BackForwardGoToItem";
    case MessageName::WebPageProxy_BackForwardItemAtIndex:
        return "WebPageProxy::BackForwardItemAtIndex";
    case MessageName::WebPageProxy_BackForwardListCounts:
        return "WebPageProxy::BackForwardListCounts";
    case MessageName::WebPageProxy_BackForwardClear:
        return "WebPageProxy::BackForwardClear";
    case MessageName::WebPageProxy_WillGoToBackForwardListItem:
        return "WebPageProxy::WillGoToBackForwardListItem";
    case MessageName::WebPageProxy_RegisterEditCommandForUndo:
        return "WebPageProxy::RegisterEditCommandForUndo";
    case MessageName::WebPageProxy_ClearAllEditCommands:
        return "WebPageProxy::ClearAllEditCommands";
    case MessageName::WebPageProxy_RegisterInsertionUndoGrouping:
        return "WebPageProxy::RegisterInsertionUndoGrouping";
    case MessageName::WebPageProxy_CanUndoRedo:
        return "WebPageProxy::CanUndoRedo";
    case MessageName::WebPageProxy_ExecuteUndoRedo:
        return "WebPageProxy::ExecuteUndoRedo";
    case MessageName::WebPageProxy_LogDiagnosticMessage:
        return "WebPageProxy::LogDiagnosticMessage";
    case MessageName::WebPageProxy_LogDiagnosticMessageWithResult:
        return "WebPageProxy::LogDiagnosticMessageWithResult";
    case MessageName::WebPageProxy_LogDiagnosticMessageWithValue:
        return "WebPageProxy::LogDiagnosticMessageWithValue";
    case MessageName::WebPageProxy_LogDiagnosticMessageWithEnhancedPrivacy:
        return "WebPageProxy::LogDiagnosticMessageWithEnhancedPrivacy";
    case MessageName::WebPageProxy_LogDiagnosticMessageWithValueDictionary:
        return "WebPageProxy::LogDiagnosticMessageWithValueDictionary";
    case MessageName::WebPageProxy_LogScrollingEvent:
        return "WebPageProxy::LogScrollingEvent";
    case MessageName::WebPageProxy_EditorStateChanged:
        return "WebPageProxy::EditorStateChanged";
    case MessageName::WebPageProxy_CompositionWasCanceled:
        return "WebPageProxy::CompositionWasCanceled";
    case MessageName::WebPageProxy_SetHasHadSelectionChangesFromUserInteraction:
        return "WebPageProxy::SetHasHadSelectionChangesFromUserInteraction";
#if HAVE(TOUCH_BAR)
    case MessageName::WebPageProxy_SetIsTouchBarUpdateSupressedForHiddenContentEditable:
        return "WebPageProxy::SetIsTouchBarUpdateSupressedForHiddenContentEditable";
#endif
#if HAVE(TOUCH_BAR)
    case MessageName::WebPageProxy_SetIsNeverRichlyEditableForTouchBar:
        return "WebPageProxy::SetIsNeverRichlyEditableForTouchBar";
#endif
    case MessageName::WebPageProxy_RequestDOMPasteAccess:
        return "WebPageProxy::RequestDOMPasteAccess";
    case MessageName::WebPageProxy_DidCountStringMatches:
        return "WebPageProxy::DidCountStringMatches";
    case MessageName::WebPageProxy_SetTextIndicator:
        return "WebPageProxy::SetTextIndicator";
    case MessageName::WebPageProxy_ClearTextIndicator:
        return "WebPageProxy::ClearTextIndicator";
    case MessageName::WebPageProxy_DidFindString:
        return "WebPageProxy::DidFindString";
    case MessageName::WebPageProxy_DidFailToFindString:
        return "WebPageProxy::DidFailToFindString";
    case MessageName::WebPageProxy_DidFindStringMatches:
        return "WebPageProxy::DidFindStringMatches";
    case MessageName::WebPageProxy_DidGetImageForFindMatch:
        return "WebPageProxy::DidGetImageForFindMatch";
    case MessageName::WebPageProxy_ShowPopupMenu:
        return "WebPageProxy::ShowPopupMenu";
    case MessageName::WebPageProxy_HidePopupMenu:
        return "WebPageProxy::HidePopupMenu";
#if ENABLE(CONTEXT_MENUS)
    case MessageName::WebPageProxy_ShowContextMenu:
        return "WebPageProxy::ShowContextMenu";
#endif
    case MessageName::WebPageProxy_ExceededDatabaseQuota:
        return "WebPageProxy::ExceededDatabaseQuota";
    case MessageName::WebPageProxy_ReachedApplicationCacheOriginQuota:
        return "WebPageProxy::ReachedApplicationCacheOriginQuota";
    case MessageName::WebPageProxy_RequestGeolocationPermissionForFrame:
        return "WebPageProxy::RequestGeolocationPermissionForFrame";
    case MessageName::WebPageProxy_RevokeGeolocationAuthorizationToken:
        return "WebPageProxy::RevokeGeolocationAuthorizationToken";
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebPageProxy_RequestUserMediaPermissionForFrame:
        return "WebPageProxy::RequestUserMediaPermissionForFrame";
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebPageProxy_EnumerateMediaDevicesForFrame:
        return "WebPageProxy::EnumerateMediaDevicesForFrame";
    case MessageName::WebPageProxy_EnumerateMediaDevicesForFrameReply:
        return "WebPageProxy::EnumerateMediaDevicesForFrameReply";
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebPageProxy_BeginMonitoringCaptureDevices:
        return "WebPageProxy::BeginMonitoringCaptureDevices";
#endif
    case MessageName::WebPageProxy_RequestNotificationPermission:
        return "WebPageProxy::RequestNotificationPermission";
    case MessageName::WebPageProxy_ShowNotification:
        return "WebPageProxy::ShowNotification";
    case MessageName::WebPageProxy_CancelNotification:
        return "WebPageProxy::CancelNotification";
    case MessageName::WebPageProxy_ClearNotifications:
        return "WebPageProxy::ClearNotifications";
    case MessageName::WebPageProxy_DidDestroyNotification:
        return "WebPageProxy::DidDestroyNotification";
#if USE(UNIFIED_TEXT_CHECKING)
    case MessageName::WebPageProxy_CheckTextOfParagraph:
        return "WebPageProxy::CheckTextOfParagraph";
#endif
    case MessageName::WebPageProxy_CheckSpellingOfString:
        return "WebPageProxy::CheckSpellingOfString";
    case MessageName::WebPageProxy_CheckGrammarOfString:
        return "WebPageProxy::CheckGrammarOfString";
    case MessageName::WebPageProxy_SpellingUIIsShowing:
        return "WebPageProxy::SpellingUIIsShowing";
    case MessageName::WebPageProxy_UpdateSpellingUIWithMisspelledWord:
        return "WebPageProxy::UpdateSpellingUIWithMisspelledWord";
    case MessageName::WebPageProxy_UpdateSpellingUIWithGrammarString:
        return "WebPageProxy::UpdateSpellingUIWithGrammarString";
    case MessageName::WebPageProxy_GetGuessesForWord:
        return "WebPageProxy::GetGuessesForWord";
    case MessageName::WebPageProxy_LearnWord:
        return "WebPageProxy::LearnWord";
    case MessageName::WebPageProxy_IgnoreWord:
        return "WebPageProxy::IgnoreWord";
    case MessageName::WebPageProxy_RequestCheckingOfString:
        return "WebPageProxy::RequestCheckingOfString";
#if ENABLE(DRAG_SUPPORT)
    case MessageName::WebPageProxy_DidPerformDragControllerAction:
        return "WebPageProxy::DidPerformDragControllerAction";
#endif
#if ENABLE(DRAG_SUPPORT)
    case MessageName::WebPageProxy_DidEndDragging:
        return "WebPageProxy::DidEndDragging";
#endif
#if PLATFORM(COCOA) && ENABLE(DRAG_SUPPORT)
    case MessageName::WebPageProxy_StartDrag:
        return "WebPageProxy::StartDrag";
#endif
#if PLATFORM(COCOA) && ENABLE(DRAG_SUPPORT)
    case MessageName::WebPageProxy_SetPromisedDataForImage:
        return "WebPageProxy::SetPromisedDataForImage";
#endif
#if (PLATFORM(GTK) || PLATFORM(HBD)) && ENABLE(DRAG_SUPPORT)
    case MessageName::WebPageProxy_StartDrag:
        return "WebPageProxy::StartDrag";
#endif
#if ENABLE(DRAG_SUPPORT)
    case MessageName::WebPageProxy_DidPerformDragOperation:
        return "WebPageProxy::DidPerformDragOperation";
#endif
#if ENABLE(DATA_INTERACTION)
    case MessageName::WebPageProxy_DidHandleDragStartRequest:
        return "WebPageProxy::DidHandleDragStartRequest";
#endif
#if ENABLE(DATA_INTERACTION)
    case MessageName::WebPageProxy_DidHandleAdditionalDragItemsRequest:
        return "WebPageProxy::DidHandleAdditionalDragItemsRequest";
#endif
#if ENABLE(DATA_INTERACTION)
    case MessageName::WebPageProxy_WillReceiveEditDragSnapshot:
        return "WebPageProxy::WillReceiveEditDragSnapshot";
#endif
#if ENABLE(DATA_INTERACTION)
    case MessageName::WebPageProxy_DidReceiveEditDragSnapshot:
        return "WebPageProxy::DidReceiveEditDragSnapshot";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_DidPerformDictionaryLookup:
        return "WebPageProxy::DidPerformDictionaryLookup";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_ExecuteSavedCommandBySelector:
        return "WebPageProxy::ExecuteSavedCommandBySelector";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_RegisterWebProcessAccessibilityToken:
        return "WebPageProxy::RegisterWebProcessAccessibilityToken";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_PluginFocusOrWindowFocusChanged:
        return "WebPageProxy::PluginFocusOrWindowFocusChanged";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_SetPluginComplexTextInputState:
        return "WebPageProxy::SetPluginComplexTextInputState";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_GetIsSpeaking:
        return "WebPageProxy::GetIsSpeaking";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_Speak:
        return "WebPageProxy::Speak";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_StopSpeaking:
        return "WebPageProxy::StopSpeaking";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_MakeFirstResponder:
        return "WebPageProxy::MakeFirstResponder";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_AssistiveTechnologyMakeFirstResponder:
        return "WebPageProxy::AssistiveTechnologyMakeFirstResponder";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_SearchWithSpotlight:
        return "WebPageProxy::SearchWithSpotlight";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_SearchTheWeb:
        return "WebPageProxy::SearchTheWeb";
#endif
#if HAVE(TOUCH_BAR)
    case MessageName::WebPageProxy_TouchBarMenuDataChanged:
        return "WebPageProxy::TouchBarMenuDataChanged";
#endif
#if HAVE(TOUCH_BAR)
    case MessageName::WebPageProxy_TouchBarMenuItemDataAdded:
        return "WebPageProxy::TouchBarMenuItemDataAdded";
#endif
#if HAVE(TOUCH_BAR)
    case MessageName::WebPageProxy_TouchBarMenuItemDataRemoved:
        return "WebPageProxy::TouchBarMenuItemDataRemoved";
#endif
#if USE(APPKIT)
    case MessageName::WebPageProxy_SubstitutionsPanelIsShowing:
        return "WebPageProxy::SubstitutionsPanelIsShowing";
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    case MessageName::WebPageProxy_toggleSmartInsertDelete:
        return "WebPageProxy::toggleSmartInsertDelete";
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    case MessageName::WebPageProxy_toggleAutomaticQuoteSubstitution:
        return "WebPageProxy::toggleAutomaticQuoteSubstitution";
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    case MessageName::WebPageProxy_toggleAutomaticLinkDetection:
        return "WebPageProxy::toggleAutomaticLinkDetection";
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    case MessageName::WebPageProxy_toggleAutomaticDashSubstitution:
        return "WebPageProxy::toggleAutomaticDashSubstitution";
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    case MessageName::WebPageProxy_toggleAutomaticTextReplacement:
        return "WebPageProxy::toggleAutomaticTextReplacement";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPageProxy_ShowCorrectionPanel:
        return "WebPageProxy::ShowCorrectionPanel";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPageProxy_DismissCorrectionPanel:
        return "WebPageProxy::DismissCorrectionPanel";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPageProxy_DismissCorrectionPanelSoon:
        return "WebPageProxy::DismissCorrectionPanelSoon";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPageProxy_RecordAutocorrectionResponse:
        return "WebPageProxy::RecordAutocorrectionResponse";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPageProxy_SetEditableElementIsFocused:
        return "WebPageProxy::SetEditableElementIsFocused";
#endif
#if USE(DICTATION_ALTERNATIVES)
    case MessageName::WebPageProxy_ShowDictationAlternativeUI:
        return "WebPageProxy::ShowDictationAlternativeUI";
#endif
#if USE(DICTATION_ALTERNATIVES)
    case MessageName::WebPageProxy_RemoveDictationAlternatives:
        return "WebPageProxy::RemoveDictationAlternatives";
#endif
#if USE(DICTATION_ALTERNATIVES)
    case MessageName::WebPageProxy_DictationAlternatives:
        return "WebPageProxy::DictationAlternatives";
#endif
#if PLATFORM(X11) && ENABLE(NETSCAPE_PLUGIN_API)
    case MessageName::WebPageProxy_CreatePluginContainer:
        return "WebPageProxy::CreatePluginContainer";
#endif
#if PLATFORM(X11) && ENABLE(NETSCAPE_PLUGIN_API)
    case MessageName::WebPageProxy_WindowedPluginGeometryDidChange:
        return "WebPageProxy::WindowedPluginGeometryDidChange";
#endif
#if PLATFORM(X11) && ENABLE(NETSCAPE_PLUGIN_API)
    case MessageName::WebPageProxy_WindowedPluginVisibilityDidChange:
        return "WebPageProxy::WindowedPluginVisibilityDidChange";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_CouldNotRestorePageState:
        return "WebPageProxy::CouldNotRestorePageState";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_RestorePageState:
        return "WebPageProxy::RestorePageState";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_RestorePageCenterAndScale:
        return "WebPageProxy::RestorePageCenterAndScale";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_DidGetTapHighlightGeometries:
        return "WebPageProxy::DidGetTapHighlightGeometries";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_ElementDidFocus:
        return "WebPageProxy::ElementDidFocus";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_ElementDidBlur:
        return "WebPageProxy::ElementDidBlur";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_UpdateInputContextAfterBlurringAndRefocusingElement:
        return "WebPageProxy::UpdateInputContextAfterBlurringAndRefocusingElement";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_FocusedElementDidChangeInputMode:
        return "WebPageProxy::FocusedElementDidChangeInputMode";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_ScrollingNodeScrollWillStartScroll:
        return "WebPageProxy::ScrollingNodeScrollWillStartScroll";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_ScrollingNodeScrollDidEndScroll:
        return "WebPageProxy::ScrollingNodeScrollDidEndScroll";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_ShowInspectorHighlight:
        return "WebPageProxy::ShowInspectorHighlight";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_HideInspectorHighlight:
        return "WebPageProxy::HideInspectorHighlight";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_FocusedElementInformationCallback:
        return "WebPageProxy::FocusedElementInformationCallback";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_ShowInspectorIndication:
        return "WebPageProxy::ShowInspectorIndication";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_HideInspectorIndication:
        return "WebPageProxy::HideInspectorIndication";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_EnableInspectorNodeSearch:
        return "WebPageProxy::EnableInspectorNodeSearch";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_DisableInspectorNodeSearch:
        return "WebPageProxy::DisableInspectorNodeSearch";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_UpdateStringForFind:
        return "WebPageProxy::UpdateStringForFind";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_HandleAutocorrectionContext:
        return "WebPageProxy::HandleAutocorrectionContext";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_ShowDataDetectorsUIForPositionInformation:
        return "WebPageProxy::ShowDataDetectorsUIForPositionInformation";
#endif
    case MessageName::WebPageProxy_DidChangeInspectorFrontendCount:
        return "WebPageProxy::DidChangeInspectorFrontendCount";
    case MessageName::WebPageProxy_CreateInspectorTarget:
        return "WebPageProxy::CreateInspectorTarget";
    case MessageName::WebPageProxy_DestroyInspectorTarget:
        return "WebPageProxy::DestroyInspectorTarget";
    case MessageName::WebPageProxy_SendMessageToInspectorFrontend:
        return "WebPageProxy::SendMessageToInspectorFrontend";
    case MessageName::WebPageProxy_SaveRecentSearches:
        return "WebPageProxy::SaveRecentSearches";
    case MessageName::WebPageProxy_LoadRecentSearches:
        return "WebPageProxy::LoadRecentSearches";
    case MessageName::WebPageProxy_SavePDFToFileInDownloadsFolder:
        return "WebPageProxy::SavePDFToFileInDownloadsFolder";
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_SavePDFToTemporaryFolderAndOpenWithNativeApplication:
        return "WebPageProxy::SavePDFToTemporaryFolderAndOpenWithNativeApplication";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_OpenPDFFromTemporaryFolderWithNativeApplication:
        return "WebPageProxy::OpenPDFFromTemporaryFolderWithNativeApplication";
#endif
#if ENABLE(PDFKIT_PLUGIN)
    case MessageName::WebPageProxy_ShowPDFContextMenu:
        return "WebPageProxy::ShowPDFContextMenu";
#endif
#if ENABLE(NETSCAPE_PLUGIN_API)
    case MessageName::WebPageProxy_FindPlugin:
        return "WebPageProxy::FindPlugin";
#endif
    case MessageName::WebPageProxy_DidUpdateActivityState:
        return "WebPageProxy::DidUpdateActivityState";
#if ENABLE(WEB_CRYPTO)
    case MessageName::WebPageProxy_WrapCryptoKey:
        return "WebPageProxy::WrapCryptoKey";
#endif
#if ENABLE(WEB_CRYPTO)
    case MessageName::WebPageProxy_UnwrapCryptoKey:
        return "WebPageProxy::UnwrapCryptoKey";
#endif
#if (ENABLE(TELEPHONE_NUMBER_DETECTION) && PLATFORM(MAC))
    case MessageName::WebPageProxy_ShowTelephoneNumberMenu:
        return "WebPageProxy::ShowTelephoneNumberMenu";
#endif
#if USE(QUICK_LOOK)
    case MessageName::WebPageProxy_DidStartLoadForQuickLookDocumentInMainFrame:
        return "WebPageProxy::DidStartLoadForQuickLookDocumentInMainFrame";
#endif
#if USE(QUICK_LOOK)
    case MessageName::WebPageProxy_DidFinishLoadForQuickLookDocumentInMainFrame:
        return "WebPageProxy::DidFinishLoadForQuickLookDocumentInMainFrame";
#endif
#if USE(QUICK_LOOK)
    case MessageName::WebPageProxy_RequestPasswordForQuickLookDocumentInMainFrame:
        return "WebPageProxy::RequestPasswordForQuickLookDocumentInMainFrame";
    case MessageName::WebPageProxy_RequestPasswordForQuickLookDocumentInMainFrameReply:
        return "WebPageProxy::RequestPasswordForQuickLookDocumentInMainFrameReply";
#endif
#if ENABLE(CONTENT_FILTERING)
    case MessageName::WebPageProxy_ContentFilterDidBlockLoadForFrame:
        return "WebPageProxy::ContentFilterDidBlockLoadForFrame";
#endif
    case MessageName::WebPageProxy_IsPlayingMediaDidChange:
        return "WebPageProxy::IsPlayingMediaDidChange";
    case MessageName::WebPageProxy_HandleAutoplayEvent:
        return "WebPageProxy::HandleAutoplayEvent";
#if ENABLE(MEDIA_SESSION)
    case MessageName::WebPageProxy_HasMediaSessionWithActiveMediaElementsDidChange:
        return "WebPageProxy::HasMediaSessionWithActiveMediaElementsDidChange";
#endif
#if ENABLE(MEDIA_SESSION)
    case MessageName::WebPageProxy_MediaSessionMetadataDidChange:
        return "WebPageProxy::MediaSessionMetadataDidChange";
#endif
#if ENABLE(MEDIA_SESSION)
    case MessageName::WebPageProxy_FocusedContentMediaElementDidChange:
        return "WebPageProxy::FocusedContentMediaElementDidChange";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPageProxy_DidPerformImmediateActionHitTest:
        return "WebPageProxy::DidPerformImmediateActionHitTest";
#endif
    case MessageName::WebPageProxy_HandleMessage:
        return "WebPageProxy::HandleMessage";
    case MessageName::WebPageProxy_HandleSynchronousMessage:
        return "WebPageProxy::HandleSynchronousMessage";
    case MessageName::WebPageProxy_HandleAutoFillButtonClick:
        return "WebPageProxy::HandleAutoFillButtonClick";
    case MessageName::WebPageProxy_DidResignInputElementStrongPasswordAppearance:
        return "WebPageProxy::DidResignInputElementStrongPasswordAppearance";
    case MessageName::WebPageProxy_ContentRuleListNotification:
        return "WebPageProxy::ContentRuleListNotification";
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_AddPlaybackTargetPickerClient:
        return "WebPageProxy::AddPlaybackTargetPickerClient";
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_RemovePlaybackTargetPickerClient:
        return "WebPageProxy::RemovePlaybackTargetPickerClient";
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_ShowPlaybackTargetPicker:
        return "WebPageProxy::ShowPlaybackTargetPicker";
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_PlaybackTargetPickerClientStateDidChange:
        return "WebPageProxy::PlaybackTargetPickerClientStateDidChange";
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_SetMockMediaPlaybackTargetPickerEnabled:
        return "WebPageProxy::SetMockMediaPlaybackTargetPickerEnabled";
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_SetMockMediaPlaybackTargetPickerState:
        return "WebPageProxy::SetMockMediaPlaybackTargetPickerState";
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_MockMediaPlaybackTargetPickerDismissPopup:
        return "WebPageProxy::MockMediaPlaybackTargetPickerDismissPopup";
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    case MessageName::WebPageProxy_SetMockVideoPresentationModeEnabled:
        return "WebPageProxy::SetMockVideoPresentationModeEnabled";
#endif
#if ENABLE(POINTER_LOCK)
    case MessageName::WebPageProxy_RequestPointerLock:
        return "WebPageProxy::RequestPointerLock";
#endif
#if ENABLE(POINTER_LOCK)
    case MessageName::WebPageProxy_RequestPointerUnlock:
        return "WebPageProxy::RequestPointerUnlock";
#endif
    case MessageName::WebPageProxy_DidFailToSuspendAfterProcessSwap:
        return "WebPageProxy::DidFailToSuspendAfterProcessSwap";
    case MessageName::WebPageProxy_DidSuspendAfterProcessSwap:
        return "WebPageProxy::DidSuspendAfterProcessSwap";
    case MessageName::WebPageProxy_ImageOrMediaDocumentSizeChanged:
        return "WebPageProxy::ImageOrMediaDocumentSizeChanged";
    case MessageName::WebPageProxy_UseFixedLayoutDidChange:
        return "WebPageProxy::UseFixedLayoutDidChange";
    case MessageName::WebPageProxy_FixedLayoutSizeDidChange:
        return "WebPageProxy::FixedLayoutSizeDidChange";
#if ENABLE(VIDEO) && USE(GSTREAMER)
    case MessageName::WebPageProxy_RequestInstallMissingMediaPlugins:
        return "WebPageProxy::RequestInstallMissingMediaPlugins";
#endif
    case MessageName::WebPageProxy_DidRestoreScrollPosition:
        return "WebPageProxy::DidRestoreScrollPosition";
    case MessageName::WebPageProxy_GetLoadDecisionForIcon:
        return "WebPageProxy::GetLoadDecisionForIcon";
    case MessageName::WebPageProxy_FinishedLoadingIcon:
        return "WebPageProxy::FinishedLoadingIcon";
#if PLATFORM(MAC)
    case MessageName::WebPageProxy_DidHandleAcceptedCandidate:
        return "WebPageProxy::DidHandleAcceptedCandidate";
#endif
    case MessageName::WebPageProxy_SetIsUsingHighPerformanceWebGL:
        return "WebPageProxy::SetIsUsingHighPerformanceWebGL";
    case MessageName::WebPageProxy_StartURLSchemeTask:
        return "WebPageProxy::StartURLSchemeTask";
    case MessageName::WebPageProxy_StopURLSchemeTask:
        return "WebPageProxy::StopURLSchemeTask";
    case MessageName::WebPageProxy_LoadSynchronousURLSchemeTask:
        return "WebPageProxy::LoadSynchronousURLSchemeTask";
#if ENABLE(DEVICE_ORIENTATION)
    case MessageName::WebPageProxy_ShouldAllowDeviceOrientationAndMotionAccess:
        return "WebPageProxy::ShouldAllowDeviceOrientationAndMotionAccess";
    case MessageName::WebPageProxy_ShouldAllowDeviceOrientationAndMotionAccessReply:
        return "WebPageProxy::ShouldAllowDeviceOrientationAndMotionAccessReply";
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPageProxy_RegisterAttachmentIdentifierFromData:
        return "WebPageProxy::RegisterAttachmentIdentifierFromData";
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPageProxy_RegisterAttachmentIdentifierFromFilePath:
        return "WebPageProxy::RegisterAttachmentIdentifierFromFilePath";
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPageProxy_RegisterAttachmentIdentifier:
        return "WebPageProxy::RegisterAttachmentIdentifier";
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPageProxy_RegisterAttachmentsFromSerializedData:
        return "WebPageProxy::RegisterAttachmentsFromSerializedData";
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPageProxy_CloneAttachmentData:
        return "WebPageProxy::CloneAttachmentData";
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPageProxy_DidInsertAttachmentWithIdentifier:
        return "WebPageProxy::DidInsertAttachmentWithIdentifier";
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPageProxy_DidRemoveAttachmentWithIdentifier:
        return "WebPageProxy::DidRemoveAttachmentWithIdentifier";
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPageProxy_SerializedAttachmentDataForIdentifiers:
        return "WebPageProxy::SerializedAttachmentDataForIdentifiers";
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPageProxy_WritePromisedAttachmentToPasteboard:
        return "WebPageProxy::WritePromisedAttachmentToPasteboard";
#endif
    case MessageName::WebPageProxy_SignedPublicKeyAndChallengeString:
        return "WebPageProxy::SignedPublicKeyAndChallengeString";
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPageProxy_SpeechSynthesisVoiceList:
        return "WebPageProxy::SpeechSynthesisVoiceList";
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPageProxy_SpeechSynthesisSpeak:
        return "WebPageProxy::SpeechSynthesisSpeak";
    case MessageName::WebPageProxy_SpeechSynthesisSpeakReply:
        return "WebPageProxy::SpeechSynthesisSpeakReply";
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPageProxy_SpeechSynthesisSetFinishedCallback:
        return "WebPageProxy::SpeechSynthesisSetFinishedCallback";
    case MessageName::WebPageProxy_SpeechSynthesisSetFinishedCallbackReply:
        return "WebPageProxy::SpeechSynthesisSetFinishedCallbackReply";
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPageProxy_SpeechSynthesisCancel:
        return "WebPageProxy::SpeechSynthesisCancel";
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPageProxy_SpeechSynthesisPause:
        return "WebPageProxy::SpeechSynthesisPause";
    case MessageName::WebPageProxy_SpeechSynthesisPauseReply:
        return "WebPageProxy::SpeechSynthesisPauseReply";
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPageProxy_SpeechSynthesisResume:
        return "WebPageProxy::SpeechSynthesisResume";
    case MessageName::WebPageProxy_SpeechSynthesisResumeReply:
        return "WebPageProxy::SpeechSynthesisResumeReply";
#endif
    case MessageName::WebPageProxy_ConfigureLoggingChannel:
        return "WebPageProxy::ConfigureLoggingChannel";
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPageProxy_ShowEmojiPicker:
        return "WebPageProxy::ShowEmojiPicker";
    case MessageName::WebPageProxy_ShowEmojiPickerReply:
        return "WebPageProxy::ShowEmojiPickerReply";
#endif
#if HAVE(VISIBILITY_PROPAGATION_VIEW)
    case MessageName::WebPageProxy_DidCreateContextForVisibilityPropagation:
        return "WebPageProxy::DidCreateContextForVisibilityPropagation";
#endif
#if ENABLE(WEB_AUTHN)
    case MessageName::WebPageProxy_SetMockWebAuthenticationConfiguration:
        return "WebPageProxy::SetMockWebAuthenticationConfiguration";
#endif
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    case MessageName::WebPageProxy_SendMessageToWebView:
        return "WebPageProxy::SendMessageToWebView";
#endif
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    case MessageName::WebPageProxy_SendMessageToWebViewWithReply:
        return "WebPageProxy::SendMessageToWebViewWithReply";
    case MessageName::WebPageProxy_SendMessageToWebViewWithReplyReply:
        return "WebPageProxy::SendMessageToWebViewWithReplyReply";
#endif
    case MessageName::WebPageProxy_DidFindTextManipulationItems:
        return "WebPageProxy::DidFindTextManipulationItems";
#if ENABLE(MEDIA_USAGE)
    case MessageName::WebPageProxy_AddMediaUsageManagerSession:
        return "WebPageProxy::AddMediaUsageManagerSession";
#endif
#if ENABLE(MEDIA_USAGE)
    case MessageName::WebPageProxy_UpdateMediaUsageManagerSessionState:
        return "WebPageProxy::UpdateMediaUsageManagerSessionState";
#endif
#if ENABLE(MEDIA_USAGE)
    case MessageName::WebPageProxy_RemoveMediaUsageManagerSession:
        return "WebPageProxy::RemoveMediaUsageManagerSession";
#endif
    case MessageName::WebPageProxy_SetHasExecutedAppBoundBehaviorBeforeNavigation:
        return "WebPageProxy::SetHasExecutedAppBoundBehaviorBeforeNavigation";
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPasteboardProxy_WriteURLToPasteboard:
        return "WebPasteboardProxy::WriteURLToPasteboard";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPasteboardProxy_WriteWebContentToPasteboard:
        return "WebPasteboardProxy::WriteWebContentToPasteboard";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPasteboardProxy_WriteImageToPasteboard:
        return "WebPasteboardProxy::WriteImageToPasteboard";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPasteboardProxy_WriteStringToPasteboard:
        return "WebPasteboardProxy::WriteStringToPasteboard";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPasteboardProxy_UpdateSupportedTypeIdentifiers:
        return "WebPasteboardProxy::UpdateSupportedTypeIdentifiers";
#endif
    case MessageName::WebPasteboardProxy_WriteCustomData:
        return "WebPasteboardProxy::WriteCustomData";
    case MessageName::WebPasteboardProxy_TypesSafeForDOMToReadAndWrite:
        return "WebPasteboardProxy::TypesSafeForDOMToReadAndWrite";
    case MessageName::WebPasteboardProxy_AllPasteboardItemInfo:
        return "WebPasteboardProxy::AllPasteboardItemInfo";
    case MessageName::WebPasteboardProxy_InformationForItemAtIndex:
        return "WebPasteboardProxy::InformationForItemAtIndex";
    case MessageName::WebPasteboardProxy_GetPasteboardItemsCount:
        return "WebPasteboardProxy::GetPasteboardItemsCount";
    case MessageName::WebPasteboardProxy_ReadStringFromPasteboard:
        return "WebPasteboardProxy::ReadStringFromPasteboard";
    case MessageName::WebPasteboardProxy_ReadURLFromPasteboard:
        return "WebPasteboardProxy::ReadURLFromPasteboard";
    case MessageName::WebPasteboardProxy_ReadBufferFromPasteboard:
        return "WebPasteboardProxy::ReadBufferFromPasteboard";
    case MessageName::WebPasteboardProxy_ContainsStringSafeForDOMToReadForType:
        return "WebPasteboardProxy::ContainsStringSafeForDOMToReadForType";
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_GetNumberOfFiles:
        return "WebPasteboardProxy::GetNumberOfFiles";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_GetPasteboardTypes:
        return "WebPasteboardProxy::GetPasteboardTypes";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_GetPasteboardPathnamesForType:
        return "WebPasteboardProxy::GetPasteboardPathnamesForType";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_GetPasteboardStringForType:
        return "WebPasteboardProxy::GetPasteboardStringForType";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_GetPasteboardStringsForType:
        return "WebPasteboardProxy::GetPasteboardStringsForType";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_GetPasteboardBufferForType:
        return "WebPasteboardProxy::GetPasteboardBufferForType";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_GetPasteboardChangeCount:
        return "WebPasteboardProxy::GetPasteboardChangeCount";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_GetPasteboardColor:
        return "WebPasteboardProxy::GetPasteboardColor";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_GetPasteboardURL:
        return "WebPasteboardProxy::GetPasteboardURL";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_AddPasteboardTypes:
        return "WebPasteboardProxy::AddPasteboardTypes";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_SetPasteboardTypes:
        return "WebPasteboardProxy::SetPasteboardTypes";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_SetPasteboardURL:
        return "WebPasteboardProxy::SetPasteboardURL";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_SetPasteboardColor:
        return "WebPasteboardProxy::SetPasteboardColor";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_SetPasteboardStringForType:
        return "WebPasteboardProxy::SetPasteboardStringForType";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_SetPasteboardBufferForType:
        return "WebPasteboardProxy::SetPasteboardBufferForType";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_ContainsURLStringSuitableForLoading:
        return "WebPasteboardProxy::ContainsURLStringSuitableForLoading";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_URLStringSuitableForLoading:
        return "WebPasteboardProxy::URLStringSuitableForLoading";
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPasteboardProxy_GetTypes:
        return "WebPasteboardProxy::GetTypes";
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPasteboardProxy_ReadText:
        return "WebPasteboardProxy::ReadText";
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPasteboardProxy_ReadFilePaths:
        return "WebPasteboardProxy::ReadFilePaths";
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPasteboardProxy_ReadBuffer:
        return "WebPasteboardProxy::ReadBuffer";
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPasteboardProxy_WriteToClipboard:
        return "WebPasteboardProxy::WriteToClipboard";
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPasteboardProxy_ClearClipboard:
        return "WebPasteboardProxy::ClearClipboard";
#endif
#if USE(LIBWPE)
    case MessageName::WebPasteboardProxy_GetPasteboardTypes:
        return "WebPasteboardProxy::GetPasteboardTypes";
#endif
#if USE(LIBWPE)
    case MessageName::WebPasteboardProxy_WriteWebContentToPasteboard:
        return "WebPasteboardProxy::WriteWebContentToPasteboard";
#endif
#if USE(LIBWPE)
    case MessageName::WebPasteboardProxy_WriteStringToPasteboard:
        return "WebPasteboardProxy::WriteStringToPasteboard";
#endif
    case MessageName::WebProcessPool_HandleMessage:
        return "WebProcessPool::HandleMessage";
    case MessageName::WebProcessPool_HandleSynchronousMessage:
        return "WebProcessPool::HandleSynchronousMessage";
#if ENABLE(GAMEPAD)
    case MessageName::WebProcessPool_StartedUsingGamepads:
        return "WebProcessPool::StartedUsingGamepads";
#endif
#if ENABLE(GAMEPAD)
    case MessageName::WebProcessPool_StoppedUsingGamepads:
        return "WebProcessPool::StoppedUsingGamepads";
#endif
    case MessageName::WebProcessPool_ReportWebContentCPUTime:
        return "WebProcessPool::ReportWebContentCPUTime";
    case MessageName::WebProcessProxy_UpdateBackForwardItem:
        return "WebProcessProxy::UpdateBackForwardItem";
    case MessageName::WebProcessProxy_DidDestroyFrame:
        return "WebProcessProxy::DidDestroyFrame";
    case MessageName::WebProcessProxy_DidDestroyUserGestureToken:
        return "WebProcessProxy::DidDestroyUserGestureToken";
    case MessageName::WebProcessProxy_ShouldTerminate:
        return "WebProcessProxy::ShouldTerminate";
    case MessageName::WebProcessProxy_EnableSuddenTermination:
        return "WebProcessProxy::EnableSuddenTermination";
    case MessageName::WebProcessProxy_DisableSuddenTermination:
        return "WebProcessProxy::DisableSuddenTermination";
#if ENABLE(NETSCAPE_PLUGIN_API)
    case MessageName::WebProcessProxy_GetPlugins:
        return "WebProcessProxy::GetPlugins";
#endif
#if ENABLE(NETSCAPE_PLUGIN_API)
    case MessageName::WebProcessProxy_GetPluginProcessConnection:
        return "WebProcessProxy::GetPluginProcessConnection";
#endif
    case MessageName::WebProcessProxy_GetNetworkProcessConnection:
        return "WebProcessProxy::GetNetworkProcessConnection";
#if ENABLE(GPU_PROCESS)
    case MessageName::WebProcessProxy_GetGPUProcessConnection:
        return "WebProcessProxy::GetGPUProcessConnection";
#endif
    case MessageName::WebProcessProxy_SetIsHoldingLockedFiles:
        return "WebProcessProxy::SetIsHoldingLockedFiles";
    case MessageName::WebProcessProxy_DidExceedActiveMemoryLimit:
        return "WebProcessProxy::DidExceedActiveMemoryLimit";
    case MessageName::WebProcessProxy_DidExceedInactiveMemoryLimit:
        return "WebProcessProxy::DidExceedInactiveMemoryLimit";
    case MessageName::WebProcessProxy_DidExceedCPULimit:
        return "WebProcessProxy::DidExceedCPULimit";
    case MessageName::WebProcessProxy_StopResponsivenessTimer:
        return "WebProcessProxy::StopResponsivenessTimer";
    case MessageName::WebProcessProxy_DidReceiveMainThreadPing:
        return "WebProcessProxy::DidReceiveMainThreadPing";
    case MessageName::WebProcessProxy_DidReceiveBackgroundResponsivenessPing:
        return "WebProcessProxy::DidReceiveBackgroundResponsivenessPing";
    case MessageName::WebProcessProxy_MemoryPressureStatusChanged:
        return "WebProcessProxy::MemoryPressureStatusChanged";
    case MessageName::WebProcessProxy_DidExceedInactiveMemoryLimitWhileActive:
        return "WebProcessProxy::DidExceedInactiveMemoryLimitWhileActive";
    case MessageName::WebProcessProxy_DidCollectPrewarmInformation:
        return "WebProcessProxy::DidCollectPrewarmInformation";
#if PLATFORM(COCOA)
    case MessageName::WebProcessProxy_CacheMediaMIMETypes:
        return "WebProcessProxy::CacheMediaMIMETypes";
#endif
#if PLATFORM(MAC)
    case MessageName::WebProcessProxy_RequestHighPerformanceGPU:
        return "WebProcessProxy::RequestHighPerformanceGPU";
#endif
#if PLATFORM(MAC)
    case MessageName::WebProcessProxy_ReleaseHighPerformanceGPU:
        return "WebProcessProxy::ReleaseHighPerformanceGPU";
#endif
#if PLATFORM(MAC) && ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    case MessageName::WebProcessProxy_StartDisplayLink:
        return "WebProcessProxy::StartDisplayLink";
#endif
#if PLATFORM(MAC) && ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    case MessageName::WebProcessProxy_StopDisplayLink:
        return "WebProcessProxy::StopDisplayLink";
#endif
    case MessageName::WebProcessProxy_AddPlugInAutoStartOriginHash:
        return "WebProcessProxy::AddPlugInAutoStartOriginHash";
    case MessageName::WebProcessProxy_PlugInDidReceiveUserInteraction:
        return "WebProcessProxy::PlugInDidReceiveUserInteraction";
#if PLATFORM(GTK) || PLATFORM(WPE)
    case MessageName::WebProcessProxy_SendMessageToWebContext:
        return "WebProcessProxy::SendMessageToWebContext";
#endif
#if PLATFORM(GTK) || PLATFORM(WPE)
    case MessageName::WebProcessProxy_SendMessageToWebContextWithReply:
        return "WebProcessProxy::SendMessageToWebContextWithReply";
    case MessageName::WebProcessProxy_SendMessageToWebContextWithReplyReply:
        return "WebProcessProxy::SendMessageToWebContextWithReplyReply";
#endif
    case MessageName::WebProcessProxy_DidCreateSleepDisabler:
        return "WebProcessProxy::DidCreateSleepDisabler";
    case MessageName::WebProcessProxy_DidDestroySleepDisabler:
        return "WebProcessProxy::DidDestroySleepDisabler";
    case MessageName::WebAutomationSession_DidEvaluateJavaScriptFunction:
        return "WebAutomationSession::DidEvaluateJavaScriptFunction";
    case MessageName::WebAutomationSession_DidTakeScreenshot:
        return "WebAutomationSession::DidTakeScreenshot";
    case MessageName::DownloadProxy_DidStart:
        return "DownloadProxy::DidStart";
    case MessageName::DownloadProxy_DidReceiveAuthenticationChallenge:
        return "DownloadProxy::DidReceiveAuthenticationChallenge";
    case MessageName::DownloadProxy_WillSendRequest:
        return "DownloadProxy::WillSendRequest";
    case MessageName::DownloadProxy_DecideDestinationWithSuggestedFilenameAsync:
        return "DownloadProxy::DecideDestinationWithSuggestedFilenameAsync";
    case MessageName::DownloadProxy_DidReceiveResponse:
        return "DownloadProxy::DidReceiveResponse";
    case MessageName::DownloadProxy_DidReceiveData:
        return "DownloadProxy::DidReceiveData";
    case MessageName::DownloadProxy_DidCreateDestination:
        return "DownloadProxy::DidCreateDestination";
    case MessageName::DownloadProxy_DidFinish:
        return "DownloadProxy::DidFinish";
    case MessageName::DownloadProxy_DidFail:
        return "DownloadProxy::DidFail";
    case MessageName::DownloadProxy_DidCancel:
        return "DownloadProxy::DidCancel";
#if HAVE(VISIBILITY_PROPAGATION_VIEW)
    case MessageName::GPUProcessProxy_DidCreateContextForVisibilityPropagation:
        return "GPUProcessProxy::DidCreateContextForVisibilityPropagation";
#endif
    case MessageName::RemoteWebInspectorProxy_FrontendDidClose:
        return "RemoteWebInspectorProxy::FrontendDidClose";
    case MessageName::RemoteWebInspectorProxy_Reopen:
        return "RemoteWebInspectorProxy::Reopen";
    case MessageName::RemoteWebInspectorProxy_ResetState:
        return "RemoteWebInspectorProxy::ResetState";
    case MessageName::RemoteWebInspectorProxy_BringToFront:
        return "RemoteWebInspectorProxy::BringToFront";
    case MessageName::RemoteWebInspectorProxy_Save:
        return "RemoteWebInspectorProxy::Save";
    case MessageName::RemoteWebInspectorProxy_Append:
        return "RemoteWebInspectorProxy::Append";
    case MessageName::RemoteWebInspectorProxy_SetForcedAppearance:
        return "RemoteWebInspectorProxy::SetForcedAppearance";
    case MessageName::RemoteWebInspectorProxy_SetSheetRect:
        return "RemoteWebInspectorProxy::SetSheetRect";
    case MessageName::RemoteWebInspectorProxy_StartWindowDrag:
        return "RemoteWebInspectorProxy::StartWindowDrag";
    case MessageName::RemoteWebInspectorProxy_OpenInNewTab:
        return "RemoteWebInspectorProxy::OpenInNewTab";
    case MessageName::RemoteWebInspectorProxy_ShowCertificate:
        return "RemoteWebInspectorProxy::ShowCertificate";
    case MessageName::RemoteWebInspectorProxy_SendMessageToBackend:
        return "RemoteWebInspectorProxy::SendMessageToBackend";
    case MessageName::WebInspectorProxy_OpenLocalInspectorFrontend:
        return "WebInspectorProxy::OpenLocalInspectorFrontend";
    case MessageName::WebInspectorProxy_SetFrontendConnection:
        return "WebInspectorProxy::SetFrontendConnection";
    case MessageName::WebInspectorProxy_SendMessageToBackend:
        return "WebInspectorProxy::SendMessageToBackend";
    case MessageName::WebInspectorProxy_FrontendLoaded:
        return "WebInspectorProxy::FrontendLoaded";
    case MessageName::WebInspectorProxy_DidClose:
        return "WebInspectorProxy::DidClose";
    case MessageName::WebInspectorProxy_BringToFront:
        return "WebInspectorProxy::BringToFront";
    case MessageName::WebInspectorProxy_BringInspectedPageToFront:
        return "WebInspectorProxy::BringInspectedPageToFront";
    case MessageName::WebInspectorProxy_Reopen:
        return "WebInspectorProxy::Reopen";
    case MessageName::WebInspectorProxy_ResetState:
        return "WebInspectorProxy::ResetState";
    case MessageName::WebInspectorProxy_SetForcedAppearance:
        return "WebInspectorProxy::SetForcedAppearance";
    case MessageName::WebInspectorProxy_InspectedURLChanged:
        return "WebInspectorProxy::InspectedURLChanged";
    case MessageName::WebInspectorProxy_ShowCertificate:
        return "WebInspectorProxy::ShowCertificate";
    case MessageName::WebInspectorProxy_ElementSelectionChanged:
        return "WebInspectorProxy::ElementSelectionChanged";
    case MessageName::WebInspectorProxy_TimelineRecordingChanged:
        return "WebInspectorProxy::TimelineRecordingChanged";
    case MessageName::WebInspectorProxy_SetDeveloperPreferenceOverride:
        return "WebInspectorProxy::SetDeveloperPreferenceOverride";
    case MessageName::WebInspectorProxy_Save:
        return "WebInspectorProxy::Save";
    case MessageName::WebInspectorProxy_Append:
        return "WebInspectorProxy::Append";
    case MessageName::WebInspectorProxy_AttachBottom:
        return "WebInspectorProxy::AttachBottom";
    case MessageName::WebInspectorProxy_AttachRight:
        return "WebInspectorProxy::AttachRight";
    case MessageName::WebInspectorProxy_AttachLeft:
        return "WebInspectorProxy::AttachLeft";
    case MessageName::WebInspectorProxy_Detach:
        return "WebInspectorProxy::Detach";
    case MessageName::WebInspectorProxy_AttachAvailabilityChanged:
        return "WebInspectorProxy::AttachAvailabilityChanged";
    case MessageName::WebInspectorProxy_SetAttachedWindowHeight:
        return "WebInspectorProxy::SetAttachedWindowHeight";
    case MessageName::WebInspectorProxy_SetAttachedWindowWidth:
        return "WebInspectorProxy::SetAttachedWindowWidth";
    case MessageName::WebInspectorProxy_SetSheetRect:
        return "WebInspectorProxy::SetSheetRect";
    case MessageName::WebInspectorProxy_StartWindowDrag:
        return "WebInspectorProxy::StartWindowDrag";
    case MessageName::NetworkProcessProxy_DidReceiveAuthenticationChallenge:
        return "NetworkProcessProxy::DidReceiveAuthenticationChallenge";
    case MessageName::NetworkProcessProxy_NegotiatedLegacyTLS:
        return "NetworkProcessProxy::NegotiatedLegacyTLS";
    case MessageName::NetworkProcessProxy_DidNegotiateModernTLS:
        return "NetworkProcessProxy::DidNegotiateModernTLS";
    case MessageName::NetworkProcessProxy_DidFetchWebsiteData:
        return "NetworkProcessProxy::DidFetchWebsiteData";
    case MessageName::NetworkProcessProxy_DidDeleteWebsiteData:
        return "NetworkProcessProxy::DidDeleteWebsiteData";
    case MessageName::NetworkProcessProxy_DidDeleteWebsiteDataForOrigins:
        return "NetworkProcessProxy::DidDeleteWebsiteDataForOrigins";
    case MessageName::NetworkProcessProxy_DidSyncAllCookies:
        return "NetworkProcessProxy::DidSyncAllCookies";
    case MessageName::NetworkProcessProxy_TestProcessIncomingSyncMessagesWhenWaitingForSyncReply:
        return "NetworkProcessProxy::TestProcessIncomingSyncMessagesWhenWaitingForSyncReply";
    case MessageName::NetworkProcessProxy_TerminateUnresponsiveServiceWorkerProcesses:
        return "NetworkProcessProxy::TerminateUnresponsiveServiceWorkerProcesses";
    case MessageName::NetworkProcessProxy_SetIsHoldingLockedFiles:
        return "NetworkProcessProxy::SetIsHoldingLockedFiles";
    case MessageName::NetworkProcessProxy_LogDiagnosticMessage:
        return "NetworkProcessProxy::LogDiagnosticMessage";
    case MessageName::NetworkProcessProxy_LogDiagnosticMessageWithResult:
        return "NetworkProcessProxy::LogDiagnosticMessageWithResult";
    case MessageName::NetworkProcessProxy_LogDiagnosticMessageWithValue:
        return "NetworkProcessProxy::LogDiagnosticMessageWithValue";
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_LogTestingEvent:
        return "NetworkProcessProxy::LogTestingEvent";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_NotifyResourceLoadStatisticsProcessed:
        return "NetworkProcessProxy::NotifyResourceLoadStatisticsProcessed";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_NotifyWebsiteDataDeletionForRegistrableDomainsFinished:
        return "NetworkProcessProxy::NotifyWebsiteDataDeletionForRegistrableDomainsFinished";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_NotifyWebsiteDataScanForRegistrableDomainsFinished:
        return "NetworkProcessProxy::NotifyWebsiteDataScanForRegistrableDomainsFinished";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_NotifyResourceLoadStatisticsTelemetryFinished:
        return "NetworkProcessProxy::NotifyResourceLoadStatisticsTelemetryFinished";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_RequestStorageAccessConfirm:
        return "NetworkProcessProxy::RequestStorageAccessConfirm";
    case MessageName::NetworkProcessProxy_RequestStorageAccessConfirmReply:
        return "NetworkProcessProxy::RequestStorageAccessConfirmReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_DeleteWebsiteDataInUIProcessForRegistrableDomains:
        return "NetworkProcessProxy::DeleteWebsiteDataInUIProcessForRegistrableDomains";
    case MessageName::NetworkProcessProxy_DeleteWebsiteDataInUIProcessForRegistrableDomainsReply:
        return "NetworkProcessProxy::DeleteWebsiteDataInUIProcessForRegistrableDomainsReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_DidCommitCrossSiteLoadWithDataTransferFromPrevalentResource:
        return "NetworkProcessProxy::DidCommitCrossSiteLoadWithDataTransferFromPrevalentResource";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_SetDomainsWithUserInteraction:
        return "NetworkProcessProxy::SetDomainsWithUserInteraction";
#endif
#if ENABLE(CONTENT_EXTENSIONS)
    case MessageName::NetworkProcessProxy_ContentExtensionRules:
        return "NetworkProcessProxy::ContentExtensionRules";
#endif
    case MessageName::NetworkProcessProxy_RetrieveCacheStorageParameters:
        return "NetworkProcessProxy::RetrieveCacheStorageParameters";
    case MessageName::NetworkProcessProxy_TerminateWebProcess:
        return "NetworkProcessProxy::TerminateWebProcess";
#if ENABLE(SERVICE_WORKER)
    case MessageName::NetworkProcessProxy_EstablishWorkerContextConnectionToNetworkProcess:
        return "NetworkProcessProxy::EstablishWorkerContextConnectionToNetworkProcess";
    case MessageName::NetworkProcessProxy_EstablishWorkerContextConnectionToNetworkProcessReply:
        return "NetworkProcessProxy::EstablishWorkerContextConnectionToNetworkProcessReply";
#endif
#if ENABLE(SERVICE_WORKER)
    case MessageName::NetworkProcessProxy_WorkerContextConnectionNoLongerNeeded:
        return "NetworkProcessProxy::WorkerContextConnectionNoLongerNeeded";
#endif
#if ENABLE(SERVICE_WORKER)
    case MessageName::NetworkProcessProxy_RegisterServiceWorkerClientProcess:
        return "NetworkProcessProxy::RegisterServiceWorkerClientProcess";
#endif
#if ENABLE(SERVICE_WORKER)
    case MessageName::NetworkProcessProxy_UnregisterServiceWorkerClientProcess:
        return "NetworkProcessProxy::UnregisterServiceWorkerClientProcess";
#endif
    case MessageName::NetworkProcessProxy_SetWebProcessHasUploads:
        return "NetworkProcessProxy::SetWebProcessHasUploads";
    case MessageName::NetworkProcessProxy_GetAppBoundDomains:
        return "NetworkProcessProxy::GetAppBoundDomains";
    case MessageName::NetworkProcessProxy_GetAppBoundDomainsReply:
        return "NetworkProcessProxy::GetAppBoundDomainsReply";
    case MessageName::NetworkProcessProxy_RequestStorageSpace:
        return "NetworkProcessProxy::RequestStorageSpace";
    case MessageName::NetworkProcessProxy_RequestStorageSpaceReply:
        return "NetworkProcessProxy::RequestStorageSpaceReply";
    case MessageName::NetworkProcessProxy_ResourceLoadDidSendRequest:
        return "NetworkProcessProxy::ResourceLoadDidSendRequest";
    case MessageName::NetworkProcessProxy_ResourceLoadDidPerformHTTPRedirection:
        return "NetworkProcessProxy::ResourceLoadDidPerformHTTPRedirection";
    case MessageName::NetworkProcessProxy_ResourceLoadDidReceiveChallenge:
        return "NetworkProcessProxy::ResourceLoadDidReceiveChallenge";
    case MessageName::NetworkProcessProxy_ResourceLoadDidReceiveResponse:
        return "NetworkProcessProxy::ResourceLoadDidReceiveResponse";
    case MessageName::NetworkProcessProxy_ResourceLoadDidCompleteWithError:
        return "NetworkProcessProxy::ResourceLoadDidCompleteWithError";
    case MessageName::PluginProcessProxy_DidCreateWebProcessConnection:
        return "PluginProcessProxy::DidCreateWebProcessConnection";
    case MessageName::PluginProcessProxy_DidGetSitesWithData:
        return "PluginProcessProxy::DidGetSitesWithData";
    case MessageName::PluginProcessProxy_DidDeleteWebsiteData:
        return "PluginProcessProxy::DidDeleteWebsiteData";
    case MessageName::PluginProcessProxy_DidDeleteWebsiteDataForHostNames:
        return "PluginProcessProxy::DidDeleteWebsiteDataForHostNames";
#if PLATFORM(COCOA)
    case MessageName::PluginProcessProxy_SetModalWindowIsShowing:
        return "PluginProcessProxy::SetModalWindowIsShowing";
#endif
#if PLATFORM(COCOA)
    case MessageName::PluginProcessProxy_SetFullscreenWindowIsShowing:
        return "PluginProcessProxy::SetFullscreenWindowIsShowing";
#endif
#if PLATFORM(COCOA)
    case MessageName::PluginProcessProxy_LaunchProcess:
        return "PluginProcessProxy::LaunchProcess";
#endif
#if PLATFORM(COCOA)
    case MessageName::PluginProcessProxy_LaunchApplicationAtURL:
        return "PluginProcessProxy::LaunchApplicationAtURL";
#endif
#if PLATFORM(COCOA)
    case MessageName::PluginProcessProxy_OpenURL:
        return "PluginProcessProxy::OpenURL";
#endif
#if PLATFORM(COCOA)
    case MessageName::PluginProcessProxy_OpenFile:
        return "PluginProcessProxy::OpenFile";
#endif
    case MessageName::WebUserContentControllerProxy_DidPostMessage:
        return "WebUserContentControllerProxy::DidPostMessage";
    case MessageName::WebUserContentControllerProxy_DidPostMessageReply:
        return "WebUserContentControllerProxy::DidPostMessageReply";
    case MessageName::WebProcess_InitializeWebProcess:
        return "WebProcess::InitializeWebProcess";
    case MessageName::WebProcess_SetWebsiteDataStoreParameters:
        return "WebProcess::SetWebsiteDataStoreParameters";
    case MessageName::WebProcess_CreateWebPage:
        return "WebProcess::CreateWebPage";
    case MessageName::WebProcess_PrewarmGlobally:
        return "WebProcess::PrewarmGlobally";
    case MessageName::WebProcess_PrewarmWithDomainInformation:
        return "WebProcess::PrewarmWithDomainInformation";
    case MessageName::WebProcess_SetCacheModel:
        return "WebProcess::SetCacheModel";
    case MessageName::WebProcess_RegisterURLSchemeAsEmptyDocument:
        return "WebProcess::RegisterURLSchemeAsEmptyDocument";
    case MessageName::WebProcess_RegisterURLSchemeAsSecure:
        return "WebProcess::RegisterURLSchemeAsSecure";
    case MessageName::WebProcess_RegisterURLSchemeAsBypassingContentSecurityPolicy:
        return "WebProcess::RegisterURLSchemeAsBypassingContentSecurityPolicy";
    case MessageName::WebProcess_SetDomainRelaxationForbiddenForURLScheme:
        return "WebProcess::SetDomainRelaxationForbiddenForURLScheme";
    case MessageName::WebProcess_RegisterURLSchemeAsLocal:
        return "WebProcess::RegisterURLSchemeAsLocal";
    case MessageName::WebProcess_RegisterURLSchemeAsNoAccess:
        return "WebProcess::RegisterURLSchemeAsNoAccess";
    case MessageName::WebProcess_RegisterURLSchemeAsDisplayIsolated:
        return "WebProcess::RegisterURLSchemeAsDisplayIsolated";
    case MessageName::WebProcess_RegisterURLSchemeAsCORSEnabled:
        return "WebProcess::RegisterURLSchemeAsCORSEnabled";
    case MessageName::WebProcess_RegisterURLSchemeAsCachePartitioned:
        return "WebProcess::RegisterURLSchemeAsCachePartitioned";
    case MessageName::WebProcess_RegisterURLSchemeAsCanDisplayOnlyIfCanRequest:
        return "WebProcess::RegisterURLSchemeAsCanDisplayOnlyIfCanRequest";
    case MessageName::WebProcess_SetDefaultRequestTimeoutInterval:
        return "WebProcess::SetDefaultRequestTimeoutInterval";
    case MessageName::WebProcess_SetAlwaysUsesComplexTextCodePath:
        return "WebProcess::SetAlwaysUsesComplexTextCodePath";
    case MessageName::WebProcess_SetShouldUseFontSmoothing:
        return "WebProcess::SetShouldUseFontSmoothing";
    case MessageName::WebProcess_SetResourceLoadStatisticsEnabled:
        return "WebProcess::SetResourceLoadStatisticsEnabled";
    case MessageName::WebProcess_ClearResourceLoadStatistics:
        return "WebProcess::ClearResourceLoadStatistics";
    case MessageName::WebProcess_UserPreferredLanguagesChanged:
        return "WebProcess::UserPreferredLanguagesChanged";
    case MessageName::WebProcess_FullKeyboardAccessModeChanged:
        return "WebProcess::FullKeyboardAccessModeChanged";
    case MessageName::WebProcess_DidAddPlugInAutoStartOriginHash:
        return "WebProcess::DidAddPlugInAutoStartOriginHash";
    case MessageName::WebProcess_ResetPlugInAutoStartOriginHashes:
        return "WebProcess::ResetPlugInAutoStartOriginHashes";
    case MessageName::WebProcess_SetPluginLoadClientPolicy:
        return "WebProcess::SetPluginLoadClientPolicy";
    case MessageName::WebProcess_ResetPluginLoadClientPolicies:
        return "WebProcess::ResetPluginLoadClientPolicies";
    case MessageName::WebProcess_ClearPluginClientPolicies:
        return "WebProcess::ClearPluginClientPolicies";
    case MessageName::WebProcess_RefreshPlugins:
        return "WebProcess::RefreshPlugins";
    case MessageName::WebProcess_StartMemorySampler:
        return "WebProcess::StartMemorySampler";
    case MessageName::WebProcess_StopMemorySampler:
        return "WebProcess::StopMemorySampler";
    case MessageName::WebProcess_SetTextCheckerState:
        return "WebProcess::SetTextCheckerState";
    case MessageName::WebProcess_SetEnhancedAccessibility:
        return "WebProcess::SetEnhancedAccessibility";
    case MessageName::WebProcess_GarbageCollectJavaScriptObjects:
        return "WebProcess::GarbageCollectJavaScriptObjects";
    case MessageName::WebProcess_SetJavaScriptGarbageCollectorTimerEnabled:
        return "WebProcess::SetJavaScriptGarbageCollectorTimerEnabled";
    case MessageName::WebProcess_SetInjectedBundleParameter:
        return "WebProcess::SetInjectedBundleParameter";
    case MessageName::WebProcess_SetInjectedBundleParameters:
        return "WebProcess::SetInjectedBundleParameters";
    case MessageName::WebProcess_HandleInjectedBundleMessage:
        return "WebProcess::HandleInjectedBundleMessage";
    case MessageName::WebProcess_FetchWebsiteData:
        return "WebProcess::FetchWebsiteData";
    case MessageName::WebProcess_FetchWebsiteDataReply:
        return "WebProcess::FetchWebsiteDataReply";
    case MessageName::WebProcess_DeleteWebsiteData:
        return "WebProcess::DeleteWebsiteData";
    case MessageName::WebProcess_DeleteWebsiteDataReply:
        return "WebProcess::DeleteWebsiteDataReply";
    case MessageName::WebProcess_DeleteWebsiteDataForOrigins:
        return "WebProcess::DeleteWebsiteDataForOrigins";
    case MessageName::WebProcess_DeleteWebsiteDataForOriginsReply:
        return "WebProcess::DeleteWebsiteDataForOriginsReply";
    case MessageName::WebProcess_SetHiddenPageDOMTimerThrottlingIncreaseLimit:
        return "WebProcess::SetHiddenPageDOMTimerThrottlingIncreaseLimit";
#if PLATFORM(COCOA)
    case MessageName::WebProcess_SetQOS:
        return "WebProcess::SetQOS";
#endif
    case MessageName::WebProcess_SetMemoryCacheDisabled:
        return "WebProcess::SetMemoryCacheDisabled";
#if ENABLE(SERVICE_CONTROLS)
    case MessageName::WebProcess_SetEnabledServices:
        return "WebProcess::SetEnabledServices";
#endif
    case MessageName::WebProcess_EnsureAutomationSessionProxy:
        return "WebProcess::EnsureAutomationSessionProxy";
    case MessageName::WebProcess_DestroyAutomationSessionProxy:
        return "WebProcess::DestroyAutomationSessionProxy";
    case MessageName::WebProcess_PrepareToSuspend:
        return "WebProcess::PrepareToSuspend";
    case MessageName::WebProcess_PrepareToSuspendReply:
        return "WebProcess::PrepareToSuspendReply";
    case MessageName::WebProcess_ProcessDidResume:
        return "WebProcess::ProcessDidResume";
    case MessageName::WebProcess_MainThreadPing:
        return "WebProcess::MainThreadPing";
    case MessageName::WebProcess_BackgroundResponsivenessPing:
        return "WebProcess::BackgroundResponsivenessPing";
#if ENABLE(GAMEPAD)
    case MessageName::WebProcess_SetInitialGamepads:
        return "WebProcess::SetInitialGamepads";
#endif
#if ENABLE(GAMEPAD)
    case MessageName::WebProcess_GamepadConnected:
        return "WebProcess::GamepadConnected";
#endif
#if ENABLE(GAMEPAD)
    case MessageName::WebProcess_GamepadDisconnected:
        return "WebProcess::GamepadDisconnected";
#endif
#if ENABLE(SERVICE_WORKER)
    case MessageName::WebProcess_EstablishWorkerContextConnectionToNetworkProcess:
        return "WebProcess::EstablishWorkerContextConnectionToNetworkProcess";
    case MessageName::WebProcess_EstablishWorkerContextConnectionToNetworkProcessReply:
        return "WebProcess::EstablishWorkerContextConnectionToNetworkProcessReply";
#endif
    case MessageName::WebProcess_SetHasSuspendedPageProxy:
        return "WebProcess::SetHasSuspendedPageProxy";
    case MessageName::WebProcess_SetIsInProcessCache:
        return "WebProcess::SetIsInProcessCache";
    case MessageName::WebProcess_MarkIsNoLongerPrewarmed:
        return "WebProcess::MarkIsNoLongerPrewarmed";
    case MessageName::WebProcess_GetActivePagesOriginsForTesting:
        return "WebProcess::GetActivePagesOriginsForTesting";
    case MessageName::WebProcess_GetActivePagesOriginsForTestingReply:
        return "WebProcess::GetActivePagesOriginsForTestingReply";
#if PLATFORM(COCOA)
    case MessageName::WebProcess_SetScreenProperties:
        return "WebProcess::SetScreenProperties";
#endif
#if PLATFORM(MAC) && ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    case MessageName::WebProcess_ScrollerStylePreferenceChanged:
        return "WebProcess::ScrollerStylePreferenceChanged";
#endif
#if PLATFORM(MAC) && ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    case MessageName::WebProcess_DisplayConfigurationChanged:
        return "WebProcess::DisplayConfigurationChanged";
#endif
#if PLATFORM(IOS_FAMILY) && !PLATFORM(MACCATALYST)
    case MessageName::WebProcess_BacklightLevelDidChange:
        return "WebProcess::BacklightLevelDidChange";
#endif
    case MessageName::WebProcess_IsJITEnabled:
        return "WebProcess::IsJITEnabled";
    case MessageName::WebProcess_IsJITEnabledReply:
        return "WebProcess::IsJITEnabledReply";
#if PLATFORM(COCOA)
    case MessageName::WebProcess_SetMediaMIMETypes:
        return "WebProcess::SetMediaMIMETypes";
#endif
#if (PLATFORM(COCOA) && ENABLE(REMOTE_INSPECTOR))
    case MessageName::WebProcess_EnableRemoteWebInspector:
        return "WebProcess::EnableRemoteWebInspector";
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebProcess_AddMockMediaDevice:
        return "WebProcess::AddMockMediaDevice";
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebProcess_ClearMockMediaDevices:
        return "WebProcess::ClearMockMediaDevices";
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebProcess_RemoveMockMediaDevice:
        return "WebProcess::RemoveMockMediaDevice";
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebProcess_ResetMockMediaDevices:
        return "WebProcess::ResetMockMediaDevices";
#endif
#if (ENABLE(MEDIA_STREAM) && ENABLE(SANDBOX_EXTENSIONS))
    case MessageName::WebProcess_GrantUserMediaDeviceSandboxExtensions:
        return "WebProcess::GrantUserMediaDeviceSandboxExtensions";
#endif
#if (ENABLE(MEDIA_STREAM) && ENABLE(SANDBOX_EXTENSIONS))
    case MessageName::WebProcess_RevokeUserMediaDeviceSandboxExtensions:
        return "WebProcess::RevokeUserMediaDeviceSandboxExtensions";
#endif
    case MessageName::WebProcess_ClearCurrentModifierStateForTesting:
        return "WebProcess::ClearCurrentModifierStateForTesting";
    case MessageName::WebProcess_SetBackForwardCacheCapacity:
        return "WebProcess::SetBackForwardCacheCapacity";
    case MessageName::WebProcess_ClearCachedPage:
        return "WebProcess::ClearCachedPage";
    case MessageName::WebProcess_ClearCachedPageReply:
        return "WebProcess::ClearCachedPageReply";
#if PLATFORM(GTK) || PLATFORM(WPE)
    case MessageName::WebProcess_SendMessageToWebExtension:
        return "WebProcess::SendMessageToWebExtension";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::WebProcess_SeedResourceLoadStatisticsForTesting:
        return "WebProcess::SeedResourceLoadStatisticsForTesting";
    case MessageName::WebProcess_SeedResourceLoadStatisticsForTestingReply:
        return "WebProcess::SeedResourceLoadStatisticsForTestingReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::WebProcess_SetThirdPartyCookieBlockingMode:
        return "WebProcess::SetThirdPartyCookieBlockingMode";
    case MessageName::WebProcess_SetThirdPartyCookieBlockingModeReply:
        return "WebProcess::SetThirdPartyCookieBlockingModeReply";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::WebProcess_SetDomainsWithUserInteraction:
        return "WebProcess::SetDomainsWithUserInteraction";
#endif
#if PLATFORM(IOS)
    case MessageName::WebProcess_GrantAccessToAssetServices:
        return "WebProcess::GrantAccessToAssetServices";
#endif
#if PLATFORM(IOS)
    case MessageName::WebProcess_RevokeAccessToAssetServices:
        return "WebProcess::RevokeAccessToAssetServices";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebProcess_UnblockServicesRequiredByAccessibility:
        return "WebProcess::UnblockServicesRequiredByAccessibility";
#endif
#if (PLATFORM(COCOA) && ENABLE(CFPREFS_DIRECT_MODE))
    case MessageName::WebProcess_NotifyPreferencesChanged:
        return "WebProcess::NotifyPreferencesChanged";
#endif
#if (PLATFORM(COCOA) && ENABLE(CFPREFS_DIRECT_MODE))
    case MessageName::WebProcess_UnblockPreferenceService:
        return "WebProcess::UnblockPreferenceService";
#endif
#if PLATFORM(GTK) && !USE(GTK4)
    case MessageName::WebProcess_SetUseSystemAppearanceForScrollbars:
        return "WebProcess::SetUseSystemAppearanceForScrollbars";
#endif
    case MessageName::WebAutomationSessionProxy_EvaluateJavaScriptFunction:
        return "WebAutomationSessionProxy::EvaluateJavaScriptFunction";
    case MessageName::WebAutomationSessionProxy_ResolveChildFrameWithOrdinal:
        return "WebAutomationSessionProxy::ResolveChildFrameWithOrdinal";
    case MessageName::WebAutomationSessionProxy_ResolveChildFrameWithOrdinalReply:
        return "WebAutomationSessionProxy::ResolveChildFrameWithOrdinalReply";
    case MessageName::WebAutomationSessionProxy_ResolveChildFrameWithNodeHandle:
        return "WebAutomationSessionProxy::ResolveChildFrameWithNodeHandle";
    case MessageName::WebAutomationSessionProxy_ResolveChildFrameWithNodeHandleReply:
        return "WebAutomationSessionProxy::ResolveChildFrameWithNodeHandleReply";
    case MessageName::WebAutomationSessionProxy_ResolveChildFrameWithName:
        return "WebAutomationSessionProxy::ResolveChildFrameWithName";
    case MessageName::WebAutomationSessionProxy_ResolveChildFrameWithNameReply:
        return "WebAutomationSessionProxy::ResolveChildFrameWithNameReply";
    case MessageName::WebAutomationSessionProxy_ResolveParentFrame:
        return "WebAutomationSessionProxy::ResolveParentFrame";
    case MessageName::WebAutomationSessionProxy_ResolveParentFrameReply:
        return "WebAutomationSessionProxy::ResolveParentFrameReply";
    case MessageName::WebAutomationSessionProxy_FocusFrame:
        return "WebAutomationSessionProxy::FocusFrame";
    case MessageName::WebAutomationSessionProxy_ComputeElementLayout:
        return "WebAutomationSessionProxy::ComputeElementLayout";
    case MessageName::WebAutomationSessionProxy_ComputeElementLayoutReply:
        return "WebAutomationSessionProxy::ComputeElementLayoutReply";
    case MessageName::WebAutomationSessionProxy_SelectOptionElement:
        return "WebAutomationSessionProxy::SelectOptionElement";
    case MessageName::WebAutomationSessionProxy_SelectOptionElementReply:
        return "WebAutomationSessionProxy::SelectOptionElementReply";
    case MessageName::WebAutomationSessionProxy_SetFilesForInputFileUpload:
        return "WebAutomationSessionProxy::SetFilesForInputFileUpload";
    case MessageName::WebAutomationSessionProxy_SetFilesForInputFileUploadReply:
        return "WebAutomationSessionProxy::SetFilesForInputFileUploadReply";
    case MessageName::WebAutomationSessionProxy_TakeScreenshot:
        return "WebAutomationSessionProxy::TakeScreenshot";
    case MessageName::WebAutomationSessionProxy_SnapshotRectForScreenshot:
        return "WebAutomationSessionProxy::SnapshotRectForScreenshot";
    case MessageName::WebAutomationSessionProxy_SnapshotRectForScreenshotReply:
        return "WebAutomationSessionProxy::SnapshotRectForScreenshotReply";
    case MessageName::WebAutomationSessionProxy_GetCookiesForFrame:
        return "WebAutomationSessionProxy::GetCookiesForFrame";
    case MessageName::WebAutomationSessionProxy_GetCookiesForFrameReply:
        return "WebAutomationSessionProxy::GetCookiesForFrameReply";
    case MessageName::WebAutomationSessionProxy_DeleteCookie:
        return "WebAutomationSessionProxy::DeleteCookie";
    case MessageName::WebAutomationSessionProxy_DeleteCookieReply:
        return "WebAutomationSessionProxy::DeleteCookieReply";
    case MessageName::WebIDBConnectionToServer_DidDeleteDatabase:
        return "WebIDBConnectionToServer::DidDeleteDatabase";
    case MessageName::WebIDBConnectionToServer_DidOpenDatabase:
        return "WebIDBConnectionToServer::DidOpenDatabase";
    case MessageName::WebIDBConnectionToServer_DidAbortTransaction:
        return "WebIDBConnectionToServer::DidAbortTransaction";
    case MessageName::WebIDBConnectionToServer_DidCommitTransaction:
        return "WebIDBConnectionToServer::DidCommitTransaction";
    case MessageName::WebIDBConnectionToServer_DidCreateObjectStore:
        return "WebIDBConnectionToServer::DidCreateObjectStore";
    case MessageName::WebIDBConnectionToServer_DidDeleteObjectStore:
        return "WebIDBConnectionToServer::DidDeleteObjectStore";
    case MessageName::WebIDBConnectionToServer_DidRenameObjectStore:
        return "WebIDBConnectionToServer::DidRenameObjectStore";
    case MessageName::WebIDBConnectionToServer_DidClearObjectStore:
        return "WebIDBConnectionToServer::DidClearObjectStore";
    case MessageName::WebIDBConnectionToServer_DidCreateIndex:
        return "WebIDBConnectionToServer::DidCreateIndex";
    case MessageName::WebIDBConnectionToServer_DidDeleteIndex:
        return "WebIDBConnectionToServer::DidDeleteIndex";
    case MessageName::WebIDBConnectionToServer_DidRenameIndex:
        return "WebIDBConnectionToServer::DidRenameIndex";
    case MessageName::WebIDBConnectionToServer_DidPutOrAdd:
        return "WebIDBConnectionToServer::DidPutOrAdd";
    case MessageName::WebIDBConnectionToServer_DidGetRecord:
        return "WebIDBConnectionToServer::DidGetRecord";
    case MessageName::WebIDBConnectionToServer_DidGetAllRecords:
        return "WebIDBConnectionToServer::DidGetAllRecords";
    case MessageName::WebIDBConnectionToServer_DidGetCount:
        return "WebIDBConnectionToServer::DidGetCount";
    case MessageName::WebIDBConnectionToServer_DidDeleteRecord:
        return "WebIDBConnectionToServer::DidDeleteRecord";
    case MessageName::WebIDBConnectionToServer_DidOpenCursor:
        return "WebIDBConnectionToServer::DidOpenCursor";
    case MessageName::WebIDBConnectionToServer_DidIterateCursor:
        return "WebIDBConnectionToServer::DidIterateCursor";
    case MessageName::WebIDBConnectionToServer_FireVersionChangeEvent:
        return "WebIDBConnectionToServer::FireVersionChangeEvent";
    case MessageName::WebIDBConnectionToServer_DidStartTransaction:
        return "WebIDBConnectionToServer::DidStartTransaction";
    case MessageName::WebIDBConnectionToServer_DidCloseFromServer:
        return "WebIDBConnectionToServer::DidCloseFromServer";
    case MessageName::WebIDBConnectionToServer_NotifyOpenDBRequestBlocked:
        return "WebIDBConnectionToServer::NotifyOpenDBRequestBlocked";
    case MessageName::WebIDBConnectionToServer_DidGetAllDatabaseNamesAndVersions:
        return "WebIDBConnectionToServer::DidGetAllDatabaseNamesAndVersions";
    case MessageName::WebFullScreenManager_RequestExitFullScreen:
        return "WebFullScreenManager::RequestExitFullScreen";
    case MessageName::WebFullScreenManager_WillEnterFullScreen:
        return "WebFullScreenManager::WillEnterFullScreen";
    case MessageName::WebFullScreenManager_DidEnterFullScreen:
        return "WebFullScreenManager::DidEnterFullScreen";
    case MessageName::WebFullScreenManager_WillExitFullScreen:
        return "WebFullScreenManager::WillExitFullScreen";
    case MessageName::WebFullScreenManager_DidExitFullScreen:
        return "WebFullScreenManager::DidExitFullScreen";
    case MessageName::WebFullScreenManager_SetAnimatingFullScreen:
        return "WebFullScreenManager::SetAnimatingFullScreen";
    case MessageName::WebFullScreenManager_SaveScrollPosition:
        return "WebFullScreenManager::SaveScrollPosition";
    case MessageName::WebFullScreenManager_RestoreScrollPosition:
        return "WebFullScreenManager::RestoreScrollPosition";
    case MessageName::WebFullScreenManager_SetFullscreenInsets:
        return "WebFullScreenManager::SetFullscreenInsets";
    case MessageName::WebFullScreenManager_SetFullscreenAutoHideDuration:
        return "WebFullScreenManager::SetFullscreenAutoHideDuration";
    case MessageName::WebFullScreenManager_SetFullscreenControlsHidden:
        return "WebFullScreenManager::SetFullscreenControlsHidden";
    case MessageName::GPUProcessConnection_DidReceiveRemoteCommand:
        return "GPUProcessConnection::DidReceiveRemoteCommand";
    case MessageName::RemoteRenderingBackend_CreateImageBufferBackend:
        return "RemoteRenderingBackend::CreateImageBufferBackend";
    case MessageName::RemoteRenderingBackend_CommitImageBufferFlushContext:
        return "RemoteRenderingBackend::CommitImageBufferFlushContext";
    case MessageName::MediaPlayerPrivateRemote_NetworkStateChanged:
        return "MediaPlayerPrivateRemote::NetworkStateChanged";
    case MessageName::MediaPlayerPrivateRemote_ReadyStateChanged:
        return "MediaPlayerPrivateRemote::ReadyStateChanged";
    case MessageName::MediaPlayerPrivateRemote_FirstVideoFrameAvailable:
        return "MediaPlayerPrivateRemote::FirstVideoFrameAvailable";
    case MessageName::MediaPlayerPrivateRemote_VolumeChanged:
        return "MediaPlayerPrivateRemote::VolumeChanged";
    case MessageName::MediaPlayerPrivateRemote_MuteChanged:
        return "MediaPlayerPrivateRemote::MuteChanged";
    case MessageName::MediaPlayerPrivateRemote_TimeChanged:
        return "MediaPlayerPrivateRemote::TimeChanged";
    case MessageName::MediaPlayerPrivateRemote_DurationChanged:
        return "MediaPlayerPrivateRemote::DurationChanged";
    case MessageName::MediaPlayerPrivateRemote_RateChanged:
        return "MediaPlayerPrivateRemote::RateChanged";
    case MessageName::MediaPlayerPrivateRemote_PlaybackStateChanged:
        return "MediaPlayerPrivateRemote::PlaybackStateChanged";
    case MessageName::MediaPlayerPrivateRemote_EngineFailedToLoad:
        return "MediaPlayerPrivateRemote::EngineFailedToLoad";
    case MessageName::MediaPlayerPrivateRemote_UpdateCachedState:
        return "MediaPlayerPrivateRemote::UpdateCachedState";
    case MessageName::MediaPlayerPrivateRemote_CharacteristicChanged:
        return "MediaPlayerPrivateRemote::CharacteristicChanged";
    case MessageName::MediaPlayerPrivateRemote_SizeChanged:
        return "MediaPlayerPrivateRemote::SizeChanged";
    case MessageName::MediaPlayerPrivateRemote_AddRemoteAudioTrack:
        return "MediaPlayerPrivateRemote::AddRemoteAudioTrack";
    case MessageName::MediaPlayerPrivateRemote_RemoveRemoteAudioTrack:
        return "MediaPlayerPrivateRemote::RemoveRemoteAudioTrack";
    case MessageName::MediaPlayerPrivateRemote_RemoteAudioTrackConfigurationChanged:
        return "MediaPlayerPrivateRemote::RemoteAudioTrackConfigurationChanged";
    case MessageName::MediaPlayerPrivateRemote_AddRemoteTextTrack:
        return "MediaPlayerPrivateRemote::AddRemoteTextTrack";
    case MessageName::MediaPlayerPrivateRemote_RemoveRemoteTextTrack:
        return "MediaPlayerPrivateRemote::RemoveRemoteTextTrack";
    case MessageName::MediaPlayerPrivateRemote_RemoteTextTrackConfigurationChanged:
        return "MediaPlayerPrivateRemote::RemoteTextTrackConfigurationChanged";
    case MessageName::MediaPlayerPrivateRemote_ParseWebVTTFileHeader:
        return "MediaPlayerPrivateRemote::ParseWebVTTFileHeader";
    case MessageName::MediaPlayerPrivateRemote_ParseWebVTTCueData:
        return "MediaPlayerPrivateRemote::ParseWebVTTCueData";
    case MessageName::MediaPlayerPrivateRemote_ParseWebVTTCueDataStruct:
        return "MediaPlayerPrivateRemote::ParseWebVTTCueDataStruct";
    case MessageName::MediaPlayerPrivateRemote_AddDataCue:
        return "MediaPlayerPrivateRemote::AddDataCue";
#if ENABLE(DATACUE_VALUE)
    case MessageName::MediaPlayerPrivateRemote_AddDataCueWithType:
        return "MediaPlayerPrivateRemote::AddDataCueWithType";
#endif
#if ENABLE(DATACUE_VALUE)
    case MessageName::MediaPlayerPrivateRemote_UpdateDataCue:
        return "MediaPlayerPrivateRemote::UpdateDataCue";
#endif
#if ENABLE(DATACUE_VALUE)
    case MessageName::MediaPlayerPrivateRemote_RemoveDataCue:
        return "MediaPlayerPrivateRemote::RemoveDataCue";
#endif
    case MessageName::MediaPlayerPrivateRemote_AddGenericCue:
        return "MediaPlayerPrivateRemote::AddGenericCue";
    case MessageName::MediaPlayerPrivateRemote_UpdateGenericCue:
        return "MediaPlayerPrivateRemote::UpdateGenericCue";
    case MessageName::MediaPlayerPrivateRemote_RemoveGenericCue:
        return "MediaPlayerPrivateRemote::RemoveGenericCue";
    case MessageName::MediaPlayerPrivateRemote_AddRemoteVideoTrack:
        return "MediaPlayerPrivateRemote::AddRemoteVideoTrack";
    case MessageName::MediaPlayerPrivateRemote_RemoveRemoteVideoTrack:
        return "MediaPlayerPrivateRemote::RemoveRemoteVideoTrack";
    case MessageName::MediaPlayerPrivateRemote_RemoteVideoTrackConfigurationChanged:
        return "MediaPlayerPrivateRemote::RemoteVideoTrackConfigurationChanged";
    case MessageName::MediaPlayerPrivateRemote_RequestResource:
        return "MediaPlayerPrivateRemote::RequestResource";
    case MessageName::MediaPlayerPrivateRemote_RequestResourceReply:
        return "MediaPlayerPrivateRemote::RequestResourceReply";
    case MessageName::MediaPlayerPrivateRemote_RemoveResource:
        return "MediaPlayerPrivateRemote::RemoveResource";
    case MessageName::MediaPlayerPrivateRemote_ResourceNotSupported:
        return "MediaPlayerPrivateRemote::ResourceNotSupported";
    case MessageName::MediaPlayerPrivateRemote_EngineUpdated:
        return "MediaPlayerPrivateRemote::EngineUpdated";
    case MessageName::MediaPlayerPrivateRemote_ActiveSourceBuffersChanged:
        return "MediaPlayerPrivateRemote::ActiveSourceBuffersChanged";
#if ENABLE(ENCRYPTED_MEDIA)
    case MessageName::MediaPlayerPrivateRemote_WaitingForKeyChanged:
        return "MediaPlayerPrivateRemote::WaitingForKeyChanged";
#endif
#if ENABLE(ENCRYPTED_MEDIA)
    case MessageName::MediaPlayerPrivateRemote_InitializationDataEncountered:
        return "MediaPlayerPrivateRemote::InitializationDataEncountered";
#endif
#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    case MessageName::MediaPlayerPrivateRemote_MediaPlayerKeyNeeded:
        return "MediaPlayerPrivateRemote::MediaPlayerKeyNeeded";
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    case MessageName::MediaPlayerPrivateRemote_CurrentPlaybackTargetIsWirelessChanged:
        return "MediaPlayerPrivateRemote::CurrentPlaybackTargetIsWirelessChanged";
#endif
    case MessageName::RemoteAudioDestinationProxy_RenderBuffer:
        return "RemoteAudioDestinationProxy::RenderBuffer";
    case MessageName::RemoteAudioDestinationProxy_RenderBufferReply:
        return "RemoteAudioDestinationProxy::RenderBufferReply";
    case MessageName::RemoteAudioDestinationProxy_DidChangeIsPlaying:
        return "RemoteAudioDestinationProxy::DidChangeIsPlaying";
    case MessageName::RemoteAudioSession_ConfigurationChanged:
        return "RemoteAudioSession::ConfigurationChanged";
    case MessageName::RemoteAudioSession_BeginInterruption:
        return "RemoteAudioSession::BeginInterruption";
    case MessageName::RemoteAudioSession_EndInterruption:
        return "RemoteAudioSession::EndInterruption";
    case MessageName::RemoteCDMInstanceSession_UpdateKeyStatuses:
        return "RemoteCDMInstanceSession::UpdateKeyStatuses";
    case MessageName::RemoteCDMInstanceSession_SendMessage:
        return "RemoteCDMInstanceSession::SendMessage";
    case MessageName::RemoteCDMInstanceSession_SessionIdChanged:
        return "RemoteCDMInstanceSession::SessionIdChanged";
    case MessageName::RemoteLegacyCDMSession_SendMessage:
        return "RemoteLegacyCDMSession::SendMessage";
    case MessageName::RemoteLegacyCDMSession_SendError:
        return "RemoteLegacyCDMSession::SendError";
    case MessageName::LibWebRTCCodecs_FailedDecoding:
        return "LibWebRTCCodecs::FailedDecoding";
    case MessageName::LibWebRTCCodecs_CompletedDecoding:
        return "LibWebRTCCodecs::CompletedDecoding";
    case MessageName::LibWebRTCCodecs_CompletedEncoding:
        return "LibWebRTCCodecs::CompletedEncoding";
    case MessageName::SampleBufferDisplayLayer_SetDidFail:
        return "SampleBufferDisplayLayer::SetDidFail";
    case MessageName::WebGeolocationManager_DidChangePosition:
        return "WebGeolocationManager::DidChangePosition";
    case MessageName::WebGeolocationManager_DidFailToDeterminePosition:
        return "WebGeolocationManager::DidFailToDeterminePosition";
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebGeolocationManager_ResetPermissions:
        return "WebGeolocationManager::ResetPermissions";
#endif
    case MessageName::RemoteWebInspectorUI_Initialize:
        return "RemoteWebInspectorUI::Initialize";
    case MessageName::RemoteWebInspectorUI_UpdateFindString:
        return "RemoteWebInspectorUI::UpdateFindString";
#if ENABLE(INSPECTOR_TELEMETRY)
    case MessageName::RemoteWebInspectorUI_SetDiagnosticLoggingAvailable:
        return "RemoteWebInspectorUI::SetDiagnosticLoggingAvailable";
#endif
    case MessageName::RemoteWebInspectorUI_DidSave:
        return "RemoteWebInspectorUI::DidSave";
    case MessageName::RemoteWebInspectorUI_DidAppend:
        return "RemoteWebInspectorUI::DidAppend";
    case MessageName::RemoteWebInspectorUI_SendMessageToFrontend:
        return "RemoteWebInspectorUI::SendMessageToFrontend";
    case MessageName::WebInspector_Show:
        return "WebInspector::Show";
    case MessageName::WebInspector_Close:
        return "WebInspector::Close";
    case MessageName::WebInspector_SetAttached:
        return "WebInspector::SetAttached";
    case MessageName::WebInspector_ShowConsole:
        return "WebInspector::ShowConsole";
    case MessageName::WebInspector_ShowResources:
        return "WebInspector::ShowResources";
    case MessageName::WebInspector_ShowMainResourceForFrame:
        return "WebInspector::ShowMainResourceForFrame";
    case MessageName::WebInspector_OpenInNewTab:
        return "WebInspector::OpenInNewTab";
    case MessageName::WebInspector_StartPageProfiling:
        return "WebInspector::StartPageProfiling";
    case MessageName::WebInspector_StopPageProfiling:
        return "WebInspector::StopPageProfiling";
    case MessageName::WebInspector_StartElementSelection:
        return "WebInspector::StartElementSelection";
    case MessageName::WebInspector_StopElementSelection:
        return "WebInspector::StopElementSelection";
    case MessageName::WebInspector_SetFrontendConnection:
        return "WebInspector::SetFrontendConnection";
    case MessageName::WebInspectorInterruptDispatcher_NotifyNeedDebuggerBreak:
        return "WebInspectorInterruptDispatcher::NotifyNeedDebuggerBreak";
    case MessageName::WebInspectorUI_EstablishConnection:
        return "WebInspectorUI::EstablishConnection";
    case MessageName::WebInspectorUI_UpdateConnection:
        return "WebInspectorUI::UpdateConnection";
    case MessageName::WebInspectorUI_AttachedBottom:
        return "WebInspectorUI::AttachedBottom";
    case MessageName::WebInspectorUI_AttachedRight:
        return "WebInspectorUI::AttachedRight";
    case MessageName::WebInspectorUI_AttachedLeft:
        return "WebInspectorUI::AttachedLeft";
    case MessageName::WebInspectorUI_Detached:
        return "WebInspectorUI::Detached";
    case MessageName::WebInspectorUI_SetDockingUnavailable:
        return "WebInspectorUI::SetDockingUnavailable";
    case MessageName::WebInspectorUI_SetIsVisible:
        return "WebInspectorUI::SetIsVisible";
    case MessageName::WebInspectorUI_UpdateFindString:
        return "WebInspectorUI::UpdateFindString";
#if ENABLE(INSPECTOR_TELEMETRY)
    case MessageName::WebInspectorUI_SetDiagnosticLoggingAvailable:
        return "WebInspectorUI::SetDiagnosticLoggingAvailable";
#endif
    case MessageName::WebInspectorUI_ShowConsole:
        return "WebInspectorUI::ShowConsole";
    case MessageName::WebInspectorUI_ShowResources:
        return "WebInspectorUI::ShowResources";
    case MessageName::WebInspectorUI_ShowMainResourceForFrame:
        return "WebInspectorUI::ShowMainResourceForFrame";
    case MessageName::WebInspectorUI_StartPageProfiling:
        return "WebInspectorUI::StartPageProfiling";
    case MessageName::WebInspectorUI_StopPageProfiling:
        return "WebInspectorUI::StopPageProfiling";
    case MessageName::WebInspectorUI_StartElementSelection:
        return "WebInspectorUI::StartElementSelection";
    case MessageName::WebInspectorUI_StopElementSelection:
        return "WebInspectorUI::StopElementSelection";
    case MessageName::WebInspectorUI_DidSave:
        return "WebInspectorUI::DidSave";
    case MessageName::WebInspectorUI_DidAppend:
        return "WebInspectorUI::DidAppend";
    case MessageName::WebInspectorUI_SendMessageToFrontend:
        return "WebInspectorUI::SendMessageToFrontend";
    case MessageName::LibWebRTCNetwork_SignalReadPacket:
        return "LibWebRTCNetwork::SignalReadPacket";
    case MessageName::LibWebRTCNetwork_SignalSentPacket:
        return "LibWebRTCNetwork::SignalSentPacket";
    case MessageName::LibWebRTCNetwork_SignalAddressReady:
        return "LibWebRTCNetwork::SignalAddressReady";
    case MessageName::LibWebRTCNetwork_SignalConnect:
        return "LibWebRTCNetwork::SignalConnect";
    case MessageName::LibWebRTCNetwork_SignalClose:
        return "LibWebRTCNetwork::SignalClose";
    case MessageName::LibWebRTCNetwork_SignalNewConnection:
        return "LibWebRTCNetwork::SignalNewConnection";
    case MessageName::WebMDNSRegister_FinishedRegisteringMDNSName:
        return "WebMDNSRegister::FinishedRegisteringMDNSName";
    case MessageName::WebRTCMonitor_NetworksChanged:
        return "WebRTCMonitor::NetworksChanged";
    case MessageName::WebRTCResolver_SetResolvedAddress:
        return "WebRTCResolver::SetResolvedAddress";
    case MessageName::WebRTCResolver_ResolvedAddressError:
        return "WebRTCResolver::ResolvedAddressError";
#if ENABLE(SHAREABLE_RESOURCE)
    case MessageName::NetworkProcessConnection_DidCacheResource:
        return "NetworkProcessConnection::DidCacheResource";
#endif
    case MessageName::NetworkProcessConnection_DidFinishPingLoad:
        return "NetworkProcessConnection::DidFinishPingLoad";
    case MessageName::NetworkProcessConnection_DidFinishPreconnection:
        return "NetworkProcessConnection::DidFinishPreconnection";
    case MessageName::NetworkProcessConnection_SetOnLineState:
        return "NetworkProcessConnection::SetOnLineState";
    case MessageName::NetworkProcessConnection_CookieAcceptPolicyChanged:
        return "NetworkProcessConnection::CookieAcceptPolicyChanged";
#if HAVE(COOKIE_CHANGE_LISTENER_API)
    case MessageName::NetworkProcessConnection_CookiesAdded:
        return "NetworkProcessConnection::CookiesAdded";
#endif
#if HAVE(COOKIE_CHANGE_LISTENER_API)
    case MessageName::NetworkProcessConnection_CookiesDeleted:
        return "NetworkProcessConnection::CookiesDeleted";
#endif
#if HAVE(COOKIE_CHANGE_LISTENER_API)
    case MessageName::NetworkProcessConnection_AllCookiesDeleted:
        return "NetworkProcessConnection::AllCookiesDeleted";
#endif
    case MessageName::NetworkProcessConnection_CheckProcessLocalPortForActivity:
        return "NetworkProcessConnection::CheckProcessLocalPortForActivity";
    case MessageName::NetworkProcessConnection_CheckProcessLocalPortForActivityReply:
        return "NetworkProcessConnection::CheckProcessLocalPortForActivityReply";
    case MessageName::NetworkProcessConnection_MessagesAvailableForPort:
        return "NetworkProcessConnection::MessagesAvailableForPort";
    case MessageName::NetworkProcessConnection_BroadcastConsoleMessage:
        return "NetworkProcessConnection::BroadcastConsoleMessage";
    case MessageName::WebResourceLoader_WillSendRequest:
        return "WebResourceLoader::WillSendRequest";
    case MessageName::WebResourceLoader_DidSendData:
        return "WebResourceLoader::DidSendData";
    case MessageName::WebResourceLoader_DidReceiveResponse:
        return "WebResourceLoader::DidReceiveResponse";
    case MessageName::WebResourceLoader_DidReceiveData:
        return "WebResourceLoader::DidReceiveData";
    case MessageName::WebResourceLoader_DidReceiveSharedBuffer:
        return "WebResourceLoader::DidReceiveSharedBuffer";
    case MessageName::WebResourceLoader_DidFinishResourceLoad:
        return "WebResourceLoader::DidFinishResourceLoad";
    case MessageName::WebResourceLoader_DidFailResourceLoad:
        return "WebResourceLoader::DidFailResourceLoad";
    case MessageName::WebResourceLoader_DidFailServiceWorkerLoad:
        return "WebResourceLoader::DidFailServiceWorkerLoad";
    case MessageName::WebResourceLoader_ServiceWorkerDidNotHandle:
        return "WebResourceLoader::ServiceWorkerDidNotHandle";
    case MessageName::WebResourceLoader_DidBlockAuthenticationChallenge:
        return "WebResourceLoader::DidBlockAuthenticationChallenge";
    case MessageName::WebResourceLoader_StopLoadingAfterXFrameOptionsOrContentSecurityPolicyDenied:
        return "WebResourceLoader::StopLoadingAfterXFrameOptionsOrContentSecurityPolicyDenied";
#if ENABLE(SHAREABLE_RESOURCE)
    case MessageName::WebResourceLoader_DidReceiveResource:
        return "WebResourceLoader::DidReceiveResource";
#endif
    case MessageName::WebSocketChannel_DidConnect:
        return "WebSocketChannel::DidConnect";
    case MessageName::WebSocketChannel_DidClose:
        return "WebSocketChannel::DidClose";
    case MessageName::WebSocketChannel_DidReceiveText:
        return "WebSocketChannel::DidReceiveText";
    case MessageName::WebSocketChannel_DidReceiveBinaryData:
        return "WebSocketChannel::DidReceiveBinaryData";
    case MessageName::WebSocketChannel_DidReceiveMessageError:
        return "WebSocketChannel::DidReceiveMessageError";
    case MessageName::WebSocketChannel_DidSendHandshakeRequest:
        return "WebSocketChannel::DidSendHandshakeRequest";
    case MessageName::WebSocketChannel_DidReceiveHandshakeResponse:
        return "WebSocketChannel::DidReceiveHandshakeResponse";
    case MessageName::WebSocketStream_DidOpenSocketStream:
        return "WebSocketStream::DidOpenSocketStream";
    case MessageName::WebSocketStream_DidCloseSocketStream:
        return "WebSocketStream::DidCloseSocketStream";
    case MessageName::WebSocketStream_DidReceiveSocketStreamData:
        return "WebSocketStream::DidReceiveSocketStreamData";
    case MessageName::WebSocketStream_DidFailToReceiveSocketStreamData:
        return "WebSocketStream::DidFailToReceiveSocketStreamData";
    case MessageName::WebSocketStream_DidUpdateBufferedAmount:
        return "WebSocketStream::DidUpdateBufferedAmount";
    case MessageName::WebSocketStream_DidFailSocketStream:
        return "WebSocketStream::DidFailSocketStream";
    case MessageName::WebSocketStream_DidSendData:
        return "WebSocketStream::DidSendData";
    case MessageName::WebSocketStream_DidSendHandshake:
        return "WebSocketStream::DidSendHandshake";
    case MessageName::WebNotificationManager_DidShowNotification:
        return "WebNotificationManager::DidShowNotification";
    case MessageName::WebNotificationManager_DidClickNotification:
        return "WebNotificationManager::DidClickNotification";
    case MessageName::WebNotificationManager_DidCloseNotifications:
        return "WebNotificationManager::DidCloseNotifications";
    case MessageName::WebNotificationManager_DidUpdateNotificationDecision:
        return "WebNotificationManager::DidUpdateNotificationDecision";
    case MessageName::WebNotificationManager_DidRemoveNotificationDecisions:
        return "WebNotificationManager::DidRemoveNotificationDecisions";
    case MessageName::PluginProcessConnection_SetException:
        return "PluginProcessConnection::SetException";
    case MessageName::PluginProcessConnectionManager_PluginProcessCrashed:
        return "PluginProcessConnectionManager::PluginProcessCrashed";
    case MessageName::PluginProxy_LoadURL:
        return "PluginProxy::LoadURL";
    case MessageName::PluginProxy_Update:
        return "PluginProxy::Update";
    case MessageName::PluginProxy_ProxiesForURL:
        return "PluginProxy::ProxiesForURL";
    case MessageName::PluginProxy_CookiesForURL:
        return "PluginProxy::CookiesForURL";
    case MessageName::PluginProxy_SetCookiesForURL:
        return "PluginProxy::SetCookiesForURL";
    case MessageName::PluginProxy_GetAuthenticationInfo:
        return "PluginProxy::GetAuthenticationInfo";
    case MessageName::PluginProxy_GetPluginElementNPObject:
        return "PluginProxy::GetPluginElementNPObject";
    case MessageName::PluginProxy_Evaluate:
        return "PluginProxy::Evaluate";
    case MessageName::PluginProxy_CancelStreamLoad:
        return "PluginProxy::CancelStreamLoad";
    case MessageName::PluginProxy_ContinueStreamLoad:
        return "PluginProxy::ContinueStreamLoad";
    case MessageName::PluginProxy_CancelManualStreamLoad:
        return "PluginProxy::CancelManualStreamLoad";
    case MessageName::PluginProxy_SetStatusbarText:
        return "PluginProxy::SetStatusbarText";
#if PLATFORM(COCOA)
    case MessageName::PluginProxy_PluginFocusOrWindowFocusChanged:
        return "PluginProxy::PluginFocusOrWindowFocusChanged";
#endif
#if PLATFORM(COCOA)
    case MessageName::PluginProxy_SetComplexTextInputState:
        return "PluginProxy::SetComplexTextInputState";
#endif
#if PLATFORM(COCOA)
    case MessageName::PluginProxy_SetLayerHostingContextID:
        return "PluginProxy::SetLayerHostingContextID";
#endif
#if PLATFORM(X11)
    case MessageName::PluginProxy_CreatePluginContainer:
        return "PluginProxy::CreatePluginContainer";
#endif
#if PLATFORM(X11)
    case MessageName::PluginProxy_WindowedPluginGeometryDidChange:
        return "PluginProxy::WindowedPluginGeometryDidChange";
#endif
#if PLATFORM(X11)
    case MessageName::PluginProxy_WindowedPluginVisibilityDidChange:
        return "PluginProxy::WindowedPluginVisibilityDidChange";
#endif
    case MessageName::PluginProxy_DidCreatePlugin:
        return "PluginProxy::DidCreatePlugin";
    case MessageName::PluginProxy_DidFailToCreatePlugin:
        return "PluginProxy::DidFailToCreatePlugin";
    case MessageName::PluginProxy_SetPluginIsPlayingAudio:
        return "PluginProxy::SetPluginIsPlayingAudio";
    case MessageName::WebSWClientConnection_JobRejectedInServer:
        return "WebSWClientConnection::JobRejectedInServer";
    case MessageName::WebSWClientConnection_RegistrationJobResolvedInServer:
        return "WebSWClientConnection::RegistrationJobResolvedInServer";
    case MessageName::WebSWClientConnection_StartScriptFetchForServer:
        return "WebSWClientConnection::StartScriptFetchForServer";
    case MessageName::WebSWClientConnection_UpdateRegistrationState:
        return "WebSWClientConnection::UpdateRegistrationState";
    case MessageName::WebSWClientConnection_UpdateWorkerState:
        return "WebSWClientConnection::UpdateWorkerState";
    case MessageName::WebSWClientConnection_FireUpdateFoundEvent:
        return "WebSWClientConnection::FireUpdateFoundEvent";
    case MessageName::WebSWClientConnection_SetRegistrationLastUpdateTime:
        return "WebSWClientConnection::SetRegistrationLastUpdateTime";
    case MessageName::WebSWClientConnection_SetRegistrationUpdateViaCache:
        return "WebSWClientConnection::SetRegistrationUpdateViaCache";
    case MessageName::WebSWClientConnection_NotifyClientsOfControllerChange:
        return "WebSWClientConnection::NotifyClientsOfControllerChange";
    case MessageName::WebSWClientConnection_SetSWOriginTableIsImported:
        return "WebSWClientConnection::SetSWOriginTableIsImported";
    case MessageName::WebSWClientConnection_SetSWOriginTableSharedMemory:
        return "WebSWClientConnection::SetSWOriginTableSharedMemory";
    case MessageName::WebSWClientConnection_PostMessageToServiceWorkerClient:
        return "WebSWClientConnection::PostMessageToServiceWorkerClient";
    case MessageName::WebSWClientConnection_DidMatchRegistration:
        return "WebSWClientConnection::DidMatchRegistration";
    case MessageName::WebSWClientConnection_DidGetRegistrations:
        return "WebSWClientConnection::DidGetRegistrations";
    case MessageName::WebSWClientConnection_RegistrationReady:
        return "WebSWClientConnection::RegistrationReady";
    case MessageName::WebSWClientConnection_SetDocumentIsControlled:
        return "WebSWClientConnection::SetDocumentIsControlled";
    case MessageName::WebSWClientConnection_SetDocumentIsControlledReply:
        return "WebSWClientConnection::SetDocumentIsControlledReply";
    case MessageName::WebSWContextManagerConnection_InstallServiceWorker:
        return "WebSWContextManagerConnection::InstallServiceWorker";
    case MessageName::WebSWContextManagerConnection_StartFetch:
        return "WebSWContextManagerConnection::StartFetch";
    case MessageName::WebSWContextManagerConnection_CancelFetch:
        return "WebSWContextManagerConnection::CancelFetch";
    case MessageName::WebSWContextManagerConnection_ContinueDidReceiveFetchResponse:
        return "WebSWContextManagerConnection::ContinueDidReceiveFetchResponse";
    case MessageName::WebSWContextManagerConnection_PostMessageToServiceWorker:
        return "WebSWContextManagerConnection::PostMessageToServiceWorker";
    case MessageName::WebSWContextManagerConnection_FireInstallEvent:
        return "WebSWContextManagerConnection::FireInstallEvent";
    case MessageName::WebSWContextManagerConnection_FireActivateEvent:
        return "WebSWContextManagerConnection::FireActivateEvent";
    case MessageName::WebSWContextManagerConnection_TerminateWorker:
        return "WebSWContextManagerConnection::TerminateWorker";
    case MessageName::WebSWContextManagerConnection_FindClientByIdentifierCompleted:
        return "WebSWContextManagerConnection::FindClientByIdentifierCompleted";
    case MessageName::WebSWContextManagerConnection_MatchAllCompleted:
        return "WebSWContextManagerConnection::MatchAllCompleted";
    case MessageName::WebSWContextManagerConnection_SetUserAgent:
        return "WebSWContextManagerConnection::SetUserAgent";
    case MessageName::WebSWContextManagerConnection_UpdatePreferencesStore:
        return "WebSWContextManagerConnection::UpdatePreferencesStore";
    case MessageName::WebSWContextManagerConnection_Close:
        return "WebSWContextManagerConnection::Close";
    case MessageName::WebSWContextManagerConnection_SetThrottleState:
        return "WebSWContextManagerConnection::SetThrottleState";
    case MessageName::WebUserContentController_AddContentWorlds:
        return "WebUserContentController::AddContentWorlds";
    case MessageName::WebUserContentController_RemoveContentWorlds:
        return "WebUserContentController::RemoveContentWorlds";
    case MessageName::WebUserContentController_AddUserScripts:
        return "WebUserContentController::AddUserScripts";
    case MessageName::WebUserContentController_RemoveUserScript:
        return "WebUserContentController::RemoveUserScript";
    case MessageName::WebUserContentController_RemoveAllUserScripts:
        return "WebUserContentController::RemoveAllUserScripts";
    case MessageName::WebUserContentController_AddUserStyleSheets:
        return "WebUserContentController::AddUserStyleSheets";
    case MessageName::WebUserContentController_RemoveUserStyleSheet:
        return "WebUserContentController::RemoveUserStyleSheet";
    case MessageName::WebUserContentController_RemoveAllUserStyleSheets:
        return "WebUserContentController::RemoveAllUserStyleSheets";
    case MessageName::WebUserContentController_AddUserScriptMessageHandlers:
        return "WebUserContentController::AddUserScriptMessageHandlers";
    case MessageName::WebUserContentController_RemoveUserScriptMessageHandler:
        return "WebUserContentController::RemoveUserScriptMessageHandler";
    case MessageName::WebUserContentController_RemoveAllUserScriptMessageHandlersForWorlds:
        return "WebUserContentController::RemoveAllUserScriptMessageHandlersForWorlds";
    case MessageName::WebUserContentController_RemoveAllUserScriptMessageHandlers:
        return "WebUserContentController::RemoveAllUserScriptMessageHandlers";
#if ENABLE(CONTENT_EXTENSIONS)
    case MessageName::WebUserContentController_AddContentRuleLists:
        return "WebUserContentController::AddContentRuleLists";
#endif
#if ENABLE(CONTENT_EXTENSIONS)
    case MessageName::WebUserContentController_RemoveContentRuleList:
        return "WebUserContentController::RemoveContentRuleList";
#endif
#if ENABLE(CONTENT_EXTENSIONS)
    case MessageName::WebUserContentController_RemoveAllContentRuleLists:
        return "WebUserContentController::RemoveAllContentRuleLists";
#endif
#if USE(COORDINATED_GRAPHICS) || USE(TEXTURE_MAPPER)
    case MessageName::DrawingArea_UpdateBackingStoreState:
        return "DrawingArea::UpdateBackingStoreState";
#endif
    case MessageName::DrawingArea_DidUpdate:
        return "DrawingArea::DidUpdate";
#if PLATFORM(COCOA)
    case MessageName::DrawingArea_UpdateGeometry:
        return "DrawingArea::UpdateGeometry";
#endif
#if PLATFORM(COCOA)
    case MessageName::DrawingArea_SetDeviceScaleFactor:
        return "DrawingArea::SetDeviceScaleFactor";
#endif
#if PLATFORM(COCOA)
    case MessageName::DrawingArea_SetColorSpace:
        return "DrawingArea::SetColorSpace";
#endif
#if PLATFORM(COCOA)
    case MessageName::DrawingArea_SetViewExposedRect:
        return "DrawingArea::SetViewExposedRect";
#endif
#if PLATFORM(COCOA)
    case MessageName::DrawingArea_AdjustTransientZoom:
        return "DrawingArea::AdjustTransientZoom";
#endif
#if PLATFORM(COCOA)
    case MessageName::DrawingArea_CommitTransientZoom:
        return "DrawingArea::CommitTransientZoom";
#endif
#if PLATFORM(COCOA)
    case MessageName::DrawingArea_AcceleratedAnimationDidStart:
        return "DrawingArea::AcceleratedAnimationDidStart";
#endif
#if PLATFORM(COCOA)
    case MessageName::DrawingArea_AcceleratedAnimationDidEnd:
        return "DrawingArea::AcceleratedAnimationDidEnd";
#endif
#if PLATFORM(COCOA)
    case MessageName::DrawingArea_AddTransactionCallbackID:
        return "DrawingArea::AddTransactionCallbackID";
#endif
    case MessageName::EventDispatcher_WheelEvent:
        return "EventDispatcher::WheelEvent";
#if ENABLE(IOS_TOUCH_EVENTS)
    case MessageName::EventDispatcher_TouchEvent:
        return "EventDispatcher::TouchEvent";
#endif
#if ENABLE(MAC_GESTURE_EVENTS)
    case MessageName::EventDispatcher_GestureEvent:
        return "EventDispatcher::GestureEvent";
#endif
#if ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    case MessageName::EventDispatcher_DisplayWasRefreshed:
        return "EventDispatcher::DisplayWasRefreshed";
#endif
    case MessageName::VisitedLinkTableController_SetVisitedLinkTable:
        return "VisitedLinkTableController::SetVisitedLinkTable";
    case MessageName::VisitedLinkTableController_VisitedLinkStateChanged:
        return "VisitedLinkTableController::VisitedLinkStateChanged";
    case MessageName::VisitedLinkTableController_AllVisitedLinkStateChanged:
        return "VisitedLinkTableController::AllVisitedLinkStateChanged";
    case MessageName::VisitedLinkTableController_RemoveAllVisitedLinks:
        return "VisitedLinkTableController::RemoveAllVisitedLinks";
    case MessageName::WebPage_SetInitialFocus:
        return "WebPage::SetInitialFocus";
    case MessageName::WebPage_SetInitialFocusReply:
        return "WebPage::SetInitialFocusReply";
    case MessageName::WebPage_SetActivityState:
        return "WebPage::SetActivityState";
    case MessageName::WebPage_SetLayerHostingMode:
        return "WebPage::SetLayerHostingMode";
    case MessageName::WebPage_SetBackgroundColor:
        return "WebPage::SetBackgroundColor";
    case MessageName::WebPage_AddConsoleMessage:
        return "WebPage::AddConsoleMessage";
    case MessageName::WebPage_SendCSPViolationReport:
        return "WebPage::SendCSPViolationReport";
    case MessageName::WebPage_EnqueueSecurityPolicyViolationEvent:
        return "WebPage::EnqueueSecurityPolicyViolationEvent";
    case MessageName::WebPage_TestProcessIncomingSyncMessagesWhenWaitingForSyncReply:
        return "WebPage::TestProcessIncomingSyncMessagesWhenWaitingForSyncReply";
#if PLATFORM(COCOA)
    case MessageName::WebPage_SetTopContentInsetFenced:
        return "WebPage::SetTopContentInsetFenced";
#endif
    case MessageName::WebPage_SetTopContentInset:
        return "WebPage::SetTopContentInset";
    case MessageName::WebPage_SetUnderlayColor:
        return "WebPage::SetUnderlayColor";
    case MessageName::WebPage_ViewWillStartLiveResize:
        return "WebPage::ViewWillStartLiveResize";
    case MessageName::WebPage_ViewWillEndLiveResize:
        return "WebPage::ViewWillEndLiveResize";
    case MessageName::WebPage_ExecuteEditCommandWithCallback:
        return "WebPage::ExecuteEditCommandWithCallback";
    case MessageName::WebPage_ExecuteEditCommandWithCallbackReply:
        return "WebPage::ExecuteEditCommandWithCallbackReply";
    case MessageName::WebPage_KeyEvent:
        return "WebPage::KeyEvent";
    case MessageName::WebPage_MouseEvent:
        return "WebPage::MouseEvent";
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetViewportConfigurationViewLayoutSize:
        return "WebPage::SetViewportConfigurationViewLayoutSize";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetMaximumUnobscuredSize:
        return "WebPage::SetMaximumUnobscuredSize";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetDeviceOrientation:
        return "WebPage::SetDeviceOrientation";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetOverrideViewportArguments:
        return "WebPage::SetOverrideViewportArguments";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_DynamicViewportSizeUpdate:
        return "WebPage::DynamicViewportSizeUpdate";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetScreenIsBeingCaptured:
        return "WebPage::SetScreenIsBeingCaptured";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_HandleTap:
        return "WebPage::HandleTap";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_PotentialTapAtPosition:
        return "WebPage::PotentialTapAtPosition";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_CommitPotentialTap:
        return "WebPage::CommitPotentialTap";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_CancelPotentialTap:
        return "WebPage::CancelPotentialTap";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_TapHighlightAtPosition:
        return "WebPage::TapHighlightAtPosition";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_DidRecognizeLongPress:
        return "WebPage::DidRecognizeLongPress";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_HandleDoubleTapForDoubleClickAtPoint:
        return "WebPage::HandleDoubleTapForDoubleClickAtPoint";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_InspectorNodeSearchMovedToPosition:
        return "WebPage::InspectorNodeSearchMovedToPosition";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_InspectorNodeSearchEndedAtPosition:
        return "WebPage::InspectorNodeSearchEndedAtPosition";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_BlurFocusedElement:
        return "WebPage::BlurFocusedElement";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SelectWithGesture:
        return "WebPage::SelectWithGesture";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_UpdateSelectionWithTouches:
        return "WebPage::UpdateSelectionWithTouches";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SelectWithTwoTouches:
        return "WebPage::SelectWithTwoTouches";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ExtendSelection:
        return "WebPage::ExtendSelection";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SelectWordBackward:
        return "WebPage::SelectWordBackward";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_MoveSelectionByOffset:
        return "WebPage::MoveSelectionByOffset";
    case MessageName::WebPage_MoveSelectionByOffsetReply:
        return "WebPage::MoveSelectionByOffsetReply";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SelectTextWithGranularityAtPoint:
        return "WebPage::SelectTextWithGranularityAtPoint";
    case MessageName::WebPage_SelectTextWithGranularityAtPointReply:
        return "WebPage::SelectTextWithGranularityAtPointReply";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SelectPositionAtBoundaryWithDirection:
        return "WebPage::SelectPositionAtBoundaryWithDirection";
    case MessageName::WebPage_SelectPositionAtBoundaryWithDirectionReply:
        return "WebPage::SelectPositionAtBoundaryWithDirectionReply";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_MoveSelectionAtBoundaryWithDirection:
        return "WebPage::MoveSelectionAtBoundaryWithDirection";
    case MessageName::WebPage_MoveSelectionAtBoundaryWithDirectionReply:
        return "WebPage::MoveSelectionAtBoundaryWithDirectionReply";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SelectPositionAtPoint:
        return "WebPage::SelectPositionAtPoint";
    case MessageName::WebPage_SelectPositionAtPointReply:
        return "WebPage::SelectPositionAtPointReply";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_BeginSelectionInDirection:
        return "WebPage::BeginSelectionInDirection";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_UpdateSelectionWithExtentPoint:
        return "WebPage::UpdateSelectionWithExtentPoint";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_UpdateSelectionWithExtentPointAndBoundary:
        return "WebPage::UpdateSelectionWithExtentPointAndBoundary";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RequestDictationContext:
        return "WebPage::RequestDictationContext";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ReplaceDictatedText:
        return "WebPage::ReplaceDictatedText";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ReplaceSelectedText:
        return "WebPage::ReplaceSelectedText";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RequestAutocorrectionData:
        return "WebPage::RequestAutocorrectionData";
    case MessageName::WebPage_RequestAutocorrectionDataReply:
        return "WebPage::RequestAutocorrectionDataReply";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ApplyAutocorrection:
        return "WebPage::ApplyAutocorrection";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SyncApplyAutocorrection:
        return "WebPage::SyncApplyAutocorrection";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RequestAutocorrectionContext:
        return "WebPage::RequestAutocorrectionContext";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RequestEvasionRectsAboveSelection:
        return "WebPage::RequestEvasionRectsAboveSelection";
    case MessageName::WebPage_RequestEvasionRectsAboveSelectionReply:
        return "WebPage::RequestEvasionRectsAboveSelectionReply";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_GetPositionInformation:
        return "WebPage::GetPositionInformation";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RequestPositionInformation:
        return "WebPage::RequestPositionInformation";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_StartInteractionWithElementContextOrPosition:
        return "WebPage::StartInteractionWithElementContextOrPosition";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_StopInteraction:
        return "WebPage::StopInteraction";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_PerformActionOnElement:
        return "WebPage::PerformActionOnElement";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_FocusNextFocusedElement:
        return "WebPage::FocusNextFocusedElement";
    case MessageName::WebPage_FocusNextFocusedElementReply:
        return "WebPage::FocusNextFocusedElementReply";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetFocusedElementValue:
        return "WebPage::SetFocusedElementValue";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_AutofillLoginCredentials:
        return "WebPage::AutofillLoginCredentials";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetFocusedElementValueAsNumber:
        return "WebPage::SetFocusedElementValueAsNumber";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetFocusedElementSelectedIndex:
        return "WebPage::SetFocusedElementSelectedIndex";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ApplicationWillResignActive:
        return "WebPage::ApplicationWillResignActive";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ApplicationDidEnterBackground:
        return "WebPage::ApplicationDidEnterBackground";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ApplicationDidFinishSnapshottingAfterEnteringBackground:
        return "WebPage::ApplicationDidFinishSnapshottingAfterEnteringBackground";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ApplicationWillEnterForeground:
        return "WebPage::ApplicationWillEnterForeground";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ApplicationDidBecomeActive:
        return "WebPage::ApplicationDidBecomeActive";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ApplicationDidEnterBackgroundForMedia:
        return "WebPage::ApplicationDidEnterBackgroundForMedia";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ApplicationWillEnterForegroundForMedia:
        return "WebPage::ApplicationWillEnterForegroundForMedia";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ContentSizeCategoryDidChange:
        return "WebPage::ContentSizeCategoryDidChange";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_GetSelectionContext:
        return "WebPage::GetSelectionContext";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetAllowsMediaDocumentInlinePlayback:
        return "WebPage::SetAllowsMediaDocumentInlinePlayback";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_HandleTwoFingerTapAtPoint:
        return "WebPage::HandleTwoFingerTapAtPoint";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_HandleStylusSingleTapAtPoint:
        return "WebPage::HandleStylusSingleTapAtPoint";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetForceAlwaysUserScalable:
        return "WebPage::SetForceAlwaysUserScalable";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_GetRectsForGranularityWithSelectionOffset:
        return "WebPage::GetRectsForGranularityWithSelectionOffset";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_GetRectsAtSelectionOffsetWithText:
        return "WebPage::GetRectsAtSelectionOffsetWithText";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_StoreSelectionForAccessibility:
        return "WebPage::StoreSelectionForAccessibility";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_StartAutoscrollAtPosition:
        return "WebPage::StartAutoscrollAtPosition";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_CancelAutoscroll:
        return "WebPage::CancelAutoscroll";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RequestFocusedElementInformation:
        return "WebPage::RequestFocusedElementInformation";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_HardwareKeyboardAvailabilityChanged:
        return "WebPage::HardwareKeyboardAvailabilityChanged";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetIsShowingInputViewForFocusedElement:
        return "WebPage::SetIsShowingInputViewForFocusedElement";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_UpdateSelectionWithDelta:
        return "WebPage::UpdateSelectionWithDelta";
    case MessageName::WebPage_UpdateSelectionWithDeltaReply:
        return "WebPage::UpdateSelectionWithDeltaReply";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RequestDocumentEditingContext:
        return "WebPage::RequestDocumentEditingContext";
    case MessageName::WebPage_RequestDocumentEditingContextReply:
        return "WebPage::RequestDocumentEditingContextReply";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_GenerateSyntheticEditingCommand:
        return "WebPage::GenerateSyntheticEditingCommand";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetShouldRevealCurrentSelectionAfterInsertion:
        return "WebPage::SetShouldRevealCurrentSelectionAfterInsertion";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_InsertTextPlaceholder:
        return "WebPage::InsertTextPlaceholder";
    case MessageName::WebPage_InsertTextPlaceholderReply:
        return "WebPage::InsertTextPlaceholderReply";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RemoveTextPlaceholder:
        return "WebPage::RemoveTextPlaceholder";
    case MessageName::WebPage_RemoveTextPlaceholderReply:
        return "WebPage::RemoveTextPlaceholderReply";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_TextInputContextsInRect:
        return "WebPage::TextInputContextsInRect";
    case MessageName::WebPage_TextInputContextsInRectReply:
        return "WebPage::TextInputContextsInRectReply";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_FocusTextInputContextAndPlaceCaret:
        return "WebPage::FocusTextInputContextAndPlaceCaret";
    case MessageName::WebPage_FocusTextInputContextAndPlaceCaretReply:
        return "WebPage::FocusTextInputContextAndPlaceCaretReply";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ClearServiceWorkerEntitlementOverride:
        return "WebPage::ClearServiceWorkerEntitlementOverride";
    case MessageName::WebPage_ClearServiceWorkerEntitlementOverrideReply:
        return "WebPage::ClearServiceWorkerEntitlementOverrideReply";
#endif
    case MessageName::WebPage_SetControlledByAutomation:
        return "WebPage::SetControlledByAutomation";
    case MessageName::WebPage_ConnectInspector:
        return "WebPage::ConnectInspector";
    case MessageName::WebPage_DisconnectInspector:
        return "WebPage::DisconnectInspector";
    case MessageName::WebPage_SendMessageToTargetBackend:
        return "WebPage::SendMessageToTargetBackend";
#if ENABLE(REMOTE_INSPECTOR)
    case MessageName::WebPage_SetIndicating:
        return "WebPage::SetIndicating";
#endif
#if ENABLE(IOS_TOUCH_EVENTS)
    case MessageName::WebPage_ResetPotentialTapSecurityOrigin:
        return "WebPage::ResetPotentialTapSecurityOrigin";
#endif
#if ENABLE(IOS_TOUCH_EVENTS)
    case MessageName::WebPage_TouchEventSync:
        return "WebPage::TouchEventSync";
#endif
#if !ENABLE(IOS_TOUCH_EVENTS) && ENABLE(TOUCH_EVENTS)
    case MessageName::WebPage_TouchEvent:
        return "WebPage::TouchEvent";
#endif
    case MessageName::WebPage_CancelPointer:
        return "WebPage::CancelPointer";
    case MessageName::WebPage_TouchWithIdentifierWasRemoved:
        return "WebPage::TouchWithIdentifierWasRemoved";
#if ENABLE(INPUT_TYPE_COLOR)
    case MessageName::WebPage_DidEndColorPicker:
        return "WebPage::DidEndColorPicker";
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    case MessageName::WebPage_DidChooseColor:
        return "WebPage::DidChooseColor";
#endif
#if ENABLE(DATALIST_ELEMENT)
    case MessageName::WebPage_DidSelectDataListOption:
        return "WebPage::DidSelectDataListOption";
#endif
#if ENABLE(DATALIST_ELEMENT)
    case MessageName::WebPage_DidCloseSuggestions:
        return "WebPage::DidCloseSuggestions";
#endif
#if ENABLE(CONTEXT_MENUS)
    case MessageName::WebPage_ContextMenuHidden:
        return "WebPage::ContextMenuHidden";
#endif
#if ENABLE(CONTEXT_MENUS)
    case MessageName::WebPage_ContextMenuForKeyEvent:
        return "WebPage::ContextMenuForKeyEvent";
#endif
    case MessageName::WebPage_ScrollBy:
        return "WebPage::ScrollBy";
    case MessageName::WebPage_CenterSelectionInVisibleArea:
        return "WebPage::CenterSelectionInVisibleArea";
    case MessageName::WebPage_GoToBackForwardItem:
        return "WebPage::GoToBackForwardItem";
    case MessageName::WebPage_TryRestoreScrollPosition:
        return "WebPage::TryRestoreScrollPosition";
    case MessageName::WebPage_LoadURLInFrame:
        return "WebPage::LoadURLInFrame";
    case MessageName::WebPage_LoadDataInFrame:
        return "WebPage::LoadDataInFrame";
    case MessageName::WebPage_LoadRequest:
        return "WebPage::LoadRequest";
    case MessageName::WebPage_LoadRequestWaitingForProcessLaunch:
        return "WebPage::LoadRequestWaitingForProcessLaunch";
    case MessageName::WebPage_LoadData:
        return "WebPage::LoadData";
    case MessageName::WebPage_LoadAlternateHTML:
        return "WebPage::LoadAlternateHTML";
    case MessageName::WebPage_NavigateToPDFLinkWithSimulatedClick:
        return "WebPage::NavigateToPDFLinkWithSimulatedClick";
    case MessageName::WebPage_Reload:
        return "WebPage::Reload";
    case MessageName::WebPage_StopLoading:
        return "WebPage::StopLoading";
    case MessageName::WebPage_StopLoadingFrame:
        return "WebPage::StopLoadingFrame";
    case MessageName::WebPage_RestoreSession:
        return "WebPage::RestoreSession";
    case MessageName::WebPage_UpdateBackForwardListForReattach:
        return "WebPage::UpdateBackForwardListForReattach";
    case MessageName::WebPage_SetCurrentHistoryItemForReattach:
        return "WebPage::SetCurrentHistoryItemForReattach";
    case MessageName::WebPage_DidRemoveBackForwardItem:
        return "WebPage::DidRemoveBackForwardItem";
    case MessageName::WebPage_UpdateWebsitePolicies:
        return "WebPage::UpdateWebsitePolicies";
    case MessageName::WebPage_NotifyUserScripts:
        return "WebPage::NotifyUserScripts";
    case MessageName::WebPage_DidReceivePolicyDecision:
        return "WebPage::DidReceivePolicyDecision";
    case MessageName::WebPage_ContinueWillSubmitForm:
        return "WebPage::ContinueWillSubmitForm";
    case MessageName::WebPage_ClearSelection:
        return "WebPage::ClearSelection";
    case MessageName::WebPage_RestoreSelectionInFocusedEditableElement:
        return "WebPage::RestoreSelectionInFocusedEditableElement";
    case MessageName::WebPage_GetContentsAsString:
        return "WebPage::GetContentsAsString";
    case MessageName::WebPage_GetAllFrames:
        return "WebPage::GetAllFrames";
    case MessageName::WebPage_GetAllFramesReply:
        return "WebPage::GetAllFramesReply";
#if PLATFORM(COCOA)
    case MessageName::WebPage_GetContentsAsAttributedString:
        return "WebPage::GetContentsAsAttributedString";
    case MessageName::WebPage_GetContentsAsAttributedStringReply:
        return "WebPage::GetContentsAsAttributedStringReply";
#endif
#if ENABLE(MHTML)
    case MessageName::WebPage_GetContentsAsMHTMLData:
        return "WebPage::GetContentsAsMHTMLData";
#endif
    case MessageName::WebPage_GetMainResourceDataOfFrame:
        return "WebPage::GetMainResourceDataOfFrame";
    case MessageName::WebPage_GetResourceDataFromFrame:
        return "WebPage::GetResourceDataFromFrame";
    case MessageName::WebPage_GetRenderTreeExternalRepresentation:
        return "WebPage::GetRenderTreeExternalRepresentation";
    case MessageName::WebPage_GetSelectionOrContentsAsString:
        return "WebPage::GetSelectionOrContentsAsString";
    case MessageName::WebPage_GetSelectionAsWebArchiveData:
        return "WebPage::GetSelectionAsWebArchiveData";
    case MessageName::WebPage_GetSourceForFrame:
        return "WebPage::GetSourceForFrame";
    case MessageName::WebPage_GetWebArchiveOfFrame:
        return "WebPage::GetWebArchiveOfFrame";
    case MessageName::WebPage_RunJavaScriptInFrameInScriptWorld:
        return "WebPage::RunJavaScriptInFrameInScriptWorld";
    case MessageName::WebPage_ForceRepaint:
        return "WebPage::ForceRepaint";
    case MessageName::WebPage_SelectAll:
        return "WebPage::SelectAll";
    case MessageName::WebPage_ScheduleFullEditorStateUpdate:
        return "WebPage::ScheduleFullEditorStateUpdate";
#if PLATFORM(COCOA)
    case MessageName::WebPage_PerformDictionaryLookupOfCurrentSelection:
        return "WebPage::PerformDictionaryLookupOfCurrentSelection";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_PerformDictionaryLookupAtLocation:
        return "WebPage::PerformDictionaryLookupAtLocation";
#endif
#if ENABLE(DATA_DETECTION)
    case MessageName::WebPage_DetectDataInAllFrames:
        return "WebPage::DetectDataInAllFrames";
    case MessageName::WebPage_DetectDataInAllFramesReply:
        return "WebPage::DetectDataInAllFramesReply";
#endif
#if ENABLE(DATA_DETECTION)
    case MessageName::WebPage_RemoveDataDetectedLinks:
        return "WebPage::RemoveDataDetectedLinks";
    case MessageName::WebPage_RemoveDataDetectedLinksReply:
        return "WebPage::RemoveDataDetectedLinksReply";
#endif
    case MessageName::WebPage_ChangeFont:
        return "WebPage::ChangeFont";
    case MessageName::WebPage_ChangeFontAttributes:
        return "WebPage::ChangeFontAttributes";
    case MessageName::WebPage_PreferencesDidChange:
        return "WebPage::PreferencesDidChange";
    case MessageName::WebPage_SetUserAgent:
        return "WebPage::SetUserAgent";
    case MessageName::WebPage_SetCustomTextEncodingName:
        return "WebPage::SetCustomTextEncodingName";
    case MessageName::WebPage_SuspendActiveDOMObjectsAndAnimations:
        return "WebPage::SuspendActiveDOMObjectsAndAnimations";
    case MessageName::WebPage_ResumeActiveDOMObjectsAndAnimations:
        return "WebPage::ResumeActiveDOMObjectsAndAnimations";
    case MessageName::WebPage_Close:
        return "WebPage::Close";
    case MessageName::WebPage_TryClose:
        return "WebPage::TryClose";
    case MessageName::WebPage_TryCloseReply:
        return "WebPage::TryCloseReply";
    case MessageName::WebPage_SetEditable:
        return "WebPage::SetEditable";
    case MessageName::WebPage_ValidateCommand:
        return "WebPage::ValidateCommand";
    case MessageName::WebPage_ExecuteEditCommand:
        return "WebPage::ExecuteEditCommand";
    case MessageName::WebPage_IncreaseListLevel:
        return "WebPage::IncreaseListLevel";
    case MessageName::WebPage_DecreaseListLevel:
        return "WebPage::DecreaseListLevel";
    case MessageName::WebPage_ChangeListType:
        return "WebPage::ChangeListType";
    case MessageName::WebPage_SetBaseWritingDirection:
        return "WebPage::SetBaseWritingDirection";
    case MessageName::WebPage_SetNeedsFontAttributes:
        return "WebPage::SetNeedsFontAttributes";
    case MessageName::WebPage_RequestFontAttributesAtSelectionStart:
        return "WebPage::RequestFontAttributesAtSelectionStart";
    case MessageName::WebPage_DidRemoveEditCommand:
        return "WebPage::DidRemoveEditCommand";
    case MessageName::WebPage_ReapplyEditCommand:
        return "WebPage::ReapplyEditCommand";
    case MessageName::WebPage_UnapplyEditCommand:
        return "WebPage::UnapplyEditCommand";
    case MessageName::WebPage_SetPageAndTextZoomFactors:
        return "WebPage::SetPageAndTextZoomFactors";
    case MessageName::WebPage_SetPageZoomFactor:
        return "WebPage::SetPageZoomFactor";
    case MessageName::WebPage_SetTextZoomFactor:
        return "WebPage::SetTextZoomFactor";
    case MessageName::WebPage_WindowScreenDidChange:
        return "WebPage::WindowScreenDidChange";
    case MessageName::WebPage_AccessibilitySettingsDidChange:
        return "WebPage::AccessibilitySettingsDidChange";
    case MessageName::WebPage_ScalePage:
        return "WebPage::ScalePage";
    case MessageName::WebPage_ScalePageInViewCoordinates:
        return "WebPage::ScalePageInViewCoordinates";
    case MessageName::WebPage_ScaleView:
        return "WebPage::ScaleView";
    case MessageName::WebPage_SetUseFixedLayout:
        return "WebPage::SetUseFixedLayout";
    case MessageName::WebPage_SetFixedLayoutSize:
        return "WebPage::SetFixedLayoutSize";
    case MessageName::WebPage_ListenForLayoutMilestones:
        return "WebPage::ListenForLayoutMilestones";
    case MessageName::WebPage_SetSuppressScrollbarAnimations:
        return "WebPage::SetSuppressScrollbarAnimations";
    case MessageName::WebPage_SetEnableVerticalRubberBanding:
        return "WebPage::SetEnableVerticalRubberBanding";
    case MessageName::WebPage_SetEnableHorizontalRubberBanding:
        return "WebPage::SetEnableHorizontalRubberBanding";
    case MessageName::WebPage_SetBackgroundExtendsBeyondPage:
        return "WebPage::SetBackgroundExtendsBeyondPage";
    case MessageName::WebPage_SetPaginationMode:
        return "WebPage::SetPaginationMode";
    case MessageName::WebPage_SetPaginationBehavesLikeColumns:
        return "WebPage::SetPaginationBehavesLikeColumns";
    case MessageName::WebPage_SetPageLength:
        return "WebPage::SetPageLength";
    case MessageName::WebPage_SetGapBetweenPages:
        return "WebPage::SetGapBetweenPages";
    case MessageName::WebPage_SetPaginationLineGridEnabled:
        return "WebPage::SetPaginationLineGridEnabled";
    case MessageName::WebPage_PostInjectedBundleMessage:
        return "WebPage::PostInjectedBundleMessage";
    case MessageName::WebPage_FindString:
        return "WebPage::FindString";
    case MessageName::WebPage_FindStringMatches:
        return "WebPage::FindStringMatches";
    case MessageName::WebPage_GetImageForFindMatch:
        return "WebPage::GetImageForFindMatch";
    case MessageName::WebPage_SelectFindMatch:
        return "WebPage::SelectFindMatch";
    case MessageName::WebPage_IndicateFindMatch:
        return "WebPage::IndicateFindMatch";
    case MessageName::WebPage_HideFindUI:
        return "WebPage::HideFindUI";
    case MessageName::WebPage_CountStringMatches:
        return "WebPage::CountStringMatches";
    case MessageName::WebPage_ReplaceMatches:
        return "WebPage::ReplaceMatches";
    case MessageName::WebPage_AddMIMETypeWithCustomContentProvider:
        return "WebPage::AddMIMETypeWithCustomContentProvider";
#if (PLATFORM(GTK) || PLATFORM(HBD)) && ENABLE(DRAG_SUPPORT)
    case MessageName::WebPage_PerformDragControllerAction:
        return "WebPage::PerformDragControllerAction";
#endif
#if !PLATFORM(GTK) && !PLATFORM(HBD) && ENABLE(DRAG_SUPPORT)
    case MessageName::WebPage_PerformDragControllerAction:
        return "WebPage::PerformDragControllerAction";
#endif
#if ENABLE(DRAG_SUPPORT)
    case MessageName::WebPage_DidStartDrag:
        return "WebPage::DidStartDrag";
#endif
#if ENABLE(DRAG_SUPPORT)
    case MessageName::WebPage_DragEnded:
        return "WebPage::DragEnded";
#endif
#if ENABLE(DRAG_SUPPORT)
    case MessageName::WebPage_DragCancelled:
        return "WebPage::DragCancelled";
#endif
#if PLATFORM(IOS_FAMILY) && ENABLE(DRAG_SUPPORT)
    case MessageName::WebPage_RequestDragStart:
        return "WebPage::RequestDragStart";
#endif
#if PLATFORM(IOS_FAMILY) && ENABLE(DRAG_SUPPORT)
    case MessageName::WebPage_RequestAdditionalItemsForDragSession:
        return "WebPage::RequestAdditionalItemsForDragSession";
#endif
#if PLATFORM(IOS_FAMILY) && ENABLE(DRAG_SUPPORT)
    case MessageName::WebPage_InsertDroppedImagePlaceholders:
        return "WebPage::InsertDroppedImagePlaceholders";
    case MessageName::WebPage_InsertDroppedImagePlaceholdersReply:
        return "WebPage::InsertDroppedImagePlaceholdersReply";
#endif
#if PLATFORM(IOS_FAMILY) && ENABLE(DRAG_SUPPORT)
    case MessageName::WebPage_DidConcludeDrop:
        return "WebPage::DidConcludeDrop";
#endif
    case MessageName::WebPage_DidChangeSelectedIndexForActivePopupMenu:
        return "WebPage::DidChangeSelectedIndexForActivePopupMenu";
    case MessageName::WebPage_SetTextForActivePopupMenu:
        return "WebPage::SetTextForActivePopupMenu";
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPage_FailedToShowPopupMenu:
        return "WebPage::FailedToShowPopupMenu";
#endif
#if ENABLE(CONTEXT_MENUS)
    case MessageName::WebPage_DidSelectItemFromActiveContextMenu:
        return "WebPage::DidSelectItemFromActiveContextMenu";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_DidChooseFilesForOpenPanelWithDisplayStringAndIcon:
        return "WebPage::DidChooseFilesForOpenPanelWithDisplayStringAndIcon";
#endif
    case MessageName::WebPage_DidChooseFilesForOpenPanel:
        return "WebPage::DidChooseFilesForOpenPanel";
    case MessageName::WebPage_DidCancelForOpenPanel:
        return "WebPage::DidCancelForOpenPanel";
#if ENABLE(SANDBOX_EXTENSIONS)
    case MessageName::WebPage_ExtendSandboxForFilesFromOpenPanel:
        return "WebPage::ExtendSandboxForFilesFromOpenPanel";
#endif
    case MessageName::WebPage_AdvanceToNextMisspelling:
        return "WebPage::AdvanceToNextMisspelling";
    case MessageName::WebPage_ChangeSpellingToWord:
        return "WebPage::ChangeSpellingToWord";
    case MessageName::WebPage_DidFinishCheckingText:
        return "WebPage::DidFinishCheckingText";
    case MessageName::WebPage_DidCancelCheckingText:
        return "WebPage::DidCancelCheckingText";
#if USE(APPKIT)
    case MessageName::WebPage_UppercaseWord:
        return "WebPage::UppercaseWord";
#endif
#if USE(APPKIT)
    case MessageName::WebPage_LowercaseWord:
        return "WebPage::LowercaseWord";
#endif
#if USE(APPKIT)
    case MessageName::WebPage_CapitalizeWord:
        return "WebPage::CapitalizeWord";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_SetSmartInsertDeleteEnabled:
        return "WebPage::SetSmartInsertDeleteEnabled";
#endif
#if ENABLE(GEOLOCATION)
    case MessageName::WebPage_DidReceiveGeolocationPermissionDecision:
        return "WebPage::DidReceiveGeolocationPermissionDecision";
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebPage_UserMediaAccessWasGranted:
        return "WebPage::UserMediaAccessWasGranted";
    case MessageName::WebPage_UserMediaAccessWasGrantedReply:
        return "WebPage::UserMediaAccessWasGrantedReply";
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebPage_UserMediaAccessWasDenied:
        return "WebPage::UserMediaAccessWasDenied";
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebPage_CaptureDevicesChanged:
        return "WebPage::CaptureDevicesChanged";
#endif
    case MessageName::WebPage_StopAllMediaPlayback:
        return "WebPage::StopAllMediaPlayback";
    case MessageName::WebPage_SuspendAllMediaPlayback:
        return "WebPage::SuspendAllMediaPlayback";
    case MessageName::WebPage_ResumeAllMediaPlayback:
        return "WebPage::ResumeAllMediaPlayback";
    case MessageName::WebPage_DidReceiveNotificationPermissionDecision:
        return "WebPage::DidReceiveNotificationPermissionDecision";
    case MessageName::WebPage_FreezeLayerTreeDueToSwipeAnimation:
        return "WebPage::FreezeLayerTreeDueToSwipeAnimation";
    case MessageName::WebPage_UnfreezeLayerTreeDueToSwipeAnimation:
        return "WebPage::UnfreezeLayerTreeDueToSwipeAnimation";
    case MessageName::WebPage_BeginPrinting:
        return "WebPage::BeginPrinting";
    case MessageName::WebPage_EndPrinting:
        return "WebPage::EndPrinting";
    case MessageName::WebPage_ComputePagesForPrinting:
        return "WebPage::ComputePagesForPrinting";
#if PLATFORM(COCOA)
    case MessageName::WebPage_DrawRectToImage:
        return "WebPage::DrawRectToImage";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_DrawPagesToPDF:
        return "WebPage::DrawPagesToPDF";
#endif
#if (PLATFORM(COCOA) && PLATFORM(IOS_FAMILY))
    case MessageName::WebPage_ComputePagesForPrintingAndDrawToPDF:
        return "WebPage::ComputePagesForPrintingAndDrawToPDF";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_DrawToPDF:
        return "WebPage::DrawToPDF";
#endif
#if PLATFORM(GTK)
    case MessageName::WebPage_DrawPagesForPrinting:
        return "WebPage::DrawPagesForPrinting";
#endif
    case MessageName::WebPage_SetMediaVolume:
        return "WebPage::SetMediaVolume";
    case MessageName::WebPage_SetMuted:
        return "WebPage::SetMuted";
    case MessageName::WebPage_SetMayStartMediaWhenInWindow:
        return "WebPage::SetMayStartMediaWhenInWindow";
    case MessageName::WebPage_StopMediaCapture:
        return "WebPage::StopMediaCapture";
#if ENABLE(MEDIA_SESSION)
    case MessageName::WebPage_HandleMediaEvent:
        return "WebPage::HandleMediaEvent";
#endif
#if ENABLE(MEDIA_SESSION)
    case MessageName::WebPage_SetVolumeOfMediaElement:
        return "WebPage::SetVolumeOfMediaElement";
#endif
    case MessageName::WebPage_SetCanRunBeforeUnloadConfirmPanel:
        return "WebPage::SetCanRunBeforeUnloadConfirmPanel";
    case MessageName::WebPage_SetCanRunModal:
        return "WebPage::SetCanRunModal";
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    case MessageName::WebPage_CancelComposition:
        return "WebPage::CancelComposition";
#endif
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    case MessageName::WebPage_DeleteSurrounding:
        return "WebPage::DeleteSurrounding";
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPage_CollapseSelectionInFrame:
        return "WebPage::CollapseSelectionInFrame";
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPage_GetCenterForZoomGesture:
        return "WebPage::GetCenterForZoomGesture";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_SendComplexTextInputToPlugin:
        return "WebPage::SendComplexTextInputToPlugin";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_WindowAndViewFramesChanged:
        return "WebPage::WindowAndViewFramesChanged";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_SetMainFrameIsScrollable:
        return "WebPage::SetMainFrameIsScrollable";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_RegisterUIProcessAccessibilityTokens:
        return "WebPage::RegisterUIProcessAccessibilityTokens";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_GetStringSelectionForPasteboard:
        return "WebPage::GetStringSelectionForPasteboard";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_GetDataSelectionForPasteboard:
        return "WebPage::GetDataSelectionForPasteboard";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_ReadSelectionFromPasteboard:
        return "WebPage::ReadSelectionFromPasteboard";
#endif
#if (PLATFORM(COCOA) && ENABLE(SERVICE_CONTROLS))
    case MessageName::WebPage_ReplaceSelectionWithPasteboardData:
        return "WebPage::ReplaceSelectionWithPasteboardData";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_ShouldDelayWindowOrderingEvent:
        return "WebPage::ShouldDelayWindowOrderingEvent";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_AcceptsFirstMouse:
        return "WebPage::AcceptsFirstMouse";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_SetTextAsync:
        return "WebPage::SetTextAsync";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_InsertTextAsync:
        return "WebPage::InsertTextAsync";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_InsertDictatedTextAsync:
        return "WebPage::InsertDictatedTextAsync";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_HasMarkedText:
        return "WebPage::HasMarkedText";
    case MessageName::WebPage_HasMarkedTextReply:
        return "WebPage::HasMarkedTextReply";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_GetMarkedRangeAsync:
        return "WebPage::GetMarkedRangeAsync";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_GetSelectedRangeAsync:
        return "WebPage::GetSelectedRangeAsync";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_CharacterIndexForPointAsync:
        return "WebPage::CharacterIndexForPointAsync";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_FirstRectForCharacterRangeAsync:
        return "WebPage::FirstRectForCharacterRangeAsync";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_SetCompositionAsync:
        return "WebPage::SetCompositionAsync";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_ConfirmCompositionAsync:
        return "WebPage::ConfirmCompositionAsync";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_AttributedSubstringForCharacterRangeAsync:
        return "WebPage::AttributedSubstringForCharacterRangeAsync";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_FontAtSelection:
        return "WebPage::FontAtSelection";
#endif
    case MessageName::WebPage_SetAlwaysShowsHorizontalScroller:
        return "WebPage::SetAlwaysShowsHorizontalScroller";
    case MessageName::WebPage_SetAlwaysShowsVerticalScroller:
        return "WebPage::SetAlwaysShowsVerticalScroller";
    case MessageName::WebPage_SetMinimumSizeForAutoLayout:
        return "WebPage::SetMinimumSizeForAutoLayout";
    case MessageName::WebPage_SetSizeToContentAutoSizeMaximumSize:
        return "WebPage::SetSizeToContentAutoSizeMaximumSize";
    case MessageName::WebPage_SetAutoSizingShouldExpandToViewHeight:
        return "WebPage::SetAutoSizingShouldExpandToViewHeight";
    case MessageName::WebPage_SetViewportSizeForCSSViewportUnits:
        return "WebPage::SetViewportSizeForCSSViewportUnits";
#if PLATFORM(COCOA)
    case MessageName::WebPage_HandleAlternativeTextUIResult:
        return "WebPage::HandleAlternativeTextUIResult";
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_WillStartUserTriggeredZooming:
        return "WebPage::WillStartUserTriggeredZooming";
#endif
    case MessageName::WebPage_SetScrollPinningBehavior:
        return "WebPage::SetScrollPinningBehavior";
    case MessageName::WebPage_SetScrollbarOverlayStyle:
        return "WebPage::SetScrollbarOverlayStyle";
    case MessageName::WebPage_GetBytecodeProfile:
        return "WebPage::GetBytecodeProfile";
    case MessageName::WebPage_GetSamplingProfilerOutput:
        return "WebPage::GetSamplingProfilerOutput";
    case MessageName::WebPage_TakeSnapshot:
        return "WebPage::TakeSnapshot";
#if PLATFORM(MAC)
    case MessageName::WebPage_PerformImmediateActionHitTestAtLocation:
        return "WebPage::PerformImmediateActionHitTestAtLocation";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_ImmediateActionDidUpdate:
        return "WebPage::ImmediateActionDidUpdate";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_ImmediateActionDidCancel:
        return "WebPage::ImmediateActionDidCancel";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_ImmediateActionDidComplete:
        return "WebPage::ImmediateActionDidComplete";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_DataDetectorsDidPresentUI:
        return "WebPage::DataDetectorsDidPresentUI";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_DataDetectorsDidChangeUI:
        return "WebPage::DataDetectorsDidChangeUI";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_DataDetectorsDidHideUI:
        return "WebPage::DataDetectorsDidHideUI";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_HandleAcceptedCandidate:
        return "WebPage::HandleAcceptedCandidate";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_SetUseSystemAppearance:
        return "WebPage::SetUseSystemAppearance";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_SetHeaderBannerHeightForTesting:
        return "WebPage::SetHeaderBannerHeightForTesting";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_SetFooterBannerHeightForTesting:
        return "WebPage::SetFooterBannerHeightForTesting";
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_DidEndMagnificationGesture:
        return "WebPage::DidEndMagnificationGesture";
#endif
    case MessageName::WebPage_EffectiveAppearanceDidChange:
        return "WebPage::EffectiveAppearanceDidChange";
#if PLATFORM(GTK)
    case MessageName::WebPage_ThemeDidChange:
        return "WebPage::ThemeDidChange";
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_RequestActiveNowPlayingSessionInfo:
        return "WebPage::RequestActiveNowPlayingSessionInfo";
#endif
    case MessageName::WebPage_SetShouldDispatchFakeMouseMoveEvents:
        return "WebPage::SetShouldDispatchFakeMouseMoveEvents";
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_PlaybackTargetSelected:
        return "WebPage::PlaybackTargetSelected";
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_PlaybackTargetAvailabilityDidChange:
        return "WebPage::PlaybackTargetAvailabilityDidChange";
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetShouldPlayToPlaybackTarget:
        return "WebPage::SetShouldPlayToPlaybackTarget";
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_PlaybackTargetPickerWasDismissed:
        return "WebPage::PlaybackTargetPickerWasDismissed";
#endif
#if ENABLE(POINTER_LOCK)
    case MessageName::WebPage_DidAcquirePointerLock:
        return "WebPage::DidAcquirePointerLock";
#endif
#if ENABLE(POINTER_LOCK)
    case MessageName::WebPage_DidNotAcquirePointerLock:
        return "WebPage::DidNotAcquirePointerLock";
#endif
#if ENABLE(POINTER_LOCK)
    case MessageName::WebPage_DidLosePointerLock:
        return "WebPage::DidLosePointerLock";
#endif
    case MessageName::WebPage_clearWheelEventTestMonitor:
        return "WebPage::clearWheelEventTestMonitor";
    case MessageName::WebPage_SetShouldScaleViewToFitDocument:
        return "WebPage::SetShouldScaleViewToFitDocument";
#if ENABLE(VIDEO) && USE(GSTREAMER)
    case MessageName::WebPage_DidEndRequestInstallMissingMediaPlugins:
        return "WebPage::DidEndRequestInstallMissingMediaPlugins";
#endif
    case MessageName::WebPage_SetUserInterfaceLayoutDirection:
        return "WebPage::SetUserInterfaceLayoutDirection";
    case MessageName::WebPage_DidGetLoadDecisionForIcon:
        return "WebPage::DidGetLoadDecisionForIcon";
    case MessageName::WebPage_SetUseIconLoadingClient:
        return "WebPage::SetUseIconLoadingClient";
#if ENABLE(GAMEPAD)
    case MessageName::WebPage_GamepadActivity:
        return "WebPage::GamepadActivity";
#endif
    case MessageName::WebPage_FrameBecameRemote:
        return "WebPage::FrameBecameRemote";
    case MessageName::WebPage_RegisterURLSchemeHandler:
        return "WebPage::RegisterURLSchemeHandler";
    case MessageName::WebPage_URLSchemeTaskDidPerformRedirection:
        return "WebPage::URLSchemeTaskDidPerformRedirection";
    case MessageName::WebPage_URLSchemeTaskDidReceiveResponse:
        return "WebPage::URLSchemeTaskDidReceiveResponse";
    case MessageName::WebPage_URLSchemeTaskDidReceiveData:
        return "WebPage::URLSchemeTaskDidReceiveData";
    case MessageName::WebPage_URLSchemeTaskDidComplete:
        return "WebPage::URLSchemeTaskDidComplete";
    case MessageName::WebPage_SetIsSuspended:
        return "WebPage::SetIsSuspended";
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPage_InsertAttachment:
        return "WebPage::InsertAttachment";
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPage_UpdateAttachmentAttributes:
        return "WebPage::UpdateAttachmentAttributes";
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPage_UpdateAttachmentIcon:
        return "WebPage::UpdateAttachmentIcon";
#endif
#if ENABLE(APPLICATION_MANIFEST)
    case MessageName::WebPage_GetApplicationManifest:
        return "WebPage::GetApplicationManifest";
#endif
    case MessageName::WebPage_SetDefersLoading:
        return "WebPage::SetDefersLoading";
    case MessageName::WebPage_UpdateCurrentModifierState:
        return "WebPage::UpdateCurrentModifierState";
    case MessageName::WebPage_SimulateDeviceOrientationChange:
        return "WebPage::SimulateDeviceOrientationChange";
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPage_SpeakingErrorOccurred:
        return "WebPage::SpeakingErrorOccurred";
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPage_BoundaryEventOccurred:
        return "WebPage::BoundaryEventOccurred";
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPage_VoicesDidChange:
        return "WebPage::VoicesDidChange";
#endif
    case MessageName::WebPage_SetCanShowPlaceholder:
        return "WebPage::SetCanShowPlaceholder";
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::WebPage_WasLoadedWithDataTransferFromPrevalentResource:
        return "WebPage::WasLoadedWithDataTransferFromPrevalentResource";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::WebPage_ClearLoadedThirdPartyDomains:
        return "WebPage::ClearLoadedThirdPartyDomains";
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::WebPage_LoadedThirdPartyDomains:
        return "WebPage::LoadedThirdPartyDomains";
    case MessageName::WebPage_LoadedThirdPartyDomainsReply:
        return "WebPage::LoadedThirdPartyDomainsReply";
#endif
#if USE(SYSTEM_PREVIEW)
    case MessageName::WebPage_SystemPreviewActionTriggered:
        return "WebPage::SystemPreviewActionTriggered";
#endif
#if PLATFORM(GTK) || PLATFORM(WPE)
    case MessageName::WebPage_SendMessageToWebExtension:
        return "WebPage::SendMessageToWebExtension";
#endif
#if PLATFORM(GTK) || PLATFORM(WPE)
    case MessageName::WebPage_SendMessageToWebExtensionWithReply:
        return "WebPage::SendMessageToWebExtensionWithReply";
    case MessageName::WebPage_SendMessageToWebExtensionWithReplyReply:
        return "WebPage::SendMessageToWebExtensionWithReplyReply";
#endif
    case MessageName::WebPage_StartTextManipulations:
        return "WebPage::StartTextManipulations";
    case MessageName::WebPage_StartTextManipulationsReply:
        return "WebPage::StartTextManipulationsReply";
    case MessageName::WebPage_CompleteTextManipulation:
        return "WebPage::CompleteTextManipulation";
    case MessageName::WebPage_CompleteTextManipulationReply:
        return "WebPage::CompleteTextManipulationReply";
    case MessageName::WebPage_SetOverriddenMediaType:
        return "WebPage::SetOverriddenMediaType";
    case MessageName::WebPage_GetProcessDisplayName:
        return "WebPage::GetProcessDisplayName";
    case MessageName::WebPage_GetProcessDisplayNameReply:
        return "WebPage::GetProcessDisplayNameReply";
    case MessageName::WebPage_UpdateCORSDisablingPatterns:
        return "WebPage::UpdateCORSDisablingPatterns";
    case MessageName::WebPage_SetShouldFireEvents:
        return "WebPage::SetShouldFireEvents";
    case MessageName::WebPage_SetNeedsDOMWindowResizeEvent:
        return "WebPage::SetNeedsDOMWindowResizeEvent";
    case MessageName::WebPage_SetHasResourceLoadClient:
        return "WebPage::SetHasResourceLoadClient";
    case MessageName::StorageAreaMap_DidSetItem:
        return "StorageAreaMap::DidSetItem";
    case MessageName::StorageAreaMap_DidRemoveItem:
        return "StorageAreaMap::DidRemoveItem";
    case MessageName::StorageAreaMap_DidClear:
        return "StorageAreaMap::DidClear";
    case MessageName::StorageAreaMap_DispatchStorageEvent:
        return "StorageAreaMap::DispatchStorageEvent";
    case MessageName::StorageAreaMap_ClearCache:
        return "StorageAreaMap::ClearCache";
#if PLATFORM(MAC)
    case MessageName::ViewGestureController_DidCollectGeometryForMagnificationGesture:
        return "ViewGestureController::DidCollectGeometryForMagnificationGesture";
#endif
#if PLATFORM(MAC)
    case MessageName::ViewGestureController_DidCollectGeometryForSmartMagnificationGesture:
        return "ViewGestureController::DidCollectGeometryForSmartMagnificationGesture";
#endif
#if !PLATFORM(IOS_FAMILY)
    case MessageName::ViewGestureController_DidHitRenderTreeSizeThreshold:
        return "ViewGestureController::DidHitRenderTreeSizeThreshold";
#endif
#if PLATFORM(COCOA)
    case MessageName::ViewGestureGeometryCollector_CollectGeometryForSmartMagnificationGesture:
        return "ViewGestureGeometryCollector::CollectGeometryForSmartMagnificationGesture";
#endif
#if PLATFORM(MAC)
    case MessageName::ViewGestureGeometryCollector_CollectGeometryForMagnificationGesture:
        return "ViewGestureGeometryCollector::CollectGeometryForMagnificationGesture";
#endif
#if !PLATFORM(IOS_FAMILY)
    case MessageName::ViewGestureGeometryCollector_SetRenderTreeSizeNotificationThreshold:
        return "ViewGestureGeometryCollector::SetRenderTreeSizeNotificationThreshold";
#endif
    case MessageName::WrappedAsyncMessageForTesting:
        return "IPC::WrappedAsyncMessageForTesting";
    case MessageName::SyncMessageReply:
        return "IPC::SyncMessageReply";
    case MessageName::InitializeConnection:
        return "IPC::InitializeConnection";
    case MessageName::LegacySessionState:
        return "IPC::LegacySessionState";
    }
    ASSERT_NOT_REACHED();
    return "<invalid message name>";
}

ReceiverName receiverName(MessageName messageName)
{
    switch (messageName) {
    case MessageName::GPUConnectionToWebProcess_CreateRenderingBackend:
    case MessageName::GPUConnectionToWebProcess_ReleaseRenderingBackend:
    case MessageName::GPUConnectionToWebProcess_ClearNowPlayingInfo:
    case MessageName::GPUConnectionToWebProcess_SetNowPlayingInfo:
#if USE(AUDIO_SESSION)
    case MessageName::GPUConnectionToWebProcess_EnsureAudioSession:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::GPUConnectionToWebProcess_EnsureMediaSessionHelper:
#endif
        return ReceiverName::GPUConnectionToWebProcess;
    case MessageName::GPUProcess_InitializeGPUProcess:
    case MessageName::GPUProcess_CreateGPUConnectionToWebProcess:
    case MessageName::GPUProcess_ProcessDidTransitionToForeground:
    case MessageName::GPUProcess_ProcessDidTransitionToBackground:
    case MessageName::GPUProcess_AddSession:
    case MessageName::GPUProcess_RemoveSession:
#if ENABLE(MEDIA_STREAM)
    case MessageName::GPUProcess_SetMockCaptureDevicesEnabled:
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::GPUProcess_SetOrientationForMediaCapture:
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::GPUProcess_UpdateCaptureAccess:
#endif
        return ReceiverName::GPUProcess;
    case MessageName::RemoteRenderingBackendProxy_CreateImageBuffer:
    case MessageName::RemoteRenderingBackendProxy_ReleaseImageBuffer:
    case MessageName::RemoteRenderingBackendProxy_FlushImageBufferDrawingContext:
    case MessageName::RemoteRenderingBackendProxy_FlushImageBufferDrawingContextAndCommit:
    case MessageName::RemoteRenderingBackendProxy_GetImageData:
        return ReceiverName::RemoteRenderingBackendProxy;
    case MessageName::RemoteAudioDestinationManager_CreateAudioDestination:
    case MessageName::RemoteAudioDestinationManager_DeleteAudioDestination:
    case MessageName::RemoteAudioDestinationManager_StartAudioDestination:
    case MessageName::RemoteAudioDestinationManager_StopAudioDestination:
        return ReceiverName::RemoteAudioDestinationManager;
    case MessageName::RemoteAudioSessionProxy_SetCategory:
    case MessageName::RemoteAudioSessionProxy_SetPreferredBufferSize:
    case MessageName::RemoteAudioSessionProxy_TryToSetActive:
        return ReceiverName::RemoteAudioSessionProxy;
    case MessageName::RemoteCDMFactoryProxy_CreateCDM:
    case MessageName::RemoteCDMFactoryProxy_SupportsKeySystem:
        return ReceiverName::RemoteCDMFactoryProxy;
    case MessageName::RemoteCDMInstanceProxy_CreateSession:
    case MessageName::RemoteCDMInstanceProxy_InitializeWithConfiguration:
    case MessageName::RemoteCDMInstanceProxy_SetServerCertificate:
    case MessageName::RemoteCDMInstanceProxy_SetStorageDirectory:
        return ReceiverName::RemoteCDMInstanceProxy;
    case MessageName::RemoteCDMInstanceSessionProxy_RequestLicense:
    case MessageName::RemoteCDMInstanceSessionProxy_UpdateLicense:
    case MessageName::RemoteCDMInstanceSessionProxy_LoadSession:
    case MessageName::RemoteCDMInstanceSessionProxy_CloseSession:
    case MessageName::RemoteCDMInstanceSessionProxy_RemoveSessionData:
    case MessageName::RemoteCDMInstanceSessionProxy_StoreRecordOfKeyUsage:
        return ReceiverName::RemoteCDMInstanceSessionProxy;
    case MessageName::RemoteCDMProxy_GetSupportedConfiguration:
    case MessageName::RemoteCDMProxy_CreateInstance:
    case MessageName::RemoteCDMProxy_LoadAndInitialize:
        return ReceiverName::RemoteCDMProxy;
    case MessageName::RemoteLegacyCDMFactoryProxy_CreateCDM:
    case MessageName::RemoteLegacyCDMFactoryProxy_SupportsKeySystem:
        return ReceiverName::RemoteLegacyCDMFactoryProxy;
    case MessageName::RemoteLegacyCDMProxy_SupportsMIMEType:
    case MessageName::RemoteLegacyCDMProxy_CreateSession:
    case MessageName::RemoteLegacyCDMProxy_SetPlayerId:
        return ReceiverName::RemoteLegacyCDMProxy;
    case MessageName::RemoteLegacyCDMSessionProxy_GenerateKeyRequest:
    case MessageName::RemoteLegacyCDMSessionProxy_ReleaseKeys:
    case MessageName::RemoteLegacyCDMSessionProxy_Update:
    case MessageName::RemoteLegacyCDMSessionProxy_CachedKeyForKeyID:
        return ReceiverName::RemoteLegacyCDMSessionProxy;
    case MessageName::RemoteMediaPlayerManagerProxy_CreateMediaPlayer:
    case MessageName::RemoteMediaPlayerManagerProxy_DeleteMediaPlayer:
    case MessageName::RemoteMediaPlayerManagerProxy_GetSupportedTypes:
    case MessageName::RemoteMediaPlayerManagerProxy_SupportsTypeAndCodecs:
    case MessageName::RemoteMediaPlayerManagerProxy_CanDecodeExtendedType:
    case MessageName::RemoteMediaPlayerManagerProxy_OriginsInMediaCache:
    case MessageName::RemoteMediaPlayerManagerProxy_ClearMediaCache:
    case MessageName::RemoteMediaPlayerManagerProxy_ClearMediaCacheForOrigins:
    case MessageName::RemoteMediaPlayerManagerProxy_SupportsKeySystem:
        return ReceiverName::RemoteMediaPlayerManagerProxy;
    case MessageName::RemoteMediaPlayerProxy_PrepareForPlayback:
    case MessageName::RemoteMediaPlayerProxy_Load:
    case MessageName::RemoteMediaPlayerProxy_CancelLoad:
    case MessageName::RemoteMediaPlayerProxy_PrepareToPlay:
    case MessageName::RemoteMediaPlayerProxy_Play:
    case MessageName::RemoteMediaPlayerProxy_Pause:
    case MessageName::RemoteMediaPlayerProxy_SetVolume:
    case MessageName::RemoteMediaPlayerProxy_SetMuted:
    case MessageName::RemoteMediaPlayerProxy_Seek:
    case MessageName::RemoteMediaPlayerProxy_SeekWithTolerance:
    case MessageName::RemoteMediaPlayerProxy_SetPreload:
    case MessageName::RemoteMediaPlayerProxy_SetPrivateBrowsingMode:
    case MessageName::RemoteMediaPlayerProxy_SetPreservesPitch:
    case MessageName::RemoteMediaPlayerProxy_PrepareForRendering:
    case MessageName::RemoteMediaPlayerProxy_SetVisible:
    case MessageName::RemoteMediaPlayerProxy_SetShouldMaintainAspectRatio:
#if ENABLE(VIDEO_PRESENTATION_MODE)
    case MessageName::RemoteMediaPlayerProxy_SetVideoFullscreenGravity:
#endif
    case MessageName::RemoteMediaPlayerProxy_AcceleratedRenderingStateChanged:
    case MessageName::RemoteMediaPlayerProxy_SetShouldDisableSleep:
    case MessageName::RemoteMediaPlayerProxy_SetRate:
#if ENABLE(VIDEO_PRESENTATION_MODE)
    case MessageName::RemoteMediaPlayerProxy_UpdateVideoFullscreenInlineImage:
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    case MessageName::RemoteMediaPlayerProxy_SetVideoFullscreenMode:
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    case MessageName::RemoteMediaPlayerProxy_VideoFullscreenStandbyChanged:
#endif
    case MessageName::RemoteMediaPlayerProxy_SetBufferingPolicy:
    case MessageName::RemoteMediaPlayerProxy_AudioTrackSetEnabled:
    case MessageName::RemoteMediaPlayerProxy_VideoTrackSetSelected:
    case MessageName::RemoteMediaPlayerProxy_TextTrackSetMode:
#if PLATFORM(COCOA)
    case MessageName::RemoteMediaPlayerProxy_SetVideoInlineSizeFenced:
#endif
#if (PLATFORM(COCOA) && ENABLE(VIDEO_PRESENTATION_MODE))
    case MessageName::RemoteMediaPlayerProxy_SetVideoFullscreenFrameFenced:
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    case MessageName::RemoteMediaPlayerProxy_EnterFullscreen:
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    case MessageName::RemoteMediaPlayerProxy_ExitFullscreen:
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    case MessageName::RemoteMediaPlayerProxy_SetWirelessVideoPlaybackDisabled:
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    case MessageName::RemoteMediaPlayerProxy_SetShouldPlayToPlaybackTarget:
#endif
#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    case MessageName::RemoteMediaPlayerProxy_SetLegacyCDMSession:
#endif
#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    case MessageName::RemoteMediaPlayerProxy_KeyAdded:
#endif
#if ENABLE(ENCRYPTED_MEDIA)
    case MessageName::RemoteMediaPlayerProxy_CdmInstanceAttached:
#endif
#if ENABLE(ENCRYPTED_MEDIA)
    case MessageName::RemoteMediaPlayerProxy_CdmInstanceDetached:
#endif
#if ENABLE(ENCRYPTED_MEDIA)
    case MessageName::RemoteMediaPlayerProxy_AttemptToDecryptWithInstance:
#endif
#if ENABLE(LEGACY_ENCRYPTED_MEDIA) && ENABLE(ENCRYPTED_MEDIA)
    case MessageName::RemoteMediaPlayerProxy_SetShouldContinueAfterKeyNeeded:
#endif
    case MessageName::RemoteMediaPlayerProxy_BeginSimulatedHDCPError:
    case MessageName::RemoteMediaPlayerProxy_EndSimulatedHDCPError:
    case MessageName::RemoteMediaPlayerProxy_NotifyActiveSourceBuffersChanged:
    case MessageName::RemoteMediaPlayerProxy_ApplicationWillResignActive:
    case MessageName::RemoteMediaPlayerProxy_ApplicationDidBecomeActive:
    case MessageName::RemoteMediaPlayerProxy_NotifyTrackModeChanged:
    case MessageName::RemoteMediaPlayerProxy_TracksChanged:
    case MessageName::RemoteMediaPlayerProxy_SyncTextTrackBounds:
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    case MessageName::RemoteMediaPlayerProxy_SetWirelessPlaybackTarget:
#endif
    case MessageName::RemoteMediaPlayerProxy_PerformTaskAtMediaTime:
    case MessageName::RemoteMediaPlayerProxy_WouldTaintOrigin:
#if PLATFORM(IOS_FAMILY)
    case MessageName::RemoteMediaPlayerProxy_ErrorLog:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::RemoteMediaPlayerProxy_AccessLog:
#endif
        return ReceiverName::RemoteMediaPlayerProxy;
    case MessageName::RemoteMediaResourceManager_ResponseReceived:
    case MessageName::RemoteMediaResourceManager_RedirectReceived:
    case MessageName::RemoteMediaResourceManager_DataSent:
    case MessageName::RemoteMediaResourceManager_DataReceived:
    case MessageName::RemoteMediaResourceManager_AccessControlCheckFailed:
    case MessageName::RemoteMediaResourceManager_LoadFailed:
    case MessageName::RemoteMediaResourceManager_LoadFinished:
        return ReceiverName::RemoteMediaResourceManager;
    case MessageName::LibWebRTCCodecsProxy_CreateH264Decoder:
    case MessageName::LibWebRTCCodecsProxy_CreateH265Decoder:
    case MessageName::LibWebRTCCodecsProxy_ReleaseDecoder:
    case MessageName::LibWebRTCCodecsProxy_DecodeFrame:
    case MessageName::LibWebRTCCodecsProxy_CreateEncoder:
    case MessageName::LibWebRTCCodecsProxy_ReleaseEncoder:
    case MessageName::LibWebRTCCodecsProxy_InitializeEncoder:
    case MessageName::LibWebRTCCodecsProxy_EncodeFrame:
    case MessageName::LibWebRTCCodecsProxy_SetEncodeRates:
        return ReceiverName::LibWebRTCCodecsProxy;
    case MessageName::RemoteAudioMediaStreamTrackRenderer_Start:
    case MessageName::RemoteAudioMediaStreamTrackRenderer_Stop:
    case MessageName::RemoteAudioMediaStreamTrackRenderer_Clear:
    case MessageName::RemoteAudioMediaStreamTrackRenderer_SetVolume:
    case MessageName::RemoteAudioMediaStreamTrackRenderer_AudioSamplesStorageChanged:
    case MessageName::RemoteAudioMediaStreamTrackRenderer_AudioSamplesAvailable:
        return ReceiverName::RemoteAudioMediaStreamTrackRenderer;
    case MessageName::RemoteAudioMediaStreamTrackRendererManager_CreateRenderer:
    case MessageName::RemoteAudioMediaStreamTrackRendererManager_ReleaseRenderer:
        return ReceiverName::RemoteAudioMediaStreamTrackRendererManager;
    case MessageName::RemoteMediaRecorder_AudioSamplesStorageChanged:
    case MessageName::RemoteMediaRecorder_AudioSamplesAvailable:
    case MessageName::RemoteMediaRecorder_VideoSampleAvailable:
    case MessageName::RemoteMediaRecorder_FetchData:
    case MessageName::RemoteMediaRecorder_StopRecording:
        return ReceiverName::RemoteMediaRecorder;
    case MessageName::RemoteMediaRecorderManager_CreateRecorder:
    case MessageName::RemoteMediaRecorderManager_ReleaseRecorder:
        return ReceiverName::RemoteMediaRecorderManager;
    case MessageName::RemoteSampleBufferDisplayLayer_UpdateDisplayMode:
    case MessageName::RemoteSampleBufferDisplayLayer_UpdateAffineTransform:
    case MessageName::RemoteSampleBufferDisplayLayer_UpdateBoundsAndPosition:
    case MessageName::RemoteSampleBufferDisplayLayer_Flush:
    case MessageName::RemoteSampleBufferDisplayLayer_FlushAndRemoveImage:
    case MessageName::RemoteSampleBufferDisplayLayer_EnqueueSample:
    case MessageName::RemoteSampleBufferDisplayLayer_ClearEnqueuedSamples:
        return ReceiverName::RemoteSampleBufferDisplayLayer;
    case MessageName::RemoteSampleBufferDisplayLayerManager_CreateLayer:
    case MessageName::RemoteSampleBufferDisplayLayerManager_ReleaseLayer:
        return ReceiverName::RemoteSampleBufferDisplayLayerManager;
    case MessageName::WebCookieManager_GetHostnamesWithCookies:
    case MessageName::WebCookieManager_DeleteCookiesForHostnames:
    case MessageName::WebCookieManager_DeleteAllCookies:
    case MessageName::WebCookieManager_SetCookie:
    case MessageName::WebCookieManager_SetCookies:
    case MessageName::WebCookieManager_GetAllCookies:
    case MessageName::WebCookieManager_GetCookies:
    case MessageName::WebCookieManager_DeleteCookie:
    case MessageName::WebCookieManager_DeleteAllCookiesModifiedSince:
    case MessageName::WebCookieManager_SetHTTPCookieAcceptPolicy:
    case MessageName::WebCookieManager_GetHTTPCookieAcceptPolicy:
    case MessageName::WebCookieManager_StartObservingCookieChanges:
    case MessageName::WebCookieManager_StopObservingCookieChanges:
#if USE(SOUP)
    case MessageName::WebCookieManager_SetCookiePersistentStorage:
#endif
        return ReceiverName::WebCookieManager;
    case MessageName::WebIDBServer_DeleteDatabase:
    case MessageName::WebIDBServer_OpenDatabase:
    case MessageName::WebIDBServer_AbortTransaction:
    case MessageName::WebIDBServer_CommitTransaction:
    case MessageName::WebIDBServer_DidFinishHandlingVersionChangeTransaction:
    case MessageName::WebIDBServer_CreateObjectStore:
    case MessageName::WebIDBServer_DeleteObjectStore:
    case MessageName::WebIDBServer_RenameObjectStore:
    case MessageName::WebIDBServer_ClearObjectStore:
    case MessageName::WebIDBServer_CreateIndex:
    case MessageName::WebIDBServer_DeleteIndex:
    case MessageName::WebIDBServer_RenameIndex:
    case MessageName::WebIDBServer_PutOrAdd:
    case MessageName::WebIDBServer_GetRecord:
    case MessageName::WebIDBServer_GetAllRecords:
    case MessageName::WebIDBServer_GetCount:
    case MessageName::WebIDBServer_DeleteRecord:
    case MessageName::WebIDBServer_OpenCursor:
    case MessageName::WebIDBServer_IterateCursor:
    case MessageName::WebIDBServer_EstablishTransaction:
    case MessageName::WebIDBServer_DatabaseConnectionPendingClose:
    case MessageName::WebIDBServer_DatabaseConnectionClosed:
    case MessageName::WebIDBServer_AbortOpenAndUpgradeNeeded:
    case MessageName::WebIDBServer_DidFireVersionChangeEvent:
    case MessageName::WebIDBServer_OpenDBRequestCancelled:
    case MessageName::WebIDBServer_GetAllDatabaseNamesAndVersions:
        return ReceiverName::WebIDBServer;
    case MessageName::NetworkConnectionToWebProcess_ScheduleResourceLoad:
    case MessageName::NetworkConnectionToWebProcess_PerformSynchronousLoad:
    case MessageName::NetworkConnectionToWebProcess_TestProcessIncomingSyncMessagesWhenWaitingForSyncReply:
    case MessageName::NetworkConnectionToWebProcess_LoadPing:
    case MessageName::NetworkConnectionToWebProcess_RemoveLoadIdentifier:
    case MessageName::NetworkConnectionToWebProcess_PageLoadCompleted:
    case MessageName::NetworkConnectionToWebProcess_BrowsingContextRemoved:
    case MessageName::NetworkConnectionToWebProcess_PrefetchDNS:
    case MessageName::NetworkConnectionToWebProcess_PreconnectTo:
    case MessageName::NetworkConnectionToWebProcess_StartDownload:
    case MessageName::NetworkConnectionToWebProcess_ConvertMainResourceLoadToDownload:
    case MessageName::NetworkConnectionToWebProcess_CookiesForDOM:
    case MessageName::NetworkConnectionToWebProcess_SetCookiesFromDOM:
    case MessageName::NetworkConnectionToWebProcess_CookieRequestHeaderFieldValue:
    case MessageName::NetworkConnectionToWebProcess_GetRawCookies:
    case MessageName::NetworkConnectionToWebProcess_SetRawCookie:
    case MessageName::NetworkConnectionToWebProcess_DeleteCookie:
    case MessageName::NetworkConnectionToWebProcess_DomCookiesForHost:
#if HAVE(COOKIE_CHANGE_LISTENER_API)
    case MessageName::NetworkConnectionToWebProcess_UnsubscribeFromCookieChangeNotifications:
#endif
    case MessageName::NetworkConnectionToWebProcess_RegisterFileBlobURL:
    case MessageName::NetworkConnectionToWebProcess_RegisterBlobURL:
    case MessageName::NetworkConnectionToWebProcess_RegisterBlobURLFromURL:
    case MessageName::NetworkConnectionToWebProcess_RegisterBlobURLOptionallyFileBacked:
    case MessageName::NetworkConnectionToWebProcess_RegisterBlobURLForSlice:
    case MessageName::NetworkConnectionToWebProcess_UnregisterBlobURL:
    case MessageName::NetworkConnectionToWebProcess_BlobSize:
    case MessageName::NetworkConnectionToWebProcess_WriteBlobsToTemporaryFiles:
    case MessageName::NetworkConnectionToWebProcess_SetCaptureExtraNetworkLoadMetricsEnabled:
    case MessageName::NetworkConnectionToWebProcess_CreateSocketStream:
    case MessageName::NetworkConnectionToWebProcess_CreateSocketChannel:
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkConnectionToWebProcess_RemoveStorageAccessForFrame:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkConnectionToWebProcess_ClearPageSpecificDataForResourceLoadStatistics:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkConnectionToWebProcess_LogUserInteraction:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkConnectionToWebProcess_ResourceLoadStatisticsUpdated:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkConnectionToWebProcess_HasStorageAccess:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkConnectionToWebProcess_RequestStorageAccess:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkConnectionToWebProcess_RequestStorageAccessUnderOpener:
#endif
    case MessageName::NetworkConnectionToWebProcess_AddOriginAccessWhitelistEntry:
    case MessageName::NetworkConnectionToWebProcess_RemoveOriginAccessWhitelistEntry:
    case MessageName::NetworkConnectionToWebProcess_ResetOriginAccessWhitelists:
    case MessageName::NetworkConnectionToWebProcess_GetNetworkLoadInformationResponse:
    case MessageName::NetworkConnectionToWebProcess_GetNetworkLoadIntermediateInformation:
    case MessageName::NetworkConnectionToWebProcess_TakeNetworkLoadInformationMetrics:
#if ENABLE(SERVICE_WORKER)
    case MessageName::NetworkConnectionToWebProcess_EstablishSWContextConnection:
#endif
#if ENABLE(SERVICE_WORKER)
    case MessageName::NetworkConnectionToWebProcess_CloseSWContextConnection:
#endif
    case MessageName::NetworkConnectionToWebProcess_UpdateQuotaBasedOnSpaceUsageForTesting:
    case MessageName::NetworkConnectionToWebProcess_CreateNewMessagePortChannel:
    case MessageName::NetworkConnectionToWebProcess_EntangleLocalPortInThisProcessToRemote:
    case MessageName::NetworkConnectionToWebProcess_MessagePortDisentangled:
    case MessageName::NetworkConnectionToWebProcess_MessagePortClosed:
    case MessageName::NetworkConnectionToWebProcess_TakeAllMessagesForPort:
    case MessageName::NetworkConnectionToWebProcess_PostMessageToRemote:
    case MessageName::NetworkConnectionToWebProcess_CheckRemotePortForActivity:
    case MessageName::NetworkConnectionToWebProcess_DidDeliverMessagePortMessages:
    case MessageName::NetworkConnectionToWebProcess_RegisterURLSchemesAsCORSEnabled:
        return ReceiverName::NetworkConnectionToWebProcess;
    case MessageName::NetworkContentRuleListManager_Remove:
    case MessageName::NetworkContentRuleListManager_AddContentRuleLists:
    case MessageName::NetworkContentRuleListManager_RemoveContentRuleList:
    case MessageName::NetworkContentRuleListManager_RemoveAllContentRuleLists:
        return ReceiverName::NetworkContentRuleListManager;
    case MessageName::NetworkProcess_InitializeNetworkProcess:
    case MessageName::NetworkProcess_CreateNetworkConnectionToWebProcess:
#if USE(SOUP)
    case MessageName::NetworkProcess_SetIgnoreTLSErrors:
#endif
#if USE(SOUP)
    case MessageName::NetworkProcess_UserPreferredLanguagesChanged:
#endif
#if USE(SOUP)
    case MessageName::NetworkProcess_SetNetworkProxySettings:
#endif
#if USE(SOUP)
    case MessageName::NetworkProcess_PrefetchDNS:
#endif
#if USE(CURL)
    case MessageName::NetworkProcess_SetNetworkProxySettings:
#endif
    case MessageName::NetworkProcess_ClearCachedCredentials:
    case MessageName::NetworkProcess_AddWebsiteDataStore:
    case MessageName::NetworkProcess_DestroySession:
    case MessageName::NetworkProcess_FetchWebsiteData:
    case MessageName::NetworkProcess_DeleteWebsiteData:
    case MessageName::NetworkProcess_DeleteWebsiteDataForOrigins:
    case MessageName::NetworkProcess_RenameOriginInWebsiteData:
    case MessageName::NetworkProcess_DownloadRequest:
    case MessageName::NetworkProcess_ResumeDownload:
    case MessageName::NetworkProcess_CancelDownload:
#if PLATFORM(COCOA)
    case MessageName::NetworkProcess_PublishDownloadProgress:
#endif
    case MessageName::NetworkProcess_ApplicationDidEnterBackground:
    case MessageName::NetworkProcess_ApplicationWillEnterForeground:
    case MessageName::NetworkProcess_ContinueWillSendRequest:
    case MessageName::NetworkProcess_ContinueDecidePendingDownloadDestination:
#if PLATFORM(COCOA)
    case MessageName::NetworkProcess_SetQOS:
#endif
#if PLATFORM(COCOA)
    case MessageName::NetworkProcess_SetStorageAccessAPIEnabled:
#endif
    case MessageName::NetworkProcess_SetAllowsAnySSLCertificateForWebSocket:
    case MessageName::NetworkProcess_SyncAllCookies:
    case MessageName::NetworkProcess_AllowSpecificHTTPSCertificateForHost:
    case MessageName::NetworkProcess_SetCacheModel:
    case MessageName::NetworkProcess_SetCacheModelSynchronouslyForTesting:
    case MessageName::NetworkProcess_ProcessDidTransitionToBackground:
    case MessageName::NetworkProcess_ProcessDidTransitionToForeground:
    case MessageName::NetworkProcess_ProcessWillSuspendImminentlyForTestingSync:
    case MessageName::NetworkProcess_PrepareToSuspend:
    case MessageName::NetworkProcess_ProcessDidResume:
    case MessageName::NetworkProcess_PreconnectTo:
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ClearPrevalentResource:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ClearUserInteraction:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_DumpResourceLoadStatistics:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetResourceLoadStatisticsEnabled:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetResourceLoadStatisticsLogTestingEvent:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_UpdatePrevalentDomainsToBlockCookiesFor:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsGrandfathered:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsPrevalentResource:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsVeryPrevalentResource:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetAgeCapForClientSideCookies:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetLastSeen:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_MergeStatisticForTesting:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_InsertExpiredStatisticForTesting:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetPrevalentResource:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetPrevalentResourceForDebugMode:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsResourceLoadStatisticsEphemeral:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_HadUserInteraction:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsRelationshipOnlyInDatabaseOnce:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_HasLocalStorage:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_GetAllStorageAccessEntries:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsRegisteredAsRedirectingTo:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsRegisteredAsSubFrameUnder:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsRegisteredAsSubresourceUnder:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_DomainIDExistsInDatabase:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_LogFrameNavigation:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_LogUserInteraction:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ResetParametersToDefaultValues:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ScheduleClearInMemoryAndPersistent:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ScheduleCookieBlockingUpdate:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ScheduleStatisticsAndDataRecordsProcessing:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_StatisticsDatabaseHasAllTables:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SubmitTelemetry:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetCacheMaxAgeCapForPrevalentResources:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetGrandfathered:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetUseITPDatabase:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_GetResourceLoadStatisticsDataSummary:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetGrandfatheringTime:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetMaxStatisticsEntries:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetMinimumTimeBetweenDataRecordsRemoval:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetPruneEntriesDownTo:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetShouldClassifyResourcesBeforeDataRecordsRemoval:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetNotifyPagesWhenDataRecordsWereScanned:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetIsRunningResourceLoadStatisticsTest:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetNotifyPagesWhenTelemetryWasCaptured:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetResourceLoadStatisticsDebugMode:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetVeryPrevalentResource:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetSubframeUnderTopFrameDomain:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetSubresourceUnderTopFrameDomain:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetSubresourceUniqueRedirectTo:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetSubresourceUniqueRedirectFrom:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetTimeToLiveUserInteraction:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetTopFrameUniqueRedirectTo:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetTopFrameUniqueRedirectFrom:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ResetCacheMaxAgeCapForPrevalentResources:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_DidCommitCrossSiteLoadWithDataTransfer:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetCrossSiteLoadWithLinkDecorationForTesting:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ResetCrossSiteLoadsWithLinkDecorationForTesting:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_DeleteCookiesForTesting:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_HasIsolatedSession:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetAppBoundDomainsForResourceLoadStatistics:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetShouldDowngradeReferrerForTesting:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetThirdPartyCookieBlockingMode:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetShouldEnbleSameSiteStrictEnforcementForTesting:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetFirstPartyWebsiteDataRemovalModeForTesting:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetToSameSiteStrictCookiesForTesting:
#endif
    case MessageName::NetworkProcess_SetAdClickAttributionDebugMode:
    case MessageName::NetworkProcess_SetSessionIsControlledByAutomation:
    case MessageName::NetworkProcess_RegisterURLSchemeAsSecure:
    case MessageName::NetworkProcess_RegisterURLSchemeAsBypassingContentSecurityPolicy:
    case MessageName::NetworkProcess_RegisterURLSchemeAsLocal:
    case MessageName::NetworkProcess_RegisterURLSchemeAsNoAccess:
    case MessageName::NetworkProcess_SetCacheStorageParameters:
    case MessageName::NetworkProcess_SyncLocalStorage:
    case MessageName::NetworkProcess_ClearLegacyPrivateBrowsingLocalStorage:
    case MessageName::NetworkProcess_StoreAdClickAttribution:
    case MessageName::NetworkProcess_DumpAdClickAttribution:
    case MessageName::NetworkProcess_ClearAdClickAttribution:
    case MessageName::NetworkProcess_SetAdClickAttributionOverrideTimerForTesting:
    case MessageName::NetworkProcess_SetAdClickAttributionConversionURLForTesting:
    case MessageName::NetworkProcess_MarkAdClickAttributionsAsExpiredForTesting:
    case MessageName::NetworkProcess_GetLocalStorageOriginDetails:
    case MessageName::NetworkProcess_SetServiceWorkerFetchTimeoutForTesting:
    case MessageName::NetworkProcess_ResetServiceWorkerFetchTimeoutForTesting:
    case MessageName::NetworkProcess_ResetQuota:
    case MessageName::NetworkProcess_HasAppBoundSession:
    case MessageName::NetworkProcess_ClearAppBoundSession:
#if PLATFORM(IOS_FAMILY) && !PLATFORM(MACCATALYST)
    case MessageName::NetworkProcess_DisableServiceWorkerEntitlement:
#endif
#if PLATFORM(IOS_FAMILY) && !PLATFORM(MACCATALYST)
    case MessageName::NetworkProcess_ClearServiceWorkerEntitlementOverride:
#endif
    case MessageName::NetworkProcess_UpdateBundleIdentifier:
    case MessageName::NetworkProcess_ClearBundleIdentifier:
        return ReceiverName::NetworkProcess;
    case MessageName::NetworkResourceLoader_ContinueWillSendRequest:
    case MessageName::NetworkResourceLoader_ContinueDidReceiveResponse:
        return ReceiverName::NetworkResourceLoader;
    case MessageName::NetworkSocketChannel_SendString:
    case MessageName::NetworkSocketChannel_SendData:
    case MessageName::NetworkSocketChannel_Close:
        return ReceiverName::NetworkSocketChannel;
    case MessageName::NetworkSocketStream_SendData:
    case MessageName::NetworkSocketStream_SendHandshake:
    case MessageName::NetworkSocketStream_Close:
        return ReceiverName::NetworkSocketStream;
    case MessageName::ServiceWorkerFetchTask_DidNotHandle:
    case MessageName::ServiceWorkerFetchTask_DidFail:
    case MessageName::ServiceWorkerFetchTask_DidReceiveRedirectResponse:
    case MessageName::ServiceWorkerFetchTask_DidReceiveResponse:
    case MessageName::ServiceWorkerFetchTask_DidReceiveData:
    case MessageName::ServiceWorkerFetchTask_DidReceiveSharedBuffer:
    case MessageName::ServiceWorkerFetchTask_DidReceiveFormData:
    case MessageName::ServiceWorkerFetchTask_DidFinish:
        return ReceiverName::ServiceWorkerFetchTask;
    case MessageName::WebSWServerConnection_ScheduleJobInServer:
    case MessageName::WebSWServerConnection_ScheduleUnregisterJobInServer:
    case MessageName::WebSWServerConnection_FinishFetchingScriptInServer:
    case MessageName::WebSWServerConnection_AddServiceWorkerRegistrationInServer:
    case MessageName::WebSWServerConnection_RemoveServiceWorkerRegistrationInServer:
    case MessageName::WebSWServerConnection_PostMessageToServiceWorker:
    case MessageName::WebSWServerConnection_DidResolveRegistrationPromise:
    case MessageName::WebSWServerConnection_MatchRegistration:
    case MessageName::WebSWServerConnection_WhenRegistrationReady:
    case MessageName::WebSWServerConnection_GetRegistrations:
    case MessageName::WebSWServerConnection_RegisterServiceWorkerClient:
    case MessageName::WebSWServerConnection_UnregisterServiceWorkerClient:
    case MessageName::WebSWServerConnection_TerminateWorkerFromClient:
    case MessageName::WebSWServerConnection_WhenServiceWorkerIsTerminatedForTesting:
    case MessageName::WebSWServerConnection_SetThrottleState:
    case MessageName::WebSWServerConnection_StoreRegistrationsOnDisk:
        return ReceiverName::WebSWServerConnection;
    case MessageName::WebSWServerToContextConnection_ScriptContextFailedToStart:
    case MessageName::WebSWServerToContextConnection_ScriptContextStarted:
    case MessageName::WebSWServerToContextConnection_DidFinishInstall:
    case MessageName::WebSWServerToContextConnection_DidFinishActivation:
    case MessageName::WebSWServerToContextConnection_SetServiceWorkerHasPendingEvents:
    case MessageName::WebSWServerToContextConnection_SkipWaiting:
    case MessageName::WebSWServerToContextConnection_WorkerTerminated:
    case MessageName::WebSWServerToContextConnection_FindClientByIdentifier:
    case MessageName::WebSWServerToContextConnection_MatchAll:
    case MessageName::WebSWServerToContextConnection_Claim:
    case MessageName::WebSWServerToContextConnection_SetScriptResource:
    case MessageName::WebSWServerToContextConnection_PostMessageToServiceWorkerClient:
    case MessageName::WebSWServerToContextConnection_DidFailHeartBeatCheck:
        return ReceiverName::WebSWServerToContextConnection;
    case MessageName::StorageManagerSet_ConnectToLocalStorageArea:
    case MessageName::StorageManagerSet_ConnectToTransientLocalStorageArea:
    case MessageName::StorageManagerSet_ConnectToSessionStorageArea:
    case MessageName::StorageManagerSet_DisconnectFromStorageArea:
    case MessageName::StorageManagerSet_GetValues:
    case MessageName::StorageManagerSet_CloneSessionStorageNamespace:
    case MessageName::StorageManagerSet_SetItem:
    case MessageName::StorageManagerSet_RemoveItem:
    case MessageName::StorageManagerSet_Clear:
        return ReceiverName::StorageManagerSet;
    case MessageName::CacheStorageEngineConnection_Reference:
    case MessageName::CacheStorageEngineConnection_Dereference:
    case MessageName::CacheStorageEngineConnection_Open:
    case MessageName::CacheStorageEngineConnection_Remove:
    case MessageName::CacheStorageEngineConnection_Caches:
    case MessageName::CacheStorageEngineConnection_RetrieveRecords:
    case MessageName::CacheStorageEngineConnection_DeleteMatchingRecords:
    case MessageName::CacheStorageEngineConnection_PutRecords:
    case MessageName::CacheStorageEngineConnection_ClearMemoryRepresentation:
    case MessageName::CacheStorageEngineConnection_EngineRepresentation:
        return ReceiverName::CacheStorageEngineConnection;
    case MessageName::NetworkMDNSRegister_UnregisterMDNSNames:
    case MessageName::NetworkMDNSRegister_RegisterMDNSName:
        return ReceiverName::NetworkMDNSRegister;
    case MessageName::NetworkRTCMonitor_StartUpdatingIfNeeded:
    case MessageName::NetworkRTCMonitor_StopUpdating:
        return ReceiverName::NetworkRTCMonitor;
    case MessageName::NetworkRTCProvider_CreateUDPSocket:
    case MessageName::NetworkRTCProvider_CreateServerTCPSocket:
    case MessageName::NetworkRTCProvider_CreateClientTCPSocket:
    case MessageName::NetworkRTCProvider_WrapNewTCPConnection:
    case MessageName::NetworkRTCProvider_CreateResolver:
    case MessageName::NetworkRTCProvider_StopResolver:
        return ReceiverName::NetworkRTCProvider;
    case MessageName::NetworkRTCSocket_SendTo:
    case MessageName::NetworkRTCSocket_Close:
    case MessageName::NetworkRTCSocket_SetOption:
        return ReceiverName::NetworkRTCSocket;
    case MessageName::PluginControllerProxy_GeometryDidChange:
    case MessageName::PluginControllerProxy_VisibilityDidChange:
    case MessageName::PluginControllerProxy_FrameDidFinishLoading:
    case MessageName::PluginControllerProxy_FrameDidFail:
    case MessageName::PluginControllerProxy_DidEvaluateJavaScript:
    case MessageName::PluginControllerProxy_StreamWillSendRequest:
    case MessageName::PluginControllerProxy_StreamDidReceiveResponse:
    case MessageName::PluginControllerProxy_StreamDidReceiveData:
    case MessageName::PluginControllerProxy_StreamDidFinishLoading:
    case MessageName::PluginControllerProxy_StreamDidFail:
    case MessageName::PluginControllerProxy_ManualStreamDidReceiveResponse:
    case MessageName::PluginControllerProxy_ManualStreamDidReceiveData:
    case MessageName::PluginControllerProxy_ManualStreamDidFinishLoading:
    case MessageName::PluginControllerProxy_ManualStreamDidFail:
    case MessageName::PluginControllerProxy_HandleMouseEvent:
    case MessageName::PluginControllerProxy_HandleWheelEvent:
    case MessageName::PluginControllerProxy_HandleMouseEnterEvent:
    case MessageName::PluginControllerProxy_HandleMouseLeaveEvent:
    case MessageName::PluginControllerProxy_HandleKeyboardEvent:
    case MessageName::PluginControllerProxy_HandleEditingCommand:
    case MessageName::PluginControllerProxy_IsEditingCommandEnabled:
    case MessageName::PluginControllerProxy_HandlesPageScaleFactor:
    case MessageName::PluginControllerProxy_RequiresUnifiedScaleFactor:
    case MessageName::PluginControllerProxy_SetFocus:
    case MessageName::PluginControllerProxy_DidUpdate:
    case MessageName::PluginControllerProxy_PaintEntirePlugin:
    case MessageName::PluginControllerProxy_GetPluginScriptableNPObject:
    case MessageName::PluginControllerProxy_WindowFocusChanged:
    case MessageName::PluginControllerProxy_WindowVisibilityChanged:
#if PLATFORM(COCOA)
    case MessageName::PluginControllerProxy_SendComplexTextInput:
#endif
#if PLATFORM(COCOA)
    case MessageName::PluginControllerProxy_WindowAndViewFramesChanged:
#endif
#if PLATFORM(COCOA)
    case MessageName::PluginControllerProxy_SetLayerHostingMode:
#endif
    case MessageName::PluginControllerProxy_SupportsSnapshotting:
    case MessageName::PluginControllerProxy_Snapshot:
    case MessageName::PluginControllerProxy_StorageBlockingStateChanged:
    case MessageName::PluginControllerProxy_PrivateBrowsingStateChanged:
    case MessageName::PluginControllerProxy_GetFormValue:
    case MessageName::PluginControllerProxy_MutedStateChanged:
        return ReceiverName::PluginControllerProxy;
    case MessageName::PluginProcess_InitializePluginProcess:
    case MessageName::PluginProcess_CreateWebProcessConnection:
    case MessageName::PluginProcess_GetSitesWithData:
    case MessageName::PluginProcess_DeleteWebsiteData:
    case MessageName::PluginProcess_DeleteWebsiteDataForHostNames:
#if PLATFORM(COCOA)
    case MessageName::PluginProcess_SetQOS:
#endif
        return ReceiverName::PluginProcess;
    case MessageName::WebProcessConnection_CreatePlugin:
    case MessageName::WebProcessConnection_CreatePluginAsynchronously:
    case MessageName::WebProcessConnection_DestroyPlugin:
        return ReceiverName::WebProcessConnection;
    case MessageName::AuxiliaryProcess_ShutDown:
    case MessageName::AuxiliaryProcess_SetProcessSuppressionEnabled:
#if OS(LINUX)
    case MessageName::AuxiliaryProcess_DidReceiveMemoryPressureEvent:
#endif
        return ReceiverName::AuxiliaryProcess;
    case MessageName::WebConnection_HandleMessage:
        return ReceiverName::WebConnection;
    case MessageName::AuthenticationManager_CompleteAuthenticationChallenge:
        return ReceiverName::AuthenticationManager;
    case MessageName::NPObjectMessageReceiver_Deallocate:
    case MessageName::NPObjectMessageReceiver_HasMethod:
    case MessageName::NPObjectMessageReceiver_Invoke:
    case MessageName::NPObjectMessageReceiver_InvokeDefault:
    case MessageName::NPObjectMessageReceiver_HasProperty:
    case MessageName::NPObjectMessageReceiver_GetProperty:
    case MessageName::NPObjectMessageReceiver_SetProperty:
    case MessageName::NPObjectMessageReceiver_RemoveProperty:
    case MessageName::NPObjectMessageReceiver_Enumerate:
    case MessageName::NPObjectMessageReceiver_Construct:
        return ReceiverName::NPObjectMessageReceiver;
    case MessageName::DrawingAreaProxy_EnterAcceleratedCompositingMode:
    case MessageName::DrawingAreaProxy_UpdateAcceleratedCompositingMode:
    case MessageName::DrawingAreaProxy_DidFirstLayerFlush:
    case MessageName::DrawingAreaProxy_DispatchPresentationCallbacksAfterFlushingLayers:
#if PLATFORM(COCOA)
    case MessageName::DrawingAreaProxy_DidUpdateGeometry:
#endif
#if USE(COORDINATED_GRAPHICS) || USE(TEXTURE_MAPPER)
    case MessageName::DrawingAreaProxy_Update:
#endif
#if USE(COORDINATED_GRAPHICS) || USE(TEXTURE_MAPPER)
    case MessageName::DrawingAreaProxy_DidUpdateBackingStoreState:
#endif
#if USE(COORDINATED_GRAPHICS) || USE(TEXTURE_MAPPER)
    case MessageName::DrawingAreaProxy_ExitAcceleratedCompositingMode:
#endif
        return ReceiverName::DrawingAreaProxy;
    case MessageName::VisitedLinkStore_AddVisitedLinkHashFromPage:
        return ReceiverName::VisitedLinkStore;
    case MessageName::WebCookieManagerProxy_CookiesDidChange:
        return ReceiverName::WebCookieManagerProxy;
    case MessageName::WebFullScreenManagerProxy_SupportsFullScreen:
    case MessageName::WebFullScreenManagerProxy_EnterFullScreen:
    case MessageName::WebFullScreenManagerProxy_ExitFullScreen:
    case MessageName::WebFullScreenManagerProxy_BeganEnterFullScreen:
    case MessageName::WebFullScreenManagerProxy_BeganExitFullScreen:
    case MessageName::WebFullScreenManagerProxy_Close:
        return ReceiverName::WebFullScreenManagerProxy;
    case MessageName::WebGeolocationManagerProxy_StartUpdating:
    case MessageName::WebGeolocationManagerProxy_StopUpdating:
    case MessageName::WebGeolocationManagerProxy_SetEnableHighAccuracy:
        return ReceiverName::WebGeolocationManagerProxy;
    case MessageName::WebPageProxy_CreateNewPage:
    case MessageName::WebPageProxy_ShowPage:
    case MessageName::WebPageProxy_ClosePage:
    case MessageName::WebPageProxy_RunJavaScriptAlert:
    case MessageName::WebPageProxy_RunJavaScriptConfirm:
    case MessageName::WebPageProxy_RunJavaScriptPrompt:
    case MessageName::WebPageProxy_MouseDidMoveOverElement:
#if ENABLE(NETSCAPE_PLUGIN_API)
    case MessageName::WebPageProxy_UnavailablePluginButtonClicked:
#endif
#if ENABLE(WEBGL)
    case MessageName::WebPageProxy_WebGLPolicyForURL:
#endif
#if ENABLE(WEBGL)
    case MessageName::WebPageProxy_ResolveWebGLPolicyForURL:
#endif
    case MessageName::WebPageProxy_DidChangeViewportProperties:
    case MessageName::WebPageProxy_DidReceiveEvent:
    case MessageName::WebPageProxy_SetCursor:
    case MessageName::WebPageProxy_SetCursorHiddenUntilMouseMoves:
    case MessageName::WebPageProxy_SetStatusText:
    case MessageName::WebPageProxy_SetFocus:
    case MessageName::WebPageProxy_TakeFocus:
    case MessageName::WebPageProxy_FocusedFrameChanged:
    case MessageName::WebPageProxy_SetRenderTreeSize:
    case MessageName::WebPageProxy_SetToolbarsAreVisible:
    case MessageName::WebPageProxy_GetToolbarsAreVisible:
    case MessageName::WebPageProxy_SetMenuBarIsVisible:
    case MessageName::WebPageProxy_GetMenuBarIsVisible:
    case MessageName::WebPageProxy_SetStatusBarIsVisible:
    case MessageName::WebPageProxy_GetStatusBarIsVisible:
    case MessageName::WebPageProxy_SetIsResizable:
    case MessageName::WebPageProxy_SetWindowFrame:
    case MessageName::WebPageProxy_GetWindowFrame:
    case MessageName::WebPageProxy_ScreenToRootView:
    case MessageName::WebPageProxy_RootViewToScreen:
    case MessageName::WebPageProxy_AccessibilityScreenToRootView:
    case MessageName::WebPageProxy_RootViewToAccessibilityScreen:
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_ShowValidationMessage:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_HideValidationMessage:
#endif
    case MessageName::WebPageProxy_RunBeforeUnloadConfirmPanel:
    case MessageName::WebPageProxy_PageDidScroll:
    case MessageName::WebPageProxy_RunOpenPanel:
    case MessageName::WebPageProxy_ShowShareSheet:
    case MessageName::WebPageProxy_PrintFrame:
    case MessageName::WebPageProxy_RunModal:
    case MessageName::WebPageProxy_NotifyScrollerThumbIsVisibleInRect:
    case MessageName::WebPageProxy_RecommendedScrollbarStyleDidChange:
    case MessageName::WebPageProxy_DidChangeScrollbarsForMainFrame:
    case MessageName::WebPageProxy_DidChangeScrollOffsetPinningForMainFrame:
    case MessageName::WebPageProxy_DidChangePageCount:
    case MessageName::WebPageProxy_PageExtendedBackgroundColorDidChange:
#if ENABLE(NETSCAPE_PLUGIN_API)
    case MessageName::WebPageProxy_DidFailToInitializePlugin:
#endif
#if ENABLE(NETSCAPE_PLUGIN_API)
    case MessageName::WebPageProxy_DidBlockInsecurePluginVersion:
#endif
    case MessageName::WebPageProxy_SetCanShortCircuitHorizontalWheelEvents:
    case MessageName::WebPageProxy_DidChangeContentSize:
    case MessageName::WebPageProxy_DidChangeIntrinsicContentSize:
#if ENABLE(INPUT_TYPE_COLOR)
    case MessageName::WebPageProxy_ShowColorPicker:
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    case MessageName::WebPageProxy_SetColorPickerColor:
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    case MessageName::WebPageProxy_EndColorPicker:
#endif
#if ENABLE(DATALIST_ELEMENT)
    case MessageName::WebPageProxy_ShowDataListSuggestions:
#endif
#if ENABLE(DATALIST_ELEMENT)
    case MessageName::WebPageProxy_HandleKeydownInDataList:
#endif
#if ENABLE(DATALIST_ELEMENT)
    case MessageName::WebPageProxy_EndDataListSuggestions:
#endif
    case MessageName::WebPageProxy_DecidePolicyForResponse:
    case MessageName::WebPageProxy_DecidePolicyForNavigationActionAsync:
    case MessageName::WebPageProxy_DecidePolicyForNavigationActionSync:
    case MessageName::WebPageProxy_DecidePolicyForNewWindowAction:
    case MessageName::WebPageProxy_UnableToImplementPolicy:
    case MessageName::WebPageProxy_DidChangeProgress:
    case MessageName::WebPageProxy_DidFinishProgress:
    case MessageName::WebPageProxy_DidStartProgress:
    case MessageName::WebPageProxy_SetNetworkRequestsInProgress:
    case MessageName::WebPageProxy_DidCreateMainFrame:
    case MessageName::WebPageProxy_DidCreateSubframe:
    case MessageName::WebPageProxy_DidCreateWindow:
    case MessageName::WebPageProxy_DidStartProvisionalLoadForFrame:
    case MessageName::WebPageProxy_DidReceiveServerRedirectForProvisionalLoadForFrame:
    case MessageName::WebPageProxy_WillPerformClientRedirectForFrame:
    case MessageName::WebPageProxy_DidCancelClientRedirectForFrame:
    case MessageName::WebPageProxy_DidChangeProvisionalURLForFrame:
    case MessageName::WebPageProxy_DidFailProvisionalLoadForFrame:
    case MessageName::WebPageProxy_DidCommitLoadForFrame:
    case MessageName::WebPageProxy_DidFailLoadForFrame:
    case MessageName::WebPageProxy_DidFinishDocumentLoadForFrame:
    case MessageName::WebPageProxy_DidFinishLoadForFrame:
    case MessageName::WebPageProxy_DidFirstLayoutForFrame:
    case MessageName::WebPageProxy_DidFirstVisuallyNonEmptyLayoutForFrame:
    case MessageName::WebPageProxy_DidReachLayoutMilestone:
    case MessageName::WebPageProxy_DidReceiveTitleForFrame:
    case MessageName::WebPageProxy_DidDisplayInsecureContentForFrame:
    case MessageName::WebPageProxy_DidRunInsecureContentForFrame:
    case MessageName::WebPageProxy_DidDetectXSSForFrame:
    case MessageName::WebPageProxy_DidSameDocumentNavigationForFrame:
    case MessageName::WebPageProxy_DidChangeMainDocument:
    case MessageName::WebPageProxy_DidExplicitOpenForFrame:
    case MessageName::WebPageProxy_DidDestroyNavigation:
    case MessageName::WebPageProxy_MainFramePluginHandlesPageScaleGestureDidChange:
    case MessageName::WebPageProxy_DidNavigateWithNavigationData:
    case MessageName::WebPageProxy_DidPerformClientRedirect:
    case MessageName::WebPageProxy_DidPerformServerRedirect:
    case MessageName::WebPageProxy_DidUpdateHistoryTitle:
    case MessageName::WebPageProxy_DidFinishLoadingDataForCustomContentProvider:
    case MessageName::WebPageProxy_WillSubmitForm:
    case MessageName::WebPageProxy_VoidCallback:
    case MessageName::WebPageProxy_DataCallback:
    case MessageName::WebPageProxy_ImageCallback:
    case MessageName::WebPageProxy_StringCallback:
    case MessageName::WebPageProxy_BoolCallback:
    case MessageName::WebPageProxy_InvalidateStringCallback:
    case MessageName::WebPageProxy_ScriptValueCallback:
    case MessageName::WebPageProxy_ComputedPagesCallback:
    case MessageName::WebPageProxy_ValidateCommandCallback:
    case MessageName::WebPageProxy_EditingRangeCallback:
    case MessageName::WebPageProxy_UnsignedCallback:
    case MessageName::WebPageProxy_RectForCharacterRangeCallback:
#if ENABLE(APPLICATION_MANIFEST)
    case MessageName::WebPageProxy_ApplicationManifestCallback:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPageProxy_AttributedStringForCharacterRangeCallback:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPageProxy_FontAtSelectionCallback:
#endif
    case MessageName::WebPageProxy_FontAttributesCallback:
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_GestureCallback:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_TouchesCallback:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_SelectionContextCallback:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_InterpretKeyEvent:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_DidReceivePositionInformation:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_SaveImageToLibrary:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_ShowPlaybackTargetPicker:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_CommitPotentialTapFailed:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_DidNotHandleTapAsClick:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_DidCompleteSyntheticClick:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_DisableDoubleTapGesturesDuringTapIfNecessary:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_HandleSmartMagnificationInformationForPotentialTap:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_SelectionRectsCallback:
#endif
#if ENABLE(DATA_DETECTION)
    case MessageName::WebPageProxy_SetDataDetectionResult:
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPageProxy_PrintFinishedCallback:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_DrawToPDFCallback:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_NowPlayingInfoCallback:
#endif
    case MessageName::WebPageProxy_FindStringCallback:
    case MessageName::WebPageProxy_PageScaleFactorDidChange:
    case MessageName::WebPageProxy_PluginScaleFactorDidChange:
    case MessageName::WebPageProxy_PluginZoomFactorDidChange:
#if USE(ATK)
    case MessageName::WebPageProxy_BindAccessibilityTree:
#endif
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    case MessageName::WebPageProxy_SetInputMethodState:
#endif
    case MessageName::WebPageProxy_BackForwardAddItem:
    case MessageName::WebPageProxy_BackForwardGoToItem:
    case MessageName::WebPageProxy_BackForwardItemAtIndex:
    case MessageName::WebPageProxy_BackForwardListCounts:
    case MessageName::WebPageProxy_BackForwardClear:
    case MessageName::WebPageProxy_WillGoToBackForwardListItem:
    case MessageName::WebPageProxy_RegisterEditCommandForUndo:
    case MessageName::WebPageProxy_ClearAllEditCommands:
    case MessageName::WebPageProxy_RegisterInsertionUndoGrouping:
    case MessageName::WebPageProxy_CanUndoRedo:
    case MessageName::WebPageProxy_ExecuteUndoRedo:
    case MessageName::WebPageProxy_LogDiagnosticMessage:
    case MessageName::WebPageProxy_LogDiagnosticMessageWithResult:
    case MessageName::WebPageProxy_LogDiagnosticMessageWithValue:
    case MessageName::WebPageProxy_LogDiagnosticMessageWithEnhancedPrivacy:
    case MessageName::WebPageProxy_LogDiagnosticMessageWithValueDictionary:
    case MessageName::WebPageProxy_LogScrollingEvent:
    case MessageName::WebPageProxy_EditorStateChanged:
    case MessageName::WebPageProxy_CompositionWasCanceled:
    case MessageName::WebPageProxy_SetHasHadSelectionChangesFromUserInteraction:
#if HAVE(TOUCH_BAR)
    case MessageName::WebPageProxy_SetIsTouchBarUpdateSupressedForHiddenContentEditable:
#endif
#if HAVE(TOUCH_BAR)
    case MessageName::WebPageProxy_SetIsNeverRichlyEditableForTouchBar:
#endif
    case MessageName::WebPageProxy_RequestDOMPasteAccess:
    case MessageName::WebPageProxy_DidCountStringMatches:
    case MessageName::WebPageProxy_SetTextIndicator:
    case MessageName::WebPageProxy_ClearTextIndicator:
    case MessageName::WebPageProxy_DidFindString:
    case MessageName::WebPageProxy_DidFailToFindString:
    case MessageName::WebPageProxy_DidFindStringMatches:
    case MessageName::WebPageProxy_DidGetImageForFindMatch:
    case MessageName::WebPageProxy_ShowPopupMenu:
    case MessageName::WebPageProxy_HidePopupMenu:
#if ENABLE(CONTEXT_MENUS)
    case MessageName::WebPageProxy_ShowContextMenu:
#endif
    case MessageName::WebPageProxy_ExceededDatabaseQuota:
    case MessageName::WebPageProxy_ReachedApplicationCacheOriginQuota:
    case MessageName::WebPageProxy_RequestGeolocationPermissionForFrame:
    case MessageName::WebPageProxy_RevokeGeolocationAuthorizationToken:
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebPageProxy_RequestUserMediaPermissionForFrame:
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebPageProxy_EnumerateMediaDevicesForFrame:
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebPageProxy_BeginMonitoringCaptureDevices:
#endif
    case MessageName::WebPageProxy_RequestNotificationPermission:
    case MessageName::WebPageProxy_ShowNotification:
    case MessageName::WebPageProxy_CancelNotification:
    case MessageName::WebPageProxy_ClearNotifications:
    case MessageName::WebPageProxy_DidDestroyNotification:
#if USE(UNIFIED_TEXT_CHECKING)
    case MessageName::WebPageProxy_CheckTextOfParagraph:
#endif
    case MessageName::WebPageProxy_CheckSpellingOfString:
    case MessageName::WebPageProxy_CheckGrammarOfString:
    case MessageName::WebPageProxy_SpellingUIIsShowing:
    case MessageName::WebPageProxy_UpdateSpellingUIWithMisspelledWord:
    case MessageName::WebPageProxy_UpdateSpellingUIWithGrammarString:
    case MessageName::WebPageProxy_GetGuessesForWord:
    case MessageName::WebPageProxy_LearnWord:
    case MessageName::WebPageProxy_IgnoreWord:
    case MessageName::WebPageProxy_RequestCheckingOfString:
#if ENABLE(DRAG_SUPPORT)
    case MessageName::WebPageProxy_DidPerformDragControllerAction:
#endif
#if ENABLE(DRAG_SUPPORT)
    case MessageName::WebPageProxy_DidEndDragging:
#endif
#if PLATFORM(COCOA) && ENABLE(DRAG_SUPPORT)
    case MessageName::WebPageProxy_StartDrag:
#endif
#if PLATFORM(COCOA) && ENABLE(DRAG_SUPPORT)
    case MessageName::WebPageProxy_SetPromisedDataForImage:
#endif
#if (PLATFORM(GTK) || PLATFORM(HBD)) && ENABLE(DRAG_SUPPORT)
    case MessageName::WebPageProxy_StartDrag:
#endif
#if ENABLE(DRAG_SUPPORT)
    case MessageName::WebPageProxy_DidPerformDragOperation:
#endif
#if ENABLE(DATA_INTERACTION)
    case MessageName::WebPageProxy_DidHandleDragStartRequest:
#endif
#if ENABLE(DATA_INTERACTION)
    case MessageName::WebPageProxy_DidHandleAdditionalDragItemsRequest:
#endif
#if ENABLE(DATA_INTERACTION)
    case MessageName::WebPageProxy_WillReceiveEditDragSnapshot:
#endif
#if ENABLE(DATA_INTERACTION)
    case MessageName::WebPageProxy_DidReceiveEditDragSnapshot:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_DidPerformDictionaryLookup:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_ExecuteSavedCommandBySelector:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_RegisterWebProcessAccessibilityToken:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_PluginFocusOrWindowFocusChanged:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_SetPluginComplexTextInputState:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_GetIsSpeaking:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_Speak:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_StopSpeaking:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_MakeFirstResponder:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_AssistiveTechnologyMakeFirstResponder:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_SearchWithSpotlight:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_SearchTheWeb:
#endif
#if HAVE(TOUCH_BAR)
    case MessageName::WebPageProxy_TouchBarMenuDataChanged:
#endif
#if HAVE(TOUCH_BAR)
    case MessageName::WebPageProxy_TouchBarMenuItemDataAdded:
#endif
#if HAVE(TOUCH_BAR)
    case MessageName::WebPageProxy_TouchBarMenuItemDataRemoved:
#endif
#if USE(APPKIT)
    case MessageName::WebPageProxy_SubstitutionsPanelIsShowing:
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    case MessageName::WebPageProxy_toggleSmartInsertDelete:
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    case MessageName::WebPageProxy_toggleAutomaticQuoteSubstitution:
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    case MessageName::WebPageProxy_toggleAutomaticLinkDetection:
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    case MessageName::WebPageProxy_toggleAutomaticDashSubstitution:
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    case MessageName::WebPageProxy_toggleAutomaticTextReplacement:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPageProxy_ShowCorrectionPanel:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPageProxy_DismissCorrectionPanel:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPageProxy_DismissCorrectionPanelSoon:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPageProxy_RecordAutocorrectionResponse:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPageProxy_SetEditableElementIsFocused:
#endif
#if USE(DICTATION_ALTERNATIVES)
    case MessageName::WebPageProxy_ShowDictationAlternativeUI:
#endif
#if USE(DICTATION_ALTERNATIVES)
    case MessageName::WebPageProxy_RemoveDictationAlternatives:
#endif
#if USE(DICTATION_ALTERNATIVES)
    case MessageName::WebPageProxy_DictationAlternatives:
#endif
#if PLATFORM(X11) && ENABLE(NETSCAPE_PLUGIN_API)
    case MessageName::WebPageProxy_CreatePluginContainer:
#endif
#if PLATFORM(X11) && ENABLE(NETSCAPE_PLUGIN_API)
    case MessageName::WebPageProxy_WindowedPluginGeometryDidChange:
#endif
#if PLATFORM(X11) && ENABLE(NETSCAPE_PLUGIN_API)
    case MessageName::WebPageProxy_WindowedPluginVisibilityDidChange:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_CouldNotRestorePageState:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_RestorePageState:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_RestorePageCenterAndScale:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_DidGetTapHighlightGeometries:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_ElementDidFocus:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_ElementDidBlur:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_UpdateInputContextAfterBlurringAndRefocusingElement:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_FocusedElementDidChangeInputMode:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_ScrollingNodeScrollWillStartScroll:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_ScrollingNodeScrollDidEndScroll:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_ShowInspectorHighlight:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_HideInspectorHighlight:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_FocusedElementInformationCallback:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_ShowInspectorIndication:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_HideInspectorIndication:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_EnableInspectorNodeSearch:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_DisableInspectorNodeSearch:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_UpdateStringForFind:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_HandleAutocorrectionContext:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_ShowDataDetectorsUIForPositionInformation:
#endif
    case MessageName::WebPageProxy_DidChangeInspectorFrontendCount:
    case MessageName::WebPageProxy_CreateInspectorTarget:
    case MessageName::WebPageProxy_DestroyInspectorTarget:
    case MessageName::WebPageProxy_SendMessageToInspectorFrontend:
    case MessageName::WebPageProxy_SaveRecentSearches:
    case MessageName::WebPageProxy_LoadRecentSearches:
    case MessageName::WebPageProxy_SavePDFToFileInDownloadsFolder:
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_SavePDFToTemporaryFolderAndOpenWithNativeApplication:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPageProxy_OpenPDFFromTemporaryFolderWithNativeApplication:
#endif
#if ENABLE(PDFKIT_PLUGIN)
    case MessageName::WebPageProxy_ShowPDFContextMenu:
#endif
#if ENABLE(NETSCAPE_PLUGIN_API)
    case MessageName::WebPageProxy_FindPlugin:
#endif
    case MessageName::WebPageProxy_DidUpdateActivityState:
#if ENABLE(WEB_CRYPTO)
    case MessageName::WebPageProxy_WrapCryptoKey:
#endif
#if ENABLE(WEB_CRYPTO)
    case MessageName::WebPageProxy_UnwrapCryptoKey:
#endif
#if (ENABLE(TELEPHONE_NUMBER_DETECTION) && PLATFORM(MAC))
    case MessageName::WebPageProxy_ShowTelephoneNumberMenu:
#endif
#if USE(QUICK_LOOK)
    case MessageName::WebPageProxy_DidStartLoadForQuickLookDocumentInMainFrame:
#endif
#if USE(QUICK_LOOK)
    case MessageName::WebPageProxy_DidFinishLoadForQuickLookDocumentInMainFrame:
#endif
#if USE(QUICK_LOOK)
    case MessageName::WebPageProxy_RequestPasswordForQuickLookDocumentInMainFrame:
#endif
#if ENABLE(CONTENT_FILTERING)
    case MessageName::WebPageProxy_ContentFilterDidBlockLoadForFrame:
#endif
    case MessageName::WebPageProxy_IsPlayingMediaDidChange:
    case MessageName::WebPageProxy_HandleAutoplayEvent:
#if ENABLE(MEDIA_SESSION)
    case MessageName::WebPageProxy_HasMediaSessionWithActiveMediaElementsDidChange:
#endif
#if ENABLE(MEDIA_SESSION)
    case MessageName::WebPageProxy_MediaSessionMetadataDidChange:
#endif
#if ENABLE(MEDIA_SESSION)
    case MessageName::WebPageProxy_FocusedContentMediaElementDidChange:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPageProxy_DidPerformImmediateActionHitTest:
#endif
    case MessageName::WebPageProxy_HandleMessage:
    case MessageName::WebPageProxy_HandleSynchronousMessage:
    case MessageName::WebPageProxy_HandleAutoFillButtonClick:
    case MessageName::WebPageProxy_DidResignInputElementStrongPasswordAppearance:
    case MessageName::WebPageProxy_ContentRuleListNotification:
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_AddPlaybackTargetPickerClient:
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_RemovePlaybackTargetPickerClient:
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_ShowPlaybackTargetPicker:
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_PlaybackTargetPickerClientStateDidChange:
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_SetMockMediaPlaybackTargetPickerEnabled:
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_SetMockMediaPlaybackTargetPickerState:
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPageProxy_MockMediaPlaybackTargetPickerDismissPopup:
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    case MessageName::WebPageProxy_SetMockVideoPresentationModeEnabled:
#endif
#if ENABLE(POINTER_LOCK)
    case MessageName::WebPageProxy_RequestPointerLock:
#endif
#if ENABLE(POINTER_LOCK)
    case MessageName::WebPageProxy_RequestPointerUnlock:
#endif
    case MessageName::WebPageProxy_DidFailToSuspendAfterProcessSwap:
    case MessageName::WebPageProxy_DidSuspendAfterProcessSwap:
    case MessageName::WebPageProxy_ImageOrMediaDocumentSizeChanged:
    case MessageName::WebPageProxy_UseFixedLayoutDidChange:
    case MessageName::WebPageProxy_FixedLayoutSizeDidChange:
#if ENABLE(VIDEO) && USE(GSTREAMER)
    case MessageName::WebPageProxy_RequestInstallMissingMediaPlugins:
#endif
    case MessageName::WebPageProxy_DidRestoreScrollPosition:
    case MessageName::WebPageProxy_GetLoadDecisionForIcon:
    case MessageName::WebPageProxy_FinishedLoadingIcon:
#if PLATFORM(MAC)
    case MessageName::WebPageProxy_DidHandleAcceptedCandidate:
#endif
    case MessageName::WebPageProxy_SetIsUsingHighPerformanceWebGL:
    case MessageName::WebPageProxy_StartURLSchemeTask:
    case MessageName::WebPageProxy_StopURLSchemeTask:
    case MessageName::WebPageProxy_LoadSynchronousURLSchemeTask:
#if ENABLE(DEVICE_ORIENTATION)
    case MessageName::WebPageProxy_ShouldAllowDeviceOrientationAndMotionAccess:
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPageProxy_RegisterAttachmentIdentifierFromData:
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPageProxy_RegisterAttachmentIdentifierFromFilePath:
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPageProxy_RegisterAttachmentIdentifier:
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPageProxy_RegisterAttachmentsFromSerializedData:
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPageProxy_CloneAttachmentData:
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPageProxy_DidInsertAttachmentWithIdentifier:
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPageProxy_DidRemoveAttachmentWithIdentifier:
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPageProxy_SerializedAttachmentDataForIdentifiers:
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPageProxy_WritePromisedAttachmentToPasteboard:
#endif
    case MessageName::WebPageProxy_SignedPublicKeyAndChallengeString:
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPageProxy_SpeechSynthesisVoiceList:
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPageProxy_SpeechSynthesisSpeak:
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPageProxy_SpeechSynthesisSetFinishedCallback:
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPageProxy_SpeechSynthesisCancel:
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPageProxy_SpeechSynthesisPause:
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPageProxy_SpeechSynthesisResume:
#endif
    case MessageName::WebPageProxy_ConfigureLoggingChannel:
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPageProxy_ShowEmojiPicker:
#endif
#if HAVE(VISIBILITY_PROPAGATION_VIEW)
    case MessageName::WebPageProxy_DidCreateContextForVisibilityPropagation:
#endif
#if ENABLE(WEB_AUTHN)
    case MessageName::WebPageProxy_SetMockWebAuthenticationConfiguration:
#endif
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    case MessageName::WebPageProxy_SendMessageToWebView:
#endif
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    case MessageName::WebPageProxy_SendMessageToWebViewWithReply:
#endif
    case MessageName::WebPageProxy_DidFindTextManipulationItems:
#if ENABLE(MEDIA_USAGE)
    case MessageName::WebPageProxy_AddMediaUsageManagerSession:
#endif
#if ENABLE(MEDIA_USAGE)
    case MessageName::WebPageProxy_UpdateMediaUsageManagerSessionState:
#endif
#if ENABLE(MEDIA_USAGE)
    case MessageName::WebPageProxy_RemoveMediaUsageManagerSession:
#endif
    case MessageName::WebPageProxy_SetHasExecutedAppBoundBehaviorBeforeNavigation:
        return ReceiverName::WebPageProxy;
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPasteboardProxy_WriteURLToPasteboard:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPasteboardProxy_WriteWebContentToPasteboard:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPasteboardProxy_WriteImageToPasteboard:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPasteboardProxy_WriteStringToPasteboard:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPasteboardProxy_UpdateSupportedTypeIdentifiers:
#endif
    case MessageName::WebPasteboardProxy_WriteCustomData:
    case MessageName::WebPasteboardProxy_TypesSafeForDOMToReadAndWrite:
    case MessageName::WebPasteboardProxy_AllPasteboardItemInfo:
    case MessageName::WebPasteboardProxy_InformationForItemAtIndex:
    case MessageName::WebPasteboardProxy_GetPasteboardItemsCount:
    case MessageName::WebPasteboardProxy_ReadStringFromPasteboard:
    case MessageName::WebPasteboardProxy_ReadURLFromPasteboard:
    case MessageName::WebPasteboardProxy_ReadBufferFromPasteboard:
    case MessageName::WebPasteboardProxy_ContainsStringSafeForDOMToReadForType:
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_GetNumberOfFiles:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_GetPasteboardTypes:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_GetPasteboardPathnamesForType:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_GetPasteboardStringForType:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_GetPasteboardStringsForType:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_GetPasteboardBufferForType:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_GetPasteboardChangeCount:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_GetPasteboardColor:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_GetPasteboardURL:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_AddPasteboardTypes:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_SetPasteboardTypes:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_SetPasteboardURL:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_SetPasteboardColor:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_SetPasteboardStringForType:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_SetPasteboardBufferForType:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_ContainsURLStringSuitableForLoading:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPasteboardProxy_URLStringSuitableForLoading:
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPasteboardProxy_GetTypes:
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPasteboardProxy_ReadText:
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPasteboardProxy_ReadFilePaths:
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPasteboardProxy_ReadBuffer:
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPasteboardProxy_WriteToClipboard:
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPasteboardProxy_ClearClipboard:
#endif
#if USE(LIBWPE)
    case MessageName::WebPasteboardProxy_GetPasteboardTypes:
#endif
#if USE(LIBWPE)
    case MessageName::WebPasteboardProxy_WriteWebContentToPasteboard:
#endif
#if USE(LIBWPE)
    case MessageName::WebPasteboardProxy_WriteStringToPasteboard:
#endif
        return ReceiverName::WebPasteboardProxy;
    case MessageName::WebProcessPool_HandleMessage:
    case MessageName::WebProcessPool_HandleSynchronousMessage:
#if ENABLE(GAMEPAD)
    case MessageName::WebProcessPool_StartedUsingGamepads:
#endif
#if ENABLE(GAMEPAD)
    case MessageName::WebProcessPool_StoppedUsingGamepads:
#endif
    case MessageName::WebProcessPool_ReportWebContentCPUTime:
        return ReceiverName::WebProcessPool;
    case MessageName::WebProcessProxy_UpdateBackForwardItem:
    case MessageName::WebProcessProxy_DidDestroyFrame:
    case MessageName::WebProcessProxy_DidDestroyUserGestureToken:
    case MessageName::WebProcessProxy_ShouldTerminate:
    case MessageName::WebProcessProxy_EnableSuddenTermination:
    case MessageName::WebProcessProxy_DisableSuddenTermination:
#if ENABLE(NETSCAPE_PLUGIN_API)
    case MessageName::WebProcessProxy_GetPlugins:
#endif
#if ENABLE(NETSCAPE_PLUGIN_API)
    case MessageName::WebProcessProxy_GetPluginProcessConnection:
#endif
    case MessageName::WebProcessProxy_GetNetworkProcessConnection:
#if ENABLE(GPU_PROCESS)
    case MessageName::WebProcessProxy_GetGPUProcessConnection:
#endif
    case MessageName::WebProcessProxy_SetIsHoldingLockedFiles:
    case MessageName::WebProcessProxy_DidExceedActiveMemoryLimit:
    case MessageName::WebProcessProxy_DidExceedInactiveMemoryLimit:
    case MessageName::WebProcessProxy_DidExceedCPULimit:
    case MessageName::WebProcessProxy_StopResponsivenessTimer:
    case MessageName::WebProcessProxy_DidReceiveMainThreadPing:
    case MessageName::WebProcessProxy_DidReceiveBackgroundResponsivenessPing:
    case MessageName::WebProcessProxy_MemoryPressureStatusChanged:
    case MessageName::WebProcessProxy_DidExceedInactiveMemoryLimitWhileActive:
    case MessageName::WebProcessProxy_DidCollectPrewarmInformation:
#if PLATFORM(COCOA)
    case MessageName::WebProcessProxy_CacheMediaMIMETypes:
#endif
#if PLATFORM(MAC)
    case MessageName::WebProcessProxy_RequestHighPerformanceGPU:
#endif
#if PLATFORM(MAC)
    case MessageName::WebProcessProxy_ReleaseHighPerformanceGPU:
#endif
#if PLATFORM(MAC) && ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    case MessageName::WebProcessProxy_StartDisplayLink:
#endif
#if PLATFORM(MAC) && ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    case MessageName::WebProcessProxy_StopDisplayLink:
#endif
    case MessageName::WebProcessProxy_AddPlugInAutoStartOriginHash:
    case MessageName::WebProcessProxy_PlugInDidReceiveUserInteraction:
#if PLATFORM(GTK) || PLATFORM(WPE)
    case MessageName::WebProcessProxy_SendMessageToWebContext:
#endif
#if PLATFORM(GTK) || PLATFORM(WPE)
    case MessageName::WebProcessProxy_SendMessageToWebContextWithReply:
#endif
    case MessageName::WebProcessProxy_DidCreateSleepDisabler:
    case MessageName::WebProcessProxy_DidDestroySleepDisabler:
        return ReceiverName::WebProcessProxy;
    case MessageName::WebAutomationSession_DidEvaluateJavaScriptFunction:
    case MessageName::WebAutomationSession_DidTakeScreenshot:
        return ReceiverName::WebAutomationSession;
    case MessageName::DownloadProxy_DidStart:
    case MessageName::DownloadProxy_DidReceiveAuthenticationChallenge:
    case MessageName::DownloadProxy_WillSendRequest:
    case MessageName::DownloadProxy_DecideDestinationWithSuggestedFilenameAsync:
    case MessageName::DownloadProxy_DidReceiveResponse:
    case MessageName::DownloadProxy_DidReceiveData:
    case MessageName::DownloadProxy_DidCreateDestination:
    case MessageName::DownloadProxy_DidFinish:
    case MessageName::DownloadProxy_DidFail:
    case MessageName::DownloadProxy_DidCancel:
        return ReceiverName::DownloadProxy;
#if HAVE(VISIBILITY_PROPAGATION_VIEW)
    case MessageName::GPUProcessProxy_DidCreateContextForVisibilityPropagation:
#endif
        return ReceiverName::GPUProcessProxy;
    case MessageName::RemoteWebInspectorProxy_FrontendDidClose:
    case MessageName::RemoteWebInspectorProxy_Reopen:
    case MessageName::RemoteWebInspectorProxy_ResetState:
    case MessageName::RemoteWebInspectorProxy_BringToFront:
    case MessageName::RemoteWebInspectorProxy_Save:
    case MessageName::RemoteWebInspectorProxy_Append:
    case MessageName::RemoteWebInspectorProxy_SetForcedAppearance:
    case MessageName::RemoteWebInspectorProxy_SetSheetRect:
    case MessageName::RemoteWebInspectorProxy_StartWindowDrag:
    case MessageName::RemoteWebInspectorProxy_OpenInNewTab:
    case MessageName::RemoteWebInspectorProxy_ShowCertificate:
    case MessageName::RemoteWebInspectorProxy_SendMessageToBackend:
        return ReceiverName::RemoteWebInspectorProxy;
    case MessageName::WebInspectorProxy_OpenLocalInspectorFrontend:
    case MessageName::WebInspectorProxy_SetFrontendConnection:
    case MessageName::WebInspectorProxy_SendMessageToBackend:
    case MessageName::WebInspectorProxy_FrontendLoaded:
    case MessageName::WebInspectorProxy_DidClose:
    case MessageName::WebInspectorProxy_BringToFront:
    case MessageName::WebInspectorProxy_BringInspectedPageToFront:
    case MessageName::WebInspectorProxy_Reopen:
    case MessageName::WebInspectorProxy_ResetState:
    case MessageName::WebInspectorProxy_SetForcedAppearance:
    case MessageName::WebInspectorProxy_InspectedURLChanged:
    case MessageName::WebInspectorProxy_ShowCertificate:
    case MessageName::WebInspectorProxy_ElementSelectionChanged:
    case MessageName::WebInspectorProxy_TimelineRecordingChanged:
    case MessageName::WebInspectorProxy_SetDeveloperPreferenceOverride:
    case MessageName::WebInspectorProxy_Save:
    case MessageName::WebInspectorProxy_Append:
    case MessageName::WebInspectorProxy_AttachBottom:
    case MessageName::WebInspectorProxy_AttachRight:
    case MessageName::WebInspectorProxy_AttachLeft:
    case MessageName::WebInspectorProxy_Detach:
    case MessageName::WebInspectorProxy_AttachAvailabilityChanged:
    case MessageName::WebInspectorProxy_SetAttachedWindowHeight:
    case MessageName::WebInspectorProxy_SetAttachedWindowWidth:
    case MessageName::WebInspectorProxy_SetSheetRect:
    case MessageName::WebInspectorProxy_StartWindowDrag:
        return ReceiverName::WebInspectorProxy;
    case MessageName::NetworkProcessProxy_DidReceiveAuthenticationChallenge:
    case MessageName::NetworkProcessProxy_NegotiatedLegacyTLS:
    case MessageName::NetworkProcessProxy_DidNegotiateModernTLS:
    case MessageName::NetworkProcessProxy_DidFetchWebsiteData:
    case MessageName::NetworkProcessProxy_DidDeleteWebsiteData:
    case MessageName::NetworkProcessProxy_DidDeleteWebsiteDataForOrigins:
    case MessageName::NetworkProcessProxy_DidSyncAllCookies:
    case MessageName::NetworkProcessProxy_TestProcessIncomingSyncMessagesWhenWaitingForSyncReply:
    case MessageName::NetworkProcessProxy_TerminateUnresponsiveServiceWorkerProcesses:
    case MessageName::NetworkProcessProxy_SetIsHoldingLockedFiles:
    case MessageName::NetworkProcessProxy_LogDiagnosticMessage:
    case MessageName::NetworkProcessProxy_LogDiagnosticMessageWithResult:
    case MessageName::NetworkProcessProxy_LogDiagnosticMessageWithValue:
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_LogTestingEvent:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_NotifyResourceLoadStatisticsProcessed:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_NotifyWebsiteDataDeletionForRegistrableDomainsFinished:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_NotifyWebsiteDataScanForRegistrableDomainsFinished:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_NotifyResourceLoadStatisticsTelemetryFinished:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_RequestStorageAccessConfirm:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_DeleteWebsiteDataInUIProcessForRegistrableDomains:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_DidCommitCrossSiteLoadWithDataTransferFromPrevalentResource:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_SetDomainsWithUserInteraction:
#endif
#if ENABLE(CONTENT_EXTENSIONS)
    case MessageName::NetworkProcessProxy_ContentExtensionRules:
#endif
    case MessageName::NetworkProcessProxy_RetrieveCacheStorageParameters:
    case MessageName::NetworkProcessProxy_TerminateWebProcess:
#if ENABLE(SERVICE_WORKER)
    case MessageName::NetworkProcessProxy_EstablishWorkerContextConnectionToNetworkProcess:
#endif
#if ENABLE(SERVICE_WORKER)
    case MessageName::NetworkProcessProxy_WorkerContextConnectionNoLongerNeeded:
#endif
#if ENABLE(SERVICE_WORKER)
    case MessageName::NetworkProcessProxy_RegisterServiceWorkerClientProcess:
#endif
#if ENABLE(SERVICE_WORKER)
    case MessageName::NetworkProcessProxy_UnregisterServiceWorkerClientProcess:
#endif
    case MessageName::NetworkProcessProxy_SetWebProcessHasUploads:
    case MessageName::NetworkProcessProxy_GetAppBoundDomains:
    case MessageName::NetworkProcessProxy_RequestStorageSpace:
    case MessageName::NetworkProcessProxy_ResourceLoadDidSendRequest:
    case MessageName::NetworkProcessProxy_ResourceLoadDidPerformHTTPRedirection:
    case MessageName::NetworkProcessProxy_ResourceLoadDidReceiveChallenge:
    case MessageName::NetworkProcessProxy_ResourceLoadDidReceiveResponse:
    case MessageName::NetworkProcessProxy_ResourceLoadDidCompleteWithError:
        return ReceiverName::NetworkProcessProxy;
    case MessageName::PluginProcessProxy_DidCreateWebProcessConnection:
    case MessageName::PluginProcessProxy_DidGetSitesWithData:
    case MessageName::PluginProcessProxy_DidDeleteWebsiteData:
    case MessageName::PluginProcessProxy_DidDeleteWebsiteDataForHostNames:
#if PLATFORM(COCOA)
    case MessageName::PluginProcessProxy_SetModalWindowIsShowing:
#endif
#if PLATFORM(COCOA)
    case MessageName::PluginProcessProxy_SetFullscreenWindowIsShowing:
#endif
#if PLATFORM(COCOA)
    case MessageName::PluginProcessProxy_LaunchProcess:
#endif
#if PLATFORM(COCOA)
    case MessageName::PluginProcessProxy_LaunchApplicationAtURL:
#endif
#if PLATFORM(COCOA)
    case MessageName::PluginProcessProxy_OpenURL:
#endif
#if PLATFORM(COCOA)
    case MessageName::PluginProcessProxy_OpenFile:
#endif
        return ReceiverName::PluginProcessProxy;
    case MessageName::WebUserContentControllerProxy_DidPostMessage:
        return ReceiverName::WebUserContentControllerProxy;
    case MessageName::WebProcess_InitializeWebProcess:
    case MessageName::WebProcess_SetWebsiteDataStoreParameters:
    case MessageName::WebProcess_CreateWebPage:
    case MessageName::WebProcess_PrewarmGlobally:
    case MessageName::WebProcess_PrewarmWithDomainInformation:
    case MessageName::WebProcess_SetCacheModel:
    case MessageName::WebProcess_RegisterURLSchemeAsEmptyDocument:
    case MessageName::WebProcess_RegisterURLSchemeAsSecure:
    case MessageName::WebProcess_RegisterURLSchemeAsBypassingContentSecurityPolicy:
    case MessageName::WebProcess_SetDomainRelaxationForbiddenForURLScheme:
    case MessageName::WebProcess_RegisterURLSchemeAsLocal:
    case MessageName::WebProcess_RegisterURLSchemeAsNoAccess:
    case MessageName::WebProcess_RegisterURLSchemeAsDisplayIsolated:
    case MessageName::WebProcess_RegisterURLSchemeAsCORSEnabled:
    case MessageName::WebProcess_RegisterURLSchemeAsCachePartitioned:
    case MessageName::WebProcess_RegisterURLSchemeAsCanDisplayOnlyIfCanRequest:
    case MessageName::WebProcess_SetDefaultRequestTimeoutInterval:
    case MessageName::WebProcess_SetAlwaysUsesComplexTextCodePath:
    case MessageName::WebProcess_SetShouldUseFontSmoothing:
    case MessageName::WebProcess_SetResourceLoadStatisticsEnabled:
    case MessageName::WebProcess_ClearResourceLoadStatistics:
    case MessageName::WebProcess_UserPreferredLanguagesChanged:
    case MessageName::WebProcess_FullKeyboardAccessModeChanged:
    case MessageName::WebProcess_DidAddPlugInAutoStartOriginHash:
    case MessageName::WebProcess_ResetPlugInAutoStartOriginHashes:
    case MessageName::WebProcess_SetPluginLoadClientPolicy:
    case MessageName::WebProcess_ResetPluginLoadClientPolicies:
    case MessageName::WebProcess_ClearPluginClientPolicies:
    case MessageName::WebProcess_RefreshPlugins:
    case MessageName::WebProcess_StartMemorySampler:
    case MessageName::WebProcess_StopMemorySampler:
    case MessageName::WebProcess_SetTextCheckerState:
    case MessageName::WebProcess_SetEnhancedAccessibility:
    case MessageName::WebProcess_GarbageCollectJavaScriptObjects:
    case MessageName::WebProcess_SetJavaScriptGarbageCollectorTimerEnabled:
    case MessageName::WebProcess_SetInjectedBundleParameter:
    case MessageName::WebProcess_SetInjectedBundleParameters:
    case MessageName::WebProcess_HandleInjectedBundleMessage:
    case MessageName::WebProcess_FetchWebsiteData:
    case MessageName::WebProcess_DeleteWebsiteData:
    case MessageName::WebProcess_DeleteWebsiteDataForOrigins:
    case MessageName::WebProcess_SetHiddenPageDOMTimerThrottlingIncreaseLimit:
#if PLATFORM(COCOA)
    case MessageName::WebProcess_SetQOS:
#endif
    case MessageName::WebProcess_SetMemoryCacheDisabled:
#if ENABLE(SERVICE_CONTROLS)
    case MessageName::WebProcess_SetEnabledServices:
#endif
    case MessageName::WebProcess_EnsureAutomationSessionProxy:
    case MessageName::WebProcess_DestroyAutomationSessionProxy:
    case MessageName::WebProcess_PrepareToSuspend:
    case MessageName::WebProcess_ProcessDidResume:
    case MessageName::WebProcess_MainThreadPing:
    case MessageName::WebProcess_BackgroundResponsivenessPing:
#if ENABLE(GAMEPAD)
    case MessageName::WebProcess_SetInitialGamepads:
#endif
#if ENABLE(GAMEPAD)
    case MessageName::WebProcess_GamepadConnected:
#endif
#if ENABLE(GAMEPAD)
    case MessageName::WebProcess_GamepadDisconnected:
#endif
#if ENABLE(SERVICE_WORKER)
    case MessageName::WebProcess_EstablishWorkerContextConnectionToNetworkProcess:
#endif
    case MessageName::WebProcess_SetHasSuspendedPageProxy:
    case MessageName::WebProcess_SetIsInProcessCache:
    case MessageName::WebProcess_MarkIsNoLongerPrewarmed:
    case MessageName::WebProcess_GetActivePagesOriginsForTesting:
#if PLATFORM(COCOA)
    case MessageName::WebProcess_SetScreenProperties:
#endif
#if PLATFORM(MAC) && ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    case MessageName::WebProcess_ScrollerStylePreferenceChanged:
#endif
#if PLATFORM(MAC) && ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    case MessageName::WebProcess_DisplayConfigurationChanged:
#endif
#if PLATFORM(IOS_FAMILY) && !PLATFORM(MACCATALYST)
    case MessageName::WebProcess_BacklightLevelDidChange:
#endif
    case MessageName::WebProcess_IsJITEnabled:
#if PLATFORM(COCOA)
    case MessageName::WebProcess_SetMediaMIMETypes:
#endif
#if (PLATFORM(COCOA) && ENABLE(REMOTE_INSPECTOR))
    case MessageName::WebProcess_EnableRemoteWebInspector:
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebProcess_AddMockMediaDevice:
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebProcess_ClearMockMediaDevices:
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebProcess_RemoveMockMediaDevice:
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebProcess_ResetMockMediaDevices:
#endif
#if (ENABLE(MEDIA_STREAM) && ENABLE(SANDBOX_EXTENSIONS))
    case MessageName::WebProcess_GrantUserMediaDeviceSandboxExtensions:
#endif
#if (ENABLE(MEDIA_STREAM) && ENABLE(SANDBOX_EXTENSIONS))
    case MessageName::WebProcess_RevokeUserMediaDeviceSandboxExtensions:
#endif
    case MessageName::WebProcess_ClearCurrentModifierStateForTesting:
    case MessageName::WebProcess_SetBackForwardCacheCapacity:
    case MessageName::WebProcess_ClearCachedPage:
#if PLATFORM(GTK) || PLATFORM(WPE)
    case MessageName::WebProcess_SendMessageToWebExtension:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::WebProcess_SeedResourceLoadStatisticsForTesting:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::WebProcess_SetThirdPartyCookieBlockingMode:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::WebProcess_SetDomainsWithUserInteraction:
#endif
#if PLATFORM(IOS)
    case MessageName::WebProcess_GrantAccessToAssetServices:
#endif
#if PLATFORM(IOS)
    case MessageName::WebProcess_RevokeAccessToAssetServices:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebProcess_UnblockServicesRequiredByAccessibility:
#endif
#if (PLATFORM(COCOA) && ENABLE(CFPREFS_DIRECT_MODE))
    case MessageName::WebProcess_NotifyPreferencesChanged:
#endif
#if (PLATFORM(COCOA) && ENABLE(CFPREFS_DIRECT_MODE))
    case MessageName::WebProcess_UnblockPreferenceService:
#endif
#if PLATFORM(GTK) && !USE(GTK4)
    case MessageName::WebProcess_SetUseSystemAppearanceForScrollbars:
#endif
        return ReceiverName::WebProcess;
    case MessageName::WebAutomationSessionProxy_EvaluateJavaScriptFunction:
    case MessageName::WebAutomationSessionProxy_ResolveChildFrameWithOrdinal:
    case MessageName::WebAutomationSessionProxy_ResolveChildFrameWithNodeHandle:
    case MessageName::WebAutomationSessionProxy_ResolveChildFrameWithName:
    case MessageName::WebAutomationSessionProxy_ResolveParentFrame:
    case MessageName::WebAutomationSessionProxy_FocusFrame:
    case MessageName::WebAutomationSessionProxy_ComputeElementLayout:
    case MessageName::WebAutomationSessionProxy_SelectOptionElement:
    case MessageName::WebAutomationSessionProxy_SetFilesForInputFileUpload:
    case MessageName::WebAutomationSessionProxy_TakeScreenshot:
    case MessageName::WebAutomationSessionProxy_SnapshotRectForScreenshot:
    case MessageName::WebAutomationSessionProxy_GetCookiesForFrame:
    case MessageName::WebAutomationSessionProxy_DeleteCookie:
        return ReceiverName::WebAutomationSessionProxy;
    case MessageName::WebIDBConnectionToServer_DidDeleteDatabase:
    case MessageName::WebIDBConnectionToServer_DidOpenDatabase:
    case MessageName::WebIDBConnectionToServer_DidAbortTransaction:
    case MessageName::WebIDBConnectionToServer_DidCommitTransaction:
    case MessageName::WebIDBConnectionToServer_DidCreateObjectStore:
    case MessageName::WebIDBConnectionToServer_DidDeleteObjectStore:
    case MessageName::WebIDBConnectionToServer_DidRenameObjectStore:
    case MessageName::WebIDBConnectionToServer_DidClearObjectStore:
    case MessageName::WebIDBConnectionToServer_DidCreateIndex:
    case MessageName::WebIDBConnectionToServer_DidDeleteIndex:
    case MessageName::WebIDBConnectionToServer_DidRenameIndex:
    case MessageName::WebIDBConnectionToServer_DidPutOrAdd:
    case MessageName::WebIDBConnectionToServer_DidGetRecord:
    case MessageName::WebIDBConnectionToServer_DidGetAllRecords:
    case MessageName::WebIDBConnectionToServer_DidGetCount:
    case MessageName::WebIDBConnectionToServer_DidDeleteRecord:
    case MessageName::WebIDBConnectionToServer_DidOpenCursor:
    case MessageName::WebIDBConnectionToServer_DidIterateCursor:
    case MessageName::WebIDBConnectionToServer_FireVersionChangeEvent:
    case MessageName::WebIDBConnectionToServer_DidStartTransaction:
    case MessageName::WebIDBConnectionToServer_DidCloseFromServer:
    case MessageName::WebIDBConnectionToServer_NotifyOpenDBRequestBlocked:
    case MessageName::WebIDBConnectionToServer_DidGetAllDatabaseNamesAndVersions:
        return ReceiverName::WebIDBConnectionToServer;
    case MessageName::WebFullScreenManager_RequestExitFullScreen:
    case MessageName::WebFullScreenManager_WillEnterFullScreen:
    case MessageName::WebFullScreenManager_DidEnterFullScreen:
    case MessageName::WebFullScreenManager_WillExitFullScreen:
    case MessageName::WebFullScreenManager_DidExitFullScreen:
    case MessageName::WebFullScreenManager_SetAnimatingFullScreen:
    case MessageName::WebFullScreenManager_SaveScrollPosition:
    case MessageName::WebFullScreenManager_RestoreScrollPosition:
    case MessageName::WebFullScreenManager_SetFullscreenInsets:
    case MessageName::WebFullScreenManager_SetFullscreenAutoHideDuration:
    case MessageName::WebFullScreenManager_SetFullscreenControlsHidden:
        return ReceiverName::WebFullScreenManager;
    case MessageName::GPUProcessConnection_DidReceiveRemoteCommand:
        return ReceiverName::GPUProcessConnection;
    case MessageName::RemoteRenderingBackend_CreateImageBufferBackend:
    case MessageName::RemoteRenderingBackend_CommitImageBufferFlushContext:
        return ReceiverName::RemoteRenderingBackend;
    case MessageName::MediaPlayerPrivateRemote_NetworkStateChanged:
    case MessageName::MediaPlayerPrivateRemote_ReadyStateChanged:
    case MessageName::MediaPlayerPrivateRemote_FirstVideoFrameAvailable:
    case MessageName::MediaPlayerPrivateRemote_VolumeChanged:
    case MessageName::MediaPlayerPrivateRemote_MuteChanged:
    case MessageName::MediaPlayerPrivateRemote_TimeChanged:
    case MessageName::MediaPlayerPrivateRemote_DurationChanged:
    case MessageName::MediaPlayerPrivateRemote_RateChanged:
    case MessageName::MediaPlayerPrivateRemote_PlaybackStateChanged:
    case MessageName::MediaPlayerPrivateRemote_EngineFailedToLoad:
    case MessageName::MediaPlayerPrivateRemote_UpdateCachedState:
    case MessageName::MediaPlayerPrivateRemote_CharacteristicChanged:
    case MessageName::MediaPlayerPrivateRemote_SizeChanged:
    case MessageName::MediaPlayerPrivateRemote_AddRemoteAudioTrack:
    case MessageName::MediaPlayerPrivateRemote_RemoveRemoteAudioTrack:
    case MessageName::MediaPlayerPrivateRemote_RemoteAudioTrackConfigurationChanged:
    case MessageName::MediaPlayerPrivateRemote_AddRemoteTextTrack:
    case MessageName::MediaPlayerPrivateRemote_RemoveRemoteTextTrack:
    case MessageName::MediaPlayerPrivateRemote_RemoteTextTrackConfigurationChanged:
    case MessageName::MediaPlayerPrivateRemote_ParseWebVTTFileHeader:
    case MessageName::MediaPlayerPrivateRemote_ParseWebVTTCueData:
    case MessageName::MediaPlayerPrivateRemote_ParseWebVTTCueDataStruct:
    case MessageName::MediaPlayerPrivateRemote_AddDataCue:
#if ENABLE(DATACUE_VALUE)
    case MessageName::MediaPlayerPrivateRemote_AddDataCueWithType:
#endif
#if ENABLE(DATACUE_VALUE)
    case MessageName::MediaPlayerPrivateRemote_UpdateDataCue:
#endif
#if ENABLE(DATACUE_VALUE)
    case MessageName::MediaPlayerPrivateRemote_RemoveDataCue:
#endif
    case MessageName::MediaPlayerPrivateRemote_AddGenericCue:
    case MessageName::MediaPlayerPrivateRemote_UpdateGenericCue:
    case MessageName::MediaPlayerPrivateRemote_RemoveGenericCue:
    case MessageName::MediaPlayerPrivateRemote_AddRemoteVideoTrack:
    case MessageName::MediaPlayerPrivateRemote_RemoveRemoteVideoTrack:
    case MessageName::MediaPlayerPrivateRemote_RemoteVideoTrackConfigurationChanged:
    case MessageName::MediaPlayerPrivateRemote_RequestResource:
    case MessageName::MediaPlayerPrivateRemote_RemoveResource:
    case MessageName::MediaPlayerPrivateRemote_ResourceNotSupported:
    case MessageName::MediaPlayerPrivateRemote_EngineUpdated:
    case MessageName::MediaPlayerPrivateRemote_ActiveSourceBuffersChanged:
#if ENABLE(ENCRYPTED_MEDIA)
    case MessageName::MediaPlayerPrivateRemote_WaitingForKeyChanged:
#endif
#if ENABLE(ENCRYPTED_MEDIA)
    case MessageName::MediaPlayerPrivateRemote_InitializationDataEncountered:
#endif
#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    case MessageName::MediaPlayerPrivateRemote_MediaPlayerKeyNeeded:
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    case MessageName::MediaPlayerPrivateRemote_CurrentPlaybackTargetIsWirelessChanged:
#endif
        return ReceiverName::MediaPlayerPrivateRemote;
    case MessageName::RemoteAudioDestinationProxy_RenderBuffer:
    case MessageName::RemoteAudioDestinationProxy_DidChangeIsPlaying:
        return ReceiverName::RemoteAudioDestinationProxy;
    case MessageName::RemoteAudioSession_ConfigurationChanged:
    case MessageName::RemoteAudioSession_BeginInterruption:
    case MessageName::RemoteAudioSession_EndInterruption:
        return ReceiverName::RemoteAudioSession;
    case MessageName::RemoteCDMInstanceSession_UpdateKeyStatuses:
    case MessageName::RemoteCDMInstanceSession_SendMessage:
    case MessageName::RemoteCDMInstanceSession_SessionIdChanged:
        return ReceiverName::RemoteCDMInstanceSession;
    case MessageName::RemoteLegacyCDMSession_SendMessage:
    case MessageName::RemoteLegacyCDMSession_SendError:
        return ReceiverName::RemoteLegacyCDMSession;
        return ReceiverName::RemoteMediaPlayerManager;
    case MessageName::LibWebRTCCodecs_FailedDecoding:
    case MessageName::LibWebRTCCodecs_CompletedDecoding:
    case MessageName::LibWebRTCCodecs_CompletedEncoding:
        return ReceiverName::LibWebRTCCodecs;
    case MessageName::SampleBufferDisplayLayer_SetDidFail:
        return ReceiverName::SampleBufferDisplayLayer;
    case MessageName::WebGeolocationManager_DidChangePosition:
    case MessageName::WebGeolocationManager_DidFailToDeterminePosition:
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebGeolocationManager_ResetPermissions:
#endif
        return ReceiverName::WebGeolocationManager;
    case MessageName::RemoteWebInspectorUI_Initialize:
    case MessageName::RemoteWebInspectorUI_UpdateFindString:
#if ENABLE(INSPECTOR_TELEMETRY)
    case MessageName::RemoteWebInspectorUI_SetDiagnosticLoggingAvailable:
#endif
    case MessageName::RemoteWebInspectorUI_DidSave:
    case MessageName::RemoteWebInspectorUI_DidAppend:
    case MessageName::RemoteWebInspectorUI_SendMessageToFrontend:
        return ReceiverName::RemoteWebInspectorUI;
    case MessageName::WebInspector_Show:
    case MessageName::WebInspector_Close:
    case MessageName::WebInspector_SetAttached:
    case MessageName::WebInspector_ShowConsole:
    case MessageName::WebInspector_ShowResources:
    case MessageName::WebInspector_ShowMainResourceForFrame:
    case MessageName::WebInspector_OpenInNewTab:
    case MessageName::WebInspector_StartPageProfiling:
    case MessageName::WebInspector_StopPageProfiling:
    case MessageName::WebInspector_StartElementSelection:
    case MessageName::WebInspector_StopElementSelection:
    case MessageName::WebInspector_SetFrontendConnection:
        return ReceiverName::WebInspector;
    case MessageName::WebInspectorInterruptDispatcher_NotifyNeedDebuggerBreak:
        return ReceiverName::WebInspectorInterruptDispatcher;
    case MessageName::WebInspectorUI_EstablishConnection:
    case MessageName::WebInspectorUI_UpdateConnection:
    case MessageName::WebInspectorUI_AttachedBottom:
    case MessageName::WebInspectorUI_AttachedRight:
    case MessageName::WebInspectorUI_AttachedLeft:
    case MessageName::WebInspectorUI_Detached:
    case MessageName::WebInspectorUI_SetDockingUnavailable:
    case MessageName::WebInspectorUI_SetIsVisible:
    case MessageName::WebInspectorUI_UpdateFindString:
#if ENABLE(INSPECTOR_TELEMETRY)
    case MessageName::WebInspectorUI_SetDiagnosticLoggingAvailable:
#endif
    case MessageName::WebInspectorUI_ShowConsole:
    case MessageName::WebInspectorUI_ShowResources:
    case MessageName::WebInspectorUI_ShowMainResourceForFrame:
    case MessageName::WebInspectorUI_StartPageProfiling:
    case MessageName::WebInspectorUI_StopPageProfiling:
    case MessageName::WebInspectorUI_StartElementSelection:
    case MessageName::WebInspectorUI_StopElementSelection:
    case MessageName::WebInspectorUI_DidSave:
    case MessageName::WebInspectorUI_DidAppend:
    case MessageName::WebInspectorUI_SendMessageToFrontend:
        return ReceiverName::WebInspectorUI;
    case MessageName::LibWebRTCNetwork_SignalReadPacket:
    case MessageName::LibWebRTCNetwork_SignalSentPacket:
    case MessageName::LibWebRTCNetwork_SignalAddressReady:
    case MessageName::LibWebRTCNetwork_SignalConnect:
    case MessageName::LibWebRTCNetwork_SignalClose:
    case MessageName::LibWebRTCNetwork_SignalNewConnection:
        return ReceiverName::LibWebRTCNetwork;
    case MessageName::WebMDNSRegister_FinishedRegisteringMDNSName:
        return ReceiverName::WebMDNSRegister;
    case MessageName::WebRTCMonitor_NetworksChanged:
        return ReceiverName::WebRTCMonitor;
    case MessageName::WebRTCResolver_SetResolvedAddress:
    case MessageName::WebRTCResolver_ResolvedAddressError:
        return ReceiverName::WebRTCResolver;
#if ENABLE(SHAREABLE_RESOURCE)
    case MessageName::NetworkProcessConnection_DidCacheResource:
#endif
    case MessageName::NetworkProcessConnection_DidFinishPingLoad:
    case MessageName::NetworkProcessConnection_DidFinishPreconnection:
    case MessageName::NetworkProcessConnection_SetOnLineState:
    case MessageName::NetworkProcessConnection_CookieAcceptPolicyChanged:
#if HAVE(COOKIE_CHANGE_LISTENER_API)
    case MessageName::NetworkProcessConnection_CookiesAdded:
#endif
#if HAVE(COOKIE_CHANGE_LISTENER_API)
    case MessageName::NetworkProcessConnection_CookiesDeleted:
#endif
#if HAVE(COOKIE_CHANGE_LISTENER_API)
    case MessageName::NetworkProcessConnection_AllCookiesDeleted:
#endif
    case MessageName::NetworkProcessConnection_CheckProcessLocalPortForActivity:
    case MessageName::NetworkProcessConnection_MessagesAvailableForPort:
    case MessageName::NetworkProcessConnection_BroadcastConsoleMessage:
        return ReceiverName::NetworkProcessConnection;
    case MessageName::WebResourceLoader_WillSendRequest:
    case MessageName::WebResourceLoader_DidSendData:
    case MessageName::WebResourceLoader_DidReceiveResponse:
    case MessageName::WebResourceLoader_DidReceiveData:
    case MessageName::WebResourceLoader_DidReceiveSharedBuffer:
    case MessageName::WebResourceLoader_DidFinishResourceLoad:
    case MessageName::WebResourceLoader_DidFailResourceLoad:
    case MessageName::WebResourceLoader_DidFailServiceWorkerLoad:
    case MessageName::WebResourceLoader_ServiceWorkerDidNotHandle:
    case MessageName::WebResourceLoader_DidBlockAuthenticationChallenge:
    case MessageName::WebResourceLoader_StopLoadingAfterXFrameOptionsOrContentSecurityPolicyDenied:
#if ENABLE(SHAREABLE_RESOURCE)
    case MessageName::WebResourceLoader_DidReceiveResource:
#endif
        return ReceiverName::WebResourceLoader;
    case MessageName::WebSocketChannel_DidConnect:
    case MessageName::WebSocketChannel_DidClose:
    case MessageName::WebSocketChannel_DidReceiveText:
    case MessageName::WebSocketChannel_DidReceiveBinaryData:
    case MessageName::WebSocketChannel_DidReceiveMessageError:
    case MessageName::WebSocketChannel_DidSendHandshakeRequest:
    case MessageName::WebSocketChannel_DidReceiveHandshakeResponse:
        return ReceiverName::WebSocketChannel;
    case MessageName::WebSocketStream_DidOpenSocketStream:
    case MessageName::WebSocketStream_DidCloseSocketStream:
    case MessageName::WebSocketStream_DidReceiveSocketStreamData:
    case MessageName::WebSocketStream_DidFailToReceiveSocketStreamData:
    case MessageName::WebSocketStream_DidUpdateBufferedAmount:
    case MessageName::WebSocketStream_DidFailSocketStream:
    case MessageName::WebSocketStream_DidSendData:
    case MessageName::WebSocketStream_DidSendHandshake:
        return ReceiverName::WebSocketStream;
    case MessageName::WebNotificationManager_DidShowNotification:
    case MessageName::WebNotificationManager_DidClickNotification:
    case MessageName::WebNotificationManager_DidCloseNotifications:
    case MessageName::WebNotificationManager_DidUpdateNotificationDecision:
    case MessageName::WebNotificationManager_DidRemoveNotificationDecisions:
        return ReceiverName::WebNotificationManager;
    case MessageName::PluginProcessConnection_SetException:
        return ReceiverName::PluginProcessConnection;
    case MessageName::PluginProcessConnectionManager_PluginProcessCrashed:
        return ReceiverName::PluginProcessConnectionManager;
    case MessageName::PluginProxy_LoadURL:
    case MessageName::PluginProxy_Update:
    case MessageName::PluginProxy_ProxiesForURL:
    case MessageName::PluginProxy_CookiesForURL:
    case MessageName::PluginProxy_SetCookiesForURL:
    case MessageName::PluginProxy_GetAuthenticationInfo:
    case MessageName::PluginProxy_GetPluginElementNPObject:
    case MessageName::PluginProxy_Evaluate:
    case MessageName::PluginProxy_CancelStreamLoad:
    case MessageName::PluginProxy_ContinueStreamLoad:
    case MessageName::PluginProxy_CancelManualStreamLoad:
    case MessageName::PluginProxy_SetStatusbarText:
#if PLATFORM(COCOA)
    case MessageName::PluginProxy_PluginFocusOrWindowFocusChanged:
#endif
#if PLATFORM(COCOA)
    case MessageName::PluginProxy_SetComplexTextInputState:
#endif
#if PLATFORM(COCOA)
    case MessageName::PluginProxy_SetLayerHostingContextID:
#endif
#if PLATFORM(X11)
    case MessageName::PluginProxy_CreatePluginContainer:
#endif
#if PLATFORM(X11)
    case MessageName::PluginProxy_WindowedPluginGeometryDidChange:
#endif
#if PLATFORM(X11)
    case MessageName::PluginProxy_WindowedPluginVisibilityDidChange:
#endif
    case MessageName::PluginProxy_DidCreatePlugin:
    case MessageName::PluginProxy_DidFailToCreatePlugin:
    case MessageName::PluginProxy_SetPluginIsPlayingAudio:
        return ReceiverName::PluginProxy;
    case MessageName::WebSWClientConnection_JobRejectedInServer:
    case MessageName::WebSWClientConnection_RegistrationJobResolvedInServer:
    case MessageName::WebSWClientConnection_StartScriptFetchForServer:
    case MessageName::WebSWClientConnection_UpdateRegistrationState:
    case MessageName::WebSWClientConnection_UpdateWorkerState:
    case MessageName::WebSWClientConnection_FireUpdateFoundEvent:
    case MessageName::WebSWClientConnection_SetRegistrationLastUpdateTime:
    case MessageName::WebSWClientConnection_SetRegistrationUpdateViaCache:
    case MessageName::WebSWClientConnection_NotifyClientsOfControllerChange:
    case MessageName::WebSWClientConnection_SetSWOriginTableIsImported:
    case MessageName::WebSWClientConnection_SetSWOriginTableSharedMemory:
    case MessageName::WebSWClientConnection_PostMessageToServiceWorkerClient:
    case MessageName::WebSWClientConnection_DidMatchRegistration:
    case MessageName::WebSWClientConnection_DidGetRegistrations:
    case MessageName::WebSWClientConnection_RegistrationReady:
    case MessageName::WebSWClientConnection_SetDocumentIsControlled:
        return ReceiverName::WebSWClientConnection;
    case MessageName::WebSWContextManagerConnection_InstallServiceWorker:
    case MessageName::WebSWContextManagerConnection_StartFetch:
    case MessageName::WebSWContextManagerConnection_CancelFetch:
    case MessageName::WebSWContextManagerConnection_ContinueDidReceiveFetchResponse:
    case MessageName::WebSWContextManagerConnection_PostMessageToServiceWorker:
    case MessageName::WebSWContextManagerConnection_FireInstallEvent:
    case MessageName::WebSWContextManagerConnection_FireActivateEvent:
    case MessageName::WebSWContextManagerConnection_TerminateWorker:
    case MessageName::WebSWContextManagerConnection_FindClientByIdentifierCompleted:
    case MessageName::WebSWContextManagerConnection_MatchAllCompleted:
    case MessageName::WebSWContextManagerConnection_SetUserAgent:
    case MessageName::WebSWContextManagerConnection_UpdatePreferencesStore:
    case MessageName::WebSWContextManagerConnection_Close:
    case MessageName::WebSWContextManagerConnection_SetThrottleState:
        return ReceiverName::WebSWContextManagerConnection;
    case MessageName::WebUserContentController_AddContentWorlds:
    case MessageName::WebUserContentController_RemoveContentWorlds:
    case MessageName::WebUserContentController_AddUserScripts:
    case MessageName::WebUserContentController_RemoveUserScript:
    case MessageName::WebUserContentController_RemoveAllUserScripts:
    case MessageName::WebUserContentController_AddUserStyleSheets:
    case MessageName::WebUserContentController_RemoveUserStyleSheet:
    case MessageName::WebUserContentController_RemoveAllUserStyleSheets:
    case MessageName::WebUserContentController_AddUserScriptMessageHandlers:
    case MessageName::WebUserContentController_RemoveUserScriptMessageHandler:
    case MessageName::WebUserContentController_RemoveAllUserScriptMessageHandlersForWorlds:
    case MessageName::WebUserContentController_RemoveAllUserScriptMessageHandlers:
#if ENABLE(CONTENT_EXTENSIONS)
    case MessageName::WebUserContentController_AddContentRuleLists:
#endif
#if ENABLE(CONTENT_EXTENSIONS)
    case MessageName::WebUserContentController_RemoveContentRuleList:
#endif
#if ENABLE(CONTENT_EXTENSIONS)
    case MessageName::WebUserContentController_RemoveAllContentRuleLists:
#endif
        return ReceiverName::WebUserContentController;
#if USE(COORDINATED_GRAPHICS) || USE(TEXTURE_MAPPER)
    case MessageName::DrawingArea_UpdateBackingStoreState:
#endif
    case MessageName::DrawingArea_DidUpdate:
#if PLATFORM(COCOA)
    case MessageName::DrawingArea_UpdateGeometry:
#endif
#if PLATFORM(COCOA)
    case MessageName::DrawingArea_SetDeviceScaleFactor:
#endif
#if PLATFORM(COCOA)
    case MessageName::DrawingArea_SetColorSpace:
#endif
#if PLATFORM(COCOA)
    case MessageName::DrawingArea_SetViewExposedRect:
#endif
#if PLATFORM(COCOA)
    case MessageName::DrawingArea_AdjustTransientZoom:
#endif
#if PLATFORM(COCOA)
    case MessageName::DrawingArea_CommitTransientZoom:
#endif
#if PLATFORM(COCOA)
    case MessageName::DrawingArea_AcceleratedAnimationDidStart:
#endif
#if PLATFORM(COCOA)
    case MessageName::DrawingArea_AcceleratedAnimationDidEnd:
#endif
#if PLATFORM(COCOA)
    case MessageName::DrawingArea_AddTransactionCallbackID:
#endif
        return ReceiverName::DrawingArea;
    case MessageName::EventDispatcher_WheelEvent:
#if ENABLE(IOS_TOUCH_EVENTS)
    case MessageName::EventDispatcher_TouchEvent:
#endif
#if ENABLE(MAC_GESTURE_EVENTS)
    case MessageName::EventDispatcher_GestureEvent:
#endif
#if ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    case MessageName::EventDispatcher_DisplayWasRefreshed:
#endif
        return ReceiverName::EventDispatcher;
    case MessageName::VisitedLinkTableController_SetVisitedLinkTable:
    case MessageName::VisitedLinkTableController_VisitedLinkStateChanged:
    case MessageName::VisitedLinkTableController_AllVisitedLinkStateChanged:
    case MessageName::VisitedLinkTableController_RemoveAllVisitedLinks:
        return ReceiverName::VisitedLinkTableController;
    case MessageName::WebPage_SetInitialFocus:
    case MessageName::WebPage_SetActivityState:
    case MessageName::WebPage_SetLayerHostingMode:
    case MessageName::WebPage_SetBackgroundColor:
    case MessageName::WebPage_AddConsoleMessage:
    case MessageName::WebPage_SendCSPViolationReport:
    case MessageName::WebPage_EnqueueSecurityPolicyViolationEvent:
    case MessageName::WebPage_TestProcessIncomingSyncMessagesWhenWaitingForSyncReply:
#if PLATFORM(COCOA)
    case MessageName::WebPage_SetTopContentInsetFenced:
#endif
    case MessageName::WebPage_SetTopContentInset:
    case MessageName::WebPage_SetUnderlayColor:
    case MessageName::WebPage_ViewWillStartLiveResize:
    case MessageName::WebPage_ViewWillEndLiveResize:
    case MessageName::WebPage_ExecuteEditCommandWithCallback:
    case MessageName::WebPage_KeyEvent:
    case MessageName::WebPage_MouseEvent:
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetViewportConfigurationViewLayoutSize:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetMaximumUnobscuredSize:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetDeviceOrientation:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetOverrideViewportArguments:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_DynamicViewportSizeUpdate:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetScreenIsBeingCaptured:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_HandleTap:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_PotentialTapAtPosition:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_CommitPotentialTap:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_CancelPotentialTap:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_TapHighlightAtPosition:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_DidRecognizeLongPress:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_HandleDoubleTapForDoubleClickAtPoint:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_InspectorNodeSearchMovedToPosition:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_InspectorNodeSearchEndedAtPosition:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_BlurFocusedElement:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SelectWithGesture:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_UpdateSelectionWithTouches:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SelectWithTwoTouches:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ExtendSelection:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SelectWordBackward:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_MoveSelectionByOffset:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SelectTextWithGranularityAtPoint:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SelectPositionAtBoundaryWithDirection:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_MoveSelectionAtBoundaryWithDirection:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SelectPositionAtPoint:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_BeginSelectionInDirection:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_UpdateSelectionWithExtentPoint:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_UpdateSelectionWithExtentPointAndBoundary:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RequestDictationContext:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ReplaceDictatedText:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ReplaceSelectedText:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RequestAutocorrectionData:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ApplyAutocorrection:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SyncApplyAutocorrection:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RequestAutocorrectionContext:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RequestEvasionRectsAboveSelection:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_GetPositionInformation:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RequestPositionInformation:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_StartInteractionWithElementContextOrPosition:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_StopInteraction:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_PerformActionOnElement:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_FocusNextFocusedElement:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetFocusedElementValue:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_AutofillLoginCredentials:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetFocusedElementValueAsNumber:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetFocusedElementSelectedIndex:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ApplicationWillResignActive:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ApplicationDidEnterBackground:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ApplicationDidFinishSnapshottingAfterEnteringBackground:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ApplicationWillEnterForeground:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ApplicationDidBecomeActive:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ApplicationDidEnterBackgroundForMedia:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ApplicationWillEnterForegroundForMedia:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ContentSizeCategoryDidChange:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_GetSelectionContext:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetAllowsMediaDocumentInlinePlayback:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_HandleTwoFingerTapAtPoint:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_HandleStylusSingleTapAtPoint:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetForceAlwaysUserScalable:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_GetRectsForGranularityWithSelectionOffset:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_GetRectsAtSelectionOffsetWithText:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_StoreSelectionForAccessibility:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_StartAutoscrollAtPosition:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_CancelAutoscroll:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RequestFocusedElementInformation:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_HardwareKeyboardAvailabilityChanged:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetIsShowingInputViewForFocusedElement:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_UpdateSelectionWithDelta:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RequestDocumentEditingContext:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_GenerateSyntheticEditingCommand:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetShouldRevealCurrentSelectionAfterInsertion:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_InsertTextPlaceholder:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RemoveTextPlaceholder:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_TextInputContextsInRect:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_FocusTextInputContextAndPlaceCaret:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ClearServiceWorkerEntitlementOverride:
#endif
    case MessageName::WebPage_SetControlledByAutomation:
    case MessageName::WebPage_ConnectInspector:
    case MessageName::WebPage_DisconnectInspector:
    case MessageName::WebPage_SendMessageToTargetBackend:
#if ENABLE(REMOTE_INSPECTOR)
    case MessageName::WebPage_SetIndicating:
#endif
#if ENABLE(IOS_TOUCH_EVENTS)
    case MessageName::WebPage_ResetPotentialTapSecurityOrigin:
#endif
#if ENABLE(IOS_TOUCH_EVENTS)
    case MessageName::WebPage_TouchEventSync:
#endif
#if !ENABLE(IOS_TOUCH_EVENTS) && ENABLE(TOUCH_EVENTS)
    case MessageName::WebPage_TouchEvent:
#endif
    case MessageName::WebPage_CancelPointer:
    case MessageName::WebPage_TouchWithIdentifierWasRemoved:
#if ENABLE(INPUT_TYPE_COLOR)
    case MessageName::WebPage_DidEndColorPicker:
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    case MessageName::WebPage_DidChooseColor:
#endif
#if ENABLE(DATALIST_ELEMENT)
    case MessageName::WebPage_DidSelectDataListOption:
#endif
#if ENABLE(DATALIST_ELEMENT)
    case MessageName::WebPage_DidCloseSuggestions:
#endif
#if ENABLE(CONTEXT_MENUS)
    case MessageName::WebPage_ContextMenuHidden:
#endif
#if ENABLE(CONTEXT_MENUS)
    case MessageName::WebPage_ContextMenuForKeyEvent:
#endif
    case MessageName::WebPage_ScrollBy:
    case MessageName::WebPage_CenterSelectionInVisibleArea:
    case MessageName::WebPage_GoToBackForwardItem:
    case MessageName::WebPage_TryRestoreScrollPosition:
    case MessageName::WebPage_LoadURLInFrame:
    case MessageName::WebPage_LoadDataInFrame:
    case MessageName::WebPage_LoadRequest:
    case MessageName::WebPage_LoadRequestWaitingForProcessLaunch:
    case MessageName::WebPage_LoadData:
    case MessageName::WebPage_LoadAlternateHTML:
    case MessageName::WebPage_NavigateToPDFLinkWithSimulatedClick:
    case MessageName::WebPage_Reload:
    case MessageName::WebPage_StopLoading:
    case MessageName::WebPage_StopLoadingFrame:
    case MessageName::WebPage_RestoreSession:
    case MessageName::WebPage_UpdateBackForwardListForReattach:
    case MessageName::WebPage_SetCurrentHistoryItemForReattach:
    case MessageName::WebPage_DidRemoveBackForwardItem:
    case MessageName::WebPage_UpdateWebsitePolicies:
    case MessageName::WebPage_NotifyUserScripts:
    case MessageName::WebPage_DidReceivePolicyDecision:
    case MessageName::WebPage_ContinueWillSubmitForm:
    case MessageName::WebPage_ClearSelection:
    case MessageName::WebPage_RestoreSelectionInFocusedEditableElement:
    case MessageName::WebPage_GetContentsAsString:
    case MessageName::WebPage_GetAllFrames:
#if PLATFORM(COCOA)
    case MessageName::WebPage_GetContentsAsAttributedString:
#endif
#if ENABLE(MHTML)
    case MessageName::WebPage_GetContentsAsMHTMLData:
#endif
    case MessageName::WebPage_GetMainResourceDataOfFrame:
    case MessageName::WebPage_GetResourceDataFromFrame:
    case MessageName::WebPage_GetRenderTreeExternalRepresentation:
    case MessageName::WebPage_GetSelectionOrContentsAsString:
    case MessageName::WebPage_GetSelectionAsWebArchiveData:
    case MessageName::WebPage_GetSourceForFrame:
    case MessageName::WebPage_GetWebArchiveOfFrame:
    case MessageName::WebPage_RunJavaScriptInFrameInScriptWorld:
    case MessageName::WebPage_ForceRepaint:
    case MessageName::WebPage_SelectAll:
    case MessageName::WebPage_ScheduleFullEditorStateUpdate:
#if PLATFORM(COCOA)
    case MessageName::WebPage_PerformDictionaryLookupOfCurrentSelection:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_PerformDictionaryLookupAtLocation:
#endif
#if ENABLE(DATA_DETECTION)
    case MessageName::WebPage_DetectDataInAllFrames:
#endif
#if ENABLE(DATA_DETECTION)
    case MessageName::WebPage_RemoveDataDetectedLinks:
#endif
    case MessageName::WebPage_ChangeFont:
    case MessageName::WebPage_ChangeFontAttributes:
    case MessageName::WebPage_PreferencesDidChange:
    case MessageName::WebPage_SetUserAgent:
    case MessageName::WebPage_SetCustomTextEncodingName:
    case MessageName::WebPage_SuspendActiveDOMObjectsAndAnimations:
    case MessageName::WebPage_ResumeActiveDOMObjectsAndAnimations:
    case MessageName::WebPage_Close:
    case MessageName::WebPage_TryClose:
    case MessageName::WebPage_SetEditable:
    case MessageName::WebPage_ValidateCommand:
    case MessageName::WebPage_ExecuteEditCommand:
    case MessageName::WebPage_IncreaseListLevel:
    case MessageName::WebPage_DecreaseListLevel:
    case MessageName::WebPage_ChangeListType:
    case MessageName::WebPage_SetBaseWritingDirection:
    case MessageName::WebPage_SetNeedsFontAttributes:
    case MessageName::WebPage_RequestFontAttributesAtSelectionStart:
    case MessageName::WebPage_DidRemoveEditCommand:
    case MessageName::WebPage_ReapplyEditCommand:
    case MessageName::WebPage_UnapplyEditCommand:
    case MessageName::WebPage_SetPageAndTextZoomFactors:
    case MessageName::WebPage_SetPageZoomFactor:
    case MessageName::WebPage_SetTextZoomFactor:
    case MessageName::WebPage_WindowScreenDidChange:
    case MessageName::WebPage_AccessibilitySettingsDidChange:
    case MessageName::WebPage_ScalePage:
    case MessageName::WebPage_ScalePageInViewCoordinates:
    case MessageName::WebPage_ScaleView:
    case MessageName::WebPage_SetUseFixedLayout:
    case MessageName::WebPage_SetFixedLayoutSize:
    case MessageName::WebPage_ListenForLayoutMilestones:
    case MessageName::WebPage_SetSuppressScrollbarAnimations:
    case MessageName::WebPage_SetEnableVerticalRubberBanding:
    case MessageName::WebPage_SetEnableHorizontalRubberBanding:
    case MessageName::WebPage_SetBackgroundExtendsBeyondPage:
    case MessageName::WebPage_SetPaginationMode:
    case MessageName::WebPage_SetPaginationBehavesLikeColumns:
    case MessageName::WebPage_SetPageLength:
    case MessageName::WebPage_SetGapBetweenPages:
    case MessageName::WebPage_SetPaginationLineGridEnabled:
    case MessageName::WebPage_PostInjectedBundleMessage:
    case MessageName::WebPage_FindString:
    case MessageName::WebPage_FindStringMatches:
    case MessageName::WebPage_GetImageForFindMatch:
    case MessageName::WebPage_SelectFindMatch:
    case MessageName::WebPage_IndicateFindMatch:
    case MessageName::WebPage_HideFindUI:
    case MessageName::WebPage_CountStringMatches:
    case MessageName::WebPage_ReplaceMatches:
    case MessageName::WebPage_AddMIMETypeWithCustomContentProvider:
#if (PLATFORM(GTK) || PLATFORM(HBD)) && ENABLE(DRAG_SUPPORT)
    case MessageName::WebPage_PerformDragControllerAction:
#endif
#if !PLATFORM(GTK) && !PLATFORM(HBD) && ENABLE(DRAG_SUPPORT)
    case MessageName::WebPage_PerformDragControllerAction:
#endif
#if ENABLE(DRAG_SUPPORT)
    case MessageName::WebPage_DidStartDrag:
#endif
#if ENABLE(DRAG_SUPPORT)
    case MessageName::WebPage_DragEnded:
#endif
#if ENABLE(DRAG_SUPPORT)
    case MessageName::WebPage_DragCancelled:
#endif
#if PLATFORM(IOS_FAMILY) && ENABLE(DRAG_SUPPORT)
    case MessageName::WebPage_RequestDragStart:
#endif
#if PLATFORM(IOS_FAMILY) && ENABLE(DRAG_SUPPORT)
    case MessageName::WebPage_RequestAdditionalItemsForDragSession:
#endif
#if PLATFORM(IOS_FAMILY) && ENABLE(DRAG_SUPPORT)
    case MessageName::WebPage_InsertDroppedImagePlaceholders:
#endif
#if PLATFORM(IOS_FAMILY) && ENABLE(DRAG_SUPPORT)
    case MessageName::WebPage_DidConcludeDrop:
#endif
    case MessageName::WebPage_DidChangeSelectedIndexForActivePopupMenu:
    case MessageName::WebPage_SetTextForActivePopupMenu:
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPage_FailedToShowPopupMenu:
#endif
#if ENABLE(CONTEXT_MENUS)
    case MessageName::WebPage_DidSelectItemFromActiveContextMenu:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_DidChooseFilesForOpenPanelWithDisplayStringAndIcon:
#endif
    case MessageName::WebPage_DidChooseFilesForOpenPanel:
    case MessageName::WebPage_DidCancelForOpenPanel:
#if ENABLE(SANDBOX_EXTENSIONS)
    case MessageName::WebPage_ExtendSandboxForFilesFromOpenPanel:
#endif
    case MessageName::WebPage_AdvanceToNextMisspelling:
    case MessageName::WebPage_ChangeSpellingToWord:
    case MessageName::WebPage_DidFinishCheckingText:
    case MessageName::WebPage_DidCancelCheckingText:
#if USE(APPKIT)
    case MessageName::WebPage_UppercaseWord:
#endif
#if USE(APPKIT)
    case MessageName::WebPage_LowercaseWord:
#endif
#if USE(APPKIT)
    case MessageName::WebPage_CapitalizeWord:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_SetSmartInsertDeleteEnabled:
#endif
#if ENABLE(GEOLOCATION)
    case MessageName::WebPage_DidReceiveGeolocationPermissionDecision:
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebPage_UserMediaAccessWasGranted:
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebPage_UserMediaAccessWasDenied:
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebPage_CaptureDevicesChanged:
#endif
    case MessageName::WebPage_StopAllMediaPlayback:
    case MessageName::WebPage_SuspendAllMediaPlayback:
    case MessageName::WebPage_ResumeAllMediaPlayback:
    case MessageName::WebPage_DidReceiveNotificationPermissionDecision:
    case MessageName::WebPage_FreezeLayerTreeDueToSwipeAnimation:
    case MessageName::WebPage_UnfreezeLayerTreeDueToSwipeAnimation:
    case MessageName::WebPage_BeginPrinting:
    case MessageName::WebPage_EndPrinting:
    case MessageName::WebPage_ComputePagesForPrinting:
#if PLATFORM(COCOA)
    case MessageName::WebPage_DrawRectToImage:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_DrawPagesToPDF:
#endif
#if (PLATFORM(COCOA) && PLATFORM(IOS_FAMILY))
    case MessageName::WebPage_ComputePagesForPrintingAndDrawToPDF:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_DrawToPDF:
#endif
#if PLATFORM(GTK)
    case MessageName::WebPage_DrawPagesForPrinting:
#endif
    case MessageName::WebPage_SetMediaVolume:
    case MessageName::WebPage_SetMuted:
    case MessageName::WebPage_SetMayStartMediaWhenInWindow:
    case MessageName::WebPage_StopMediaCapture:
#if ENABLE(MEDIA_SESSION)
    case MessageName::WebPage_HandleMediaEvent:
#endif
#if ENABLE(MEDIA_SESSION)
    case MessageName::WebPage_SetVolumeOfMediaElement:
#endif
    case MessageName::WebPage_SetCanRunBeforeUnloadConfirmPanel:
    case MessageName::WebPage_SetCanRunModal:
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    case MessageName::WebPage_CancelComposition:
#endif
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    case MessageName::WebPage_DeleteSurrounding:
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPage_CollapseSelectionInFrame:
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPage_GetCenterForZoomGesture:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_SendComplexTextInputToPlugin:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_WindowAndViewFramesChanged:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_SetMainFrameIsScrollable:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_RegisterUIProcessAccessibilityTokens:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_GetStringSelectionForPasteboard:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_GetDataSelectionForPasteboard:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_ReadSelectionFromPasteboard:
#endif
#if (PLATFORM(COCOA) && ENABLE(SERVICE_CONTROLS))
    case MessageName::WebPage_ReplaceSelectionWithPasteboardData:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_ShouldDelayWindowOrderingEvent:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_AcceptsFirstMouse:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_SetTextAsync:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_InsertTextAsync:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_InsertDictatedTextAsync:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_HasMarkedText:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_GetMarkedRangeAsync:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_GetSelectedRangeAsync:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_CharacterIndexForPointAsync:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_FirstRectForCharacterRangeAsync:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_SetCompositionAsync:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_ConfirmCompositionAsync:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_AttributedSubstringForCharacterRangeAsync:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_FontAtSelection:
#endif
    case MessageName::WebPage_SetAlwaysShowsHorizontalScroller:
    case MessageName::WebPage_SetAlwaysShowsVerticalScroller:
    case MessageName::WebPage_SetMinimumSizeForAutoLayout:
    case MessageName::WebPage_SetSizeToContentAutoSizeMaximumSize:
    case MessageName::WebPage_SetAutoSizingShouldExpandToViewHeight:
    case MessageName::WebPage_SetViewportSizeForCSSViewportUnits:
#if PLATFORM(COCOA)
    case MessageName::WebPage_HandleAlternativeTextUIResult:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_WillStartUserTriggeredZooming:
#endif
    case MessageName::WebPage_SetScrollPinningBehavior:
    case MessageName::WebPage_SetScrollbarOverlayStyle:
    case MessageName::WebPage_GetBytecodeProfile:
    case MessageName::WebPage_GetSamplingProfilerOutput:
    case MessageName::WebPage_TakeSnapshot:
#if PLATFORM(MAC)
    case MessageName::WebPage_PerformImmediateActionHitTestAtLocation:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_ImmediateActionDidUpdate:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_ImmediateActionDidCancel:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_ImmediateActionDidComplete:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_DataDetectorsDidPresentUI:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_DataDetectorsDidChangeUI:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_DataDetectorsDidHideUI:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_HandleAcceptedCandidate:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_SetUseSystemAppearance:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_SetHeaderBannerHeightForTesting:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_SetFooterBannerHeightForTesting:
#endif
#if PLATFORM(MAC)
    case MessageName::WebPage_DidEndMagnificationGesture:
#endif
    case MessageName::WebPage_EffectiveAppearanceDidChange:
#if PLATFORM(GTK)
    case MessageName::WebPage_ThemeDidChange:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_RequestActiveNowPlayingSessionInfo:
#endif
    case MessageName::WebPage_SetShouldDispatchFakeMouseMoveEvents:
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_PlaybackTargetSelected:
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_PlaybackTargetAvailabilityDidChange:
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SetShouldPlayToPlaybackTarget:
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_PlaybackTargetPickerWasDismissed:
#endif
#if ENABLE(POINTER_LOCK)
    case MessageName::WebPage_DidAcquirePointerLock:
#endif
#if ENABLE(POINTER_LOCK)
    case MessageName::WebPage_DidNotAcquirePointerLock:
#endif
#if ENABLE(POINTER_LOCK)
    case MessageName::WebPage_DidLosePointerLock:
#endif
    case MessageName::WebPage_clearWheelEventTestMonitor:
    case MessageName::WebPage_SetShouldScaleViewToFitDocument:
#if ENABLE(VIDEO) && USE(GSTREAMER)
    case MessageName::WebPage_DidEndRequestInstallMissingMediaPlugins:
#endif
    case MessageName::WebPage_SetUserInterfaceLayoutDirection:
    case MessageName::WebPage_DidGetLoadDecisionForIcon:
    case MessageName::WebPage_SetUseIconLoadingClient:
#if ENABLE(GAMEPAD)
    case MessageName::WebPage_GamepadActivity:
#endif
    case MessageName::WebPage_FrameBecameRemote:
    case MessageName::WebPage_RegisterURLSchemeHandler:
    case MessageName::WebPage_URLSchemeTaskDidPerformRedirection:
    case MessageName::WebPage_URLSchemeTaskDidReceiveResponse:
    case MessageName::WebPage_URLSchemeTaskDidReceiveData:
    case MessageName::WebPage_URLSchemeTaskDidComplete:
    case MessageName::WebPage_SetIsSuspended:
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPage_InsertAttachment:
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPage_UpdateAttachmentAttributes:
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    case MessageName::WebPage_UpdateAttachmentIcon:
#endif
#if ENABLE(APPLICATION_MANIFEST)
    case MessageName::WebPage_GetApplicationManifest:
#endif
    case MessageName::WebPage_SetDefersLoading:
    case MessageName::WebPage_UpdateCurrentModifierState:
    case MessageName::WebPage_SimulateDeviceOrientationChange:
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPage_SpeakingErrorOccurred:
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPage_BoundaryEventOccurred:
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPage_VoicesDidChange:
#endif
    case MessageName::WebPage_SetCanShowPlaceholder:
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::WebPage_WasLoadedWithDataTransferFromPrevalentResource:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::WebPage_ClearLoadedThirdPartyDomains:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::WebPage_LoadedThirdPartyDomains:
#endif
#if USE(SYSTEM_PREVIEW)
    case MessageName::WebPage_SystemPreviewActionTriggered:
#endif
#if PLATFORM(GTK) || PLATFORM(WPE)
    case MessageName::WebPage_SendMessageToWebExtension:
#endif
#if PLATFORM(GTK) || PLATFORM(WPE)
    case MessageName::WebPage_SendMessageToWebExtensionWithReply:
#endif
    case MessageName::WebPage_StartTextManipulations:
    case MessageName::WebPage_CompleteTextManipulation:
    case MessageName::WebPage_SetOverriddenMediaType:
    case MessageName::WebPage_GetProcessDisplayName:
    case MessageName::WebPage_UpdateCORSDisablingPatterns:
    case MessageName::WebPage_SetShouldFireEvents:
    case MessageName::WebPage_SetNeedsDOMWindowResizeEvent:
    case MessageName::WebPage_SetHasResourceLoadClient:
        return ReceiverName::WebPage;
    case MessageName::StorageAreaMap_DidSetItem:
    case MessageName::StorageAreaMap_DidRemoveItem:
    case MessageName::StorageAreaMap_DidClear:
    case MessageName::StorageAreaMap_DispatchStorageEvent:
    case MessageName::StorageAreaMap_ClearCache:
        return ReceiverName::StorageAreaMap;
#if PLATFORM(MAC)
    case MessageName::ViewGestureController_DidCollectGeometryForMagnificationGesture:
#endif
#if PLATFORM(MAC)
    case MessageName::ViewGestureController_DidCollectGeometryForSmartMagnificationGesture:
#endif
#if !PLATFORM(IOS_FAMILY)
    case MessageName::ViewGestureController_DidHitRenderTreeSizeThreshold:
#endif
        return ReceiverName::ViewGestureController;
#if PLATFORM(COCOA)
    case MessageName::ViewGestureGeometryCollector_CollectGeometryForSmartMagnificationGesture:
#endif
#if PLATFORM(MAC)
    case MessageName::ViewGestureGeometryCollector_CollectGeometryForMagnificationGesture:
#endif
#if !PLATFORM(IOS_FAMILY)
    case MessageName::ViewGestureGeometryCollector_SetRenderTreeSizeNotificationThreshold:
#endif
        return ReceiverName::ViewGestureGeometryCollector;
    case MessageName::GPUProcess_CreateGPUConnectionToWebProcessReply:
#if ENABLE(MEDIA_STREAM)
    case MessageName::GPUProcess_UpdateCaptureAccessReply:
#endif
    case MessageName::RemoteAudioDestinationManager_DeleteAudioDestinationReply:
    case MessageName::RemoteCDMInstanceProxy_InitializeWithConfigurationReply:
    case MessageName::RemoteCDMInstanceProxy_SetServerCertificateReply:
    case MessageName::RemoteCDMInstanceSessionProxy_RequestLicenseReply:
    case MessageName::RemoteCDMInstanceSessionProxy_UpdateLicenseReply:
    case MessageName::RemoteCDMInstanceSessionProxy_LoadSessionReply:
    case MessageName::RemoteCDMInstanceSessionProxy_CloseSessionReply:
    case MessageName::RemoteCDMInstanceSessionProxy_RemoveSessionDataReply:
    case MessageName::RemoteCDMProxy_GetSupportedConfigurationReply:
    case MessageName::RemoteMediaPlayerManagerProxy_CreateMediaPlayerReply:
    case MessageName::RemoteMediaPlayerProxy_PrepareForPlaybackReply:
    case MessageName::RemoteMediaPlayerProxy_LoadReply:
#if ENABLE(VIDEO_PRESENTATION_MODE)
    case MessageName::RemoteMediaPlayerProxy_EnterFullscreenReply:
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    case MessageName::RemoteMediaPlayerProxy_ExitFullscreenReply:
#endif
    case MessageName::RemoteMediaPlayerProxy_PerformTaskAtMediaTimeReply:
    case MessageName::RemoteMediaResourceManager_ResponseReceivedReply:
    case MessageName::RemoteMediaResourceManager_RedirectReceivedReply:
    case MessageName::RemoteMediaRecorder_FetchDataReply:
    case MessageName::RemoteMediaRecorderManager_CreateRecorderReply:
    case MessageName::RemoteSampleBufferDisplayLayerManager_CreateLayerReply:
    case MessageName::WebCookieManager_GetHostnamesWithCookiesReply:
    case MessageName::WebCookieManager_SetCookieReply:
    case MessageName::WebCookieManager_SetCookiesReply:
    case MessageName::WebCookieManager_GetAllCookiesReply:
    case MessageName::WebCookieManager_GetCookiesReply:
    case MessageName::WebCookieManager_DeleteCookieReply:
    case MessageName::WebCookieManager_DeleteAllCookiesModifiedSinceReply:
    case MessageName::WebCookieManager_SetHTTPCookieAcceptPolicyReply:
    case MessageName::WebCookieManager_GetHTTPCookieAcceptPolicyReply:
    case MessageName::NetworkConnectionToWebProcess_WriteBlobsToTemporaryFilesReply:
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkConnectionToWebProcess_HasStorageAccessReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkConnectionToWebProcess_RequestStorageAccessReply:
#endif
#if ENABLE(SERVICE_WORKER)
    case MessageName::NetworkConnectionToWebProcess_EstablishSWContextConnectionReply:
#endif
    case MessageName::NetworkConnectionToWebProcess_TakeAllMessagesForPortReply:
    case MessageName::NetworkConnectionToWebProcess_CheckRemotePortForActivityReply:
    case MessageName::NetworkProcess_CreateNetworkConnectionToWebProcessReply:
    case MessageName::NetworkProcess_RenameOriginInWebsiteDataReply:
    case MessageName::NetworkProcess_PrepareToSuspendReply:
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ClearPrevalentResourceReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ClearUserInteractionReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_DumpResourceLoadStatisticsReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_UpdatePrevalentDomainsToBlockCookiesForReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsGrandfatheredReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsPrevalentResourceReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsVeryPrevalentResourceReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetAgeCapForClientSideCookiesReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetLastSeenReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_MergeStatisticForTestingReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_InsertExpiredStatisticForTestingReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetPrevalentResourceReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetPrevalentResourceForDebugModeReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsResourceLoadStatisticsEphemeralReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_HadUserInteractionReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsRelationshipOnlyInDatabaseOnceReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_HasLocalStorageReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_GetAllStorageAccessEntriesReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsRegisteredAsRedirectingToReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsRegisteredAsSubFrameUnderReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_IsRegisteredAsSubresourceUnderReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_DomainIDExistsInDatabaseReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_LogUserInteractionReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ResetParametersToDefaultValuesReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ScheduleClearInMemoryAndPersistentReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ScheduleCookieBlockingUpdateReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ScheduleStatisticsAndDataRecordsProcessingReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_StatisticsDatabaseHasAllTablesReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SubmitTelemetryReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetCacheMaxAgeCapForPrevalentResourcesReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetGrandfatheredReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetUseITPDatabaseReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_GetResourceLoadStatisticsDataSummaryReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetGrandfatheringTimeReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetMaxStatisticsEntriesReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetMinimumTimeBetweenDataRecordsRemovalReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetPruneEntriesDownToReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetShouldClassifyResourcesBeforeDataRecordsRemovalReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetNotifyPagesWhenDataRecordsWereScannedReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetIsRunningResourceLoadStatisticsTestReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetNotifyPagesWhenTelemetryWasCapturedReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetResourceLoadStatisticsDebugModeReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetVeryPrevalentResourceReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetSubframeUnderTopFrameDomainReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetSubresourceUnderTopFrameDomainReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetSubresourceUniqueRedirectToReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetSubresourceUniqueRedirectFromReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetTimeToLiveUserInteractionReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetTopFrameUniqueRedirectToReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetTopFrameUniqueRedirectFromReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ResetCacheMaxAgeCapForPrevalentResourcesReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetCrossSiteLoadWithLinkDecorationForTestingReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_ResetCrossSiteLoadsWithLinkDecorationForTestingReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_DeleteCookiesForTestingReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_HasIsolatedSessionReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetAppBoundDomainsForResourceLoadStatisticsReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetShouldDowngradeReferrerForTestingReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetThirdPartyCookieBlockingModeReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetShouldEnbleSameSiteStrictEnforcementForTestingReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetFirstPartyWebsiteDataRemovalModeForTestingReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcess_SetToSameSiteStrictCookiesForTestingReply:
#endif
    case MessageName::NetworkProcess_DumpAdClickAttributionReply:
    case MessageName::NetworkProcess_ClearAdClickAttributionReply:
    case MessageName::NetworkProcess_SetAdClickAttributionOverrideTimerForTestingReply:
    case MessageName::NetworkProcess_SetAdClickAttributionConversionURLForTestingReply:
    case MessageName::NetworkProcess_MarkAdClickAttributionsAsExpiredForTestingReply:
    case MessageName::NetworkProcess_GetLocalStorageOriginDetailsReply:
    case MessageName::NetworkProcess_ResetQuotaReply:
    case MessageName::NetworkProcess_HasAppBoundSessionReply:
    case MessageName::NetworkProcess_ClearAppBoundSessionReply:
#if PLATFORM(IOS_FAMILY) && !PLATFORM(MACCATALYST)
    case MessageName::NetworkProcess_ClearServiceWorkerEntitlementOverrideReply:
#endif
    case MessageName::NetworkProcess_UpdateBundleIdentifierReply:
    case MessageName::NetworkProcess_ClearBundleIdentifierReply:
    case MessageName::NetworkSocketChannel_SendStringReply:
    case MessageName::NetworkSocketChannel_SendDataReply:
    case MessageName::WebSWServerConnection_ScheduleUnregisterJobInServerReply:
    case MessageName::WebSWServerConnection_TerminateWorkerFromClientReply:
    case MessageName::WebSWServerConnection_WhenServiceWorkerIsTerminatedForTestingReply:
    case MessageName::WebSWServerConnection_StoreRegistrationsOnDiskReply:
    case MessageName::WebSWServerToContextConnection_SkipWaitingReply:
    case MessageName::WebSWServerToContextConnection_ClaimReply:
    case MessageName::CacheStorageEngineConnection_OpenReply:
    case MessageName::CacheStorageEngineConnection_RemoveReply:
    case MessageName::CacheStorageEngineConnection_CachesReply:
    case MessageName::CacheStorageEngineConnection_RetrieveRecordsReply:
    case MessageName::CacheStorageEngineConnection_DeleteMatchingRecordsReply:
    case MessageName::CacheStorageEngineConnection_PutRecordsReply:
    case MessageName::CacheStorageEngineConnection_ClearMemoryRepresentationReply:
    case MessageName::CacheStorageEngineConnection_EngineRepresentationReply:
    case MessageName::WebPageProxy_ShowShareSheetReply:
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebPageProxy_EnumerateMediaDevicesForFrameReply:
#endif
#if USE(QUICK_LOOK)
    case MessageName::WebPageProxy_RequestPasswordForQuickLookDocumentInMainFrameReply:
#endif
#if ENABLE(DEVICE_ORIENTATION)
    case MessageName::WebPageProxy_ShouldAllowDeviceOrientationAndMotionAccessReply:
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPageProxy_SpeechSynthesisSpeakReply:
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPageProxy_SpeechSynthesisSetFinishedCallbackReply:
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPageProxy_SpeechSynthesisPauseReply:
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    case MessageName::WebPageProxy_SpeechSynthesisResumeReply:
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    case MessageName::WebPageProxy_ShowEmojiPickerReply:
#endif
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    case MessageName::WebPageProxy_SendMessageToWebViewWithReplyReply:
#endif
#if PLATFORM(GTK) || PLATFORM(WPE)
    case MessageName::WebProcessProxy_SendMessageToWebContextWithReplyReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_RequestStorageAccessConfirmReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::NetworkProcessProxy_DeleteWebsiteDataInUIProcessForRegistrableDomainsReply:
#endif
#if ENABLE(SERVICE_WORKER)
    case MessageName::NetworkProcessProxy_EstablishWorkerContextConnectionToNetworkProcessReply:
#endif
    case MessageName::NetworkProcessProxy_GetAppBoundDomainsReply:
    case MessageName::NetworkProcessProxy_RequestStorageSpaceReply:
    case MessageName::WebUserContentControllerProxy_DidPostMessageReply:
    case MessageName::WebProcess_FetchWebsiteDataReply:
    case MessageName::WebProcess_DeleteWebsiteDataReply:
    case MessageName::WebProcess_DeleteWebsiteDataForOriginsReply:
    case MessageName::WebProcess_PrepareToSuspendReply:
#if ENABLE(SERVICE_WORKER)
    case MessageName::WebProcess_EstablishWorkerContextConnectionToNetworkProcessReply:
#endif
    case MessageName::WebProcess_GetActivePagesOriginsForTestingReply:
    case MessageName::WebProcess_IsJITEnabledReply:
    case MessageName::WebProcess_ClearCachedPageReply:
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::WebProcess_SeedResourceLoadStatisticsForTestingReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::WebProcess_SetThirdPartyCookieBlockingModeReply:
#endif
    case MessageName::WebAutomationSessionProxy_ResolveChildFrameWithOrdinalReply:
    case MessageName::WebAutomationSessionProxy_ResolveChildFrameWithNodeHandleReply:
    case MessageName::WebAutomationSessionProxy_ResolveChildFrameWithNameReply:
    case MessageName::WebAutomationSessionProxy_ResolveParentFrameReply:
    case MessageName::WebAutomationSessionProxy_ComputeElementLayoutReply:
    case MessageName::WebAutomationSessionProxy_SelectOptionElementReply:
    case MessageName::WebAutomationSessionProxy_SetFilesForInputFileUploadReply:
    case MessageName::WebAutomationSessionProxy_SnapshotRectForScreenshotReply:
    case MessageName::WebAutomationSessionProxy_GetCookiesForFrameReply:
    case MessageName::WebAutomationSessionProxy_DeleteCookieReply:
    case MessageName::MediaPlayerPrivateRemote_RequestResourceReply:
    case MessageName::RemoteAudioDestinationProxy_RenderBufferReply:
    case MessageName::NetworkProcessConnection_CheckProcessLocalPortForActivityReply:
    case MessageName::WebSWClientConnection_SetDocumentIsControlledReply:
    case MessageName::WebPage_SetInitialFocusReply:
    case MessageName::WebPage_ExecuteEditCommandWithCallbackReply:
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_MoveSelectionByOffsetReply:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SelectTextWithGranularityAtPointReply:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SelectPositionAtBoundaryWithDirectionReply:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_MoveSelectionAtBoundaryWithDirectionReply:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_SelectPositionAtPointReply:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RequestAutocorrectionDataReply:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RequestEvasionRectsAboveSelectionReply:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_FocusNextFocusedElementReply:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_UpdateSelectionWithDeltaReply:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RequestDocumentEditingContextReply:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_InsertTextPlaceholderReply:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_RemoveTextPlaceholderReply:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_TextInputContextsInRectReply:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_FocusTextInputContextAndPlaceCaretReply:
#endif
#if PLATFORM(IOS_FAMILY)
    case MessageName::WebPage_ClearServiceWorkerEntitlementOverrideReply:
#endif
    case MessageName::WebPage_GetAllFramesReply:
#if PLATFORM(COCOA)
    case MessageName::WebPage_GetContentsAsAttributedStringReply:
#endif
#if ENABLE(DATA_DETECTION)
    case MessageName::WebPage_DetectDataInAllFramesReply:
#endif
#if ENABLE(DATA_DETECTION)
    case MessageName::WebPage_RemoveDataDetectedLinksReply:
#endif
    case MessageName::WebPage_TryCloseReply:
#if PLATFORM(IOS_FAMILY) && ENABLE(DRAG_SUPPORT)
    case MessageName::WebPage_InsertDroppedImagePlaceholdersReply:
#endif
#if ENABLE(MEDIA_STREAM)
    case MessageName::WebPage_UserMediaAccessWasGrantedReply:
#endif
#if PLATFORM(COCOA)
    case MessageName::WebPage_HasMarkedTextReply:
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    case MessageName::WebPage_LoadedThirdPartyDomainsReply:
#endif
#if PLATFORM(GTK) || PLATFORM(WPE)
    case MessageName::WebPage_SendMessageToWebExtensionWithReplyReply:
#endif
    case MessageName::WebPage_StartTextManipulationsReply:
    case MessageName::WebPage_CompleteTextManipulationReply:
    case MessageName::WebPage_GetProcessDisplayNameReply:
        return ReceiverName::AsyncReply;
    case MessageName::WrappedAsyncMessageForTesting:
    case MessageName::SyncMessageReply:
    case MessageName::InitializeConnection:
    case MessageName::LegacySessionState:
        return ReceiverName::IPC;
    }
    ASSERT_NOT_REACHED();
    return ReceiverName::Invalid;
}

bool isValidMessageName(MessageName messageName)
{
    if (messageName == IPC::MessageName::GPUConnectionToWebProcess_CreateRenderingBackend)
        return true;
    if (messageName == IPC::MessageName::GPUConnectionToWebProcess_ReleaseRenderingBackend)
        return true;
    if (messageName == IPC::MessageName::GPUConnectionToWebProcess_ClearNowPlayingInfo)
        return true;
    if (messageName == IPC::MessageName::GPUConnectionToWebProcess_SetNowPlayingInfo)
        return true;
#if USE(AUDIO_SESSION)
    if (messageName == IPC::MessageName::GPUConnectionToWebProcess_EnsureAudioSession)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::GPUConnectionToWebProcess_EnsureMediaSessionHelper)
        return true;
#endif
    if (messageName == IPC::MessageName::GPUProcess_InitializeGPUProcess)
        return true;
    if (messageName == IPC::MessageName::GPUProcess_CreateGPUConnectionToWebProcess)
        return true;
    if (messageName == IPC::MessageName::GPUProcess_CreateGPUConnectionToWebProcessReply)
        return true;
    if (messageName == IPC::MessageName::GPUProcess_ProcessDidTransitionToForeground)
        return true;
    if (messageName == IPC::MessageName::GPUProcess_ProcessDidTransitionToBackground)
        return true;
    if (messageName == IPC::MessageName::GPUProcess_AddSession)
        return true;
    if (messageName == IPC::MessageName::GPUProcess_RemoveSession)
        return true;
#if ENABLE(MEDIA_STREAM)
    if (messageName == IPC::MessageName::GPUProcess_SetMockCaptureDevicesEnabled)
        return true;
#endif
#if ENABLE(MEDIA_STREAM)
    if (messageName == IPC::MessageName::GPUProcess_SetOrientationForMediaCapture)
        return true;
#endif
#if ENABLE(MEDIA_STREAM)
    if (messageName == IPC::MessageName::GPUProcess_UpdateCaptureAccess)
        return true;
    if (messageName == IPC::MessageName::GPUProcess_UpdateCaptureAccessReply)
        return true;
#endif
    if (messageName == IPC::MessageName::RemoteRenderingBackendProxy_CreateImageBuffer)
        return true;
    if (messageName == IPC::MessageName::RemoteRenderingBackendProxy_ReleaseImageBuffer)
        return true;
    if (messageName == IPC::MessageName::RemoteRenderingBackendProxy_FlushImageBufferDrawingContext)
        return true;
    if (messageName == IPC::MessageName::RemoteRenderingBackendProxy_FlushImageBufferDrawingContextAndCommit)
        return true;
    if (messageName == IPC::MessageName::RemoteRenderingBackendProxy_GetImageData)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioDestinationManager_CreateAudioDestination)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioDestinationManager_DeleteAudioDestination)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioDestinationManager_DeleteAudioDestinationReply)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioDestinationManager_StartAudioDestination)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioDestinationManager_StopAudioDestination)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioSessionProxy_SetCategory)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioSessionProxy_SetPreferredBufferSize)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioSessionProxy_TryToSetActive)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMFactoryProxy_CreateCDM)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMFactoryProxy_SupportsKeySystem)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceProxy_CreateSession)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceProxy_InitializeWithConfiguration)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceProxy_InitializeWithConfigurationReply)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceProxy_SetServerCertificate)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceProxy_SetServerCertificateReply)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceProxy_SetStorageDirectory)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceSessionProxy_RequestLicense)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceSessionProxy_RequestLicenseReply)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceSessionProxy_UpdateLicense)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceSessionProxy_UpdateLicenseReply)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceSessionProxy_LoadSession)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceSessionProxy_LoadSessionReply)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceSessionProxy_CloseSession)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceSessionProxy_CloseSessionReply)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceSessionProxy_RemoveSessionData)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceSessionProxy_RemoveSessionDataReply)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceSessionProxy_StoreRecordOfKeyUsage)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMProxy_GetSupportedConfiguration)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMProxy_GetSupportedConfigurationReply)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMProxy_CreateInstance)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMProxy_LoadAndInitialize)
        return true;
    if (messageName == IPC::MessageName::RemoteLegacyCDMFactoryProxy_CreateCDM)
        return true;
    if (messageName == IPC::MessageName::RemoteLegacyCDMFactoryProxy_SupportsKeySystem)
        return true;
    if (messageName == IPC::MessageName::RemoteLegacyCDMProxy_SupportsMIMEType)
        return true;
    if (messageName == IPC::MessageName::RemoteLegacyCDMProxy_CreateSession)
        return true;
    if (messageName == IPC::MessageName::RemoteLegacyCDMProxy_SetPlayerId)
        return true;
    if (messageName == IPC::MessageName::RemoteLegacyCDMSessionProxy_GenerateKeyRequest)
        return true;
    if (messageName == IPC::MessageName::RemoteLegacyCDMSessionProxy_ReleaseKeys)
        return true;
    if (messageName == IPC::MessageName::RemoteLegacyCDMSessionProxy_Update)
        return true;
    if (messageName == IPC::MessageName::RemoteLegacyCDMSessionProxy_CachedKeyForKeyID)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerManagerProxy_CreateMediaPlayer)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerManagerProxy_CreateMediaPlayerReply)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerManagerProxy_DeleteMediaPlayer)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerManagerProxy_GetSupportedTypes)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerManagerProxy_SupportsTypeAndCodecs)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerManagerProxy_CanDecodeExtendedType)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerManagerProxy_OriginsInMediaCache)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerManagerProxy_ClearMediaCache)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerManagerProxy_ClearMediaCacheForOrigins)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerManagerProxy_SupportsKeySystem)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_PrepareForPlayback)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_PrepareForPlaybackReply)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_Load)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_LoadReply)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_CancelLoad)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_PrepareToPlay)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_Play)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_Pause)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SetVolume)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SetMuted)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_Seek)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SeekWithTolerance)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SetPreload)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SetPrivateBrowsingMode)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SetPreservesPitch)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_PrepareForRendering)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SetVisible)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SetShouldMaintainAspectRatio)
        return true;
#if ENABLE(VIDEO_PRESENTATION_MODE)
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SetVideoFullscreenGravity)
        return true;
#endif
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_AcceleratedRenderingStateChanged)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SetShouldDisableSleep)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SetRate)
        return true;
#if ENABLE(VIDEO_PRESENTATION_MODE)
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_UpdateVideoFullscreenInlineImage)
        return true;
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SetVideoFullscreenMode)
        return true;
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_VideoFullscreenStandbyChanged)
        return true;
#endif
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SetBufferingPolicy)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_AudioTrackSetEnabled)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_VideoTrackSetSelected)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_TextTrackSetMode)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SetVideoInlineSizeFenced)
        return true;
#endif
#if (PLATFORM(COCOA) && ENABLE(VIDEO_PRESENTATION_MODE))
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SetVideoFullscreenFrameFenced)
        return true;
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_EnterFullscreen)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_EnterFullscreenReply)
        return true;
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_ExitFullscreen)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_ExitFullscreenReply)
        return true;
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SetWirelessVideoPlaybackDisabled)
        return true;
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SetShouldPlayToPlaybackTarget)
        return true;
#endif
#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SetLegacyCDMSession)
        return true;
#endif
#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_KeyAdded)
        return true;
#endif
#if ENABLE(ENCRYPTED_MEDIA)
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_CdmInstanceAttached)
        return true;
#endif
#if ENABLE(ENCRYPTED_MEDIA)
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_CdmInstanceDetached)
        return true;
#endif
#if ENABLE(ENCRYPTED_MEDIA)
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_AttemptToDecryptWithInstance)
        return true;
#endif
#if ENABLE(LEGACY_ENCRYPTED_MEDIA) && ENABLE(ENCRYPTED_MEDIA)
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SetShouldContinueAfterKeyNeeded)
        return true;
#endif
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_BeginSimulatedHDCPError)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_EndSimulatedHDCPError)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_NotifyActiveSourceBuffersChanged)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_ApplicationWillResignActive)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_ApplicationDidBecomeActive)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_NotifyTrackModeChanged)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_TracksChanged)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SyncTextTrackBounds)
        return true;
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_SetWirelessPlaybackTarget)
        return true;
#endif
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_PerformTaskAtMediaTime)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_PerformTaskAtMediaTimeReply)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_WouldTaintOrigin)
        return true;
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_ErrorLog)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::RemoteMediaPlayerProxy_AccessLog)
        return true;
#endif
    if (messageName == IPC::MessageName::RemoteMediaResourceManager_ResponseReceived)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaResourceManager_ResponseReceivedReply)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaResourceManager_RedirectReceived)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaResourceManager_RedirectReceivedReply)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaResourceManager_DataSent)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaResourceManager_DataReceived)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaResourceManager_AccessControlCheckFailed)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaResourceManager_LoadFailed)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaResourceManager_LoadFinished)
        return true;
    if (messageName == IPC::MessageName::LibWebRTCCodecsProxy_CreateH264Decoder)
        return true;
    if (messageName == IPC::MessageName::LibWebRTCCodecsProxy_CreateH265Decoder)
        return true;
    if (messageName == IPC::MessageName::LibWebRTCCodecsProxy_ReleaseDecoder)
        return true;
    if (messageName == IPC::MessageName::LibWebRTCCodecsProxy_DecodeFrame)
        return true;
    if (messageName == IPC::MessageName::LibWebRTCCodecsProxy_CreateEncoder)
        return true;
    if (messageName == IPC::MessageName::LibWebRTCCodecsProxy_ReleaseEncoder)
        return true;
    if (messageName == IPC::MessageName::LibWebRTCCodecsProxy_InitializeEncoder)
        return true;
    if (messageName == IPC::MessageName::LibWebRTCCodecsProxy_EncodeFrame)
        return true;
    if (messageName == IPC::MessageName::LibWebRTCCodecsProxy_SetEncodeRates)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioMediaStreamTrackRenderer_Start)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioMediaStreamTrackRenderer_Stop)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioMediaStreamTrackRenderer_Clear)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioMediaStreamTrackRenderer_SetVolume)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioMediaStreamTrackRenderer_AudioSamplesStorageChanged)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioMediaStreamTrackRenderer_AudioSamplesAvailable)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioMediaStreamTrackRendererManager_CreateRenderer)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioMediaStreamTrackRendererManager_ReleaseRenderer)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaRecorder_AudioSamplesStorageChanged)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaRecorder_AudioSamplesAvailable)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaRecorder_VideoSampleAvailable)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaRecorder_FetchData)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaRecorder_FetchDataReply)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaRecorder_StopRecording)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaRecorderManager_CreateRecorder)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaRecorderManager_CreateRecorderReply)
        return true;
    if (messageName == IPC::MessageName::RemoteMediaRecorderManager_ReleaseRecorder)
        return true;
    if (messageName == IPC::MessageName::RemoteSampleBufferDisplayLayer_UpdateDisplayMode)
        return true;
    if (messageName == IPC::MessageName::RemoteSampleBufferDisplayLayer_UpdateAffineTransform)
        return true;
    if (messageName == IPC::MessageName::RemoteSampleBufferDisplayLayer_UpdateBoundsAndPosition)
        return true;
    if (messageName == IPC::MessageName::RemoteSampleBufferDisplayLayer_Flush)
        return true;
    if (messageName == IPC::MessageName::RemoteSampleBufferDisplayLayer_FlushAndRemoveImage)
        return true;
    if (messageName == IPC::MessageName::RemoteSampleBufferDisplayLayer_EnqueueSample)
        return true;
    if (messageName == IPC::MessageName::RemoteSampleBufferDisplayLayer_ClearEnqueuedSamples)
        return true;
    if (messageName == IPC::MessageName::RemoteSampleBufferDisplayLayerManager_CreateLayer)
        return true;
    if (messageName == IPC::MessageName::RemoteSampleBufferDisplayLayerManager_CreateLayerReply)
        return true;
    if (messageName == IPC::MessageName::RemoteSampleBufferDisplayLayerManager_ReleaseLayer)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_GetHostnamesWithCookies)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_GetHostnamesWithCookiesReply)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_DeleteCookiesForHostnames)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_DeleteAllCookies)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_SetCookie)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_SetCookieReply)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_SetCookies)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_SetCookiesReply)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_GetAllCookies)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_GetAllCookiesReply)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_GetCookies)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_GetCookiesReply)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_DeleteCookie)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_DeleteCookieReply)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_DeleteAllCookiesModifiedSince)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_DeleteAllCookiesModifiedSinceReply)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_SetHTTPCookieAcceptPolicy)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_SetHTTPCookieAcceptPolicyReply)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_GetHTTPCookieAcceptPolicy)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_GetHTTPCookieAcceptPolicyReply)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_StartObservingCookieChanges)
        return true;
    if (messageName == IPC::MessageName::WebCookieManager_StopObservingCookieChanges)
        return true;
#if USE(SOUP)
    if (messageName == IPC::MessageName::WebCookieManager_SetCookiePersistentStorage)
        return true;
#endif
    if (messageName == IPC::MessageName::WebIDBServer_DeleteDatabase)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_OpenDatabase)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_AbortTransaction)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_CommitTransaction)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_DidFinishHandlingVersionChangeTransaction)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_CreateObjectStore)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_DeleteObjectStore)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_RenameObjectStore)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_ClearObjectStore)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_CreateIndex)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_DeleteIndex)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_RenameIndex)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_PutOrAdd)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_GetRecord)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_GetAllRecords)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_GetCount)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_DeleteRecord)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_OpenCursor)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_IterateCursor)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_EstablishTransaction)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_DatabaseConnectionPendingClose)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_DatabaseConnectionClosed)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_AbortOpenAndUpgradeNeeded)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_DidFireVersionChangeEvent)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_OpenDBRequestCancelled)
        return true;
    if (messageName == IPC::MessageName::WebIDBServer_GetAllDatabaseNamesAndVersions)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_ScheduleResourceLoad)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_PerformSynchronousLoad)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_TestProcessIncomingSyncMessagesWhenWaitingForSyncReply)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_LoadPing)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_RemoveLoadIdentifier)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_PageLoadCompleted)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_BrowsingContextRemoved)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_PrefetchDNS)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_PreconnectTo)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_StartDownload)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_ConvertMainResourceLoadToDownload)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_CookiesForDOM)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_SetCookiesFromDOM)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_CookieRequestHeaderFieldValue)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_GetRawCookies)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_SetRawCookie)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_DeleteCookie)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_DomCookiesForHost)
        return true;
#if HAVE(COOKIE_CHANGE_LISTENER_API)
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_UnsubscribeFromCookieChangeNotifications)
        return true;
#endif
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_RegisterFileBlobURL)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_RegisterBlobURL)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_RegisterBlobURLFromURL)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_RegisterBlobURLOptionallyFileBacked)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_RegisterBlobURLForSlice)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_UnregisterBlobURL)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_BlobSize)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_WriteBlobsToTemporaryFiles)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_WriteBlobsToTemporaryFilesReply)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_SetCaptureExtraNetworkLoadMetricsEnabled)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_CreateSocketStream)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_CreateSocketChannel)
        return true;
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_RemoveStorageAccessForFrame)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_ClearPageSpecificDataForResourceLoadStatistics)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_LogUserInteraction)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_ResourceLoadStatisticsUpdated)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_HasStorageAccess)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_HasStorageAccessReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_RequestStorageAccess)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_RequestStorageAccessReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_RequestStorageAccessUnderOpener)
        return true;
#endif
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_AddOriginAccessWhitelistEntry)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_RemoveOriginAccessWhitelistEntry)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_ResetOriginAccessWhitelists)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_GetNetworkLoadInformationResponse)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_GetNetworkLoadIntermediateInformation)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_TakeNetworkLoadInformationMetrics)
        return true;
#if ENABLE(SERVICE_WORKER)
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_EstablishSWContextConnection)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_EstablishSWContextConnectionReply)
        return true;
#endif
#if ENABLE(SERVICE_WORKER)
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_CloseSWContextConnection)
        return true;
#endif
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_UpdateQuotaBasedOnSpaceUsageForTesting)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_CreateNewMessagePortChannel)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_EntangleLocalPortInThisProcessToRemote)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_MessagePortDisentangled)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_MessagePortClosed)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_TakeAllMessagesForPort)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_TakeAllMessagesForPortReply)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_PostMessageToRemote)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_CheckRemotePortForActivity)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_CheckRemotePortForActivityReply)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_DidDeliverMessagePortMessages)
        return true;
    if (messageName == IPC::MessageName::NetworkConnectionToWebProcess_RegisterURLSchemesAsCORSEnabled)
        return true;
    if (messageName == IPC::MessageName::NetworkContentRuleListManager_Remove)
        return true;
    if (messageName == IPC::MessageName::NetworkContentRuleListManager_AddContentRuleLists)
        return true;
    if (messageName == IPC::MessageName::NetworkContentRuleListManager_RemoveContentRuleList)
        return true;
    if (messageName == IPC::MessageName::NetworkContentRuleListManager_RemoveAllContentRuleLists)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_InitializeNetworkProcess)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_CreateNetworkConnectionToWebProcess)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_CreateNetworkConnectionToWebProcessReply)
        return true;
#if USE(SOUP)
    if (messageName == IPC::MessageName::NetworkProcess_SetIgnoreTLSErrors)
        return true;
#endif
#if USE(SOUP)
    if (messageName == IPC::MessageName::NetworkProcess_UserPreferredLanguagesChanged)
        return true;
#endif
#if USE(SOUP)
    if (messageName == IPC::MessageName::NetworkProcess_SetNetworkProxySettings)
        return true;
#endif
#if USE(SOUP)
    if (messageName == IPC::MessageName::NetworkProcess_PrefetchDNS)
        return true;
#endif
#if USE(CURL)
    if (messageName == IPC::MessageName::NetworkProcess_SetNetworkProxySettings)
        return true;
#endif
    if (messageName == IPC::MessageName::NetworkProcess_ClearCachedCredentials)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_AddWebsiteDataStore)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_DestroySession)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_FetchWebsiteData)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_DeleteWebsiteData)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_DeleteWebsiteDataForOrigins)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_RenameOriginInWebsiteData)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_RenameOriginInWebsiteDataReply)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_DownloadRequest)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ResumeDownload)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_CancelDownload)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::NetworkProcess_PublishDownloadProgress)
        return true;
#endif
    if (messageName == IPC::MessageName::NetworkProcess_ApplicationDidEnterBackground)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ApplicationWillEnterForeground)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ContinueWillSendRequest)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ContinueDecidePendingDownloadDestination)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::NetworkProcess_SetQOS)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::NetworkProcess_SetStorageAccessAPIEnabled)
        return true;
#endif
    if (messageName == IPC::MessageName::NetworkProcess_SetAllowsAnySSLCertificateForWebSocket)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SyncAllCookies)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_AllowSpecificHTTPSCertificateForHost)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetCacheModel)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetCacheModelSynchronouslyForTesting)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ProcessDidTransitionToBackground)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ProcessDidTransitionToForeground)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ProcessWillSuspendImminentlyForTestingSync)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_PrepareToSuspend)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_PrepareToSuspendReply)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ProcessDidResume)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_PreconnectTo)
        return true;
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_ClearPrevalentResource)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ClearPrevalentResourceReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_ClearUserInteraction)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ClearUserInteractionReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_DumpResourceLoadStatistics)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_DumpResourceLoadStatisticsReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetResourceLoadStatisticsEnabled)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetResourceLoadStatisticsLogTestingEvent)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_UpdatePrevalentDomainsToBlockCookiesFor)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_UpdatePrevalentDomainsToBlockCookiesForReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_IsGrandfathered)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_IsGrandfatheredReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_IsPrevalentResource)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_IsPrevalentResourceReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_IsVeryPrevalentResource)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_IsVeryPrevalentResourceReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetAgeCapForClientSideCookies)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetAgeCapForClientSideCookiesReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetLastSeen)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetLastSeenReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_MergeStatisticForTesting)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_MergeStatisticForTestingReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_InsertExpiredStatisticForTesting)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_InsertExpiredStatisticForTestingReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetPrevalentResource)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetPrevalentResourceReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetPrevalentResourceForDebugMode)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetPrevalentResourceForDebugModeReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_IsResourceLoadStatisticsEphemeral)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_IsResourceLoadStatisticsEphemeralReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_HadUserInteraction)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_HadUserInteractionReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_IsRelationshipOnlyInDatabaseOnce)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_IsRelationshipOnlyInDatabaseOnceReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_HasLocalStorage)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_HasLocalStorageReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_GetAllStorageAccessEntries)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_GetAllStorageAccessEntriesReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_IsRegisteredAsRedirectingTo)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_IsRegisteredAsRedirectingToReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_IsRegisteredAsSubFrameUnder)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_IsRegisteredAsSubFrameUnderReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_IsRegisteredAsSubresourceUnder)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_IsRegisteredAsSubresourceUnderReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_DomainIDExistsInDatabase)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_DomainIDExistsInDatabaseReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_LogFrameNavigation)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_LogUserInteraction)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_LogUserInteractionReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_ResetParametersToDefaultValues)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ResetParametersToDefaultValuesReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_ScheduleClearInMemoryAndPersistent)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ScheduleClearInMemoryAndPersistentReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_ScheduleCookieBlockingUpdate)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ScheduleCookieBlockingUpdateReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_ScheduleStatisticsAndDataRecordsProcessing)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ScheduleStatisticsAndDataRecordsProcessingReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_StatisticsDatabaseHasAllTables)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_StatisticsDatabaseHasAllTablesReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SubmitTelemetry)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SubmitTelemetryReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetCacheMaxAgeCapForPrevalentResources)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetCacheMaxAgeCapForPrevalentResourcesReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetGrandfathered)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetGrandfatheredReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetUseITPDatabase)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetUseITPDatabaseReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_GetResourceLoadStatisticsDataSummary)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_GetResourceLoadStatisticsDataSummaryReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetGrandfatheringTime)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetGrandfatheringTimeReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetMaxStatisticsEntries)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetMaxStatisticsEntriesReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetMinimumTimeBetweenDataRecordsRemoval)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetMinimumTimeBetweenDataRecordsRemovalReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetPruneEntriesDownTo)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetPruneEntriesDownToReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetShouldClassifyResourcesBeforeDataRecordsRemoval)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetShouldClassifyResourcesBeforeDataRecordsRemovalReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetNotifyPagesWhenDataRecordsWereScanned)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetNotifyPagesWhenDataRecordsWereScannedReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetIsRunningResourceLoadStatisticsTest)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetIsRunningResourceLoadStatisticsTestReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetNotifyPagesWhenTelemetryWasCaptured)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetNotifyPagesWhenTelemetryWasCapturedReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetResourceLoadStatisticsDebugMode)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetResourceLoadStatisticsDebugModeReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetVeryPrevalentResource)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetVeryPrevalentResourceReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetSubframeUnderTopFrameDomain)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetSubframeUnderTopFrameDomainReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetSubresourceUnderTopFrameDomain)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetSubresourceUnderTopFrameDomainReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetSubresourceUniqueRedirectTo)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetSubresourceUniqueRedirectToReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetSubresourceUniqueRedirectFrom)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetSubresourceUniqueRedirectFromReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetTimeToLiveUserInteraction)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetTimeToLiveUserInteractionReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetTopFrameUniqueRedirectTo)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetTopFrameUniqueRedirectToReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetTopFrameUniqueRedirectFrom)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetTopFrameUniqueRedirectFromReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_ResetCacheMaxAgeCapForPrevalentResources)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ResetCacheMaxAgeCapForPrevalentResourcesReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_DidCommitCrossSiteLoadWithDataTransfer)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetCrossSiteLoadWithLinkDecorationForTesting)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetCrossSiteLoadWithLinkDecorationForTestingReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_ResetCrossSiteLoadsWithLinkDecorationForTesting)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ResetCrossSiteLoadsWithLinkDecorationForTestingReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_DeleteCookiesForTesting)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_DeleteCookiesForTestingReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_HasIsolatedSession)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_HasIsolatedSessionReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetAppBoundDomainsForResourceLoadStatistics)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetAppBoundDomainsForResourceLoadStatisticsReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetShouldDowngradeReferrerForTesting)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetShouldDowngradeReferrerForTestingReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetThirdPartyCookieBlockingMode)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetThirdPartyCookieBlockingModeReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetShouldEnbleSameSiteStrictEnforcementForTesting)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetShouldEnbleSameSiteStrictEnforcementForTestingReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetFirstPartyWebsiteDataRemovalModeForTesting)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetFirstPartyWebsiteDataRemovalModeForTestingReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcess_SetToSameSiteStrictCookiesForTesting)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetToSameSiteStrictCookiesForTestingReply)
        return true;
#endif
    if (messageName == IPC::MessageName::NetworkProcess_SetAdClickAttributionDebugMode)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetSessionIsControlledByAutomation)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_RegisterURLSchemeAsSecure)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_RegisterURLSchemeAsBypassingContentSecurityPolicy)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_RegisterURLSchemeAsLocal)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_RegisterURLSchemeAsNoAccess)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetCacheStorageParameters)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SyncLocalStorage)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ClearLegacyPrivateBrowsingLocalStorage)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_StoreAdClickAttribution)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_DumpAdClickAttribution)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_DumpAdClickAttributionReply)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ClearAdClickAttribution)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ClearAdClickAttributionReply)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetAdClickAttributionOverrideTimerForTesting)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetAdClickAttributionOverrideTimerForTestingReply)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetAdClickAttributionConversionURLForTesting)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetAdClickAttributionConversionURLForTestingReply)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_MarkAdClickAttributionsAsExpiredForTesting)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_MarkAdClickAttributionsAsExpiredForTestingReply)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_GetLocalStorageOriginDetails)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_GetLocalStorageOriginDetailsReply)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_SetServiceWorkerFetchTimeoutForTesting)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ResetServiceWorkerFetchTimeoutForTesting)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ResetQuota)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ResetQuotaReply)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_HasAppBoundSession)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_HasAppBoundSessionReply)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ClearAppBoundSession)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ClearAppBoundSessionReply)
        return true;
#if PLATFORM(IOS_FAMILY) && !PLATFORM(MACCATALYST)
    if (messageName == IPC::MessageName::NetworkProcess_DisableServiceWorkerEntitlement)
        return true;
#endif
#if PLATFORM(IOS_FAMILY) && !PLATFORM(MACCATALYST)
    if (messageName == IPC::MessageName::NetworkProcess_ClearServiceWorkerEntitlementOverride)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ClearServiceWorkerEntitlementOverrideReply)
        return true;
#endif
    if (messageName == IPC::MessageName::NetworkProcess_UpdateBundleIdentifier)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_UpdateBundleIdentifierReply)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ClearBundleIdentifier)
        return true;
    if (messageName == IPC::MessageName::NetworkProcess_ClearBundleIdentifierReply)
        return true;
    if (messageName == IPC::MessageName::NetworkResourceLoader_ContinueWillSendRequest)
        return true;
    if (messageName == IPC::MessageName::NetworkResourceLoader_ContinueDidReceiveResponse)
        return true;
    if (messageName == IPC::MessageName::NetworkSocketChannel_SendString)
        return true;
    if (messageName == IPC::MessageName::NetworkSocketChannel_SendStringReply)
        return true;
    if (messageName == IPC::MessageName::NetworkSocketChannel_SendData)
        return true;
    if (messageName == IPC::MessageName::NetworkSocketChannel_SendDataReply)
        return true;
    if (messageName == IPC::MessageName::NetworkSocketChannel_Close)
        return true;
    if (messageName == IPC::MessageName::NetworkSocketStream_SendData)
        return true;
    if (messageName == IPC::MessageName::NetworkSocketStream_SendHandshake)
        return true;
    if (messageName == IPC::MessageName::NetworkSocketStream_Close)
        return true;
    if (messageName == IPC::MessageName::ServiceWorkerFetchTask_DidNotHandle)
        return true;
    if (messageName == IPC::MessageName::ServiceWorkerFetchTask_DidFail)
        return true;
    if (messageName == IPC::MessageName::ServiceWorkerFetchTask_DidReceiveRedirectResponse)
        return true;
    if (messageName == IPC::MessageName::ServiceWorkerFetchTask_DidReceiveResponse)
        return true;
    if (messageName == IPC::MessageName::ServiceWorkerFetchTask_DidReceiveData)
        return true;
    if (messageName == IPC::MessageName::ServiceWorkerFetchTask_DidReceiveSharedBuffer)
        return true;
    if (messageName == IPC::MessageName::ServiceWorkerFetchTask_DidReceiveFormData)
        return true;
    if (messageName == IPC::MessageName::ServiceWorkerFetchTask_DidFinish)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_ScheduleJobInServer)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_ScheduleUnregisterJobInServer)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_ScheduleUnregisterJobInServerReply)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_FinishFetchingScriptInServer)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_AddServiceWorkerRegistrationInServer)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_RemoveServiceWorkerRegistrationInServer)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_PostMessageToServiceWorker)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_DidResolveRegistrationPromise)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_MatchRegistration)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_WhenRegistrationReady)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_GetRegistrations)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_RegisterServiceWorkerClient)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_UnregisterServiceWorkerClient)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_TerminateWorkerFromClient)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_TerminateWorkerFromClientReply)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_WhenServiceWorkerIsTerminatedForTesting)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_WhenServiceWorkerIsTerminatedForTestingReply)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_SetThrottleState)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_StoreRegistrationsOnDisk)
        return true;
    if (messageName == IPC::MessageName::WebSWServerConnection_StoreRegistrationsOnDiskReply)
        return true;
    if (messageName == IPC::MessageName::WebSWServerToContextConnection_ScriptContextFailedToStart)
        return true;
    if (messageName == IPC::MessageName::WebSWServerToContextConnection_ScriptContextStarted)
        return true;
    if (messageName == IPC::MessageName::WebSWServerToContextConnection_DidFinishInstall)
        return true;
    if (messageName == IPC::MessageName::WebSWServerToContextConnection_DidFinishActivation)
        return true;
    if (messageName == IPC::MessageName::WebSWServerToContextConnection_SetServiceWorkerHasPendingEvents)
        return true;
    if (messageName == IPC::MessageName::WebSWServerToContextConnection_SkipWaiting)
        return true;
    if (messageName == IPC::MessageName::WebSWServerToContextConnection_SkipWaitingReply)
        return true;
    if (messageName == IPC::MessageName::WebSWServerToContextConnection_WorkerTerminated)
        return true;
    if (messageName == IPC::MessageName::WebSWServerToContextConnection_FindClientByIdentifier)
        return true;
    if (messageName == IPC::MessageName::WebSWServerToContextConnection_MatchAll)
        return true;
    if (messageName == IPC::MessageName::WebSWServerToContextConnection_Claim)
        return true;
    if (messageName == IPC::MessageName::WebSWServerToContextConnection_ClaimReply)
        return true;
    if (messageName == IPC::MessageName::WebSWServerToContextConnection_SetScriptResource)
        return true;
    if (messageName == IPC::MessageName::WebSWServerToContextConnection_PostMessageToServiceWorkerClient)
        return true;
    if (messageName == IPC::MessageName::WebSWServerToContextConnection_DidFailHeartBeatCheck)
        return true;
    if (messageName == IPC::MessageName::StorageManagerSet_ConnectToLocalStorageArea)
        return true;
    if (messageName == IPC::MessageName::StorageManagerSet_ConnectToTransientLocalStorageArea)
        return true;
    if (messageName == IPC::MessageName::StorageManagerSet_ConnectToSessionStorageArea)
        return true;
    if (messageName == IPC::MessageName::StorageManagerSet_DisconnectFromStorageArea)
        return true;
    if (messageName == IPC::MessageName::StorageManagerSet_GetValues)
        return true;
    if (messageName == IPC::MessageName::StorageManagerSet_CloneSessionStorageNamespace)
        return true;
    if (messageName == IPC::MessageName::StorageManagerSet_SetItem)
        return true;
    if (messageName == IPC::MessageName::StorageManagerSet_RemoveItem)
        return true;
    if (messageName == IPC::MessageName::StorageManagerSet_Clear)
        return true;
    if (messageName == IPC::MessageName::CacheStorageEngineConnection_Reference)
        return true;
    if (messageName == IPC::MessageName::CacheStorageEngineConnection_Dereference)
        return true;
    if (messageName == IPC::MessageName::CacheStorageEngineConnection_Open)
        return true;
    if (messageName == IPC::MessageName::CacheStorageEngineConnection_OpenReply)
        return true;
    if (messageName == IPC::MessageName::CacheStorageEngineConnection_Remove)
        return true;
    if (messageName == IPC::MessageName::CacheStorageEngineConnection_RemoveReply)
        return true;
    if (messageName == IPC::MessageName::CacheStorageEngineConnection_Caches)
        return true;
    if (messageName == IPC::MessageName::CacheStorageEngineConnection_CachesReply)
        return true;
    if (messageName == IPC::MessageName::CacheStorageEngineConnection_RetrieveRecords)
        return true;
    if (messageName == IPC::MessageName::CacheStorageEngineConnection_RetrieveRecordsReply)
        return true;
    if (messageName == IPC::MessageName::CacheStorageEngineConnection_DeleteMatchingRecords)
        return true;
    if (messageName == IPC::MessageName::CacheStorageEngineConnection_DeleteMatchingRecordsReply)
        return true;
    if (messageName == IPC::MessageName::CacheStorageEngineConnection_PutRecords)
        return true;
    if (messageName == IPC::MessageName::CacheStorageEngineConnection_PutRecordsReply)
        return true;
    if (messageName == IPC::MessageName::CacheStorageEngineConnection_ClearMemoryRepresentation)
        return true;
    if (messageName == IPC::MessageName::CacheStorageEngineConnection_ClearMemoryRepresentationReply)
        return true;
    if (messageName == IPC::MessageName::CacheStorageEngineConnection_EngineRepresentation)
        return true;
    if (messageName == IPC::MessageName::CacheStorageEngineConnection_EngineRepresentationReply)
        return true;
    if (messageName == IPC::MessageName::NetworkMDNSRegister_UnregisterMDNSNames)
        return true;
    if (messageName == IPC::MessageName::NetworkMDNSRegister_RegisterMDNSName)
        return true;
    if (messageName == IPC::MessageName::NetworkRTCMonitor_StartUpdatingIfNeeded)
        return true;
    if (messageName == IPC::MessageName::NetworkRTCMonitor_StopUpdating)
        return true;
    if (messageName == IPC::MessageName::NetworkRTCProvider_CreateUDPSocket)
        return true;
    if (messageName == IPC::MessageName::NetworkRTCProvider_CreateServerTCPSocket)
        return true;
    if (messageName == IPC::MessageName::NetworkRTCProvider_CreateClientTCPSocket)
        return true;
    if (messageName == IPC::MessageName::NetworkRTCProvider_WrapNewTCPConnection)
        return true;
    if (messageName == IPC::MessageName::NetworkRTCProvider_CreateResolver)
        return true;
    if (messageName == IPC::MessageName::NetworkRTCProvider_StopResolver)
        return true;
    if (messageName == IPC::MessageName::NetworkRTCSocket_SendTo)
        return true;
    if (messageName == IPC::MessageName::NetworkRTCSocket_Close)
        return true;
    if (messageName == IPC::MessageName::NetworkRTCSocket_SetOption)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_GeometryDidChange)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_VisibilityDidChange)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_FrameDidFinishLoading)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_FrameDidFail)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_DidEvaluateJavaScript)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_StreamWillSendRequest)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_StreamDidReceiveResponse)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_StreamDidReceiveData)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_StreamDidFinishLoading)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_StreamDidFail)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_ManualStreamDidReceiveResponse)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_ManualStreamDidReceiveData)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_ManualStreamDidFinishLoading)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_ManualStreamDidFail)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_HandleMouseEvent)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_HandleWheelEvent)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_HandleMouseEnterEvent)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_HandleMouseLeaveEvent)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_HandleKeyboardEvent)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_HandleEditingCommand)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_IsEditingCommandEnabled)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_HandlesPageScaleFactor)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_RequiresUnifiedScaleFactor)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_SetFocus)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_DidUpdate)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_PaintEntirePlugin)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_GetPluginScriptableNPObject)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_WindowFocusChanged)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_WindowVisibilityChanged)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::PluginControllerProxy_SendComplexTextInput)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::PluginControllerProxy_WindowAndViewFramesChanged)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::PluginControllerProxy_SetLayerHostingMode)
        return true;
#endif
    if (messageName == IPC::MessageName::PluginControllerProxy_SupportsSnapshotting)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_Snapshot)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_StorageBlockingStateChanged)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_PrivateBrowsingStateChanged)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_GetFormValue)
        return true;
    if (messageName == IPC::MessageName::PluginControllerProxy_MutedStateChanged)
        return true;
    if (messageName == IPC::MessageName::PluginProcess_InitializePluginProcess)
        return true;
    if (messageName == IPC::MessageName::PluginProcess_CreateWebProcessConnection)
        return true;
    if (messageName == IPC::MessageName::PluginProcess_GetSitesWithData)
        return true;
    if (messageName == IPC::MessageName::PluginProcess_DeleteWebsiteData)
        return true;
    if (messageName == IPC::MessageName::PluginProcess_DeleteWebsiteDataForHostNames)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::PluginProcess_SetQOS)
        return true;
#endif
    if (messageName == IPC::MessageName::WebProcessConnection_CreatePlugin)
        return true;
    if (messageName == IPC::MessageName::WebProcessConnection_CreatePluginAsynchronously)
        return true;
    if (messageName == IPC::MessageName::WebProcessConnection_DestroyPlugin)
        return true;
    if (messageName == IPC::MessageName::AuxiliaryProcess_ShutDown)
        return true;
    if (messageName == IPC::MessageName::AuxiliaryProcess_SetProcessSuppressionEnabled)
        return true;
#if OS(LINUX)
    if (messageName == IPC::MessageName::AuxiliaryProcess_DidReceiveMemoryPressureEvent)
        return true;
#endif
    if (messageName == IPC::MessageName::WebConnection_HandleMessage)
        return true;
    if (messageName == IPC::MessageName::AuthenticationManager_CompleteAuthenticationChallenge)
        return true;
    if (messageName == IPC::MessageName::NPObjectMessageReceiver_Deallocate)
        return true;
    if (messageName == IPC::MessageName::NPObjectMessageReceiver_HasMethod)
        return true;
    if (messageName == IPC::MessageName::NPObjectMessageReceiver_Invoke)
        return true;
    if (messageName == IPC::MessageName::NPObjectMessageReceiver_InvokeDefault)
        return true;
    if (messageName == IPC::MessageName::NPObjectMessageReceiver_HasProperty)
        return true;
    if (messageName == IPC::MessageName::NPObjectMessageReceiver_GetProperty)
        return true;
    if (messageName == IPC::MessageName::NPObjectMessageReceiver_SetProperty)
        return true;
    if (messageName == IPC::MessageName::NPObjectMessageReceiver_RemoveProperty)
        return true;
    if (messageName == IPC::MessageName::NPObjectMessageReceiver_Enumerate)
        return true;
    if (messageName == IPC::MessageName::NPObjectMessageReceiver_Construct)
        return true;
    if (messageName == IPC::MessageName::DrawingAreaProxy_EnterAcceleratedCompositingMode)
        return true;
    if (messageName == IPC::MessageName::DrawingAreaProxy_UpdateAcceleratedCompositingMode)
        return true;
    if (messageName == IPC::MessageName::DrawingAreaProxy_DidFirstLayerFlush)
        return true;
    if (messageName == IPC::MessageName::DrawingAreaProxy_DispatchPresentationCallbacksAfterFlushingLayers)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::DrawingAreaProxy_DidUpdateGeometry)
        return true;
#endif
#if USE(COORDINATED_GRAPHICS) || USE(TEXTURE_MAPPER)
    if (messageName == IPC::MessageName::DrawingAreaProxy_Update)
        return true;
#endif
#if USE(COORDINATED_GRAPHICS) || USE(TEXTURE_MAPPER)
    if (messageName == IPC::MessageName::DrawingAreaProxy_DidUpdateBackingStoreState)
        return true;
#endif
#if USE(COORDINATED_GRAPHICS) || USE(TEXTURE_MAPPER)
    if (messageName == IPC::MessageName::DrawingAreaProxy_ExitAcceleratedCompositingMode)
        return true;
#endif
    if (messageName == IPC::MessageName::VisitedLinkStore_AddVisitedLinkHashFromPage)
        return true;
    if (messageName == IPC::MessageName::WebCookieManagerProxy_CookiesDidChange)
        return true;
    if (messageName == IPC::MessageName::WebFullScreenManagerProxy_SupportsFullScreen)
        return true;
    if (messageName == IPC::MessageName::WebFullScreenManagerProxy_EnterFullScreen)
        return true;
    if (messageName == IPC::MessageName::WebFullScreenManagerProxy_ExitFullScreen)
        return true;
    if (messageName == IPC::MessageName::WebFullScreenManagerProxy_BeganEnterFullScreen)
        return true;
    if (messageName == IPC::MessageName::WebFullScreenManagerProxy_BeganExitFullScreen)
        return true;
    if (messageName == IPC::MessageName::WebFullScreenManagerProxy_Close)
        return true;
    if (messageName == IPC::MessageName::WebGeolocationManagerProxy_StartUpdating)
        return true;
    if (messageName == IPC::MessageName::WebGeolocationManagerProxy_StopUpdating)
        return true;
    if (messageName == IPC::MessageName::WebGeolocationManagerProxy_SetEnableHighAccuracy)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_CreateNewPage)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ShowPage)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ClosePage)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_RunJavaScriptAlert)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_RunJavaScriptConfirm)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_RunJavaScriptPrompt)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_MouseDidMoveOverElement)
        return true;
#if ENABLE(NETSCAPE_PLUGIN_API)
    if (messageName == IPC::MessageName::WebPageProxy_UnavailablePluginButtonClicked)
        return true;
#endif
#if ENABLE(WEBGL)
    if (messageName == IPC::MessageName::WebPageProxy_WebGLPolicyForURL)
        return true;
#endif
#if ENABLE(WEBGL)
    if (messageName == IPC::MessageName::WebPageProxy_ResolveWebGLPolicyForURL)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_DidChangeViewportProperties)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidReceiveEvent)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SetCursor)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SetCursorHiddenUntilMouseMoves)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SetStatusText)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SetFocus)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_TakeFocus)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_FocusedFrameChanged)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SetRenderTreeSize)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SetToolbarsAreVisible)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_GetToolbarsAreVisible)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SetMenuBarIsVisible)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_GetMenuBarIsVisible)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SetStatusBarIsVisible)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_GetStatusBarIsVisible)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SetIsResizable)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SetWindowFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_GetWindowFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ScreenToRootView)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_RootViewToScreen)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_AccessibilityScreenToRootView)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_RootViewToAccessibilityScreen)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPageProxy_ShowValidationMessage)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPageProxy_HideValidationMessage)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_RunBeforeUnloadConfirmPanel)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_PageDidScroll)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_RunOpenPanel)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ShowShareSheet)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ShowShareSheetReply)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_PrintFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_RunModal)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_NotifyScrollerThumbIsVisibleInRect)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_RecommendedScrollbarStyleDidChange)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidChangeScrollbarsForMainFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidChangeScrollOffsetPinningForMainFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidChangePageCount)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_PageExtendedBackgroundColorDidChange)
        return true;
#if ENABLE(NETSCAPE_PLUGIN_API)
    if (messageName == IPC::MessageName::WebPageProxy_DidFailToInitializePlugin)
        return true;
#endif
#if ENABLE(NETSCAPE_PLUGIN_API)
    if (messageName == IPC::MessageName::WebPageProxy_DidBlockInsecurePluginVersion)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_SetCanShortCircuitHorizontalWheelEvents)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidChangeContentSize)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidChangeIntrinsicContentSize)
        return true;
#if ENABLE(INPUT_TYPE_COLOR)
    if (messageName == IPC::MessageName::WebPageProxy_ShowColorPicker)
        return true;
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    if (messageName == IPC::MessageName::WebPageProxy_SetColorPickerColor)
        return true;
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    if (messageName == IPC::MessageName::WebPageProxy_EndColorPicker)
        return true;
#endif
#if ENABLE(DATALIST_ELEMENT)
    if (messageName == IPC::MessageName::WebPageProxy_ShowDataListSuggestions)
        return true;
#endif
#if ENABLE(DATALIST_ELEMENT)
    if (messageName == IPC::MessageName::WebPageProxy_HandleKeydownInDataList)
        return true;
#endif
#if ENABLE(DATALIST_ELEMENT)
    if (messageName == IPC::MessageName::WebPageProxy_EndDataListSuggestions)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_DecidePolicyForResponse)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DecidePolicyForNavigationActionAsync)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DecidePolicyForNavigationActionSync)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DecidePolicyForNewWindowAction)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_UnableToImplementPolicy)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidChangeProgress)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidFinishProgress)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidStartProgress)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SetNetworkRequestsInProgress)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidCreateMainFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidCreateSubframe)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidCreateWindow)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidStartProvisionalLoadForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidReceiveServerRedirectForProvisionalLoadForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_WillPerformClientRedirectForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidCancelClientRedirectForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidChangeProvisionalURLForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidFailProvisionalLoadForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidCommitLoadForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidFailLoadForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidFinishDocumentLoadForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidFinishLoadForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidFirstLayoutForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidFirstVisuallyNonEmptyLayoutForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidReachLayoutMilestone)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidReceiveTitleForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidDisplayInsecureContentForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidRunInsecureContentForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidDetectXSSForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidSameDocumentNavigationForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidChangeMainDocument)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidExplicitOpenForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidDestroyNavigation)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_MainFramePluginHandlesPageScaleGestureDidChange)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidNavigateWithNavigationData)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidPerformClientRedirect)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidPerformServerRedirect)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidUpdateHistoryTitle)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidFinishLoadingDataForCustomContentProvider)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_WillSubmitForm)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_VoidCallback)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DataCallback)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ImageCallback)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_StringCallback)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_BoolCallback)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_InvalidateStringCallback)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ScriptValueCallback)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ComputedPagesCallback)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ValidateCommandCallback)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_EditingRangeCallback)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_UnsignedCallback)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_RectForCharacterRangeCallback)
        return true;
#if ENABLE(APPLICATION_MANIFEST)
    if (messageName == IPC::MessageName::WebPageProxy_ApplicationManifestCallback)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPageProxy_AttributedStringForCharacterRangeCallback)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPageProxy_FontAtSelectionCallback)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_FontAttributesCallback)
        return true;
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_GestureCallback)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_TouchesCallback)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_SelectionContextCallback)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_InterpretKeyEvent)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_DidReceivePositionInformation)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_SaveImageToLibrary)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_ShowPlaybackTargetPicker)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_CommitPotentialTapFailed)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_DidNotHandleTapAsClick)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_DidCompleteSyntheticClick)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_DisableDoubleTapGesturesDuringTapIfNecessary)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_HandleSmartMagnificationInformationForPotentialTap)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_SelectionRectsCallback)
        return true;
#endif
#if ENABLE(DATA_DETECTION)
    if (messageName == IPC::MessageName::WebPageProxy_SetDataDetectionResult)
        return true;
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    if (messageName == IPC::MessageName::WebPageProxy_PrintFinishedCallback)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPageProxy_DrawToPDFCallback)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPageProxy_NowPlayingInfoCallback)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_FindStringCallback)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_PageScaleFactorDidChange)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_PluginScaleFactorDidChange)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_PluginZoomFactorDidChange)
        return true;
#if USE(ATK)
    if (messageName == IPC::MessageName::WebPageProxy_BindAccessibilityTree)
        return true;
#endif
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    if (messageName == IPC::MessageName::WebPageProxy_SetInputMethodState)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_BackForwardAddItem)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_BackForwardGoToItem)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_BackForwardItemAtIndex)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_BackForwardListCounts)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_BackForwardClear)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_WillGoToBackForwardListItem)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_RegisterEditCommandForUndo)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ClearAllEditCommands)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_RegisterInsertionUndoGrouping)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_CanUndoRedo)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ExecuteUndoRedo)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_LogDiagnosticMessage)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_LogDiagnosticMessageWithResult)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_LogDiagnosticMessageWithValue)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_LogDiagnosticMessageWithEnhancedPrivacy)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_LogDiagnosticMessageWithValueDictionary)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_LogScrollingEvent)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_EditorStateChanged)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_CompositionWasCanceled)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SetHasHadSelectionChangesFromUserInteraction)
        return true;
#if HAVE(TOUCH_BAR)
    if (messageName == IPC::MessageName::WebPageProxy_SetIsTouchBarUpdateSupressedForHiddenContentEditable)
        return true;
#endif
#if HAVE(TOUCH_BAR)
    if (messageName == IPC::MessageName::WebPageProxy_SetIsNeverRichlyEditableForTouchBar)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_RequestDOMPasteAccess)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidCountStringMatches)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SetTextIndicator)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ClearTextIndicator)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidFindString)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidFailToFindString)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidFindStringMatches)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidGetImageForFindMatch)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ShowPopupMenu)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_HidePopupMenu)
        return true;
#if ENABLE(CONTEXT_MENUS)
    if (messageName == IPC::MessageName::WebPageProxy_ShowContextMenu)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_ExceededDatabaseQuota)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ReachedApplicationCacheOriginQuota)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_RequestGeolocationPermissionForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_RevokeGeolocationAuthorizationToken)
        return true;
#if ENABLE(MEDIA_STREAM)
    if (messageName == IPC::MessageName::WebPageProxy_RequestUserMediaPermissionForFrame)
        return true;
#endif
#if ENABLE(MEDIA_STREAM)
    if (messageName == IPC::MessageName::WebPageProxy_EnumerateMediaDevicesForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_EnumerateMediaDevicesForFrameReply)
        return true;
#endif
#if ENABLE(MEDIA_STREAM)
    if (messageName == IPC::MessageName::WebPageProxy_BeginMonitoringCaptureDevices)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_RequestNotificationPermission)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ShowNotification)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_CancelNotification)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ClearNotifications)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidDestroyNotification)
        return true;
#if USE(UNIFIED_TEXT_CHECKING)
    if (messageName == IPC::MessageName::WebPageProxy_CheckTextOfParagraph)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_CheckSpellingOfString)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_CheckGrammarOfString)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SpellingUIIsShowing)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_UpdateSpellingUIWithMisspelledWord)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_UpdateSpellingUIWithGrammarString)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_GetGuessesForWord)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_LearnWord)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_IgnoreWord)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_RequestCheckingOfString)
        return true;
#if ENABLE(DRAG_SUPPORT)
    if (messageName == IPC::MessageName::WebPageProxy_DidPerformDragControllerAction)
        return true;
#endif
#if ENABLE(DRAG_SUPPORT)
    if (messageName == IPC::MessageName::WebPageProxy_DidEndDragging)
        return true;
#endif
#if PLATFORM(COCOA) && ENABLE(DRAG_SUPPORT)
    if (messageName == IPC::MessageName::WebPageProxy_StartDrag)
        return true;
#endif
#if PLATFORM(COCOA) && ENABLE(DRAG_SUPPORT)
    if (messageName == IPC::MessageName::WebPageProxy_SetPromisedDataForImage)
        return true;
#endif
#if (PLATFORM(GTK) || PLATFORM(HBD)) && ENABLE(DRAG_SUPPORT)
    if (messageName == IPC::MessageName::WebPageProxy_StartDrag)
        return true;
#endif
#if ENABLE(DRAG_SUPPORT)
    if (messageName == IPC::MessageName::WebPageProxy_DidPerformDragOperation)
        return true;
#endif
#if ENABLE(DATA_INTERACTION)
    if (messageName == IPC::MessageName::WebPageProxy_DidHandleDragStartRequest)
        return true;
#endif
#if ENABLE(DATA_INTERACTION)
    if (messageName == IPC::MessageName::WebPageProxy_DidHandleAdditionalDragItemsRequest)
        return true;
#endif
#if ENABLE(DATA_INTERACTION)
    if (messageName == IPC::MessageName::WebPageProxy_WillReceiveEditDragSnapshot)
        return true;
#endif
#if ENABLE(DATA_INTERACTION)
    if (messageName == IPC::MessageName::WebPageProxy_DidReceiveEditDragSnapshot)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPageProxy_DidPerformDictionaryLookup)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPageProxy_ExecuteSavedCommandBySelector)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPageProxy_RegisterWebProcessAccessibilityToken)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPageProxy_PluginFocusOrWindowFocusChanged)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPageProxy_SetPluginComplexTextInputState)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPageProxy_GetIsSpeaking)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPageProxy_Speak)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPageProxy_StopSpeaking)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPageProxy_MakeFirstResponder)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPageProxy_AssistiveTechnologyMakeFirstResponder)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPageProxy_SearchWithSpotlight)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPageProxy_SearchTheWeb)
        return true;
#endif
#if HAVE(TOUCH_BAR)
    if (messageName == IPC::MessageName::WebPageProxy_TouchBarMenuDataChanged)
        return true;
#endif
#if HAVE(TOUCH_BAR)
    if (messageName == IPC::MessageName::WebPageProxy_TouchBarMenuItemDataAdded)
        return true;
#endif
#if HAVE(TOUCH_BAR)
    if (messageName == IPC::MessageName::WebPageProxy_TouchBarMenuItemDataRemoved)
        return true;
#endif
#if USE(APPKIT)
    if (messageName == IPC::MessageName::WebPageProxy_SubstitutionsPanelIsShowing)
        return true;
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    if (messageName == IPC::MessageName::WebPageProxy_toggleSmartInsertDelete)
        return true;
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    if (messageName == IPC::MessageName::WebPageProxy_toggleAutomaticQuoteSubstitution)
        return true;
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    if (messageName == IPC::MessageName::WebPageProxy_toggleAutomaticLinkDetection)
        return true;
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    if (messageName == IPC::MessageName::WebPageProxy_toggleAutomaticDashSubstitution)
        return true;
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    if (messageName == IPC::MessageName::WebPageProxy_toggleAutomaticTextReplacement)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPageProxy_ShowCorrectionPanel)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPageProxy_DismissCorrectionPanel)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPageProxy_DismissCorrectionPanelSoon)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPageProxy_RecordAutocorrectionResponse)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPageProxy_SetEditableElementIsFocused)
        return true;
#endif
#if USE(DICTATION_ALTERNATIVES)
    if (messageName == IPC::MessageName::WebPageProxy_ShowDictationAlternativeUI)
        return true;
#endif
#if USE(DICTATION_ALTERNATIVES)
    if (messageName == IPC::MessageName::WebPageProxy_RemoveDictationAlternatives)
        return true;
#endif
#if USE(DICTATION_ALTERNATIVES)
    if (messageName == IPC::MessageName::WebPageProxy_DictationAlternatives)
        return true;
#endif
#if PLATFORM(X11) && ENABLE(NETSCAPE_PLUGIN_API)
    if (messageName == IPC::MessageName::WebPageProxy_CreatePluginContainer)
        return true;
#endif
#if PLATFORM(X11) && ENABLE(NETSCAPE_PLUGIN_API)
    if (messageName == IPC::MessageName::WebPageProxy_WindowedPluginGeometryDidChange)
        return true;
#endif
#if PLATFORM(X11) && ENABLE(NETSCAPE_PLUGIN_API)
    if (messageName == IPC::MessageName::WebPageProxy_WindowedPluginVisibilityDidChange)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_CouldNotRestorePageState)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_RestorePageState)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_RestorePageCenterAndScale)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_DidGetTapHighlightGeometries)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_ElementDidFocus)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_ElementDidBlur)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_UpdateInputContextAfterBlurringAndRefocusingElement)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_FocusedElementDidChangeInputMode)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_ScrollingNodeScrollWillStartScroll)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_ScrollingNodeScrollDidEndScroll)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_ShowInspectorHighlight)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_HideInspectorHighlight)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_FocusedElementInformationCallback)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_ShowInspectorIndication)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_HideInspectorIndication)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_EnableInspectorNodeSearch)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_DisableInspectorNodeSearch)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_UpdateStringForFind)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_HandleAutocorrectionContext)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_ShowDataDetectorsUIForPositionInformation)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_DidChangeInspectorFrontendCount)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_CreateInspectorTarget)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DestroyInspectorTarget)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SendMessageToInspectorFrontend)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SaveRecentSearches)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_LoadRecentSearches)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SavePDFToFileInDownloadsFolder)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPageProxy_SavePDFToTemporaryFolderAndOpenWithNativeApplication)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPageProxy_OpenPDFFromTemporaryFolderWithNativeApplication)
        return true;
#endif
#if ENABLE(PDFKIT_PLUGIN)
    if (messageName == IPC::MessageName::WebPageProxy_ShowPDFContextMenu)
        return true;
#endif
#if ENABLE(NETSCAPE_PLUGIN_API)
    if (messageName == IPC::MessageName::WebPageProxy_FindPlugin)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_DidUpdateActivityState)
        return true;
#if ENABLE(WEB_CRYPTO)
    if (messageName == IPC::MessageName::WebPageProxy_WrapCryptoKey)
        return true;
#endif
#if ENABLE(WEB_CRYPTO)
    if (messageName == IPC::MessageName::WebPageProxy_UnwrapCryptoKey)
        return true;
#endif
#if (ENABLE(TELEPHONE_NUMBER_DETECTION) && PLATFORM(MAC))
    if (messageName == IPC::MessageName::WebPageProxy_ShowTelephoneNumberMenu)
        return true;
#endif
#if USE(QUICK_LOOK)
    if (messageName == IPC::MessageName::WebPageProxy_DidStartLoadForQuickLookDocumentInMainFrame)
        return true;
#endif
#if USE(QUICK_LOOK)
    if (messageName == IPC::MessageName::WebPageProxy_DidFinishLoadForQuickLookDocumentInMainFrame)
        return true;
#endif
#if USE(QUICK_LOOK)
    if (messageName == IPC::MessageName::WebPageProxy_RequestPasswordForQuickLookDocumentInMainFrame)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_RequestPasswordForQuickLookDocumentInMainFrameReply)
        return true;
#endif
#if ENABLE(CONTENT_FILTERING)
    if (messageName == IPC::MessageName::WebPageProxy_ContentFilterDidBlockLoadForFrame)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_IsPlayingMediaDidChange)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_HandleAutoplayEvent)
        return true;
#if ENABLE(MEDIA_SESSION)
    if (messageName == IPC::MessageName::WebPageProxy_HasMediaSessionWithActiveMediaElementsDidChange)
        return true;
#endif
#if ENABLE(MEDIA_SESSION)
    if (messageName == IPC::MessageName::WebPageProxy_MediaSessionMetadataDidChange)
        return true;
#endif
#if ENABLE(MEDIA_SESSION)
    if (messageName == IPC::MessageName::WebPageProxy_FocusedContentMediaElementDidChange)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPageProxy_DidPerformImmediateActionHitTest)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_HandleMessage)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_HandleSynchronousMessage)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_HandleAutoFillButtonClick)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidResignInputElementStrongPasswordAppearance)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ContentRuleListNotification)
        return true;
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_AddPlaybackTargetPickerClient)
        return true;
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_RemovePlaybackTargetPickerClient)
        return true;
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_ShowPlaybackTargetPicker)
        return true;
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_PlaybackTargetPickerClientStateDidChange)
        return true;
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_SetMockMediaPlaybackTargetPickerEnabled)
        return true;
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_SetMockMediaPlaybackTargetPickerState)
        return true;
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPageProxy_MockMediaPlaybackTargetPickerDismissPopup)
        return true;
#endif
#if ENABLE(VIDEO_PRESENTATION_MODE)
    if (messageName == IPC::MessageName::WebPageProxy_SetMockVideoPresentationModeEnabled)
        return true;
#endif
#if ENABLE(POINTER_LOCK)
    if (messageName == IPC::MessageName::WebPageProxy_RequestPointerLock)
        return true;
#endif
#if ENABLE(POINTER_LOCK)
    if (messageName == IPC::MessageName::WebPageProxy_RequestPointerUnlock)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_DidFailToSuspendAfterProcessSwap)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_DidSuspendAfterProcessSwap)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ImageOrMediaDocumentSizeChanged)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_UseFixedLayoutDidChange)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_FixedLayoutSizeDidChange)
        return true;
#if ENABLE(VIDEO) && USE(GSTREAMER)
    if (messageName == IPC::MessageName::WebPageProxy_RequestInstallMissingMediaPlugins)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_DidRestoreScrollPosition)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_GetLoadDecisionForIcon)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_FinishedLoadingIcon)
        return true;
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPageProxy_DidHandleAcceptedCandidate)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_SetIsUsingHighPerformanceWebGL)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_StartURLSchemeTask)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_StopURLSchemeTask)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_LoadSynchronousURLSchemeTask)
        return true;
#if ENABLE(DEVICE_ORIENTATION)
    if (messageName == IPC::MessageName::WebPageProxy_ShouldAllowDeviceOrientationAndMotionAccess)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ShouldAllowDeviceOrientationAndMotionAccessReply)
        return true;
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    if (messageName == IPC::MessageName::WebPageProxy_RegisterAttachmentIdentifierFromData)
        return true;
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    if (messageName == IPC::MessageName::WebPageProxy_RegisterAttachmentIdentifierFromFilePath)
        return true;
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    if (messageName == IPC::MessageName::WebPageProxy_RegisterAttachmentIdentifier)
        return true;
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    if (messageName == IPC::MessageName::WebPageProxy_RegisterAttachmentsFromSerializedData)
        return true;
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    if (messageName == IPC::MessageName::WebPageProxy_CloneAttachmentData)
        return true;
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    if (messageName == IPC::MessageName::WebPageProxy_DidInsertAttachmentWithIdentifier)
        return true;
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    if (messageName == IPC::MessageName::WebPageProxy_DidRemoveAttachmentWithIdentifier)
        return true;
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    if (messageName == IPC::MessageName::WebPageProxy_SerializedAttachmentDataForIdentifiers)
        return true;
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    if (messageName == IPC::MessageName::WebPageProxy_WritePromisedAttachmentToPasteboard)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_SignedPublicKeyAndChallengeString)
        return true;
#if ENABLE(SPEECH_SYNTHESIS)
    if (messageName == IPC::MessageName::WebPageProxy_SpeechSynthesisVoiceList)
        return true;
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    if (messageName == IPC::MessageName::WebPageProxy_SpeechSynthesisSpeak)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SpeechSynthesisSpeakReply)
        return true;
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    if (messageName == IPC::MessageName::WebPageProxy_SpeechSynthesisSetFinishedCallback)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SpeechSynthesisSetFinishedCallbackReply)
        return true;
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    if (messageName == IPC::MessageName::WebPageProxy_SpeechSynthesisCancel)
        return true;
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    if (messageName == IPC::MessageName::WebPageProxy_SpeechSynthesisPause)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SpeechSynthesisPauseReply)
        return true;
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    if (messageName == IPC::MessageName::WebPageProxy_SpeechSynthesisResume)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SpeechSynthesisResumeReply)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_ConfigureLoggingChannel)
        return true;
#if PLATFORM(GTK) || PLATFORM(HBD)
    if (messageName == IPC::MessageName::WebPageProxy_ShowEmojiPicker)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_ShowEmojiPickerReply)
        return true;
#endif
#if HAVE(VISIBILITY_PROPAGATION_VIEW)
    if (messageName == IPC::MessageName::WebPageProxy_DidCreateContextForVisibilityPropagation)
        return true;
#endif
#if ENABLE(WEB_AUTHN)
    if (messageName == IPC::MessageName::WebPageProxy_SetMockWebAuthenticationConfiguration)
        return true;
#endif
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    if (messageName == IPC::MessageName::WebPageProxy_SendMessageToWebView)
        return true;
#endif
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    if (messageName == IPC::MessageName::WebPageProxy_SendMessageToWebViewWithReply)
        return true;
    if (messageName == IPC::MessageName::WebPageProxy_SendMessageToWebViewWithReplyReply)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_DidFindTextManipulationItems)
        return true;
#if ENABLE(MEDIA_USAGE)
    if (messageName == IPC::MessageName::WebPageProxy_AddMediaUsageManagerSession)
        return true;
#endif
#if ENABLE(MEDIA_USAGE)
    if (messageName == IPC::MessageName::WebPageProxy_UpdateMediaUsageManagerSessionState)
        return true;
#endif
#if ENABLE(MEDIA_USAGE)
    if (messageName == IPC::MessageName::WebPageProxy_RemoveMediaUsageManagerSession)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPageProxy_SetHasExecutedAppBoundBehaviorBeforeNavigation)
        return true;
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPasteboardProxy_WriteURLToPasteboard)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPasteboardProxy_WriteWebContentToPasteboard)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPasteboardProxy_WriteImageToPasteboard)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPasteboardProxy_WriteStringToPasteboard)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPasteboardProxy_UpdateSupportedTypeIdentifiers)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPasteboardProxy_WriteCustomData)
        return true;
    if (messageName == IPC::MessageName::WebPasteboardProxy_TypesSafeForDOMToReadAndWrite)
        return true;
    if (messageName == IPC::MessageName::WebPasteboardProxy_AllPasteboardItemInfo)
        return true;
    if (messageName == IPC::MessageName::WebPasteboardProxy_InformationForItemAtIndex)
        return true;
    if (messageName == IPC::MessageName::WebPasteboardProxy_GetPasteboardItemsCount)
        return true;
    if (messageName == IPC::MessageName::WebPasteboardProxy_ReadStringFromPasteboard)
        return true;
    if (messageName == IPC::MessageName::WebPasteboardProxy_ReadURLFromPasteboard)
        return true;
    if (messageName == IPC::MessageName::WebPasteboardProxy_ReadBufferFromPasteboard)
        return true;
    if (messageName == IPC::MessageName::WebPasteboardProxy_ContainsStringSafeForDOMToReadForType)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPasteboardProxy_GetNumberOfFiles)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPasteboardProxy_GetPasteboardTypes)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPasteboardProxy_GetPasteboardPathnamesForType)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPasteboardProxy_GetPasteboardStringForType)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPasteboardProxy_GetPasteboardStringsForType)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPasteboardProxy_GetPasteboardBufferForType)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPasteboardProxy_GetPasteboardChangeCount)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPasteboardProxy_GetPasteboardColor)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPasteboardProxy_GetPasteboardURL)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPasteboardProxy_AddPasteboardTypes)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPasteboardProxy_SetPasteboardTypes)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPasteboardProxy_SetPasteboardURL)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPasteboardProxy_SetPasteboardColor)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPasteboardProxy_SetPasteboardStringForType)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPasteboardProxy_SetPasteboardBufferForType)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPasteboardProxy_ContainsURLStringSuitableForLoading)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPasteboardProxy_URLStringSuitableForLoading)
        return true;
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    if (messageName == IPC::MessageName::WebPasteboardProxy_GetTypes)
        return true;
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    if (messageName == IPC::MessageName::WebPasteboardProxy_ReadText)
        return true;
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    if (messageName == IPC::MessageName::WebPasteboardProxy_ReadFilePaths)
        return true;
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    if (messageName == IPC::MessageName::WebPasteboardProxy_ReadBuffer)
        return true;
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    if (messageName == IPC::MessageName::WebPasteboardProxy_WriteToClipboard)
        return true;
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    if (messageName == IPC::MessageName::WebPasteboardProxy_ClearClipboard)
        return true;
#endif
#if USE(LIBWPE)
    if (messageName == IPC::MessageName::WebPasteboardProxy_GetPasteboardTypes)
        return true;
#endif
#if USE(LIBWPE)
    if (messageName == IPC::MessageName::WebPasteboardProxy_WriteWebContentToPasteboard)
        return true;
#endif
#if USE(LIBWPE)
    if (messageName == IPC::MessageName::WebPasteboardProxy_WriteStringToPasteboard)
        return true;
#endif
    if (messageName == IPC::MessageName::WebProcessPool_HandleMessage)
        return true;
    if (messageName == IPC::MessageName::WebProcessPool_HandleSynchronousMessage)
        return true;
#if ENABLE(GAMEPAD)
    if (messageName == IPC::MessageName::WebProcessPool_StartedUsingGamepads)
        return true;
#endif
#if ENABLE(GAMEPAD)
    if (messageName == IPC::MessageName::WebProcessPool_StoppedUsingGamepads)
        return true;
#endif
    if (messageName == IPC::MessageName::WebProcessPool_ReportWebContentCPUTime)
        return true;
    if (messageName == IPC::MessageName::WebProcessProxy_UpdateBackForwardItem)
        return true;
    if (messageName == IPC::MessageName::WebProcessProxy_DidDestroyFrame)
        return true;
    if (messageName == IPC::MessageName::WebProcessProxy_DidDestroyUserGestureToken)
        return true;
    if (messageName == IPC::MessageName::WebProcessProxy_ShouldTerminate)
        return true;
    if (messageName == IPC::MessageName::WebProcessProxy_EnableSuddenTermination)
        return true;
    if (messageName == IPC::MessageName::WebProcessProxy_DisableSuddenTermination)
        return true;
#if ENABLE(NETSCAPE_PLUGIN_API)
    if (messageName == IPC::MessageName::WebProcessProxy_GetPlugins)
        return true;
#endif
#if ENABLE(NETSCAPE_PLUGIN_API)
    if (messageName == IPC::MessageName::WebProcessProxy_GetPluginProcessConnection)
        return true;
#endif
    if (messageName == IPC::MessageName::WebProcessProxy_GetNetworkProcessConnection)
        return true;
#if ENABLE(GPU_PROCESS)
    if (messageName == IPC::MessageName::WebProcessProxy_GetGPUProcessConnection)
        return true;
#endif
    if (messageName == IPC::MessageName::WebProcessProxy_SetIsHoldingLockedFiles)
        return true;
    if (messageName == IPC::MessageName::WebProcessProxy_DidExceedActiveMemoryLimit)
        return true;
    if (messageName == IPC::MessageName::WebProcessProxy_DidExceedInactiveMemoryLimit)
        return true;
    if (messageName == IPC::MessageName::WebProcessProxy_DidExceedCPULimit)
        return true;
    if (messageName == IPC::MessageName::WebProcessProxy_StopResponsivenessTimer)
        return true;
    if (messageName == IPC::MessageName::WebProcessProxy_DidReceiveMainThreadPing)
        return true;
    if (messageName == IPC::MessageName::WebProcessProxy_DidReceiveBackgroundResponsivenessPing)
        return true;
    if (messageName == IPC::MessageName::WebProcessProxy_MemoryPressureStatusChanged)
        return true;
    if (messageName == IPC::MessageName::WebProcessProxy_DidExceedInactiveMemoryLimitWhileActive)
        return true;
    if (messageName == IPC::MessageName::WebProcessProxy_DidCollectPrewarmInformation)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebProcessProxy_CacheMediaMIMETypes)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebProcessProxy_RequestHighPerformanceGPU)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebProcessProxy_ReleaseHighPerformanceGPU)
        return true;
#endif
#if PLATFORM(MAC) && ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    if (messageName == IPC::MessageName::WebProcessProxy_StartDisplayLink)
        return true;
#endif
#if PLATFORM(MAC) && ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    if (messageName == IPC::MessageName::WebProcessProxy_StopDisplayLink)
        return true;
#endif
    if (messageName == IPC::MessageName::WebProcessProxy_AddPlugInAutoStartOriginHash)
        return true;
    if (messageName == IPC::MessageName::WebProcessProxy_PlugInDidReceiveUserInteraction)
        return true;
#if PLATFORM(GTK) || PLATFORM(WPE)
    if (messageName == IPC::MessageName::WebProcessProxy_SendMessageToWebContext)
        return true;
#endif
#if PLATFORM(GTK) || PLATFORM(WPE)
    if (messageName == IPC::MessageName::WebProcessProxy_SendMessageToWebContextWithReply)
        return true;
    if (messageName == IPC::MessageName::WebProcessProxy_SendMessageToWebContextWithReplyReply)
        return true;
#endif
    if (messageName == IPC::MessageName::WebProcessProxy_DidCreateSleepDisabler)
        return true;
    if (messageName == IPC::MessageName::WebProcessProxy_DidDestroySleepDisabler)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSession_DidEvaluateJavaScriptFunction)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSession_DidTakeScreenshot)
        return true;
    if (messageName == IPC::MessageName::DownloadProxy_DidStart)
        return true;
    if (messageName == IPC::MessageName::DownloadProxy_DidReceiveAuthenticationChallenge)
        return true;
    if (messageName == IPC::MessageName::DownloadProxy_WillSendRequest)
        return true;
    if (messageName == IPC::MessageName::DownloadProxy_DecideDestinationWithSuggestedFilenameAsync)
        return true;
    if (messageName == IPC::MessageName::DownloadProxy_DidReceiveResponse)
        return true;
    if (messageName == IPC::MessageName::DownloadProxy_DidReceiveData)
        return true;
    if (messageName == IPC::MessageName::DownloadProxy_DidCreateDestination)
        return true;
    if (messageName == IPC::MessageName::DownloadProxy_DidFinish)
        return true;
    if (messageName == IPC::MessageName::DownloadProxy_DidFail)
        return true;
    if (messageName == IPC::MessageName::DownloadProxy_DidCancel)
        return true;
#if HAVE(VISIBILITY_PROPAGATION_VIEW)
    if (messageName == IPC::MessageName::GPUProcessProxy_DidCreateContextForVisibilityPropagation)
        return true;
#endif
    if (messageName == IPC::MessageName::RemoteWebInspectorProxy_FrontendDidClose)
        return true;
    if (messageName == IPC::MessageName::RemoteWebInspectorProxy_Reopen)
        return true;
    if (messageName == IPC::MessageName::RemoteWebInspectorProxy_ResetState)
        return true;
    if (messageName == IPC::MessageName::RemoteWebInspectorProxy_BringToFront)
        return true;
    if (messageName == IPC::MessageName::RemoteWebInspectorProxy_Save)
        return true;
    if (messageName == IPC::MessageName::RemoteWebInspectorProxy_Append)
        return true;
    if (messageName == IPC::MessageName::RemoteWebInspectorProxy_SetForcedAppearance)
        return true;
    if (messageName == IPC::MessageName::RemoteWebInspectorProxy_SetSheetRect)
        return true;
    if (messageName == IPC::MessageName::RemoteWebInspectorProxy_StartWindowDrag)
        return true;
    if (messageName == IPC::MessageName::RemoteWebInspectorProxy_OpenInNewTab)
        return true;
    if (messageName == IPC::MessageName::RemoteWebInspectorProxy_ShowCertificate)
        return true;
    if (messageName == IPC::MessageName::RemoteWebInspectorProxy_SendMessageToBackend)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_OpenLocalInspectorFrontend)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_SetFrontendConnection)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_SendMessageToBackend)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_FrontendLoaded)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_DidClose)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_BringToFront)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_BringInspectedPageToFront)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_Reopen)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_ResetState)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_SetForcedAppearance)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_InspectedURLChanged)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_ShowCertificate)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_ElementSelectionChanged)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_TimelineRecordingChanged)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_SetDeveloperPreferenceOverride)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_Save)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_Append)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_AttachBottom)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_AttachRight)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_AttachLeft)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_Detach)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_AttachAvailabilityChanged)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_SetAttachedWindowHeight)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_SetAttachedWindowWidth)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_SetSheetRect)
        return true;
    if (messageName == IPC::MessageName::WebInspectorProxy_StartWindowDrag)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_DidReceiveAuthenticationChallenge)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_NegotiatedLegacyTLS)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_DidNegotiateModernTLS)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_DidFetchWebsiteData)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_DidDeleteWebsiteData)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_DidDeleteWebsiteDataForOrigins)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_DidSyncAllCookies)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_TestProcessIncomingSyncMessagesWhenWaitingForSyncReply)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_TerminateUnresponsiveServiceWorkerProcesses)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_SetIsHoldingLockedFiles)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_LogDiagnosticMessage)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_LogDiagnosticMessageWithResult)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_LogDiagnosticMessageWithValue)
        return true;
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcessProxy_LogTestingEvent)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcessProxy_NotifyResourceLoadStatisticsProcessed)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcessProxy_NotifyWebsiteDataDeletionForRegistrableDomainsFinished)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcessProxy_NotifyWebsiteDataScanForRegistrableDomainsFinished)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcessProxy_NotifyResourceLoadStatisticsTelemetryFinished)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcessProxy_RequestStorageAccessConfirm)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_RequestStorageAccessConfirmReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcessProxy_DeleteWebsiteDataInUIProcessForRegistrableDomains)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_DeleteWebsiteDataInUIProcessForRegistrableDomainsReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcessProxy_DidCommitCrossSiteLoadWithDataTransferFromPrevalentResource)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::NetworkProcessProxy_SetDomainsWithUserInteraction)
        return true;
#endif
#if ENABLE(CONTENT_EXTENSIONS)
    if (messageName == IPC::MessageName::NetworkProcessProxy_ContentExtensionRules)
        return true;
#endif
    if (messageName == IPC::MessageName::NetworkProcessProxy_RetrieveCacheStorageParameters)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_TerminateWebProcess)
        return true;
#if ENABLE(SERVICE_WORKER)
    if (messageName == IPC::MessageName::NetworkProcessProxy_EstablishWorkerContextConnectionToNetworkProcess)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_EstablishWorkerContextConnectionToNetworkProcessReply)
        return true;
#endif
#if ENABLE(SERVICE_WORKER)
    if (messageName == IPC::MessageName::NetworkProcessProxy_WorkerContextConnectionNoLongerNeeded)
        return true;
#endif
#if ENABLE(SERVICE_WORKER)
    if (messageName == IPC::MessageName::NetworkProcessProxy_RegisterServiceWorkerClientProcess)
        return true;
#endif
#if ENABLE(SERVICE_WORKER)
    if (messageName == IPC::MessageName::NetworkProcessProxy_UnregisterServiceWorkerClientProcess)
        return true;
#endif
    if (messageName == IPC::MessageName::NetworkProcessProxy_SetWebProcessHasUploads)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_GetAppBoundDomains)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_GetAppBoundDomainsReply)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_RequestStorageSpace)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_RequestStorageSpaceReply)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_ResourceLoadDidSendRequest)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_ResourceLoadDidPerformHTTPRedirection)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_ResourceLoadDidReceiveChallenge)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_ResourceLoadDidReceiveResponse)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessProxy_ResourceLoadDidCompleteWithError)
        return true;
    if (messageName == IPC::MessageName::PluginProcessProxy_DidCreateWebProcessConnection)
        return true;
    if (messageName == IPC::MessageName::PluginProcessProxy_DidGetSitesWithData)
        return true;
    if (messageName == IPC::MessageName::PluginProcessProxy_DidDeleteWebsiteData)
        return true;
    if (messageName == IPC::MessageName::PluginProcessProxy_DidDeleteWebsiteDataForHostNames)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::PluginProcessProxy_SetModalWindowIsShowing)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::PluginProcessProxy_SetFullscreenWindowIsShowing)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::PluginProcessProxy_LaunchProcess)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::PluginProcessProxy_LaunchApplicationAtURL)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::PluginProcessProxy_OpenURL)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::PluginProcessProxy_OpenFile)
        return true;
#endif
    if (messageName == IPC::MessageName::WebUserContentControllerProxy_DidPostMessage)
        return true;
    if (messageName == IPC::MessageName::WebUserContentControllerProxy_DidPostMessageReply)
        return true;
    if (messageName == IPC::MessageName::WebProcess_InitializeWebProcess)
        return true;
    if (messageName == IPC::MessageName::WebProcess_SetWebsiteDataStoreParameters)
        return true;
    if (messageName == IPC::MessageName::WebProcess_CreateWebPage)
        return true;
    if (messageName == IPC::MessageName::WebProcess_PrewarmGlobally)
        return true;
    if (messageName == IPC::MessageName::WebProcess_PrewarmWithDomainInformation)
        return true;
    if (messageName == IPC::MessageName::WebProcess_SetCacheModel)
        return true;
    if (messageName == IPC::MessageName::WebProcess_RegisterURLSchemeAsEmptyDocument)
        return true;
    if (messageName == IPC::MessageName::WebProcess_RegisterURLSchemeAsSecure)
        return true;
    if (messageName == IPC::MessageName::WebProcess_RegisterURLSchemeAsBypassingContentSecurityPolicy)
        return true;
    if (messageName == IPC::MessageName::WebProcess_SetDomainRelaxationForbiddenForURLScheme)
        return true;
    if (messageName == IPC::MessageName::WebProcess_RegisterURLSchemeAsLocal)
        return true;
    if (messageName == IPC::MessageName::WebProcess_RegisterURLSchemeAsNoAccess)
        return true;
    if (messageName == IPC::MessageName::WebProcess_RegisterURLSchemeAsDisplayIsolated)
        return true;
    if (messageName == IPC::MessageName::WebProcess_RegisterURLSchemeAsCORSEnabled)
        return true;
    if (messageName == IPC::MessageName::WebProcess_RegisterURLSchemeAsCachePartitioned)
        return true;
    if (messageName == IPC::MessageName::WebProcess_RegisterURLSchemeAsCanDisplayOnlyIfCanRequest)
        return true;
    if (messageName == IPC::MessageName::WebProcess_SetDefaultRequestTimeoutInterval)
        return true;
    if (messageName == IPC::MessageName::WebProcess_SetAlwaysUsesComplexTextCodePath)
        return true;
    if (messageName == IPC::MessageName::WebProcess_SetShouldUseFontSmoothing)
        return true;
    if (messageName == IPC::MessageName::WebProcess_SetResourceLoadStatisticsEnabled)
        return true;
    if (messageName == IPC::MessageName::WebProcess_ClearResourceLoadStatistics)
        return true;
    if (messageName == IPC::MessageName::WebProcess_UserPreferredLanguagesChanged)
        return true;
    if (messageName == IPC::MessageName::WebProcess_FullKeyboardAccessModeChanged)
        return true;
    if (messageName == IPC::MessageName::WebProcess_DidAddPlugInAutoStartOriginHash)
        return true;
    if (messageName == IPC::MessageName::WebProcess_ResetPlugInAutoStartOriginHashes)
        return true;
    if (messageName == IPC::MessageName::WebProcess_SetPluginLoadClientPolicy)
        return true;
    if (messageName == IPC::MessageName::WebProcess_ResetPluginLoadClientPolicies)
        return true;
    if (messageName == IPC::MessageName::WebProcess_ClearPluginClientPolicies)
        return true;
    if (messageName == IPC::MessageName::WebProcess_RefreshPlugins)
        return true;
    if (messageName == IPC::MessageName::WebProcess_StartMemorySampler)
        return true;
    if (messageName == IPC::MessageName::WebProcess_StopMemorySampler)
        return true;
    if (messageName == IPC::MessageName::WebProcess_SetTextCheckerState)
        return true;
    if (messageName == IPC::MessageName::WebProcess_SetEnhancedAccessibility)
        return true;
    if (messageName == IPC::MessageName::WebProcess_GarbageCollectJavaScriptObjects)
        return true;
    if (messageName == IPC::MessageName::WebProcess_SetJavaScriptGarbageCollectorTimerEnabled)
        return true;
    if (messageName == IPC::MessageName::WebProcess_SetInjectedBundleParameter)
        return true;
    if (messageName == IPC::MessageName::WebProcess_SetInjectedBundleParameters)
        return true;
    if (messageName == IPC::MessageName::WebProcess_HandleInjectedBundleMessage)
        return true;
    if (messageName == IPC::MessageName::WebProcess_FetchWebsiteData)
        return true;
    if (messageName == IPC::MessageName::WebProcess_FetchWebsiteDataReply)
        return true;
    if (messageName == IPC::MessageName::WebProcess_DeleteWebsiteData)
        return true;
    if (messageName == IPC::MessageName::WebProcess_DeleteWebsiteDataReply)
        return true;
    if (messageName == IPC::MessageName::WebProcess_DeleteWebsiteDataForOrigins)
        return true;
    if (messageName == IPC::MessageName::WebProcess_DeleteWebsiteDataForOriginsReply)
        return true;
    if (messageName == IPC::MessageName::WebProcess_SetHiddenPageDOMTimerThrottlingIncreaseLimit)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebProcess_SetQOS)
        return true;
#endif
    if (messageName == IPC::MessageName::WebProcess_SetMemoryCacheDisabled)
        return true;
#if ENABLE(SERVICE_CONTROLS)
    if (messageName == IPC::MessageName::WebProcess_SetEnabledServices)
        return true;
#endif
    if (messageName == IPC::MessageName::WebProcess_EnsureAutomationSessionProxy)
        return true;
    if (messageName == IPC::MessageName::WebProcess_DestroyAutomationSessionProxy)
        return true;
    if (messageName == IPC::MessageName::WebProcess_PrepareToSuspend)
        return true;
    if (messageName == IPC::MessageName::WebProcess_PrepareToSuspendReply)
        return true;
    if (messageName == IPC::MessageName::WebProcess_ProcessDidResume)
        return true;
    if (messageName == IPC::MessageName::WebProcess_MainThreadPing)
        return true;
    if (messageName == IPC::MessageName::WebProcess_BackgroundResponsivenessPing)
        return true;
#if ENABLE(GAMEPAD)
    if (messageName == IPC::MessageName::WebProcess_SetInitialGamepads)
        return true;
#endif
#if ENABLE(GAMEPAD)
    if (messageName == IPC::MessageName::WebProcess_GamepadConnected)
        return true;
#endif
#if ENABLE(GAMEPAD)
    if (messageName == IPC::MessageName::WebProcess_GamepadDisconnected)
        return true;
#endif
#if ENABLE(SERVICE_WORKER)
    if (messageName == IPC::MessageName::WebProcess_EstablishWorkerContextConnectionToNetworkProcess)
        return true;
    if (messageName == IPC::MessageName::WebProcess_EstablishWorkerContextConnectionToNetworkProcessReply)
        return true;
#endif
    if (messageName == IPC::MessageName::WebProcess_SetHasSuspendedPageProxy)
        return true;
    if (messageName == IPC::MessageName::WebProcess_SetIsInProcessCache)
        return true;
    if (messageName == IPC::MessageName::WebProcess_MarkIsNoLongerPrewarmed)
        return true;
    if (messageName == IPC::MessageName::WebProcess_GetActivePagesOriginsForTesting)
        return true;
    if (messageName == IPC::MessageName::WebProcess_GetActivePagesOriginsForTestingReply)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebProcess_SetScreenProperties)
        return true;
#endif
#if PLATFORM(MAC) && ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    if (messageName == IPC::MessageName::WebProcess_ScrollerStylePreferenceChanged)
        return true;
#endif
#if PLATFORM(MAC) && ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    if (messageName == IPC::MessageName::WebProcess_DisplayConfigurationChanged)
        return true;
#endif
#if PLATFORM(IOS_FAMILY) && !PLATFORM(MACCATALYST)
    if (messageName == IPC::MessageName::WebProcess_BacklightLevelDidChange)
        return true;
#endif
    if (messageName == IPC::MessageName::WebProcess_IsJITEnabled)
        return true;
    if (messageName == IPC::MessageName::WebProcess_IsJITEnabledReply)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebProcess_SetMediaMIMETypes)
        return true;
#endif
#if (PLATFORM(COCOA) && ENABLE(REMOTE_INSPECTOR))
    if (messageName == IPC::MessageName::WebProcess_EnableRemoteWebInspector)
        return true;
#endif
#if ENABLE(MEDIA_STREAM)
    if (messageName == IPC::MessageName::WebProcess_AddMockMediaDevice)
        return true;
#endif
#if ENABLE(MEDIA_STREAM)
    if (messageName == IPC::MessageName::WebProcess_ClearMockMediaDevices)
        return true;
#endif
#if ENABLE(MEDIA_STREAM)
    if (messageName == IPC::MessageName::WebProcess_RemoveMockMediaDevice)
        return true;
#endif
#if ENABLE(MEDIA_STREAM)
    if (messageName == IPC::MessageName::WebProcess_ResetMockMediaDevices)
        return true;
#endif
#if (ENABLE(MEDIA_STREAM) && ENABLE(SANDBOX_EXTENSIONS))
    if (messageName == IPC::MessageName::WebProcess_GrantUserMediaDeviceSandboxExtensions)
        return true;
#endif
#if (ENABLE(MEDIA_STREAM) && ENABLE(SANDBOX_EXTENSIONS))
    if (messageName == IPC::MessageName::WebProcess_RevokeUserMediaDeviceSandboxExtensions)
        return true;
#endif
    if (messageName == IPC::MessageName::WebProcess_ClearCurrentModifierStateForTesting)
        return true;
    if (messageName == IPC::MessageName::WebProcess_SetBackForwardCacheCapacity)
        return true;
    if (messageName == IPC::MessageName::WebProcess_ClearCachedPage)
        return true;
    if (messageName == IPC::MessageName::WebProcess_ClearCachedPageReply)
        return true;
#if PLATFORM(GTK) || PLATFORM(WPE)
    if (messageName == IPC::MessageName::WebProcess_SendMessageToWebExtension)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::WebProcess_SeedResourceLoadStatisticsForTesting)
        return true;
    if (messageName == IPC::MessageName::WebProcess_SeedResourceLoadStatisticsForTestingReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::WebProcess_SetThirdPartyCookieBlockingMode)
        return true;
    if (messageName == IPC::MessageName::WebProcess_SetThirdPartyCookieBlockingModeReply)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::WebProcess_SetDomainsWithUserInteraction)
        return true;
#endif
#if PLATFORM(IOS)
    if (messageName == IPC::MessageName::WebProcess_GrantAccessToAssetServices)
        return true;
#endif
#if PLATFORM(IOS)
    if (messageName == IPC::MessageName::WebProcess_RevokeAccessToAssetServices)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebProcess_UnblockServicesRequiredByAccessibility)
        return true;
#endif
#if (PLATFORM(COCOA) && ENABLE(CFPREFS_DIRECT_MODE))
    if (messageName == IPC::MessageName::WebProcess_NotifyPreferencesChanged)
        return true;
#endif
#if (PLATFORM(COCOA) && ENABLE(CFPREFS_DIRECT_MODE))
    if (messageName == IPC::MessageName::WebProcess_UnblockPreferenceService)
        return true;
#endif
#if PLATFORM(GTK) && !USE(GTK4)
    if (messageName == IPC::MessageName::WebProcess_SetUseSystemAppearanceForScrollbars)
        return true;
#endif
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_EvaluateJavaScriptFunction)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_ResolveChildFrameWithOrdinal)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_ResolveChildFrameWithOrdinalReply)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_ResolveChildFrameWithNodeHandle)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_ResolveChildFrameWithNodeHandleReply)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_ResolveChildFrameWithName)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_ResolveChildFrameWithNameReply)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_ResolveParentFrame)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_ResolveParentFrameReply)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_FocusFrame)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_ComputeElementLayout)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_ComputeElementLayoutReply)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_SelectOptionElement)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_SelectOptionElementReply)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_SetFilesForInputFileUpload)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_SetFilesForInputFileUploadReply)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_TakeScreenshot)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_SnapshotRectForScreenshot)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_SnapshotRectForScreenshotReply)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_GetCookiesForFrame)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_GetCookiesForFrameReply)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_DeleteCookie)
        return true;
    if (messageName == IPC::MessageName::WebAutomationSessionProxy_DeleteCookieReply)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidDeleteDatabase)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidOpenDatabase)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidAbortTransaction)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidCommitTransaction)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidCreateObjectStore)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidDeleteObjectStore)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidRenameObjectStore)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidClearObjectStore)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidCreateIndex)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidDeleteIndex)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidRenameIndex)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidPutOrAdd)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidGetRecord)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidGetAllRecords)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidGetCount)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidDeleteRecord)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidOpenCursor)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidIterateCursor)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_FireVersionChangeEvent)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidStartTransaction)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidCloseFromServer)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_NotifyOpenDBRequestBlocked)
        return true;
    if (messageName == IPC::MessageName::WebIDBConnectionToServer_DidGetAllDatabaseNamesAndVersions)
        return true;
    if (messageName == IPC::MessageName::WebFullScreenManager_RequestExitFullScreen)
        return true;
    if (messageName == IPC::MessageName::WebFullScreenManager_WillEnterFullScreen)
        return true;
    if (messageName == IPC::MessageName::WebFullScreenManager_DidEnterFullScreen)
        return true;
    if (messageName == IPC::MessageName::WebFullScreenManager_WillExitFullScreen)
        return true;
    if (messageName == IPC::MessageName::WebFullScreenManager_DidExitFullScreen)
        return true;
    if (messageName == IPC::MessageName::WebFullScreenManager_SetAnimatingFullScreen)
        return true;
    if (messageName == IPC::MessageName::WebFullScreenManager_SaveScrollPosition)
        return true;
    if (messageName == IPC::MessageName::WebFullScreenManager_RestoreScrollPosition)
        return true;
    if (messageName == IPC::MessageName::WebFullScreenManager_SetFullscreenInsets)
        return true;
    if (messageName == IPC::MessageName::WebFullScreenManager_SetFullscreenAutoHideDuration)
        return true;
    if (messageName == IPC::MessageName::WebFullScreenManager_SetFullscreenControlsHidden)
        return true;
    if (messageName == IPC::MessageName::GPUProcessConnection_DidReceiveRemoteCommand)
        return true;
    if (messageName == IPC::MessageName::RemoteRenderingBackend_CreateImageBufferBackend)
        return true;
    if (messageName == IPC::MessageName::RemoteRenderingBackend_CommitImageBufferFlushContext)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_NetworkStateChanged)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_ReadyStateChanged)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_FirstVideoFrameAvailable)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_VolumeChanged)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_MuteChanged)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_TimeChanged)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_DurationChanged)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_RateChanged)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_PlaybackStateChanged)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_EngineFailedToLoad)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_UpdateCachedState)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_CharacteristicChanged)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_SizeChanged)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_AddRemoteAudioTrack)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_RemoveRemoteAudioTrack)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_RemoteAudioTrackConfigurationChanged)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_AddRemoteTextTrack)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_RemoveRemoteTextTrack)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_RemoteTextTrackConfigurationChanged)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_ParseWebVTTFileHeader)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_ParseWebVTTCueData)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_ParseWebVTTCueDataStruct)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_AddDataCue)
        return true;
#if ENABLE(DATACUE_VALUE)
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_AddDataCueWithType)
        return true;
#endif
#if ENABLE(DATACUE_VALUE)
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_UpdateDataCue)
        return true;
#endif
#if ENABLE(DATACUE_VALUE)
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_RemoveDataCue)
        return true;
#endif
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_AddGenericCue)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_UpdateGenericCue)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_RemoveGenericCue)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_AddRemoteVideoTrack)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_RemoveRemoteVideoTrack)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_RemoteVideoTrackConfigurationChanged)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_RequestResource)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_RequestResourceReply)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_RemoveResource)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_ResourceNotSupported)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_EngineUpdated)
        return true;
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_ActiveSourceBuffersChanged)
        return true;
#if ENABLE(ENCRYPTED_MEDIA)
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_WaitingForKeyChanged)
        return true;
#endif
#if ENABLE(ENCRYPTED_MEDIA)
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_InitializationDataEncountered)
        return true;
#endif
#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_MediaPlayerKeyNeeded)
        return true;
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    if (messageName == IPC::MessageName::MediaPlayerPrivateRemote_CurrentPlaybackTargetIsWirelessChanged)
        return true;
#endif
    if (messageName == IPC::MessageName::RemoteAudioDestinationProxy_RenderBuffer)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioDestinationProxy_RenderBufferReply)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioDestinationProxy_DidChangeIsPlaying)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioSession_ConfigurationChanged)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioSession_BeginInterruption)
        return true;
    if (messageName == IPC::MessageName::RemoteAudioSession_EndInterruption)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceSession_UpdateKeyStatuses)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceSession_SendMessage)
        return true;
    if (messageName == IPC::MessageName::RemoteCDMInstanceSession_SessionIdChanged)
        return true;
    if (messageName == IPC::MessageName::RemoteLegacyCDMSession_SendMessage)
        return true;
    if (messageName == IPC::MessageName::RemoteLegacyCDMSession_SendError)
        return true;
    if (messageName == IPC::MessageName::LibWebRTCCodecs_FailedDecoding)
        return true;
    if (messageName == IPC::MessageName::LibWebRTCCodecs_CompletedDecoding)
        return true;
    if (messageName == IPC::MessageName::LibWebRTCCodecs_CompletedEncoding)
        return true;
    if (messageName == IPC::MessageName::SampleBufferDisplayLayer_SetDidFail)
        return true;
    if (messageName == IPC::MessageName::WebGeolocationManager_DidChangePosition)
        return true;
    if (messageName == IPC::MessageName::WebGeolocationManager_DidFailToDeterminePosition)
        return true;
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebGeolocationManager_ResetPermissions)
        return true;
#endif
    if (messageName == IPC::MessageName::RemoteWebInspectorUI_Initialize)
        return true;
    if (messageName == IPC::MessageName::RemoteWebInspectorUI_UpdateFindString)
        return true;
#if ENABLE(INSPECTOR_TELEMETRY)
    if (messageName == IPC::MessageName::RemoteWebInspectorUI_SetDiagnosticLoggingAvailable)
        return true;
#endif
    if (messageName == IPC::MessageName::RemoteWebInspectorUI_DidSave)
        return true;
    if (messageName == IPC::MessageName::RemoteWebInspectorUI_DidAppend)
        return true;
    if (messageName == IPC::MessageName::RemoteWebInspectorUI_SendMessageToFrontend)
        return true;
    if (messageName == IPC::MessageName::WebInspector_Show)
        return true;
    if (messageName == IPC::MessageName::WebInspector_Close)
        return true;
    if (messageName == IPC::MessageName::WebInspector_SetAttached)
        return true;
    if (messageName == IPC::MessageName::WebInspector_ShowConsole)
        return true;
    if (messageName == IPC::MessageName::WebInspector_ShowResources)
        return true;
    if (messageName == IPC::MessageName::WebInspector_ShowMainResourceForFrame)
        return true;
    if (messageName == IPC::MessageName::WebInspector_OpenInNewTab)
        return true;
    if (messageName == IPC::MessageName::WebInspector_StartPageProfiling)
        return true;
    if (messageName == IPC::MessageName::WebInspector_StopPageProfiling)
        return true;
    if (messageName == IPC::MessageName::WebInspector_StartElementSelection)
        return true;
    if (messageName == IPC::MessageName::WebInspector_StopElementSelection)
        return true;
    if (messageName == IPC::MessageName::WebInspector_SetFrontendConnection)
        return true;
    if (messageName == IPC::MessageName::WebInspectorInterruptDispatcher_NotifyNeedDebuggerBreak)
        return true;
    if (messageName == IPC::MessageName::WebInspectorUI_EstablishConnection)
        return true;
    if (messageName == IPC::MessageName::WebInspectorUI_UpdateConnection)
        return true;
    if (messageName == IPC::MessageName::WebInspectorUI_AttachedBottom)
        return true;
    if (messageName == IPC::MessageName::WebInspectorUI_AttachedRight)
        return true;
    if (messageName == IPC::MessageName::WebInspectorUI_AttachedLeft)
        return true;
    if (messageName == IPC::MessageName::WebInspectorUI_Detached)
        return true;
    if (messageName == IPC::MessageName::WebInspectorUI_SetDockingUnavailable)
        return true;
    if (messageName == IPC::MessageName::WebInspectorUI_SetIsVisible)
        return true;
    if (messageName == IPC::MessageName::WebInspectorUI_UpdateFindString)
        return true;
#if ENABLE(INSPECTOR_TELEMETRY)
    if (messageName == IPC::MessageName::WebInspectorUI_SetDiagnosticLoggingAvailable)
        return true;
#endif
    if (messageName == IPC::MessageName::WebInspectorUI_ShowConsole)
        return true;
    if (messageName == IPC::MessageName::WebInspectorUI_ShowResources)
        return true;
    if (messageName == IPC::MessageName::WebInspectorUI_ShowMainResourceForFrame)
        return true;
    if (messageName == IPC::MessageName::WebInspectorUI_StartPageProfiling)
        return true;
    if (messageName == IPC::MessageName::WebInspectorUI_StopPageProfiling)
        return true;
    if (messageName == IPC::MessageName::WebInspectorUI_StartElementSelection)
        return true;
    if (messageName == IPC::MessageName::WebInspectorUI_StopElementSelection)
        return true;
    if (messageName == IPC::MessageName::WebInspectorUI_DidSave)
        return true;
    if (messageName == IPC::MessageName::WebInspectorUI_DidAppend)
        return true;
    if (messageName == IPC::MessageName::WebInspectorUI_SendMessageToFrontend)
        return true;
    if (messageName == IPC::MessageName::LibWebRTCNetwork_SignalReadPacket)
        return true;
    if (messageName == IPC::MessageName::LibWebRTCNetwork_SignalSentPacket)
        return true;
    if (messageName == IPC::MessageName::LibWebRTCNetwork_SignalAddressReady)
        return true;
    if (messageName == IPC::MessageName::LibWebRTCNetwork_SignalConnect)
        return true;
    if (messageName == IPC::MessageName::LibWebRTCNetwork_SignalClose)
        return true;
    if (messageName == IPC::MessageName::LibWebRTCNetwork_SignalNewConnection)
        return true;
    if (messageName == IPC::MessageName::WebMDNSRegister_FinishedRegisteringMDNSName)
        return true;
    if (messageName == IPC::MessageName::WebRTCMonitor_NetworksChanged)
        return true;
    if (messageName == IPC::MessageName::WebRTCResolver_SetResolvedAddress)
        return true;
    if (messageName == IPC::MessageName::WebRTCResolver_ResolvedAddressError)
        return true;
#if ENABLE(SHAREABLE_RESOURCE)
    if (messageName == IPC::MessageName::NetworkProcessConnection_DidCacheResource)
        return true;
#endif
    if (messageName == IPC::MessageName::NetworkProcessConnection_DidFinishPingLoad)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessConnection_DidFinishPreconnection)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessConnection_SetOnLineState)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessConnection_CookieAcceptPolicyChanged)
        return true;
#if HAVE(COOKIE_CHANGE_LISTENER_API)
    if (messageName == IPC::MessageName::NetworkProcessConnection_CookiesAdded)
        return true;
#endif
#if HAVE(COOKIE_CHANGE_LISTENER_API)
    if (messageName == IPC::MessageName::NetworkProcessConnection_CookiesDeleted)
        return true;
#endif
#if HAVE(COOKIE_CHANGE_LISTENER_API)
    if (messageName == IPC::MessageName::NetworkProcessConnection_AllCookiesDeleted)
        return true;
#endif
    if (messageName == IPC::MessageName::NetworkProcessConnection_CheckProcessLocalPortForActivity)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessConnection_CheckProcessLocalPortForActivityReply)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessConnection_MessagesAvailableForPort)
        return true;
    if (messageName == IPC::MessageName::NetworkProcessConnection_BroadcastConsoleMessage)
        return true;
    if (messageName == IPC::MessageName::WebResourceLoader_WillSendRequest)
        return true;
    if (messageName == IPC::MessageName::WebResourceLoader_DidSendData)
        return true;
    if (messageName == IPC::MessageName::WebResourceLoader_DidReceiveResponse)
        return true;
    if (messageName == IPC::MessageName::WebResourceLoader_DidReceiveData)
        return true;
    if (messageName == IPC::MessageName::WebResourceLoader_DidReceiveSharedBuffer)
        return true;
    if (messageName == IPC::MessageName::WebResourceLoader_DidFinishResourceLoad)
        return true;
    if (messageName == IPC::MessageName::WebResourceLoader_DidFailResourceLoad)
        return true;
    if (messageName == IPC::MessageName::WebResourceLoader_DidFailServiceWorkerLoad)
        return true;
    if (messageName == IPC::MessageName::WebResourceLoader_ServiceWorkerDidNotHandle)
        return true;
    if (messageName == IPC::MessageName::WebResourceLoader_DidBlockAuthenticationChallenge)
        return true;
    if (messageName == IPC::MessageName::WebResourceLoader_StopLoadingAfterXFrameOptionsOrContentSecurityPolicyDenied)
        return true;
#if ENABLE(SHAREABLE_RESOURCE)
    if (messageName == IPC::MessageName::WebResourceLoader_DidReceiveResource)
        return true;
#endif
    if (messageName == IPC::MessageName::WebSocketChannel_DidConnect)
        return true;
    if (messageName == IPC::MessageName::WebSocketChannel_DidClose)
        return true;
    if (messageName == IPC::MessageName::WebSocketChannel_DidReceiveText)
        return true;
    if (messageName == IPC::MessageName::WebSocketChannel_DidReceiveBinaryData)
        return true;
    if (messageName == IPC::MessageName::WebSocketChannel_DidReceiveMessageError)
        return true;
    if (messageName == IPC::MessageName::WebSocketChannel_DidSendHandshakeRequest)
        return true;
    if (messageName == IPC::MessageName::WebSocketChannel_DidReceiveHandshakeResponse)
        return true;
    if (messageName == IPC::MessageName::WebSocketStream_DidOpenSocketStream)
        return true;
    if (messageName == IPC::MessageName::WebSocketStream_DidCloseSocketStream)
        return true;
    if (messageName == IPC::MessageName::WebSocketStream_DidReceiveSocketStreamData)
        return true;
    if (messageName == IPC::MessageName::WebSocketStream_DidFailToReceiveSocketStreamData)
        return true;
    if (messageName == IPC::MessageName::WebSocketStream_DidUpdateBufferedAmount)
        return true;
    if (messageName == IPC::MessageName::WebSocketStream_DidFailSocketStream)
        return true;
    if (messageName == IPC::MessageName::WebSocketStream_DidSendData)
        return true;
    if (messageName == IPC::MessageName::WebSocketStream_DidSendHandshake)
        return true;
    if (messageName == IPC::MessageName::WebNotificationManager_DidShowNotification)
        return true;
    if (messageName == IPC::MessageName::WebNotificationManager_DidClickNotification)
        return true;
    if (messageName == IPC::MessageName::WebNotificationManager_DidCloseNotifications)
        return true;
    if (messageName == IPC::MessageName::WebNotificationManager_DidUpdateNotificationDecision)
        return true;
    if (messageName == IPC::MessageName::WebNotificationManager_DidRemoveNotificationDecisions)
        return true;
    if (messageName == IPC::MessageName::PluginProcessConnection_SetException)
        return true;
    if (messageName == IPC::MessageName::PluginProcessConnectionManager_PluginProcessCrashed)
        return true;
    if (messageName == IPC::MessageName::PluginProxy_LoadURL)
        return true;
    if (messageName == IPC::MessageName::PluginProxy_Update)
        return true;
    if (messageName == IPC::MessageName::PluginProxy_ProxiesForURL)
        return true;
    if (messageName == IPC::MessageName::PluginProxy_CookiesForURL)
        return true;
    if (messageName == IPC::MessageName::PluginProxy_SetCookiesForURL)
        return true;
    if (messageName == IPC::MessageName::PluginProxy_GetAuthenticationInfo)
        return true;
    if (messageName == IPC::MessageName::PluginProxy_GetPluginElementNPObject)
        return true;
    if (messageName == IPC::MessageName::PluginProxy_Evaluate)
        return true;
    if (messageName == IPC::MessageName::PluginProxy_CancelStreamLoad)
        return true;
    if (messageName == IPC::MessageName::PluginProxy_ContinueStreamLoad)
        return true;
    if (messageName == IPC::MessageName::PluginProxy_CancelManualStreamLoad)
        return true;
    if (messageName == IPC::MessageName::PluginProxy_SetStatusbarText)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::PluginProxy_PluginFocusOrWindowFocusChanged)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::PluginProxy_SetComplexTextInputState)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::PluginProxy_SetLayerHostingContextID)
        return true;
#endif
#if PLATFORM(X11)
    if (messageName == IPC::MessageName::PluginProxy_CreatePluginContainer)
        return true;
#endif
#if PLATFORM(X11)
    if (messageName == IPC::MessageName::PluginProxy_WindowedPluginGeometryDidChange)
        return true;
#endif
#if PLATFORM(X11)
    if (messageName == IPC::MessageName::PluginProxy_WindowedPluginVisibilityDidChange)
        return true;
#endif
    if (messageName == IPC::MessageName::PluginProxy_DidCreatePlugin)
        return true;
    if (messageName == IPC::MessageName::PluginProxy_DidFailToCreatePlugin)
        return true;
    if (messageName == IPC::MessageName::PluginProxy_SetPluginIsPlayingAudio)
        return true;
    if (messageName == IPC::MessageName::WebSWClientConnection_JobRejectedInServer)
        return true;
    if (messageName == IPC::MessageName::WebSWClientConnection_RegistrationJobResolvedInServer)
        return true;
    if (messageName == IPC::MessageName::WebSWClientConnection_StartScriptFetchForServer)
        return true;
    if (messageName == IPC::MessageName::WebSWClientConnection_UpdateRegistrationState)
        return true;
    if (messageName == IPC::MessageName::WebSWClientConnection_UpdateWorkerState)
        return true;
    if (messageName == IPC::MessageName::WebSWClientConnection_FireUpdateFoundEvent)
        return true;
    if (messageName == IPC::MessageName::WebSWClientConnection_SetRegistrationLastUpdateTime)
        return true;
    if (messageName == IPC::MessageName::WebSWClientConnection_SetRegistrationUpdateViaCache)
        return true;
    if (messageName == IPC::MessageName::WebSWClientConnection_NotifyClientsOfControllerChange)
        return true;
    if (messageName == IPC::MessageName::WebSWClientConnection_SetSWOriginTableIsImported)
        return true;
    if (messageName == IPC::MessageName::WebSWClientConnection_SetSWOriginTableSharedMemory)
        return true;
    if (messageName == IPC::MessageName::WebSWClientConnection_PostMessageToServiceWorkerClient)
        return true;
    if (messageName == IPC::MessageName::WebSWClientConnection_DidMatchRegistration)
        return true;
    if (messageName == IPC::MessageName::WebSWClientConnection_DidGetRegistrations)
        return true;
    if (messageName == IPC::MessageName::WebSWClientConnection_RegistrationReady)
        return true;
    if (messageName == IPC::MessageName::WebSWClientConnection_SetDocumentIsControlled)
        return true;
    if (messageName == IPC::MessageName::WebSWClientConnection_SetDocumentIsControlledReply)
        return true;
    if (messageName == IPC::MessageName::WebSWContextManagerConnection_InstallServiceWorker)
        return true;
    if (messageName == IPC::MessageName::WebSWContextManagerConnection_StartFetch)
        return true;
    if (messageName == IPC::MessageName::WebSWContextManagerConnection_CancelFetch)
        return true;
    if (messageName == IPC::MessageName::WebSWContextManagerConnection_ContinueDidReceiveFetchResponse)
        return true;
    if (messageName == IPC::MessageName::WebSWContextManagerConnection_PostMessageToServiceWorker)
        return true;
    if (messageName == IPC::MessageName::WebSWContextManagerConnection_FireInstallEvent)
        return true;
    if (messageName == IPC::MessageName::WebSWContextManagerConnection_FireActivateEvent)
        return true;
    if (messageName == IPC::MessageName::WebSWContextManagerConnection_TerminateWorker)
        return true;
    if (messageName == IPC::MessageName::WebSWContextManagerConnection_FindClientByIdentifierCompleted)
        return true;
    if (messageName == IPC::MessageName::WebSWContextManagerConnection_MatchAllCompleted)
        return true;
    if (messageName == IPC::MessageName::WebSWContextManagerConnection_SetUserAgent)
        return true;
    if (messageName == IPC::MessageName::WebSWContextManagerConnection_UpdatePreferencesStore)
        return true;
    if (messageName == IPC::MessageName::WebSWContextManagerConnection_Close)
        return true;
    if (messageName == IPC::MessageName::WebSWContextManagerConnection_SetThrottleState)
        return true;
    if (messageName == IPC::MessageName::WebUserContentController_AddContentWorlds)
        return true;
    if (messageName == IPC::MessageName::WebUserContentController_RemoveContentWorlds)
        return true;
    if (messageName == IPC::MessageName::WebUserContentController_AddUserScripts)
        return true;
    if (messageName == IPC::MessageName::WebUserContentController_RemoveUserScript)
        return true;
    if (messageName == IPC::MessageName::WebUserContentController_RemoveAllUserScripts)
        return true;
    if (messageName == IPC::MessageName::WebUserContentController_AddUserStyleSheets)
        return true;
    if (messageName == IPC::MessageName::WebUserContentController_RemoveUserStyleSheet)
        return true;
    if (messageName == IPC::MessageName::WebUserContentController_RemoveAllUserStyleSheets)
        return true;
    if (messageName == IPC::MessageName::WebUserContentController_AddUserScriptMessageHandlers)
        return true;
    if (messageName == IPC::MessageName::WebUserContentController_RemoveUserScriptMessageHandler)
        return true;
    if (messageName == IPC::MessageName::WebUserContentController_RemoveAllUserScriptMessageHandlersForWorlds)
        return true;
    if (messageName == IPC::MessageName::WebUserContentController_RemoveAllUserScriptMessageHandlers)
        return true;
#if ENABLE(CONTENT_EXTENSIONS)
    if (messageName == IPC::MessageName::WebUserContentController_AddContentRuleLists)
        return true;
#endif
#if ENABLE(CONTENT_EXTENSIONS)
    if (messageName == IPC::MessageName::WebUserContentController_RemoveContentRuleList)
        return true;
#endif
#if ENABLE(CONTENT_EXTENSIONS)
    if (messageName == IPC::MessageName::WebUserContentController_RemoveAllContentRuleLists)
        return true;
#endif
#if USE(COORDINATED_GRAPHICS) || USE(TEXTURE_MAPPER)
    if (messageName == IPC::MessageName::DrawingArea_UpdateBackingStoreState)
        return true;
#endif
    if (messageName == IPC::MessageName::DrawingArea_DidUpdate)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::DrawingArea_UpdateGeometry)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::DrawingArea_SetDeviceScaleFactor)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::DrawingArea_SetColorSpace)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::DrawingArea_SetViewExposedRect)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::DrawingArea_AdjustTransientZoom)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::DrawingArea_CommitTransientZoom)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::DrawingArea_AcceleratedAnimationDidStart)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::DrawingArea_AcceleratedAnimationDidEnd)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::DrawingArea_AddTransactionCallbackID)
        return true;
#endif
    if (messageName == IPC::MessageName::EventDispatcher_WheelEvent)
        return true;
#if ENABLE(IOS_TOUCH_EVENTS)
    if (messageName == IPC::MessageName::EventDispatcher_TouchEvent)
        return true;
#endif
#if ENABLE(MAC_GESTURE_EVENTS)
    if (messageName == IPC::MessageName::EventDispatcher_GestureEvent)
        return true;
#endif
#if ENABLE(WEBPROCESS_WINDOWSERVER_BLOCKING)
    if (messageName == IPC::MessageName::EventDispatcher_DisplayWasRefreshed)
        return true;
#endif
    if (messageName == IPC::MessageName::VisitedLinkTableController_SetVisitedLinkTable)
        return true;
    if (messageName == IPC::MessageName::VisitedLinkTableController_VisitedLinkStateChanged)
        return true;
    if (messageName == IPC::MessageName::VisitedLinkTableController_AllVisitedLinkStateChanged)
        return true;
    if (messageName == IPC::MessageName::VisitedLinkTableController_RemoveAllVisitedLinks)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetInitialFocus)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetInitialFocusReply)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetActivityState)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetLayerHostingMode)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetBackgroundColor)
        return true;
    if (messageName == IPC::MessageName::WebPage_AddConsoleMessage)
        return true;
    if (messageName == IPC::MessageName::WebPage_SendCSPViolationReport)
        return true;
    if (messageName == IPC::MessageName::WebPage_EnqueueSecurityPolicyViolationEvent)
        return true;
    if (messageName == IPC::MessageName::WebPage_TestProcessIncomingSyncMessagesWhenWaitingForSyncReply)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_SetTopContentInsetFenced)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_SetTopContentInset)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetUnderlayColor)
        return true;
    if (messageName == IPC::MessageName::WebPage_ViewWillStartLiveResize)
        return true;
    if (messageName == IPC::MessageName::WebPage_ViewWillEndLiveResize)
        return true;
    if (messageName == IPC::MessageName::WebPage_ExecuteEditCommandWithCallback)
        return true;
    if (messageName == IPC::MessageName::WebPage_ExecuteEditCommandWithCallbackReply)
        return true;
    if (messageName == IPC::MessageName::WebPage_KeyEvent)
        return true;
    if (messageName == IPC::MessageName::WebPage_MouseEvent)
        return true;
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SetViewportConfigurationViewLayoutSize)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SetMaximumUnobscuredSize)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SetDeviceOrientation)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SetOverrideViewportArguments)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_DynamicViewportSizeUpdate)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SetScreenIsBeingCaptured)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_HandleTap)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_PotentialTapAtPosition)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_CommitPotentialTap)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_CancelPotentialTap)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_TapHighlightAtPosition)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_DidRecognizeLongPress)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_HandleDoubleTapForDoubleClickAtPoint)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_InspectorNodeSearchMovedToPosition)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_InspectorNodeSearchEndedAtPosition)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_BlurFocusedElement)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SelectWithGesture)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_UpdateSelectionWithTouches)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SelectWithTwoTouches)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_ExtendSelection)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SelectWordBackward)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_MoveSelectionByOffset)
        return true;
    if (messageName == IPC::MessageName::WebPage_MoveSelectionByOffsetReply)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SelectTextWithGranularityAtPoint)
        return true;
    if (messageName == IPC::MessageName::WebPage_SelectTextWithGranularityAtPointReply)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SelectPositionAtBoundaryWithDirection)
        return true;
    if (messageName == IPC::MessageName::WebPage_SelectPositionAtBoundaryWithDirectionReply)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_MoveSelectionAtBoundaryWithDirection)
        return true;
    if (messageName == IPC::MessageName::WebPage_MoveSelectionAtBoundaryWithDirectionReply)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SelectPositionAtPoint)
        return true;
    if (messageName == IPC::MessageName::WebPage_SelectPositionAtPointReply)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_BeginSelectionInDirection)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_UpdateSelectionWithExtentPoint)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_UpdateSelectionWithExtentPointAndBoundary)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_RequestDictationContext)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_ReplaceDictatedText)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_ReplaceSelectedText)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_RequestAutocorrectionData)
        return true;
    if (messageName == IPC::MessageName::WebPage_RequestAutocorrectionDataReply)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_ApplyAutocorrection)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SyncApplyAutocorrection)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_RequestAutocorrectionContext)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_RequestEvasionRectsAboveSelection)
        return true;
    if (messageName == IPC::MessageName::WebPage_RequestEvasionRectsAboveSelectionReply)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_GetPositionInformation)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_RequestPositionInformation)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_StartInteractionWithElementContextOrPosition)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_StopInteraction)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_PerformActionOnElement)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_FocusNextFocusedElement)
        return true;
    if (messageName == IPC::MessageName::WebPage_FocusNextFocusedElementReply)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SetFocusedElementValue)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_AutofillLoginCredentials)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SetFocusedElementValueAsNumber)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SetFocusedElementSelectedIndex)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_ApplicationWillResignActive)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_ApplicationDidEnterBackground)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_ApplicationDidFinishSnapshottingAfterEnteringBackground)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_ApplicationWillEnterForeground)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_ApplicationDidBecomeActive)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_ApplicationDidEnterBackgroundForMedia)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_ApplicationWillEnterForegroundForMedia)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_ContentSizeCategoryDidChange)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_GetSelectionContext)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SetAllowsMediaDocumentInlinePlayback)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_HandleTwoFingerTapAtPoint)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_HandleStylusSingleTapAtPoint)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SetForceAlwaysUserScalable)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_GetRectsForGranularityWithSelectionOffset)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_GetRectsAtSelectionOffsetWithText)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_StoreSelectionForAccessibility)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_StartAutoscrollAtPosition)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_CancelAutoscroll)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_RequestFocusedElementInformation)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_HardwareKeyboardAvailabilityChanged)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SetIsShowingInputViewForFocusedElement)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_UpdateSelectionWithDelta)
        return true;
    if (messageName == IPC::MessageName::WebPage_UpdateSelectionWithDeltaReply)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_RequestDocumentEditingContext)
        return true;
    if (messageName == IPC::MessageName::WebPage_RequestDocumentEditingContextReply)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_GenerateSyntheticEditingCommand)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SetShouldRevealCurrentSelectionAfterInsertion)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_InsertTextPlaceholder)
        return true;
    if (messageName == IPC::MessageName::WebPage_InsertTextPlaceholderReply)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_RemoveTextPlaceholder)
        return true;
    if (messageName == IPC::MessageName::WebPage_RemoveTextPlaceholderReply)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_TextInputContextsInRect)
        return true;
    if (messageName == IPC::MessageName::WebPage_TextInputContextsInRectReply)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_FocusTextInputContextAndPlaceCaret)
        return true;
    if (messageName == IPC::MessageName::WebPage_FocusTextInputContextAndPlaceCaretReply)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_ClearServiceWorkerEntitlementOverride)
        return true;
    if (messageName == IPC::MessageName::WebPage_ClearServiceWorkerEntitlementOverrideReply)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_SetControlledByAutomation)
        return true;
    if (messageName == IPC::MessageName::WebPage_ConnectInspector)
        return true;
    if (messageName == IPC::MessageName::WebPage_DisconnectInspector)
        return true;
    if (messageName == IPC::MessageName::WebPage_SendMessageToTargetBackend)
        return true;
#if ENABLE(REMOTE_INSPECTOR)
    if (messageName == IPC::MessageName::WebPage_SetIndicating)
        return true;
#endif
#if ENABLE(IOS_TOUCH_EVENTS)
    if (messageName == IPC::MessageName::WebPage_ResetPotentialTapSecurityOrigin)
        return true;
#endif
#if ENABLE(IOS_TOUCH_EVENTS)
    if (messageName == IPC::MessageName::WebPage_TouchEventSync)
        return true;
#endif
#if !ENABLE(IOS_TOUCH_EVENTS) && ENABLE(TOUCH_EVENTS)
    if (messageName == IPC::MessageName::WebPage_TouchEvent)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_CancelPointer)
        return true;
    if (messageName == IPC::MessageName::WebPage_TouchWithIdentifierWasRemoved)
        return true;
#if ENABLE(INPUT_TYPE_COLOR)
    if (messageName == IPC::MessageName::WebPage_DidEndColorPicker)
        return true;
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    if (messageName == IPC::MessageName::WebPage_DidChooseColor)
        return true;
#endif
#if ENABLE(DATALIST_ELEMENT)
    if (messageName == IPC::MessageName::WebPage_DidSelectDataListOption)
        return true;
#endif
#if ENABLE(DATALIST_ELEMENT)
    if (messageName == IPC::MessageName::WebPage_DidCloseSuggestions)
        return true;
#endif
#if ENABLE(CONTEXT_MENUS)
    if (messageName == IPC::MessageName::WebPage_ContextMenuHidden)
        return true;
#endif
#if ENABLE(CONTEXT_MENUS)
    if (messageName == IPC::MessageName::WebPage_ContextMenuForKeyEvent)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_ScrollBy)
        return true;
    if (messageName == IPC::MessageName::WebPage_CenterSelectionInVisibleArea)
        return true;
    if (messageName == IPC::MessageName::WebPage_GoToBackForwardItem)
        return true;
    if (messageName == IPC::MessageName::WebPage_TryRestoreScrollPosition)
        return true;
    if (messageName == IPC::MessageName::WebPage_LoadURLInFrame)
        return true;
    if (messageName == IPC::MessageName::WebPage_LoadDataInFrame)
        return true;
    if (messageName == IPC::MessageName::WebPage_LoadRequest)
        return true;
    if (messageName == IPC::MessageName::WebPage_LoadRequestWaitingForProcessLaunch)
        return true;
    if (messageName == IPC::MessageName::WebPage_LoadData)
        return true;
    if (messageName == IPC::MessageName::WebPage_LoadAlternateHTML)
        return true;
    if (messageName == IPC::MessageName::WebPage_NavigateToPDFLinkWithSimulatedClick)
        return true;
    if (messageName == IPC::MessageName::WebPage_Reload)
        return true;
    if (messageName == IPC::MessageName::WebPage_StopLoading)
        return true;
    if (messageName == IPC::MessageName::WebPage_StopLoadingFrame)
        return true;
    if (messageName == IPC::MessageName::WebPage_RestoreSession)
        return true;
    if (messageName == IPC::MessageName::WebPage_UpdateBackForwardListForReattach)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetCurrentHistoryItemForReattach)
        return true;
    if (messageName == IPC::MessageName::WebPage_DidRemoveBackForwardItem)
        return true;
    if (messageName == IPC::MessageName::WebPage_UpdateWebsitePolicies)
        return true;
    if (messageName == IPC::MessageName::WebPage_NotifyUserScripts)
        return true;
    if (messageName == IPC::MessageName::WebPage_DidReceivePolicyDecision)
        return true;
    if (messageName == IPC::MessageName::WebPage_ContinueWillSubmitForm)
        return true;
    if (messageName == IPC::MessageName::WebPage_ClearSelection)
        return true;
    if (messageName == IPC::MessageName::WebPage_RestoreSelectionInFocusedEditableElement)
        return true;
    if (messageName == IPC::MessageName::WebPage_GetContentsAsString)
        return true;
    if (messageName == IPC::MessageName::WebPage_GetAllFrames)
        return true;
    if (messageName == IPC::MessageName::WebPage_GetAllFramesReply)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_GetContentsAsAttributedString)
        return true;
    if (messageName == IPC::MessageName::WebPage_GetContentsAsAttributedStringReply)
        return true;
#endif
#if ENABLE(MHTML)
    if (messageName == IPC::MessageName::WebPage_GetContentsAsMHTMLData)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_GetMainResourceDataOfFrame)
        return true;
    if (messageName == IPC::MessageName::WebPage_GetResourceDataFromFrame)
        return true;
    if (messageName == IPC::MessageName::WebPage_GetRenderTreeExternalRepresentation)
        return true;
    if (messageName == IPC::MessageName::WebPage_GetSelectionOrContentsAsString)
        return true;
    if (messageName == IPC::MessageName::WebPage_GetSelectionAsWebArchiveData)
        return true;
    if (messageName == IPC::MessageName::WebPage_GetSourceForFrame)
        return true;
    if (messageName == IPC::MessageName::WebPage_GetWebArchiveOfFrame)
        return true;
    if (messageName == IPC::MessageName::WebPage_RunJavaScriptInFrameInScriptWorld)
        return true;
    if (messageName == IPC::MessageName::WebPage_ForceRepaint)
        return true;
    if (messageName == IPC::MessageName::WebPage_SelectAll)
        return true;
    if (messageName == IPC::MessageName::WebPage_ScheduleFullEditorStateUpdate)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_PerformDictionaryLookupOfCurrentSelection)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_PerformDictionaryLookupAtLocation)
        return true;
#endif
#if ENABLE(DATA_DETECTION)
    if (messageName == IPC::MessageName::WebPage_DetectDataInAllFrames)
        return true;
    if (messageName == IPC::MessageName::WebPage_DetectDataInAllFramesReply)
        return true;
#endif
#if ENABLE(DATA_DETECTION)
    if (messageName == IPC::MessageName::WebPage_RemoveDataDetectedLinks)
        return true;
    if (messageName == IPC::MessageName::WebPage_RemoveDataDetectedLinksReply)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_ChangeFont)
        return true;
    if (messageName == IPC::MessageName::WebPage_ChangeFontAttributes)
        return true;
    if (messageName == IPC::MessageName::WebPage_PreferencesDidChange)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetUserAgent)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetCustomTextEncodingName)
        return true;
    if (messageName == IPC::MessageName::WebPage_SuspendActiveDOMObjectsAndAnimations)
        return true;
    if (messageName == IPC::MessageName::WebPage_ResumeActiveDOMObjectsAndAnimations)
        return true;
    if (messageName == IPC::MessageName::WebPage_Close)
        return true;
    if (messageName == IPC::MessageName::WebPage_TryClose)
        return true;
    if (messageName == IPC::MessageName::WebPage_TryCloseReply)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetEditable)
        return true;
    if (messageName == IPC::MessageName::WebPage_ValidateCommand)
        return true;
    if (messageName == IPC::MessageName::WebPage_ExecuteEditCommand)
        return true;
    if (messageName == IPC::MessageName::WebPage_IncreaseListLevel)
        return true;
    if (messageName == IPC::MessageName::WebPage_DecreaseListLevel)
        return true;
    if (messageName == IPC::MessageName::WebPage_ChangeListType)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetBaseWritingDirection)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetNeedsFontAttributes)
        return true;
    if (messageName == IPC::MessageName::WebPage_RequestFontAttributesAtSelectionStart)
        return true;
    if (messageName == IPC::MessageName::WebPage_DidRemoveEditCommand)
        return true;
    if (messageName == IPC::MessageName::WebPage_ReapplyEditCommand)
        return true;
    if (messageName == IPC::MessageName::WebPage_UnapplyEditCommand)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetPageAndTextZoomFactors)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetPageZoomFactor)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetTextZoomFactor)
        return true;
    if (messageName == IPC::MessageName::WebPage_WindowScreenDidChange)
        return true;
    if (messageName == IPC::MessageName::WebPage_AccessibilitySettingsDidChange)
        return true;
    if (messageName == IPC::MessageName::WebPage_ScalePage)
        return true;
    if (messageName == IPC::MessageName::WebPage_ScalePageInViewCoordinates)
        return true;
    if (messageName == IPC::MessageName::WebPage_ScaleView)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetUseFixedLayout)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetFixedLayoutSize)
        return true;
    if (messageName == IPC::MessageName::WebPage_ListenForLayoutMilestones)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetSuppressScrollbarAnimations)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetEnableVerticalRubberBanding)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetEnableHorizontalRubberBanding)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetBackgroundExtendsBeyondPage)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetPaginationMode)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetPaginationBehavesLikeColumns)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetPageLength)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetGapBetweenPages)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetPaginationLineGridEnabled)
        return true;
    if (messageName == IPC::MessageName::WebPage_PostInjectedBundleMessage)
        return true;
    if (messageName == IPC::MessageName::WebPage_FindString)
        return true;
    if (messageName == IPC::MessageName::WebPage_FindStringMatches)
        return true;
    if (messageName == IPC::MessageName::WebPage_GetImageForFindMatch)
        return true;
    if (messageName == IPC::MessageName::WebPage_SelectFindMatch)
        return true;
    if (messageName == IPC::MessageName::WebPage_IndicateFindMatch)
        return true;
    if (messageName == IPC::MessageName::WebPage_HideFindUI)
        return true;
    if (messageName == IPC::MessageName::WebPage_CountStringMatches)
        return true;
    if (messageName == IPC::MessageName::WebPage_ReplaceMatches)
        return true;
    if (messageName == IPC::MessageName::WebPage_AddMIMETypeWithCustomContentProvider)
        return true;
#if (PLATFORM(GTK) || PLATFORM(HBD)) && ENABLE(DRAG_SUPPORT)
    if (messageName == IPC::MessageName::WebPage_PerformDragControllerAction)
        return true;
#endif
#if !PLATFORM(GTK) && !PLATFORM(HBD) && ENABLE(DRAG_SUPPORT)
    if (messageName == IPC::MessageName::WebPage_PerformDragControllerAction)
        return true;
#endif
#if ENABLE(DRAG_SUPPORT)
    if (messageName == IPC::MessageName::WebPage_DidStartDrag)
        return true;
#endif
#if ENABLE(DRAG_SUPPORT)
    if (messageName == IPC::MessageName::WebPage_DragEnded)
        return true;
#endif
#if ENABLE(DRAG_SUPPORT)
    if (messageName == IPC::MessageName::WebPage_DragCancelled)
        return true;
#endif
#if PLATFORM(IOS_FAMILY) && ENABLE(DRAG_SUPPORT)
    if (messageName == IPC::MessageName::WebPage_RequestDragStart)
        return true;
#endif
#if PLATFORM(IOS_FAMILY) && ENABLE(DRAG_SUPPORT)
    if (messageName == IPC::MessageName::WebPage_RequestAdditionalItemsForDragSession)
        return true;
#endif
#if PLATFORM(IOS_FAMILY) && ENABLE(DRAG_SUPPORT)
    if (messageName == IPC::MessageName::WebPage_InsertDroppedImagePlaceholders)
        return true;
    if (messageName == IPC::MessageName::WebPage_InsertDroppedImagePlaceholdersReply)
        return true;
#endif
#if PLATFORM(IOS_FAMILY) && ENABLE(DRAG_SUPPORT)
    if (messageName == IPC::MessageName::WebPage_DidConcludeDrop)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_DidChangeSelectedIndexForActivePopupMenu)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetTextForActivePopupMenu)
        return true;
#if PLATFORM(GTK) || PLATFORM(HBD)
    if (messageName == IPC::MessageName::WebPage_FailedToShowPopupMenu)
        return true;
#endif
#if ENABLE(CONTEXT_MENUS)
    if (messageName == IPC::MessageName::WebPage_DidSelectItemFromActiveContextMenu)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_DidChooseFilesForOpenPanelWithDisplayStringAndIcon)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_DidChooseFilesForOpenPanel)
        return true;
    if (messageName == IPC::MessageName::WebPage_DidCancelForOpenPanel)
        return true;
#if ENABLE(SANDBOX_EXTENSIONS)
    if (messageName == IPC::MessageName::WebPage_ExtendSandboxForFilesFromOpenPanel)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_AdvanceToNextMisspelling)
        return true;
    if (messageName == IPC::MessageName::WebPage_ChangeSpellingToWord)
        return true;
    if (messageName == IPC::MessageName::WebPage_DidFinishCheckingText)
        return true;
    if (messageName == IPC::MessageName::WebPage_DidCancelCheckingText)
        return true;
#if USE(APPKIT)
    if (messageName == IPC::MessageName::WebPage_UppercaseWord)
        return true;
#endif
#if USE(APPKIT)
    if (messageName == IPC::MessageName::WebPage_LowercaseWord)
        return true;
#endif
#if USE(APPKIT)
    if (messageName == IPC::MessageName::WebPage_CapitalizeWord)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_SetSmartInsertDeleteEnabled)
        return true;
#endif
#if ENABLE(GEOLOCATION)
    if (messageName == IPC::MessageName::WebPage_DidReceiveGeolocationPermissionDecision)
        return true;
#endif
#if ENABLE(MEDIA_STREAM)
    if (messageName == IPC::MessageName::WebPage_UserMediaAccessWasGranted)
        return true;
    if (messageName == IPC::MessageName::WebPage_UserMediaAccessWasGrantedReply)
        return true;
#endif
#if ENABLE(MEDIA_STREAM)
    if (messageName == IPC::MessageName::WebPage_UserMediaAccessWasDenied)
        return true;
#endif
#if ENABLE(MEDIA_STREAM)
    if (messageName == IPC::MessageName::WebPage_CaptureDevicesChanged)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_StopAllMediaPlayback)
        return true;
    if (messageName == IPC::MessageName::WebPage_SuspendAllMediaPlayback)
        return true;
    if (messageName == IPC::MessageName::WebPage_ResumeAllMediaPlayback)
        return true;
    if (messageName == IPC::MessageName::WebPage_DidReceiveNotificationPermissionDecision)
        return true;
    if (messageName == IPC::MessageName::WebPage_FreezeLayerTreeDueToSwipeAnimation)
        return true;
    if (messageName == IPC::MessageName::WebPage_UnfreezeLayerTreeDueToSwipeAnimation)
        return true;
    if (messageName == IPC::MessageName::WebPage_BeginPrinting)
        return true;
    if (messageName == IPC::MessageName::WebPage_EndPrinting)
        return true;
    if (messageName == IPC::MessageName::WebPage_ComputePagesForPrinting)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_DrawRectToImage)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_DrawPagesToPDF)
        return true;
#endif
#if (PLATFORM(COCOA) && PLATFORM(IOS_FAMILY))
    if (messageName == IPC::MessageName::WebPage_ComputePagesForPrintingAndDrawToPDF)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_DrawToPDF)
        return true;
#endif
#if PLATFORM(GTK)
    if (messageName == IPC::MessageName::WebPage_DrawPagesForPrinting)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_SetMediaVolume)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetMuted)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetMayStartMediaWhenInWindow)
        return true;
    if (messageName == IPC::MessageName::WebPage_StopMediaCapture)
        return true;
#if ENABLE(MEDIA_SESSION)
    if (messageName == IPC::MessageName::WebPage_HandleMediaEvent)
        return true;
#endif
#if ENABLE(MEDIA_SESSION)
    if (messageName == IPC::MessageName::WebPage_SetVolumeOfMediaElement)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_SetCanRunBeforeUnloadConfirmPanel)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetCanRunModal)
        return true;
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    if (messageName == IPC::MessageName::WebPage_CancelComposition)
        return true;
#endif
#if PLATFORM(GTK) || PLATFORM(WPE) || PLATFORM(HBD)
    if (messageName == IPC::MessageName::WebPage_DeleteSurrounding)
        return true;
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    if (messageName == IPC::MessageName::WebPage_CollapseSelectionInFrame)
        return true;
#endif
#if PLATFORM(GTK) || PLATFORM(HBD)
    if (messageName == IPC::MessageName::WebPage_GetCenterForZoomGesture)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_SendComplexTextInputToPlugin)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_WindowAndViewFramesChanged)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_SetMainFrameIsScrollable)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_RegisterUIProcessAccessibilityTokens)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_GetStringSelectionForPasteboard)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_GetDataSelectionForPasteboard)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_ReadSelectionFromPasteboard)
        return true;
#endif
#if (PLATFORM(COCOA) && ENABLE(SERVICE_CONTROLS))
    if (messageName == IPC::MessageName::WebPage_ReplaceSelectionWithPasteboardData)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_ShouldDelayWindowOrderingEvent)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_AcceptsFirstMouse)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_SetTextAsync)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_InsertTextAsync)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_InsertDictatedTextAsync)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_HasMarkedText)
        return true;
    if (messageName == IPC::MessageName::WebPage_HasMarkedTextReply)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_GetMarkedRangeAsync)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_GetSelectedRangeAsync)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_CharacterIndexForPointAsync)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_FirstRectForCharacterRangeAsync)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_SetCompositionAsync)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_ConfirmCompositionAsync)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPage_AttributedSubstringForCharacterRangeAsync)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPage_FontAtSelection)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_SetAlwaysShowsHorizontalScroller)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetAlwaysShowsVerticalScroller)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetMinimumSizeForAutoLayout)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetSizeToContentAutoSizeMaximumSize)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetAutoSizingShouldExpandToViewHeight)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetViewportSizeForCSSViewportUnits)
        return true;
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_HandleAlternativeTextUIResult)
        return true;
#endif
#if PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_WillStartUserTriggeredZooming)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_SetScrollPinningBehavior)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetScrollbarOverlayStyle)
        return true;
    if (messageName == IPC::MessageName::WebPage_GetBytecodeProfile)
        return true;
    if (messageName == IPC::MessageName::WebPage_GetSamplingProfilerOutput)
        return true;
    if (messageName == IPC::MessageName::WebPage_TakeSnapshot)
        return true;
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPage_PerformImmediateActionHitTestAtLocation)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPage_ImmediateActionDidUpdate)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPage_ImmediateActionDidCancel)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPage_ImmediateActionDidComplete)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPage_DataDetectorsDidPresentUI)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPage_DataDetectorsDidChangeUI)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPage_DataDetectorsDidHideUI)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPage_HandleAcceptedCandidate)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPage_SetUseSystemAppearance)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPage_SetHeaderBannerHeightForTesting)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPage_SetFooterBannerHeightForTesting)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::WebPage_DidEndMagnificationGesture)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_EffectiveAppearanceDidChange)
        return true;
#if PLATFORM(GTK)
    if (messageName == IPC::MessageName::WebPage_ThemeDidChange)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::WebPage_RequestActiveNowPlayingSessionInfo)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_SetShouldDispatchFakeMouseMoveEvents)
        return true;
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_PlaybackTargetSelected)
        return true;
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_PlaybackTargetAvailabilityDidChange)
        return true;
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_SetShouldPlayToPlaybackTarget)
        return true;
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::WebPage_PlaybackTargetPickerWasDismissed)
        return true;
#endif
#if ENABLE(POINTER_LOCK)
    if (messageName == IPC::MessageName::WebPage_DidAcquirePointerLock)
        return true;
#endif
#if ENABLE(POINTER_LOCK)
    if (messageName == IPC::MessageName::WebPage_DidNotAcquirePointerLock)
        return true;
#endif
#if ENABLE(POINTER_LOCK)
    if (messageName == IPC::MessageName::WebPage_DidLosePointerLock)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_clearWheelEventTestMonitor)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetShouldScaleViewToFitDocument)
        return true;
#if ENABLE(VIDEO) && USE(GSTREAMER)
    if (messageName == IPC::MessageName::WebPage_DidEndRequestInstallMissingMediaPlugins)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_SetUserInterfaceLayoutDirection)
        return true;
    if (messageName == IPC::MessageName::WebPage_DidGetLoadDecisionForIcon)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetUseIconLoadingClient)
        return true;
#if ENABLE(GAMEPAD)
    if (messageName == IPC::MessageName::WebPage_GamepadActivity)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_FrameBecameRemote)
        return true;
    if (messageName == IPC::MessageName::WebPage_RegisterURLSchemeHandler)
        return true;
    if (messageName == IPC::MessageName::WebPage_URLSchemeTaskDidPerformRedirection)
        return true;
    if (messageName == IPC::MessageName::WebPage_URLSchemeTaskDidReceiveResponse)
        return true;
    if (messageName == IPC::MessageName::WebPage_URLSchemeTaskDidReceiveData)
        return true;
    if (messageName == IPC::MessageName::WebPage_URLSchemeTaskDidComplete)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetIsSuspended)
        return true;
#if ENABLE(ATTACHMENT_ELEMENT)
    if (messageName == IPC::MessageName::WebPage_InsertAttachment)
        return true;
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    if (messageName == IPC::MessageName::WebPage_UpdateAttachmentAttributes)
        return true;
#endif
#if ENABLE(ATTACHMENT_ELEMENT)
    if (messageName == IPC::MessageName::WebPage_UpdateAttachmentIcon)
        return true;
#endif
#if ENABLE(APPLICATION_MANIFEST)
    if (messageName == IPC::MessageName::WebPage_GetApplicationManifest)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_SetDefersLoading)
        return true;
    if (messageName == IPC::MessageName::WebPage_UpdateCurrentModifierState)
        return true;
    if (messageName == IPC::MessageName::WebPage_SimulateDeviceOrientationChange)
        return true;
#if ENABLE(SPEECH_SYNTHESIS)
    if (messageName == IPC::MessageName::WebPage_SpeakingErrorOccurred)
        return true;
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    if (messageName == IPC::MessageName::WebPage_BoundaryEventOccurred)
        return true;
#endif
#if ENABLE(SPEECH_SYNTHESIS)
    if (messageName == IPC::MessageName::WebPage_VoicesDidChange)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_SetCanShowPlaceholder)
        return true;
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::WebPage_WasLoadedWithDataTransferFromPrevalentResource)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::WebPage_ClearLoadedThirdPartyDomains)
        return true;
#endif
#if ENABLE(RESOURCE_LOAD_STATISTICS)
    if (messageName == IPC::MessageName::WebPage_LoadedThirdPartyDomains)
        return true;
    if (messageName == IPC::MessageName::WebPage_LoadedThirdPartyDomainsReply)
        return true;
#endif
#if USE(SYSTEM_PREVIEW)
    if (messageName == IPC::MessageName::WebPage_SystemPreviewActionTriggered)
        return true;
#endif
#if PLATFORM(GTK) || PLATFORM(WPE)
    if (messageName == IPC::MessageName::WebPage_SendMessageToWebExtension)
        return true;
#endif
#if PLATFORM(GTK) || PLATFORM(WPE)
    if (messageName == IPC::MessageName::WebPage_SendMessageToWebExtensionWithReply)
        return true;
    if (messageName == IPC::MessageName::WebPage_SendMessageToWebExtensionWithReplyReply)
        return true;
#endif
    if (messageName == IPC::MessageName::WebPage_StartTextManipulations)
        return true;
    if (messageName == IPC::MessageName::WebPage_StartTextManipulationsReply)
        return true;
    if (messageName == IPC::MessageName::WebPage_CompleteTextManipulation)
        return true;
    if (messageName == IPC::MessageName::WebPage_CompleteTextManipulationReply)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetOverriddenMediaType)
        return true;
    if (messageName == IPC::MessageName::WebPage_GetProcessDisplayName)
        return true;
    if (messageName == IPC::MessageName::WebPage_GetProcessDisplayNameReply)
        return true;
    if (messageName == IPC::MessageName::WebPage_UpdateCORSDisablingPatterns)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetShouldFireEvents)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetNeedsDOMWindowResizeEvent)
        return true;
    if (messageName == IPC::MessageName::WebPage_SetHasResourceLoadClient)
        return true;
    if (messageName == IPC::MessageName::StorageAreaMap_DidSetItem)
        return true;
    if (messageName == IPC::MessageName::StorageAreaMap_DidRemoveItem)
        return true;
    if (messageName == IPC::MessageName::StorageAreaMap_DidClear)
        return true;
    if (messageName == IPC::MessageName::StorageAreaMap_DispatchStorageEvent)
        return true;
    if (messageName == IPC::MessageName::StorageAreaMap_ClearCache)
        return true;
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::ViewGestureController_DidCollectGeometryForMagnificationGesture)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::ViewGestureController_DidCollectGeometryForSmartMagnificationGesture)
        return true;
#endif
#if !PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::ViewGestureController_DidHitRenderTreeSizeThreshold)
        return true;
#endif
#if PLATFORM(COCOA)
    if (messageName == IPC::MessageName::ViewGestureGeometryCollector_CollectGeometryForSmartMagnificationGesture)
        return true;
#endif
#if PLATFORM(MAC)
    if (messageName == IPC::MessageName::ViewGestureGeometryCollector_CollectGeometryForMagnificationGesture)
        return true;
#endif
#if !PLATFORM(IOS_FAMILY)
    if (messageName == IPC::MessageName::ViewGestureGeometryCollector_SetRenderTreeSizeNotificationThreshold)
        return true;
#endif
    if (messageName == IPC::MessageName::WrappedAsyncMessageForTesting)
        return true;
    if (messageName == IPC::MessageName::SyncMessageReply)
        return true;
    if (messageName == IPC::MessageName::InitializeConnection)
        return true;
    if (messageName == IPC::MessageName::LegacySessionState)
        return true;
    return false;
};

} // namespace IPC
