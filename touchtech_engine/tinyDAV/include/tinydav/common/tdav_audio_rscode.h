//
//  tdav_auiod_rscode.h
//  youme_voice_engine
//
//  Created by 游密 on 2017/5/8.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef tdav_auiod_rscode_h
#define tdav_auiod_rscode_h

#include "tinydav_config.h"
#include "tsk_object.h"
#include "tsk_list.h"
#include "tsk_thread.h"
#include "tinyrtp/trtp_manager.h"
//#include "RscodeDefine.h"
#include "tdav_session_audio.h"



TDAV_BEGIN_DECLS

#define TDAV_AUDIO_RSCODE(self)   ((tdav_audio_rscode_t*)(self))


typedef enum RscType_Audio_t{
    RSC_TYPE_ENCODE = 0,
    RSC_TYPE_DECODE = 1,
} RscAudioType;


typedef struct tdav_audio_rscode_s {
    TSK_DECLARE_OBJECT;
    
    int session_id;
    trtp_manager_t *rtp_manager;
    
    RscAudioType rs_type;
   
    int last_seq_num;
    tsk_list_t* in_rtp_packets_list;
    
    tsk_bool_t running;
    tsk_thread_handle_t *thread_handle;
    
    //编码和解码使用的字段,每一个char* 的,编解码公用字段
    int iNPAR;//这个会动态改变，视频的I 和P 会不一样
    char **dataBuffer; //用来缓存需要encode或者decode的数据
    int *dataBufferLen;//每一个缓存里面实际数据的长度。数据的长度
    int *dataBufferRealLen; //缓存的实际长度，为了避免频繁申请内存，如果需要的内存比实际的大才申请。
    void* recodeInstance;
    uint16_t iPacketGroupSeq;
    
    int iCheckCodeLen;//rscode校验码的长度
    int iCodeLen;   //rscode数据的长度，加上冗余的校验码的长度

    //编码字段
    int iCurrentBufferIndex;
    int iNPARChangeCount;
    uint16_t iReSetPacketSeqNum; //需要重新设置包的序号
    
    uint64_t lastAdjustTime;
    
   //是否启动rscode
    tsk_bool_t useRscode;
    
    //C 封装的一个std::set
    void* stdSetInstance;
    
	//用来及时发送语音，减少延迟
	tdav_session_audio_t *audio;
    
    int pop_remove_count;

    uint64_t uBusinessId;

    TSK_DECLARE_SAFEOBJ;
}
tdav_audio_rscode_t;

tdav_audio_rscode_t *tdav_audio_rscode_create(tdav_session_audio_t *sendaudio,int session_id, RscAudioType rs_type, trtp_manager_t *rtp_manager);
int tdav_audio_rscode_start(tdav_audio_rscode_t *self);
int tdav_audio_rscode_stop(tdav_audio_rscode_t *self);
int tdav_audio_rscode_push_rtp_packet(tdav_audio_rscode_t *self, trtp_rtp_packet_t *packet);
void tdav_audio_rscode_reset_NPAR(tdav_audio_rscode_t *self, int iNewNPAR , int iNewCheckCodeLen );

TDAV_END_DECLS

#endif /* tdav_auiod_rscode_h */
