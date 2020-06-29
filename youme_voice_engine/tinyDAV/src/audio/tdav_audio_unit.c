//
//  tdav_audio_resample.c
//  youme_voice_engine
//
//  Created by mac on 2017/8/10.
//  Copyright © 2017年 Youme. All rights reserved.
//
#include "tsk_memory.h"
#include "tsk_debug.h"
#include "tmedia_utils.h"
#include "YouMeEngineManagerForQiniuCWrapper.h"
#include "tdav_audio_unit.h"

static tsk_object_t *tdav_audio_unit_ctor (tsk_object_t *_self, va_list *app)
{
    tdav_audio_unit_t *self = _self;
    if (self)
    {
        self->size = 0;
        self->timestamp = 0;
        self->data = tsk_calloc(1, self->size);
    }
    return _self;
}

static tsk_object_t *tdav_audio_unit_dtor (tsk_object_t *_self)
{
    tdav_audio_unit_t *self = _self;
    if (self)
    {
        if(self->data) {
            tsk_free(&(self->data));
        }
        self->size = 0;
        self->timestamp = 0;
    }
    return _self;
}

static int tdav_audio_unit_cmp (const tsk_object_t *_p1, const tsk_object_t *_p2)
{
    const tdav_audio_unit_t *p1 = _p1;
    const tdav_audio_unit_t *p2 = _p2;
    if(p1 && p2) {
        return (int)(p1->timestamp - p2->timestamp);
    } else if(!p1 && !p2) {
        return 0;
    } else {
        return -1;
    }
}

static const tsk_object_def_t tdav_audio_unit_def_s = {
    sizeof (tdav_audio_unit_t),
    tdav_audio_unit_ctor,
    tdav_audio_unit_dtor,
    tdav_audio_unit_cmp,
};

const tsk_object_def_t *tdav_audio_unit_def_t = &tdav_audio_unit_def_s;

tdav_audio_unit_t* tdav_audio_unit_create(uint32_t size) {
    tdav_audio_unit_t* self;
    self = tsk_object_new (tdav_audio_unit_def_t);
    if(self) {
        self->size = size;
        self->data = tsk_realloc(self->data, self->size);
    }
    return self;
}

/********************************************************************************/

static void* TSK_STDCALL tdav_audio_resample_thread_func(void *arg) {
    tdav_audio_resample_t *self = (tdav_audio_resample_t *)arg;
    TSK_DEBUG_INFO("tdav_audio_resample_thread_func thread enters");
    
    while(1) {
        if(!self->running) {
            break;
        } else {
            if(self->resampler && self->resampler_mutex) {
                tsk_mutex_lock(self->resampler_mutex);

                tsk_list_item_t* item = tsk_null;
                //发送端直接从 list 读数据
                tsk_list_lock(self->audio_in);
                item = tsk_list_pop_first_item(self->audio_in);
                tsk_list_unlock(self->audio_in);
                
                if(item) {
                    tdav_audio_unit_t* audio_unit = TDAV_AUDIO_UNIT(item->data);
                    //TMEDIA_DUMP_TO_FILE("/pcm_data_16_2.bin", audio_unit->data, audio_unit->size);
                    uint32_t out_size = audio_unit->size * self->samplerate_out / self->samplerate_in;
                    if(out_size > self->temp_buffer_size) {
                        self->temp_buffer = tsk_realloc(self->temp_buffer, out_size);
                        self->temp_buffer_size = out_size;
                    }
                    uint32_t in_samples  = audio_unit->size / sizeof(spx_int16_t);
                    uint32_t out_samples = out_size / sizeof(spx_int16_t);
                    
                    // 判断是否需要转频
                    if (self->samplerate_out != self->samplerate_in) {
                        tsk_mutex_lock(self->resampler_mutex);
                        speex_resampler_process_int(self->resampler, 0, (const spx_int16_t*)audio_unit->data, &in_samples, (spx_int16_t*)self->temp_buffer, &out_samples);
                        //TMEDIA_DUMP_TO_FILE("/pcm_data_48.bin", self->temp_buffer, self->temp_buffer_size);
                        tsk_mutex_unlock(self->resampler_mutex);
                    } else {
                        memcpy(self->temp_buffer, audio_unit->data, out_size);
                    }
                    
                    if((self->sessionid == 0) && (self->isMixing == tsk_true)) {
                        YM_Qiniu_onAudioFrameMixedCallback(self->temp_buffer, self->temp_buffer_size, audio_unit->timestamp);
                    } else if(self->isMixing == tsk_false) {
                        YM_Qiniu_onAudioFrameCallback(self->sessionid, self->temp_buffer, self->temp_buffer_size, audio_unit->timestamp);
                    }
                    
                    TSK_OBJECT_SAFE_FREE(item);
                } else {
                    tsk_mutex_unlock(self->resampler_mutex);
                    tsk_thread_sleep(20);
                    continue;
                }

                tsk_mutex_unlock(self->resampler_mutex);
            } else {
                tsk_thread_sleep(20);
            }
        }
    }
    
    TSK_DEBUG_INFO("tdav_audio_resample_thread_func thread exits");
    return tsk_null;
}

