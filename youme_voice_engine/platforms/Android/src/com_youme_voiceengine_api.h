/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_youme_voiceengine_api */

#ifndef _Included_com_youme_voiceengine_api
#define _Included_com_youme_voiceengine_api
#ifdef __cplusplus
extern "C" {
#endif
#undef com_youme_voiceengine_api_YOUME_RTC_BGM_TO_SPEAKER
#define com_youme_voiceengine_api_YOUME_RTC_BGM_TO_SPEAKER 0L
#undef com_youme_voiceengine_api_YOUME_RTC_BGM_TO_MIC
#define com_youme_voiceengine_api_YOUME_RTC_BGM_TO_MIC 1L
/*
 * Class:     com_youme_voiceengine_api
 * Method:    setExternalInputMode
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setExternalInputMode
  (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    init
 * Signature: (Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_init
  (JNIEnv *, jclass, jstring, jstring, jint, jstring);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    unInit
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_unInit
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setToken
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setToken
  (JNIEnv *, jclass, jstring);
  
/*
 * Class:     com_youme_voiceengine_api
 * Method:    setTokenV3
 * Signature: (Ljava/lang/String;J)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setTokenV3
  (JNIEnv *, jclass, jstring, jlong);
  
/*
 * Class:     com_youme_voiceengine_api
 * Method:    setTCPMode
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setTCPMode
  (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setUserLogPath
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setUserLogPath
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setOutputToSpeaker
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setOutputToSpeaker
  (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setSpeakerMute
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setSpeakerMute
  (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    getSpeakerMute
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_getSpeakerMute
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    getMicrophoneMute
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_getMicrophoneMute
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setMicrophoneMute
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setMicrophoneMute
  (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setAutoSendStatus
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setAutoSendStatus
  (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    getVolume
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_getVolume
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVolume
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setVolume
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    getUseMobileNetworkEnabled
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_getUseMobileNetworkEnabled
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setUseMobileNetworkEnabled
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setUseMobileNetworkEnabled
  (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setLocalConnectionInfo
 * Signature: (Ljava/lang/String;I;Ljava/lang/String;I)I
 */
JNIEXPORT int JNICALL Java_com_youme_voiceengine_api_setLocalConnectionInfo
(JNIEnv *env, jclass className, jstring, jint, jstring, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    clearLocalConnectionInfo
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_clearLocalConnectionInfo
(JNIEnv *env, jclass className);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setRouteChangeFlag
 * Signature: (I)I
 */
JNIEXPORT int JNICALL Java_com_youme_voiceengine_api_setRouteChangeFlag
(JNIEnv *env, jclass className, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    joinChannelSingleMode
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_joinChannelSingleMode
  (JNIEnv *, jclass, jstring, jstring, jint, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    joinChannelMultiMode
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_joinChannelMultiMode
  (JNIEnv *, jclass, jstring, jstring, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    joinChannelSingleModeWithAppKey
 * Signature: (Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_joinChannelSingleModeWithAppKey
  (JNIEnv *, jclass, jstring, jstring, jint, jstring, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    speakToChannel
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_speakToChannel
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    leaveChannelMultiMode
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_leaveChannelMultiMode
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    leaveChannelAll
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_leaveChannelAll
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    getChannelUserList
 * Signature: (Ljava/lang/String;IZ)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_getChannelUserList
  (JNIEnv *, jclass, jstring, jint, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setOtherMicMute
 * Signature: (Ljava/lang/String;Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setOtherMicMute
  (JNIEnv *, jclass, jstring, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setOtherSpeakerMute
 * Signature: (Ljava/lang/String;Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setOtherSpeakerMute
  (JNIEnv *, jclass, jstring, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setListenOtherVoice
 * Signature: (Ljava/lang/String;Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setListenOtherVoice
  (JNIEnv *, jclass, jstring, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setServerRegion
 * Signature: (ILjava/lang/String;Z)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setServerRegion
  (JNIEnv *, jclass, jint, jstring, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    playBackgroundMusic
 * Signature: (Ljava/lang/String;Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_playBackgroundMusic
  (JNIEnv *, jclass, jstring, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    pauseBackgroundMusic
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_pauseBackgroundMusic
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    resumeBackgroundMusic
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_resumeBackgroundMusic
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    stopBackgroundMusic
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_stopBackgroundMusic
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setBackgroundMusicVolume
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setBackgroundMusicVolume
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setHeadsetMonitorOn
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setHeadsetMonitorOn__Z
  (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setHeadsetMonitorOn
 * Signature: (ZZ)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setHeadsetMonitorOn__ZZ
  (JNIEnv *, jclass, jboolean, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setReverbEnabled
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setReverbEnabled
  (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    pauseChannel
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_pauseChannel
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    resumeChannel
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_resumeChannel
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVadCallbackEnabled
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVadCallbackEnabled
  (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setRecordingTimeMs
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setRecordingTimeMs
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setPlayingTimeMs
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setPlayingTimeMs
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setMicLevelCallback
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setMicLevelCallback
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setFarendVoiceLevelCallback
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setFarendVoiceLevelCallback
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setReleaseMicWhenMute
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setReleaseMicWhenMute
  (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setExitCommModeWhenHeadsetPlugin
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setExitCommModeWhenHeadsetPlugin
  (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    getSdkInfo
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_youme_voiceengine_api_getSdkInfo
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    createRender
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_createRender
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    deleteRender
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_deleteRender
  (JNIEnv *, jclass, jint);
    
/*
 * Class:     com_youme_voiceengine_api
 * Method:    deleteRender
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_deleteRenderByUserID
    (JNIEnv * env, jclass className, jstring userId);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoCallback
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoCallback
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    requestRestApi
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_requestRestApi
  (JNIEnv *, jclass, jstring, jstring);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setGrabMicOption
 * Signature: (Ljava/lang/String;IIII)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setGrabMicOption
  (JNIEnv *, jclass, jstring, jint, jint, jint, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    startGrabMicAction
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_startGrabMicAction
  (JNIEnv *, jclass, jstring, jstring);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    stopGrabMicAction
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_stopGrabMicAction
  (JNIEnv *, jclass, jstring, jstring);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    requestGrabMic
 * Signature: (Ljava/lang/String;IZLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_requestGrabMic
  (JNIEnv *, jclass, jstring, jint, jboolean, jstring);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    releaseGrabMic
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_releaseGrabMic
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setInviteMicOption
 * Signature: (Ljava/lang/String;II)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setInviteMicOption
  (JNIEnv *, jclass, jstring, jint, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    requestInviteMic
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_requestInviteMic
  (JNIEnv *, jclass, jstring, jstring, jstring);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    responseInviteMic
 * Signature: (Ljava/lang/String;ZLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_responseInviteMic
  (JNIEnv *, jclass, jstring, jboolean, jstring);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    stopInviteMic
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_stopInviteMic
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    sendMessage
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_sendMessage
  (JNIEnv *, jclass, jstring, jstring);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    sendMessageToUser
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_sendMessageToUser
  (JNIEnv *, jclass, jstring, jstring, jstring);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    kickOtherFromChannel
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_kickOtherFromChannel
  (JNIEnv *, jclass, jstring, jstring, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setLogLevel
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setLogLevel
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setExternalInputSampleRate
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setExternalInputSampleRate
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setAudioQuality
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setAudioQuality
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoSmooth
 * Signature: (I)V
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoSmooth
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoNetAdjustmode
 * Signature: (I)V
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoNetAdjustmode
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoNetResolution
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoNetResolution
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoNetResolutionForSecond
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoNetResolutionForSecond
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoNetResolutionForShare
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoNetResolutionForShare
  (JNIEnv *, jclass, jint, jint);


/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoCodeBitrate
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setVideoCodeBitrate
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoCodeBitrateForSecond
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setVideoCodeBitrateForSecond
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoCodeBitrateForShare
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setVideoCodeBitrateForShare
  (JNIEnv *, jclass, jint, jint);


/*
 * Class:     com_youme_voiceengine_api
 * Method:    getCurrentVideoCodeBitrate
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_getCurrentVideoCodeBitrate
  (JNIEnv *, jclass);
    
/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVBR
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVBR
    (JNIEnv *, jclass, jboolean);
    
/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVBRForSecond
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVBRForSecond
    (JNIEnv *, jclass, jboolean);
    
/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVBRForShare
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVBRForShare
    (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoHardwareCodeEnable
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setVideoHardwareCodeEnable
  (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    getVideoHardwareCodeEnable
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_getVideoHardwareCodeEnable
  (JNIEnv *, jclass);
    
/*
 * Class:     com_youme_voiceengine_api
 * Method:    getVideoHardwareCodeEnable
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_getUseGL
(JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoNoFrameTimeout
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setVideoNoFrameTimeout
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setAVStatisticInterval
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setAVStatisticInterval
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    openVideoEncoder
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_openVideoEncoder
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoLocalResolution
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoLocalResolution
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setCaptureFrontCameraEnable
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setCaptureFrontCameraEnable
  (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoPreviewFps
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoPreviewFps
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoFps
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoFps
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoFpsForSecond
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoFpsForSecond
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoFpsForShare
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoFpsForShare
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setUserRole
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setUserRole
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    getUserRole
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_getUserRole
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    isInChannel
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_isInChannel
  (JNIEnv *, jclass, jstring);
    
/*
 * Class:     com_youme_voiceengine_api
 * Method:    isInChannel
 * Signature: (Ljava/lang)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_isJoined
(JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    isBackgroundMusicPlaying
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_isBackgroundMusicPlaying
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    isInited
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_isInited
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    queryUsersVideoInfo
 * Signature: ([Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_queryUsersVideoInfo
  (JNIEnv *, jclass, jobjectArray);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setUsersVideoInfo
 * Signature: ([Ljava/lang/String;[I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setUsersVideoInfo
  (JNIEnv *, jclass, jobjectArray, jintArray);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setExternalFilterEnabled
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setExternalFilterEnabled
  (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoFrameRawCbEnabled
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoFrameRawCbEnabled
  (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoDecodeRawCbEnabled
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoDecodeRawCbEnabled
  (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    isCameraZoomSupported
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_isCameraZoomSupported
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setCameraZoomFactor
 * Signature: (F)F
 */
JNIEXPORT jfloat JNICALL Java_com_youme_voiceengine_api_setCameraZoomFactor
  (JNIEnv *, jclass, jfloat);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    isCameraFocusPositionInPreviewSupported
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_isCameraFocusPositionInPreviewSupported
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setCameraFocusPositionInPreview
 * Signature: (FF)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_setCameraFocusPositionInPreview
  (JNIEnv *, jclass, jfloat, jfloat);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    isCameraTorchSupported
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_isCameraTorchSupported
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setCameraTorchOn
 * Signature: (Z)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_setCameraTorchOn
  (JNIEnv *, jclass, jboolean);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    isCameraAutoFocusFaceModeSupported
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_isCameraAutoFocusFaceModeSupported
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setCameraAutoFocusFaceModeEnabled
 * Signature: (Z)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_setCameraAutoFocusFaceModeEnabled
  (JNIEnv *, jclass, jboolean);

  
/*
 * Class:     com_youme_voiceengine_api
 * Method:    startPush
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_startPush
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    stopPush
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_stopPush
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setPushMix
 * Signature: (Ljava/lang/String;II)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setPushMix
  (JNIEnv *, jclass, jstring, jint, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    clearPushMix
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_clearPushMix
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    addPushMixUser
 * Signature: (Ljava/lang/String;IIIII)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_addPushMixUser
  (JNIEnv *, jclass, jstring, jint, jint, jint, jint, jint);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    removePushMixUser
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_removePushMixUser
  (JNIEnv *, jclass, jstring);


/*
* Class:     com_youme_voiceengine_api
* Method:    switchResolutionForLandscape
* Signature: ()I
*/
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_switchResolutionForLandscape
(JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setlocalVideoPreviewMirror
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setlocalVideoPreviewMirror
(JNIEnv *env, jclass className, jboolean enable);

/*
* Class:     com_youme_voiceengine_api
* Method:    resetResolutionForPortrait
* Signature: ()I
*/
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_resetResolutionForPortrait
(JNIEnv *, jclass);

/*
* Class:     com_youme_voiceengine_api
* Method:    translateText
* Signature: (Ljava/lang/String;II)I
*/
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_translateText
(JNIEnv *, jclass ,  jstring , int , int );

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setScreenSharedEGLContext
 * Signature: ()Ljava/lang/Object;
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setScreenSharedEGLContext
 (JNIEnv *, jclass,jobject);


#ifdef __cplusplus
}
#endif
#endif
