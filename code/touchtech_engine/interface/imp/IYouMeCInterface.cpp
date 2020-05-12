//
//  IYouMeCInterface.cpp
//  youme_voice_engine
//
//  Created by YouMe.im on 15/12/10.
//  Copyright © 2015年 youme. All rights reserved.
//
#define  _DLLExport

#include <memory>
#include <mutex>
#include <YouMeCommon/XAny.h>
#include <YouMeCommon/json/json.h>
#include <YouMeCommon/StringUtil.hpp>
#include "tsk_debug.h"
#include "tmedia_utils.h"

#include "IYouMeCInterface.h"
#include "IYouMeVoiceEngine.h"
#include "TLSMemory.h"
#include <YouMeCommon/yuvlib/libyuv.h>
#if __APPLE__
#include <CoreVideo/CoreVideo.h>
#endif

VideoFrameCallbackForFFI videoFrameCallbackForFFICallback = NULL;
VideoEventCallbackForFFI videoEventCallbackForFFICallback = NULL;

extern void SetSoundtouchEnabled(bool bEnabled);
extern bool GetSoundtouchEnabled();
extern float GetSoundtouchTempo();
extern void SetSoundtouchTempo(float nTempo);
extern float GetSoundtouchRate();
extern void SetSoundtouchRate(float nRate);
extern float GetSoundtouchPitch();
extern void SetSoundtouchPitch(float nPitch);
extern void SetServerMode(SERVER_MODE serverMode);
extern void SetServerIpPort(const char* ip, int port);

std::string g_videoCallbackName;

std::recursive_mutex *copyMutex=new std::recursive_mutex;

#define CLAMP(i)   (((i) < 0) ? 0 : (((i) > 255) ? 255 : (i)))

// 如果直接定义一个Mutex对象，在iOS上有一个问题，当App退后台时，所有全局对象会被析构，如果这时 Mutex 是被锁住的状态，
// 会触发 Abort 信号并闪退。

typedef enum CallbackType {
    CALLBACK_TYPE_EVENT = 0,
    CALLBACK_TYPE_REST_API_RESPONSE,
    CALLBACK_TYPE_MEM_CHANGE,
    CALLBACK_TYPE_BROADCAST,
    CALLBACK_TYPE_AVSTATISTIC,
    CALLBACK_TYPE_SENTENCE_BEGIN,
    CALLBACK_TYPE_SENTENCE_CHANGED,
    CALLBACK_TYPE_SENTENCE_END,
    CALLBACK_TYPE_TRANSLATE,
} CallbackType_t;

static std::mutex* g_pMsgQueueMutex = NULL;
static std::list<XString> g_msgQueue;
static std::string g_strJoinUserId;

