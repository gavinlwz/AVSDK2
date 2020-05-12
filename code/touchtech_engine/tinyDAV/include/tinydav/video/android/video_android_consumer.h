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

#ifndef YOUME_VIDEO_ANDROID_CONSUMER_H_
#define YOUME_VIDEO_ANDROID_CONSUMER_H_

#include "tinysak_config.h"
#include "video_android.h"
#include "tinydav/video/tdav_consumer_video.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct video_consumer_android_s
{
    TDAV_DECLARE_CONSUMER_VIDEO;
    void *videoInstHandle;
    struct
    {
        void *ptr;
        tsk_bool_t isFull;
        int size;
        int index;
    } buffer;
} video_consumer_android_t;

extern const struct tmedia_consumer_plugin_def_s *video_consumer_android_plugin_def_t;

int video_consumer_android_get_data(const struct video_consumer_android_s *self,
                                    void *videoFrame,
                                    int nFrameSize,
                                    int nFps,
                                    int nWidth,
                                    int nHeight);
#if 0
// exter video jni渲染接口
extern void JNI_Init_Video_Renderer(int nFps,
                                    int nWidth,
                                    int nHeight,
                                    tmedia_consumer_t *pConsumerInstanse);

extern void JNI_Refresh_Video_Render(int sessionId, int nWidth, int nHeight, int nRotationDegree, int nBufSize, const void *buf);
#endif
    
#ifdef __cplusplus
}
#endif

#endif /* YOUME_VIDEO_ANDROID_CONSUMER_H_ */
