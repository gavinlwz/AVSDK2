//
//  tdav_rscode.c
//  youme_voice_engine
//
//  Created by 游密 on 2017/5/8.
//  Author : chengjl
//  Copyright © 2017年 Youme. All rights reserved.
//
#include "tsk_time.h"
#include "tsk_debug.h"
#include "tsk_memory.h"
#include "trtp_rtp_packet.h"
#include "tinyrtp/trtp_manager.h"
#include "tmedia_utils.h"
#include "tdav_rscode.h"
#include <YouMeCommon/rscode-1.3/RscodeWrapper2.h>
#include "XConfigCWrapper.hpp"
#include "tinydav/video/tdav_session_video.h"

#define RSCODE_NPAR_DEFAULT  20
#define RSCODE_CHECK_CODE_LEN_DEFAULT 4

//--------------------------------------
//	rscode object definition begin
//--------------------------------------
static tsk_object_t *tdav_rscode_ctor (tsk_object_t *_self, va_list *app)
{
    tdav_rscode_t *self = _self;
    if (self)
    {
        tsk_safeobj_init(self);
        int i=0;
        self->iNPAROringia = Config_GetInt("RSCODE_NPAR", RSCODE_NPAR_DEFAULT);
        self->iNPAR = Config_GetInt("RSCODE_NPAR", RSCODE_NPAR_DEFAULT);
        int ckLen = Config_GetInt( "RSCODE_CK_LEN", RSCODE_CHECK_CODE_LEN_DEFAULT );
		self->iCheckCodeLen = ckLen; //开始时冗余增加一倍,配置为1时，固定下来，为了兼容老版本
        self->iCodeLen = self->iNPAR + self->iCheckCodeLen;
        self->iRecvDataPktCnt = 0;
        self->dataBuffer = tsk_malloc((self->iCodeLen)*sizeof(char*));
        self->dataBufferLen = tsk_malloc((self->iCodeLen)*sizeof(int));
        self->dataBufferRealLen = tsk_malloc((self->iCodeLen)*sizeof(int));
        self->iVideoID =-1;
        for (i=0; i<self->iCodeLen; i++) {
            self->dataBuffer[i]=NULL;
            self->dataBufferLen[i]=0;
            self->dataBufferRealLen[i]=0;
        }
        self->useRscode = Config_GetInt("RSCODE_Enabled",1);
		if (Config_GetInt("rtp_use_tcp", 0))
		{
			//tcp 模式下强制关闭RSCODE
			self->useRscode = 0;
		}
        self->recodeInstance = rscode_create_instance2( self->iCheckCodeLen );
        self->iPacketGroupSeq=1;
        self->iReSetPacketSeqNum=0;
        self->iCurrentBufferIndex =-1;
        self->stdSetInstance = NULL;
        self->iNPARChangeCount = 0;
        self->iMaxListCount = Config_GetInt("RS_LIST", 8);
        self->iGroupRecvCount = 0;
        self->iLastPopGroup = 0;
        self->rscodeBreak = tsk_false;
        self->lastAdjustTime = tsk_gettimeofday_ms();
        if(!(self->in_rtp_packets_list = tsk_list_create())) {
            TSK_DEBUG_ERROR("Failed to create rtp packets list.");
            return tsk_null;
        }
        
        if(!(self->out_rtp_packets_list = tsk_list_create())) {
            TSK_OBJECT_SAFE_FREE(self->in_rtp_packets_list);
            TSK_DEBUG_ERROR("Failed to create rtp packets list.");
            return tsk_null;
        }
        self->running = tsk_false;
        self->last_seq_num = -1;
    }
    return _self;
}