class InterImpEventCallback: public IYouMeEventCallback, public IRestApiCallback, public IYouMeMemberChangeCallback,
public IYouMeChannelMsgCallback, public IYouMeAVStatisticCallback, public IYouMeTranscriberCallback, public IYouMeTranslateCallback,public IYouMePcmCallback 
{
public:
    EventCb             m_eventCallback = nullptr;
    RequestRestAPICb    m_restApiCallback = nullptr;
    MemberChangeCb      m_memberChangeCallback = nullptr;
    BroadcastCb         m_broadcastCallback = nullptr;
    AVStatisticCb       m_staticsCallback= nullptr;
    SentenceBeginCb     m_sentenceBeginCallback= nullptr;
    SentenceEndCb       m_sentenceEndCallback= nullptr;
    SentenceChangedCb   m_sentenceChangedCallback= nullptr;
    TranslateTextCompleteCb  m_translateTextCompleteCb = nullptr;
	OnPcmDataCallback   m_pcmCallback = nullptr;
    bool                b_useCb = false;

public:
    void setcallback(EventCb YMEventCallback,
                     RequestRestAPICb YMRestApiCallback,
                     MemberChangeCb YMMemberChangeCallback,
                     BroadcastCb YMBroadcastCallback,
                     AVStatisticCb YMAVStatisticCallback,
                     SentenceBeginCb YMSentenceBeginCallback,
                     SentenceEndCb YMSentenceEndCallback,
                     SentenceChangedCb YMSentenceChangedCallback,
                     TranslateTextCompleteCb  YMTranslateTextCompleteCallback)
    {
        if (!YMEventCallback && !YMRestApiCallback 
            && !YMMemberChangeCallback && !YMBroadcastCallback 
            && !YMAVStatisticCallback) {

            return;
        }

        b_useCb                 = true;
        m_eventCallback         = YMEventCallback;
        m_restApiCallback       = YMRestApiCallback;
        m_memberChangeCallback  = YMMemberChangeCallback;
        m_broadcastCallback     = YMBroadcastCallback;
        m_staticsCallback       = YMAVStatisticCallback;
        m_sentenceBeginCallback = YMSentenceBeginCallback;
        m_sentenceEndCallback   = YMSentenceEndCallback;
        m_sentenceChangedCallback = YMSentenceChangedCallback;
        m_translateTextCompleteCb = YMTranslateTextCompleteCallback;
    }

	void setPCMCallback(OnPcmDataCallback pcmCallback){
		m_pcmCallback = pcmCallback;
	}

	virtual void onPcmDataRemote(int channelNum, int samplingRateHz, int bytesPerSample, void* data, int dataSizeInByte) override
	{
		if (m_pcmCallback != nullptr)
		{
			m_pcmCallback(channelNum, samplingRateHz, bytesPerSample, data, dataSizeInByte, 1);
		}
	}
	//本地录音pcm数据回调
	virtual void onPcmDataRecord(int channelNum, int samplingRateHz, int bytesPerSample, void* data, int dataSizeInByte) override
	{
		if (m_pcmCallback != nullptr)
		{
			//TSK_DEBUG_INFO("@@ onPcmDataRecord %d %d", channelNum, samplingRateHz);
			m_pcmCallback(channelNum, samplingRateHz, bytesPerSample, data, dataSizeInByte, 2);
		}
	}
	//远端pcm和本地录音pcm的混合pcm数据回调
	virtual void onPcmDataMix(int channelNum, int samplingRateHz, int bytesPerSample, void* data, int dataSizeInByte) override
	{
		if (m_pcmCallback != nullptr)
		{
			m_pcmCallback(channelNum, samplingRateHz, bytesPerSample, data, dataSizeInByte, 4);
		}
	}

    virtual void onEvent(const YouMeEvent event, const YouMeErrorCode error, const char * channel, const char * param) override
    {
        if (NULL == g_pMsgQueueMutex) {
            return;
        }
        
        //char buffer[512] = {0};
        //snprintf(buffer, sizeof(buffer), "%d,%d,%s,%s", event, error, channel, param); // use json ?
        
        if (b_useCb) {
            if (m_eventCallback) {
                m_eventCallback(event, error, channel, param);
            }

            return;
        }

        youmecommon::Value jsonRoot;
        jsonRoot["type"] = youmecommon::Value((int)CALLBACK_TYPE_EVENT);
        jsonRoot["event"] = youmecommon::Value((int)event);
        jsonRoot["error"] = youmecommon::Value((int)error);
        jsonRoot["channelid"]  = youmecommon::Value(channel);
        jsonRoot["param"] = youmecommon::Value(param);
        
        if (videoEventCallbackForFFICallback != NULL)
        {
            TSK_DEBUG_INFO("@@ videoEventCallbackForFFICallback onEvent");
            std::string strUtf8Msg = XStringToUTF8(jsonRoot.toSimpleString());
            videoEventCallbackForFFICallback(strUtf8Msg.c_str());
        }else{
            std::lock_guard<std::mutex> lock(*g_pMsgQueueMutex);
            g_msgQueue.push_back(jsonRoot.toSimpleString());
        }
    }
    
    virtual void onRequestRestAPI(int requestID, const YouMeErrorCode &iErrorCode, const char* strQuery,  const char* strResult) override
    {
        if (NULL == g_pMsgQueueMutex) {
            return;
        }
        
        if (b_useCb) {
            if (m_restApiCallback) {
                m_restApiCallback(requestID, iErrorCode, strQuery, strResult);
            }

            return;
        }
        //char buffer[512] = {0};
        //snprintf(buffer, sizeof(buffer), "%d,%d,%s,%s", requestID, iErrorCode, strQuery.c_str(), strResult.c_str());
        
        youmecommon::Value jsonRoot;
        jsonRoot["type"] = youmecommon::Value((int)CALLBACK_TYPE_REST_API_RESPONSE);
        jsonRoot["requestid"] = youmecommon::Value((int)requestID);
        jsonRoot["error"] = youmecommon::Value((int)iErrorCode);
        jsonRoot["query"]  = youmecommon::Value(strQuery);
        jsonRoot["result"] = youmecommon::Value(strResult);
        
        if (videoEventCallbackForFFICallback != NULL)
        {
            TSK_DEBUG_INFO("@@ videoEventCallbackForFFICallback onRequestRestAPI");
            std::string strUtf8Msg = XStringToUTF8(jsonRoot.toSimpleString());
            videoEventCallbackForFFICallback(strUtf8Msg.c_str());
        }else{
            std::lock_guard<std::mutex> lock(*g_pMsgQueueMutex);
            g_msgQueue.push_back(jsonRoot.toSimpleString());
        }
    }
    
    virtual void onMemberChange( const char*  channel, const char* userID,bool isJoin, bool bUpdate) override
    {
        if (NULL == g_pMsgQueueMutex) {
            return;
        }
        
        if (b_useCb) {
            if (m_memberChangeCallback) {
                char * userID = nullptr;
                bool isJoin = false;
                m_memberChangeCallback(channel, userID, isJoin, bUpdate);
            }
        }
    }
    
    virtual void onMemberChange( const char*  channel, const char* listMemberChange, bool bUpdate ) override
    {
        if (NULL == g_pMsgQueueMutex) {
            return;
        }
    
        if (b_useCb) {
            return;
        }
        
        if (videoEventCallbackForFFICallback != NULL)
        {
            TSK_DEBUG_INFO("@@ videoEventCallbackForFFICallback onMemberChange");
            videoEventCallbackForFFICallback(listMemberChange);
        }else{
            std::lock_guard<std::mutex> lock(*g_pMsgQueueMutex);
            g_msgQueue.push_back(UTF8TOXString2(listMemberChange));
        }
    }
    
    virtual void onBroadcast(const YouMeBroadcast bc, const char*  channel, const char*  param1, const char*  param2,const char*  strContent) override
    {
        if (NULL == g_pMsgQueueMutex) {
            return;
        }
        
        //char buffer[512] = {0};
        //snprintf(buffer, sizeof(buffer), "%d,%d,%s,%s", event, error, channel, param); // use json ?
        if (b_useCb) {
            if (m_broadcastCallback) {
                m_broadcastCallback(bc, channel, param1, param2, strContent);
            }

            return;
        }
        
        youmecommon::Value jsonRoot;
        jsonRoot["type"] = youmecommon::Value((int)CALLBACK_TYPE_BROADCAST);
        jsonRoot["bc"] = youmecommon::Value((int)bc);
        jsonRoot["channelid"] = youmecommon::Value(channel);
        jsonRoot["param1"] = youmecommon::Value(param1);
        jsonRoot["param2"] = youmecommon::Value(param2);
        jsonRoot["content"] = youmecommon::Value(strContent);
        
        if (videoEventCallbackForFFICallback != NULL)
        {
            TSK_DEBUG_INFO("@@ videoEventCallbackForFFICallback onBroadcast");
            std::string strUtf8Msg = XStringToUTF8(jsonRoot.toSimpleString());
            videoEventCallbackForFFICallback(strUtf8Msg.c_str());
        }else{
            std::lock_guard<std::mutex> lock(*g_pMsgQueueMutex);
            g_msgQueue.push_back(jsonRoot.toSimpleString());
        }
    }
    
    virtual void onAVStatistic( YouMeAVStatisticType type,  const char* userID,  int value ) override
    {
        if (NULL == g_pMsgQueueMutex) {
            return;
        }
        
        if (b_useCb) {
            if (m_staticsCallback) {
                m_staticsCallback(type, userID, value);
            }

            return;
        }

        youmecommon::Value jsonRoot;
        jsonRoot["type"] = youmecommon::Value( (int)CALLBACK_TYPE_AVSTATISTIC );
        jsonRoot["avtype"] = youmecommon::Value( (int)type );
        jsonRoot["userid"] = youmecommon::Value( userID );
        jsonRoot["value"] = youmecommon::Value( value );
        
        if (videoEventCallbackForFFICallback != NULL)
        {
            TSK_DEBUG_INFO("@@ videoEventCallbackForFFICallback onAVStatistic");
            std::string strUtf8Msg = XStringToUTF8(jsonRoot.toSimpleString());
            videoEventCallbackForFFICallback(strUtf8Msg.c_str());
        }else{
            std::lock_guard<std::mutex> lock(*g_pMsgQueueMutex);
            g_msgQueue.push_back(jsonRoot.toSimpleString());
        }
    }
    
    virtual void onSentenceBegin( std::string userid , int sentenceIndex) override
    {
        if (NULL == g_pMsgQueueMutex) {
            return;
        }
        
        if (b_useCb) {
            if ( m_sentenceBeginCallback) {
                m_sentenceBeginCallback( userid.c_str(), sentenceIndex);
            }
            
            return;
        }
        
        youmecommon::Value jsonRoot;
        jsonRoot["type"] = youmecommon::Value( (int)CALLBACK_TYPE_SENTENCE_BEGIN );
        jsonRoot["userid"] = youmecommon::Value( userid );
        jsonRoot["sentence_index"] = youmecommon::Value( sentenceIndex );
        
        if (videoEventCallbackForFFICallback != NULL)
        {
            TSK_DEBUG_INFO("@@ videoEventCallbackForFFICallback onSentenceBegin");
            std::string strUtf8Msg = XStringToUTF8(jsonRoot.toSimpleString());
            videoEventCallbackForFFICallback(strUtf8Msg.c_str());
        }else{
            std::lock_guard<std::mutex> lock(*g_pMsgQueueMutex);
            g_msgQueue.push_back(jsonRoot.toSimpleString());
        }
    }
	virtual void onSentenceEnd(std::string userid, int sentenceIndex, std::string result, std::string transLang = "", std::string transResult = "") override
    {
		TSK_DEBUG_INFO("@@ onSentenceEnd:%s", result.c_str());
        if (NULL == g_pMsgQueueMutex) {
            return;
        }
        
        if (b_useCb) {
            if ( m_sentenceEndCallback) {
                m_sentenceEndCallback( userid.c_str(), sentenceIndex, result.c_str(),transLang.c_str(), transResult.c_str());
            }
            
            return;
        }
        
        youmecommon::Value jsonRoot;
        jsonRoot["type"] = youmecommon::Value( (int)CALLBACK_TYPE_SENTENCE_END );
        jsonRoot["userid"] = youmecommon::Value( userid );
        jsonRoot["sentence_index"] = youmecommon::Value( sentenceIndex );
        jsonRoot["result"] = youmecommon::Value( result );
		jsonRoot["transLang"] = youmecommon::Value(transLang);
		jsonRoot["transResult"] = youmecommon::Value(transResult);
        
        if (videoEventCallbackForFFICallback != NULL)
        {
            TSK_DEBUG_INFO("@@ videoEventCallbackForFFICallback onSentenceEnd");
            std::string strUtf8Msg = XStringToUTF8(jsonRoot.toSimpleString());
            videoEventCallbackForFFICallback(strUtf8Msg.c_str());
        }else{
            std::lock_guard<std::mutex> lock(*g_pMsgQueueMutex);
            g_msgQueue.push_back(jsonRoot.toSimpleString());
        }
    }
	virtual void onSentenceChanged(std::string userid, int sentenceIndex, std::string result, std::string transLang = "", std::string transResult = "") override
    {
        if (NULL == g_pMsgQueueMutex) {
            return;
        }
        
        if (b_useCb) {
            if ( m_sentenceChangedCallback) {
				m_sentenceChangedCallback(userid.c_str(), sentenceIndex, result.c_str(), transLang.c_str(), transResult.c_str());
            }
            
            return;
        }
        
        youmecommon::Value jsonRoot;
        jsonRoot["type"] = youmecommon::Value( (int)CALLBACK_TYPE_SENTENCE_CHANGED );
        jsonRoot["userid"] = youmecommon::Value( userid );
        jsonRoot["sentence_index"] = youmecommon::Value( sentenceIndex );
        jsonRoot["result"] = youmecommon::Value( result );
		jsonRoot["transLang"] = youmecommon::Value(transLang);
		jsonRoot["transResult"] = youmecommon::Value(transResult);
        
        if (videoEventCallbackForFFICallback != NULL)
        {
            TSK_DEBUG_INFO("@@ videoEventCallbackForFFICallback onSentenceChanged");
            std::string strUtf8Msg = XStringToUTF8(jsonRoot.toSimpleString());
            videoEventCallbackForFFICallback(strUtf8Msg.c_str());
        }else{
            std::lock_guard<std::mutex> lock(*g_pMsgQueueMutex);
            g_msgQueue.push_back(jsonRoot.toSimpleString());
        }
    }
    
    virtual void onTranslateTextComplete(YouMeErrorCode errorcode, unsigned int requestID, const std::string& text, YouMeLanguageCode srcLangCode, YouMeLanguageCode destLangCode)
    {
        if (NULL == g_pMsgQueueMutex) {
            return;
        }
        
        if (b_useCb) {
            if ( m_translateTextCompleteCb) {
                m_translateTextCompleteCb( errorcode, requestID, text.c_str(), srcLangCode, destLangCode);
            }
            
            return;
        }
        
        youmecommon::Value jsonRoot;
        jsonRoot["type"] = youmecommon::Value( (int)CALLBACK_TYPE_TRANSLATE );
        jsonRoot["error"] = youmecommon::Value( errorcode );
        jsonRoot["requestid"] = youmecommon::Value( requestID );
        jsonRoot["text"] = youmecommon::Value( text );
        jsonRoot["srclangcode"] = youmecommon::Value( srcLangCode );
        jsonRoot["destlangcode"] = youmecommon::Value( destLangCode );
        
        if (videoEventCallbackForFFICallback != NULL)
        {
            TSK_DEBUG_INFO("@@ videoEventCallbackForFFICallback onTranslateTextComplete");
            std::string strUtf8Msg = XStringToUTF8(jsonRoot.toSimpleString());
            videoEventCallbackForFFICallback(strUtf8Msg.c_str());
        }else{
            std::lock_guard<std::mutex> lock(*g_pMsgQueueMutex);
            g_msgQueue.push_back(jsonRoot.toSimpleString());
        }
        
    }
};
YOUME_API const char* youme_getCbMessage2()
{
    if (NULL == g_pMsgQueueMutex)
    {
        return NULL;
    }
    
    std::lock_guard<std::mutex> lock(*g_pMsgQueueMutex);
    if (g_msgQueue.size() <= 0) {
        return NULL;
    }
    XString xstrMsg = *g_msgQueue.begin();
    std::string strUtf8Msg = XStringToUTF8(xstrMsg);
    
    
    char* pszTmpBuffer = new char[strUtf8Msg.length() + 1];
    strcpy(pszTmpBuffer, strUtf8Msg.c_str());
    g_msgQueue.pop_front();
    return pszTmpBuffer;
}

