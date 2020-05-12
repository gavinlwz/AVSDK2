//
//  tdav_auiod_rscode.c
//  youme_voice_engine
//
//  Created by 游密 on 2017/5/8.
//  Author : chengjl
//  Copyright © 2017年 Youme. All rights reserved.
//
#include <math.h>
#include "tsk_debug.h"
#include "tsk_memory.h"
#include "trtp_rtp_packet.h"
#include "tinyrtp/trtp_manager.h"
#include "tmedia_utils.h"
#include "tinydav/common/tdav_audio_rscode.h"
#include <YouMeCommon/rscode-1.3/RscodeWrapper2.h>
#include "XConfigCWrapper.hpp"

#define RSCODE_AUDIO_NPAR_DEFAULT  6
#define  RSCODE_AUDIO_CHECK_CODE_LEN_DEFAULT 1

//--------------------------------------
//	rscode object definition begin
//--------------------------------------
static tsk_object_t *tdav_auiod_rscode_ctor (tsk_object_t *_self, va_list *app)
{
    tdav_audio_rscode_t *self = _self;
    if (self)
    {
        tsk_safeobj_init(self);
        int i=0;
		self->iNPAR = Config_GetInt("RSCODE_A_NPAR", RSCODE_AUDIO_NPAR_DEFAULT);
        self->iCheckCodeLen = 0;
        self->iCodeLen = self->iNPAR + self->iCheckCodeLen;
        self->dataBuffer = tsk_malloc((self->iCodeLen )*sizeof(char*));
        self->dataBufferLen = tsk_malloc((self->iCodeLen )*sizeof(int));
        self->dataBufferRealLen = tsk_malloc((self->iCodeLen )*sizeof(int));

        for (i=0; i<self->iCodeLen; i++) {
            self->dataBuffer[i]=NULL;
            self->dataBufferLen[i]=0;
            self->dataBufferRealLen[i]=0;
        }
        self->useRscode = Config_GetInt("RSCODE_A_Enabled",0);
		if (Config_GetInt("rtp_use_tcp", 0))
		{
			//tcp 模式下强制关闭RSCODE
			self->useRscode = 0;
		}
        self->recodeInstance = rscode_create_instance2( self->iCheckCodeLen );
        self->iPacketGroupSeq=1;
        self->iNPARChangeCount = 0;
        self->iReSetPacketSeqNum=0;
        self->iCurrentBufferIndex =-1;
        self->lastAdjustTime = tsk_gettimeofday_ms();
        self->stdSetInstance = NULL;
        if(!(self->in_rtp_packets_list = tsk_list_create())) {
            TSK_DEBUG_ERROR("Failed to create rtp packets list.");
            return tsk_null;
        }
        
     
        self->running = tsk_false;
        self->last_seq_num = -1;
    }
    return _self;
}

static tsk_object_t *tdav_auiod_rscode_dtor (tsk_object_t *_self)
{
    tdav_audio_rscode_t *self = _self;
    if (self)
    {
        if(self->running)
        {
            tdav_audio_rscode_stop(self);
        }
        int i=0;
        TSK_OBJECT_SAFE_FREE(self->in_rtp_packets_list);
        recode_destroy_instance2(self->recodeInstance);
        Destroy_StdSet(self->stdSetInstance);
        for (i=0; i<self->iCodeLen; i++) {
            TSK_FREE(self->dataBuffer[i]);
        }
        TSK_FREE(self->dataBuffer);
        TSK_FREE(self->dataBufferLen);
        TSK_FREE(self->dataBufferRealLen);
        tsk_safeobj_deinit(self);
    }
    return _self;
}

static int tdav_auiod_rscode_cmp (const tsk_object_t *_p1, const tsk_object_t *_p2)
{
    const tdav_audio_rscode_t *p1 = _p1;
    const tdav_audio_rscode_t *p2 = _p2;
    if(p1 && p2) {
        return (int)(p1->session_id - p2->session_id);
    } else if(!p1 && !p2) {
        return 0;
    } else {
        return -1;
    }
}

static const tsk_object_def_t tdav_auiod_rscode_def_s = {
    sizeof (tdav_audio_rscode_t),
    tdav_auiod_rscode_ctor,
    tdav_auiod_rscode_dtor,
    tdav_auiod_rscode_cmp,
};
const tsk_object_def_t *tdav_auiod_rscode_def_t = &tdav_auiod_rscode_def_s;

