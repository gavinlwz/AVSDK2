//
//  trtp_statistic.c
//  youme_voice_engine
//
//  Created by mac on 2017/5/25.
//  Copyright © 2017年 Youme. All rights reserved.
//
#include "tsk_debug.h"
#include "tsk_time.h"
#include "tsk_string.h"
#include "tmedia_utils.h"
#include "trtp_statistic.h"

//--------------------------------------
//	trtp log object definition
//--------------------------------------
static tsk_object_t *trtp_dot_ctor (tsk_object_t *_self, va_list *app)
{
    trtp_dot_t *self = _self;
    if (self)
    {
    }
    return _self;
}

static tsk_object_t *trtp_dot_dtor (tsk_object_t *_self)
{
    trtp_dot_t *self = _self;
    if (self)
    {
    }
    return _self;
}

static int trtp_dot_cmp (const tsk_object_t *_p1, const tsk_object_t *_p2)
{
    const trtp_dot_t *p1 = _p1;
    const trtp_dot_t *p2 = _p2;
    if(p1 && p2) {
        return (int)(p1->seq_num - p2->seq_num);
    } else if(!p1 && !p2) {
        return 0;
    } else {
        return -1;
    }
}

static const tsk_object_def_t trtp_dot_def_s = {
    sizeof (trtp_dot_t),
    trtp_dot_ctor,
    trtp_dot_dtor,
    trtp_dot_cmp,
};
const tsk_object_def_t *trtp_dot_def_t = &trtp_dot_def_s;

//--------------------------------------
//	trtp statistic object definition
//--------------------------------------

static void* TSK_STDCALL trtp_statistic_thread_func(void *arg) {
    trtp_statistic_t *self = (trtp_statistic_t *)arg;
    TSK_DEBUG_INFO("statistic thread enters");
    int count = 0;
    self->running = tsk_true;
    while(1) {
        count++;
        if(0 == (count % TRTP_STATISTIC_COMPUTE_CYCLE)) {
            tsk_list_item_t* item = tsk_null;
            uint64_t now = tsk_gettimeofday_ms();
            tsk_list_lock(self->dots_list);
            tsk_list_foreach(item, self->dots_list) {
                if(TRTP_DOT((item->data))->recv_timestamp < (now - self->max_time_range)) {
                    tsk_list_remove_item(self->dots_list, item);
                }
            }
            self->total = tsk_list_count_all(self->dots_list);
            if(self->total > 0) {
                self->min_seq_num = TRTP_DOT((self->dots_list->head)->data)->seq_num;
                self->max_seq_num = TRTP_DOT((self->dots_list->tail)->data)->seq_num;
                self->loss = self->max_seq_num - self->min_seq_num + 1 - self->total;
                
                int max_consec_loss = 0;
                tsk_list_item_t* item = tsk_null;
                tsk_list_foreach(item, self->dots_list) {
                    if(item->next) {
                        int temp = TRTP_DOT(item->next->data)->seq_num - TRTP_DOT(item->data)->seq_num - 1;
                        max_consec_loss = (temp > max_consec_loss)?temp:max_consec_loss;
                    }
                }
                self->max_consec_loss = max_consec_loss;
            }
            tsk_list_unlock(self->dots_list);
            if(self->total > 0) {
                char *filename = tsk_null;
                tsk_sprintf(&filename, "/rtp_statistic_%d.txt", (int)self);
                TMEDIA_DUMP_LOG_TO_FILE(filename, "statistic seq_num:%d-%d total:%d loss:%d max_loss:%d timestamp:%lld-%lld=%lld\n",
                                        self->min_seq_num, self->max_seq_num,
                                        self->total, self->loss,
                                        self->max_consec_loss,
                                        TRTP_DOT((self->dots_list->tail)->data)->rtp_timestamp,
                                        TRTP_DOT((self->dots_list->head)->data)->rtp_timestamp,
                                        TRTP_DOT((self->dots_list->tail)->data)->rtp_timestamp-TRTP_DOT((self->dots_list->head)->data)->rtp_timestamp);
                tsk_free((void**)&filename);
                TSK_DEBUG_INFO("statistic seq_num:%d-%d total:%d loss:%d max_loss:%d timestamp:%lld-%lld=%lld",
                               self->max_seq_num, self->min_seq_num,
                               self->total, self->loss,
                               self->max_consec_loss,
                               TRTP_DOT((self->dots_list->tail)->data)->rtp_timestamp,
                               TRTP_DOT((self->dots_list->head)->data)->rtp_timestamp,
                               TRTP_DOT((self->dots_list->tail)->data)->rtp_timestamp-TRTP_DOT((self->dots_list->head)->data)->rtp_timestamp);
            }
        }
        tsk_thread_sleep(TRTP_STATISTIC_THREAD_CYCLE);
        if(!self->running) {
            break;
        }
    }
    
    TSK_DEBUG_INFO("statistic thread exits");
    return tsk_null;
}