YOUME_API void youme_freeCbMessage2(const char* pMsg)
{
    delete[] pMsg;
}
const XCHAR* youme_getCbMessage()
{
    if (NULL == g_pMsgQueueMutex)
    {
        return NULL;
    }
    
    std::lock_guard<std::mutex> lock(*g_pMsgQueueMutex);
    if (g_msgQueue.size() <= 0) {
        return NULL;
    }
    
    XCHAR* pszTmpBuffer = new XCHAR[g_msgQueue.begin()->length() + 1];
    XStrCpy_S(pszTmpBuffer, int(g_msgQueue.begin()->length() + 1), g_msgQueue.begin()->c_str());
    g_msgQueue.pop_front();
    return pszTmpBuffer;
    
}

void youme_freeCbMessage(const XCHAR* pMsg)
{
    if (pMsg) {
        delete[] pMsg;
        pMsg = NULL;
    }
}



InterImpEventCallback* g_InterImpCallback = NULL;

void youme_setExternalInputMode( bool bInputModeEnabled )
{
    IYouMeVoiceEngine::getInstance()->setExternalInputMode(bInputModeEnabled);
}

int youme_init(const char* strAPPKey, const char* strAPPSecret,
               YOUME_RTC_SERVER_REGION serverRegionId, const char* pExtServerRegionName)
{
    if (NULL == g_InterImpCallback) {
        g_InterImpCallback = new InterImpEventCallback;
    }
    
    if (NULL == g_pMsgQueueMutex) {
        g_pMsgQueueMutex = new std::mutex();
    }
    g_msgQueue.clear();
    
    IYouMeVoiceEngine::getInstance()->setRestApiCallback( g_InterImpCallback );
    IYouMeVoiceEngine::getInstance()->setMemberChangeCallback( g_InterImpCallback );
    IYouMeVoiceEngine::getInstance()->setNotifyCallback( g_InterImpCallback );
    IYouMeVoiceEngine::getInstance()->setAVStatisticCallback( g_InterImpCallback );
    IYouMeVoiceEngine::getInstance()->setTranslateCallback( g_InterImpCallback );
    return IYouMeVoiceEngine::getInstance()->init(g_InterImpCallback, strAPPKey, strAPPSecret, serverRegionId, pExtServerRegionName);
}

/* video callback for unity */
class InterImpVideoCallback : public IYouMeVideoFrameCallback {
private:
    InterImpVideoCallback() { /* */ }
    ~InterImpVideoCallback() {
        std::lock_guard<std::recursive_mutex> stateLock(*copyMutex);
        for (std::map<std::string, I420Frame*>::iterator iter = renders.begin(); iter != renders.end(); iter++ )
        {
            I420Frame* frame = iter->second;
            
            if (frame->data != NULL) {
                delete [] frame->data;
            }
            
            if (frame->datacopy != NULL) {
                delete [] frame->datacopy;
            }
            
            delete frame;
        }
        renders.clear();
		videoFrameCallbackForFFICallback = NULL;
        videoEventCallbackForFFICallback = NULL;
    }
    
public:
    static InterImpVideoCallback * pInstance;
    
public:
    static InterImpVideoCallback * getInstance()
    {
        if (pInstance == NULL) {
            pInstance = new InterImpVideoCallback();
        }
        return pInstance;
    }
    static void destory()
    {
        if (pInstance != NULL) {
            delete pInstance;
        }
        
        pInstance = NULL;
    }
    
public:
    std::map<std::string, I420Frame*> renders;
    typedef unsigned char  uint_8;
    typedef unsigned int   uint_32;
    
    bool isEngine = true;
    
public:
    void onVideoFrameCallback(const char* userId, void * data, int len, int width, int height, int fmt, uint64_t timestamp) override{
        frameRender(userId, width, height, fmt, 0, len, data);
    }
	void onVideoFrameMixedCallback(void * data, int len, int width, int height, int fmt, uint64_t timestamp)override{
        frameRender(g_strJoinUserId, width, height, fmt, 0, len, data);
    }
    
    //引擎不支持纹理渲染，先用rgb吧
	void onVideoFrameCallbackGLESForIOS(const char* userId, void* pixelBuffer, int width, int height, int fmt, uint64_t timestamp) override{
        frameRender(userId, width, height, fmt, 0, 0, pixelBuffer);
    }
	void onVideoFrameMixedCallbackGLESForIOS(void* pixelBuffer, int width, int height, int fmt, uint64_t timestamp)override{
        frameRender(g_strJoinUserId, width, height, fmt, 0, 0, pixelBuffer);
    }
    
    int getFormatSize(int fmt, int width, int height){
        if(isEngine)
            return width*height*3;
        int size = 0;
        switch (fmt) {
            case VIDEO_FMT_YUV420P:
                size = width*height*3/2;
                break;
            case VIDEO_FMT_RGB24:
                size = width*height*3;
                break;
            case VIDEO_FMT_BGRA32:
            case VIDEO_FMT_RGBA32:
            case VIDEO_FMT_ABGR32:
            case VIDEO_FMT_CVPIXELBUFFER:
                size = width*height*4;
                break;
        }
        return size;
    }
    
	void frameRender(std::string userId, int nWidth, int nHeight, int fmt, int nRotationDegree, int nBufSize, const void * buf)
    {
        
        I420Frame* frame = NULL;
        int frameSize = getFormatSize(fmt, nWidth, nHeight);
        {
            std::lock_guard<std::recursive_mutex> stateLock(*copyMutex);
            std::map<std::string, I420Frame*>::iterator iter = renders.find(userId);
            if (iter == renders.end()) {
                frame = new I420Frame();
                frame->degree   = nRotationDegree;
                frame->width = 0;
                frame->height = 0;
                frame->data = NULL;
                frame->datacopy = NULL;
                renders.insert(std::map<std::string, I420Frame*>::value_type(userId, frame));
            }
            {
                iter = renders.find(userId);
                frame = iter->second;
                frame->bIsNew = true;
                if(frameSize != frame->len){
                    if(frame->data != NULL)
                        delete [] frame->data;
                    if(frame->datacopy != NULL)
                        delete [] frame->datacopy;
                    frame->len      = frameSize;
                    frame->data     = new(std::nothrow) char[frameSize];
                    //frame->datacopy = new(std::nothrow) char[frameSize];
                }
                
                if (frame->data != NULL) {
                    frame->width    = nWidth;
                    frame->height   = nHeight;
                    if (!isEngine) {
                        switch (fmt) {
                            case VIDEO_FMT_YUV420P:
                                memcpy(frame->data, buf, nBufSize);
                                frame->fmt = YOUME_VIDEO_FMT_I420;
                                break;
                            case VIDEO_FMT_CVPIXELBUFFER:
    #if __APPLE__
                                CVPixelBufferToBGRA((CVPixelBufferRef)buf, (uint_8 *)frame->data);
                                frame->fmt = YOUME_VIDEO_FMT_BGRA;
    #endif
                                break;
                            default:
                                TSK_DEBUG_INFO("@@ cc video frame fmt don’t support :%d", fmt);
                                break;
                        }
                        
                    }else{
                        switch (fmt) {
                            case VIDEO_FMT_YUV420P:
                                libyuv_yuv420p_to_rgb24((uint_8 *)frame->data, (uint_8 *)buf, nWidth, nHeight);
                                break;
                            case VIDEO_FMT_RGB24:
                                memcpy(frame->data, buf, nBufSize);
                                break;
                            case VIDEO_FMT_BGRA32:
                                libyuv_bgra32_to_rgb24((uint_8 *)frame->data, (uint_8 *)buf, nWidth, nHeight);
                                break;
                            case VIDEO_FMT_RGBA32:
                                libyuv_rgba32_to_rgb24((uint_8 *)frame->data, (uint_8 *)buf, nWidth, nHeight);
                                break;
                            case VIDEO_FMT_CVPIXELBUFFER:
    #if __APPLE__
                                pixelbuffer_to_rgb24((CVPixelBufferRef)buf, (uint_8 *)frame->data, nWidth, nHeight);
    #endif
                                break;
                            default:
                                TSK_DEBUG_INFO("@@ cc video frame fmt don’t support :%d", fmt);
                                break;
                        }
                        frame->fmt = YOUME_VIDEO_FMT_RGB24;
                    }
                }
            }
        }
        
		if (videoFrameCallbackForFFICallback != NULL && !isEngine && frame != NULL){
			videoFrameCallbackForFFICallback(userId.c_str(), frame->data, frameSize, nWidth, nHeight, frame->fmt);
		}
		
    }
    
    
private:
    
    
    void libyuv_yuv420p_to_rgb24(uint_8 * rgb, uint_8 * yuv, int width, int height) {
        
        uint8_t* rgbBuf = rgb;
        const uint32_t y_length = width * height;
        const uint32_t uv_stride = (width + 1) / 2;
        const uint32_t uv_length = uv_stride * ((height + 1) / 2);
        unsigned char *Y_data_src_rotate = yuv;
        unsigned char *U_data_src_rotate = yuv + y_length;
        unsigned char *V_data_src_rotate = U_data_src_rotate + uv_length;
        
        int Dst_Stride_Y = width;
        YOUME_libyuv::I420ToRGB24(Y_data_src_rotate, Dst_Stride_Y,
                                  U_data_src_rotate, uv_stride,
                                  V_data_src_rotate, uv_stride,
                                  rgbBuf,
                                  width*3,
                                  width, height);
        
    }
    