//--------------------------------------
//	rscode object definition end
//--------------------------------------

static tsk_list_t* tdav_auiod_rscode_process(tdav_audio_rscode_t *self, trtp_rtp_packet_t *packet) {
    
    int offset;

    tsk_list_t* rtp_packets_list = tsk_list_create();
    if(RSC_TYPE_DECODE == self->rs_type) {
        //如果是没有经过rscode 的包，那就直接放进去然后返回
        uint32_t iSerialPerGroup=0;
        if ((packet->header->csrc_count<3) || (packet->header->csrc[2]==0))
        {
            tsk_object_t* temp = tsk_object_ref(TSK_OBJECT(packet));
            tsk_list_push_back_data(rtp_packets_list, (tsk_object_t**)&temp);
            return rtp_packets_list;

        }
        
        //判断分组是否已经完成,暂时这么判断,需要分离出
        uint8_t* pGrounNAP= (uint8_t*)(&packet->header->csrc[2]);
        uint16_t iPacketGroupSeq =pGrounNAP[0] <<8 | pGrounNAP[1];
        uint16_t iNAP =pGrounNAP[2] <<8 | pGrounNAP[3];
        
        uint32_t iCheckCodeLen = 1;
        if( packet->header->csrc_count >= 6 ){
            iCheckCodeLen = packet->header->csrc[5];
        }
        
        if (self->iPacketGroupSeq != iPacketGroupSeq)
        {
            //判断是否需要做decode。如果数据包正常收到了就不要decode了
             int i=0;
            do
            {
                //测试故意丢掉一些
//                self->dataBufferLen[0]=0;
//               self->dataBufferLen[1]=0;
//                self->dataBufferLen[3]=0;
//                memset(self->dataBuffer[0], 0, self->dataBufferRealLen[0]);
//                memset(self->dataBuffer[1], 0, self->dataBufferRealLen[1]);
//                memset(self->dataBuffer[3], 0, self->dataBufferRealLen[3]);
                
                int iNerIndex=0;
                int *pNerasure=NULL;
                int iMaxDataLen = 0;
                int iRecvDataPacket = 0;//接收到的数据包的个数，不是冗余包
                int iLostPacket=0;//所有的丢失包的个数，包括冗余包
               
                for (i=0; i<self->iCodeLen; i++) {
                    if (self->dataBufferLen[i]==0) {
                        iLostPacket++;
                    }
                    else
                    {
                        //判断索引号,是不是数据包
                        if (i < self->iNPAR) {
                            iRecvDataPacket++;
                        }
                        if (iMaxDataLen < self->dataBufferLen[i])
                        {
                            iMaxDataLen = self->dataBufferLen[i];
                        }
                    }
                   
                }
                //TSK_DEBUG_INFO("rscode decode packet:%d  %d %d ",self->iNPAR,iRecvDataPacket,iLostPacket);
                
                if (iRecvDataPacket == self->iNPAR) {
                    //都收到了就不需要了
                    break;
                }
                //这里固定只能丢 < self->iCheckCodeLen 个包
                if (iLostPacket > self->iCheckCodeLen  ) {
                    //丢太多了，解不出来了
                    break;
                }
                //需要解包了
                pNerasure=tsk_malloc(sizeof(int)*iLostPacket);
                for (i=0; i<self->iCodeLen; i++) {
                    if (self->dataBufferLen[i]==0)
                    {
                        //说明丢包了
                        pNerasure[iNerIndex]=i;
                        iNerIndex++;
                        
                        //可能需要重新申请空间
                        if (self->dataBufferRealLen[i] < iMaxDataLen) {
                            self->dataBuffer[i] =tsk_realloc(self->dataBuffer[i], iMaxDataLen);
                            self->dataBufferRealLen[i]  = iMaxDataLen;
                        }
                        self->dataBufferLen[i]=iMaxDataLen;//一定要赋值，否则decode 之后数据是不对的
                    }
                }
                
                rscode_decode_rtp_packet2(self->recodeInstance,(unsigned char**)self->dataBuffer, self->dataBufferLen, iLostPacket, pNerasure,self->iNPAR);
                //把包反序列化然后发出去
                for (i=0; i<iNerIndex; i++) {
                    if (pNerasure[i] >= self->iNPAR) {
                        //冗余包不需要，直接退出
                        break;
                    }
                    
                    int iDataLen=0;
                    trtp_rtp_packet_t* pDecodePacket=NULL;                   
                    memcpy(&iDataLen, self->dataBuffer[pNerasure[i]], sizeof(int));
                    iDataLen = ntohl(iDataLen);
                    pDecodePacket = trtp_rtp_packet_deserialize(self->dataBuffer[pNerasure[i]]+sizeof(int),iDataLen);
                    if (NULL != pDecodePacket) {
                        pDecodePacket->header->my_session_id = packet->header->my_session_id;
                        pDecodePacket->header->session_id=packet->header->session_id;
                        pDecodePacket->header->receive_timestamp_ms = packet->header->receive_timestamp_ms;
                        tsk_list_push_front_data(rtp_packets_list, (tsk_object_t**)&pDecodePacket);
                    }
                    else
                    {
                        TSK_DEBUG_INFO("rscode decode packet failed,chuwentile");
                    }
                }
                TSK_FREE(pNerasure);
            }
            while (0);
            
        
            //当前组序号重新赋值
            self->iPacketGroupSeq=iPacketGroupSeq;
            //重新开始设置一组rscode 包，判断当前的 NPAR 和现在的是否一致
            if(self->iNPAR != iNAP || self->iCheckCodeLen != iCheckCodeLen )
            {
                //重置
                TSK_DEBUG_INFO("rscode reset nap:%d cklen:%d",iNAP, iCheckCodeLen);
                tdav_audio_rscode_reset_NPAR(self,iNAP , iCheckCodeLen );
            }
            
            
            for (i=0; i<self->iCodeLen; i++)
            {
                self->dataBufferLen[i]=0;
                memset(self->dataBuffer[i], 0, self->dataBufferRealLen[i]);
            }
        }

        
        
        //把data 放到缓存中，可能需要使用rsdecode,注意判断序号的合法性
        iSerialPerGroup =packet->header->csrc[3];
        if (iSerialPerGroup >= self->iCodeLen)
        {
            //序号错误
            TSK_DEBUG_INFO("rscode decode packet serial:%d",iSerialPerGroup);
        }
        else
        {
            
            if (self->dataBufferRealLen[iSerialPerGroup] < packet->payload.size)
            {
                self->dataBuffer[iSerialPerGroup]= tsk_realloc (self->dataBuffer[iSerialPerGroup], packet->payload.size);
                self->dataBufferRealLen[iSerialPerGroup]=packet->payload.size;
            }
            memcpy(self->dataBuffer[iSerialPerGroup], packet->payload.data, packet->payload.size);
            self->dataBufferLen[iSerialPerGroup] = packet->payload.size;
        }
        
        
        //只有数据才需要解包
        if (packet->header->csrc[1] == 0)
        {
            trtp_rtp_packet_t* pDecodePacket = trtp_rtp_packet_deserialize((char*)(packet->payload.data)+sizeof(int),packet->payload.size-sizeof(int));
            if (NULL != pDecodePacket) {
                
                pDecodePacket->header->my_session_id = packet->header->my_session_id;
                pDecodePacket->header->session_id=packet->header->session_id;
                pDecodePacket->header->receive_timestamp_ms = packet->header->receive_timestamp_ms;
             
                
                uint16_t first_packet_seq_num = pDecodePacket->header->csrc[2] >> 16;
                uint16_t packet_num = pDecodePacket->header->csrc[2] & 0x0000ffff;

                
                tsk_list_push_back_data(rtp_packets_list, (tsk_object_t**)&pDecodePacket);
                
               }

        }
        //原样放入,测试代码
        
    } else {
        
        ////重新创建一个rtp 包 把包放进去  Encode 部分
        tsk_size_t xsize=0;
        int iSerialLen = 0;
        int iNetOrderSerial =0;
        int iGroupSeqNPAR=0;

        self->iCurrentBufferIndex++;
        
        //重新开始设置一组rscode 包，判断当前的 NPAR 和现在的是否一致
        //根据上行丢包率计算
        int currentLossRate = Config_GetInt("audio_up_lossRate", -1 );
        int iNPAROringia = Config_GetInt("RSCODE_A_NPAR", RSCODE_AUDIO_NPAR_DEFAULT);
        
        uint64_t currentTimeMs = tsk_gettimeofday_ms();
        
        if (0 == self->iCurrentBufferIndex && currentTimeMs - self->lastAdjustTime > 300) {
            self->lastAdjustTime  = currentTimeMs;
            int iCheckCodeLen = 0;
            self->iNPARChangeCount ++;
            if(currentLossRate > 0 || self->iNPARChangeCount > 5) { // 丢包率恢复为0时，需要等5次再调
                self->iNPARChangeCount = 0;
                if(currentLossRate > 0){
                    float checkCodeFactor = 1.0f;
                    if(currentLossRate >= 50){
                        checkCodeFactor = 4.0f;//3.0f;
                    }else if(currentLossRate >= 45){
                        checkCodeFactor = 4.0f;//2.5f;
                    }else if(currentLossRate >= 40){
                        checkCodeFactor = 4.0f;//2.5f;
                    }else if(currentLossRate >= 35){
                        checkCodeFactor = 3.3f;//2.3f;
                    }else if(currentLossRate >= 30){
                        checkCodeFactor = 2.8f;//2.0f;
                    }else if(currentLossRate >= 25){
                        checkCodeFactor = 2.4f;//1.5f;
                    }else if(currentLossRate >= 20){
                        checkCodeFactor = 2.0f;//1.0f;
                    }else if(currentLossRate >= 15){
                        checkCodeFactor = 1.8f;//0.5f;
                    }else if(currentLossRate >= 10){
                        checkCodeFactor = 1.5f;//0.5f;
                    }else if(currentLossRate >= 8){
                        checkCodeFactor = 1.0f;//0.4f;
                    }else if(currentLossRate >= 7){
                        checkCodeFactor = 1.0f;//0.4f;
                    }else if(currentLossRate >= 6){
                        checkCodeFactor = 1.0f;//0.3f;
                    }else if(currentLossRate >= 5){
                        checkCodeFactor = 1.0f;//0.3f;
                    }else if(currentLossRate >= 4){
                        checkCodeFactor = 0.5f;//0.2f;
                    }else {
                        checkCodeFactor = 0.2f;//0.2f;
                    }
                    iCheckCodeLen = iNPAROringia * checkCodeFactor + 0.5;
                }else if(currentLossRate == 0){
                    iNPAROringia = 1;
                    iCheckCodeLen = 0;
                }
                if(currentLossRate != -1){
                    tdav_audio_rscode_reset_NPAR(self,iNPAROringia,  iCheckCodeLen);
                }
                TMEDIA_I_AM_ACTIVE(17, "------ audio iNPAROringia:%d, iCheckCodeLen:%d,  currentLossRate:%d", self->iNPAR, self->iCheckCodeLen, currentLossRate);
            }
        }
//        if(0 == self->iCheckCodeLen) {
//            tsk_object_t* temp = tsk_object_ref(TSK_OBJECT(packet));
//            TSK_DEBUG_INFO("mark iCheckCodeLen is zero, send raw pkt");
//            tsk_list_push_back_data(rtp_packets_list, (tsk_object_t**)&temp);
//            self->iCurrentBufferIndex = -1;
//            return rtp_packets_list;
//        }
      
        trtp_rtp_packet_t*pEncodePacket = trtp_rtp_packet_copy(packet);
        pEncodePacket->header->extension=0;
        
        pEncodePacket->header->seq_num = ++self->iReSetPacketSeqNum;
        pEncodePacket->header->csrc[1] =0;//非冗余包
        
        //重新计算组序号和NPAR
        uint8_t* p2Buffer=(uint8_t*)(&iGroupSeqNPAR);
        p2Buffer[0] = self->iPacketGroupSeq>>8;
        p2Buffer[1] = self->iPacketGroupSeq& 0xFF;
        p2Buffer[2] = self->iNPAR>>8;
        p2Buffer[3] = self->iNPAR&0xFF;
        pEncodePacket->header->csrc[2] = iGroupSeqNPAR; //组序号+NPAR(各占两个字节)
        
        
        pEncodePacket->header->csrc[3]=self->iCurrentBufferIndex;
        
        pEncodePacket->header->csrc[5]=self->iCheckCodeLen;
        pEncodePacket->header->csrc_count=6;
        
        //把packet 序列化到一段缓存
        xsize = trtp_rtp_packet_guess_serialbuff_size (packet);
        if (self->dataBufferRealLen[self->iCurrentBufferIndex] < xsize+sizeof(int))
        {
            self->dataBuffer[self->iCurrentBufferIndex]= tsk_realloc (self->dataBuffer[self->iCurrentBufferIndex], xsize+sizeof(int));
            self->dataBufferRealLen[self->iCurrentBufferIndex]=xsize+sizeof(int);
        }
        //头4字节保存数据长度,所以需要从4字节之后偏移。因为decode 之后没发找到原始长度
        iSerialLen =(int)trtp_rtp_packet_serialize_to (packet,self->dataBuffer[self->iCurrentBufferIndex]+sizeof(int), xsize);
        self->dataBufferLen[self->iCurrentBufferIndex] =iSerialLen+sizeof(int);
        //把长度拷贝到前4个字节
        iNetOrderSerial = htonl(iSerialLen);
        memcpy(self->dataBuffer[self->iCurrentBufferIndex], &iNetOrderSerial, sizeof(int));
        
        //把数据放到payload，放入队列发送
        pEncodePacket->payload.size =iSerialLen+sizeof(int);
        pEncodePacket->payload.data=tsk_malloc(pEncodePacket->payload.size);
        memcpy(pEncodePacket->payload.data,self->dataBuffer[self->iCurrentBufferIndex], pEncodePacket->payload.size);
        
        if (self->uBusinessId != 0) {
            /*  | extension magic num（2byte） |    extension lenth（2byte）          |
                |    data type（2byte）        |    data size（2byte）                |
                |  data(以4字节对齐，数据不是4的整数倍，在data高位第一个4字节前面补0x00)      |
            */
            // BusinessID is put into rtp extension
            // 16bit magic num + 16bit extension lenth + 16bit type lenth + 16bit business id len + business id size
            pEncodePacket->extension.size = RTP_EXTENSION_MAGIC_NUM_LEN_PROFILE+RTP_EXTENSION_LEN_PROFILE + RTP_EXTENSION_TYPE_PROFILE + BUSINESS_ID_LEN_PROFILE + sizeof(uint64_t);
            pEncodePacket->extension.data = (unsigned char *)malloc(pEncodePacket->extension.size);
            memset(pEncodePacket->extension.data, 0, pEncodePacket->extension.size);

            //往rtp head extension中写入魔法数字
            offset = 0;
            uint16_t magic_num_net_order = htons(RTP_EXTENSION_MAGIC_NUM);
            memcpy((unsigned char *)pEncodePacket->extension.data+offset, &magic_num_net_order, RTP_EXTENSION_MAGIC_NUM_LEN_PROFILE);
            offset+=RTP_EXTENSION_MAGIC_NUM_LEN_PROFILE;

            //往rtp head extension中写入扩展内容长度
            uint16_t rtp_extension_len = (RTP_EXTENSION_TYPE_PROFILE + BUSINESS_ID_LEN_PROFILE + sizeof(uint64_t)) / 4;
            uint16_t rtp_extension_len_net_order = htons(rtp_extension_len);
            memcpy((unsigned char *)pEncodePacket->extension.data+offset, &rtp_extension_len_net_order, RTP_EXTENSION_LEN_PROFILE);
            offset+=RTP_EXTENSION_LEN_PROFILE;

            //往rtp head extension中写入内容数据类型
            uint16_t rtp_extension_type_net_order = htons(BUSINESSID);
            memcpy((unsigned char *)pEncodePacket->extension.data+offset, &rtp_extension_type_net_order, RTP_EXTENSION_TYPE_PROFILE);
            offset+=RTP_EXTENSION_TYPE_PROFILE;

            //往rtp head extension中写入内容数据长度
            uint16_t rtp_extension_type_len_net_order = htons(sizeof(uint64_t));
            memcpy((unsigned char *)pEncodePacket->extension.data+offset, &rtp_extension_type_len_net_order, BUSINESS_ID_LEN_PROFILE);
            offset+=BUSINESS_ID_LEN_PROFILE;

            //往rtp head extension中写入内容数据
            uint32_t business_id_net_order_high = htonl((self->uBusinessId >> 32) & 0xffffffff);
            memcpy((unsigned char *)pEncodePacket->extension.data+offset, &business_id_net_order_high, sizeof(uint32_t));
            offset += sizeof(uint32_t);
            uint32_t business_id_net_order_low = htonl(self->uBusinessId & 0xffffffff);
            memcpy((unsigned char *)pEncodePacket->extension.data+offset, &business_id_net_order_low, sizeof(uint32_t));
            
            pEncodePacket->header->extension = 1;
        }

        tsk_list_push_back_data(rtp_packets_list, (tsk_object_t**)&pEncodePacket);
        
        
        //放入原始包,测试代码
      //  tsk_object_ref(packet);
        //tsk_list_push_back_data(rtp_packets_list, (tsk_object_t**)&packet);
        
        
        
        //如果是NPAR 个包，那就做一次encode
        if (self->iCurrentBufferIndex == self->iNPAR-1) {
            //找出一个最大的数据长度
            int iMaxDataLen =self->dataBufferLen[0];
            int i=0;
            for (i=1; i<self->iNPAR; i++) {
                if (self->dataBufferLen[i] > iMaxDataLen) {
                    iMaxDataLen = self->dataBufferLen[i];
                }
            }
            
            // 冗余包为0时，不做rscode处理
            if (self->iCheckCodeLen > 0) {
                //判断每个冗余包是否需要重新申请内存,
                for( i = 0 ; i < self->iCheckCodeLen; i++ ){
                    int index = self->iNPAR + i ;
                    if (self->dataBufferRealLen[ index ] < iMaxDataLen)
                    {
                        self->dataBuffer[index]= tsk_realloc (self->dataBuffer[index], iMaxDataLen);
                        self->dataBufferRealLen[index]=iMaxDataLen;
                        
                    }
                    self->dataBufferLen[index]=iMaxDataLen;
                }
                
                //开始rscode
                rscode_encode_rtp_packet2(self->recodeInstance, (unsigned char**)self->dataBuffer, self->dataBufferLen,self->iNPAR);

                //发送各个冗余包
                for( i = 0 ; i < self->iCheckCodeLen; i++ ){
                    int index = self->iNPAR + i ;
                    trtp_rtp_packet_t*pRscodePacket = trtp_rtp_packet_copy(packet);
                    pRscodePacket->header->extension=0;
                    pRscodePacket->header->seq_num = ++self->iReSetPacketSeqNum;
                    pRscodePacket->header->csrc[1] =1;//冗余包
                    pRscodePacket->header->csrc[2] = iGroupSeqNPAR; //组序号
                    pRscodePacket->header->csrc[3]= index;
                    pRscodePacket->header->csrc[5] = self->iCheckCodeLen;

                    pRscodePacket->header->csrc_count=6;


                    pRscodePacket->payload.size =self->dataBufferLen[index];
                    pRscodePacket->payload.data=tsk_malloc(self->dataBufferLen[index]);
                    memcpy(pRscodePacket->payload.data,self->dataBuffer[index], self->dataBufferLen[index]);

                    tsk_list_push_back_data(rtp_packets_list, (tsk_object_t**)&pRscodePacket);
                }
            }
        
            //重置包序号以及自增组序号
            self->iPacketGroupSeq++;
            self->iCurrentBufferIndex=-1;
        }
        
        
    }
    
    return rtp_packets_list;
}

