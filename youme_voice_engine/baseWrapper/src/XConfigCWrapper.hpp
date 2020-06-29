//
//  XConfigCWrapper.hpp
//  youme_voice_engine
//
//  Created by joexie on 16/1/27.
//  Copyright © 2016年 youme. All rights reserved.
//

#ifndef XConfigCWrapper_hpp
#define XConfigCWrapper_hpp

#include <stdio.h>
#include <stdint.h>
//给C 调用的全局配置的函数，内存配置


#ifdef __cplusplus
extern "C" {
#endif
    
#define CONFIG_MediaCtlPort "MediaCtlPort"
#define VIDEOCODE_LEVEl_PARAM_MAX  5
 

    
    //C 接口的部分服务器下发都是有符号整形，C99 里面没有bool ，使用整形
    unsigned int Config_GetUInt(const char* szKey,unsigned int iDefault);
    int Config_GetInt(const char* szKey,int iDefault);
    void Config_SetInt(const char* szKey,int iDefault);
    int Config_GetBool(const char* szKey,int iDefault);
    
    void Config_SetString(const char* szKey ,char *pValue);
    void Config_GetString(const char* szKey,const char* pDefault,char *pValue);
    
    //封装一个C 使用的std set 用来保存序号，你妹的C
    void* Create_StdSet();
    void  Push_StdSet(void* ,unsigned short value);
    int   Has_StdSet(void*, unsigned short value,int iDeleteItem);
    void  DeleteItem_StdSet(void* pSetInstance,int iCount);
    int   Count_StdSet(void*);
    void  Destroy_StdSet(void* pSetInstance);
	void  Clear_StdSet(void* pSetInstance);
    

    
    
    //监控AV统计数据
    void AddAudioCode( int codeCount, int sessionId  );
    void AddVideoCode( int codeCount, int sessionId );
    void AddVideoShareCode( int codeCount, int sessionId );
    void AddVideoFrame( int frameCount, int sessionId );
    // void AddVideoBlock( int blockCount, int sessionId );
    
    void AddVideoBlockTime( int blockTime, int32_t sessionId );
    
    void AddAudioPacket( int seqNum, int sessionId );
    void AddVideoPacket( int seqNum, int sessionId );
    
    void AddVideoPacketTimeGap( int timegap, int sessionId );
    void AddVideoPacketDelay( int timeDelay, int sessionId );
    
    void AddSelfVideoPacket( int lost, int total, int sessionId );
    void AddSelfAudioPacket( int lost, int total, int sessionId );

    void setAudioUpPacketLossRtcp( int lost, int sessionId );
    void setAudioDnPacketLossRtcp( int lost, int sessionId );
    void setVideoUpPacketLossRtcp( int lost, int sessionId );
    void setVideoDnPacketLossRtcp( int lost, int sessionId );
    void setAudioPacketDelayRtcp( int delay, int sessionId );
    void setVideoPacketDelayRtcp( int delay, int sessionId );
    void setRecvDataStat(uint64_t recvStat);

    void SendNotifyEvent( const char* typeName,  int value  );

    enum MediaCtrlType
    {
        MEDIA_TYPE_AUDIO = 1,
        MEDIA_TYPE_VIDEO = 2
    };
 
    struct MediaCtlRsp
    {
        int min_seq;
        int max_seq;
        int total_recv_num;
        long long  last_stat_st;
        long long  curr_stat_st;
        int media_type;   //1表示音频，2表示视频
    };
    
    //返回0表示成功
    int GetMediaCtlReqBuffer( int bStart, int sessionId, int stat_type, char* buff,  int* len  );
    int  GetMediaCtlRspFromBuffer( char* data, int len, struct MediaCtlRsp* rspData, int* cmd );
    

	//自定义的数据回调, uSessionId为发送用户
	typedef void(*OnInputDataNotify)(const void*pData, int iDataLen, unsigned long long uTimeSpa,void* pParam, uint32_t uSessionId);
	void SetCustonInputCallback(OnInputDataNotify pCallback,void* pParam);

	//接收到自定义的消息数据，回调给上面, uSessionId为发送用户
	void NotifyCustomData(const void*pData, int iDataLen, unsigned long long uTimeSpan, uint32_t uSessionId);






    
    
//    const char* getDocumentpath();
    

	//用std::string 模拟一个可追加的缓存区
	void* Create_ByteArray();
	void  Destroy_ByteArray(void* pInstance);
	void  ByteArray_Append(void* pInstance, const void*buffer, int iLen);
	int   ByteArray_Length(void* pInstance);
	const void*   ByteArray_Data(void* pInstance);
	void ByteArray_Remove(void* pInstance, int iOffset, int iLen);

	int Encrypt_Data(const char*pszBuffer, int iBufferLen, const char*pszPasswd, void**pszResult);
#ifdef __cplusplus
}
#endif

#endif /* XConfigCWrapper_hpp */