    void libyuv_bgra32_to_rgb24(uint_8 * rgb, uint_8 * bgra, int width, int height) {
        
#if 0
        int rgbaSize = width*height*4;
        int iSlot = -1;
        uint8_t* argbBuffer = (uint8_t*)CTLSMemory::GetInstance()->Allocate(rgbaSize, iSlot);
        YOUME_libyuv::BGRAToARGB(bgra, width*4, argbBuffer, width*4, width, height);
        YOUME_libyuv::ARGBToRGB24(argbBuffer, width*4, rgb,  width*3, width, height);
        if(argbBuffer) {
            CTLSMemory::GetInstance()->Free(iSlot);
        }
#else
        int pos1 = 0;
        int pos2 = 0;
        int size  = width*height;
        for (int i = 0; i < size; ++i) {
            pos1 = i*3;
            pos2 = i*4;
            rgb[pos1] = bgra[pos2];
            rgb[pos1+1] = bgra[pos2 + 1];
            rgb[pos1+2] = bgra[pos2 + 2];
        }
#endif
    }
    
    void libyuv_rgba32_to_rgb24(uint_8 * rgb, uint_8 * rgba, int width, int height) {
        
#if 0
        int rgbaSize = width*height*4;
        int iSlot = -1;
        uint8_t* argbBuffer = (uint8_t*)CTLSMemory::GetInstance()->Allocate(rgbaSize, iSlot);
        YOUME_libyuv::RGBAToARGB(rgba, width*4, argbBuffer, width*4, width, height);
        YOUME_libyuv::ARGBToRGB24(argbBuffer, width*4, rgb,  width*3, width, height);
        if(argbBuffer) {
            CTLSMemory::GetInstance()->Free(iSlot);
        }
#else
        int pos1 = 0;
        int pos2 = 0;
        int size  = width*height;
        for (int i = 0; i < size; ++i) {
            pos1 = i*3;
            pos2 = i*4;
            rgb[pos1] = rgba[pos2 + 2];
            rgb[pos1+1] = rgba[pos2 + 1];
            rgb[pos1+2] = rgba[pos2 + 0];
        }
#endif
    }
    
    void yuv420p_to_rgb888(uint_8 * rgb, const uint_8 * yplane, const uint_8 * uplane, const uint_8 * vplane, const uint_32 width, const uint_32 height)
    {
        uint_8 * ptr = rgb;
        int r, g, b;
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                uint_8 yp = yplane[(y * width) + x];
                uint_8 up = uplane[((y / 2) * (width / 2)) + (x / 2)];
                uint_8 vp = vplane[((y / 2) * (width / 2)) + (x / 2)];
                
                r = ((1164 * (yp - 16) + 1596 * (vp - 128)                    ) / 1000);
                g = ((1164 * (yp - 16) -  813 * (vp - 128) -  391 * (up - 128)) / 1000);
                b = ((1164 * (yp - 16)                     + 2018 * (up - 128)) / 1000);
                
                *ptr++ = CLAMP(r);
                *ptr++ = CLAMP(g);
                *ptr++ = CLAMP(b);
            }
        }
    }
    
    void yuv420p_to_rgba8888(uint_8 * rgba, const uint_8 * yplane, const uint_8 * uplane, const uint_8 * vplane, const uint_32 width, const uint_32 height)
    {
        uint_8 * ptr = rgba;
        int r, g, b;
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                uint_8 yp = yplane[(y * width) + x];
                uint_8 up = uplane[((y / 2) * (width / 2)) + (x / 2)];
                uint_8 vp = vplane[((y / 2) * (width / 2)) + (x / 2)];
                
                r = ((1164 * (yp - 16) + 1596 * (vp - 128)                    ) / 1000);
                g = ((1164 * (yp - 16) -  813 * (vp - 128) -  391 * (up - 128)) / 1000);
                b = ((1164 * (yp - 16)                     + 2018 * (up - 128)) / 1000);
                
                *ptr++ = CLAMP(r);
                *ptr++ = CLAMP(g);
                *ptr++ = CLAMP(b);
                *ptr++ = CLAMP(0);
            }
        }
    }
    
#if __APPLE__
    
    void pixelbuffer_to_rgb24(CVPixelBufferRef pixelBuffer, uint_8* rgb, int width, int height){
        int rgbaSize = width*height*4;
        int iSlot = -1;
        uint8_t* bgraBuffer = (uint8_t*)CTLSMemory::GetInstance()->Allocate(rgbaSize, iSlot);
        CVPixelBufferToBGRA(pixelBuffer, bgraBuffer);
        libyuv_bgra32_to_rgb24(rgb, bgraBuffer, width, height);
        if(bgraBuffer) {
            CTLSMemory::GetInstance()->Free(iSlot);
        }
    }
    void CVPixelBufferToBGRA(CVPixelBufferRef pixelBuffer, uint_8* bgraBuffer){
        uint_8* destBuffer = bgraBuffer;
        CVPixelBufferRef videoFrame = (CVPixelBufferRef)pixelBuffer;
        CVPixelBufferLockBaseAddress(videoFrame, 0);
        uint8_t* baseAddress = (uint8_t*)CVPixelBufferGetBaseAddress(videoFrame);
        if(!baseAddress)
        {
            CVPixelBufferUnlockBaseAddress(videoFrame, 0);
            return ;
        }
        size_t bufferSize = CVPixelBufferGetDataSize(videoFrame);
        size_t videowidth = CVPixelBufferGetWidth(videoFrame);
        size_t videoheight = CVPixelBufferGetHeight(videoFrame);
        size_t bytesPerRow = CVPixelBufferGetBytesPerRow(videoFrame);
        int pixelFormat = CVPixelBufferGetPixelFormatType(videoFrame);
        if(pixelFormat != kCVPixelFormatType_32BGRA){
            CVPixelBufferUnlockBaseAddress(videoFrame, 0);
            return ;
        }
        
        if(bytesPerRow != videowidth*4 ||
           bufferSize != bytesPerRow*videoheight)
        {
            bufferSize = videoheight*videowidth*4;
            for (int i = 0; i < videoheight; i++) {
                memcpy(&destBuffer[i*videowidth*4], &baseAddress[i*bytesPerRow], videowidth*4);
            }
        }
        else{
            memcpy(destBuffer, baseAddress, bufferSize);
        }
        CVPixelBufferUnlockBaseAddress(videoFrame, 0);
    }
#endif
    
};

void youme_setCallback( EventCb YMEventCallback,
                        RequestRestAPICb YMRestApiCallback,
                        MemberChangeCb YMMemberChangeCallback,
                        BroadcastCb YMBroadcastCallback,
                        AVStatisticCb YMAVStatisticCallback,
                       SentenceBeginCb YMSentenceBeginCallback,
                       SentenceEndCb YMSentenceEndCallback,
                       SentenceChangedCb YMSentenceChangedCallback,
                       TranslateTextCompleteCb YMTranslateTextCompleteCallback)
{
    if (g_InterImpCallback) {
        g_InterImpCallback->setcallback(YMEventCallback, YMRestApiCallback, YMMemberChangeCallback, YMBroadcastCallback, YMAVStatisticCallback,
                                        YMSentenceBeginCallback, YMSentenceEndCallback, YMSentenceChangedCallback,YMTranslateTextCompleteCallback  );
    }

    return;
}

void youme_setPCMCallback(OnPcmDataCallback pcmCallback, int flag, bool bOutputToSpeaker, int nOutputSampleRate, int nOutputChannel)
{
	IYouMeVoiceEngine::getInstance()->setPcmCallbackEnable(g_InterImpCallback, flag, bOutputToSpeaker, nOutputSampleRate, nOutputChannel);
	g_InterImpCallback->setPCMCallback(pcmCallback);
}

void youme_Log(const char* msg)
{
	TSK_DEBUG_INFO("@@ youme_Log: %s", msg);
}

