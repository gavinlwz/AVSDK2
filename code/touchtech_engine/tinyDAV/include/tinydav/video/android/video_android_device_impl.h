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

#ifndef YOUME_VIDEO_ANDROID_DEVICE_IMPL_H_
#define YOUME_VIDEO_ANDROID_DEVICE_IMPL_H_

#include "video_android_device_impl.h"
#include "video_android_producer.h"
#include "video_android_consumer.h"
#include "ICameraManager.hpp"

class VideoAndroidDeviceCallbackImpl : public CameraPreviewCallback
{
public:
    VideoAndroidDeviceCallbackImpl ();
    ~VideoAndroidDeviceCallbackImpl ();
    
    int32_t CapturedDataIsAvailable (const void *videoFrame,
                                     const uint32_t nFrameSize,
                                     const uint32_t nFps,
                                     const uint32_t nWidth,
                                     const uint32_t nHeight);
    
    int32_t NeedMoreRenderedData (const uint32_t nFrameSize,
                                  const uint32_t nFps,
                                  const uint32_t nWidth,
                                  const uint32_t nHeight,
                                  void *videoFrame,
                                  uint32_t &nFrameOut);
    void onPreviewFrame(const void *data, int size, int width, int height, uint64_t timestamp, int fmt = VIDEO_FMT_YUV420P) override;
    void onPreviewFrameNew(const void *data, int size, int width, int height, int videoid, uint64_t timestamp, int fmt = VIDEO_FMT_YUV420P)override;
    int  onPreviewFrameGLES(const void *data, int width, int height, int videoid, uint64_t timestamp, int fmt = VIDEO_FMT_YUV420P) override;
    
    inline void SetConsumer (const struct video_consumer_android_s *pConsumer)
    {
        m_pConsumer = pConsumer;
    }
    inline void SetProducer (const struct video_producer_android_s *pProducer)
    {
        m_pProducer = pProducer;
    }
private:
    const struct video_producer_android_s *m_pProducer; // must be const and must not take reference
    const struct video_consumer_android_s *m_pConsumer; // mut be const and must not take reference
};

#endif /* YOUME_VIDEO_ANDROID_DEVICE_IMPL_H_ */
