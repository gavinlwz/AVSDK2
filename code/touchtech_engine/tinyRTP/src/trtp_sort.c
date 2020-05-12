//
//  trtp_sort.c
//  youme_voice_engine
//
//  Created by mac on 2017/5/25.
//  Copyright © 2017年 Youme. All rights reserved.
//

#include "tsk_string.h"
#include "tmedia_utils.h"
#include "trtp_rtp_packet.h"
#include "tinyrtp/trtp_manager.h"
#include "trtp_sort.h"


//--------------------------------------
//	rscode object definition begin
//--------------------------------------
static tsk_object_t *trtp_sort_ctor (tsk_object_t *_self, va_list *app)
{
    trtp_sort_t *self = _self;
    if (self)
    {
        self->session_id = -1;
        self->payload_type = -1;
        self->last_seq_num = -1;
        
        if(!(self->in_rtp_packets_list = tsk_list_create())) {
            TSK_DEBUG_ERROR("Failed to create rtp packets list.");
            return tsk_null;
        }
        self->running = tsk_false;
        self->thread_handle = tsk_null;
        
        self->session = tsk_null;
        self->callback = tsk_null;
        
        self->statistic = trtp_statistic_create();
        
        self->first_timestamp = tsk_gettimeofday_ms();
        self->last_timestamp = tsk_gettimeofday_ms();
    }
    return _self;
}

static tsk_object_t *trtp_sort_dtor (tsk_object_t *_self)
{
    trtp_sort_t *self = _self;
    if (self)
    {
        if(self->in_rtp_packets_list) {
            TSK_OBJECT_SAFE_FREE(self->in_rtp_packets_list);
        }
        
        if(self->statistic) {
            TSK_OBJECT_SAFE_FREE(self->statistic);
        }
    }
    return _self;
}

static int trtp_sort_cmp (const tsk_object_t *_p1, const tsk_object_t *_p2)
{
    const trtp_sort_t *p1 = _p1;
    const trtp_sort_t *p2 = _p2;
    if(p1 && p2) {
        return (int)(p1->session_id - p2->session_id);
    } else if(!p1 && !p2) {
        return 0;
    } else {
        return -1;
    }
}

static const tsk_object_def_t trtp_sort_def_s = {
    sizeof (trtp_sort_t),
    trtp_sort_ctor,
    trtp_sort_dtor,
    trtp_sort_cmp,
};
const tsk_object_def_t *trtp_sort_def_t = &trtp_sort_def_s;

//--------------------------------------
//	sort object definition end
//--------------------------------------

static tsk_list_item_t* trtp_sort_pop_packet_by_seq_num(trtp_sort_t *self, int seq_num) {
    tsk_list_item_t* item = tsk_null;
    tsk_list_lock(self->in_rtp_packets_list);
    tsk_list_foreach(item, self->in_rtp_packets_list) {
        if(((trtp_rtp_packet_t*)item->data)->header->seq_num == seq_num) {
            item = tsk_list_pop_item_by_data(self->in_rtp_packets_list, (tsk_object_t*)item->data);
            break;
        }
    }
    
    if((!item) && (tsk_list_count_all(self->in_rtp_packets_list) > 10)) {
        item = tsk_list_pop_first_item(self->in_rtp_packets_list);
    }
    tsk_list_unlock(self->in_rtp_packets_list);
    return item;
}