void youme_setSentenceEnd(const char* userid, int sentenceIndex, const char* result, const char* transLang, const char* transResult)
{
	g_InterImpCallback->onSentenceEnd(userid, sentenceIndex, result, transLang, transResult);
}

void youme_setSentenceChanged(const char* userid, int sentenceIndex, const char* result, const char* transLang, const char* transResult)
{
	g_InterImpCallback->onSentenceChanged(userid, sentenceIndex, result, transLang, transResult);
}

void youme_setEvent(int event, int error, const char * channel, const char * param)
{
	g_InterImpCallback->onEvent((YouMeEvent)event, (YouMeErrorCode)error, channel, param);
}

InterImpVideoCallback * InterImpVideoCallback::pInstance = NULL;

int youme_unInit()
{
    return  IYouMeVoiceEngine::getInstance()->unInit();
}

/**
 *  功能描述:切换语音输出设备
 *  默认输出到扬声器，在加入房间成功后设置（iOS受系统限制，如果已释放麦克风则无法切换到听筒）
 *
 *  @param bOutputToSpeaker:true——使用扬声器，false——使用听筒
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
YOUME_API int youme_setOutputToSpeaker (bool bOutputToSpeaker)
{
    return IYouMeVoiceEngine::getInstance()->setOutputToSpeaker(bOutputToSpeaker);
    
}

/**
 *  功能描述:扬声器静音打开/关闭
 *
 *  @param bOn:true——打开，false——关闭
 *  @return 无
 */
void youme_setSpeakerMute (bool bOn)
{
    IYouMeVoiceEngine::getInstance()->setSpeakerMute(bOn);
}


void youme_setAutoSendStatus( bool bAutoSend ){
    IYouMeVoiceEngine::getInstance()->setAutoSendStatus( bAutoSend );
    
}

/**
 *  功能描述:获取扬声器静音状态
 *
 *  @return true——打开，false——关闭
 */
bool youme_getSpeakerMute ()
{
    return IYouMeVoiceEngine::getInstance()->getSpeakerMute();
}

/**
 *  功能描述:获取麦克风静音状态
 *
 *  @return true——打开，false——关闭
 */
bool youme_getMicrophoneMute ()
{
    return IYouMeVoiceEngine::getInstance()->getMicrophoneMute();
}

/**
 *  功能描述:麦克风静音打开/关闭
 *
 *  @param bOn:true——打开，false——关闭
 *  @return 无
 */
void youme_setMicrophoneMute (bool mute)
{
    return IYouMeVoiceEngine::getInstance()->setMicrophoneMute(mute);
}

/**
 *  功能描述:获取音量大小,此音量值为程序内部的音量，与系统音量相乘得到程序使用的实际音量
 *
 *  @return 音量值[0,100]
 */
unsigned int youme_getVolume ()
{
    return  IYouMeVoiceEngine::getInstance()->getVolume();
}

/**
 *  功能描述:增加音量，音量数值加1
 *
 *  @return 无
 */
void youme_setVolume (const unsigned int uiVolume)
{
    IYouMeVoiceEngine::getInstance()->setVolume(uiVolume);
}

/**
 *  功能描述:是否可使用移动网络
 *
 *  @return true-可以使用，false-禁用
 */
bool youme_getUseMobileNetworkEnabled ()
{
    return IYouMeVoiceEngine::getInstance()->getUseMobileNetworkEnabled();
}

/**
 *  功能描述:启用/禁用移动网络
 *
 *  @param bEnabled:true-可以启用，false-禁用，默认禁用
 *
 *  @return 无
 */
void youme_setUseMobileNetworkEnabled (bool bEnabled)
{
    IYouMeVoiceEngine::getInstance()->setUseMobileNetworkEnabled(bEnabled);
}

void youme_setToken( const char* pToken, uint32_t uTimeStamp ){
    IYouMeVoiceEngine::getInstance()->setToken( pToken, uTimeStamp );
}    

 /**
  *  功能描述:设置本地连接信息，用于p2p传输，本接口在join房间之前调用
  *
  *  @param pLocalIP:本端ip
  *  @param iLocalPort:本端数据端口
  *  @param pRemoteIP:远端ip
  *  @param iRemotePort:远端数据端口
  *  @param iNetType: 0:ipv4, 1:ipv6, 目前仅支持ipv4
  *
  *  @return 错误码，详见YouMeConstDefine.h定义
 */
YOUME_API int youme_setLocalConnectionInfo(const char* pLocalIP, int iLocalPort, const char* pRemoteIP, int iRemotePort, int iNetType)
{
    return IYouMeVoiceEngine::getInstance()->setLocalConnectionInfo(pLocalIP, iLocalPort, pRemoteIP, iRemotePort, iNetType);
}

/**
 *  功能描述:清除本地局域网连接信息，强制server转发
 *
 *  @return 无
 */
YOUME_API void youme_clearLocalConnectionInfo()
{
    IYouMeVoiceEngine::getInstance()->clearLocalConnectionInfo( );
}

/**
 *  功能描述:设置是否切换server通路
 *
 *  @param enable: 设置是否切换server通路标志
 *
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
YOUME_API int youme_setRouteChangeFlag(bool enable)
{
    return IYouMeVoiceEngine::getInstance()->setRouteChangeFlag( enable );
} 

int youme_setUserLogPath(const char* pFilePath){
    return IYouMeVoiceEngine::getInstance()->setUserLogPath( pFilePath );
}


//---------------------多人语音接口---------------------//

/**
 *  功能描述:加入语音频道
 *
 *  @param strChannelID:频道ID，要保证全局唯一
 *
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
int youme_joinChannelSingleMode(const char* pUserID, const char* pChannelID, int userRole, bool autoRecvVideo)
{
    g_strJoinUserId = pUserID;
    YouMeUserRole_t eUserRole = (YouMeUserRole_t)userRole;
    return IYouMeVoiceEngine::getInstance()->joinChannelSingleMode(pUserID, pChannelID, eUserRole, autoRecvVideo);
}

/**
 *  功能描述：加入音频会议（多房间模式）
 *
 */
int youme_joinChannelMultiMode(const char * pUserID, const char * pChannelID, int userRole)
{
    YouMeUserRole_t eUserRole = (YouMeUserRole_t)userRole;
    return IYouMeVoiceEngine::getInstance()->joinChannelMultiMode(pUserID, pChannelID, eUserRole);
}

/**
 *  功能描述：对指定房间说话
 *
 */
int youme_speakToChannel(const char * pChannelID)
{
    return IYouMeVoiceEngine::getInstance()->speakToChannel(pChannelID);
}

/**
 *  功能描述:退出多频道模式下的某一个频道
 *
 *  @param strChannelID:频道ID，要保证全局唯一
 *
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
int youme_leaveChannelMultiMode (const char* pChannelID)
{
    
    return  IYouMeVoiceEngine::getInstance()->leaveChannelMultiMode(pChannelID);
}

/**
 *  功能描述:退出所有音频会议
 *
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
int youme_leaveChannelAll ()
{
    
    return  IYouMeVoiceEngine::getInstance()->leaveChannelAll();
}

/**
 *  功能描述:设置是否开启变声
 *
 *  @param bEnable:设置是否开启变声
 *
 *  @return void
 */
void youme_setSoundtouchEnabled(bool bEnable)
{
    SetSoundtouchEnabled(bEnable);
}

/**
 *  功能描述:设置服务器区域
 *
 *  @param regionId: 预定义的服务器区域码
 *  @param extRegionName: 扩展的服务器区域名字
 *
 *  @return void
 */
void youme_setServerRegion(int regionId, const char* extRegionName, bool bAppend)
{
    IYouMeVoiceEngine::getInstance()->setServerRegion((YOUME_RTC_SERVER_REGION)regionId, extRegionName, bAppend);
}


/**
 *  功能描述:获取是否开启变声效果
 *
 *  @return 是否开启变声
 */
bool youme_getSoundtouchEnabled(){
    return GetSoundtouchEnabled();
}

//变速,1为正常值
float youme_getSoundtouchTempo(){
    return GetSoundtouchTempo();
}

void youme_setSoundtouchTempo(float nTempo){
    SetSoundtouchTempo(nTempo);
}

//节拍,1为正常值
float youme_getSoundtouchRate(){
    return GetSoundtouchRate();
}
void youme_setSoundtouchRate(float nRate){
    SetSoundtouchRate(nRate);
}

//变调,1为正常值
float youme_getSoundtouchPitch(){
    return GetSoundtouchPitch();
}
void youme_setSoundtouchPitch(float nPitch){
    SetSoundtouchPitch(nPitch);
}

//设置是否使用测试服
void youme_setTestConfig(int serverMode){
    SetServerMode((SERVER_MODE)serverMode);
}

void youme_setServerIpPort(const char* ip, int port){
    SetServerIpPort(ip, port);
}

int youme_getSDKVersion(){
    return IYouMeVoiceEngine::getInstance()->getSDKVersion();
}

//设置其他人的麦克风状态
int youme_setOtherMicMute(const char*  pUserID,bool mute){
    return IYouMeVoiceEngine::getInstance()->setOtherMicMute(pUserID, mute );
}

