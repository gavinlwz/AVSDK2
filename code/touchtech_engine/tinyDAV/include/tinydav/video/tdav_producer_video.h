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

#ifndef TINYDAV_PRODUCER_VIDEO_H_
#define TINYDAV_PRODUCER_VIDEO_H_

#include "tinydav_config.h"

#include "tinymedia/tmedia_producer.h"

#define TDAV_PRODUCER_VIDEO(self) ((tdav_producer_video_t *)self)

TDAV_BEGIN_DECLS

typedef void (*videoRenderCb_t)(int32_t sessionId, int32_t i4Width, int32_t i4Height, int32_t i4Rotation, int32_t i4BufSize, const void *vBuf,uint64_t timestamp, int videoid);

typedef struct tdav_producer_video_s
{
    TMEDIA_DECLARE_PRODUCER;
    
    videoRenderCb_t producer_callback;
    
}tdav_producer_video_t;

TINYDAV_API int tdav_producer_video_init(tdav_producer_video_t *self);
TINYDAV_API int tdav_producer_video_cmp(const tsk_object_t *producer1, const tsk_object_t *producer2);
TINYDAV_API int tdav_producer_video_set(tdav_producer_video_t *self, const tmedia_param_t *param);
TINYDAV_API int tdav_producer_video_deinit(tdav_producer_video_t *self);

#define TDAV_DECLARE_PRODUCER_VIDEO tdav_producer_video_t __producer_video__

TDAV_END_DECLS

#endif /* TINYDAV_PRODUCER_VIDEO_H_ */
