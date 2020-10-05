//
//  com_youme_voiceengine_NativeEngineJNI.cpp
//  cocos2d-x sdk
//
//  Created by wanglei on 15/8/27.
//  Copyright (c) 2015年 youme. All rights reserved.
//
#include "com_youme_voiceengine_NativeEngine.h"
#include "com_youme_mixers_VideoMixerNative.h"
#include "NgnConfigurationEntry.h"
#include "NgnApplication.h"
#include "NgnNetworkService.h"
#include "NgnEngine.h"
#include "YouMeVoiceEngine.h"
#include "version.h"
#include "AudioMgr.h"
#include "CameraManager.h"
#include "IYouMeVoiceEngine.h"
#include "YouMeEngineManagerForQiniu.hpp"
#include "YouMeEngineAudioMixerUtils.hpp"
#include "tmedia_utils.h"
#include "YouMeVideoMixerAdapter.h"
#include "VideoMixerMgr.h"
#include "webrtc/api/java/jni/classreferenceholder.h"
#include "webrtc/api/java/jni/jni_helpers.h"
#include "webrtc/modules/video_coding/codecs/h264/include/h264.h"
#include <YouMeApplication_Android.h>
#include "VideoMixerDroid.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "tinydav/src/audio/android/audio_android_device_impl.h"
#include "tinydav/src/audio/android/audio_android_producer.h"
#include "tinydav/src/audio/android/audio_android_consumer.h"
#include "tinymedia/include/tinymedia/tmedia_producer.h"
#include "tinymedia/include/tinymedia/tmedia_consumer.h"
    
#include "tinydav/video/android/video_android_device_impl.h"
#include "tinydav/video/android/video_android_producer.h"
#include "tinydav/video/android/video_android_consumer.h"

void JNI_Init_Audio_Record  (int sampleRate, int channelNum, int bytesPerSample, int typeSpeech, tmedia_producer_t *pProducerInstanse);
void JNI_Start_Audio_Record ();
void JNI_Stop_Audio_Record  ();
int  JNI_Get_Audio_Record_Status ();
void JNI_Init_Audio_Player  (int sampleRate, int channelNum, int bytesPerSample, int isStreamVoice, tmedia_consumer_t *pConsumerInstanse);
void JNI_Start_Audio_Player ();
void JNI_Stop_Audio_Player  ();
void JNI_Pause_Audio_Record ();
void JNI_Resume_Audio_Record ();
int  JNI_Get_Audio_Player_Status ();
int  JNI_Is_Wired_HeadsetOn ();

// video jni采集接口
void JNI_Init_Video_Capturer(int nFps,
                             int nWidth,
                             int nHeight,
                             tmedia_producer_t *pProducerInstance);
void JNI_Start_Video_Capturer();
void JNI_Stop_Video_Capturer();

#if 0
// video jni渲染接口
void JNI_Init_Video_Renderer(int nFps,
                             int nWidth,
                             int nHeight,
                             tmedia_consumer_t *pConsumerInstance);
void JNI_Refresh_Video_Render(int sessionId, int nWidth, int nHeight, int nRotationDegree, int nBufSize, const void *buf);
#endif
    
#ifdef __cplusplus
}
#endif
    
extern void SetSoundtouchEnabled(bool bEnabled);
extern bool GetSoundtouchEnabled();
extern float GetSoundtouchTempo();
extern void SetSoundtouchTempo(float nTempo);
extern float GetSoundtouchRate();
extern void SetSoundtouchRate(float nRate);
extern float GetSoundtouchPitch();
extern void SetSoundtouchPitch(float nPitch);

static JavaVM *mJvm                                       = NULL;
static jclass mAudioMgrClass                              = NULL;
static jclass mAudioRecorderClass                         = NULL;
static jclass mAudioPlayerClass                           = NULL;
static jobject mJNIObj                                    = NULL;
static jmethodID mStartVoiceMethodID                      = 0;
static jmethodID mStopVoiceMethodID                       = 0;
static jmethodID mInitAudioSettingsMethodID               = 0;
static jmethodID mStartRequestPermissionForApi23MethodID  = 0;
static jmethodID mStopRequestPermissionForApi23MethodID   = 0;
static jmethodID mStartRequestPermissionForApi23MethodID_camera  = 0;
static jmethodID mStopRequestPermissionForApi23MethodID_camera   = 0;
static jmethodID mAudioRecorderMethodID                   = 0;
static jmethodID mAudioSetIsMuteMethodID                  = 0;
static jmethodID mAudioInitRecorderMethodID               = 0;
static jmethodID mAudioRecorderStatusMethodID             = 0;
static jmethodID mAudioPlayerMethodID                     = 0;
static jmethodID mAudioInitPlayerMethodID                 = 0;
static jmethodID mAudioPlayerStatusMethodID               = 0;
static jmethodID mAudioRecorderPauseMethodID              = 0;
static jmethodID mAudioMgrIsWiredHeadsetOnMethodID        = 0;


//static jclass mAudioCommonCallbackClass           = NULL;
//static jmethodID mOnCommonCallbackID              = 0;
//static jclass mAudioConferenceCallbackClass       = NULL;
//static jmethodID mOnConferenceCallbackID          = 0;
//static jmethodID mOnCommonEventStatusCallbackID   = 0;
//static jmethodID mOnReciveControllMsgCallbackID = 0;
//static jmethodID mOnMemberChangeMsgCallbackID     = 0;
static jclass mAudioEventCallbackClass            = NULL;
static jmethodID mOnEventCallbackID               = 0;
static jmethodID mOnEventByteCallbackID             = 0;
static jmethodID mOnPcmCallbackRemoteID               = 0;
static jmethodID mOnPcmCallbackRecordID               = 0;
static jmethodID mOnPcmCallbackMixID               = 0;
static jmethodID mOnRestApiCallbackID           = 0;
static jmethodID mOnMemberChangeCallbackID      = 0;
static jmethodID mOnRecvCustomDataCallbackID      = 0;
static jclass mMemberChangeClass = NULL;
static jmethodID mOnBroadcastCallbackID      	= 0;
static jmethodID mOnAVStatisticCallbackID       = 0;
static jmethodID mOnTranslateCallbackID         = 0 ;

static jclass mVideoFrameCallbackClass            = NULL;
static jmethodID mOnVideoFrameCallbackID          = 0;
static jmethodID mOnVideoFrameMixedCallbackID     = 0;
static jmethodID mOnVideoFrameCallbackGLESID          = 0;
static jmethodID mOnVideoFrameMixedCallbackGLESID     = 0;
static jmethodID mOnVideoFilterExtCallbackGLESID     = 0;
static jmethodID mOnPreDecodeCallbackID             = 0;

static jclass mAudioFrameCallbackClass            = NULL;
static jmethodID mOnAudioFrameCallbackID          = 0;
static jmethodID mOnAudioFrameMixedCallbackID     = 0;

static jclass mYouMeManagerClass                  = NULL;
static jmethodID mUpdateSelfMethodID              = 0;
static jmethodID mTriggerNetChangeMethodID        = 0;
static jmethodID mSaveLogcatMethoidID             = 0;
static tmedia_producer_t *m_pProducer             = NULL;
static tmedia_consumer_t *m_pConsumer             = NULL;


// video jni定义
static jclass mVideoCapturerClass                 = NULL;
static jclass mVideoRendererClass                 = NULL;
static jmethodID mVideoCapturerMethodID           = 0;
static jmethodID mVideoSetCamera                  = 0;
static jmethodID mVideoSetCameraFps               = 0;
static jmethodID mVideoSetCameraRotation          = 0;
static jmethodID mVideoFrameRender                = 0;
static jmethodID mVideoSetFrontCamera             = 0;

static jclass mVideoEventCallbackClass            = NULL;
static jmethodID mOnVideoEventCallbackID          = 0;
static tmedia_producer_t *m_pVideoProducer        = NULL;
static tmedia_consumer_t *m_pVideoConsumer        = NULL;

// camera jni define
static jclass mCameraMgrClass                     = NULL;
static jmethodID mStartCaptureMethodID            = NULL;
static jmethodID mStopCaptureMethodID             = NULL;
static jmethodID mSwitchCameraMethodID            = NULL;
// static jmethodID mSetBeautyLevelMethodID          = NULL;
// static jmethodID mOpenBeautifyMethodID            = NULL;
static jmethodID mIsCameraZoomSupportedMethodID   = NULL;
static jmethodID mSetCameraZoomFactorMethodID                               = NULL;
static jmethodID mIsCameraFocusPositionInPreviewSupportedMethodID           = NULL;
static jmethodID mSetCameraFocusPositionInPreviewMethodID                   = NULL;
static jmethodID mIsCameraTorchSupportedMethodID                            = NULL;
static jmethodID mSetCameraTorchOnMethodID                                  = NULL;
static jmethodID mIsCameraAutoFocusFaceModeSupportedMethodID                = NULL;
static jmethodID mSetCameraAutoFocusFaceModeEnabledMethodID                 = NULL;
static jmethodID mSetLocalVideoMirrorModeMethodID                           = NULL;

//video mixer
static jclass mVideoMixerHelperClass                                   = NULL;
static jmethodID mInitMixerMethodID                                    = NULL;
static jmethodID mSetVideoMixEGLContextMethodID                        = NULL;
static jmethodID mGetVideoMixEGLContextMethodID                        = NULL;
static jmethodID mSetVideoMixSizeMethodID                              = NULL;
static jmethodID mSetVideoNetResolutionWidthMethodID                   = NULL;
static jmethodID mSetVideoNetResolutionWidthForSecondMethodID          = NULL;
static jmethodID mAddMixOverlayVideoMethodID                           = NULL;
static jmethodID mRemoveMixOverlayVideoMethodID                        = NULL;
static jmethodID mRemoveAllOverlayVideoMethodID                        = NULL;


static jmethodID mPushVideoFrameGLESForLocalMethodID                   = NULL;
static jmethodID mPushVideoFrameRawForLocalMethodID                    = NULL;
static jmethodID mPushVideoFrameGLESForRemoteMethodID                  = NULL;
static jmethodID mPushVideoFrameRawForRemoteMethodID                   = NULL;
static jmethodID mMixOpenBeautifyMethodID                              = NULL;
static jmethodID mCloseBeautifyMethodID                                = NULL;
static jmethodID mSetBeautifyMethodID                                  = NULL;
static jmethodID mOnPauseMethodID                                      = NULL;
static jmethodID mOnResumeMethodID                                     = NULL;
static jmethodID mMixExitMethodID                                      = NULL;
static jmethodID mSetVideoFrameMixCallbackMethodID                     = NULL;
static jmethodID mSetVideoFpsMethodID                                  = NULL;

static IAndroidVideoMixerCallBack*  VideoMixerCallBack    = NULL;


static jclass mJavaApiClass                                            = NULL;
static jmethodID mSetOpenMixerRawCallBackMethodID                      = NULL;
static jmethodID mSetOpenEncoderRawCallBackMethodID                    = NULL;
static jmethodID mSetExternalFilterEnabledMethodID                     = NULL;

//screen recorder
static jclass mScreenRecorderClass                                     = NULL;
static jmethodID mSetResolutionMethodID                                = NULL;
static jmethodID mSetFpsMethodID                                       = NULL;

YouMeApplication_Android* g_AndroidSystemProvider = new YouMeApplication_Android();


// char* to jstring
jstring string2jstring (JNIEnv *env, const char *str)
{
    jstring rtstr = env->NewStringUTF (str);
    return rtstr;
}

class JNIEvnWrap
{
public:
    JNIEvnWrap()
    {
        m_pThreadJni = NULL;
        m_bAttachThread = false;
        if (NULL == mJvm) {
            return;
        }
        
        if (mJvm->GetEnv ((void **)&m_pThreadJni, JNI_VERSION_1_4) != JNI_OK)
        {
            int status = mJvm->AttachCurrentThread (&m_pThreadJni, 0);
            if (status >= 0)
            {
                m_bAttachThread = true;
            }
        }
        
    }
    ~JNIEvnWrap()
    {
        if ((NULL != m_pThreadJni) && m_bAttachThread)
        {
            mJvm->DetachCurrentThread ();
        }
    }
    
    JNIEnv* m_pThreadJni;
    bool m_bAttachThread;
};


class YouMeEventCallback:public IYouMeEventCallback, public IRestApiCallback, public IYouMeMemberChangeCallback, public IYouMeChannelMsgCallback, public IYouMeAVStatisticCallback
,public IYouMeCustomDataCallback, public IYouMeTranslateCallback
{
public:
    void onEvent (const YouMeEvent_t eventType, const YouMeErrorCode iErrorCode, const char * channel, const char * param)
    {
        JNIEvnWrap jniWrap;
        if (NULL == jniWrap.m_pThreadJni)
        {
            return;
        }
        
        if( eventType != YOUME_EVENT_MESSAGE_NOTIFY )
        {
            jniWrap.m_pThreadJni->CallStaticVoidMethod (mAudioEventCallbackClass, mOnEventCallbackID, eventType,iErrorCode, string2jstring(jniWrap.m_pThreadJni, channel) , string2jstring(jniWrap.m_pThreadJni, param));
        
        }
        else{
            std::string strParam = param;
            jbyteArray jArray = jniWrap.m_pThreadJni->NewByteArray( strParam.length() );
            jniWrap.m_pThreadJni->SetByteArrayRegion(jArray, 0, strParam.length(), (const jbyte *)param);
                
            jniWrap.m_pThreadJni->CallStaticVoidMethod (mAudioEventCallbackClass, mOnEventByteCallbackID, eventType,iErrorCode, string2jstring(jniWrap.m_pThreadJni, channel) , jArray);
            jniWrap.m_pThreadJni->DeleteLocalRef(jArray);
        }    
    }
    
    void onRequestRestAPI( int requestID, const YouMeErrorCode &iErrorCode, const char* strQuery, const char*  strResult ) {
        JNIEvnWrap jniWrap;
        if (NULL == jniWrap.m_pThreadJni)
        {
            return;
        }
        
        jniWrap.m_pThreadJni->CallStaticVoidMethod (mAudioEventCallbackClass, mOnRestApiCallbackID, requestID,iErrorCode,
                                                    string2jstring(jniWrap.m_pThreadJni, strQuery) , string2jstring(jniWrap.m_pThreadJni, strResult ));
    }
    
    
   
    void onMemberChange( const char* channel, const char* listMemberChange , bool isUpdate ) {
        JNIEvnWrap jniWrap;
        if (NULL == jniWrap.m_pThreadJni)
        {
            return;
        }
        /*
        JNIEnv* env = jniWrap.m_pThreadJni;
        
        int nCount = listMemberChange.size();
        
        jobjectArray membs = NULL;
        jsize len = nCount;
        membs = (env)->NewObjectArray(len, mMemberChangeClass, NULL );
        jfieldID nameID = (env)->GetFieldID(mMemberChangeClass, "userID", "Ljava/lang/String;");
        jfieldID isJoinID = (env)->GetFieldID(mMemberChangeClass, "isJoin", "Z");
        jmethodID  consID = (env)->GetMethodID(mMemberChangeClass, "<init>", "()V" );
        
        int i = 0;
        for( auto it = listMemberChange.begin() ; it != listMemberChange.end(); ++it  ){
            jobject obj = env->NewObject( mMemberChangeClass, consID );
            (env)->SetObjectField(obj, nameID, string2jstring( env, it->userID ));
            (env)->SetBooleanField(obj, isJoinID, jboolean( it->isJoin ) );
            
            //添加到objcet数组中
            (env)->SetObjectArrayElement(membs, i, obj);
            
            i++;
        }

        jniWrap.m_pThreadJni->CallStaticVoidMethod (mAudioEventCallbackClass, mOnMemberChangeCallbackID,
                                                    string2jstring(jniWrap.m_pThreadJni, channel ),
                                                    membs, jboolean( isUpdate )
                                                   );
         */
        jniWrap.m_pThreadJni->CallStaticVoidMethod (mAudioEventCallbackClass, mOnMemberChangeCallbackID,
                                                    string2jstring(jniWrap.m_pThreadJni, channel ),
                                                    string2jstring(jniWrap.m_pThreadJni, listMemberChange ),
                                                    isUpdate
                                                    );
    }

    void onBroadcast(const YouMeBroadcast bc, const char* channel,
    		const char* param1, const char* param2, const char* strContent){

        JNIEvnWrap jniWrap;
        if (NULL == jniWrap.m_pThreadJni)
        {
            return;
        }

        jniWrap.m_pThreadJni->CallStaticVoidMethod (mAudioEventCallbackClass, mOnBroadcastCallbackID, bc, string2jstring(jniWrap.m_pThreadJni, channel) ,
        		string2jstring(jniWrap.m_pThreadJni, param1), string2jstring(jniWrap.m_pThreadJni, param2), string2jstring(jniWrap.m_pThreadJni, strContent));
    }
    
    void onAVStatistic( YouMeAVStatisticType type,  const char* userID,  int value )
    {
        JNIEvnWrap jniWrap;
        if (NULL == jniWrap.m_pThreadJni)
        {
            return;
        }
        
        jniWrap.m_pThreadJni->CallStaticVoidMethod (mAudioEventCallbackClass, mOnAVStatisticCallbackID,
                                                    type,
                                                    string2jstring(jniWrap.m_pThreadJni, userID) ,
                                                    value);
        
    }

    virtual void OnCustomDataNotify(const void*data, int len, unsigned long long timestamp)
    {
         JNIEvnWrap jniWrap;
        if (NULL == jniWrap.m_pThreadJni)
        {
            return;
        }
        jbyteArray jArray = jniWrap.m_pThreadJni->NewByteArray(len);
        jniWrap.m_pThreadJni->SetByteArrayRegion(jArray, 0, len, (const jbyte *)data);
        jniWrap.m_pThreadJni->CallStaticVoidMethod(mAudioEventCallbackClass, mOnRecvCustomDataCallbackID,  jArray,  (jlong)timestamp);
        jniWrap.m_pThreadJni->DeleteLocalRef(jArray);
    }

    virtual void onTranslateTextComplete(YouMeErrorCode errorcode, unsigned int requestID, const std::string& text, YouMeLanguageCode srcLangCode, YouMeLanguageCode destLangCode)
    {
        JNIEvnWrap jniWrap;
        if (NULL == jniWrap.m_pThreadJni)
        {
            return;
        }
        
        jniWrap.m_pThreadJni->CallStaticVoidMethod (mAudioEventCallbackClass, mOnTranslateCallbackID,
                                                    errorcode,
                                                    requestID,
                                                    string2jstring(jniWrap.m_pThreadJni, text.c_str()) ,
                                                    srcLangCode, 
                                                    destLangCode);
    }
};