//设置其他人的麦克风状态
int youme_setOtherSpeakerMute(const char* pUserID,bool mute){
    return IYouMeVoiceEngine::getInstance()->setOtherSpeakerMute(pUserID, mute);
}

//设置屏蔽其他人的语音
int youme_setListenOtherVoice(const char*  pUserID,bool isOn){
    return IYouMeVoiceEngine::getInstance()->setListenOtherVoice(pUserID, isOn);
}


/**
 *  功能描述: (七牛接口)将提供的音频数据混合到麦克风或者扬声器的音轨里面。
 *  @param data 指向PCM数据的缓冲区
 *  @param len  音频数据的大小
 *  @param timestamp 时间搓
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
int youme_inputAudioFrame(void* data, int len, uint64_t timestamp)
{
    return IYouMeVoiceEngine::getInstance()->inputAudioFrame(data, len, timestamp);
}

int youme_inputVideoFrame(void* data, int len, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp)
{
    return IYouMeVoiceEngine::getInstance()->inputVideoFrame(data, len, width, height, fmt, rotation, mirror, timestamp);
}

// iOS 摄像头共享
int youme_inputVideoFrameForIOS(void* pixelbuffer, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp){
    return IYouMeVoiceEngine::getInstance()->inputVideoFrameForIOS(pixelbuffer, width, height, fmt, rotation, mirror, timestamp);
}

#if TARGET_OS_IPHONE
// iOS 桌面共享
int youme_inputVideoFrameForIOSShare(void* pixelbuffer, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp){
    return IYouMeVoiceEngine::getInstance()->inputVideoFrameForIOSShare(pixelbuffer, width, height, fmt, rotation, mirror, timestamp);
}
#endif

#if ANDROID
int youme_inputVideoFrameForAndroid(int textureId, float* matrix, int width, int height, int fmt, int rotation, int mirror, uint64_t timestamp){
    return IYouMeVoiceEngine::getInstance()->inputVideoFrameForAndroid(textureId, matrix, width, height, fmt, rotation, mirror, timestamp);
}
#endif

int youme_stopInputVideoFrame()
{
    return IYouMeVoiceEngine::getInstance()->stopInputVideoFrame();
}

int youme_stopInputVideoFrameForShare()
{
    return IYouMeVoiceEngine::getInstance()->stopInputVideoFrameForShare();
}

int youme_playBackgroundMusic(const char* pFilePath, bool bRepeat)
{
    return IYouMeVoiceEngine::getInstance()->playBackgroundMusic(pFilePath, bRepeat);
}

int youme_pauseBackgroundMusic()
{
    return IYouMeVoiceEngine::getInstance()->pauseBackgroundMusic();
}

int youme_resumeBackgroundMusic()
{
    return IYouMeVoiceEngine::getInstance()->resumeBackgroundMusic();
}

int youme_stopBackgroundMusic()
{
    return IYouMeVoiceEngine::getInstance()->stopBackgroundMusic();
}

int youme_setBackgroundMusicVolume(int vol)
{
    return IYouMeVoiceEngine::getInstance()->setBackgroundMusicVolume(vol);
}

int youme_setHeadsetMonitorOn(bool micEnabled, bool bgmEnabled)
{
    return IYouMeVoiceEngine::getInstance()->setHeadsetMonitorOn(micEnabled, bgmEnabled);
}


int youme_setReverbEnabled(bool enabled)
{
    return IYouMeVoiceEngine::getInstance()->setReverbEnabled(enabled);
}

int youme_setVadCallbackEnabled(bool enabled)
{
    return IYouMeVoiceEngine::getInstance()->setVadCallbackEnabled(enabled);
}

int youme_pauseChannel()
{
    return IYouMeVoiceEngine::getInstance()->pauseChannel();
}

int youme_resumeChannel()
{
    return IYouMeVoiceEngine::getInstance()->resumeChannel();
}

int youme_setMicLevelCallback(int maxLevel)
{
    return IYouMeVoiceEngine::getInstance()->setMicLevelCallback(maxLevel);
}

int youme_setFarendVoiceLevelCallback(int maxLevel)
{
    return IYouMeVoiceEngine::getInstance()->setFarendVoiceLevelCallback(maxLevel);
}

int youme_setReleaseMicWhenMute(bool enabled)
{
    return IYouMeVoiceEngine::getInstance()->setReleaseMicWhenMute(enabled);
}

int youme_setExitCommModeWhenHeadsetPlugin(bool enabled)
{
    return IYouMeVoiceEngine::getInstance()->setExitCommModeWhenHeadsetPlugin(enabled);
}

void youme_setRecordingTimeMs(unsigned int timeMs)
{
    return IYouMeVoiceEngine::getInstance()->setRecordingTimeMs(timeMs);
}

void youme_setPlayingTimeMs(unsigned int timeMs)
{
    return IYouMeVoiceEngine::getInstance()->setPlayingTimeMs(timeMs);
}

void youme_setServerMode(int mode)
{
    SetServerMode((SERVER_MODE)mode);
}

int  youme_requestRestApi(  const char* pCommand , const char*  pQueryBody, int* requestID ){
    return IYouMeVoiceEngine::getInstance()->requestRestApi(  pCommand, pQueryBody , requestID);
}

int  youme_getChannelUserList( const char* pChannelID , int maxCount, bool  notifyMemChange ){
    return IYouMeVoiceEngine::getInstance()->getChannelUserList(pChannelID, maxCount, notifyMemChange);
}

int youme_setGrabMicOption(const char* pChannelID, int mode, int maxAllowCount, int maxTalkTime, unsigned int voteTime)
{
    return IYouMeVoiceEngine::getInstance()->setGrabMicOption(pChannelID, mode, maxAllowCount, maxTalkTime, voteTime);
}

int youme_startGrabMicAction(const char* pChannelID, const char* pContent)
{
    return IYouMeVoiceEngine::getInstance()->startGrabMicAction(pChannelID, pContent);
}

int youme_stopGrabMicAction(const char* pChannelID, const char* pContent)
{
    return IYouMeVoiceEngine::getInstance()->stopGrabMicAction(pChannelID, pContent);
}

int youme_requestGrabMic(const char* pChannelID, int score, bool isAutoOpenMic, const char* pContent)
{
    return IYouMeVoiceEngine::getInstance()->requestGrabMic(pChannelID, score, isAutoOpenMic, pContent);
}

int youme_releaseGrabMic(const char* pChannelID)
{
    return IYouMeVoiceEngine::getInstance()->releaseGrabMic(pChannelID);
}

int youme_setInviteMicOption(const char* pChannelID, int waitTimeout, int maxTalkTime)
{
    return IYouMeVoiceEngine::getInstance()->setInviteMicOption(pChannelID, waitTimeout, maxTalkTime);
}

int youme_requestInviteMic(const char* pChannelID, const char* pUserID, const char* pContent)
{
    return IYouMeVoiceEngine::getInstance()->requestInviteMic(pChannelID, pUserID, pContent);
}

int youme_responseInviteMic(const char* pUserID, bool isAccept, const char* pContent)
{
    return IYouMeVoiceEngine::getInstance()->responseInviteMic(pUserID, isAccept, pContent);
}

int youme_stopInviteMic()
{
    return IYouMeVoiceEngine::getInstance()->stopInviteMic();
}

int youme_sendMessage( const char* pChannelID,  const char* pContent, int* requestID )
{
    return IYouMeVoiceEngine::getInstance()->sendMessage( pChannelID, pContent, NULL, requestID );
}

int youme_sendMessageToUser( const char* pChannelID,  const char* pContent, const char* pUserID, int* requestID )
{
    return IYouMeVoiceEngine::getInstance()->sendMessage( pChannelID, pContent, pUserID, requestID );
}

int youme_kickOtherFromChannel( const char* pUserID, const char* pChannelID , int lastTime )
{
    return IYouMeVoiceEngine::getInstance()->kickOtherFromChannel( pUserID, pChannelID, lastTime );
}

int youme_setVBR(int useVBR){
	return IYouMeVoiceEngine::getInstance()->setVBR(useVBR);
}

int youme_setVBRForSecond(int useVBR)
{
	return IYouMeVoiceEngine::getInstance()->setVBRForSecond(useVBR);
}


// 设置是否开启视频编码器
int youme_openVideoEncoder(const char* pFilePath)
{
    return IYouMeVoiceEngine::getInstance()->openVideoEncoder(pFilePath);
}

// 创建渲染ID
int youme_createRender(const char * userId)
{
    return IYouMeVoiceEngine::getInstance()->createRender(userId);
}

// 删除渲染ID
int youme_deleteRender(int renderId)
{
    return IYouMeVoiceEngine::getInstance()->deleteRender(renderId);
}

// 删除渲染ID
int youme_deleteRenderByUserID(const char * userId)
{
    return IYouMeVoiceEngine::getInstance()->deleteRender(userId);
}

// 设置视频回调
int youme_setVideoCallback(const char * strObjName)
{
    g_videoCallbackName = strObjName;
    return IYouMeVoiceEngine::getInstance()->setVideoCallback(InterImpVideoCallback::getInstance());
}

// 开始camera capture
int youme_startCapture()
{
    return IYouMeVoiceEngine::getInstance()->startCapture();
}

// 停止camera capture
int youme_stopCapture()
{
    return IYouMeVoiceEngine::getInstance()->stopCapture();
}

YOUME_API int youme_setVideoPreviewFps(int fps)
{
    return IYouMeVoiceEngine::getInstance()->setVideoPreviewFps( fps );
}

YOUME_API int youme_setVideoFps(int fps)
{
    return IYouMeVoiceEngine::getInstance()->setVideoFps( fps );
}

void youme_setLocalVideoPreviewMirror(bool enable)
{
    IYouMeVoiceEngine::getInstance()->setLocalVideoPreviewMirror(enable);
}

// 切换前后摄像头
int youme_switchCamera()
{
    return IYouMeVoiceEngine::getInstance()->switchCamera();
}

int youme_resetCamera()
{
    return IYouMeVoiceEngine::getInstance()->resetCamera();
}

// 设置camera capture property
int youme_setVideoLocalResolution(int width, int height)
{
    return IYouMeVoiceEngine::getInstance()->setVideoLocalResolution(width, height);
}

// 设置是否前置摄像头
int youme_setCaptureFrontCameraEnable(bool enable)
{
    return IYouMeVoiceEngine::getInstance()->setCaptureFrontCameraEnable(enable);
}

// 屏蔽/恢复个userId的视频流
int youme_maskVideoByUserId(const char *userId, bool mask)
{
    return IYouMeVoiceEngine::getInstance()->maskVideoByUserId(userId, mask);
}

// 获取视频数据
/****
 int youme_getVideoFrame(int renderId, I420Frame * frame)
 {
 if (frame == NULL) {
 return -1;
 }
 
 TSK_DEBUG_INFO ("-- youme_getVideoFrame renderId:%d", renderId);
 
 I420Frame frameSource;
 InterImpVideoCallback * pVideoCb = InterImpVideoCallback::getInstance();
 std::map<int, I420Frame>::iterator iter = pVideoCb->renders.find(renderId);
 if (iter == pVideoCb->renders.end()) {
 return -2;
 }
 
 frameSource = iter->second;
 
 frame->renderId = frameSource.renderId;
 frame->width = frameSource.width;
 frame->height = frameSource.height;
 frame->degree = frameSource.degree;
 frame->len = frameSource.len;
 
 TSK_DEBUG_INFO ("-- youme_getVideoFrame:%d", frameSource.len);
 
 if (frame->data != NULL) {
 std::lock_guard<std::recursive_mutex> stateLock(copyMutex);
 memcpy(frame->data, frameSource.data, frameSource.len);
 }
 
 return frameSource.len;
 }
 ***/


