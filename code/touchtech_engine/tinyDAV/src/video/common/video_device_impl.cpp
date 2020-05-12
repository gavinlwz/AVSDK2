/*******************************************************************
 *  Copyright(c) 2015-2020 YOUME All rights reserved.
 *
 *  简要描述:游密音视频通话引擎
 *
 *  当前版本:1.0
 *  作者:kenpang@youme.im
 *  日期:2017.2.23
 *  说明:对外发布接口
 ******************************************************************/

#include "YouMeVoiceEngine.h"
#include "tinydav/video/android/video_android_device_impl.h"
#include "tmedia_utils.h"

VideoAndroidDeviceCallbackImpl::VideoAndroidDeviceCallbackImpl()
{
}

VideoAndroidDeviceCallbackImpl::~VideoAndroidDeviceCallbackImpl()
{
}

int32_t VideoAndroidDeviceCallbackImpl::CapturedDataIsAvailable (const void *videoFrame,
                                                                 const uint32_t nFrameSize,
                                                                 const uint32_t nFps,
                                                                 const uint32_t nWidth,
                                                                 const uint32_t nHeight)
{
    if (!m_pProducer)
    {
        TSK_DEBUG_WARN("No wrapped producer");
        
        return 0;
    }
    
    //TSK_DEBUG_INFO("CapturedDataIsAvailable(nFrameSize=%d, nFps=%d, nWidth=%d, nHeight=%d)",
    //               nFrameSize, nFps, nWidth, nHeight);
    
    // TODO:这里调用处理视频原始数据接口
    return 0;
    /*
    return video_producer_android_handle_data(m_pProducer,
                                              videoFrame,
                                              nFrameSize,
                                              nFps,
                                              0,
                                              nWidth,
                                              nHeight);
     
     */
}

int32_t VideoAndroidDeviceCallbackImpl::NeedMoreRenderedData (const uint32_t nFrameSize,
                                                          const uint32_t nFps,
                                                          const uint32_t nWidth,
                                                          const uint32_t nHeight,
                                                          void *videoFrame,
                                                          uint32_t &nFrameOut)
{
    int frameOut;
    if (!m_pConsumer)
    {
        TSK_DEBUG_WARN("No wrapped consumer");
        
        return 0;
    }
    
    TSK_DEBUG_INFO("NeedMoreRenderedData(nFrameSize=%d, nFps=%d, nWidth=%d, nHeight=%d)",
                   nFrameSize, nFps, nWidth, nHeight);
    
    // TODO:这里调用获取视频原始数据接口
    frameOut = video_consumer_android_get_data(m_pConsumer,
                                                videoFrame,
                                                nFrameSize,
                                                nFps,
                                                nWidth,
                                                nHeight);
    if (frameOut < 0)
    {
        TSK_DEBUG_WARN("Invalid nFrameOut(%d)", frameOut);
        
        return -1;
    }
    
    nFrameOut = (uint32_t)frameOut;
    return 0;
}

void VideoAndroidDeviceCallbackImpl::onPreviewFrame(const void *data, int size, int width, int height, uint64_t timestamp, int fmt)
{
    if (!m_pProducer)
    {
        TSK_DEBUG_WARN("No wrapped producer");
        return;
    }

    tmedia_session_t* session = CYouMeVoiceEngine::getInstance()->getMediaSession(tmedia_video);
    if(!session) {
        TSK_DEBUG_WARN("get session is NULL");
        return;
    }
    
    // TODO:这里调用处理视频原始数据接口
    //TSK_DEBUG_INFO("VideoAndroidDeviceCallbackImpl::onPreviewFrame() is called");
    tsk_mutex_lock(TDAV_SESSION_AV(session)->producer_mutex);
    if(TDAV_SESSION_AV(session)->producer) {
        video_producer_android_handle_data((const struct video_producer_android_s *)TDAV_SESSION_AV(session)->producer,
                                              data,
                                              size,
                                              width,
                                              height,
                                              timestamp,
                                              fmt);
    }
    tsk_mutex_unlock(TDAV_SESSION_AV(session)->producer_mutex);
    tsk_object_unref(TSK_OBJECT(session));
}

void VideoAndroidDeviceCallbackImpl::onPreviewFrameNew(const void *data, int size, int width, int height, int videoid, uint64_t timestamp, int fmt){

    if (!m_pProducer) {
        TSK_DEBUG_WARN("No wrapped producer");
        return ;
    }
    
    
    tmedia_session_t* session = CYouMeVoiceEngine::getInstance()->getMediaSession(tmedia_video);
    if(!session) {
        TSK_DEBUG_WARN("get session is NULL");
        tsk_object_unref(TSK_OBJECT(session));
        return ;
    }
    
    // TODO:这里调用处理视频原始数据接口
    tsk_mutex_lock(TDAV_SESSION_AV(session)->producer_mutex);
    if(TDAV_SESSION_AV(session)->producer) {
        video_producer_android_handle_data_new((const struct video_producer_android_s *)TDAV_SESSION_AV(session)->producer,
                                                      data, size, width, height, videoid, timestamp, fmt);
    }
    tsk_mutex_unlock(TDAV_SESSION_AV(session)->producer_mutex);
    tsk_object_unref(TSK_OBJECT(session));
}

int VideoAndroidDeviceCallbackImpl::onPreviewFrameGLES(const void *data, int width, int height, int videoid, uint64_t timestamp, int fmt)
{
    int ret = 0;
    if (!m_pProducer) {
        TSK_DEBUG_WARN("No wrapped producer");
        return 0 ;
    }
    

    tmedia_session_t* session = CYouMeVoiceEngine::getInstance()->getMediaSession(tmedia_video);
    if(!session) {
        TSK_DEBUG_WARN("get session is NULL");
        tsk_object_unref(TSK_OBJECT(session));
        return 0;
    }
    
    // TODO:这里调用处理视频原始数据接口
    tsk_mutex_lock(TDAV_SESSION_AV(session)->producer_mutex);
    if(TDAV_SESSION_AV(session)->producer) {
       ret = video_producer_android_handle_data_gles((const struct video_producer_android_s *)TDAV_SESSION_AV(session)->producer,
                                               data, width, height, videoid, timestamp);
    }
    tsk_mutex_unlock(TDAV_SESSION_AV(session)->producer_mutex);
    tsk_object_unref(TSK_OBJECT(session));
    return ret;
}