class YouMePcmCallback: public IYouMePcmCallback
{
public:
    void onPcmDataRemote(int channelNum, int samplingRateHz, int bytesPerSample, void* data, int dataSizeInByte)
    {
        JNIEvnWrap jniWrap;
        if (NULL == jniWrap.m_pThreadJni)
        {
            return;
        }
        
        jbyteArray jArray = jniWrap.m_pThreadJni->NewByteArray(dataSizeInByte);
        jniWrap.m_pThreadJni->SetByteArrayRegion(jArray, 0, dataSizeInByte, (const jbyte *)data);
        
        jniWrap.m_pThreadJni->CallStaticVoidMethod(mAudioEventCallbackClass, mOnPcmCallbackRemoteID, channelNum, samplingRateHz, bytesPerSample, jArray);
        jniWrap.m_pThreadJni->DeleteLocalRef(jArray);
    }
    
    void onPcmDataRecord(int channelNum, int samplingRateHz, int bytesPerSample, void* data, int dataSizeInByte)
    {
        JNIEvnWrap jniWrap;
        if (NULL == jniWrap.m_pThreadJni)
        {
            return;
        }
        
        jbyteArray jArray = jniWrap.m_pThreadJni->NewByteArray(dataSizeInByte);
        jniWrap.m_pThreadJni->SetByteArrayRegion(jArray, 0, dataSizeInByte, (const jbyte *)data);
        
        jniWrap.m_pThreadJni->CallStaticVoidMethod(mAudioEventCallbackClass, mOnPcmCallbackRecordID, channelNum, samplingRateHz, bytesPerSample, jArray);
        jniWrap.m_pThreadJni->DeleteLocalRef(jArray);
    }
    
    void onPcmDataMix(int channelNum, int samplingRateHz, int bytesPerSample, void* data, int dataSizeInByte)
    {
        TMEDIA_I_AM_ACTIVE(500, "onPcmDataMix");
        JNIEvnWrap jniWrap;
        if (NULL == jniWrap.m_pThreadJni)
        {
            return;
        }
        
        jbyteArray jArray = jniWrap.m_pThreadJni->NewByteArray(dataSizeInByte);
        jniWrap.m_pThreadJni->SetByteArrayRegion(jArray, 0, dataSizeInByte, (const jbyte *)data);
        
        jniWrap.m_pThreadJni->CallStaticVoidMethod(mAudioEventCallbackClass, mOnPcmCallbackMixID, channelNum, samplingRateHz, bytesPerSample, jArray);
        jniWrap.m_pThreadJni->DeleteLocalRef(jArray);
    }
};

class YouMeVideoCallback : public IYouMeVideoFrameCallback{
    void onVideoFrameCallback(const char*  userId, void * data, int len, int width, int height, int fmt, uint64_t timestamp){
        JNIEvnWrap jniWrap;
        if (NULL == jniWrap.m_pThreadJni)
        {
            return;
        }
       // TMEDIA_I_AM_ACTIVE(100, "JIN onVideoFrameCallback: %lld",timestamp);
        jbyteArray jArray = jniWrap.m_pThreadJni->NewByteArray(len);
        jniWrap.m_pThreadJni->SetByteArrayRegion(jArray, 0, len, (const jbyte *)data);
        jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoFrameCallbackClass, mOnVideoFrameCallbackID, jniWrap.m_pThreadJni->NewStringUTF(userId), jArray, len, width, height, fmt, (jlong)timestamp);
        jniWrap.m_pThreadJni->DeleteLocalRef(jArray);

    }
    void onVideoFrameMixedCallback(void * data, int len, int width, int height, int fmt, uint64_t timestamp){
        JNIEvnWrap jniWrap;
        if (NULL == jniWrap.m_pThreadJni)
        {
            return;
        }
       // TMEDIA_I_AM_ACTIVE(100, "onVideoFrameMixedCallback");
        jbyteArray jArray = jniWrap.m_pThreadJni->NewByteArray(len);
        jniWrap.m_pThreadJni->SetByteArrayRegion(jArray, 0, len, (const jbyte *)data);
        jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoFrameCallbackClass, mOnVideoFrameMixedCallbackID, jArray, len, width, height, fmt, (jlong)timestamp);
        jniWrap.m_pThreadJni->DeleteLocalRef(jArray);
    }

    void onVideoFrameCallbackGLESForAndroid(const char* userId, int textureId, float* matrix, int width, int height, int fmt, uint64_t timestamp){
        JNIEvnWrap jniWrap;
        if (NULL == jniWrap.m_pThreadJni)
        {
            return;
        }
        if(matrix){
            jfloatArray jArray = jniWrap.m_pThreadJni->NewFloatArray(16);
            jniWrap.m_pThreadJni->SetFloatArrayRegion(jArray, 0, 16, (const jfloat *)matrix);
            jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoFrameCallbackClass, mOnVideoFrameCallbackGLESID, jniWrap.m_pThreadJni->NewStringUTF(userId), fmt, textureId, jArray, width, height,  (jlong)timestamp);
            jniWrap.m_pThreadJni->DeleteLocalRef(jArray);
        }
        else{
            jobject jArray;
            jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoFrameCallbackClass, mOnVideoFrameCallbackGLESID, jniWrap.m_pThreadJni->NewStringUTF(userId), fmt, textureId, jArray, width, height,  (jlong)timestamp);
            
        }
    }
    void onVideoFrameMixedCallbackGLESForAndroid(int textureId, float* matrix, int width, int height, int fmt, uint64_t timestamp){
        JNIEvnWrap jniWrap;
        if (NULL == jniWrap.m_pThreadJni)
        {
            return;
        }
        if(matrix){
            jfloatArray jArray = jniWrap.m_pThreadJni->NewFloatArray(16);
            jniWrap.m_pThreadJni->SetFloatArrayRegion(jArray, 0, 16, (const jfloat *)matrix);
            jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoFrameCallbackClass, mOnVideoFrameMixedCallbackGLESID, fmt, textureId, jArray, width, height,  (jlong)timestamp);
            jniWrap.m_pThreadJni->DeleteLocalRef(jArray);
        }
        else{
            jobject jArray;
            jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoFrameCallbackClass, mOnVideoFrameMixedCallbackGLESID,  fmt, textureId, jArray, width, height,  (jlong)timestamp);
            
        }
    }
    
    virtual int  onVideoRenderFilterCallback(int textureId, int width, int height, int rotation, int mirror){
        JNIEvnWrap jniWrap;
        if (NULL == jniWrap.m_pThreadJni)
        {
            return 0;
        }
        return jniWrap.m_pThreadJni->CallStaticIntMethod(mVideoFrameCallbackClass, mOnVideoFilterExtCallbackGLESID,   textureId,  width, height,  rotation, mirror);

    }

};

class YouMeVideoPreDecodeCallback : public IYouMeVideoPreDecodeCallback{
    void onVideoPreDecode(const char* userId, void* data, int dataSizeInByte, unsigned long long timestamp) {
        JNIEvnWrap jniWrap;
        if (NULL == jniWrap.m_pThreadJni)
        {
            return;
        }

        jbyteArray jArray = jniWrap.m_pThreadJni->NewByteArray(dataSizeInByte);
        jniWrap.m_pThreadJni->SetByteArrayRegion(jArray, 0, dataSizeInByte, (const jbyte *)data);
        
        jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoFrameCallbackClass, mOnPreDecodeCallbackID, string2jstring(jniWrap.m_pThreadJni,userId), jArray, dataSizeInByte, (jlong)timestamp);
        jniWrap.m_pThreadJni->DeleteLocalRef(jArray);

    }
};

void JNI_onAudioFrameCallbackID(std::string userId, const void * data, int len, uint64_t timestamp)
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    TMEDIA_I_AM_ACTIVE(500, "onAudioFrameCallback");
    jbyteArray jArray = jniWrap.m_pThreadJni->NewByteArray(len);
    jniWrap.m_pThreadJni->SetByteArrayRegion(jArray, 0, len, (const jbyte *)data);
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mAudioFrameCallbackClass, mOnAudioFrameCallbackID, jniWrap.m_pThreadJni->NewStringUTF(userId.c_str()), jArray, len, (jlong)timestamp);
    jniWrap.m_pThreadJni->DeleteLocalRef(jArray);
}

void JNI_onAudioFrameMixedCallbackID(const void * data, int len, uint64_t timestamp)
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    TMEDIA_I_AM_ACTIVE(500, "onAudioFrameMixedCallback");
    jbyteArray jArray = jniWrap.m_pThreadJni->NewByteArray(len);
    jniWrap.m_pThreadJni->SetByteArrayRegion(jArray, 0, len, (const jbyte *)data);
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mAudioFrameCallbackClass, mOnAudioFrameMixedCallbackID, jArray, len, (jlong)timestamp);
    jniWrap.m_pThreadJni->DeleteLocalRef(jArray);
}

static YouMeEventCallback* mPYouMeEventCallback = NULL;
static YouMePcmCallback*   mPYouMePcmCallback = NULL;
static YouMeVideoCallback* mPYouMeVideoCallback = NULL;
static YouMeVideoPreDecodeCallback* mPYouMeVideoPreDecodeCallback = NULL;