static tsk_list_item_t* tdav_recode_pop_packet_by_seq_num(tdav_audio_rscode_t *self, int seq_num) {
    tsk_list_item_t* item = tsk_null;
    tsk_list_lock(self->in_rtp_packets_list);
    
    if( tsk_list_count_all(self->in_rtp_packets_list) > 0 ) {
        item = tsk_list_pop_first_item(self->in_rtp_packets_list);
    }
    tsk_list_unlock(self->in_rtp_packets_list);
    return item;
}

static void* TSK_STDCALL tdav_audio_rscode_thread_func(void *arg) {
    tdav_audio_rscode_t *self = (tdav_audio_rscode_t *)arg;
    TSK_DEBUG_INFO("rscode thread enters");

    //self->running = tsk_true;
    while(1) {
        if(!self->running) {
            break;
        }
        uint64_t no_read_count = 0;
        tsk_list_item_t *item = tsk_null;
       if(self->rs_type == RSC_TYPE_ENCODE)
       {
           //发送端直接从 list 读数据
           tsk_list_lock(self->in_rtp_packets_list);
           item = tsk_list_pop_first_item(self->in_rtp_packets_list);
           tsk_list_unlock(self->in_rtp_packets_list);
       }
        else
        {
            if(self->last_seq_num < 0) {
                tsk_list_lock(self->in_rtp_packets_list);
                item = tsk_list_pop_first_item(self->in_rtp_packets_list);
                tsk_list_unlock(self->in_rtp_packets_list);
            } else {
                item = tdav_recode_pop_packet_by_seq_num(self, self->last_seq_num + 1);
            }
            
            if (item != NULL) {
                trtp_rtp_packet_t* tmpPacket =  (trtp_rtp_packet_t*)item->data;
                if (self->last_seq_num > 0 && (abs(self->last_seq_num - tmpPacket->header->seq_num) > 10 || tmpPacket->header->seq_num > 65515))
                {
                    //TSK_DEBUG_INFO("rscode duplicate cache clear: %d %d",tmpPacket->header->seq_num,self->last_seq_num);
                    Clear_StdSet(self->stdSetInstance);
                }else{
                    int iHasPacket = Has_StdSet(self->stdSetInstance, tmpPacket->header->seq_num,1);
                    if (iHasPacket !=0) {
                        //已经存在了就直接丢掉不要了
                        TSK_OBJECT_SAFE_FREE(item);
                        continue;
                    }
                    
                    if (Count_StdSet(self->stdSetInstance) >= 25) {
                        DeleteItem_StdSet(self->stdSetInstance, 15);
                    }
                }
                
                 Push_StdSet(self->stdSetInstance,tmpPacket->header->seq_num);
            }
            
        }
       
        if(item) {
            no_read_count = 0;
            TMEDIA_I_AM_ACTIVE(1000, "rscode audio thread is running. session_id:%d", self->session_id);
            self->last_seq_num = ((trtp_rtp_packet_t*)item->data)->header->seq_num;
            if (self->useRscode) {
                tsk_list_t* rtp_packet_list = tdav_auiod_rscode_process(self, (trtp_rtp_packet_t*)item->data);
                tsk_list_item_t* rtp_packet_list_item = tsk_null;
                tsk_list_foreach(rtp_packet_list_item, rtp_packet_list)
                {

                    if(self->rs_type == RSC_TYPE_ENCODE) 
					{
						trtp_manager_send_rtp_packet(self->rtp_manager, ((trtp_rtp_packet_t*)rtp_packet_list_item->data),tsk_false);
                        }
					else {
						tdav_session_audio_rscode_cb(self->audio, rtp_packet_list_item->data);
                    }
                }
                TSK_OBJECT_SAFE_FREE(rtp_packet_list);
            }
            else
            {
                if(self->rs_type == RSC_TYPE_ENCODE) {
                    //直接放到待发送的rtp 队列
					trtp_manager_send_rtp_packet(self->rtp_manager, ((trtp_rtp_packet_t*)item->data),tsk_false);
               } else {
                    //放入另一个队列，comsume 会来取，开启和没有开启rscode 需要兼容
                   // uint32_t csrc[15]; //[0] session_id  [1] 是否是rscode 冗余包  [2]rscode 包的组id [3]一组Rscode 包中 的序号 0--2*NPAR
                   trtp_rtp_packet_t*packet = (trtp_rtp_packet_t*)item->data;
                    if ((packet->header->csrc_count >=3) && (packet->header->csrc[2] != 0))
                    {
                        //说明是经过了处理的rscode 包
                        if (packet->header->csrc[1] == 0) {
                            //真实的数据包需要接解一下
                            
                            trtp_rtp_packet_t* pDecodePacket = trtp_rtp_packet_deserialize((char*)(packet->payload.data)+sizeof(int),packet->payload.size-sizeof(int));
                            if (NULL != pDecodePacket) {
                                pDecodePacket->header->my_session_id = packet->header->my_session_id;
                                pDecodePacket->header->session_id=packet->header->session_id;
                                pDecodePacket->header->receive_timestamp_ms = packet->header->receive_timestamp_ms;

								tdav_session_audio_rscode_cb(self->audio, pDecodePacket);

                            }
                            
                        }
                        
                    }
                    else
                    {
						tdav_session_audio_rscode_cb(self->audio, item->data);
                    }
                }
            }
        }
        else
        {
            //没有取到包
            //no_read_count ++;
            tsk_thread_sleep(5);
        }
        TSK_OBJECT_SAFE_FREE(item);
    }

    tsk_list_lock(self->in_rtp_packets_list);
    tsk_list_clear_items(self->in_rtp_packets_list);
    tsk_list_unlock(self->in_rtp_packets_list);
    
    //TSK_OBJECT_SAFE_FREE(self->in_rtp_packets_list);
    
    TSK_DEBUG_INFO("rscode thread exits");
    return tsk_null;
}
void tdav_audio_rscode_reset_NPAR(tdav_audio_rscode_t *self, int iNewNPAR, int iNewCheckCodeLen )
{
    if (iNewNPAR == self->iNPAR && iNewCheckCodeLen == self->iCheckCodeLen ) {
        return;
    }
    //先清掉
    int i=0;
    for (i=0; i<self->iCodeLen; i++) {
        TSK_FREE(self->dataBuffer[i]);
    }
    TSK_FREE(self->dataBuffer);
    TSK_FREE(self->dataBufferLen);
    TSK_FREE(self->dataBufferRealLen);
    
    if( iNewCheckCodeLen != self->iCheckCodeLen ){
        recode_destroy_instance2(self->recodeInstance);
        self->recodeInstance = rscode_create_instance2( iNewCheckCodeLen );
    }
    
    //重新申请
    self->iNPAR = iNewNPAR;
    self->iCheckCodeLen = iNewCheckCodeLen;
    self->iCodeLen = self->iNPAR + self->iCheckCodeLen ;
    self->dataBuffer = tsk_malloc((self->iCodeLen)*sizeof(char*));
    self->dataBufferLen = tsk_malloc((self->iCodeLen)*sizeof(int));
    self->dataBufferRealLen = tsk_malloc((self->iCodeLen)*sizeof(int));
    
    for (i=0; i<self->iCodeLen; i++) {
        self->dataBuffer[i]=NULL;
        self->dataBufferLen[i]=0;
        self->dataBufferRealLen[i]=0;
    }
}
//--------------------------------------
//	rscode object extern api
//--------------------------------------