static void* TSK_STDCALL trtp_sort_thread_func(void *arg) {
    trtp_sort_t *self = (trtp_sort_t *)arg;
    TSK_DEBUG_ERROR("sort thread enters");
    
    int count = 0;
    
    self->running = tsk_true;
    while(1) {
        count++;
        if(0 == (count % 500)) {
            TSK_DEBUG_INFO("sort thread is running. session_id:%d", self->session_id);
        }

        tsk_list_item_t *item = tsk_null;
        if(self->last_seq_num < 0) {
            tsk_list_lock(self->in_rtp_packets_list);
            item = tsk_list_pop_first_item(self->in_rtp_packets_list);
            tsk_list_unlock(self->in_rtp_packets_list);
        } else {
            item = trtp_sort_pop_packet_by_seq_num(self, self->last_seq_num + 1);
        }
        if(item) {
            self->last_seq_num = ((trtp_rtp_packet_t*)item->data)->header->seq_num;
            
            if (((trtp_rtp_packet_t*)item->data)->header->payload_type == TRTP_H264_MP)
            {
                char *filename = tsk_null;
                tsk_sprintf(&filename, "/recv_video_rtp_%d.txt", self->session_id);
                TMEDIA_DUMP_RTP_TO_FILE(filename, ((trtp_rtp_packet_t*)item->data));
                tsk_free((void**)&filename);
            } 
			else if (((trtp_rtp_packet_t*)item->data)->header->payload_type == TRTP_CUSTOM_FORMAT)
			{
				 
			}
			else {
                char *filename = tsk_null;
                tsk_sprintf(&filename, "/recv_audio_rtp_%d.txt", self->session_id);
                TMEDIA_DUMP_RTP_TO_FILE(filename, ((trtp_rtp_packet_t*)item->data));
                tsk_free((void**)&filename);
            }
            
            // rtp packet statistic begin
            //trtp_dot_t* dot = trtp_dot_create((trtp_rtp_packet_t*)item->data);
            //trtp_statistic_push_dot(self->statistic, dot);
            //TSK_OBJECT_SAFE_FREE(dot);
            // rtp packet statistic end
            
            if(self->callback && self->session) {
                self->callback(self->session, (trtp_rtp_packet_t*)(item->data));
            }
            
            TSK_OBJECT_SAFE_FREE(item);
        } else {
            tsk_thread_sleep(10);
        }

        if(!self->running) {
            break;
        }
    }
    
    TSK_DEBUG_INFO("sort thread exits");
    return tsk_null;
}

//--------------------------------------
//	rscode object extern api
//--------------------------------------
trtp_sort_t* trtp_sort_create(int session_id, int payload_type, tsk_object_t* session, trtp_sort_callback_f callback) {
    trtp_sort_t* self;
    self = tsk_object_new (trtp_sort_def_t);
    if(self) {
        self->session_id = session_id;
        self->payload_type = payload_type;
        self->session = session;
        self->callback = callback;
    }
    return self;
}

int trtp_sort_start(trtp_sort_t *self) {
    if(!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    if(self->running) {
        return 0;
    }
    
    if(!self->thread_handle) {
        int ret = tsk_thread_create(&self->thread_handle, trtp_sort_thread_func, self);
        if((0 != ret) && (!self->thread_handle)) {
            TSK_DEBUG_ERROR("Failed to create rscode thread");
            return -2;
        }
        ret = tsk_thread_set_priority(self->thread_handle, TSK_THREAD_PRIORITY_TIME_CRITICAL);
    }
    
    return 0;
}
int trtp_sort_stop(trtp_sort_t *self) {
    if(!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    if(!self->running) {
        return 0;
    }
    TSK_DEBUG_INFO("stop sort thread");
    self->running = tsk_false;
    tsk_thread_join(&self->thread_handle);
    TSK_DEBUG_INFO("stop sort thread completely");
    return 0;
}

int trtp_sort_push_rtp_packet(trtp_sort_t *self, trtp_rtp_packet_t *packet) {
    if(!self || !packet) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    tsk_list_lock(self->in_rtp_packets_list);
    tsk_object_ref(TSK_OBJECT(packet));
    tsk_list_push_ascending_data(self->in_rtp_packets_list, (void **)&packet);
    tsk_list_unlock(self->in_rtp_packets_list);
    self->last_timestamp = tsk_gettimeofday_ms();
    return 0;
}

trtp_sort_t* trtp_sort_select_by_sessionid_payloadtype(tsk_list_t* list, int session_id, int payload_type) {
    tsk_list_item_t *item = tsk_null;
    trtp_sort_t *sort = tsk_null;
    
    if(!list) {
        TSK_DEBUG_ERROR("*** sort list is null ***");
        return tsk_null;
    }
    
    tsk_list_lock(list);
    tsk_list_foreach(item, list) {
        if((TRTP_SORT(item->data)->session_id == session_id) && (TRTP_SORT(item->data)->payload_type == payload_type)) {
            sort = TRTP_SORT(item->data);
            break;
        }
    }
    tsk_list_unlock(list);
    
    return sort;
}

uint64_t trtp_sort_get_duration(trtp_sort_t *self) {
    return self->last_timestamp - self->first_timestamp;
}