//初始化的时候会调进来一次，在这个方法里持有jvm的引用
JNIEXPORT jint JNICALL JNI_OnLoad (JavaVM *jvm, void *reserved)
{
    mJvm = jvm;
    JNIEnv *pEnv = NULL;
    if (NULL == mJvm)
    {
        return -1;
    }
    if (mJvm->GetEnv ((void **)&pEnv, JNI_VERSION_1_4) != JNI_OK)
    {
        return -1;
    }
    //由于在回调的时候不能使用attached env来findclass，所以在这里获取到class
    jclass audioMgrClass = pEnv->FindClass ("com/youme/voiceengine/AudioMgr");
    if (NULL == audioMgrClass)
    {
        return -1;
    }
    mAudioMgrClass = (jclass)(pEnv->NewGlobalRef (audioMgrClass));
    mStartVoiceMethodID = pEnv->GetStaticMethodID (mAudioMgrClass, "setVoiceModeYouMeCoutum", "()V");
    mStopVoiceMethodID = pEnv->GetStaticMethodID (mAudioMgrClass, "restoreOldMode", "()V");
    mInitAudioSettingsMethodID = pEnv->GetStaticMethodID (mAudioMgrClass, "initAudioSettings", "(Z)V");
    mStartRequestPermissionForApi23MethodID = pEnv->GetStaticMethodID (mAudioMgrClass, "startRequestPermissionForApi23", "()Z");
    mStopRequestPermissionForApi23MethodID = pEnv->GetStaticMethodID (mAudioMgrClass, "stopRequestPermissionForApi23", "()V");
    mAudioMgrIsWiredHeadsetOnMethodID = pEnv->GetStaticMethodID (mAudioMgrClass, "isWiredHeadsetOn", "()I");
    mPYouMeEventCallback = new YouMeEventCallback();
    mPYouMePcmCallback = new YouMePcmCallback();
    mPYouMeVideoCallback = new YouMeVideoCallback();
    mPYouMeVideoPreDecodeCallback = new YouMeVideoPreDecodeCallback();
    
    jclass audioRecorderClass = pEnv->FindClass ("com/youme/voiceengine/AudioRecorder");
    if (NULL == audioRecorderClass)
    {
        return -1;
    }
    mAudioRecorderClass          = (jclass)(pEnv->NewGlobalRef (audioRecorderClass));
    mAudioInitRecorderMethodID   = pEnv->GetStaticMethodID (mAudioRecorderClass, "initRecorder", "(IIII)V");
    mAudioRecorderMethodID       = pEnv->GetStaticMethodID (mAudioRecorderClass, "OnAudioRecorder", "(I)V");
    mAudioSetIsMuteMethodID      = pEnv->GetStaticMethodID (mAudioRecorderClass, "setMicMuteStatus", "(I)V");
    mAudioRecorderPauseMethodID  = pEnv->GetStaticMethodID (mAudioRecorderClass, "OnAudioRecorderTmp", "(I)V");
    mAudioRecorderStatusMethodID = pEnv->GetStaticMethodID (mAudioRecorderClass, "getRecorderInitStatus", "()I");
    
    jclass audioPlayerClass = pEnv->FindClass ("com/youme/voiceengine/AudioPlayer");
    if (NULL == audioPlayerClass)
    {
        return -1;
    }
    mAudioPlayerClass            = (jclass)(pEnv->NewGlobalRef (audioPlayerClass));
    mAudioInitPlayerMethodID     = pEnv->GetStaticMethodID (mAudioPlayerClass, "initPlayer", "(IIIZ)V");
    mAudioPlayerMethodID         = pEnv->GetStaticMethodID (mAudioPlayerClass, "OnAudioPlayer", "(I)V");
    mAudioPlayerStatusMethodID   = pEnv->GetStaticMethodID (mAudioPlayerClass, "getPlayerInitStatus", "()I");
    
    jclass videoCapturerClass = pEnv->FindClass ("com/youme/voiceengine/VideoMgr");
    if (NULL == videoCapturerClass)
    {
        return -1;
    }
    mVideoCapturerClass          = (jclass)(pEnv->NewGlobalRef (videoCapturerClass));
    mVideoSetCamera              = pEnv->GetStaticMethodID (mVideoCapturerClass, "setCamera", "(III)V");
    mVideoSetCameraFps           = pEnv->GetStaticMethodID (mVideoCapturerClass, "setFps", "(I)V");
    mVideoSetCameraRotation      = pEnv->GetStaticMethodID (mVideoCapturerClass, "setRotation", "(I)V");
    mVideoSetFrontCamera         = pEnv->GetStaticMethodID (mVideoCapturerClass, "setFrontCamera", "(Z)V");
    
#if 0
    jclass videoRendererClass = pEnv->FindClass ("com/youme/voiceengine/VideoRenderer");
    if (NULL == videoRendererClass)
    {
        return -1;
    }
    mVideoRendererClass          = (jclass)(pEnv->NewGlobalRef (videoRendererClass));
    mVideoFrameRender            = pEnv->GetStaticMethodID (mVideoRendererClass, "FrameRender", "(IIII[B)V");
#endif
    
    jclass audioEventCallbackClass = pEnv->FindClass ("com/youme/voiceengine/IYouMeEventCallback");
    if (NULL == audioEventCallbackClass)
    {
        return -1;
    }
    mAudioEventCallbackClass = (jclass)(pEnv->NewGlobalRef (audioEventCallbackClass));
    mOnEventCallbackID = pEnv->GetStaticMethodID (mAudioEventCallbackClass, "onEvent", "(IILjava/lang/String;Ljava/lang/String;)V");
    mOnEventByteCallbackID = pEnv->GetStaticMethodID (mAudioEventCallbackClass, "onEventByte", "(IILjava/lang/String;[B)V");
    mOnPcmCallbackRemoteID = pEnv->GetStaticMethodID (mAudioEventCallbackClass, "onPcmDataRemote", "(III[B)V");
    mOnPcmCallbackRecordID = pEnv->GetStaticMethodID (mAudioEventCallbackClass, "onPcmDataRecord", "(III[B)V");
    mOnPcmCallbackMixID = pEnv->GetStaticMethodID (mAudioEventCallbackClass, "onPcmDataMix", "(III[B)V");
 	mOnRestApiCallbackID = pEnv->GetStaticMethodID( mAudioEventCallbackClass, "onRequestRestAPI", "(IILjava/lang/String;Ljava/lang/String;)V");
    mOnMemberChangeCallbackID = pEnv->GetStaticMethodID(mAudioEventCallbackClass, "onMemberChange","(Ljava/lang/String;Ljava/lang/String;Z)V" );
    mOnRecvCustomDataCallbackID = pEnv->GetStaticMethodID(mAudioEventCallbackClass, "onRecvCustomData","([BJ)V" );
    mOnBroadcastCallbackID = pEnv->GetStaticMethodID( mAudioEventCallbackClass, "onBroadcast", "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    mOnAVStatisticCallbackID = pEnv->GetStaticMethodID( mAudioEventCallbackClass, "onAVStatistic", "(ILjava/lang/String;I)V");
    mOnTranslateCallbackID = pEnv->GetStaticMethodID( mAudioEventCallbackClass, "onTranslateTextComplete", "(IILjava/lang/String;II)V");

    TSK_DEBUG_INFO(">>> JNI init VideoCallback set begin");
	jclass clsMemChange = pEnv->FindClass("com/youme/voiceengine/MemberChange");
    if( NULL == clsMemChange ){
        return -1;
    }
    mMemberChangeClass = (jclass)(pEnv->NewGlobalRef( clsMemChange ) );
    
    jclass videoFrameCallbackClass = pEnv->FindClass ("com/youme/voiceengine/IYouMeVideoCallback");
    if (NULL == videoFrameCallbackClass)
    {
        return -1;
    }
    mVideoFrameCallbackClass = (jclass)(pEnv->NewGlobalRef (videoFrameCallbackClass));
    mOnVideoFrameCallbackID = pEnv->GetStaticMethodID (mVideoFrameCallbackClass, "onVideoFrameCallback", "(Ljava/lang/String;[BIIIIJ)V");
    mOnVideoFrameMixedCallbackID = pEnv->GetStaticMethodID (mVideoFrameCallbackClass, "onVideoFrameMixedCallback", "([BIIIIJ)V");
    mOnVideoFrameCallbackGLESID = pEnv->GetStaticMethodID (mVideoFrameCallbackClass, "onVideoFrameCallbackGLES", "(Ljava/lang/String;II[FIIJ)V");
    mOnVideoFrameMixedCallbackGLESID = pEnv->GetStaticMethodID (mVideoFrameCallbackClass, "onVideoFrameMixedCallbackGLES", "(II[FIIJ)V");
    mOnVideoFilterExtCallbackGLESID = pEnv->GetStaticMethodID (mVideoFrameCallbackClass, "onVideoRenderFilterCallback", "(IIIII)I");
    mOnPreDecodeCallbackID = pEnv->GetStaticMethodID (mVideoFrameCallbackClass, "onVideoPreDecode", "(Ljava/lang/String;[BIJ)V");

    TSK_DEBUG_INFO(">>> JNI init VideoCallback set success");
    
    jclass audioFrameCallbackClass = pEnv->FindClass ("com/youme/voiceengine/IYouMeAudioCallback");
    if (NULL == audioFrameCallbackClass)
    {
        return -1;
    }
    mAudioFrameCallbackClass = (jclass)(pEnv->NewGlobalRef (audioFrameCallbackClass));
    mOnAudioFrameCallbackID = pEnv->GetStaticMethodID (mAudioFrameCallbackClass, "onAudioFrameCallback", "(Ljava/lang/String;[BIJ)V");
    mOnAudioFrameMixedCallbackID = pEnv->GetStaticMethodID (mAudioFrameCallbackClass, "onAudioFrameMixedCallback", "([BIJ)V");
    
    TSK_DEBUG_INFO(">>> JNI init AudioCallback set success");
    
    /*****
    jclass audioCommonCallbackClass = pEnv->FindClass ("com/youme/voiceengine/IYouMeCommonCallback");
    if (NULL == audioCommonCallbackClass)
    {
        return -1;
    }
    mAudioCommonCallbackClass = (jclass)(pEnv->NewGlobalRef (audioCommonCallbackClass));
    mOnCommonCallbackID = pEnv->GetStaticMethodID (mAudioCommonCallbackClass, "onEvent", "(II)V");
    
    jclass audioConferenceCallbackClass = pEnv->FindClass ("com/youme/voiceengine/IYouMeConferenceCallback");
    if (NULL == audioConferenceCallbackClass)
    {
        return -1;
    }
    mAudioConferenceCallbackClass = (jclass)(pEnv->NewGlobalRef (audioConferenceCallbackClass));
    mOnConferenceCallbackID = pEnv->GetStaticMethodID (mAudioConferenceCallbackClass, "onEvent", "(IILjava/lang/String;)V");
    mOnCommonEventStatusCallbackID = pEnv->GetStaticMethodID (mAudioConferenceCallbackClass, "OnCommonEventStatus", "(ILjava/lang/String;I)V");
//    mOnReciveControllMsgCallbackID = pEnv->GetStaticMethodID (mAudioConferenceCallbackClass, "OnReciveControllMsg", "(Ljava/lang/String;Ljava/lang/String;I)V");
    mOnMemberChangeMsgCallbackID = pEnv->GetStaticMethodID (mAudioConferenceCallbackClass, "OnMemberChangeMsg", "(Ljava/lang/String;Ljava/lang/String;)V");
     *****/
    
    //提供的更新接口
    jclass youmeMgrClass = pEnv->FindClass ("com/youme/voiceengine/mgr/YouMeManager");
    if (NULL == youmeMgrClass)
    {
        return -1;
    }
    mYouMeManagerClass = (jclass)(pEnv->NewGlobalRef (youmeMgrClass));
    mUpdateSelfMethodID = pEnv->GetStaticMethodID (mYouMeManagerClass, "UpdateSelf", "(Ljava/lang/String;Ljava/lang/String;)V");
    
    mTriggerNetChangeMethodID = pEnv->GetStaticMethodID (mYouMeManagerClass, "TriggerNetChange", "()V");
    
    
    mSaveLogcatMethoidID = pEnv->GetStaticMethodID (mYouMeManagerClass, "SaveLogcat", "(Ljava/lang/String;)V");
    
    TSK_DEBUG_INFO(">>> JNI init CameraMgr set beigin");
    
    // CameraMgr Class and methods
    jclass cameraMgrClass = pEnv->FindClass ("com/youme/voiceengine/CameraMgr");
    if (NULL == cameraMgrClass)
    {
        return -1;
    }
    mCameraMgrClass = (jclass)(pEnv->NewGlobalRef (cameraMgrClass));
    mStartCaptureMethodID = pEnv->GetStaticMethodID(mCameraMgrClass, "startCapture", "()I");
    mStopCaptureMethodID = pEnv->GetStaticMethodID(mCameraMgrClass, "stopCapture", "()I");
    mSwitchCameraMethodID = pEnv->GetStaticMethodID (mCameraMgrClass, "switchCamera", "()I");
    mStartRequestPermissionForApi23MethodID_camera = pEnv->GetStaticMethodID (mCameraMgrClass, "startRequestPermissionForApi23", "()Z");
    mStopRequestPermissionForApi23MethodID_camera = pEnv->GetStaticMethodID (mCameraMgrClass, "stopRequestPermissionForApi23", "()V");
    // mSetBeautyLevelMethodID = pEnv->GetStaticMethodID(mCameraMgrClass, "setBeautyLevel", "(F)V");
    // mOpenBeautifyMethodID = pEnv->GetStaticMethodID(mCameraMgrClass, "Beauty", "(Z)V");
    
    mIsCameraZoomSupportedMethodID = pEnv->GetStaticMethodID(mCameraMgrClass, "isCameraZoomSupported", "()Z");
    mSetCameraZoomFactorMethodID = pEnv->GetStaticMethodID(mCameraMgrClass, "setCameraZoomFactor", "(F)F");
    mIsCameraFocusPositionInPreviewSupportedMethodID = pEnv->GetStaticMethodID(mCameraMgrClass, "isCameraFocusPositionInPreviewSupported", "()Z");
    mSetCameraFocusPositionInPreviewMethodID = pEnv->GetStaticMethodID(mCameraMgrClass, "setCameraFocusPositionInPreview", "(FF)Z");
    mIsCameraTorchSupportedMethodID = pEnv->GetStaticMethodID(mCameraMgrClass, "isCameraTorchSupported", "()Z");
    mSetCameraTorchOnMethodID = pEnv->GetStaticMethodID(mCameraMgrClass, "setCameraTorchOn", "(Z)Z");
    mIsCameraAutoFocusFaceModeSupportedMethodID = pEnv->GetStaticMethodID(mCameraMgrClass, "isCameraAutoFocusFaceModeSupported", "()Z");
    mSetCameraAutoFocusFaceModeEnabledMethodID = pEnv->GetStaticMethodID(mCameraMgrClass, "setCameraAutoFocusFaceModeEnabled", "(Z)Z");
    
    mSetLocalVideoMirrorModeMethodID = pEnv->GetStaticMethodID (mCameraMgrClass, "setLocalVideoMirrorMode", "(I)V");
  
    //video mixer
    jclass videoMixerClass = pEnv->FindClass ("com/youme/mixers/VideoMixerHelper");
    if (NULL == videoMixerClass)
    {
        return -1;
    }
    mVideoMixerHelperClass = (jclass)(pEnv->NewGlobalRef(videoMixerClass));
    mInitMixerMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "initMixer", "()V");
    mSetVideoMixEGLContextMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "setVideoMixEGLContext", "(Ljava/lang/Object;)V");
    mGetVideoMixEGLContextMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "getVideoMixEGLContext", "()Ljava/lang/Object;");
    mSetVideoMixSizeMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "setVideoMixSize", "(II)V");
    mSetVideoNetResolutionWidthMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "setVideoNetResolutionWidth", "(II)V");
    mSetVideoNetResolutionWidthForSecondMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "setVideoNetResolutionWidthForSecond", "(II)V");
    mAddMixOverlayVideoMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "addMixOverlayVideo", "(Ljava/lang/String;IIIII)V");
    mRemoveMixOverlayVideoMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "removeMixOverlayVideo", "(Ljava/lang/String;)V");
    mRemoveAllOverlayVideoMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "removeAllOverlayVideo", "()V");
    mPushVideoFrameGLESForLocalMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "pushVideoFrameGLESForLocal", "(Ljava/lang/String;II[FIIIIJ)V");
    mPushVideoFrameRawForLocalMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "pushVideoFrameRawForLocal", "(Ljava/lang/String;I[BIIIIIJ)V");
    mPushVideoFrameGLESForRemoteMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "pushVideoFrameGLESForRemote", "(Ljava/lang/String;II[FIIJ)V");
    mPushVideoFrameRawForRemoteMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "pushVideoFrameRawForRemote", "(Ljava/lang/String;I[BIIIJ)V");
    mMixOpenBeautifyMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "openBeautify", "()V");
    mCloseBeautifyMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "closeBeautify", "()V");
    mSetBeautifyMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "setBeautify", "(F)V");
    mOnPauseMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "onPause", "()V");
    mOnResumeMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "onResume", "()V");
    mMixExitMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "mixExit", "()V");
    mSetVideoFrameMixCallbackMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "setVideoFrameMixCallback", "()V");
    mSetVideoFpsMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "setVideoFps", "(I)V");
    mSetOpenEncoderRawCallBackMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "setVideoEncoderRawCallbackEnabled", "(Z)Z");
    mSetExternalFilterEnabledMethodID = pEnv->GetStaticMethodID(mVideoMixerHelperClass, "setExternalFilterEnabled", "(Z)Z");
    jclass apiClass = pEnv->FindClass ("com/youme/voiceengine/api");
    if (NULL == apiClass)
    {
        return -1;
    }
    mJavaApiClass = (jclass)(pEnv->NewGlobalRef(apiClass));
    mSetOpenMixerRawCallBackMethodID = pEnv->GetStaticMethodID(mJavaApiClass, "setVideoMixerRawCallbackEnabled", "(Z)Z");

    jclass screenRecorderClass = pEnv->FindClass ("com/youme/voiceengine/ScreenRecorder");
    if (NULL == screenRecorderClass)
    {
        return -1;
    }
    mScreenRecorderClass = (jclass)(pEnv->NewGlobalRef(screenRecorderClass));
    mSetResolutionMethodID = pEnv->GetStaticMethodID(mScreenRecorderClass, "setResolution", "(II)Z");
    mSetFpsMethodID = pEnv->GetStaticMethodID(mScreenRecorderClass, "setFps", "(I)Z");

      //call webrtc jni_onload
    youme::webrtc::InitGlobalJniVariables(jvm);
    youme::webrtc::LoadGlobalClassReferenceHolder();
    
    TSK_DEBUG_INFO(">>> JNI init CameraMgr set success");
    return JNI_VERSION_1_4;
}


JNIEXPORT void JNICALL JNI_OnUnload (JavaVM *vm, void *reserved)
{
    JNIEnv *env = NULL;
    
    if (vm->GetEnv ((void **)&env, JNI_VERSION_1_4) != JNI_OK)
    {
        return;
    }
    if (NULL == env)
    {
        return;
    }
    env->DeleteGlobalRef (mAudioMgrClass);
    env->DeleteGlobalRef (mAudioEventCallbackClass);
	env->DeleteGlobalRef (mMemberChangeClass);
    env->DeleteGlobalRef (mVideoFrameCallbackClass);
    env->DeleteGlobalRef (mAudioFrameCallbackClass);
    env->DeleteGlobalRef (mYouMeManagerClass);
    env->DeleteGlobalRef (mAudioRecorderClass);
    env->DeleteGlobalRef (mAudioPlayerClass);
    
    // video jni接口unload
    env->DeleteGlobalRef(mVideoCapturerClass);
    env->DeleteGlobalRef(mVideoRendererClass);
    // camera mgr
    env->DeleteGlobalRef(mCameraMgrClass);
    
    youme::webrtc::FreeGlobalClassReferenceHolder();
}

int start_capture()
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return YOUME_ERROR_UNKNOWN;
    }
    TSK_DEBUG_INFO("start capture");
    return jniWrap.m_pThreadJni->CallStaticIntMethod (mCameraMgrClass, mStartCaptureMethodID);
}

int stop_capture()
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return 0;
    }
    TSK_DEBUG_INFO("stop capture");
    return jniWrap.m_pThreadJni->CallStaticIntMethod (mCameraMgrClass, mStopCaptureMethodID);
}

void set_capture_property(int nFps, int nWidth, int nHeight)
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        TSK_DEBUG_ERROR("Init video capturer failed");
        return;
    }
    TSK_DEBUG_INFO("Init video capturer in java");
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoCapturerClass,
                                               mVideoSetCamera,
                                               nWidth,
                                               nHeight,
                                               nFps);
}

void set_capture_frontCameraEnable(bool enable)
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        TSK_DEBUG_ERROR("Init video capturer failed");
        return;
    }
    TSK_DEBUG_INFO("Init video capturer in java");
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoCapturerClass,
                                               mVideoSetFrontCamera,
                                               enable);
}

int switch_camera()
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        TSK_DEBUG_ERROR("switch camera failed");
        return YOUME_ERROR_UNKNOWN;
    }
    TSK_DEBUG_INFO("Swtich camera in java");
    return jniWrap.m_pThreadJni->CallStaticIntMethod(mCameraMgrClass, mSwitchCameraMethodID);
}

void set_video_beauty_level(float level)
{
    // JNIEvnWrap jniWrap;
    // if (NULL == jniWrap.m_pThreadJni)
    // {
    //     TSK_DEBUG_ERROR("beauty camera failed");
    //     return;
    // }
    // TSK_DEBUG_INFO("beauty camera in java");
    // jniWrap.m_pThreadJni->CallStaticVoidMethod(mCameraMgrClass, mSetBeautyLevelMethodID, level);
}

void open_beauty( bool open ){
    // JNIEvnWrap jniWrap;
    // if (NULL == jniWrap.m_pThreadJni)
    // {
    //     TSK_DEBUG_ERROR("open beauty failed");
    //     return;
    // }
    // TSK_DEBUG_INFO("open beauty in java");
    // jniWrap.m_pThreadJni->CallStaticVoidMethod(mCameraMgrClass, mOpenBeautifyMethodID, open);
}


bool JNI_isCameraZoomSupported(){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        TSK_DEBUG_ERROR("is camera zoom supported failed");
        return false;
    }
    return jniWrap.m_pThreadJni->CallStaticBooleanMethod(mCameraMgrClass, mIsCameraZoomSupportedMethodID);
}
float JNI_setCameraZoomFactor(float zoomFactor){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        TSK_DEBUG_ERROR("set camera zoom  failed");
        return false;
    }
    return jniWrap.m_pThreadJni->CallStaticFloatMethod(mCameraMgrClass, mSetCameraZoomFactorMethodID, zoomFactor);
}
bool JNI_isCameraFocusPositionInPreviewSupported(){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        TSK_DEBUG_ERROR("is camera focus position supported failed");
        return false;
    }
    return jniWrap.m_pThreadJni->CallStaticBooleanMethod(mCameraMgrClass, mIsCameraFocusPositionInPreviewSupportedMethodID);
}
bool JNI_setCameraFocusPositionInPreview(float x , float y ){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        TSK_DEBUG_ERROR("set camera  focus position  failed");
        return false;
    }
    return jniWrap.m_pThreadJni->CallStaticBooleanMethod(mCameraMgrClass, mSetCameraFocusPositionInPreviewMethodID, x, y);

}
bool JNI_isCameraTorchSupported(){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        TSK_DEBUG_ERROR("is camera torch supported failed");
        return false;
    }
    return jniWrap.m_pThreadJni->CallStaticBooleanMethod(mCameraMgrClass, mIsCameraTorchSupportedMethodID);

}
bool JNI_setCameraTorchOn(bool isOn ){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        TSK_DEBUG_ERROR("set camera torch  failed");
        return false;
    }
    return jniWrap.m_pThreadJni->CallStaticBooleanMethod(mCameraMgrClass, mSetCameraTorchOnMethodID, isOn);

}
bool JNI_isCameraAutoFocusFaceModeSupported(){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        TSK_DEBUG_ERROR("is camera auto focus  supported failed");
        return false;
    }
    return jniWrap.m_pThreadJni->CallStaticBooleanMethod(mCameraMgrClass, mIsCameraAutoFocusFaceModeSupportedMethodID);

}
bool JNI_setCameraAutoFocusFaceModeEnabled(bool enable ){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        TSK_DEBUG_ERROR("set camera auto focus   failed");
        return false;
    }
    return jniWrap.m_pThreadJni->CallStaticBooleanMethod(mCameraMgrClass, mSetCameraAutoFocusFaceModeEnabledMethodID, enable);

}