static tsk_object_t *trtp_statistic_ctor (tsk_object_t *_self, va_list *app)
{
    trtp_statistic_t *self = _self;
    if (self)
    {
        self->max_time_range = TRTP_STATISTIC_MAX_TIME_RANGE;  // 30s
        self->curr_timestamp = 0;
        
        if(!(self->dots_list = tsk_list_create())) {
            TSK_DEBUG_ERROR("Failed to create rtp dots list.");
            return tsk_null;
        }
        
        if(!self->thread_handle) {
            self->running = tsk_true;
            int ret = tsk_thread_create(&self->thread_handle, trtp_statistic_thread_func, self);
            if((0 != ret) && (!self->thread_handle)) {
                TSK_DEBUG_ERROR("Failed to create rtp statistic thread");
                return tsk_null;
            }
            ret = tsk_thread_set_priority(self->thread_handle, TSK_THREAD_PRIORITY_LOW);
        }
    }
    return _self;
}

static tsk_object_t *trtp_statistic_dtor (tsk_object_t *_self)
{
    trtp_statistic_t *self = _self;
    if (self)
    {
        self->running = tsk_false;
        tsk_thread_join(&self->thread_handle);
        if(self->dots_list) {
            TSK_OBJECT_SAFE_FREE(self->dots_list);
        }
    }
    return _self;
}

static int trtp_statistic_cmp (const tsk_object_t *_p1, const tsk_object_t *_p2)
{
    const trtp_statistic_t *p1 = _p1;
    const trtp_statistic_t *p2 = _p2;
    if(p1 && p2) {
        //return (int)(p1->seq_num - p2->seq_num);
        return 0;
    } else if(!p1 && !p2) {
        return 0;
    } else {
        return -1;
    }
}

static const tsk_object_def_t trtp_statistic_def_s = {
    sizeof (trtp_statistic_t),
    trtp_statistic_ctor,
    trtp_statistic_dtor,
    trtp_statistic_cmp,
};

const tsk_object_def_t *trtp_statistic_def_t = &trtp_statistic_def_s;

//--------------------------------------
//	statistic extern api
//--------------------------------------
trtp_dot_t *trtp_dot_create(trtp_rtp_packet_t* packet) {
    trtp_dot_t* self = (trtp_dot_t *)tsk_object_new (trtp_dot_def_t);
    if(self) {
        self->seq_num = packet->header->seq_num;
        self->rtp_timestamp = packet->header->timestamp;
        self->recv_timestamp = tsk_gettimeofday_ms();
    }
    
    return self;
}

trtp_statistic_t *trtp_statistic_create() {
    return (trtp_statistic_t *)tsk_object_new (trtp_statistic_def_t);
}
#if 0
int trtp_statistic_start(trtp_statistic_t *self) {
    if(!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    if(self->running) {
        return 0;
    }
    
    if(!self->thread_handle) {
        self->running = tsk_true;
        int ret = tsk_thread_create(&self->thread_handle, trtp_statistic_thread_func, self);
        if((0 != ret) && (!self->thread_handle)) {
            TSK_DEBUG_ERROR("Failed to create rtp statistic thread");
            return tsk_null;
        }
        ret = tsk_thread_set_priority(self->thread_handle, TSK_THREAD_PRIORITY_LOW);
    }
    return 0;
}

int trtp_statistic_stop(trtp_statistic_t *self) {
    if(!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    if(!self->running) {
        return 0;
    }
    self->running = tsk_false;
    tsk_thread_join(&self->thread_handle);
    return 0;
}
#endif
int trtp_statistic_push_dot(trtp_statistic_t* statistic, trtp_dot_t* dot) {
    if(!statistic || !dot) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    tsk_list_lock(statistic->dots_list);
    trtp_dot_t* temp = tsk_object_ref(TSK_OBJECT(dot));
    tsk_list_push_ascending_data(statistic->dots_list, (void **)&temp);
    statistic->curr_timestamp = (dot->rtp_timestamp > statistic->curr_timestamp)?dot->rtp_timestamp:statistic->curr_timestamp;
    tsk_list_unlock(statistic->dots_list);
    
    return 0;
}

int trtp_statistic_get_max_consec_loss(trtp_statistic_t* self) {
    int max_consec_loss = 0;
    tsk_list_item_t* item = tsk_null;
    tsk_list_lock(self->dots_list);
    tsk_list_foreach(item, self->dots_list) {
        if(item->next) {
            int temp = TRTP_DOT(item->next->data)->seq_num - TRTP_DOT(item->data)->seq_num - 1;
            max_consec_loss = (temp > max_consec_loss)?temp:max_consec_loss;
        }
    }
    tsk_list_unlock(self->dots_list);
    return max_consec_loss;
}
