/*******************************************************************
 *  Copyright(c) 2015-2020 YOUME All rights reserved.
 *
 *  简要描述:游密音频通话引擎
 *
 *  当前版本:1.0
 *  作者:kenpang@youme.im
 *  日期:2017.2.24
 *  说明:对外发布接口
 ******************************************************************/

#include "tinydav/video/tdav_producer_video.h"

#include "tinymedia/tmedia_defaults.h"
#include "tinymedia/tmedia_common.h"

#include "tsk_string.h"
#include "tsk_debug.h"

#define TDAV_PRODUCER_VIDEO_CHROMA tmedia_chroma_yuv420p
#define TDAV_PRODUCER_VIDEO_FPS 30
#define TDAV_PRODUCER_VIDEO_WIDTH 640
#define TDAV_PRODUCER_VIDEO_HEIGHT 480

int tdav_producer_video_init(tdav_producer_video_t *self)
{
    int ret;
    
    if (!self)
    {
        TSK_DEBUG_ERROR("Invalid parameter");
        
        return -1;
    }
    
    ret = tmedia_producer_init(TMEDIA_PRODUCER(self));
    if (ret != 0)
    {
        TSK_DEBUG_ERROR("tmedia_producer_init failed");
        
        return ret;
    }
    
    // 初始化video参数
    TMEDIA_PRODUCER(self)->video.chroma = TDAV_PRODUCER_VIDEO_CHROMA;
    TMEDIA_PRODUCER(self)->video.fps = TDAV_PRODUCER_VIDEO_FPS;
    TMEDIA_PRODUCER(self)->video.width = TDAV_PRODUCER_VIDEO_WIDTH;
    TMEDIA_PRODUCER(self)->video.height = TDAV_PRODUCER_VIDEO_HEIGHT;
    
    self->producer_callback = tsk_null;
    
    TSK_DEBUG_INFO("tmedia_producer_init succ(chroma=%d, fps=%d, width=%d, height=%d)",
                   TMEDIA_PRODUCER(self)->video.chroma, TMEDIA_PRODUCER(self)->video.fps,
                   (int)TMEDIA_PRODUCER(self)->video.width, (int)TMEDIA_PRODUCER(self)->video.height);
    
    return ret;
}

int tdav_producer_video_cmp(const tsk_object_t *producer1, const tsk_object_t *producer2)
{
    int ret;
    
    tsk_subsat_int32_ptr(producer1, producer2, &ret);
    
    return ret;
}

int tdav_producer_video_set(tdav_producer_video_t *self, const tmedia_param_t *param)
{
    if (!self) {
        return 0;
    }
    
    if (param->plugin_type == tmedia_ppt_producer) {
        if (param->value_type == tmedia_pvt_pvoid) {
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_VIDEO_RENDER_CB)) {
                self->producer_callback = (videoRenderCb_t)param->value;
                TSK_DEBUG_INFO("video render callback function set in producer: producer=%d, cb=%d", self, self->producer_callback);
            }
        }
        if (param->value_type == tmedia_pvt_int32) {
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_SESSION_ID)) {
                TMEDIA_PRODUCER (self)->session_id = (uint64_t)(TSK_TO_INT32 ((uint8_t *)param->value));
                TSK_DEBUG_INFO("set session id:%llu", TMEDIA_PRODUCER (self)->session_id);
            }
        }
        if (param->value_type == tmedia_pvt_int32) {
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_NEED_OPEN_CAMERA)) {
                TMEDIA_PRODUCER (self)->camera.needOpen = (tsk_bool_t)(TSK_TO_INT32 ((uint8_t *)param->value));
                TSK_DEBUG_INFO("set camera need open:%d", TMEDIA_PRODUCER (self)->camera.needOpen);
            }
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_VIDEO_WIDTH)) {
                TMEDIA_PRODUCER (self)->camera.width = (TSK_TO_INT32 ((uint8_t *)param->value));
                TSK_DEBUG_INFO("set camera width:%d", TMEDIA_PRODUCER (self)->camera.width);
            }
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_VIDEO_HEIGHT)) {
                TMEDIA_PRODUCER (self)->camera.height = (TSK_TO_INT32 ((uint8_t *)param->value));
                TSK_DEBUG_INFO("set camera height:%d", TMEDIA_PRODUCER (self)->camera.height);
            }
            if (tsk_striequals (param->key, TMEDIA_PARAM_KEY_SCREEN_ORIENTATION)) {
                TMEDIA_PRODUCER (self)->camera.screenOrientation = (TSK_TO_INT32 ((uint8_t *)param->value));
                TSK_DEBUG_INFO("set camera screenOrientation:%d", TMEDIA_PRODUCER (self)->camera.screenOrientation);
            }
        }
    }
    return 0;
}

int tdav_producer_video_deinit(tdav_producer_video_t *self)
{
    int ret;
    
    if (!self)
    {
        TSK_DEBUG_ERROR("Invalid parameter");
        
        return -1;
    }
    
    ret = tmedia_producer_deinit((TMEDIA_PRODUCER(self)));
    if (ret != 0)
    {
        TSK_DEBUG_ERROR("tmedia_producer_deinit failed");
        
        return ret;
    }
    
    TSK_DEBUG_INFO("tmedia_producer_deinit succ");
    
    return ret;
}