void JNI_setLocalVideoMirrorMode(int mirrorMode) {
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        TSK_DEBUG_ERROR("set camera mirror mode failed");
        return;
    }
    
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mCameraMgrClass, mSetLocalVideoMirrorModeMethodID, mirrorMode);
} 

void JNI_screenRecorderSetResolution(int width, int height)
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        TSK_DEBUG_ERROR("set screen recorder resolution failed");
        return;
    }
    TSK_DEBUG_INFO("set screen recorder resolution width:%d height:%d", width, height);
    jniWrap.m_pThreadJni->CallStaticBooleanMethod(mScreenRecorderClass, mSetResolutionMethodID, (jint)width, (jint)height);
}

void JNI_screenRecorderSetFps(int fps)
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        TSK_DEBUG_ERROR("set screen recorder fps failed");
        return;
    }
    TSK_DEBUG_INFO("set screen recorder fps:%d", fps);
    jniWrap.m_pThreadJni->CallStaticBooleanMethod(mScreenRecorderClass, mSetFpsMethodID, (jint)fps);
}

void start_voice ()
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    TSK_DEBUG_INFO("Entering communication mode");
    jniWrap.m_pThreadJni->CallStaticVoidMethod (mAudioMgrClass, mStartVoiceMethodID, 0);
}

void stop_voice ()
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    TSK_DEBUG_INFO("Leaving communication mode");
    jniWrap.m_pThreadJni->CallStaticVoidMethod (mAudioMgrClass, mStopVoiceMethodID, 0);
}

void init_audio_settings(bool outputToSpeaker)
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    TSK_DEBUG_INFO("Init audio setting in java");
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mAudioMgrClass, mInitAudioSettingsMethodID, outputToSpeaker);
}

bool JNI_startRequestPermissionForApi23()
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return false;
    }
    TSK_DEBUG_INFO("Request API23 permissions");
    return jniWrap.m_pThreadJni->CallStaticBooleanMethod(mAudioMgrClass, mStartRequestPermissionForApi23MethodID, 0);
}

void JNI_stopRequestPermissionForApi23()
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    TSK_DEBUG_INFO("Stop request API23 permissions");
    return jniWrap.m_pThreadJni->CallStaticVoidMethod(mAudioMgrClass, mStopRequestPermissionForApi23MethodID, 0);
}

bool JNI_startRequestPermissionForApi23_camera()
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return false;
    }
    TSK_DEBUG_INFO("Request API23 permissions camera");
    return jniWrap.m_pThreadJni->CallStaticBooleanMethod(mCameraMgrClass, mStartRequestPermissionForApi23MethodID_camera, 0);
}

void JNI_stopRequestPermissionForApi23_camera()
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    TSK_DEBUG_INFO("Stop request API23 permissions camera");
    return jniWrap.m_pThreadJni->CallStaticVoidMethod(mCameraMgrClass, mStopRequestPermissionForApi23MethodID_camera, 0);
}

void JNI_Init_Audio_Record (int sampleRate, int channelNum, int bytesPerSample, int typeSpeech, tmedia_producer_t *pProducerInstanse)
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    TSK_DEBUG_INFO("Init audio recorder");
    jniWrap.m_pThreadJni->CallStaticVoidMethod (mAudioRecorderClass, mAudioInitRecorderMethodID, sampleRate, channelNum, bytesPerSample, typeSpeech);
    m_pProducer = pProducerInstanse;
}

void JNI_Start_Audio_Record ()
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    TSK_DEBUG_INFO("Start audio recorder");
    jniWrap.m_pThreadJni->CallStaticVoidMethod (mAudioRecorderClass, mAudioRecorderMethodID, 1);
}

void JNI_Set_Mic_isMute(int isMute)
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    TSK_DEBUG_INFO("Set JNI Mic Mute %d",isMute);
    jniWrap.m_pThreadJni->CallStaticVoidMethod (mAudioRecorderClass, mAudioSetIsMuteMethodID, isMute);
}

void JNI_Stop_Audio_Record ()
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    TSK_DEBUG_INFO("Stop audio recorder");
    jniWrap.m_pThreadJni->CallStaticVoidMethod (mAudioRecorderClass, mAudioRecorderMethodID, 0);
}

void JNI_Resume_Audio_Record ()
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    TSK_DEBUG_INFO("Start audio recorder");
    jniWrap.m_pThreadJni->CallStaticVoidMethod (mAudioRecorderClass, mAudioRecorderPauseMethodID, 1);
}

int JNI_Is_Wired_HeadsetOn ()
{
    jint res = 0;
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return res;
    }
    TSK_DEBUG_INFO("Start audio recorder");
    res = jniWrap.m_pThreadJni->CallStaticIntMethod (mAudioMgrClass, mAudioMgrIsWiredHeadsetOnMethodID);
    return (int)res;
}

void JNI_Pause_Audio_Record ()
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    TSK_DEBUG_INFO("Stop audio recorder");
    jniWrap.m_pThreadJni->CallStaticVoidMethod (mAudioRecorderClass, mAudioRecorderPauseMethodID, 0);
}

int JNI_Get_Audio_Record_Status ()
{
    jint res = 0;
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return res;
    }
    res = jniWrap.m_pThreadJni->CallStaticIntMethod (mAudioRecorderClass, mAudioRecorderStatusMethodID);
    return (int)res;
}

void JNI_Init_Audio_Player (int sampleRate, int channelNum, int bytesPerSample, int isStreamVoice, tmedia_consumer_t *pConsumerInstanse)
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    TSK_DEBUG_INFO("Init audio player");
    jniWrap.m_pThreadJni->CallStaticVoidMethod (mAudioPlayerClass, mAudioInitPlayerMethodID, (jint)sampleRate, (jint)channelNum,
                                                (jint)bytesPerSample, (jboolean)isStreamVoice);
    m_pConsumer = pConsumerInstanse;
}

void JNI_Start_Audio_Player ()
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    TSK_DEBUG_INFO("Start audio player");
    jniWrap.m_pThreadJni->CallStaticVoidMethod (mAudioPlayerClass, mAudioPlayerMethodID, 1);
}

void JNI_Stop_Audio_Player ()
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    TSK_DEBUG_INFO("Stop audio player");
    jniWrap.m_pThreadJni->CallStaticVoidMethod (mAudioPlayerClass, mAudioPlayerMethodID, 0);
}

int JNI_Get_Audio_Player_Status ()
{
    jint res = 0;
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return res;
    }
    res = jniWrap.m_pThreadJni->CallStaticIntMethod (mAudioPlayerClass, mAudioPlayerStatusMethodID);
    return (int)res;
}

void SaveLogcat(const std::string& strPath)
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    
    jniWrap.m_pThreadJni->CallStaticVoidMethod (mYouMeManagerClass, mSaveLogcatMethoidID, string2jstring(jniWrap.m_pThreadJni,strPath.c_str()));
   
}

void UpdateAndroid(const std::string& strURL,const std::string& strMD5)
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    jniWrap.m_pThreadJni->CallStaticVoidMethod (mYouMeManagerClass, mUpdateSelfMethodID, string2jstring(jniWrap.m_pThreadJni,strURL.c_str()),string2jstring(jniWrap.m_pThreadJni,strMD5.c_str()));

}

void TriggerNetChange()
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    
    jniWrap.m_pThreadJni->CallStaticVoidMethod (mYouMeManagerClass, mTriggerNetChangeMethodID);
  
    
}

