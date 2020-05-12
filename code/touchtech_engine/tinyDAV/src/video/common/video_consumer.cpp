/*******************************************************************
 *  Copyright(c) 2015-2020 YOUME All rights reserved.
 *
 *  简要描述:游密音视频通话引擎
 *
 *  当前版本:1.0
 *  作者:brucewang
 *  日期:2015.9.30
 *  说明:对外发布接口
 ******************************************************************/
#include "tinydav/video/android/video_android.h"
#include "tinydav/video/android/video_android_consumer.h"

#include "tinydav/video/tdav_consumer_video.h"
#include "tinymedia/tmedia_defaults.h"

#include "tsk_debug.h"
#include "tsk_memory.h"
#include "tsk_string.h"

#include "tinyrtp/rtp/trtp_rtp_header.h"

int video_consumer_android_get_data (const video_consumer_android_t *_self,
                                     void *videoFrame,
                                     int nFrameSize,
                                     int nFps,
                                     int nWidth,
                                     int nHeight)
{
    int nFrameOut = 0;
    if (!_self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    
    video_consumer_android_t *self = const_cast<video_consumer_android_t *> (_self);
    
    if (self->buffer.index == self->buffer.size)
    {
        if ((tdav_consumer_video_get (TDAV_CONSUMER_VIDEO (self), self->buffer.ptr, self->buffer.size)) != self->buffer.size)
        {
            self->buffer.index = self->buffer.size;
            return 0;
        }
        self->buffer.index = 0;
        tdav_consumer_video_tick (TDAV_CONSUMER_VIDEO (self));
    }
    
    int nFrameInBytes = nFrameSize;
    if (_self->buffer.index + nFrameInBytes <= _self->buffer.size)
    {
        memcpy (videoFrame, (((uint8_t *)self->buffer.ptr) + self->buffer.index), nFrameInBytes);
    }
    else
    {
        TSK_DEBUG_ERROR ("_self->buffer.index(%d) + nFrameInBytes(%d) > _self->buffer.size(%d)", _self->buffer.index, nFrameInBytes, _self->buffer.size);
    }
    self->buffer.index += nFrameInBytes;
    TSK_CLAMP (0, self->buffer.index, self->buffer.size);
    nFrameOut = nFrameSize;
    
    return nFrameOut;
}


/* ============ Media Consumer Interface ================= */
static int video_consumer_android_set (tmedia_consumer_t *_self, const tmedia_param_t *param)
{
    video_consumer_android_t *self = (video_consumer_android_t *)_self;
    int ret = tdav_consumer_video_set (TDAV_CONSUMER_VIDEO (self), param);

    return ret;
} 

static int video_consumer_android_prepare (tmedia_consumer_t *_self, const tmedia_codec_t *codec)
{
    video_consumer_android_t *self = (video_consumer_android_t *)_self;
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    
    // create video instance
    if (!(self->videoInstHandle = video_android_instance_create (TMEDIA_CONSUMER (self)->session_id, 0)))
    {
        TSK_DEBUG_ERROR ("Failed to create video instance handle");
        return -1;
    }
    // initialize input parameters from the codec information
    TMEDIA_CONSUMER(self)->video.fps = TMEDIA_CODEC_VIDEO(codec)->in.fps;
    // in
    TMEDIA_CONSUMER(self)->video.in.chroma = tmedia_chroma_yuv420p;
    TMEDIA_CONSUMER(self)->video.in.width = TMEDIA_CODEC_VIDEO(codec)->in.width;
    TMEDIA_CONSUMER(self)->video.in.height = TMEDIA_CODEC_VIDEO(codec)->in.height;
    // out(display)
    TMEDIA_CONSUMER(self)->video.display.chroma = tmedia_chroma_yuv420p;
    TMEDIA_CONSUMER(self)->video.display.width = TMEDIA_CONSUMER(self)->video.in.width;
    TMEDIA_CONSUMER(self)->video.display.height = TMEDIA_CONSUMER(self)->video.in.height;
    
    TSK_DEBUG_INFO("video_consumer_android_prepare(fps=%d, chroma=%d, width=%d, height=%d)",
                   TMEDIA_CONSUMER(self)->video.fps, TMEDIA_CONSUMER(self)->video.in.chroma,
                   (int)TMEDIA_CONSUMER(self)->video.in.width, (int)TMEDIA_CONSUMER(self)->video.in.height);
    
    // prepare playout device and update output parameters
    int ret = video_android_instance_prepare_consumer (self->videoInstHandle, &_self);
    
    // now that the producer is prepared we can initialize internal buffer using device caps
    if (ret == 0)
    {
        // TODO:这里调用jni video 渲染接口
#if 0
        JNI_Init_Video_Renderer(TMEDIA_CONSUMER(self)->video.fps,
                                TMEDIA_CONSUMER(self)->video.in.width,
                                TMEDIA_CONSUMER(self)->video.in.height,
                                _self);
#endif
        /*
        // allocate buffer
        // YUV420p每帧大小计算公式:width * height * 3/2(宽 * 高 * 3/2)
        int nSize = TMEDIA_CONSUMER(self)->video.in.width * TMEDIA_CONSUMER(self)->video.in.height * 3/2;
        TSK_DEBUG_INFO("video consumer buffer nSzie = %d", nSize);
        
        self->buffer.ptr = tsk_realloc(self->buffer.ptr, nSize);
        if (!self->buffer.ptr)
        {
            TSK_DEBUG_ERROR("Failed to allocate buffer with size = %d", nSize);
            
            self->buffer.size = 0;
        }
        
        memset(self->buffer.ptr, 0, nSize);
        self->buffer.size = nSize;
        self->buffer.index = 0;
        self->buffer.isFull = false;
        */
    }
    
    return ret;
}

static int video_consumer_android_start (tmedia_consumer_t *_self)
{
    video_consumer_android_t *self = (video_consumer_android_t *)_self;
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    TSK_DEBUG_INFO ("video_consumer_android_start");
    
    return 0;
}

static int video_consumer_android_consume (tmedia_consumer_t *_self, const void *data, tsk_size_t data_size, tsk_object_t *proto_hdr)
{
    video_consumer_android_t *self = (video_consumer_android_t *)_self;
    trtp_rtp_header_t* rtp_hdr = (trtp_rtp_header_t *)proto_hdr;
    tdav_consumer_video_t *consumer = (tdav_consumer_video_t *)_self;
    
    if (!self || !data  || !rtp_hdr)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    /* buffer is already decoded */
    //return tdav_consumer_video_put (TDAV_CONSUMER_VIDEO (self), data, data_size, proto_hdr);
    
    // 这里就不需要给到video post jitter buffer了
    // 由于JAVA的渲染是被动接收数据，只需要在这里把解码后的数据通过JNI回传至JAVA即可
    // 需要的参数是帧的宽高，session ID
    int video_display_width  = rtp_hdr->video_display_width;
    int video_display_height = rtp_hdr->video_display_height;
    
    if (!data_size || data_size == video_display_width * video_display_height * 3 / 2) {
        if (consumer->consumer_callback) {
            consumer->consumer_callback(rtp_hdr->session_id, video_display_width, video_display_height, 0, data_size, data, rtp_hdr->timestampl, rtp_hdr->video_id);
        }
    }
    
    return 0;
}

static int video_consumer_android_pause (tmedia_consumer_t *self)
{
    return 0;
}

static int video_consumer_android_stop (tmedia_consumer_t *_self)
{
    video_consumer_android_t *self = (video_consumer_android_t *)_self;
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    
    TSK_DEBUG_INFO("video_consumer_android_stop");
    
    // TODO:这里调用video jni渲染接口
    video_android_instance_stop_consumer(self->videoInstHandle);

    return 0;
}


//
//	SLES video consumer object definition
//
/* constructor */
static tsk_object_t *video_consumer_android_ctor (tsk_object_t *_self, va_list *app)
{
    video_consumer_android_t *self = (video_consumer_android_t *)_self;
    if (self)
    {
        /* init base */
        tdav_consumer_video_init (TDAV_CONSUMER_VIDEO (self));
    }
    return self;
}

/* destructor */
static tsk_object_t *video_consumer_android_dtor (tsk_object_t *_self)
{
    video_consumer_android_t *self = (video_consumer_android_t *)_self;
    if (self)
    {
        /* stop */
        video_consumer_android_stop (TMEDIA_CONSUMER (self));
        /* deinit self */
        if (self->videoInstHandle)
        {
            video_android_instance_destroy (&self->videoInstHandle);
        }
        TSK_FREE (self->buffer.ptr);
        /* deinit base */
        tdav_consumer_video_deinit (TDAV_CONSUMER_VIDEO (self));
    }
    
    return self;
}
/* object definition */
static const tsk_object_def_t video_consumer_android_def_s = {
    sizeof (video_consumer_android_t), video_consumer_android_ctor, video_consumer_android_dtor, tdav_consumer_video_cmp,
};
/* plugin definition*/
static const tmedia_consumer_plugin_def_t video_consumer_android_plugin_def_s =
{
    &video_consumer_android_def_s,
    tmedia_video,
    "ANDROID video consumer",
    
    video_consumer_android_set,
    tsk_null,
    video_consumer_android_prepare,
    video_consumer_android_start,
    video_consumer_android_consume,
    video_consumer_android_pause,
    video_consumer_android_stop
};
const tmedia_consumer_plugin_def_t *video_consumer_android_plugin_def_t = &video_consumer_android_plugin_def_s;