char * youme_getVideoFrame(const char* userId, int * len, int * width, int * height)
{
    std::lock_guard<std::recursive_mutex> stateLock(*copyMutex);
    I420Frame* frameSource = NULL;
    InterImpVideoCallback * pVideoCb = InterImpVideoCallback::getInstance();
    std::map<std::string, I420Frame*>::iterator iter = pVideoCb->renders.find(userId);
    if (iter == pVideoCb->renders.end()) {
        return NULL;
    }
    
    frameSource = iter->second;
    
    if (!frameSource->bIsNew) {
        return NULL;
    }
    
	/*
    if (frameSource->data != NULL) {
        
        memcpy(frameSource->datacopy, frameSource->data, frameSource->len);
    }
	*/
    
    TMEDIA_I_AM_ACTIVE(200, "-- youme_getVideoFrame: %d", frameSource->len);
    
    if (len != NULL) {
        *len = frameSource->len;
    }
    
    if (width != NULL) {
        *width = frameSource->width;
    }
    
    if (height != NULL) {
        *height = frameSource->height;
    }
    frameSource->bIsNew=false;
    return frameSource->data;
}

void youme_setVideoFrameCallback(VideoFrameCallbackForFFI callback)
{
    TSK_DEBUG_INFO("youme_setVideoFrameCallback: %p", callback);
	videoFrameCallbackForFFICallback = callback;
}

void youme_setVideoEventCallback(VideoEventCallbackForFFI callback)
{
    videoEventCallbackForFFICallback = callback;
}


char * youme_getVideoFrameNew(const char* userId, int * len, int * width, int * height, int * fmt){
    std::lock_guard<std::recursive_mutex> stateLock(*copyMutex);
    I420Frame* frameSource = NULL;
    InterImpVideoCallback * pVideoCb = InterImpVideoCallback::getInstance();
    std::map<std::string, I420Frame*>::iterator iter = pVideoCb->renders.find(userId);
    if (iter == pVideoCb->renders.end()) {
        return NULL;
    }
    
    frameSource = iter->second;
    
    if (!frameSource->bIsNew) {
        return NULL;
    }
    
	/*
    if (frameSource->data != NULL) {
        
        memcpy(frameSource->datacopy, frameSource->data, frameSource->len);
    }
    */
    TMEDIA_I_AM_ACTIVE(200, "-- youme_getVideoFrameNew: %d", frameSource->len);
    
    if (len != NULL) {
        *len = frameSource->len;
    }
    
    if (width != NULL) {
        *width = frameSource->width;
    }
    
    if (height != NULL) {
        *height = frameSource->height;
    }
    
    if(fmt != NULL) {
        *fmt = frameSource->fmt;
    }
    frameSource->bIsNew=false;
	return frameSource->data;
}

// 进入房间后，切换身份
int  youme_setUserRole(int userRole)
{
    return IYouMeVoiceEngine::getInstance()->setUserRole((YouMeUserRole_t)userRole);
}
// 获取身份
int  youme_getUserRole()
{
    return IYouMeVoiceEngine::getInstance()->getUserRole();
}
// 背景音乐是否在播放
bool  youme_isBackgroundMusicPlaying()
{
    return IYouMeVoiceEngine::getInstance()->isBackgroundMusicPlaying();
}
// 是否初始化成功
bool  youme_isInited()
{
    return IYouMeVoiceEngine::getInstance()->isInited();
}
// 是否在某个语音房间内
bool  youme_isInChannel(const char* pChannelID)
{
    return IYouMeVoiceEngine::getInstance()->isInChannel(pChannelID);
}

void youme_setLogLevel( int consoleLevel, int fileLevel)
{
    return IYouMeVoiceEngine::getInstance()->setLogLevel( (YOUME_LOG_LEVEL)consoleLevel, (YOUME_LOG_LEVEL)fileLevel);
}

int youme_setExternalInputSampleRate( int inputSampleRate, int mixedCallbackSampleRate)
{
    return IYouMeVoiceEngine::getInstance()->setExternalInputSampleRate( (YOUME_SAMPLE_RATE)inputSampleRate, (YOUME_SAMPLE_RATE)mixedCallbackSampleRate );
}

int  youme_setVideoNetResolution( int width, int height )
{
    return IYouMeVoiceEngine::getInstance()->setVideoNetResolution( width , height );
}

int  youme_setVideoNetResolutionForSecond( int width, int height )
{
    return IYouMeVoiceEngine::getInstance()->setVideoNetResolutionForSecond( width , height );
}

void youme_setAVStatisticInterval( int interval  )
{
    return IYouMeVoiceEngine::getInstance()->setAVStatisticInterval( interval );
}

void youme_setAudioQuality( int quality )
{
    return IYouMeVoiceEngine::getInstance()->setAudioQuality( (YOUME_AUDIO_QUALITY)quality);
}

void youme_setVideoCodeBitrate( unsigned int maxBitrate,  unsigned int minBitrate )
{
    return IYouMeVoiceEngine::getInstance()->setVideoCodeBitrate( maxBitrate, minBitrate );
}
void youme_setVideoCodeBitrateForSecond( unsigned int maxBitrate,  unsigned int minBitrate )
{
    return IYouMeVoiceEngine::getInstance()->setVideoCodeBitrateForSecond( maxBitrate, minBitrate );
}

unsigned int youme_getCurrentVideoCodeBitrate( )
{
    return IYouMeVoiceEngine::getInstance()->getCurrentVideoCodeBitrate();
}

void youme_setVideoHardwareCodeEnable( bool bEnable )
{
    return IYouMeVoiceEngine::getInstance()->setVideoHardwareCodeEnable( bEnable );
}

bool youme_getVideoHardwareCodeEnable( )
{
    return IYouMeVoiceEngine::getInstance()->getVideoHardwareCodeEnable();
}

bool youme_getUseGL( )
{
    return IYouMeVoiceEngine::getInstance()->getUseGL();
}

void youme_setVideoNoFrameTimeout(int timeout)
{
    return IYouMeVoiceEngine::getInstance()->setVideoNoFrameTimeout( timeout );
}

