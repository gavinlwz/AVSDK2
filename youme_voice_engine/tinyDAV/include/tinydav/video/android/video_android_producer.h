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

#ifndef YOUME_VIDEO_ANDROID_PRODUCER_H_
#define YOUME_VIDEO_ANDROID_PRODUCER_H_

#include "tsk.h"
#include "video_android.h"
#include "tinydav/video/tdav_converter_video.h"
#include "tinydav/video/tdav_producer_video.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct video_producer_android_s
{
    TDAV_DECLARE_PRODUCER_VIDEO;
    
    void *videoInstHandle;
    struct buffer_t
    {
        void *ptr;
        int size;
        int index;
    } buffer;
    
    struct buffer_t buffer_child;
    struct buffer_t buffer_local;
    
    TSK_DECLARE_SAFEOBJ;
    
} video_producer_android_t;
    
extern const struct tmedia_producer_plugin_def_s *video_producer_android_plugin_def_t;
    
// 设备采集的原始视频数据处理接口
int video_producer_android_handle_data(const struct video_producer_android_s *_self,
                                       const void *videoFrame,
                                       int nFrameSize,
                                       int nWidth,
                                       int nHeight,
                                       uint64_t nTimestamp,
                                       int fmt);
    
int video_producer_android_handle_data_gles(const struct video_producer_android_s *_self,
                                            const void *videoFrame,
                                            int nWidth,
                                            int nHeight,
                                            int videoid,
                                            uint64_t nTimestamp);
    
int video_producer_android_handle_data_new(const struct video_producer_android_s *_self,
                                                const void *videoFrame,
                                                int nFrameSize,
                                                int nWidth,
                                                int nHeight,
                                                int videoid,
                                                uint64_t nTimestamp,
                                                int fmt);
// extern video jni采集接口
extern void JNI_Init_Video_Capturer(int nFps,
                                   int nWidth,
                                   int nHeight,
                                   tmedia_producer_t *pProducerInstance);
    
extern void JNI_Start_Video_Capturer();
extern void JNI_Stop_Video_Capturer();
    
enum VIDEO_CAPTURER_INIT_STATUS
{
    VIDEO_CAPTURER_BAD_VALUE = -2,
    VIDEO_CAPTURER_UNINITIALIZED = 0,
    VIDEO_CAPTURER_INIT_SUCCESS = 100
};
    
#ifdef __cplusplus
}
#endif

#endif /* YOUME_VIDEO_ANDROID_PRODUCER_H_ */