static tsk_object_t *tdav_rscode_dtor (tsk_object_t *_self)
{
    tdav_rscode_t *self = _self;
    if (self)
    {
        if(self->running)
        {
            tdav_rscode_stop(self);
        }
        int i=0;
        TSK_OBJECT_SAFE_FREE(self->in_rtp_packets_list);
        TSK_OBJECT_SAFE_FREE(self->out_rtp_packets_list);
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

static int tdav_rscode_cmp (const tsk_object_t *_p1, const tsk_object_t *_p2)
{
    const tdav_rscode_t *p1 = _p1;
    const tdav_rscode_t *p2 = _p2;
    if(p1 && p2) {
        return (int)(p1->session_id - p2->session_id);
    } else if(!p1 && !p2) {
        return 0;
    } else {
        return -1;
    }
}

static const tsk_object_def_t tdav_rscode_def_s = {
    sizeof (tdav_rscode_t),
    tdav_rscode_ctor,
    tdav_rscode_dtor,
    tdav_rscode_cmp,
};
const tsk_object_def_t *tdav_rscode_def_t = &tdav_rscode_def_s;

//--------------------------------------
//	rscode object definition end
//--------------------------------------

static tsk_list_t* tdav_rscode_process(tdav_rscode_t *self, trtp_rtp_packet_t *packet) {

    tsk_list_t* rtp_packets_list = tsk_list_create();
    int offset = 0;
    
    if(RSC_TYPE_DECODE == self->rs_type) {
        //如果是没有经过rscode 的包，那就直接放进去然后返回
        if ((packet->header->csrc_count < 6) || (packet->header->csrc[2] == 0))
        {
            tsk_object_t* temp = tsk_object_ref(TSK_OBJECT(packet));
            tsk_list_push_back_data(rtp_packets_list, (tsk_object_t**)&temp);
            return rtp_packets_list;
        }
        
        //判断分组是否已经完成,暂时这么判断, 需要分离出分组号和组内序列号
        uint8_t* pGrounNAP = (uint8_t*)(&packet->header->csrc[2]);
        uint16_t iPacketGroupSeq = pGrounNAP[0] << 8 | pGrounNAP[1];
        uint16_t iNAP = pGrounNAP[2] << 8 | pGrounNAP[3];
        
        uint32_t iCheckCodeLen = 1;
        if ( packet->header->csrc_count >= 6) {
            iCheckCodeLen = packet->header->csrc[5];
        }

        // 这里优化下，当分组内非冗余包全部到达时，可以直接处理下一个分组，减小延时
        // 新的分组包到达的时候，对旧分组做下检查
        if (self->iPacketGroupSeq != iPacketGroupSeq)
        {
            //TSK_DEBUG_INFO("rscode drop current group:%d %d",(int)self->iPacketGroupSeq,(int)iPacketGroupSeq);
            //判断是否需要做decode。如果数据包正常收到了就不要decode了
            int i=0;
            do
            {
                //测试故意丢掉一些
//               self->dataBufferLen[0]=0;
//               self->dataBufferLen[1]=0;
//                self->dataBufferLen[2]=0;
//                memset(self->dataBuffer[0], 0, self->dataBufferRealLen[0]);
//                memset(self->dataBuffer[1], 0, self->dataBufferRealLen[1]);
//                memset(self->dataBuffer[2], 0, self->dataBufferRealLen[2]);

                int iNerIndex=0;
                int *pNerasure=NULL;
                int iMaxDataLen = 0;
                int iRecvDataPacket = 0;// 当前组接收到的数据包的个数，不是冗余包
                int iLostPacket=0;// 当前组丢失包的个数，包括冗余包
               
                for (i=0; i< self->iCodeLen; i++) {
                    if (self->dataBufferLen[i] == 0) {
                        iLostPacket++;
                    } else {
                        if (i < self->iNPAR) {
                            iRecvDataPacket++;
                        }

                        // 检查缓存中最大的包长度， 用于后面buffer申请
                        if (iMaxDataLen < self->dataBufferLen[i]) {
                            iMaxDataLen = self->dataBufferLen[i];
                        }
                    }
                }
                
//                printf("\n");
                //TSK_DEBUG_INFO("rscode decode packet:%d  %d %d ",self->iNPAR,iRecvDataPacket,iLostPacket);
                
                if (iRecvDataPacket == self->iNPAR) {
                    //都收到了就不需要做decode
                    //TSK_DEBUG_INFO("rscode drop ok:%d %d %d %d",iLostPacket,self->iCheckCodeLen,(int)self->iPacketGroupSeq,iPacketGroupSeq);
                    break;
                }

                // TSK_DEBUG_INFO("rscode decode npar:%d checkcode:%d, lost:%d", self->iNPAR, self->iCheckCodeLen, iLostPacket);

                if (iLostPacket > self->iCheckCodeLen  ) {
                    //丢太多了，解不出来了
                    self->rscodeBreak = tsk_true;
                    TSK_DEBUG_INFO("rscode drop:%d %d %d %d",iLostPacket,self->iCheckCodeLen,(int)self->iPacketGroupSeq,iPacketGroupSeq);
                    //TSK_DEBUG_INFO("rscode drop:%d %d",iLostPacket,self->iCheckCodeLen);
                    break;
                }

                //需要解包了
                pNerasure = tsk_malloc(sizeof(int)*iLostPacket);
                for (i=0; i<self->iCodeLen; i++) {
                    if (self->dataBufferLen[i]==0) {
                        //说明丢包了
                        pNerasure[iNerIndex]=i;
                        iNerIndex++;
                        
                        //可能需要重新申请空间
                        if (self->dataBufferRealLen[i] < iMaxDataLen) {
                            self->dataBuffer[i] = tsk_realloc(self->dataBuffer[i], iMaxDataLen);
                            self->dataBufferRealLen[i] = iMaxDataLen;
                        }
                        self->dataBufferLen[i]=iMaxDataLen; //一定要赋值，否则decode 之后数据是不对的
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
                    pDecodePacket = trtp_rtp_packet_deserialize(self->dataBuffer[pNerasure[i]]+sizeof(int), iDataLen);
                    if (NULL != pDecodePacket) {
                        pDecodePacket->header->my_session_id = packet->header->my_session_id;
                        pDecodePacket->header->session_id=packet->header->session_id;
                        pDecodePacket->header->receive_timestamp_ms = packet->header->receive_timestamp_ms;
                        pDecodePacket->header->packetGroup = self->iPacketGroupSeq;
                        //TSK_DEBUG_INFO("rscode drop decode packet success,%d", pDecodePacket->header->timestamp);
                        tsk_list_push_ascending_data(rtp_packets_list, (tsk_object_t**)&pDecodePacket);
                    }
                    else
                    {
                        static int decodefailcount = 0;
                        decodefailcount++;
                        TSK_DEBUG_INFO("rscode decode packet failed,%d", decodefailcount);
                        //rsdecode 失败，退出
                        break;
                    }
                }
                TSK_FREE(pNerasure);
            }
            while (0);
            
            //当前组序号重新赋值
            self->iPacketGroupSeq = iPacketGroupSeq;
            self->iRecvDataPktCnt = 0; 

            //重新开始设置一组rscode 包，判断当前的 NPAR 和现在的是否一致
            if(self->iNPAR != iNAP || self->iCheckCodeLen != iCheckCodeLen )
            {
                //重置
                tdav_rscode_reset_NPAR(self,iNAP, iCheckCodeLen );
            }
            
            for (i=0; i<self->iCodeLen; i++)
            {
                self->dataBufferLen[i]=0;
                memset(self->dataBuffer[i], 0, self->dataBufferRealLen[i]);
            }
        } else {
            // 检查当前分组内数据包是否全部到达
            if (self->iRecvDataPktCnt >= self->iNPAR) {
                // TSK_DEBUG_INFO("rscode decode already get all pkt, ts:%d, cnt:%d", self->iPacketGroupSeq, self->iRecvDataPktCnt);
                return rtp_packets_list;
            }
        }
          
        //把data 放到缓存中，可能需要使用rsdecode,注意判断序号的合法性
        uint32_t iSerialPerGroup =packet->header->csrc[3];
        if (iSerialPerGroup >= self->iCodeLen )
        {
            //序号错误
            TSK_DEBUG_ERROR("rscode decode packet serial:%d",iSerialPerGroup);
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
        
        //只有数据包才需要解包
        if (packet->header->csrc[1] == 0)
        {
            self->iRecvDataPktCnt++;
            trtp_rtp_packet_t* pDecodePacket = trtp_rtp_packet_deserialize((char*)(packet->payload.data)+sizeof(int),packet->payload.size-sizeof(int));
            if (NULL != pDecodePacket) {
                
                static uint16_t s_lastSerial = 1;
                if ((pDecodePacket->header->seq_num-s_lastSerial) !=1)
                {
                   // TSK_DEBUG_INFO("rscode decode recv serial:%d  last:%d",(int)pDecodePacket->header->seq_num,(int)s_lastSerial);
                }
                
                s_lastSerial = pDecodePacket->header->seq_num;
                pDecodePacket->header->my_session_id = packet->header->my_session_id;
                pDecodePacket->header->session_id=packet->header->session_id;
                pDecodePacket->header->receive_timestamp_ms = packet->header->receive_timestamp_ms;
                pDecodePacket->header->packetGroup = self->iPacketGroupSeq;
             
                
                //uint16_t first_packet_seq_num = pDecodePacket->header->csrc[2] >> 16;
                //uint16_t packet_num = pDecodePacket->header->csrc[2] & 0x0000ffff;

                
                tsk_list_push_back_data(rtp_packets_list, (tsk_object_t**)&pDecodePacket);
               }
        }
        //原样放入,测试代码
        
    } else {
        
        //测试丢掉P 帧
        /*if (packet->header->ssrc >=2)
        {
            if (packet->header->csrc[1] == 0) {
                return rtp_packets_list;
                
            }
        }*/
        
        
        //测试 动态码率
      /*  static int s_inttest = 1;
        static int iaaa =40;
        s_inttest++;
        
        if (s_inttest >= 1000) {
            s_inttest=0;
            if (iaaa == 40) {
                iaaa=100;
            }
            else
            {
                iaaa=40;
            }
            Config_SetInt("video_bitrate_level", iaaa);
            TSK_DEBUG_INFO("码率:%d",iaaa);
        }
      */
        ////重新创建一个rtp 包 把包放进去  Encode 部分
        tsk_size_t xsize=0;
        int iSerialLen = 0;
        int iNetOrderSerial =0;
        int iGroupSeqNPAR=0;
        trtp_rtp_packet_t*pEncodePacket = trtp_rtp_packet_copy(packet);
        pEncodePacket->header->extension=0;
        
        self->iCurrentBufferIndex++;
#if 0
        if (0 == self->iCurrentBufferIndex) {
            //重新开始设置一组rscode 包，判断当前的 NPAR 和现在的是否一致
            //根据上行丢包率计算
            int currentLossRate = Config_GetInt("video_lossRate", -1 );
            int bitRate = tmedia_get_video_current_bitrate();
            
            int iNPAROringia = Config_GetInt("RSCODE_NPAR", RSCODE_NPAR_DEFAULT); //低于1%时的默认值
            int iCheckCodeLen= Config_GetInt("RSCODE_CK_LEN", RSCODE_CHECK_CODE_LEN_DEFAULT );  //低于1%时的默认值
            
            self->iNPARChangeCount ++;
            if(currentLossRate > 1 || self->iNPARChangeCount >5){
                self->iNPARChangeCount = 0;
                int sliceSize = MAX_H264_STREAM_SLICE;
                if(currentLossRate > 0 && iCheckCodeLen >1){ //配置为1时，固定下来，为了兼容老版本
                    //if(currentLossRate!=-1) TSK_DEBUG_INFO("rscode currentLossRate:%d",currentLossRate);
                    int groupPktCount = iNPAROringia;
                    if(currentLossRate>=3){
                        if(bitRate <= 800){
                            sliceSize = MAX_H264_STREAM_SLICE/1.2;
                        }else if(currentLossRate>10 && bitRate <= 1360){
                            sliceSize = MAX_H264_STREAM_SLICE/1.5;
                        }
                    }
                    if(currentLossRate > 28){
                        iNPAROringia  = 20;
                        iCheckCodeLen = 6;
                    }else if(currentLossRate > 20){
                        iNPAROringia  = groupPktCount;
                        iCheckCodeLen = groupPktCount * 0.6;
                    }else if(currentLossRate > 17){
                        iNPAROringia  = groupPktCount ;
                        iCheckCodeLen = groupPktCount * 0.6;
                    }else if(currentLossRate > 10){
                        iNPAROringia  = groupPktCount;
                        iCheckCodeLen = groupPktCount * 0.5;
                    }else if(currentLossRate > 6){
                        iNPAROringia  = groupPktCount;
                        iCheckCodeLen = groupPktCount * 0.4;
                    }else if(currentLossRate>3){
                        iNPAROringia  = groupPktCount;
                        iCheckCodeLen = groupPktCount * 0.3;
                    }else if(currentLossRate>1){
                        iNPAROringia  = groupPktCount;
                        iCheckCodeLen = groupPktCount * 0.2;
                    }
                    
                    TSK_DEBUG_INFO("------ iNPAROringia:%d, iCheckCodeLen:%d,  currentLossRate:%d", iNPAROringia, iCheckCodeLen, currentLossRate);
                    
                }else{
                    // 分组个数决定了丢包恢复时的最大延迟，低码率时可以减小分组大小
                    if(bitRate <= 960){
                        sliceSize = MAX_H264_STREAM_SLICE / 1.1;
                    }
                    
                    //iNPAROringia  = iNPAROringia;//groupPktCount + 6;
                    //iCheckCodeLen = iCheckCodeLen;//groupPktCount + 10;
                    
                    if(bitRate >= 1800){
                        sliceSize = MAX_H264_STREAM_SLICE / 0.95;
                    }
                }
                //分包大小设置
                Config_SetInt("H264_SLICE_SIZE", sliceSize);
                //              TSK_DEBUG_INFO("rscode iNPAROringia:  %d",iNPAROringia);
                //              TSK_DEBUG_INFO("rscode iCheckCodeLen: %d",iCheckCodeLen);
                if(currentLossRate!=-1){
                    if (packet->header->csrc_count >=2)
                    {
                        if (packet->header->csrc[1] == 1) {
                            //当前是I帧
                            int count = Config_GetInt( "RSCODE_I", 0 );
                            tdav_rscode_reset_NPAR(self,iNPAROringia, iCheckCodeLen + count);
                        }
                        else
                        {
                            //当前是P帧
                            tdav_rscode_reset_NPAR(self, iNPAROringia, iCheckCodeLen);
                        }
                    }
                    else
                    {
                        tdav_rscode_reset_NPAR(self,iNPAROringia,  iCheckCodeLen);
                    }
                }
            }
            //if(self->iNPAROringia != Config_GetInt("RSCODE_NPAR", RSCODE_NPAR_DEFAULT))
            //{
            //    self->iNPAROringia = Config_GetInt("RSCODE_NPAR", RSCODE_NPAR_DEFAULT);
            //}
            
            
        }
#else
        uint64_t currentTimeMs = tsk_gettimeofday_ms();

        if (0 == self->iCurrentBufferIndex 
            && (currentTimeMs - self->lastAdjustTime > 1000)) {

            self->lastAdjustTime  = currentTimeMs;
            //重新开始设置一组rscode 包，判断当前的 NPAR 和现在的是否一致
            //根据上行丢包率计算
            int  halfLossrate = Config_GetInt("video_lossRate", -1 );
            int  fullLossrate = Config_GetInt("video_up_lossRate", -1 );
            int  maxLossLimit = Config_GetInt("MAX_LOSS", 50);
            int  pktGroupAdd = Config_GetInt("RS_ADD", 0); //rscode 冗余控制
            
            int currentLossRate = (halfLossrate > fullLossrate) ? halfLossrate : fullLossrate;

            // 获取 rscode 默认冗余配置
            int iNPAROringia = Config_GetInt("RSCODE_NPAR", RSCODE_NPAR_DEFAULT);
            int iCheckCodeLen= Config_GetInt("RSCODE_CK_LEN", RSCODE_CHECK_CODE_LEN_DEFAULT );
            
            int videoId = self->iVideoID;
            int current_bitrate = 0, preset_bitrate = 0;
            int bitRate_level = 0;
            unsigned width = 0, height = 0;
            if (TRTP_VIDEO_STREAM_TYPE_MAIN == videoId) {
                // main stream
                tmedia_defaults_get_video_size(&width, &height);
                bitRate_level = Config_GetInt("video_bitrate_level", 100);
                preset_bitrate = tmedia_defaults_get_video_codec_bitrate();
            }else if(TRTP_VIDEO_STREAM_TYPE_MINOR == videoId){
                // minor stream
                tmedia_defaults_get_video_size_child(&width, &height);
                bitRate_level = Config_GetInt("video_bitrate_level_second", 100);
                preset_bitrate = tmedia_defaults_get_video_codec_bitrate_child();
            } else {
                tmedia_defaults_get_video_size_share(&width, &height);
                bitRate_level = Config_GetInt("video_bitrate_share", 100);
                preset_bitrate = tmedia_defaults_get_video_codec_bitrate_share();
            }

            int default_bitrate = bitRate_level * tmedia_defaults_size_to_bitrate(width, height)/100;
            if (!preset_bitrate || preset_bitrate > default_bitrate * 2) {
                current_bitrate = default_bitrate;
            } else {
                current_bitrate = preset_bitrate;
            } 

            int groupPktCount = iNPAROringia;
            int tempAdjustPktCount = 5;
            float checkCodeFactor = 1.0f;
            // adjust rscode group count and checkCodeFactor
            if (current_bitrate >= 2000 /* 1920*1080 */) {
                groupPktCount = 18 + pktGroupAdd;
            }else if (current_bitrate >= 1600 /* 1280*720 */) {
                groupPktCount = 18 + pktGroupAdd;
            }else if (current_bitrate >= 1200 /* 800*600 */) {
                groupPktCount = 18 + pktGroupAdd;
            }else if (current_bitrate >= 800 /* 640*480 */) {
                groupPktCount = 16;
            }else if (current_bitrate >= 500 /* 480*320 */) {
                groupPktCount = 16;
                checkCodeFactor = 1.2f;
            }else if (current_bitrate >= 360 /* 320*240 */) {
                groupPktCount = 10;
                checkCodeFactor = 1.3f;
            }else {
                groupPktCount = 8;
                checkCodeFactor = 1.5f;
            }
            
            if (videoId == 0 && currentLossRate <= 10) {
                // aeron test
                groupPktCount += tempAdjustPktCount;
            }

            self->iNPARChangeCount++;
            if (currentLossRate > 1 || self->iNPARChangeCount > 5) {
                self->iNPARChangeCount = 0;
                int maxSliceSize = Config_GetInt("MAX_MTU_SIZE", DEF_MAX_H264_STREAM_SLICE);
                int checkcodeLevel = Config_GetInt("MAX_CHECK_CODE_LEVEL", 100);

                int sliceSize = maxSliceSize;//MAX_H264_STREAM_SLICE;
                if(currentLossRate > 0) { //配置为1时，固定下来，为了兼容老版本
                    //if(currentLossRate!=-1) TSK_DEBUG_INFO("rscode currentLossRate:%d",currentLossRate);
                
                    // adjust video pkt mtu size
//                    if (currentLossRate >= 5) {
                        if (current_bitrate <= 360) {
                            sliceSize = maxSliceSize / 4.0;
                        } else if (current_bitrate <= 500) {
                            sliceSize = maxSliceSize / 3;
                        } else if (current_bitrate <= 800) {
                            sliceSize = maxSliceSize / 1.5;
                        } else if (current_bitrate <= 1200) {
                            sliceSize = maxSliceSize / 1.1;
                        } else {
                            // do nothing
                        }
//                    }

                    if(videoId == 0 || videoId == 2) {
                        // main/share video stream
                        if (currentLossRate >= maxLossLimit) {
                            iCheckCodeLen = 2;
                            self->ilastHighLossTime = currentTimeMs;
                        }else if(currentLossRate >= 25){
                            iCheckCodeLen = checkCodeFactor * TSK_MAX(groupPktCount * 1.0, 1);
                        }else if(currentLossRate >= 20){
                            iCheckCodeLen = checkCodeFactor * TSK_MAX(groupPktCount * 0.9, 1);
                        }else if(currentLossRate >= 15){
                            iCheckCodeLen = checkCodeFactor * TSK_MAX(groupPktCount * 0.8, 1);
                        }else if(currentLossRate >= 8){
                            iCheckCodeLen = checkCodeFactor * TSK_MAX(groupPktCount * 0.6, 1);
                        }else if(currentLossRate >= 2){
                            iCheckCodeLen = checkCodeFactor * TSK_MAX(groupPktCount * 0.3, 1);
                        }else {
                            iCheckCodeLen = checkCodeFactor * TSK_MAX(groupPktCount * 0.1, 1);
                        }
                        
                        // 上一次高丢包的接下来两秒内不提高冗余
                        if ((int64_t)(currentTimeMs - self->ilastHighLossTime) <= 2500 ) {
                            iCheckCodeLen = 1;
                        }
                    }else if (videoId == 1) {
                        // minor video stream
                        if(currentLossRate >= maxLossLimit){
                            iCheckCodeLen = checkCodeFactor * TSK_MAX(groupPktCount * 0.4, 1);
                        }else if(currentLossRate >= 25){
                            iCheckCodeLen = checkCodeFactor * TSK_MAX(groupPktCount * 1.6, 1);
                        }else if(currentLossRate >= 20){
                            iCheckCodeLen = checkCodeFactor * TSK_MAX(groupPktCount * 1.3, 1);
                        }else if(currentLossRate >= 15){
                            iCheckCodeLen = checkCodeFactor * TSK_MAX(groupPktCount * 1.0, 1);
                        }else if(currentLossRate >= 10){
                            iCheckCodeLen = checkCodeFactor * TSK_MAX(groupPktCount * 0.8, 1);
                        }else if(currentLossRate >= 5){
                            iCheckCodeLen = checkCodeFactor * TSK_MAX(groupPktCount * 0.5, 1);
                        } else {
                            iCheckCodeLen = checkCodeFactor * TSK_MAX(groupPktCount * 0.2, 1);
                        }
                    }

                    iCheckCodeLen = iCheckCodeLen * checkcodeLevel / 100;
                    if (!iCheckCodeLen) {
                        iCheckCodeLen = 1;
                    }
                    iNPAROringia = groupPktCount;
                    
                    TSK_DEBUG_INFO("------[%s] iNPAROringia:%d, iCheckCodeLen:%d, level:%d, currentLossRate:%d, bitrate:%d"
                                   , videoId == 0? "main": videoId == 1 ?"minor" :"share", iNPAROringia, iCheckCodeLen, checkcodeLevel, currentLossRate, current_bitrate);
                   
                }else{
                    // 分组个数决定了丢包恢复时的最大延迟，低码率时可以减小分组大小
                    /*
                    if (current_bitrate <= 320) {
                        sliceSize = maxSliceSize / 8;
                    } else if (current_bitrate <= 500) {
                        sliceSize = maxSliceSize / 4;
                    } else if (current_bitrate <= 960) {
                        sliceSize = maxSliceSize / 1.2;
                    } else if (current_bitrate <= 1360) {
                        sliceSize = maxSliceSize / 1.5;
                    } else if(current_bitrate >= 1800){
                        sliceSize = maxSliceSize / 0.95;
                    } 
                    */
                    sliceSize = maxSliceSize;
                    iNPAROringia = groupPktCount;
                    iCheckCodeLen = TSK_MAX(groupPktCount * 0.2, 1);

                    TMEDIA_I_AM_ACTIVE(100, "normal: ------[%s] iNPAROringia:%d, iCheckCodeLen:%d, currentLossRate:%d, bitrate:%d"
                                   , videoId == 0? "main": videoId == 1 ?"minor" :"share", iNPAROringia, iCheckCodeLen, currentLossRate, current_bitrate);

                }

                //分包大小设置
                Config_SetInt("H264_SLICE_SIZE", sliceSize);
//              TSK_DEBUG_INFO("rscode iNPAROringia:  %d",iNPAROringia);
//              TSK_DEBUG_INFO("rscode iCheckCodeLen: %d",iCheckCodeLen);
                if(currentLossRate > 0){
                    if (packet->header->csrc_count >=2)
                    {
                        if (packet->header->csrc[1] == 1) {
                            //当前是I帧
                            int count = Config_GetInt( "RSCODE_I", 2 );
                            tdav_rscode_reset_NPAR(self,iNPAROringia, iCheckCodeLen + count);
                        }
                        else
                        {
                            //当前是P帧
							tdav_rscode_reset_NPAR(self, iNPAROringia, iCheckCodeLen);
                        }
                    }
                    else
                    {
                        tdav_rscode_reset_NPAR(self,iNPAROringia,  iCheckCodeLen);
                    }
                }
            }
            
            //if(self->iNPAROringia != Config_GetInt("RSCODE_NPAR", RSCODE_NPAR_DEFAULT))
            //{
            //    self->iNPAROringia = Config_GetInt("RSCODE_NPAR", RSCODE_NPAR_DEFAULT);
            //}
            
            
        }
#endif
        
        // rscode csrc count is 6
        // csrc[0]: sessionId
        // csrc[1]: 冗余包 flag
        // csrc[2]: 组序号 + NPAR
        // csrc[3]: 该包在当前分组中的序号
        // csrc[4]: video_id
        // csrc[5]: checkcodelen
        pEncodePacket->header->seq_num = ++self->iReSetPacketSeqNum;
        pEncodePacket->header->csrc[1] = 0;//非冗余包
        
        //重新计算组序号和NPAR
        uint8_t* p2Buffer=(uint8_t*)(&iGroupSeqNPAR);
        p2Buffer[0] = self->iPacketGroupSeq>>8;
        p2Buffer[1] = self->iPacketGroupSeq& 0xFF;
        p2Buffer[2] = self->iNPAR>>8;
        p2Buffer[3] = self->iNPAR&0xFF;
        pEncodePacket->header->csrc[2] = iGroupSeqNPAR; //组序号+NPAR(各占两个字节)
        
        pEncodePacket->header->csrc[3]=self->iCurrentBufferIndex;
        pEncodePacket->header->csrc[5] = self->iCheckCodeLen;

        // 带上videoid ，给 服务器用
        pEncodePacket->header->csrc[4] = 0;
        if (packet->header->csrc_count >= 4) {
            pEncodePacket->header->csrc[4] = packet->header->csrc[3];
        }
        pEncodePacket->header->video_id=packet->header->csrc[3];
        pEncodePacket->header->frameType =packet->header->csrc[1];
        pEncodePacket->header->frameSerial =packet->header->csrc[2] >> 16;
        pEncodePacket->header->frameCount =packet->header->csrc[2] & 0x0000ffff;
        
        if (pEncodePacket->header->csrc_count < 6) {
            pEncodePacket->header->csrc_count = 6;
        }

        if (self->uBusinessId != 0) {
            /*  | extension magic num（2byte） |    extension lenth（2byte）          |
                |    data type（2byte）        |    data size（2byte）                |
                |  data(以4字节对齐，数据不是4的整数倍，在data高位第一个4字节前面补0x00)      |
            */
            // BusinessID is put into rtp extension
            // 16bit magic num + 16bit extension lenth + 16bit type lenth + 16bit business id len + business id size
            packet->extension.size = RTP_EXTENSION_MAGIC_NUM_LEN_PROFILE+RTP_EXTENSION_LEN_PROFILE + RTP_EXTENSION_TYPE_PROFILE + BUSINESS_ID_LEN_PROFILE + sizeof(uint64_t);
            packet->extension.data = (unsigned char *)malloc(packet->extension.size);
            memset(packet->extension.data, 0, packet->extension.size);

            //往rtp head extension中写入魔法数字
            offset = 0;
            uint16_t magic_num_net_order = htons(RTP_EXTENSION_MAGIC_NUM);
            memcpy((unsigned char *)packet->extension.data+offset, &magic_num_net_order, RTP_EXTENSION_MAGIC_NUM_LEN_PROFILE);
            offset+=RTP_EXTENSION_MAGIC_NUM_LEN_PROFILE;

            //往rtp head extension中写入扩展内容长度
            uint16_t rtp_extension_len = (RTP_EXTENSION_TYPE_PROFILE + BUSINESS_ID_LEN_PROFILE + sizeof(uint64_t)) / 4;
            uint16_t rtp_extension_len_net_order = htons(rtp_extension_len);
            memcpy((unsigned char *)packet->extension.data+offset, &rtp_extension_len_net_order, RTP_EXTENSION_LEN_PROFILE);
            offset+=RTP_EXTENSION_LEN_PROFILE;

            //往rtp head extension中写入内容数据类型
            uint16_t rtp_extension_type_net_order = htons(BUSINESSID);
            memcpy((unsigned char *)packet->extension.data+offset, &rtp_extension_type_net_order, RTP_EXTENSION_TYPE_PROFILE);
            offset+=RTP_EXTENSION_TYPE_PROFILE;

            //往rtp head extension中写入内容数据长度
            uint16_t rtp_extension_type_len_net_order = htons(sizeof(uint64_t));
            memcpy((unsigned char *)packet->extension.data+offset, &rtp_extension_type_len_net_order, BUSINESS_ID_LEN_PROFILE);
            offset+=BUSINESS_ID_LEN_PROFILE;

            //往rtp head extension中写入内容数据
            uint32_t business_id_net_order_high = htonl((self->uBusinessId >> 32) & 0xffffffff);
            memcpy((unsigned char *)packet->extension.data+offset, &business_id_net_order_high, sizeof(uint32_t));
            offset += sizeof(uint32_t);
            uint32_t business_id_net_order_low = htonl(self->uBusinessId & 0xffffffff);
            memcpy((unsigned char *)packet->extension.data+offset, &business_id_net_order_low, sizeof(uint32_t));
            
            packet->header->extension = 1;
        }
        
        //把packet 序列化到一段缓存
        xsize = trtp_rtp_packet_guess_serialbuff_size (packet);
        if (self->dataBufferRealLen[self->iCurrentBufferIndex] < xsize+sizeof(int))
        {
            self->dataBuffer[self->iCurrentBufferIndex] = tsk_realloc (self->dataBuffer[self->iCurrentBufferIndex], xsize+sizeof(int));
            self->dataBufferRealLen[self->iCurrentBufferIndex] = xsize+sizeof(int);
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
        
        tsk_list_push_back_data(rtp_packets_list, (tsk_object_t**)&pEncodePacket);
        
        //如果是NPAR 个包，那就做一次encode
//        TSK_DEBUG_WARN("*** self->iCurrentBufferInde%d, self->iNPAR-1:%d",self->iCurrentBufferIndex,  self->iNPAR-1);
        if (self->iCurrentBufferIndex == self->iNPAR-1) {
            //找出一个最大的数据长度
            int iMaxDataLen =self->dataBufferLen[0];
            int i=0;
            for (i=1; i<self->iNPAR; i++) {
                if (self->dataBufferLen[i] > iMaxDataLen) {
                    iMaxDataLen = self->dataBufferLen[i];
                }
            }
            
            //判断是否需要重新申请内存,只有一个冗余包，只需要判断最后一个就可以
            for( i = 0 ; i < self->iCheckCodeLen; i++ ){
                int index = self->iNPAR + i ;
                if (self->dataBufferRealLen[index] < iMaxDataLen)
                {
                    self->dataBuffer[index]= tsk_realloc (self->dataBuffer[index], iMaxDataLen);
                    self->dataBufferRealLen[index]=iMaxDataLen;
                    
                }
                self->dataBufferLen[index]=iMaxDataLen;
                
            }
           
            //开始rscode
            rscode_encode_rtp_packet2(self->recodeInstance, (unsigned char**)self->dataBuffer, self->dataBufferLen,self->iNPAR);
            
            //发送1 个冗余包
            for( i = 0 ; i < self->iCheckCodeLen; i++ ){
                int index = self->iNPAR + i ;
                
                trtp_rtp_packet_t*pRscodePacket = trtp_rtp_packet_copy(packet);
                pRscodePacket->header->extension=0;
                pRscodePacket->header->seq_num = ++self->iReSetPacketSeqNum;
                pRscodePacket->header->csrc[1] =1;//冗余包
                pRscodePacket->header->csrc[2] = iGroupSeqNPAR; //组序号
                pRscodePacket->header->csrc[3]= index;
                if (pRscodePacket->header->csrc_count <4) {
                    pRscodePacket->header->csrc_count=4;
                }
                
                //带上videoid ，给 服务器用
                pRscodePacket->header->csrc[4] = 0;
                pRscodePacket->header->csrc[5] = self->iCheckCodeLen;
                if (packet->header->csrc_count >=4) {
                    pRscodePacket->header->csrc[4] = packet->header->csrc[3];
                }
                pRscodePacket->header->video_id=packet->header->csrc[3];
                pRscodePacket->header->frameType =packet->header->csrc[1];
                pRscodePacket->header->frameSerial =packet->header->csrc[2] >> 16;
                pRscodePacket->header->frameCount =packet->header->csrc[2] & 0x0000ffff;
                
                pRscodePacket->header->csrc_count = 6;
                
                pRscodePacket->payload.size =self->dataBufferLen[index];
                pRscodePacket->payload.data=tsk_malloc(self->dataBufferLen[index]);
                memcpy(pRscodePacket->payload.data,self->dataBuffer[index], self->dataBufferLen[index]);
                
                // TSK_DEBUG_INFO("mark rscode data pkt, ts:%llu", pRscodePacket->header->timestampl);
                tsk_list_push_back_data(rtp_packets_list, (tsk_object_t**)&pRscodePacket);
                
            }
            
            //重置包序号以及自增组序号
            self->iPacketGroupSeq++;
            self->iCurrentBufferIndex=-1;
        }
    }
    
    return rtp_packets_list;
}

static int __pred_find_rtp_by_seq(const tsk_list_item_t *item, const void *data)
{
    if(item && item->data && data){
        if(((trtp_rtp_packet_t*)item->data)->header->seq_num < ((trtp_rtp_packet_t*)data)->header->seq_num - 10 ){
            return 0;
        }
    }
    return -1;
}

// 仅用于接收端场景，发送端直接从list中去取即可
static tsk_list_item_t* tdav_rscode_pop_packet_by_seq_num(tdav_rscode_t *self, int seq_num) {
    tsk_list_item_t* item = tsk_null;
    tsk_list_lock(self->in_rtp_packets_list);
    uint64_t pktCount = 0;
    uint16_t tailPacketGroupSeq = 0;
    
    if(self->in_rtp_packets_list && self->in_rtp_packets_list->tail){
        tsk_list_item_t* tailItem = self->in_rtp_packets_list->tail;
        uint32_t csrcCount = ((trtp_rtp_packet_t*)tailItem->data)->header->csrc_count;
        if ((csrcCount >= 6) && (((trtp_rtp_packet_t*)tailItem->data)->header->csrc[2]!= 0)){
           uint8_t* pGrounNAP = (uint8_t*)(&((trtp_rtp_packet_t*)tailItem->data)->header->csrc[2]);
           tailPacketGroupSeq = pGrounNAP[0] << 8 | pGrounNAP[1];
        }
    }

    do {
        pktCount = tsk_list_count_all(self->in_rtp_packets_list);
        uint16_t diff = TSK_ABS((tailPacketGroupSeq - self->iPacketGroupSeq));
        if ( self->in_rtp_packets_list->head &&
            ( 
              ((trtp_rtp_packet_t*)self->in_rtp_packets_list->head->data)->header->seq_num <= seq_num
              || (( diff > 0 &&(diff > 1 || (++self->iGroupRecvCount) >3)) || pktCount >80)
             )
            ){
            if(self->iLastPopGroup != tailPacketGroupSeq){
                self->iGroupRecvCount = 0;
            }
            self->iLastPopGroup = tailPacketGroupSeq;
            //TSK_DEBUG_INFO("rscode drop pktCount[%d] pktGroup[%d] tailPktGroup[%d]", pktCount,self->iPacketGroupSeq,tailPacketGroupSeq);
            item = tsk_list_pop_first_item(self->in_rtp_packets_list);
            uint16_t headNum = ((trtp_rtp_packet_t*)item->data)->header->seq_num;
            int delta = seq_num - headNum;
            if(delta > 0 && headNum > 10 && delta < 200 ){
                uint32_t csrcCount = ((trtp_rtp_packet_t*)item->data)->header->csrc_count;
                if ((csrcCount >= 6) && (((trtp_rtp_packet_t*)item->data)->header->csrc[2]!= 0)){
                    uint8_t* pGrounNAP = (uint8_t*)(&((trtp_rtp_packet_t*)item->data)->header->csrc[2]);
                    uint16_t iPacketGroupSeq = pGrounNAP[0] << 8 | pGrounNAP[1];
                    if(self->iPacketGroupSeq > iPacketGroupSeq || iPacketGroupSeq - self->iPacketGroupSeq > 10) {
                        TSK_DEBUG_INFO("rscode drop enters, self group seq[%d], pkt group seq[%d]", self->iPacketGroupSeq, iPacketGroupSeq);
                        TSK_OBJECT_SAFE_FREE(item);
                        item = tsk_null;
                    }
                }
            } else {
                break;
            }
        } else {
            break;
        }

        if (item) {
            break;
        }
    } while (pktCount > 0);

    if((!item)) {
        tsk_list_foreach(item, self->in_rtp_packets_list) {
            if(((trtp_rtp_packet_t*)item->data)->header->seq_num == seq_num) {
                item = tsk_list_pop_item_by_data(self->in_rtp_packets_list, (tsk_object_t*)item->data);
                break;
            }
        }
    }

    // TODO：接收侧部分
    // 这里有个问题，若是一直没有下一个包，需要等到数据包数量到达list上限后才取第一个包，这个策略需要斟酌下
    // 或者取另外一个策略：rscode这边只需要做rscode解码，若是非当前分组的非冗余包，直接传给下层jb来处理，jb来处理抖动
    //     此时可以对没有解码成功的groupnum做下cache，当后面有该分组的非冗余包到达时可以直接传递给后面的jb
    tsk_list_unlock(self->in_rtp_packets_list);
    return item;
}


static void* TSK_STDCALL tdav_rscode_thread_func(void *arg) {
    tdav_rscode_t *self = (tdav_rscode_t *)arg;
    TSK_DEBUG_INFO("rscode thread enters");

    //self->running = tsk_true;
    while(1) {
        if(!self->running) {
            break;
        }
        uint64_t no_read_count = 0;
        tsk_list_item_t *item = tsk_null;
        if (self->rs_type == RSC_TYPE_ENCODE)
        {
           // 发送端直接从 list 读数据
           tsk_list_lock(self->in_rtp_packets_list);
           item = tsk_list_pop_first_item(self->in_rtp_packets_list);
           tsk_list_unlock(self->in_rtp_packets_list);
        }
        else
        {
            if (self->last_seq_num < 0) {
                // first get packet to decode
                tsk_list_lock(self->in_rtp_packets_list);
                item = tsk_list_pop_first_item(self->in_rtp_packets_list);
                tsk_list_unlock(self->in_rtp_packets_list);
            } else {
                item = tdav_rscode_pop_packet_by_seq_num(self, self->last_seq_num + 1);
            }
            
            // 检查当前包是否重复到达，重复到达则过滤掉，不进行后续处理
            if (item != NULL) {
                trtp_rtp_packet_t* tmpPacket =  (trtp_rtp_packet_t*)item->data;
				if (tmpPacket->header->seq_num > 65500)
				{
					Clear_StdSet(self->stdSetInstance);
				} else {
					int iHasPacket = Has_StdSet(self->stdSetInstance, tmpPacket->header->seq_num, 1);
					if (iHasPacket != 0) {
                        if(tmpPacket->header->seq_num ==  self->last_seq_num + 1){
                            Clear_StdSet(self->stdSetInstance);
                        } else {
                            // 已经存在了就直接丢掉不要了
                            TSK_OBJECT_SAFE_FREE(item);
                            continue;
                        }
					}

					if (Count_StdSet(self->stdSetInstance) >= 500) {
						Clear_StdSet(self->stdSetInstance);
					}
				}
                
                Push_StdSet(self->stdSetInstance, tmpPacket->header->seq_num);
            }
        }
       
        if (item) {
            no_read_count = 0;
            //TMEDIA_I_AM_ACTIVE(2000, "rscode thread is running. session_id:%d", self->session_id);
            self->last_seq_num = ((trtp_rtp_packet_t*)item->data)->header->seq_num;
            if (self->useRscode) {
                tsk_list_t* rtp_packet_list = tdav_rscode_process(self, (trtp_rtp_packet_t*)item->data);
                tsk_list_item_t* rtp_packet_list_item = tsk_null;
                tsk_list_foreach(rtp_packet_list_item, rtp_packet_list)
                {
                    if(self->rs_type == RSC_TYPE_ENCODE) {
                        trtp_manager_push_rtp_packet(self->rtp_manager, ((trtp_rtp_packet_t*)rtp_packet_list_item->data));               
                    } else {
                        tsk_object_t* temp = tsk_object_ref(TSK_OBJECT(rtp_packet_list_item->data));
                        tsk_list_lock(self->out_rtp_packets_list);
                        tsk_list_push_ascending_data(self->out_rtp_packets_list, (tsk_object_t**)&temp);
                        tsk_list_unlock(self->out_rtp_packets_list);
                    }
                }
                TSK_OBJECT_SAFE_FREE(rtp_packet_list);
            }
            else
            {
                if(self->rs_type == RSC_TYPE_ENCODE) {
                    //直接放到待发送的rtp 队列
                    trtp_manager_push_rtp_packet(self->rtp_manager, ((trtp_rtp_packet_t*)item->data));
                } else {
                    //放入另一个队列，comsume 会来取，开启和没有开启rscode 需要兼容
                   // uint32_t csrc[15]; //[0] session_id  [1] 是否是rscode 冗余包  [2]rscode 包的组id [3]一组Rscode 包中 的序号 0--2*NPAR
                   trtp_rtp_packet_t*packet = (trtp_rtp_packet_t*)item->data;
                    if ((packet->header->csrc_count >=6) && (packet->header->csrc[2] != 0))
                    {
                        //说明是经过了处理的rscode 包
                        if (packet->header->csrc[1] == 0) {
                            //真实的数据包需要接解一下
                            
                            trtp_rtp_packet_t* pDecodePacket = trtp_rtp_packet_deserialize((char*)(packet->payload.data)+sizeof(int),packet->payload.size-sizeof(int));
                            if (NULL != pDecodePacket) {
                                pDecodePacket->header->my_session_id = packet->header->my_session_id;
                                pDecodePacket->header->session_id=packet->header->session_id;
                            
                                tsk_list_lock(self->out_rtp_packets_list);
                                tsk_list_push_back_data(self->out_rtp_packets_list,(tsk_object_t**)&pDecodePacket);
                                tsk_list_unlock(self->out_rtp_packets_list);
                            }
                            
                        }
                        
                    }
                    else
                    {
                        tsk_list_lock(self->out_rtp_packets_list);
                        tsk_object_t* temp = tsk_object_ref(TSK_OBJECT(item->data));
                        tsk_list_push_back_data(self->out_rtp_packets_list, (tsk_object_t**)&temp);
                        tsk_list_unlock(self->out_rtp_packets_list);

                    }
                }
            }
        }
        else
        {
            //没有取到包, 若当前in_rtp_packets_list长度为0时则等待，否则继续取
            //no_read_count ++;
//            tsk_list_lock(self->in_rtp_packets_list);
//            int count = tsk_list_count_all(self->in_rtp_packets_list);
//            tsk_list_unlock(self->in_rtp_packets_list);
//
//            if (0 == count) {
                tsk_thread_sleep(5);
//            }
        }
        TSK_OBJECT_SAFE_FREE(item);
    }
    
    tsk_list_lock(self->out_rtp_packets_list);
    tsk_list_clear_items(self->out_rtp_packets_list);
    tsk_list_unlock(self->out_rtp_packets_list);
    //TSK_OBJECT_SAFE_FREE(self->out_rtp_packets_list);
    
    tsk_list_lock(self->in_rtp_packets_list);
    tsk_list_clear_items(self->in_rtp_packets_list);
    tsk_list_unlock(self->in_rtp_packets_list);
    
    //TSK_OBJECT_SAFE_FREE(self->in_rtp_packets_list);
    TSK_DEBUG_INFO("rscode thread exits");
    return tsk_null;
}
void tdav_rscode_reset_NPAR(tdav_rscode_t *self, int iNewNPAR , int iNewCheckCodeLen)
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
    self->iCodeLen = self->iNPAR + self->iCheckCodeLen;
    self->iRecvDataPktCnt = 0;
    
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

tdav_rscode_t *tdav_rscode_create(int session_id,int iVideoID, RscType rs_type, trtp_manager_t *rtp_manager) {
    tdav_rscode_t* self;
    self = tsk_object_new (tdav_rscode_def_t);
    if(self) {
        self->session_id = session_id;
        self->rtp_manager = rtp_manager;
        self->rs_type = rs_type;
        self->iVideoID = iVideoID;
        if (rs_type == RSC_TYPE_DECODE) {
            self->stdSetInstance = Create_StdSet();
        }
    }
    return self;
}

int tdav_rscode_start(tdav_rscode_t *self) {
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
        int ret = tsk_thread_create(&self->thread_handle, tdav_rscode_thread_func, self);
        if((0 != ret) && (!self->thread_handle)) {
            TSK_DEBUG_ERROR("Failed to create rscode thread");
            tsk_safeobj_unlock(self);
            return -2;
        }
        ret = tsk_thread_set_priority(self->thread_handle, TSK_THREAD_PRIORITY_HIGH);
    }

    tsk_safeobj_unlock(self);
    return 0;
}

int tdav_rscode_stop(tdav_rscode_t *self) {
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

int tdav_rscode_push_rtp_packet(tdav_rscode_t *self, trtp_rtp_packet_t *packet) {
    
    if(!self || !packet) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    tsk_list_lock(self->in_rtp_packets_list);
	/*
    if (self->useRscode)
    {*/
    tsk_object_ref(TSK_OBJECT(packet));
    
    if(tsk_list_count_all(self->in_rtp_packets_list) > TDAV_RSCODE_MAX_LIST_LEN)
    {
        TSK_DEBUG_WARN("drop more than video 1000 pkt,clear!! sessionid:%d", self->session_id);
        tsk_list_clear_items(self->in_rtp_packets_list);
    }
    
    if(RSC_TYPE_ENCODE == self->rs_type || packet->header->seq_num < 100)
    {
        //序号翻转时，小的序列号放后边
        tsk_list_push_back_data(self->in_rtp_packets_list, (void **)&packet);
    }else{
        tsk_list_push_ascending_data(self->in_rtp_packets_list, (void **)&packet);
    }
    /*}
    else
    {
        trtp_rtp_packet_t* packet_repeat = tsk_object_ref(TSK_OBJECT(packet));
        tsk_list_push_back_data(self->in_rtp_packets_list, (void **)&packet);
        取消强制加倍冗余，之后作成单独配置
        if(RSC_TYPE_ENCODE == self->rs_type) {
            //增加一个冗余
            tsk_object_ref(TSK_OBJECT(packet_repeat));
            tsk_list_push_ascending_data(self->in_rtp_packets_list, (void **)&packet_repeat);
          
        }
         
    }*/
    tsk_list_unlock(self->in_rtp_packets_list);
    return 0;
}

tsk_list_item_t* tdav_rscode_pop_rtp_packet(tdav_rscode_t *self) {
    tsk_list_item_t* item = tsk_null;
    if(!self) 
    {
        TSK_DEBUG_ERROR("Invalid parameter");
        return tsk_null;
    }
    if(!self->running)
    {
        TSK_DEBUG_ERROR("rscode not running");
        return tsk_null;
    }
    
    tsk_list_lock(self->out_rtp_packets_list);
    item = tsk_list_pop_first_item(self->out_rtp_packets_list);
    tsk_list_unlock(self->out_rtp_packets_list);
    return item;
}


