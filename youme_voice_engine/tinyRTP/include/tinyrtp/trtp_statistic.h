//
//  trtp_statistic.h
//  youme_voice_engine
//
//  Created by mac on 2017/5/25.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef trtp_statistic_h
#define trtp_statistic_h

#include "tinydav_config.h"
#include "trtp.h"
#include "trtp_rtp_packet.h"
#include "tsk_list.h"
#include "tsk_thread.h"
#include "tsk_object.h"


TRTP_BEGIN_DECLS

#define TRTP_DOT(self)   ((trtp_dot_t*)(self))
#define TRTP_STATISTIC(self)   ((trtp_statistic_t*)(self))

#define TRTP_STATISTIC_THREAD_CYCLE      (100)    //100ms
#define TRTP_STATISTIC_MAX_TIME_RANGE    (10*1000)   //10s
#define TRTP_STATISTIC_COMPUTE_CYCLE     (50)    //5s

typedef tsk_list_t trtp_dots_list_t;

typedef struct trtp_dot_s {
    TSK_DECLARE_OBJECT;
    
    uint64_t recv_timestamp; //ms
    uint64_t rtp_timestamp; //ms
    int prev_seq_num;
    int seq_num;
} trtp_dot_t;

typedef struct trtp_statistic_s {
    TSK_DECLARE_OBJECT;
    
    int total;
    int loss;
    int max_consec_loss;
    uint64_t max_time_range; //ms
    uint64_t curr_timestamp;

    int min_seq_num;
    int max_seq_num;
    
    trtp_dots_list_t* dots_list;
    tsk_bool_t running;
    tsk_thread_handle_t *thread_handle;
} trtp_statistic_t;

trtp_dot_t *trtp_dot_create(trtp_rtp_packet_t* packet);
trtp_statistic_t *trtp_statistic_create();
//int trtp_statistic_start(trtp_statistic_t *self);
//int trtp_statistic_stop(trtp_statistic_t *self);
int trtp_statistic_push_dot(trtp_statistic_t* statistic, trtp_dot_t* dot);
int trtp_statistic_get_max_consec_loss(trtp_statistic_t* self);

TRTP_END_DECLS

#endif /* trtp_statistic_h */
