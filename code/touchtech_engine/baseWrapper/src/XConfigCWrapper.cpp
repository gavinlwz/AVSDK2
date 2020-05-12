//
//  XConfigCWrapper.cpp
//  youme_voice_engine
//
//  Created by joexie on 16/1/27.
//  Copyright © 2016年 youme. All rights reserved.
//

#include "XConfigCWrapper.hpp"
#include "NgnMemoryConfiguration.hpp"
#include "AVStatistic.h"
#include "serverlogin.pb.h"
#include "tsk_debug.h"
#include <set>

//#include "NgnApplication.h"

#include "CustomInputManager.h"
#include <YouMeCommon/CryptUtil.h>
#include <YouMeCommon/CrossPlatformDefine/PlatformDef.h>
#include "tsk_memory.h"
extern "C"
{
    unsigned int Config_GetUInt(const char* szKey,unsigned int iDefault)
    {
        return CNgnMemoryConfiguration::getInstance()->GetConfiguration(szKey,iDefault);
    }
    
    int Config_GetInt(const char* szKey,int iDefault)
    {
        return CNgnMemoryConfiguration::getInstance()->GetConfiguration(szKey,iDefault);
    }
    void Config_SetInt(const char* szKey,int iDefault)
    {
        CNgnMemoryConfiguration::getInstance()->SetConfiguration(szKey,iDefault);
    }

    int Config_GetBool(const char* szKey,int iDefault)
    {
        return CNgnMemoryConfiguration::getInstance()->GetConfiguration(szKey,(bool)iDefault);
    }
    
    void Config_SetString(const char* szKey ,char *pValue)
    {
        CNgnMemoryConfiguration::getInstance()->SetConfiguration(szKey, std::string(pValue));
    }

    void Config_GetString(const char* szKey,const char* pDefault,char *pValue)
    {
        std::string str = CNgnMemoryConfiguration::getInstance()->GetConfiguration(szKey,(std::string)pDefault);
        strcpy(pValue, str.c_str());
    }

//    const char* getDocumentpath()
//    {
//        static string path = "";
//        //NgnApplication::CombinePath(NgnApplication::getInstance()->getDocumentPath() , "audio_bak.txt");
//        return path.c_str();
//    }

	void  Clear_StdSet(void* pSetInstance)
	{
		std::set<unsigned short>* pInstance = (std::set<unsigned short>*)pSetInstance;
		pInstance->clear();
	}

    void  DeleteItem_StdSet(void* pSetInstance,int iCount)
    {
        std::set<unsigned short>* pInstance = (std::set<unsigned short>*)pSetInstance;
        std::set<unsigned short>::iterator begin = pInstance->begin();
        int iIndex=0;
        for (; begin != pInstance->end();)
        {
            if (iIndex <iCount)
            {
                begin = pInstance->erase(begin);
                iIndex++;
                continue;
            }
            break;
        }
    }
    void* Create_StdSet()
    {
        return new std::set<unsigned short>();
    }
    void  Push_StdSet(void* pSetInstance,unsigned short value)
    {
        std::set<unsigned short>* pInstance = (std::set<unsigned short>*)pSetInstance;
        pInstance->insert(value);
    }
    int  Count_StdSet(void* pSetInstance)
    {
        std::set<unsigned short>* pInstance = (std::set<unsigned short>*)pSetInstance;
        return  (int)pInstance->size();
    }
    int  Has_StdSet(void* pSetInstance, unsigned short value,int iDeleteItem)
    {
        std::set<unsigned short>* pInstance = (std::set<unsigned short>*)pSetInstance;
        std::set<unsigned short>::iterator it = pInstance->find(value);
        if (it == pInstance->end()) {
            return 0;
        }
        else
        {
            if (iDeleteItem != 0)
            {
                pInstance->erase(it);
            }
        }
        return  1;
    }
    
    void Destroy_StdSet(void* pSetInstance)
    {
        std::set<unsigned short>* pInstance = (std::set<unsigned short>*)pSetInstance;
        delete pInstance;
    }
    
    
    void AddAudioCode( int codeCount, int sessionId  )
    {
        AVStatistic::getInstance()->addAudioCode( codeCount, sessionId );
        
    }
    void AddVideoCode( int codeCount, int sessionId )
    {
        AVStatistic::getInstance()->addVideoCode( codeCount, sessionId );

    }
    void AddVideoShareCode( int codeCount, int sessionId )
    {
        AVStatistic::getInstance()->addVideoShareCode( codeCount, sessionId );

    }

    void AddVideoFrame( int frameCount, int sessionId )
    {
        AVStatistic::getInstance()->addVideoFrame( frameCount, sessionId );

    }
    
    // void AddVideoBlock( int blockCount, int sessionId )
    // {
    //     AVStatistic::getInstance()->addVideoBlock( blockCount , sessionId );
    // }
    
    void AddVideoBlockTime( int blockTime, int32_t sessionId )
    {
        AVStatistic::getInstance()->addVideoBlockTime( blockTime , sessionId );
    }
    
   
    void AddAudioPacket( int seqNum, int32_t sessionId )
    {
        AVStatistic::getInstance()->addAudioPacket( seqNum, sessionId );
    }
    void AddVideoPacket( int seqNum, int32_t sessionId )
    {
        AVStatistic::getInstance()->addVideoPacket( seqNum, sessionId );

    }
    
    void AddVideoPacketTimeGap( int timegap, int sessionId )
    {
        AVStatistic::getInstance()->addVideoPacketTimeGap( timegap, sessionId );
        
    }
    
    void AddVideoPacketDelay( int timeDelay, int sessionId )
    {
        AVStatistic::getInstance()->addVideoPacketDelay( timeDelay, sessionId );
    }
    
    
    
    void AddSelfVideoPacket( int lost, int total, int sessionId )
    {
        AVStatistic::getInstance()->addSelfVideoPacket( lost , total, sessionId);
    }
    
    void AddSelfAudioPacket( int lost, int total, int sessionId )
    {
        AVStatistic::getInstance()->addSelfAudioPacket( lost , total, sessionId);
    }

    void setAudioUpPacketLossRtcp( int lost, int sessionId )
    {
        AVStatistic::getInstance()->setAudioUpPacketLossRtcp(lost, sessionId);
    }

    void setAudioDnPacketLossRtcp( int lost, int sessionId )
    {
        AVStatistic::getInstance()->setAudioDnPacketLossRtcp(lost, sessionId);
    }

    void setVideoUpPacketLossRtcp( int lost, int sessionId )
    {
        AVStatistic::getInstance()->setVideoUpPacketLossRtcp(lost, sessionId);
    }

    void setVideoDnPacketLossRtcp( int lost, int sessionId )
    {
        AVStatistic::getInstance()->setVideoDnPacketLossRtcp(lost, sessionId);
    }

    void setAudioPacketDelayRtcp( int delay, int sessionId )
    {
        AVStatistic::getInstance()->setAudioPacketDelayRtcp(delay, sessionId);
    }

    void setVideoPacketDelayRtcp( int delay, int sessionId )
    {
        AVStatistic::getInstance()->setVideoPacketDelayRtcp(delay, sessionId);
    }

    void setRecvDataStat(uint64_t recvStat)
    {
        AVStatistic::getInstance()->setRecvDataStat(recvStat);
    }
    
    void SendNotifyEvent( const char* typeName,  int value  )
    {
        sendNotifyEvent( typeName, value );
    }
    
    
    int GetMediaCtlReqBuffer( int bStart, int sessionId, int stat_type, char* buff,  int* len  )
    {
        if( buff == NULL || len == NULL )
        {
            return -1;
        }
        
        ::YouMeProtocol::YouMeVoice_Command_Media_ctl_req  reportReq;
        ::YouMeProtocol::YouMeVoice_Media_ctl_Header* head = new YouMeProtocol::YouMeVoice_Media_ctl_Header();
        
        if( bStart )
        {
            head->set_cmd( YouMeProtocol::OPEN_MEDIA_STAT_NOTIFY );
        }
        else{
            head->set_cmd( YouMeProtocol::CLOSE_MEDIA_STAT_NOTIFY );
        }
        
        head->set_seq( 0 );
        head->set_timestamp( time(0) );
        head->set_session( sessionId );
        head->set_ret_code( 0 );
        
        reportReq.set_allocated_head( head );
        reportReq.set_stat_interval( 1 );
        reportReq.set_stat_type( stat_type );

        std::string strSendData;
        reportReq.SerializeToString (&strSendData);
        
        if( *len < strSendData.length() ){
            return -1;
        }
        
        *len = strSendData.length();
        memcpy( buff, strSendData.c_str(), strSendData.length() );
        
        return 0;
    }
    
    int  GetMediaCtlRspFromBuffer( char* data, int len, MediaCtlRsp* rspData, int* cmd)
    {
        if( data == NULL || rspData == NULL )
        {
            return -1;
        }
        
        YouMeProtocol::YouMeVoice_Command_Media_ctl_rsp rsp; // 使用proto buf 解析数据
        
        if (!rsp.ParseFromArray( data, len)) {
            TSK_DEBUG_ERROR("####GetMediaCtlRspFromBuffer: protobuf parsing failed:%d",len);
            return -1;
        }
        
        if (rsp.has_head()) {
            *cmd = rsp.head().cmd();

            // TSK_DEBUG_ERROR("GetMediaCtlRspFromBuffer: cmd:%d, seq:%d, ts:%lld, session:%d, ret:%d"
            //     , rsp.head().cmd(), rsp.head().seq(), rsp.head().timestamp(), rsp.head().session()
            //     , rsp.head().ret_code());
        }

        if( rsp.has_packet_stat() )
        {
            rspData->min_seq = rsp.packet_stat().min_seq();
            rspData->max_seq = rsp.packet_stat().max_seq();
            rspData->total_recv_num = rsp.packet_stat().total_recv_num();
            rspData->last_stat_st = rsp.packet_stat().last_stat_st();
            rspData->curr_stat_st = rsp.packet_stat().curr_stat_st();
            rspData->media_type = rsp.packet_stat().media_type();
        }
        else{
            //只有包头
            return 1;
        }
        
        return 0;
    }


	void SetCustonInputCallback(OnInputDataNotify pCallback, void* pParam)
	{
		CCustomInputManager::getInstance()->setInputCallback(pCallback, pParam);
	}

	void NotifyCustomData(const void*pData, int iDataLen, unsigned long long uTimeSpan, uint32_t uSessionId)
	{
		CCustomInputManager::getInstance()->NotifyCustomData(pData, iDataLen, uTimeSpan, uSessionId);
	}



	void* Create_ByteArray()
	{
		return new std::string;
	}

	void Destroy_ByteArray(void* pInstance)
	{
		std::string* pStringInstance = (std::string*)pInstance;
		delete pStringInstance;
	}

	void ByteArray_Append(void* pInstance, const void*buffer, int iLen)
	{
		std::string* pStringInstance = (std::string*)pInstance;
		pStringInstance->append((const char*)buffer, iLen);
	}

	int ByteArray_Length(void* pInstance)
	{
		std::string* pStringInstance = (std::string*)pInstance;
		return pStringInstance->length();
	}

	const void*   ByteArray_Data(void* pInstance)
	{
		std::string* pStringInstance = (std::string*)pInstance;
		return pStringInstance->c_str();
	}

	void ByteArray_Remove(void* pInstance, int iOffset, int iLen)
	{
		std::string* pStringInstance = (std::string*)pInstance;
		pStringInstance->erase(iOffset, iLen);
	}

	int Encrypt_Data(const char*pszBuffer, int iBufferLen, const char*pszPasswd, void**pszResult)
	{
		XString strPasswd = UTF8TOXString2(pszPasswd);
		std::string strResult = youmecommon::CCryptUtil::EncryptByDES_S(pszBuffer, iBufferLen, strPasswd);
		*pszResult = tsk_malloc(strResult.length());
		memcpy(*pszResult, strResult.c_str(), strResult.length());
		return strResult.length();
	}

}