std::string jstring2string (JNIEnv *env, jstring jstr)
{
    std::string strResult;
    jclass clsstring = env->FindClass ("java/lang/String");
    jstring strencode = env->NewStringUTF ("utf-8");
    jmethodID mid = env->GetMethodID (clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr = (jbyteArray)env->CallObjectMethod (jstr, mid, strencode);
    jsize alen = env->GetArrayLength (barr);
    jbyte *ba = env->GetByteArrayElements (barr, JNI_FALSE);
    if (alen > 0)
    {
        strResult = std::string((const char*)ba,alen);
    }
    env->ReleaseByteArrayElements (barr, ba, 0);
    return strResult;
}


// For debug purpose only, so do not export through standard header file.
extern void SetServerMode(SERVER_MODE serverMode);
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setServerMode (JNIEnv *env, jclass className, jint mode)
{
    SetServerMode((SERVER_MODE)mode);
}

extern void SetServerIpPort(const char* ip, int port);
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setServerIpPort
    (JNIEnv *env, jclass jobj, jstring ip, jint port)
{
    std::string strIp = jstring2string(env, ip);
    SetServerIpPort(strIp.c_str(), (int)port);
}

//
// Uninit/init the microphone, so that the microphone can take the effect of permission changes.
//
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_resetMicrophone
    (JNIEnv *env, jclass jobj)
{
    CYouMeVoiceEngine::getInstance ()->pauseChannel(false);
    CYouMeVoiceEngine::getInstance ()->resumeChannel(false);
}

JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_callbackPermissionStatus
    (JNIEnv *env, jclass jobj, jint errCode)
{
    CYouMeVoiceEngine::getInstance ()->sendCbMsgCallEvent((YouMeEvent_t)YOUME_EVENT_REC_PERMISSION_STATUS, (YouMeErrorCode_t)errCode, "", "" );
}

JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_resetCamera
    (JNIEnv *, jclass)
{
    CYouMeVoiceEngine::getInstance ()->resetCamera();
}

JNIEXPORT void Java_com_youme_voiceengine_NativeEngine_AudioRecorderBufRefresh (JNIEnv *env,
                                                                                jclass className,
                                                                                jobject audBuf,
                                                                                jint sampleRate,
                                                                                jint channelNum,
                                                                                jint bytesPerSample)
{
    if (NULL == audBuf) {
        return;
    }
    // Handle record buffer
    audio_producer_android_t *self         = NULL;
    audio_android_instance_t *instance     = NULL;
    AudioAndroidDeviceCallbackImpl *cbFunc = NULL;
    jbyte *jArray = (jbyte *)env->GetDirectBufferAddress(audBuf);
    if (NULL == jArray)
    {
        TSK_DEBUG_WARN("Native layer jArray = NULL");
        return;
    }
    
    self = (audio_producer_android_t *)m_pProducer;
    if (!self)
    {
        TSK_DEBUG_WARN("Invalid parameter");
        return;
    }
    instance = (audio_android_instance_t *)(self->audioInstHandle);
    if (!instance)
    {
        TSK_DEBUG_WARN("Invalid parameter");
        return;
    }
    cbFunc = (AudioAndroidDeviceCallbackImpl *)(instance->callback);
    if (!cbFunc)
    {
        TSK_DEBUG_WARN("Invalid parameter");
        return;
    }
    cbFunc->RecordedDataIsAvailable(jArray, sampleRate / 100 * 2, bytesPerSample, channelNum, sampleRate);
    //env->ReleaseByteArrayElements(audBuf, jArray, 0);
    //TSK_DEBUG_INFO("%d %d %d %d", jArray[0], jArray[1], jArray[2], jArray[3]);
}

JNIEXPORT void Java_com_youme_voiceengine_NativeEngine_AudioPlayerBufRefresh (JNIEnv *env,
                                                                              jclass className,
                                                                              jobject audBuf,
                                                                              jint sampleRate,
                                                                              jint channelNum,
                                                                              jint bytesPerSample)
{
    if (NULL == audBuf) {
        return;
    }
    // Handle player buffer
    audio_consumer_android_t *self         = NULL;
    audio_android_instance_t *instance     = NULL;
    AudioAndroidDeviceCallbackImpl *cbFunc = NULL;
    uint32_t sampleNum                     = 0;
    static uint32_t preSampleNum           = 0;
    jsize arraySize = sampleRate * channelNum * bytesPerSample / 100 * 2; // 20ms data
    //jbyte *jArray = new jbyte[arraySize];
    jbyte *jArray = (jbyte *)env->GetDirectBufferAddress(audBuf);
    if (NULL == jArray)
    {
        TSK_DEBUG_WARN("Native layer jArray = NULL");
        return;
    }
    
    self = (audio_consumer_android_t *)m_pConsumer;
    instance = (audio_android_instance_t *)(self->audioInstHandle);
    cbFunc = (AudioAndroidDeviceCallbackImpl *)(instance->callback);
    if (!self || !instance || !cbFunc)
    {
        TSK_DEBUG_WARN("Invalid parameter");
        //env->ReleaseByteArrayElements(audBuf, jArray, 0);
        return;
    }
    cbFunc->NeedMorePlayData(arraySize / 2, bytesPerSample, channelNum, sampleRate, jArray, sampleNum);
    if (sampleNum != arraySize / 2) {
        memset(jArray, 0, arraySize);
        if (preSampleNum != sampleNum) {
            TSK_DEBUG_INFO("Native layer: get the wrong size of consume data: sampleNum = %d, arraySize = %d", sampleNum, arraySize / 2);
            preSampleNum = sampleNum;
        }
    }
    //env->ReleaseByteArrayElements(audBuf, jArray, 0);
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    inputAudioFrame
 * Signature: ([BIL)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_NativeEngine_inputAudioFrame (JNIEnv *env,
                                                                                jclass className,
                                                                                jbyteArray audioBuf,
                                                                                jint bufSize,
                                                                                jlong timestamp,
                                                                                jint channels,
                                                                                jboolean bInterleaved)
{
    if (NULL == audioBuf) {
        return false;
    }
    
    jbyte *jArray = (jbyte *)env->GetByteArrayElements(audioBuf, NULL);
    if (NULL == jArray)
    {
        TSK_DEBUG_WARN("Native layer jArray = NULL");
        return false;
    }
    
    IYouMeVoiceEngine::getInstance()->inputAudioFrame(jArray, bufSize, timestamp, channels, bInterleaved);
    env->ReleaseByteArrayElements(audioBuf, jArray, 0);
    return true;
}


/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    inputAudioFrameForMix
 * Signature: (I[BILjava/lang/Object;J)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_NativeEngine_inputAudioFrameForMix (JNIEnv *env,
                                                                                jclass className,
                                                                                jint streamId,
                                                                                jbyteArray audioBuf,
                                                                                jint bufSize,
                                                                                jobject frameInfoObj,
                                                                                jlong timestamp)
{
    YMAudioFrameInfo info;
    jclass clazz;
    jfieldID fid;

    // mapping bar of C to foo
    clazz = env->GetObjectClass(frameInfoObj);
    if (0 == clazz) {
        return (-1);
    }

    jfieldID id_channels = env->GetFieldID(clazz, "channels", "I");
    jint channels = env->GetIntField(frameInfoObj, id_channels);
    info.channels = channels;

    jfieldID id_sampleRate = env->GetFieldID(clazz, "sampleRate", "I");
    jint sampleRate = env->GetIntField(frameInfoObj, id_sampleRate);
    info.sampleRate = sampleRate;

    jfieldID id_bytesPerFrame = env->GetFieldID(clazz, "bytesPerFrame", "I");
    jint bytesPerFrame = env->GetIntField(frameInfoObj, id_bytesPerFrame);
    info.bytesPerFrame = bytesPerFrame;
    
    jfieldID id_isFloat = env->GetFieldID(clazz, "isFloat", "Z");
    jboolean isFloat = env->GetBooleanField(frameInfoObj, id_isFloat);
    info.isFloat = (bool)isFloat?1:0;
    
    jfieldID id_isBigEndian = env->GetFieldID(clazz, "isBigEndian", "Z");
    jboolean isBigEndian = env->GetBooleanField(frameInfoObj, id_isBigEndian);
    info.isBigEndian = (bool)isBigEndian?1:0;
    
    jfieldID id_isSignedInteger = env->GetFieldID(clazz, "isSignedInteger", "Z");
    jboolean isSignedInteger = env->GetBooleanField(frameInfoObj, id_isSignedInteger);
    info.isSignedInteger = (bool)isSignedInteger?1:0;
    
    jfieldID id_isNonInterleaved = env->GetFieldID(clazz, "isNonInterleaved", "Z");
    jboolean isNonInterleaved = env->GetBooleanField(frameInfoObj, id_isNonInterleaved);
    info.isNonInterleaved = (bool)isNonInterleaved?1:0;

    jfieldID id_timestamp = env->GetFieldID(clazz, "timestamp", "J");
    jlong info_timestamp = env->GetLongField(frameInfoObj, id_timestamp);
    info.timestamp = info_timestamp;

    if (NULL == audioBuf) {
        return false;
    }
    
    jbyte *jArray = (jbyte *)env->GetByteArrayElements(audioBuf, NULL);
    if (NULL == jArray)
    {
        TSK_DEBUG_WARN("Native layer jArray = NULL");
        return false;
    }
    
    IYouMeVoiceEngine::getInstance()->inputAudioFrameForMix(streamId, jArray, bufSize, info, timestamp);
    env->ReleaseByteArrayElements(audioBuf, jArray, 0);
    return true;
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    inputCustomData
 * Signature: ([BIJ)Z
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_NativeEngine_inputCustomData (JNIEnv *env,
                                                                                    jclass className,
                                                                                    jbyteArray audioBuf,
                                                                                    jint bufSize,
                                                                                    jlong timestamp)
{
    if (NULL == audioBuf) {
        return -1;
    }
    
    jbyte *jArray = (jbyte *)env->GetByteArrayElements(audioBuf, NULL);
    if (NULL == jArray)
    {
        TSK_DEBUG_WARN("Native layer jArray = NULL");
        return -1;
    }
    
    int iErrorCode = IYouMeVoiceEngine::getInstance()->inputCustomData(jArray, bufSize, timestamp);
    env->ReleaseByteArrayElements(audioBuf, jArray, 0);
    return iErrorCode;
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    inputCustomDataToUser
 * Signature: ([BIJLjava/lang/String;)Z
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_NativeEngine_inputCustomDataToUser (JNIEnv *env,
                                                                                    jclass className,
                                                                                    jbyteArray audioBuf,
                                                                                    jint bufSize,
                                                                                    jlong timestamp,
                                                                                    jstring userId)
{
    if (NULL == audioBuf) {
        return -1;
    }
    
    jbyte *jArray = (jbyte *)env->GetByteArrayElements(audioBuf, NULL);
    if (NULL == jArray)
    {
        TSK_DEBUG_WARN("Native layer jArray = NULL");
        return -1;
    }
    
    int iErrorCode = IYouMeVoiceEngine::getInstance()->inputCustomData(jArray, bufSize, timestamp, jstring2string(env, userId));
    env->ReleaseByteArrayElements(audioBuf, jArray, 0);
    return iErrorCode;
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    maskVideoByUserId
 * Signature: (Ljava/lang/String;Z)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_maskVideoByUserId(JNIEnv *env, jclass className, jstring userId, jboolean mask)
{
    CYouMeVoiceEngine::getInstance()->maskVideoByUserId(jstring2string(env, userId), mask);
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    inputVideoFrame
 * Signature: ([BIIIIIIJ)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_NativeEngine_inputVideoFrame
(JNIEnv * env, jclass className, jbyteArray videoBuf, jint bufSize, jint width, jint height, jint fmt, jint rotation, jint mirror, jlong timestamp)
{
    if (NULL == videoBuf) {
        return false;
    }
    
    jbyte *jArray = (jbyte *)env->GetByteArrayElements(videoBuf, NULL);
    if (NULL == jArray)
    {
        TSK_DEBUG_WARN("Native layer jArray = NULL");
        return false;
    }
    
    IYouMeVoiceEngine::getInstance()->inputVideoFrame(jArray, bufSize, width, height, fmt, rotation, mirror, timestamp);
    env->ReleaseByteArrayElements(videoBuf, jArray, 0);
    return true;
}

JNIEXPORT jboolean Java_com_youme_voiceengine_NativeEngine_inputVideoFrameByteBuffer (JNIEnv *env,
                                                                                jclass className,
                                                                                jobject videoBuf,
                                                                                jint bufSize, jint width, jint height, jint fmt, jint rotation, jint mirror, jlong timestamp)
{
    if (NULL == videoBuf) {
        return false;
    }
    // Handle buffer
    
    jbyte *jArray = (jbyte *)env->GetDirectBufferAddress(videoBuf);
    if (NULL == jArray)
    {
        TSK_DEBUG_WARN("Native layer jArray = NULL");
        return false;
    }
    
    IYouMeVoiceEngine::getInstance()->inputVideoFrame(jArray, bufSize, width, height, fmt, rotation, mirror, timestamp);
    return true;
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    inputVideoFrameEncrypt
 * Signature: ([BIIIIIIJI)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_NativeEngine_inputVideoFrameEncrypt
(JNIEnv * env, jclass className, jbyteArray videoBuf, jint bufSize, jint width, jint height, jint fmt, jint rotation, jint mirror, jlong timestamp, jint streamID)
{
    if (NULL == videoBuf) {
        return false;
    }
    
    jbyte *jArray = (jbyte *)env->GetByteArrayElements(videoBuf, NULL);
    if (NULL == jArray)
    {
        TSK_DEBUG_WARN("Native layer jArray = NULL");
        return false;
    }
    
    IYouMeVoiceEngine::getInstance()->inputVideoFrame(jArray, bufSize, width, height, fmt, rotation, mirror, timestamp, streamID);
    env->ReleaseByteArrayElements(videoBuf, jArray, 0);
    return true;
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    inputVideoFrameForShare
 * Signature: ([BIIIIIL)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_NativeEngine_inputVideoFrameForShare
(JNIEnv * env, jclass className, jbyteArray videoBuf, jint bufSize, jint width, jint height, jint fmt, jint rotation, jint mirror, jlong timestamp)
{
    if (NULL == videoBuf) {
        return false;
    }
    
    jbyte *jArray = (jbyte *)env->GetByteArrayElements(videoBuf, NULL);
    if (NULL == jArray)
    {
        TSK_DEBUG_WARN("Native layer jArray = NULL");
        return false;
    }
    
    IYouMeVoiceEngine::getInstance()->inputVideoFrameForShare(jArray, bufSize, width, height, fmt, rotation, mirror, timestamp);
    env->ReleaseByteArrayElements(videoBuf, jArray, 0);
    return true;
}

JNIEXPORT jboolean Java_com_youme_voiceengine_NativeEngine_inputVideoFrameByteBufferForShare (JNIEnv *env,
                                                                                jclass className,
                                                                                jobject videoBuf,
                                                                                jint bufSize, jint width, jint height, jint fmt, jint rotation, jint mirror, jlong timestamp)
{
    if (NULL == videoBuf) {
        return false;
    }
    // Handle buffer
    
    jbyte *jArray = (jbyte *)env->GetDirectBufferAddress(videoBuf);
    if (NULL == jArray)
    {
        TSK_DEBUG_WARN("Native layer jArray = NULL");
        return false;
    }
    
    IYouMeVoiceEngine::getInstance()->inputVideoFrameForShare(jArray, bufSize, width, height, fmt, rotation, mirror, timestamp);
    return true;
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    inputVideoFrameGLES
 * Signature: (I[FIIIIIJ)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_NativeEngine_inputVideoFrameGLES
(JNIEnv * env, jclass className, jint texture, jfloatArray matrix, jint width, jint height, jint fmt, jint rotation, jint mirror, jlong timestamp)

{
    jfloat *jArray = NULL;
    if(matrix)
      jArray = (jfloat *)env->GetFloatArrayElements(matrix, NULL);
    
    IYouMeVoiceEngine::getInstance()->inputVideoFrameForAndroid(texture, (float*)jArray, width, height, fmt, rotation, mirror, timestamp);
    
    if(jArray)
        env->ReleaseFloatArrayElements(matrix, jArray, 0);
    
     return true;
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    inputVideoFrameGLESForShare
 * Signature: (I[FIIIIIJ)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_NativeEngine_inputVideoFrameGLESForShare
(JNIEnv * env, jclass className, jint texture, jfloatArray matrix, jint width, jint height, jint fmt, jint rotation, jint mirror, jlong timestamp)

{
    jfloat *jArray = NULL;
    if(matrix)
      jArray = (jfloat *)env->GetFloatArrayElements(matrix, NULL);
    
    IYouMeVoiceEngine::getInstance()->inputVideoFrameForAndroidShare(texture, (float*)jArray, width, height, fmt, rotation, mirror, timestamp);
    
    if(jArray)
        env->ReleaseFloatArrayElements(matrix, jArray, 0);
    
     return true;
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    stopInputVideoFrame
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_stopInputVideoFrame
(JNIEnv *, jclass)
{
    CYouMeVoiceEngine::getInstance()->stopInputVideoFrame();
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    stopInputVideoFrameForShare
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_stopInputVideoFrameForShare
(JNIEnv *, jclass)
{
    CYouMeVoiceEngine::getInstance()->stopInputVideoFrameForShare();
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setModel
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setModel (JNIEnv *env, jclass object, jstring para)
{
    g_AndroidSystemProvider->m_strModel= (jstring2string (env, para));
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setSysName
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setSysName (JNIEnv *env, jclass object, jstring para)
{
    NgnApplication::getInstance ()->setSysName (jstring2string (env, para));
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setSysVersion
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setSysVersion (JNIEnv *env, jclass object, jstring para)
{
    g_AndroidSystemProvider->m_strSystemVersion = (jstring2string (env, para));
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setVersionName
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setVersionName (JNIEnv *env, jclass object, jstring para)
{
    
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setDeviceURN
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setDeviceURN (JNIEnv *env, jclass object, jstring para)
{
    
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setDeviceIMEI
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setDeviceIMEI (JNIEnv *env, jclass object, jstring para)
{
    NgnApplication::getInstance ()->setDeviceIMEI (jstring2string (env, para));
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setCPUArch
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setCPUArch (JNIEnv *env, jclass object, jstring para)
{
    g_AndroidSystemProvider->m_strCPUArchive= (jstring2string (env, para));
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setPackageName
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setPackageName (JNIEnv *env, jclass object, jstring para)
{
    g_AndroidSystemProvider->m_strPackageName= (jstring2string (env, para));
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setUUID
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setUUID (JNIEnv *env, jclass object, jstring para)
{
    NgnApplication::getInstance ()->setUUID (jstring2string (env, para));
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setScreenOrientation
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setScreenOrientation
(JNIEnv *env, jclass object, jint orientation)
{
    SCREEN_ORIENTATION_E screenOri = portrait;
    switch (orientation) {
        case 0:
            screenOri = portrait;
            break;
        case 1:
            screenOri = landscape_right;
            break;
        case 2:
            screenOri = upside_down;
            break;
        case 3:
            break;
            screenOri = landscape_left;
        default:
            break;
    }
    NgnApplication::getInstance ()->setScreenOrientation(screenOri);
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setDocumentPath
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setDocumentPath (JNIEnv *env, jclass object, jstring para)
{
    NgnApplication::getInstance ()->setDocumentPath (jstring2string (env, para));
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    onNetWorkChanged
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_onNetWorkChanged (JNIEnv *env, jclass object, jint para)
{
    NgnEngine::getInstance ()->getNetworkService ()->onNetWorkChanged ((NETWORK_TYPE)para);
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    onNetWorkChanged
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_onHeadSetPlugin (JNIEnv *env, jclass object, jint para)
{
    CYouMeVoiceEngine::getInstance ()->onHeadSetPlugin (para);
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    getSoVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_youme_voiceengine_NativeEngine_getSoVersion (JNIEnv *env, jclass className)
{
    char szSDKVersion[10] = { 0 };
    memset (szSDKVersion, 0, 10);
    snprintf (szSDKVersion, 10, SDK_VERSION, MAIN_VER, MINOR_VER, RELEASE_NUMBER, BUILD_NUMBER);
    return string2jstring (env, szSDKVersion);
}

JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setPcmCallbackEnable
    (JNIEnv *env, jclass jobj, jint flag , jboolean outputToSpeaker, jint nOutputSampleRate, jint nOutputChannel)
{
    if ( flag != 0 ) {
        CYouMeVoiceEngine::getInstance ()->setPcmCallbackEnable(mPYouMePcmCallback,  flag, (bool)outputToSpeaker, nOutputSampleRate, nOutputChannel);
    } else {
        CYouMeVoiceEngine::getInstance ()->setPcmCallbackEnable(NULL, 0 ,  false, 16000, 1);
    }
}

JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setVideoPreDecodeCallbackEnable
    (JNIEnv *env, jclass jobj, jboolean enable, jboolean needDecodeandRender)
{
    if ( (bool)enable ) {
        CYouMeVoiceEngine::getInstance ()->setVideoPreDecodeCallbackEnable(mPYouMeVideoPreDecodeCallback, (bool)needDecodeandRender);
    } else {
        CYouMeVoiceEngine::getInstance ()->setVideoPreDecodeCallbackEnable(NULL, (bool)needDecodeandRender);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
/*
 * Class:     com_youme_voiceengine_api
 * Method:    init
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_NativeEngine_connect(JNIEnv *env, jclass className, jstring strAPPKey, jstring strAPPSecret,
                                                           jint serverRegionId)
{
    CYouMeVoiceEngine::getInstance ()->setRestApiCallback( mPYouMeEventCallback );
    CYouMeVoiceEngine::getInstance ()->setMemberChangeCallback( mPYouMeEventCallback );
    CYouMeVoiceEngine::getInstance ()->setNotifyCallback( mPYouMeEventCallback );
    CYouMeVoiceEngine::getInstance ()->setAVStatisticCallback( mPYouMeEventCallback );
    CYouMeVoiceEngine::getInstance()->setRecvCustomDataCallback(mPYouMeEventCallback);
    CYouMeVoiceEngine::getInstance()->setTranslateCallback( mPYouMeEventCallback );
    return CYouMeVoiceEngine::getInstance ()->init(mPYouMeEventCallback,jstring2string (env, strAPPKey),jstring2string (env, strAPPSecret),
                                                   (YOUME_RTC_SERVER_REGION)serverRegionId);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    unInit
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_unInit(JNIEnv *env, jclass className)
{
    return CYouMeVoiceEngine::getInstance ()->unInit();
}

JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setToken
(JNIEnv *env, jclass className, jstring strToken ){
    return  CYouMeVoiceEngine::getInstance()->setToken( jstring2string(env, strToken ).c_str() );
}

JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setTokenV3
(JNIEnv *env, jclass className, jstring strToken, jlong timeStamp ){
    return  CYouMeVoiceEngine::getInstance()->setToken( jstring2string(env, strToken ).c_str(), (uint32_t)timeStamp );
}


JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setTCPMode
(JNIEnv *env, jclass className, jboolean bUseTcp ){
    return  CYouMeVoiceEngine::getInstance()->setTCPMode( bUseTcp);
}



JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setUserLogPath
(JNIEnv * env, jclass jobj, jstring filePath)
{
    return CYouMeVoiceEngine::getInstance ()->setUserLogPath(jstring2string(env,filePath));
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setOutputToSpeaker
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setOutputToSpeaker
(JNIEnv *env, jclass className, jboolean bOutputToSpeaker)
{
    return CYouMeVoiceEngine::getInstance ()->setOutputToSpeaker(bOutputToSpeaker);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setSpeakerMute
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setSpeakerMute
(JNIEnv *env, jclass className, jboolean bOn)
{
    CYouMeVoiceEngine::getInstance ()->setSpeakerMute(bOn);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    getSpeakerMute
 * Signature: ()I
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_getSpeakerMute
(JNIEnv *env, jclass className)
{
    return (jboolean)CYouMeVoiceEngine::getInstance ()->getSpeakerMute();
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    getMicrophoneMute
 * Signature: ()I
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_getMicrophoneMute
(JNIEnv *env, jclass className)
{
    return (jboolean)CYouMeVoiceEngine::getInstance ()->isMicrophoneMute();
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setMicrophoneMute
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setMicrophoneMute
(JNIEnv *env, jclass className, jboolean bOn)
{
    CYouMeVoiceEngine::getInstance ()->setMicrophoneMute(bOn);
}

JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setAutoSendStatus
(JNIEnv *env, jclass className, jboolean bOn)
{
    CYouMeVoiceEngine::getInstance()->setAutoSendStatus( bOn );
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    getVolume
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_getVolume
(JNIEnv *env, jclass className)
{
    return CYouMeVoiceEngine::getInstance ()->getVolume();
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVolume
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setVolume
(JNIEnv *env, jclass className, jint volume)
{
    return CYouMeVoiceEngine::getInstance ()->setVolume(volume);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    getUseMobileNetworkEnabled
 * Signature: ()I
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_getUseMobileNetworkEnabled
(JNIEnv *env, jclass className)
{
    return (jboolean)CYouMeVoiceEngine::getInstance ()->getUseMobileNetWorkEnabled();
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setUseMobileNetworkEnabled
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setUseMobileNetworkEnabled
(JNIEnv *env, jclass className, jboolean bUse)
{
    return CYouMeVoiceEngine::getInstance ()->setUseMobileNetworkEnabled(bUse);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setLocalConnectionInfo
 * Signature: (I)V
 */
JNIEXPORT int JNICALL Java_com_youme_voiceengine_api_setLocalConnectionInfo
(JNIEnv *env, jclass className, jstring localIP, jint localPort, jstring remoteIP, jint remotePort)
{
    return CYouMeVoiceEngine::getInstance ()->setLocalConnectionInfo(jstring2string(env, localIP), localPort, jstring2string(env, remoteIP), remotePort, 0);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    clearLocalConnectionInfo
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_clearLocalConnectionInfo(JNIEnv *env, jclass className)
{
    CYouMeVoiceEngine::getInstance ()->clearLocalConnectionInfo();
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setRouteChangeFlag
 * Signature: (I)V
 */
JNIEXPORT int JNICALL Java_com_youme_voiceengine_api_setRouteChangeFlag
(JNIEnv *env, jclass className, jboolean enable)
{
    return CYouMeVoiceEngine::getInstance ()->setRouteChangeFlag(enable);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    joinChannelSingle
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_joinChannelSingleMode
(JNIEnv *env, jclass className,jstring userid, jstring channelid, jint userRole, jboolean autoRecv)
{
    return CYouMeVoiceEngine::getInstance ()->joinChannelSingleMode(jstring2string(env,userid), jstring2string(env,channelid),
                                                                (YouMeUserRole_t)userRole, autoRecv);
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_joinChannelSingleModeWithAppKey
(JNIEnv *env, jclass className,jstring userid, jstring channelid, jint userRole, jstring appKey, jboolean autoRecv)
{
    return CYouMeVoiceEngine::getInstance ()->joinChannelSingleMode(jstring2string(env,userid), jstring2string(env,channelid),
                                                                    (YouMeUserRole_t)userRole,
                                                                    jstring2string(env,appKey), autoRecv);
}


/*
 * Class:     com_youme_voiceengine_api
 * Method:    joinChannelMultiMode
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_joinChannelMultiMode
  (JNIEnv * env, jclass className, jstring strUserId, jstring strChannelId, jint userRole)
{
	return CYouMeVoiceEngine::getInstance ()->joinChannelMultiMode(jstring2string(env, strUserId), jstring2string(env, strChannelId), (YouMeUserRole_t)userRole);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    speakToChannel
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_speakToChannel
  (JNIEnv * env, jclass className, jstring strChannelId)
{
	return CYouMeVoiceEngine::getInstance ()->speakToChannel(jstring2string(env, strChannelId));
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    leaveChannelMultiMode
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_leaveChannelMultiMode
(JNIEnv *env, jclass className, jstring channelid)
{
    return CYouMeVoiceEngine::getInstance ()->leaveChannelMultiMode(jstring2string(env,channelid));
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    leaveChannelAll
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_leaveChannelAll
(JNIEnv *env, jclass className)
{
    return CYouMeVoiceEngine::getInstance ()->leaveChannelAll();
}


/*
 * Class:     com_youme_voiceengine_api
 * Method:    getChannelUserList
 * Signature: (Ljava/lang/String;IZ)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_getChannelUserList
(JNIEnv *env, jclass className, jstring  strChannel , jint maxCount , jboolean notifyMemChange ){
    if(strChannel == NULL){
        return -2;// YOUME_ERROR_INVALID_PARAM
    }
    return CYouMeVoiceEngine::getInstance ()->getChannelUserList( jstring2string(env, strChannel).c_str(), maxCount, notifyMemChange );
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setOtherMicMute
 * Signature: (Ljava/lang/String;Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setOtherMicMute
(JNIEnv *env, jclass className, jstring strUserID , jboolean bMute){
    return CYouMeVoiceEngine::getInstance ()->setOtherMicMute( jstring2string(env, strUserID).c_str(),  bMute);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setOtherSpeakerStatus
 * Signature: (Ljava/lang/String;Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setOtherSpeakerMute
(JNIEnv *env, jclass className, jstring strUserID , jboolean bMute){
    return CYouMeVoiceEngine::getInstance ()->setOtherSpeakerMute( jstring2string(env, strUserID).c_str(),  bMute);
}
/*
 * Class:     com_youme_voiceengine_api
 * Method:    setListenOtherVoice
 * Signature: (Ljava/lang/String;Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setListenOtherVoice
(JNIEnv *env, jclass className, jstring strUserID , jboolean bOn){
    return CYouMeVoiceEngine::getInstance ()->setListenOtherVoice( jstring2string(env, strUserID).c_str(),  bOn);
}


/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setModel
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setBrand (JNIEnv *env, jclass object, jstring para)
{
    g_AndroidSystemProvider->m_strBrand=(jstring2string (env, para));
}




/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setCPUChip
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setCPUChip
(JNIEnv *env, jclass, jstring para)
{
    g_AndroidSystemProvider->m_strCpuChip= (jstring2string (env, para));
}
/*
 * Class:     com_youme_voiceengine_api
 * Method:    Java_com_youme_voiceengine_api_setServerRegion
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setServerRegion
(JNIEnv *env, jclass className,jint regionId, jstring extRegionName, jboolean bAppend)
{
    CYouMeVoiceEngine::getInstance ()->setServerRegion((YOUME_RTC_SERVER_REGION)regionId, jstring2string(env,extRegionName), (bool)bAppend);
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_playBackgroundMusic
(JNIEnv * env, jclass jobj, jstring filePath, jboolean bRepeat)
{
    return CYouMeVoiceEngine::getInstance ()->playBackgroundMusic(jstring2string(env,filePath),(bool)bRepeat);
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_pauseBackgroundMusic
(JNIEnv *env, jclass jobj)
{
    return CYouMeVoiceEngine::getInstance ()->pauseBackgroundMusic();
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_resumeBackgroundMusic
(JNIEnv *env, jclass jobj)
{
    return CYouMeVoiceEngine::getInstance ()->resumeBackgroundMusic();
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_stopBackgroundMusic
(JNIEnv *env, jclass jobj)
{
    return CYouMeVoiceEngine::getInstance ()->stopBackgroundMusic();
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setBackgroundMusicVolume
(JNIEnv *env, jclass jobj, jint vol)
{
    return CYouMeVoiceEngine::getInstance ()->setBackgroundMusicVolume((int)vol);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setHeadsetMonitorOn
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setHeadsetMonitorOn__Z
(JNIEnv *env, jclass jobj, jboolean micEnabled)
{
    return CYouMeVoiceEngine::getInstance ()->setHeadsetMonitorOn((bool)micEnabled);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setHeadsetMonitorOn
 * Signature: (ZZ)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setHeadsetMonitorOn__ZZ
(JNIEnv *env, jclass jobj, jboolean micEnabled, jboolean bgmEnabled)
{
    return CYouMeVoiceEngine::getInstance ()->setHeadsetMonitorOn((bool)micEnabled, (bool)bgmEnabled);
}


JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_pauseChannel
(JNIEnv *env, jclass jobj)
{
    return CYouMeVoiceEngine::getInstance ()->pauseChannel();
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_resumeChannel
(JNIEnv *env, jclass jobj)
{
    return CYouMeVoiceEngine::getInstance ()->resumeChannel();
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setReverbEnabled
(JNIEnv *env, jclass jobj, jboolean enabled)
{
    return CYouMeVoiceEngine::getInstance ()->setReverbEnabled((bool)enabled);
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVadCallbackEnabled
(JNIEnv *env, jclass jobj, jboolean enabled)
{
    return CYouMeVoiceEngine::getInstance ()->setVadCallbackEnabled((bool)enabled);
}

JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setRecordingTimeMs
(JNIEnv *env, jclass jobj, jint timeMs)
{
    CYouMeVoiceEngine::getInstance ()->setRecordingTimeMs((unsigned int)timeMs);
}

JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setPlayingTimeMs
(JNIEnv *env, jclass jobj, jint timeMs)
{
    CYouMeVoiceEngine::getInstance ()->setPlayingTimeMs((unsigned int)timeMs);
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setMicLevelCallback
(JNIEnv *env, jclass jobj, jint maxLevel)
{
    return CYouMeVoiceEngine::getInstance ()->setMicLevelCallback((int)maxLevel);
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setFarendVoiceLevelCallback
(JNIEnv *env, jclass jobj, jint maxLevel)
{
    return CYouMeVoiceEngine::getInstance ()->setFarendVoiceLevelCallback((int)maxLevel);
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setReleaseMicWhenMute
(JNIEnv *env, jclass jobj, jboolean enabled)
{
    return CYouMeVoiceEngine::getInstance ()->setReleaseMicWhenMute((bool)enabled);
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setExitCommModeWhenHeadsetPlugin
(JNIEnv *env, jclass jobj, jboolean enabled)
{
    return CYouMeVoiceEngine::getInstance ()->setExitCommModeWhenHeadsetPlugin((bool)enabled);
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_requestRestApi
(JNIEnv *env, jclass jobj, jstring strCommand, jstring strBody)
{
    int requestID = 0;
    int ret = CYouMeVoiceEngine::getInstance ()->requestRestApi( jstring2string(env,strCommand).c_str(), jstring2string(env,strBody).c_str(),  &requestID  );
    if( ret >= 0 ){
        ret = requestID;
    }
    
    return ret;
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setGrabMicOption
 * Signature: (Ljava/lang/String;IIII)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setGrabMicOption
  (JNIEnv * env, jclass jobj, jstring channelid, jint mode, jint maxAllowCount, jint maxTalkTime, jint voteTime)
{
    return CYouMeVoiceEngine::getInstance ()->setGrabMicOption(jstring2string(env, channelid).c_str(),
    		(int)mode, (int)maxAllowCount, (int)maxTalkTime, (unsigned int)voteTime);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    startGrabMicAction
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_startGrabMicAction
  (JNIEnv * env, jclass jobj, jstring channelid, jstring content)
{
	return CYouMeVoiceEngine::getInstance ()->startGrabMicAction(jstring2string(env, channelid).c_str(),
			jstring2string(env, content).c_str());
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    stopGrabMicAction
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_stopGrabMicAction
  (JNIEnv * env, jclass jobj, jstring channelid, jstring content)
{
	return CYouMeVoiceEngine::getInstance ()->stopGrabMicAction(jstring2string(env, channelid).c_str(),
			jstring2string(env, content).c_str());
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    requestGrabMic
 * Signature: (Ljava/lang/String;IZLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_requestGrabMic
  (JNIEnv * env, jclass jobj, jstring channelid, jint score, jboolean isAutoOpenMic, jstring content)
{
	return CYouMeVoiceEngine::getInstance ()->requestGrabMic(jstring2string(env, channelid).c_str(),
			(int)score, (bool)isAutoOpenMic, jstring2string(env, content).c_str());
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    releaseGrabMic
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_releaseGrabMic
  (JNIEnv * env, jclass jobj, jstring channelid)
{
	return CYouMeVoiceEngine::getInstance ()->releaseGrabMic(jstring2string(env, channelid).c_str());
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setInviteMicOption
 * Signature: (Ljava/lang/String;II)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setInviteMicOption
  (JNIEnv * env, jclass jobj, jstring channelid, jint waitTimeout, jint maxTalkTime)
{
	return CYouMeVoiceEngine::getInstance ()->setInviteMicOption(jstring2string(env, channelid).c_str(),
			(int)waitTimeout, (int)maxTalkTime);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    requestInviteMic
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_requestInviteMic
  (JNIEnv * env, jclass jobj, jstring channelid, jstring userid, jstring content)
{
	return CYouMeVoiceEngine::getInstance ()->requestInviteMic(jstring2string(env, channelid).c_str(),
			jstring2string(env, userid).c_str(),jstring2string(env, content).c_str());
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    responseInviteMic
 * Signature: (Ljava/lang/String;ZLjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_responseInviteMic
  (JNIEnv * env, jclass jobj, jstring userid, jboolean isAccept, jstring content)
{
	return CYouMeVoiceEngine::getInstance ()->responseInviteMic(jstring2string(env, userid).c_str(),
			(bool)isAccept, jstring2string(env, content).c_str());
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    stopInviteMic
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_stopInviteMic
  (JNIEnv * env, jclass jobj)
{
	return CYouMeVoiceEngine::getInstance ()->stopInviteMic();
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    sendMessage
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_sendMessage
  (JNIEnv * env, jclass jobj, jstring channelid, jstring content){
    int requestID = 0;
    int err =  CYouMeVoiceEngine::getInstance ()->sendMessage( jstring2string(env, channelid).c_str(), jstring2string(env, content).c_str(), NULL, &requestID );
    if( err < 0 ){
        return err;
    }
    else {
        return requestID;
    }
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    sendMessageToUser
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_sendMessageToUser
  (JNIEnv * env, jclass jobj, jstring channelid, jstring content, jstring userid){
    int requestID = 0;
    int err =  CYouMeVoiceEngine::getInstance ()->sendMessage( jstring2string(env, channelid).c_str(), jstring2string(env, content).c_str(), jstring2string(env, userid).c_str(), &requestID );
    if( err < 0 ){
        return err;
    }
    else {
        return requestID;
    }
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_kickOtherFromChannel
(JNIEnv * env, jclass jobj, jstring userID , jstring channelId , jint  lastTime )
{
    return CYouMeVoiceEngine::getInstance ()->kickOther(  jstring2string(env, userID).c_str(),
                                                                       jstring2string(env, channelId).c_str(),
                                                                      lastTime );
}

JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setLogLevel
(JNIEnv * env, jclass jobj, jint consoleLevel, jint fileLevel )
{
    CYouMeVoiceEngine::getInstance ()->setLogLevel( (YOUME_LOG_LEVEL) consoleLevel, (YOUME_LOG_LEVEL) fileLevel);
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setExternalInputSampleRate
(JNIEnv * env, jclass jobj, jint inputSamplerate, jint mixedCallbackSamplerate)
{
    return IYouMeVoiceEngine::getInstance ()->setExternalInputSampleRate( (YOUME_SAMPLE_RATE)inputSamplerate, (YOUME_SAMPLE_RATE)mixedCallbackSamplerate );
}

JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setAudioQuality
(JNIEnv * env, jclass jobj, jint quality )
{
    CYouMeVoiceEngine::getInstance ()->setAudioQuality(  (YOUME_AUDIO_QUALITY) quality );
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoSmooth
(JNIEnv * env, jclass jobj, jint  mode )
{
    return CYouMeVoiceEngine::getInstance ()->setVideoSmooth(  mode  );
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoNetAdjustmode
(JNIEnv * env, jclass jobj, jint  mode )
{
    return CYouMeVoiceEngine::getInstance ()->setVideoNetAdjustmode(  mode  );
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoNetResolution
(JNIEnv * env, jclass jobj, jint  width  ,jint height )
{
    return CYouMeVoiceEngine::getInstance ()->setVideoNetResolution(  width, height  );
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoNetResolutionForSecond
(JNIEnv * env, jclass jobj, jint  width  ,jint height )
{
    return CYouMeVoiceEngine::getInstance ()->setVideoNetResolutionForSecond(  width, height  );
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoNetResolutionForShare
(JNIEnv * env, jclass jobj, jint  width  ,jint height )
{
    return CYouMeVoiceEngine::getInstance ()->setVideoNetResolutionForShare(  width, height  );
}

JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setVideoCodeBitrate
(JNIEnv * env, jclass jobj, jint maxBitrate, jint minBitrate )
{
    CYouMeVoiceEngine::getInstance ()->setVideoCodeBitrate( maxBitrate, minBitrate  );
}

JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setVideoCodeBitrateForSecond
(JNIEnv * env, jclass jobj, jint maxBitrate, jint minBitrate )
{
    CYouMeVoiceEngine::getInstance ()->setVideoCodeBitrateForSecond( maxBitrate, minBitrate  );
}

JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setVideoCodeBitrateForShare
(JNIEnv * env, jclass jobj, jint maxBitrate, jint minBitrate )
{
    CYouMeVoiceEngine::getInstance ()->setVideoCodeBitrateForShare( maxBitrate, minBitrate  );
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVBR
(JNIEnv * env, jclass jobj, jboolean useVBR)
{
    return CYouMeVoiceEngine::getInstance ()->setVBR( useVBR );
}


JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVBRForSecond
(JNIEnv * env, jclass jobj, jboolean useVBR)
{
    return  CYouMeVoiceEngine::getInstance ()->setVBRForSecond( useVBR );
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVBRForShare
(JNIEnv * env, jclass jobj, jboolean useVBR)
{
    return  CYouMeVoiceEngine::getInstance ()->setVBRForShare( useVBR );
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_getCurrentVideoCodeBitrate
(JNIEnv * env, jclass jobj)
{
    return CYouMeVoiceEngine::getInstance ()->getCurrentVideoCodeBitrate( );
}

JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setVideoHardwareCodeEnable
(JNIEnv * env, jclass jobj, jboolean bEnable )
{
    CYouMeVoiceEngine::getInstance ()->setVideoHardwareCodeEnable( bEnable );
}

JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_getVideoHardwareCodeEnable
(JNIEnv * env, jclass jobj)
{
    return CYouMeVoiceEngine::getInstance ()->getVideoHardwareCodeEnable();
}

JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_getUseGL
(JNIEnv * env, jclass jobj)
{
    return CYouMeVoiceEngine::getInstance ()->getUseGL();
}

JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setVideoNoFrameTimeout
(JNIEnv * env, jclass jobj, jint timeout)
{
    CYouMeVoiceEngine::getInstance ()->setVideoNoFrameTimeout(timeout);
}

JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setAVStatisticInterval
(JNIEnv * env, jclass jobj, jint  interval )
{
    CYouMeVoiceEngine::getInstance ()->setAVStatisticInterval(  interval  );
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setUserRole
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setUserRole
(JNIEnv * env, jclass jobj, jint role){
    return CYouMeVoiceEngine::getInstance()->setUserRole((YouMeUserRole_t)role);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    getUserRole
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_getUserRole
(JNIEnv * env, jclass jobj){
    return CYouMeVoiceEngine::getInstance()->getUserRole();
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    isInChannel
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_isInChannel
(JNIEnv * env, jclass jobj, jstring channelid){
    return (jboolean)CYouMeVoiceEngine::getInstance()->isInRoom(jstring2string(env, channelid));
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    isInChannel
 * Signature: (Ljava/lang)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_isJoined
(JNIEnv * env, jclass jobj){
    return (jboolean)CYouMeVoiceEngine::getInstance()->isInRoom();
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    isBackgroundMusicPlaying
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_isBackgroundMusicPlaying
(JNIEnv * env, jclass jobj){
    return (jboolean)CYouMeVoiceEngine::getInstance()->isBackgroundMusicPlaying();
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    isInited
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_isInited
(JNIEnv * env, jclass jobj){
    return (jboolean)CYouMeVoiceEngine::getInstance()->isInited();
}

JNIEXPORT jstring JNICALL Java_com_youme_voiceengine_api_getSdkInfo
(JNIEnv *env, jclass jobj)
{
    std::string strInfo;
    CYouMeVoiceEngine::getInstance()->getSdkInfo(strInfo);
    return env->NewStringUTF(strInfo.c_str());
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    queryUsersVideoInfo
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_queryUsersVideoInfo
(JNIEnv *env, jclass jobj, jobjectArray userArray)
{
    jstring jstr;
    jsize len = env->GetArrayLength(userArray);
    if(!len)  return -1;
    std::vector<std::string> userList;
    for (int i=0 ; i<len; i++) {
        jstr = (jstring)env->GetObjectArrayElement(userArray, i);
        std::string userid = jstring2string(env, jstr);
        userList.push_back(userid);
    }
    return CYouMeVoiceEngine::getInstance()->queryUsersVideoInfo(userList);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setUsersVideoInfo
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setUsersVideoInfo
(JNIEnv *env, jclass jobj, jobjectArray userArray, jintArray resolutionArray)
{
    jstring jstr;
    jsize len = env->GetArrayLength(userArray);
    jint *elems = env->GetIntArrayElements(resolutionArray, NULL);
    if(!elems || !len)  return -1;
    std::vector<IYouMeVoiceEngine::userVideoInfo> userVideoList;
    for (int i=0 ; i<len; i++) {
        IYouMeVoiceEngine::userVideoInfo videoInfo;
        jstr = (jstring)env->GetObjectArrayElement(userArray, i);
        videoInfo.userId  = jstring2string(env, jstr);
        videoInfo.resolutionType = elems[i];
        userVideoList.push_back(videoInfo);
    }
    env->ReleaseIntArrayElements(resolutionArray, elems, JNI_COMMIT);
    return CYouMeVoiceEngine::getInstance()->setUsersVideoInfo(userVideoList);
}


/*
 * Class:     com_youme_voiceengine_api
 * Method:    setExternalFilterEnabled
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setExternalFilterEnabled
(JNIEnv *env, jclass jobj, jboolean enabled){
    
   return CYouMeVoiceEngine::getInstance()->setExternalFilterEnabled(enabled);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoFrameRawCbEnabled
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoFrameRawCbEnabled
(JNIEnv *env, jclass jobj, jboolean enabled){
    return CYouMeVoiceEngine::getInstance()->setVideoFrameRawCbEnabled(enabled);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoDecodeRawCbEnabled
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoDecodeRawCbEnabled
(JNIEnv *env, jclass jobj, jboolean enabled){
    return CYouMeVoiceEngine::getInstance()->setVideoDecodeRawCbEnabled(enabled);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    isCameraZoomSupported
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_isCameraZoomSupported
(JNIEnv *env , jclass jobj){
   return CYouMeVoiceEngine::getInstance()->isCameraZoomSupported();
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setCameraZoomFactor
 * Signature: (F)F
 */
JNIEXPORT jfloat JNICALL Java_com_youme_voiceengine_api_setCameraZoomFactor
(JNIEnv *env, jclass jobj, jfloat zoomfactor){
    return CYouMeVoiceEngine::getInstance()->setCameraZoomFactor(zoomfactor);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    isCameraFocusPositionInPreviewSupported
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_isCameraFocusPositionInPreviewSupported
(JNIEnv *env, jclass jobj){
    return CYouMeVoiceEngine::getInstance()->isCameraFocusPositionInPreviewSupported();
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setCameraFocusPositionInPreview
 * Signature: (FF)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_setCameraFocusPositionInPreview
(JNIEnv *env, jclass jobj, jfloat x, jfloat y){
    return CYouMeVoiceEngine::getInstance()->setCameraFocusPositionInPreview(x, y);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    isCameraTorchSupported
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_isCameraTorchSupported
(JNIEnv *env, jclass jobj){
    return CYouMeVoiceEngine::getInstance()->isCameraTorchSupported();
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setCameraTorchOn
 * Signature: (Z)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_setCameraTorchOn
(JNIEnv *env, jclass jobj, jboolean enabled){
    return CYouMeVoiceEngine::getInstance()->setCameraTorchOn(enabled);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    isCameraAutoFocusFaceModeSupported
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_isCameraAutoFocusFaceModeSupported
(JNIEnv *env, jclass jobj){
     return CYouMeVoiceEngine::getInstance()->isCameraAutoFocusFaceModeSupported();
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setCameraAutoFocusFaceModeEnabled
 * Signature: (Z)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_api_setCameraAutoFocusFaceModeEnabled
(JNIEnv *env, jclass jobj, jboolean enabled){
    return CYouMeVoiceEngine::getInstance()->setCameraAutoFocusFaceModeEnabled(enabled);
}


// video jni采集接口
void JNI_Init_Video_Capturer(int nFps,
                             int nWidth,
                             int nHeight,
                             tmedia_producer_t *pProducerInstance)
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        TSK_DEBUG_ERROR("Init video capturer failed");
        
        return;
    }
    
    TSK_DEBUG_INFO("Init video capturer in java");
    
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoCapturerClass,
                                               mVideoSetCamera,
                                               nWidth,
                                               nHeight,
                                               nFps);
    m_pVideoProducer = pProducerInstance;
    
    
    video_producer_android_t *self         = NULL;
    video_android_instance_t *instance     = NULL;
    VideoAndroidDeviceCallbackImpl *cbFunc = NULL;
    
    self = (video_producer_android_t *)m_pVideoProducer;
    if (!self)
    {
        TSK_DEBUG_WARN("Invalid parameter(m_pVideoProducer == null)");
        return;
    }
    instance = (video_android_instance_t *)(self->videoInstHandle);
    if (!instance)
    {
        TSK_DEBUG_WARN("Invalid parameter(instance == null)");
        return;
    }
    cbFunc = (VideoAndroidDeviceCallbackImpl *)(instance->callback);
    if (!cbFunc)
    {
        TSK_DEBUG_WARN("Invalid parameter(cbFunc == null)");
        return;
    }
    
    ICameraManager::getInstance()->registerCameraPreviewCallback(cbFunc);
}

void JNI_Start_Video_Capturer()
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        TSK_DEBUG_ERROR("Start video capturer faild");
        
        return;
    }
    
    TSK_DEBUG_INFO("Stop video capturer in java");
    
    //jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoCapturerClass, mVideoCapturerMethodID, 1);
}

void JNI_Stop_Video_Capturer()
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        TSK_DEBUG_ERROR("Stop video capturer faild");
        
        return;
    }
    
    ICameraManager::getInstance()->unregisterCameraPreviewCallback();
    TSK_DEBUG_INFO("Stop video capturer in java");
    
    //jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoCapturerClass, mVideoCapturerMethodID, 0);
}

#if 0
void JNI_Init_Video_Renderer(int nFps,
                             int nWidth,
                             int nHeight,
                             tmedia_consumer_t *pConsumerInstance)
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        TSK_DEBUG_ERROR("Init video renderer failed");
        
        return;
    }
    
    TSK_DEBUG_INFO("Init video renderer in java");
    m_pVideoConsumer = pConsumerInstance;
}

void JNI_Refresh_Video_Render(int sessionId, int nWidth, int nHeight, int nRotationDegree, int nBufSize, const void *buf)
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        TSK_DEBUG_ERROR("Refresh video render failed");
        return;
    }
    
    jbyteArray jArray = jniWrap.m_pThreadJni->NewByteArray(nBufSize);
    jniWrap.m_pThreadJni->SetByteArrayRegion(jArray, 0, nBufSize, (const jbyte *)buf);
    
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoRendererClass, mVideoFrameRender, sessionId, nWidth, nHeight, nRotationDegree, jArray);
    jniWrap.m_pThreadJni->DeleteLocalRef(jArray);
}
#endif

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    startCapture
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_NativeEngine_startCapture
(JNIEnv *env, jclass className)
{
    //start_capture();
    return CYouMeVoiceEngine::getInstance()->startCapture();
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    stopCapture
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_stopCapture
(JNIEnv *env, jclass className)
{
    //stop_capture();
    CYouMeVoiceEngine::getInstance()->stopCapture();
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    switchCamera
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_NativeEngine_switchCamera
(JNIEnv *env, jclass className)
{
    return CYouMeVoiceEngine::getInstance()->switchCamera();
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setBeautyLevel
 * Signature: (F)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setBeautyLevel
(JNIEnv *env, jclass className, jfloat level)
{
//    if(level)
//        CYouMeVoiceEngine::getInstance()->openBeautify(true);
//    else
//        CYouMeVoiceEngine::getInstance()->openBeautify(false);
    CYouMeVoiceEngine::getInstance()->beautifyChanged(level);
}

JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_openBeautify
(JNIEnv *env, jclass className, jboolean open)
{
    CYouMeVoiceEngine::getInstance()->openBeautify( open );
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    createRender
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_createRender
(JNIEnv * env, jclass className, jstring userId)
{
    std::string userIdstr = jstring2string(env, userId);
    return CYouMeVoiceEngine::getInstance()->createRender(userIdstr);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    deleteRender
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_deleteRender
(JNIEnv * env, jclass className, jint renderId)
{
    return CYouMeVoiceEngine::getInstance()->deleteRender(renderId);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    deleteRender
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_deleteRenderByUserID
(JNIEnv * env, jclass className, jstring userId)
{
    std::string userIdstr = jstring2string(env, userId);
    return CYouMeVoiceEngine::getInstance()->deleteRender(userIdstr);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    SetVideoCallback
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoCallback
(JNIEnv * env, jclass className)
{
    TSK_DEBUG_INFO(">>> JNI setVideoCallback");
    return CYouMeVoiceEngine::getInstance()->setVideoCallback(mPYouMeVideoCallback);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setExternalInputMode
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setExternalInputMode
(JNIEnv * env, jclass className, jboolean bInputModeEnabled)
{
    CYouMeVoiceEngine::getInstance()->setExternalInputMode(bInputModeEnabled);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    openVideoEncoder
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_openVideoEncoder
(JNIEnv * env, jclass className, jstring pFilePath)
{
    std::string pFilePathStr = jstring2string(env, pFilePath);
    return CYouMeVoiceEngine::getInstance()->openVideoEncoder(pFilePathStr);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoLocalResolution
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoLocalResolution
(JNIEnv * env, jclass className, jint width, jint height)
{
    return CYouMeVoiceEngine::getInstance()->setVideoLocalResolution((int)width, (int)height);
}


/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoPreviewFps
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoPreviewFps
(JNIEnv *env, jclass className, jint fps)
{
     return CYouMeVoiceEngine::getInstance()->setVideoPreviewFps(fps);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoFps
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoFps
(JNIEnv *env, jclass className, jint fps)
{
     return CYouMeVoiceEngine::getInstance()->setVideoFps(fps);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoFpsForSecond
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoFpsForSecond
(JNIEnv *env, jclass className, jint fps)
{
     return CYouMeVoiceEngine::getInstance()->setVideoFpsForSecond(fps);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setVideoFpsForShare
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setVideoFpsForShare
(JNIEnv *env, jclass className, jint fps)
{
     return CYouMeVoiceEngine::getInstance()->setVideoFpsForShare(fps);
}

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setCaptureFrontCameraEnable
 * Signature: (Z)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setCaptureFrontCameraEnable
(JNIEnv * env, jclass className, jboolean enable)
{
    return CYouMeVoiceEngine::getInstance()->setCaptureFrontCameraEnable((bool)enable);
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    setMixVideoSize
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_setMixVideoSize
(JNIEnv * env, jclass className, jint width, jint height) {
    YouMeVideoMixerAdapter::getInstance()->setMixVideoSize(width, height);
    //YouMeEngineManagerForQiniu::getInstance()->setMixVideoSize(width, height);
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    addMixOverlayVideo
 * Signature: (Ljava/lang/String;IIIII)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_addMixOverlayVideo
(JNIEnv *env, jclass className, jstring userId, jint x, jint y, jint z, jint width, jint height) {
    std::string userIdstr = jstring2string(env, userId);
    YouMeVideoMixerAdapter::getInstance()->addMixOverlayVideo(userIdstr.c_str(), x, y, z, width, height);
    //YouMeEngineManagerForQiniu::getInstance()->addMixOverlayVideo(userIdstr, x, y, z, width, height);
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    removeMixOverlayVideo
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_removeMixOverlayVideo
(JNIEnv *env, jclass className, jstring userId) {
    std::string userIdstr = jstring2string(env, userId);
    YouMeVideoMixerAdapter::getInstance()->removeMixOverlayVideo(userIdstr.c_str());
    //YouMeEngineManagerForQiniu::getInstance()->removeMixOverlayVideo(userIdstr);
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    removeMixAllOverlayVideo
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_removeMixAllOverlayVideo
(JNIEnv *env, jclass className) {
    YouMeVideoMixerAdapter::getInstance()->removeAllOverlayVideo();
}

/*
 * Class:     com_youme_voiceengine_YouMeEngineAudioMixerUtils
 * Method:    setAudioMixerTrackSamplerate
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_YouMeEngineAudioMixerUtils_setAudioMixerTrackSamplerate
(JNIEnv *env, jclass className, jint sampleRate) {
    return YouMeEngineAudioMixerUtils::getInstance()->setAudioMixerTrackSamplerate(sampleRate);
}

/*
 * Class:     com_youme_voiceengine_YouMeEngineAudioMixerUtils
 * Method:    setAudioMixerTrackVolume
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_YouMeEngineAudioMixerUtils_setAudioMixerTrackVolume
(JNIEnv *env, jclass className, jint volume) {
    return YouMeEngineAudioMixerUtils::getInstance()->setAudioMixerTrackVolume(volume);
}

/*
 * Class:     com_youme_voiceengine_YouMeEngineAudioMixerUtils
 * Method:    setAudioMixerInputVolume
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_youme_voiceengine_YouMeEngineAudioMixerUtils_setAudioMixerInputVolume
(JNIEnv *env, jclass className, jint volume) {
    return YouMeEngineAudioMixerUtils::getInstance()->setAudioMixerInputVolume(volume);
}

/*
 * Class:     com_youme_voiceengine_YouMeEngineAudioMixerUtils
 * Method:    pushAudioMixerTrack
 * Signature: ([BIIIIZJ)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_YouMeEngineAudioMixerUtils_pushAudioMixerTrack
(JNIEnv *env, jclass className, jbyteArray audioBuf, jint sizeInByte, jint channelNum, jint sampleRate, jint bytesPerSample, jboolean bFloat, jlong timestamp) {
    if (NULL == audioBuf) {
        return false;
    }
    
    jbyte *jArray = (jbyte *)env->GetByteArrayElements(audioBuf, NULL);
    if (NULL == jArray)
    {
        TSK_DEBUG_WARN("Native layer jArray = NULL");
        return false;
    }
    
    int result = YouMeEngineAudioMixerUtils::getInstance()->pushAudioMixerTrack(jArray, sizeInByte, channelNum, sampleRate, bytesPerSample, bFloat, timestamp);
    env->ReleaseByteArrayElements(audioBuf, jArray, 0);
    return (result == 0);
}


/*
 * Class:     com_youme_voiceengine_YouMeEngineAudioMixerUtils
 * Method:    inputAudioToMix
 * Signature: (Ljava/lang/String;[BIIIIZJ)Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_voiceengine_YouMeEngineAudioMixerUtils_inputAudioToMix
(JNIEnv *env, jclass className, jstring indexId, jbyteArray audioBuf, jint sizeInByte, jint channelNum, jint sampleRate, jint bytesPerSample, jboolean bFloat, jlong timestamp) {
    if (NULL == audioBuf) {
        return false;
    }
    
    jbyte *jArray = (jbyte *)env->GetByteArrayElements(audioBuf, NULL);
    if (NULL == jArray)
    {
        TSK_DEBUG_WARN("Native layer jArray = NULL");
        return false;
    }
    
    std::string indexIdstr = jstring2string(env, indexId);
    
    int result = YouMeEngineAudioMixerUtils::getInstance()->inputAudioToMix(indexIdstr, jArray, sizeInByte, channelNum, sampleRate, bytesPerSample, bFloat, timestamp);
    env->ReleaseByteArrayElements(audioBuf, jArray, 0);
    return (result == 0);
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    sharedEGLContext
 * Signature: ()Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_com_youme_voiceengine_NativeEngine_sharedEGLContext
(JNIEnv *env, jclass className){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni){
        return NULL;
    }
    return jniWrap.m_pThreadJni->CallStaticObjectMethod(mVideoMixerHelperClass, mGetVideoMixEGLContextMethodID);
}

/*
 * Class:     com_youme_voiceengine_NativeEngine
 * Method:    logcat
 * Signature: (Ijava/lang/String;java/lang/String;)V;
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_NativeEngine_logcat
(JNIEnv *env, jclass className, jint mode, jstring tag, jstring msg){
    std::string tagstr = jstring2string(env, tag);
    std::string msgstr = jstring2string(env, msg);
    std::string text = tagstr + ": " + msgstr;
    switch (mode) {
        case 1:
            TSK_DEBUG_INFO("%s",text.c_str());
            break;
        case 2:
            TSK_DEBUG_WARN("%s",text.c_str());
            break;
        case 3:
            TSK_DEBUG_ERROR("%s",text.c_str());
            break;
        default:
            break;
    }
}


/*
 * Class:     com_youme_mixers_VideoMixerNative
 * Method:    videoFrameMixerCallback
 * Signature: (II[FIIJ)V
 */
JNIEXPORT void JNICALL Java_com_youme_mixers_VideoMixerNative_videoFrameMixerCallback
(JNIEnv * env, jclass className, jint type, jint texture, jfloatArray matrix, jint width, jint height, jlong timestamp)
{
    jfloat *jArray = NULL;
    if(matrix)
        jArray = (jfloat *)env->GetFloatArrayElements(matrix, NULL);

    if(VideoMixerCallBack){
        VideoMixerCallBack->videoFrameMixerCallback(type, texture, jArray, width, height, timestamp);
    }
    
    if(jArray)
      env->ReleaseFloatArrayElements(matrix, jArray, 0);
    
    //TSK_DEBUG_INFO("videoFrameMixerCallback t:%d, id:%d, w:%d, h:%d, ts:%lld\n", type, texture, width, height, timestamp);
}

/*
 * Class:     com_youme_mixers_VideoMixerNative
 * Method:    videoNetFirstCallback
 * Signature: (II[FIIJ)V
 */
JNIEXPORT void JNICALL Java_com_youme_mixers_VideoMixerNative_videoNetFirstCallback
(JNIEnv * env, jclass className, jint type, jint texture, jfloatArray matrix, jint width, jint height, jlong timestamp)
{
    jfloat *jArray = NULL;
    if(matrix)
        jArray = (jfloat *)env->GetFloatArrayElements(matrix, NULL);
    
    if(VideoMixerCallBack){
        VideoMixerCallBack->videoNetFirstCallback(type, texture, jArray, width, height, timestamp);
    }
    
    if(jArray)
        env->ReleaseFloatArrayElements(matrix, jArray, 0);
}

/*
 * Class:     com_youme_mixers_VideoMixerNative
 * Method:    videoNetSecondCallback
 * Signature: (II[FIIJ)V
 */
JNIEXPORT void JNICALL Java_com_youme_mixers_VideoMixerNative_videoNetSecondCallback
(JNIEnv * env, jclass className, jint type, jint texture, jfloatArray matrix, jint width, jint height, jlong timestamp)
{
    jfloat *jArray = NULL;
    if(matrix)
        jArray = (jfloat *)env->GetFloatArrayElements(matrix, NULL);
    
    if(VideoMixerCallBack){
        VideoMixerCallBack->videoNetSecondCallback(type, texture, jArray, width, height, timestamp);
    }
    
    if(jArray)
        env->ReleaseFloatArrayElements(matrix, jArray, 0);
}

JNIEXPORT void JNICALL Java_com_youme_mixers_VideoMixerNative_videoMixerUseTextureOES
(JNIEnv *env, jclass className, jboolean use){
    if(VideoMixerCallBack){
        VideoMixerCallBack->videoMixerUseTextureOES(use);
    }
}


/*
 * Class:     com_youme_mixers_VideoMixerNative
 * Method:    videoFrameMixerDataCallback
 * Signature: (I[BIIJ)V
 */
JNIEXPORT void JNICALL Java_com_youme_mixers_VideoMixerNative_videoFrameMixerDataCallback
(JNIEnv *env, jclass className, jint type, jbyteArray databuffer, jint width, jint height, jlong timestamp){
    char *tmpbuffer = NULL;
    if(databuffer)
        tmpbuffer = (char *)env->GetByteArrayElements(databuffer, NULL);
    
    int len = env->GetArrayLength(databuffer);
    if(VideoMixerCallBack){
        VideoMixerCallBack->videoFrameMixerRawCallback(type, tmpbuffer, len, width, height, timestamp);
    }
    
    if(tmpbuffer)
        env->ReleaseByteArrayElements(databuffer, (jbyte*)tmpbuffer, 0);
}

/*
 * Class:     com_youme_mixers_VideoMixerNative
 * Method:    videoNetDataFirstCallback
 * Signature: (I[BIIJ)V
 */
JNIEXPORT void JNICALL Java_com_youme_mixers_VideoMixerNative_videoNetDataFirstCallback
(JNIEnv *env, jclass className, jint type, jbyteArray databuffer, jint width, jint height, jlong timestamp){
    char *tmpbuffer = NULL;
    if(databuffer)
        tmpbuffer = (char *)env->GetByteArrayElements(databuffer, NULL);
    
    int len = env->GetArrayLength(databuffer);
    if(VideoMixerCallBack){
        VideoMixerCallBack->videoNetFirstRawCallback(type, tmpbuffer, len, width, height, timestamp);
    }
    
    if(tmpbuffer)
        env->ReleaseByteArrayElements(databuffer, (jbyte*)tmpbuffer, 0);
}

/*
 * Class:     com_youme_mixers_VideoMixerNative
 * Method:    videoNetDataSecondCallback
 * Signature: (I[BIIJ)V
 */
JNIEXPORT void JNICALL Java_com_youme_mixers_VideoMixerNative_videoNetDataSecondCallback
(JNIEnv *env, jclass className, jint type, jbyteArray databuffer, jint width, jint height, jlong timestamp){
    char *tmpbuffer = NULL;
    if(databuffer)
        tmpbuffer = (char *)env->GetByteArrayElements(databuffer, NULL);
    
    int len = env->GetArrayLength(databuffer);
    if(VideoMixerCallBack){
        VideoMixerCallBack->videoNetSecondRawCallback(type,  tmpbuffer, len, width, height, timestamp);
    }
    
    if(tmpbuffer)
        env->ReleaseByteArrayElements(databuffer, (jbyte*)tmpbuffer, 0);
}


/*
 * Class:     com_youme_mixers_VideoMixerNative
 * Method:    videoRenderFilterCallback
 * Signature: (IIIII)I
 */
JNIEXPORT jint JNICALL Java_com_youme_mixers_VideoMixerNative_videoRenderFilterCallback
(JNIEnv *env, jclass className, jint texture, jint width, jint height, jint rotation, jint mirror){
    
    if(VideoMixerCallBack){
       return  VideoMixerCallBack->videoRenderFilterCallback( texture, width, height, rotation, mirror);
    }
    return 0;
}


JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_startPush
  (JNIEnv *env, jclass className, jstring pushUrl)
  {
     return  CYouMeVoiceEngine::getInstance()->startPush( jstring2string(env,pushUrl) );
  }

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_stopPush
  (JNIEnv *env, jclass className)
  {
     return  CYouMeVoiceEngine::getInstance()->stopPush();
  }


JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_setPushMix
  (JNIEnv *env, jclass className,  jstring pushUrl , jint width , jint height )
  {
    return  CYouMeVoiceEngine::getInstance()->setPushMix( jstring2string(env,pushUrl), width , height );
  }


JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_clearPushMix
  (JNIEnv *env, jclass className)
  {
    return  CYouMeVoiceEngine::getInstance()->clearPushMix(  );
  }

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_addPushMixUser
  (JNIEnv *env, jclass className, jstring userId , jint x, jint y , jint z , jint width , jint height )
  {
return  CYouMeVoiceEngine::getInstance()->addPushMixUser( jstring2string(env,userId), x , y , z , width , height   );
  }


JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_removePushMixUser
  (JNIEnv *env, jclass className, jstring userId )
  {
return  CYouMeVoiceEngine::getInstance()->removePushMixUser(  jstring2string(env,userId)  );
  }

/*
 * Class:     com_youme_voiceengine_api
 * Method:    setScreenSharedEGLContext
 * Signature: ()Ljava/lang/Object;
 */
JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setScreenSharedEGLContext
(JNIEnv *env, jclass className,jobject elgOBJ){
    jobject globalRef = env->NewGlobalRef(elgOBJ);
    return CYouMeVoiceEngine::getInstance()->setScreenSharedEGLContext(globalRef);
}

void JNI_setVideoMixEGLContext(glObject object)
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mSetVideoMixEGLContextMethodID, object);
}

glObject JNI_getVideoMixEGLContext(){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni){
        return NULL;
    }
    jobject object = jniWrap.m_pThreadJni->CallStaticObjectMethod(mVideoMixerHelperClass, mGetVideoMixEGLContextMethodID);
    if(object){
       jobject new_object = jniWrap.m_pThreadJni->NewGlobalRef(object);
       return (glObject)new_object;
    }
    return object;
}

void JNI_deleteVideoMixEGLContext(glObject object){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni){
        return ;
    }
    if(object){
        jniWrap.m_pThreadJni->DeleteGlobalRef((jobject)object);
    }
}

void JNI_initMixer(){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni){
        return;
    }
   jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mInitMixerMethodID);
   
}

void JNI_setVideoMixSize(int width, int height){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mSetVideoMixSizeMethodID, width, height);
}

void JNI_setVideoNetResolutionWidth(int width, int height){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mSetVideoNetResolutionWidthMethodID, width, height);
}

void JNI_setVideoNetResolutionWidthForSecond(int width, int height){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mSetVideoNetResolutionWidthForSecondMethodID, width, height);
}

void JNI_addMixOverlayVideo(std::string userId, int x, int y, int z, int width, int height){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    jstring str = string2jstring(jniWrap.m_pThreadJni, userId.c_str());
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mAddMixOverlayVideoMethodID, str, x,y,z,width,height);
}

void JNI_removeMixOverlayVideo(std::string userId){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    jstring str = string2jstring(jniWrap.m_pThreadJni, userId.c_str());
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mRemoveMixOverlayVideoMethodID, str);
}

void JNI_removeAllOverlayVideo(){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mRemoveAllOverlayVideoMethodID);
}

void JNI_pushVideoFrameGLESForLocal(std::string userId, int type, int texture, float* matrix, int width, int height, int rotation, int mirror, uint64_t timestamp) {
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    jstring str = string2jstring(jniWrap.m_pThreadJni, userId.c_str());
    if (matrix != NULL) {
        jfloatArray jArray = jniWrap.m_pThreadJni->NewFloatArray(16);
        jniWrap.m_pThreadJni->SetFloatArrayRegion(jArray, 0, 16, (const jfloat *)matrix);
        jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mPushVideoFrameGLESForLocalMethodID, str, type, texture,\
                                                jArray,width,height,rotation,\
                                                mirror,(jlong)timestamp);
        jniWrap.m_pThreadJni->DeleteLocalRef(jArray);
    } else {
        jobject jArray;
        jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mPushVideoFrameGLESForLocalMethodID, str, type, texture,\
                                                   jArray,width,height,rotation,\
                                                   mirror,(jlong)timestamp);
    }
}

void JNI_pushVideoFrameRawForLocal(std::string userId, int type, const void* buff, int buffSize, int width, int height,  int rotation, int mirror, uint64_t timestamp) {
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    jstring str = string2jstring(jniWrap.m_pThreadJni, userId.c_str());
    jbyteArray jArray = jniWrap.m_pThreadJni->NewByteArray(buffSize);
    jniWrap.m_pThreadJni->SetByteArrayRegion(jArray, 0, buffSize, (const jbyte *)buff);
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mPushVideoFrameRawForLocalMethodID, str, type, jArray,\
                                               buffSize,width,height,rotation,mirror,(jlong)timestamp);
    jniWrap.m_pThreadJni->DeleteLocalRef(jArray);
    
}


void JNI_pushVideoFrameGLESForRemote(std::string userId, int type, int texture, float* matrix, int width, int height, uint64_t timestamp)
{
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    //TSK_DEBUG_INFO("JNI_pushVideoFrameGLESForRemote userid:%s\n", userId.c_str());
    jstring str = string2jstring(jniWrap.m_pThreadJni, userId.c_str());
    if(matrix){
    jfloatArray jArray = jniWrap.m_pThreadJni->NewFloatArray(16);
    jniWrap.m_pThreadJni->SetFloatArrayRegion(jArray, 0, 16, (const jfloat *)matrix);
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mPushVideoFrameGLESForRemoteMethodID, str, type, texture, jArray,\
                                               width,height,(jlong)timestamp);
    jniWrap.m_pThreadJni->DeleteLocalRef(jArray);
    }
    else{
        jobject jArray;
        jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mPushVideoFrameGLESForRemoteMethodID, str, type, texture, jArray,\
                                                   width,height,(jlong)timestamp);
    }
}



void JNI_pushVideoFrameRawForRemote(std::string userId, int type, const void* buff, int buffSize, int width, int height,  uint64_t timestamp) {
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    jstring str = string2jstring(jniWrap.m_pThreadJni, userId.c_str());
    jbyteArray jArray = jniWrap.m_pThreadJni->NewByteArray(buffSize);
    jniWrap.m_pThreadJni->SetByteArrayRegion(jArray, 0, buffSize, (const jbyte *)buff);
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mPushVideoFrameRawForRemoteMethodID, str, type, jArray,\
                                               buffSize,width,height,(jlong)timestamp);
    jniWrap.m_pThreadJni->DeleteLocalRef(jArray);
   
}

void JNI_openBeautify() {
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mMixOpenBeautifyMethodID);
}

void JNI_closeBeautify(){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mCloseBeautifyMethodID);
}

void JNI_setBeautifyLevel(float Level){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mSetBeautifyMethodID, Level);
}

void JNI_onPause(){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mOnPauseMethodID);
}

void JNI_onResume(){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mOnResumeMethodID);
}

void JNI_mixExit(){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mMixExitMethodID);
}


void JNI_setVideoFrameMixCallback(IAndroidVideoMixerCallBack* cb){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mSetVideoFrameMixCallbackMethodID);
    VideoMixerCallBack = cb;
}

void JNI_setVideoFps(int fps){
    
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return;
    }
    jniWrap.m_pThreadJni->CallStaticVoidMethod(mVideoMixerHelperClass, mSetVideoFpsMethodID, fps);
}

bool JNI_setOpenMixerRawCallBack(bool enabled){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return false;
    }
   return  jniWrap.m_pThreadJni->CallStaticBooleanMethod(mJavaApiClass, mSetOpenMixerRawCallBackMethodID, enabled);
}

bool JNI_setOpenEncoderRawCallBack(bool enabled){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return false;
    }
    return jniWrap.m_pThreadJni->CallStaticBooleanMethod(mVideoMixerHelperClass, mSetOpenEncoderRawCallBackMethodID, enabled);
}


bool JNI_setExternalFilterEnabled(bool enabled){
    JNIEvnWrap jniWrap;
    if (NULL == jniWrap.m_pThreadJni)
    {
        return false;
    }
    return jniWrap.m_pThreadJni->CallStaticBooleanMethod(mVideoMixerHelperClass, mSetExternalFilterEnabledMethodID, enabled);

}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_switchResolutionForLandscape
(JNIEnv *env, jclass className)
{
	return  CYouMeVoiceEngine::getInstance()->switchResolutionForLandscape();
}

JNIEXPORT void JNICALL Java_com_youme_voiceengine_api_setlocalVideoPreviewMirror
(JNIEnv *env, jclass className, jboolean enable)
{
    IYouMeVoiceEngine::getInstance()->setLocalVideoPreviewMirror(enable);
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_resetResolutionForPortrait
(JNIEnv *env, jclass className)
{
	return  CYouMeVoiceEngine::getInstance()->resetResolutionForPortrait();
}

JNIEXPORT jint JNICALL Java_com_youme_voiceengine_api_translateText
(JNIEnv *env, jclass className,  jstring text, int  destLangCode, int srcLangCode )
{
    unsigned int  requestId = 0;
    int ret =   IYouMeVoiceEngine::getInstance()->translateText( &requestId,  jstring2string(env, text).c_str() , (YouMeLanguageCode)destLangCode, (YouMeLanguageCode)srcLangCode );
    if( ret < 0 )
    {
        return ret;
    }
    else{
        return (int)requestId;
    }
    
}
