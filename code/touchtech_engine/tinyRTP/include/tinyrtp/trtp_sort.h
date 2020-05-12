//
//  trtp_sort.h
//  youme_voice_engine
//
//  Created by mac on 2017/5/25.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef trtp_sort_h
#define trtp_sort_h

#include "tinydav_config.h"
#include "tsk_object.h"
#include "tsk_list.h"
#include "tsk_thread.h"
#include "trtp_statistic.h"


TRTP_BEGIN_DECLS


#define TRTP_SORT(self)   ((trtp_sort_t*)(self))

typedef int (*trtp_sort_callback_f)(tsk_object_t* output, trtp_rtp_packet_t* packet);

typedef struct trtp_sort_s {
    TSK_DECLARE_OBJECT;
    
    int session_id;
    int payload_type;
    
    int last_seq_num;
    tsk_list_t* in_rtp_packets_list;
    
    tsk_bool_t running;
    tsk_thread_handle_t *thread_handle;
    
    tsk_object_t* session;
    trtp_sort_callback_f callback;
    
    //rtp packets statistic
    trtp_statistic_t* statistic;
    
    uint64_t first_timestamp;
    uint64_t last_timestamp;
}
trtp_sort_t;

trtp_sort_t* trtp_sort_create(int session_id, int payload_type, tsk_object_t* session, trtp_sort_callback_f callback);
int trtp_sort_start(trtp_sort_t *self);
int trtp_sort_stop(trtp_sort_t *self);
int trtp_sort_push_rtp_packet(trtp_sort_t *self, trtp_rtp_packet_t *packet);
trtp_sort_t* trtp_sort_select_by_sessionid_payloadtype(tsk_list_t* self, int session_id, int payload_type);
uint64_t trtp_sort_get_duration(trtp_sort_t *self);


TRTP_END_DECLS

#endif /* trtp_sort_h */
