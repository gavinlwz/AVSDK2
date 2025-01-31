/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_youme_voiceengine_NativeEngine */

#ifndef _Included_com_youme_voiceengine_NativeEngine
#define _Included_com_youme_voiceengine_NativeEngine
#ifdef __cplusplus
extern "C" {
#endif
#undef com_youme_voiceengine_NativeEngine_SERVER_MODE_FORMAL
#define com_youme_voiceengine_NativeEngine_SERVER_MODE_FORMAL 0L
#undef com_youme_voiceengine_NativeEngine_SERVER_MODE_TEST
#define com_youme_voiceengine_NativeEngine_SERVER_MODE_TEST 1L
#undef com_youme_voiceengine_NativeEngine_SERVER_MODE_DEV
#define com_youme_voiceengine_NativeEngine_SERVER_MODE_DEV 2L
#undef com_youme_voiceengine_NativeEngine_SERVER_MODE_DEMO
#define com_youme_voiceengine_NativeEngine_SERVER_MODE_DEMO 3L
#undef com_youme_voiceengine_NativeEngine_SERVER_MODE_FIXED_IP_VALIDATE
#define com_youme_voiceengine_NativeEngine_SERVER_MODE_FIXED_IP_VALIDATE 4L
#undef com_youme_voiceengine_NativeEngine_SERVER_MODE_FIXED_IP_REDIRECT
#define com_youme_voiceengine_NativeEngine_SERVER_MODE_FIXED_IP_REDIRECT 5L
#undef com_youme_voiceengine_NativeEngine_SERVER_MODE_FIXED_IP_MCU
#define com_youme_voiceengine_NativeEngine_SERVER_MODE_FIXED_IP_MCU 6L
#undef com_youme_voiceengine_NativeEngine_SERVER_MODE_FIXED_IP_PRIVATE_SERVICE
#define com_youme_voiceengine_NativeEngine_SERVER_MODE_FIXED_IP_PRIVATE_SERVICE 7L

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setModel
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setModel
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setSysName
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setSysName
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setSysVersion
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setSysVersion
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setVersionName
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setVersionName
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setDeviceURN
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setDeviceURN
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setDeviceIMEI
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setDeviceIMEI
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setCPUArch
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setCPUArch
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setPackageName
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setPackageName
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setUUID
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setUUID
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setDocumentPath
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setDocumentPath
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    onNetWorkChanged
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_onNetWorkChanged
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    onHeadSetPlugin
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_onHeadSetPlugin
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    getSoVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_youme_voiceengine_NativeEngine_getSoVersion
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setBrand
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setBrand
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setCPUChip
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setCPUChip
  (JNIEnv *, jclass, jstring);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setScreenOrientation
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setScreenOrientation
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setServerMode
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setServerMode
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setServerIpPort
 * Signature: (Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setServerIpPort
  (JNIEnv *, jclass, jstring, jint);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    AudioRecorderBufRefresh
 * Signature: (Ljava/nio/ByteBuffer;III)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_AudioRecorderBufRefresh
  (JNIEnv *, jclass, jobject, jint, jint, jint);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    AudioPlayerBufRefresh
 * Signature: (Ljava/nio/ByteBuffer;III)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_AudioPlayerBufRefresh
  (JNIEnv *, jclass, jobject, jint, jint, jint);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    inputAudioFrame
 * Signature: ([BIJ)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_NativeEngine_inputAudioFrame
  (JNIEnv *, jclass, jbyteArray, jint, jlong, jint, jboolean);
  
/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    inputAudioFrameForMix
 * Signature: (I[BILjava/lang/Object;J)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_NativeEngine_inputAudioFrameForMix
  (JNIEnv *, jclass, jint, jbyteArray, jint, jobject, jlong);
    
/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    inputCustomData
 * Signature: ([BIJ)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_NativeEngine_inputCustomData
(JNIEnv *, jclass, jbyteArray, jint, jlong);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    inputCustomDataToUser
 * Signature: ([BIJLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_NativeEngine_inputCustomDataToUser
(JNIEnv *, jclass, jbyteArray, jint, jlong, jstring);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    startCapture
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_NativeEngine_startCapture
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    stopCapture
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_stopCapture
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    switchCamera
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_NativeEngine_switchCamera
  (JNIEnv *, jclass);
    
/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    openBeautify
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_openBeautify
    (JNIEnv *env, jclass className, jboolean open);
    
/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setBeautyLevel
 * Signature: (F)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setBeautyLevel
 (JNIEnv *, jclass, jfloat);
    

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    resetMicrophone
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_resetMicrophone
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    callbackPermissionStatus
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_callbackPermissionStatus
  (JNIEnv *, jclass, jint);
  
/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    resetCamera
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_resetCamera
  (JNIEnv *, jclass);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setPcmCallbackEnable
 * Signature: (IBII)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setPcmCallbackEnable
  (JNIEnv *, jclass, jint, jboolean, jint, jint);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setPcmCallbackEnabled
 * Signature: (ZZ)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setVideoPreDecodeCallbackEnable
  (JNIEnv *, jclass, jboolean, jboolean);
  
/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    maskVideoByUserId
 * Signature: (Ljava/lang/String;Z)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_maskVideoByUserId
  (JNIEnv *, jclass, jstring, jboolean);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    inputVideoFrame
 * Signature: ([BIIIIIIJ)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_NativeEngine_inputVideoFrame
  (JNIEnv *, jclass, jbyteArray, jint, jint, jint, jint, jint, jint, jlong);

  /*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    inputVideoFrameEncrypt
 * Signature: ([BIIIIIIJI)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_NativeEngine_inputVideoFrameEncrypt
  (JNIEnv *, jclass, jbyteArray, jint, jint, jint, jint, jint, jint, jlong, jint);

JNIEXPORT jboolean Java_com_youme_voiceengine_NativeEngine_inputVideoFrameByteBuffer
  (JNIEnv *, jclass , jobject, jint, jint, jint, jint, jint, jint, jlong);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    inputVideoFrameForShare
 * Signature: ([BIIIIIIJ)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_NativeEngine_inputVideoFrameForShare
  (JNIEnv *, jclass, jbyteArray, jint, jint, jint, jint, jint, jint, jlong);

JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_NativeEngine_inputVideoFrameByteBufferForShare
(JNIEnv *, jclass, jobject, jint, jint, jint, jint, jint, jint, jlong);

/*
* Class:     com_youme_voiceengine_NativeEngine
* Method:    inputVideoFrameGLES
* Signature: (I[FIIIIIJ)Z
*/
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_NativeEngine_inputVideoFrameGLES
(JNIEnv *, jclass, jint, jfloatArray, jint, jint, jint, jint, jint, jlong);

/*
* Class:     com_youme_voiceengine_NativeEngine
* Method:    inputVideoFrameGLESForShare
* Signature: (I[FIIIIIJ)Z
*/
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_NativeEngine_inputVideoFrameGLESForShare
(JNIEnv *, jclass, jint, jfloatArray, jint, jint, jint, jint, jint, jlong);

 /*
 * Method:    stopInputVideoFrame
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_stopInputVideoFrame
  (JNIEnv *, jclass);

 /*
 * Method:    stopInputVideoFrameForShare
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_stopInputVideoFrameForShare
  (JNIEnv *, jclass);
    
/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setMixVideoSize
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setMixVideoSize
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    addMixOverlayVideo
 * Signature: (Ljava/lang/String;IIIII)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_addMixOverlayVideo
  (JNIEnv *, jclass, jstring, jint, jint, jint, jint, jint);

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    removeMixOverlayVideo
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_removeMixOverlayVideo
  (JNIEnv *, jclass, jstring);
    
/*
* Class:     com_youme_voiceengine_NativeEngine
* Method:    removeMixAllOverlayVideo
* Signature: ()V
*/
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_removeMixAllOverlayVideo
(JNIEnv *, jclass);

    
/*
 * Class:     com_youme_voiceengine_YouMeEngineAudioMixerUtils
 * Method:    setAudioMixerTrackSamplerate
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_YouMeEngineAudioMixerUtils_setAudioMixerTrackSamplerate
    (JNIEnv *, jclass, jint);
   
/*
 * Class:     com_youme_voiceengine_YouMeEngineAudioMixerUtils
 * Method:    setAudioMixerTrackVolume
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_YouMeEngineAudioMixerUtils_setAudioMixerTrackVolume
    (JNIEnv *, jclass, jint);
    
/*
 * Class:     com_youme_voiceengine_YouMeEngineAudioMixerUtils
 * Method:    setAudioMixerInputVolume
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_YouMeEngineAudioMixerUtils_setAudioMixerInputVolume
    (JNIEnv *, jclass, jint);
    
/*
 * Class:     com_youme_voiceengine_YouMeEngineAudioMixerUtils
 * Method:    pushAudioMixerTrack
 * Signature: ([BIIIIZJ)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_YouMeEngineAudioMixerUtils_pushAudioMixerTrack
  (JNIEnv *, jclass, jbyteArray, jint, jint, jint, jint, jboolean, jlong);

/*
 * Class:     com_youme_voiceengine_YouMeEngineAudioMixerUtils
 * Method:    inputAudioToMix
 * Signature: (Ljava/lang/String;[BIIIIZJ)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_YouMeEngineAudioMixerUtils_inputAudioToMix
  (JNIEnv *, jclass, jstring, jbyteArray, jint, jint, jint, jint, jboolean, jlong);
    
    
/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    sharedEGLContext
 * Signature: ()Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_com_youme_voiceengine_NativeEngine_sharedEGLContext
(JNIEnv *, jclass);
    
/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    logcat
 * Signature: (Ijava/lang/String;java/lang/String;)V;
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_logcat
 (JNIEnv *, jclass, jint, jstring, jstring);
    

#ifdef __cplusplus
}
#endif
#endif