tdav_audio_rscode_t *tdav_audio_rscode_create(tdav_session_audio_t *sendaudio, int session_id, RscAudioType rs_type, trtp_manager_t *rtp_manager) {
    tdav_audio_rscode_t* self;
    self = tsk_object_new (tdav_auiod_rscode_def_t);
    if(self) {
		self->audio = sendaudio;
        self->session_id = session_id;
        self->rtp_manager = rtp_manager;
        self->rs_type = rs_type;
        if (rs_type == RSC_TYPE_DECODE) {
            self->stdSetInstance = Create_StdSet();
        }
    }
    return self;
}

int tdav_audio_rscode_start(tdav_audio_rscode_t *self) {
    if(!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    tsk_safeobj_lock(self);
    
    if(self->running) {
        tsk_safeobj_unlock(self);
        return 0;
    }
    
    if(!self->thread_handle) {
        self->running = tsk_true;
        int ret = tsk_thread_create(&self->thread_handle, tdav_audio_rscode_thread_func, self);
        if((0 != ret) && (!self->thread_handle)) {
            TSK_DEBUG_ERROR("Failed to create rscode thread");
            tsk_safeobj_unlock(self);
            return -2;
        }
        ret = tsk_thread_set_priority(self->thread_handle, TSK_THREAD_PRIORITY_TIME_CRITICAL);
    }

    tsk_safeobj_unlock(self);
    return 0;
}

int tdav_audio_rscode_stop(tdav_audio_rscode_t *self) {
    if(!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    tsk_safeobj_lock(self);
    
    if(!self->running) {
        tsk_safeobj_unlock(self);
        return 0;
    }
    
    self->running = tsk_false;
    tsk_thread_join(&self->thread_handle);
    tsk_safeobj_unlock(self);
    return 0;
}

int tdav_audio_rscode_push_rtp_packet(tdav_audio_rscode_t *self, trtp_rtp_packet_t *packet) {
    
    if(!self || !packet) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    
    tsk_list_lock(self->in_rtp_packets_list);
    tsk_object_ref(TSK_OBJECT(packet));
    if(RSC_TYPE_ENCODE == self->rs_type || packet->header->seq_num < 30)
    {
        //序号翻转时，小的序列号放后边
        tsk_list_push_back_data(self->in_rtp_packets_list, (void **)&packet);
    }else{
        tsk_list_push_ascending_data(self->in_rtp_packets_list, (void **)&packet);
    }
    tsk_list_unlock(self->in_rtp_packets_list);
    return 0;
}