static tsk_object_t *tdav_audio_resample_ctor (tsk_object_t *_self, va_list *app)
{
    tdav_audio_resample_t *self = _self;
    if (self)
    {
        self->sessionid = -1;
        self->running = tsk_false;
        self->audio_in = tsk_list_create();
        if(!self->thread_handle) {
            self->running = tsk_true;
            int ret = tsk_thread_create(&self->thread_handle, tdav_audio_resample_thread_func, self);
            if((0 != ret) && (!self->thread_handle)) {
                TSK_DEBUG_ERROR("Failed to create neteq thread");
                return tsk_null;
            }
            ret = tsk_thread_set_priority(self->thread_handle, TSK_THREAD_PRIORITY_TIME_CRITICAL);
        }
    }
    return _self;
}

static tsk_object_t *tdav_audio_resample_dtor (tsk_object_t *_self)
{
    tdav_audio_resample_t *self = _self;
    if (self)
    {
        if(self->running) {
            self->running = tsk_false;
            tsk_thread_join(&self->thread_handle);
        }
        if(self->resampler) {
            speex_resampler_destroy(self->resampler);
            self->resampler = tsk_null;
        }
        
        if(self->resampler_mutex) {
            tsk_mutex_destroy(&self->resampler_mutex);
        }
        
        TSK_OBJECT_SAFE_FREE(self->audio_in);
        
        if(self->temp_buffer) {
            tsk_free(&self->temp_buffer);
        }
    }
    return _self;
}

static int tdav_audio_resample_cmp (const tsk_object_t *_p1, const tsk_object_t *_p2)
{
    const tdav_audio_resample_t *p1 = _p1;
    const tdav_audio_resample_t *p2 = _p2;
    if(p1 && p2) {
        return (int)(p1->sessionid - p2->sessionid);
    } else if(!p1 && !p2) {
        return 0;
    } else {
        return -1;
    }
}

static const tsk_object_def_t tdav_audio_resample_def_s = {
    sizeof (tdav_audio_resample_t),
    tdav_audio_resample_ctor,
    tdav_audio_resample_dtor,
    tdav_audio_resample_cmp,
};

const tsk_object_def_t *tdav_audio_resample_def_t = &tdav_audio_resample_def_s;

tdav_audio_resample_t* tdav_audio_resample_create(uint32_t sessionid, tsk_boolean_t isMixing, uint32_t channels, uint32_t samplerate_in, uint32_t samplerate_out) {
    tdav_audio_resample_t* self;
    self = tsk_object_new (tdav_audio_resample_def_t);
    if(self) {
        self->sessionid = sessionid;
        self->isMixing = isMixing;
        self->samplerate_in = samplerate_in;
        self->samplerate_out = samplerate_out;
        self->temp_buffer_size = 1;
        self->temp_buffer = tsk_realloc(self->temp_buffer, self->temp_buffer_size);
        if(self->resampler_mutex) {
            tsk_mutex_destroy(&self->resampler_mutex);
        }
        self->resampler_mutex = tsk_mutex_create();
        
        if(self->resampler) {
            speex_resampler_destroy(self->resampler);
        }
        self->resampler = speex_resampler_init(channels, samplerate_in, samplerate_out, 3, NULL);
    }
    return self;
}

int tdav_audio_resample_push(tdav_audio_resample_t* self, tdav_audio_unit_t* audio_unit) {
    tsk_list_lock(self->audio_in);
    tsk_object_ref(TSK_OBJECT(audio_unit));
    tsk_list_push_back_data(self->audio_in, (void **)&audio_unit);
    tsk_list_unlock(self->audio_in);
    return 0;
}