int youme_queryUsersVideoInfo( char* userList)
{
    youmecommon::Reader jsonReader;
    youmecommon::Value jsonRoot;
    bool bRet = jsonReader.parse( userList, jsonRoot );
    
    if(!bRet || !jsonRoot.isArray()){
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    std::vector<std::string> users;
    
    for( int i = 0; i < (int)jsonRoot.size(); i++ )
    {
        users.push_back( jsonRoot[i].asString() );
    }
    
    return IYouMeVoiceEngine::getInstance()->queryUsersVideoInfo( users );
}

int  youme_setUsersVideoInfo( char* videoinfoList )
{
    youmecommon::Reader jsonReader;
    youmecommon::Value jsonRoot;
    bool bRet =  jsonReader.parse( videoinfoList, jsonRoot );
    
    if(!bRet || !jsonRoot.isArray()){
        return YOUME_ERROR_INVALID_PARAM;
    }
    
    std::vector<IYouMeVoiceEngine::userVideoInfo> users;
    
    for( int i = 0; i < (int)jsonRoot.size(); i++ )
    {
        IYouMeVoiceEngine::userVideoInfo  info;
        youmecommon::Value infoValue = jsonRoot[i];
        info.userId = infoValue["userId"].asString();
        info.resolutionType = infoValue["resolutionType"].asInt();
        
        users.push_back( info );
    }
    
    return IYouMeVoiceEngine::getInstance()->setUsersVideoInfo( users );
}


int youme_openBeautify(bool open)
{
    return IYouMeVoiceEngine::getInstance()->openBeautify(open);
}

int youme_beautifyChanged(float param)
{
    return IYouMeVoiceEngine::getInstance()->beautifyChanged(param);
}

//int youme_stretchFace(bool stretch)
//{
//    return IYouMeVoiceEngine::getInstance()->stretchFace(stretch);
//}

bool youme_releaseMicSync()
{
    return IYouMeVoiceEngine::getInstance()->releaseMicSync();
}

bool youme_resumeMicSync()
{
    return IYouMeVoiceEngine::getInstance()->resumeMicSync();
}

void youme_setMixVideoSize(int width, int height)
{
    return IYouMeVoiceEngine::getInstance()->setMixVideoSize(width,height);
}

void youme_addMixOverlayVideo(char* userId, int x, int y, int z, int width, int height)
{
	IYouMeVoiceEngine::getInstance()->addMixOverlayVideo(userId, x, y, z, width, height);
}

void youme_videoEngineModelEnabled(bool enabeld){
    InterImpVideoCallback * pVideoCb = InterImpVideoCallback::getInstance();
    pVideoCb->isEngine = enabeld;
}

int youme_getCameraCount()
{
    return IYouMeVoiceEngine::getInstance()->getCameraCount();
}

int youme_getCameraName(int cameraid, char* cameraName)
{
    std::string namestr = IYouMeVoiceEngine::getInstance()->getCameraName(cameraid);
    strcpy(cameraName,namestr.c_str());
    return namestr.length();
}

int youme_setOpenCameraId(int cameraId)
{
    return IYouMeVoiceEngine::getInstance()->setOpenCameraId(cameraId);
}

int youme_getRecordDeviceCount()
{
    return IYouMeVoiceEngine::getInstance()->getRecordDeviceCount();
}

bool youme_getRecordDeviceInfo(int index, char* deviceName, char* deviceUId)
{
    char recordDeviceName[MAX_DEVICE_ID_LENGTH] = {0};
    char recordDeviceUId[MAX_DEVICE_ID_LENGTH] = {0};
    bool bRet = IYouMeVoiceEngine::getInstance()->getRecordDeviceInfo(index, recordDeviceName, recordDeviceUId);
    if (bRet) {
        strcpy(deviceName, recordDeviceName);
        strcpy(deviceUId, recordDeviceUId);
    }
    
    return bRet;
}

int youme_setRecordDevice(const char* deviceUId)
{
    return IYouMeVoiceEngine::getInstance()->setRecordDevice(deviceUId);
}

int youme_setTCPMode(int iUseTCP)
{
	return IYouMeVoiceEngine::getInstance()->setTCPMode(iUseTCP);
}

int youme_setVideoFrameRawCbEnabled(bool enabeld)
{
    return IYouMeVoiceEngine::getInstance()->setVideoFrameRawCbEnabled(enabeld);
}

int youme_inputVideoFrameForShare(void* data, int len, int width, int  height, int fmt, int rotation, int mirror, uint64_t timestamp)
{
	return IYouMeVoiceEngine::getInstance()->inputVideoFrameForShare(data, len, width, height, fmt, rotation, mirror, timestamp);
}

int youme_setVideoFpsForShare(int fps)
{
	return IYouMeVoiceEngine::getInstance()->setVideoFpsForShare(fps);
}

int youme_setVideoFpsForSecond(int fps)
{
    return IYouMeVoiceEngine::getInstance()->setVideoFpsForSecond(fps);
}

int youme_setVideoNetResolutionForShare(int width, int height)
{
	return IYouMeVoiceEngine::getInstance()->setVideoNetResolutionForShare(width, height);
}

void youme_setVideoCodeBitrateForShare(unsigned int maxBitrate, unsigned int minBitrate)
{
	IYouMeVoiceEngine::getInstance()->setVideoCodeBitrateForShare(maxBitrate, minBitrate);
}

int youme_setVBRForShare(int useVBR)
{
	return IYouMeVoiceEngine::getInstance()->setVBRForShare(useVBR);
}

int youme_setVideoNetAdjustmode( int mode )
{
    return IYouMeVoiceEngine::getInstance()->setVideoNetAdjustmode(mode);
}

int youme_checkSharePermission()
{
#ifdef MAC_OS
    return IYouMeVoiceEngine::getInstance()->checkSharePermission();
#else 
    return 0;
#endif
}

// windows/mac screen capture
#ifdef WIN32
int youme_startShareStream(int mode, HWND renderHandle, HWND captureHandle)
{
	return IYouMeVoiceEngine::getInstance()->StartShareStream(mode, renderHandle, captureHandle);
}

int youme_startSaveScreen(const char* filePath, HWND renderHandle, HWND captureHandle)
{
    return IYouMeVoiceEngine::getInstance()->startSaveScreen( filePath, renderHandle, captureHandle);
}

#elif MAC_OS
int youme_startShareStream(int mode, int windowid)
{
    return IYouMeVoiceEngine::getInstance()->StartShareStream(mode, windowid);
}

int youme_startSaveScreen(const char* filePath)
{
    return IYouMeVoiceEngine::getInstance()->startSaveScreen(filePath);
}

#endif

#if defined(WIN32) || defined(MAC_OS)
void youme_stopShareStream()
{
	IYouMeVoiceEngine::getInstance()->StopShareStream();
}

char *youme_getShareVideoData(int * len, int * width, int * height, int * fmt)
{
	//IYouMeVoiceEngine::getInstance()->GetShareVideoData(len, width, height, fmt);
	return NULL;
}
#endif

void youme_stopSaveScreen()
{
    return IYouMeVoiceEngine::getInstance()->stopSaveScreen();
}

int youme_setTranscriberEnabled( bool enable)
{
    if( enable )
    {
        return IYouMeVoiceEngine::getInstance()->setTranscriberEnabled( enable, g_InterImpCallback );
    }
    else{
        return IYouMeVoiceEngine::getInstance()->setTranscriberEnabled( enable, nullptr );
    }
}

int  youme_translateText(unsigned int* requestID, const char* text, YouMeLanguageCode destLangCode, YouMeLanguageCode srcLangCode)
{
    return IYouMeVoiceEngine::getInstance()->translateText( requestID, text, destLangCode, srcLangCode );
}

const char *youme_getWinList()
{
#ifdef WIN32 
    HWND arrayHwnd[100] = {0};
    char arrayName[100][256] = {0};

    HWND *pHwnd = arrayHwnd;
    //char *pName = arrayName;
    int count = 0;
    IYouMeVoiceEngine::getInstance()->GetWindowInfoList(arrayHwnd, *arrayName, &count);
    
    youmecommon::Value jsonRoot;
    jsonRoot["count"] = youmecommon::Value(count);

    for( int i = 0; i < count; ++i ){
        youmecommon::Value jsonWindowInfo;
        jsonWindowInfo["hwnd"] = youmecommon::Value( (long)arrayHwnd[i] );
        jsonWindowInfo["name"] = youmecommon::Value( arrayName[i] );
        
        jsonRoot["winlist"].append(jsonWindowInfo);
    }
    
    TSK_DEBUG_INFO("window count[%d] handle[%s]", count, XStringToUTF8(jsonRoot.toSimpleString()).c_str());

	return XStringToUTF8(jsonRoot.toSimpleString()).c_str();
#elif MAC_OS
    char arrayOwner[100][256] = {0};
    char arrayWindow[100][256] = {0};
    int  arrayId[100] = {0};

    int count = 0;
    IYouMeVoiceEngine::getInstance()->GetWindowInfoList(*arrayOwner, *arrayWindow, arrayId, &count);
    
    youmecommon::Value jsonRoot;
    jsonRoot["count"] = youmecommon::Value(count);

    for( int i = 0; i < count; ++i ){
        youmecommon::Value jsonWindowInfo;
        jsonWindowInfo["id"] = youmecommon::Value(arrayId[i] );
        jsonWindowInfo["owner"] = youmecommon::Value( arrayOwner[i] );
        jsonWindowInfo["window"] = youmecommon::Value( arrayWindow[i] );
        
        jsonRoot["winlist"].append(jsonWindowInfo);
    }
    
    TSK_DEBUG_INFO("window count[%d] handle[%s]", count, XStringToUTF8(jsonRoot.toSimpleString()).c_str());

    return XStringToUTF8(jsonRoot.toSimpleString()).c_str();
#endif

    return NULL;
}
