/*******************************************************************
 *  Copyright(c) 2015-2020 YOUME All rights reserved.
 *
 *  简要描述:游密音频通话引擎
 *
 *  当前版本:1.0
 *  作者:brucewang
 *  日期:2015.9.30
 *  说明:对外发布接口
 *
 *  取代版本:0.9
 *  作者:brucewang
 *  日期:2015.9.15
 *  说明:内部测试接口
 ******************************************************************/
#ifndef TINYDAV_PRODUCER_FILE_H
#define TINYDAV_PRODUCER_FILE_H

#include "tinydav/audio/tdav_producer_audio.h"
#include "tinydav_config.h"
#include "tsk_mutex.h"
#include "youme_buffer.h"
#include "XTimerCWrapper.h"

TDAV_BEGIN_DECLS

typedef struct tdav_producer_file_s
{
    TDAV_DECLARE_PRODUCER_AUDIO;

    void **audioUnitHandle;

    unsigned started : 1;

    unsigned paused : 1;

    unsigned muted;

    struct
    {
        struct
        {
            void *buffer;
            tsk_size_t size;
        } chunck;

        YOUMEBuffer *buffer;

        tsk_size_t size;

    } ring;
    long timerID;
} tdav_producer_file_t;

TINYDAV_GEXTERN const tmedia_producer_plugin_def_t *tdav_producer_file_plugin_def_t;

TDAV_END_DECLS


#endif