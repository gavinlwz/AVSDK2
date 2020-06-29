//
//  ProtocolBufferHelp.cpp
//  YouMeVoiceEngine
//
//  Created by user on 15/9/30.
//  Copyright (c) 2015年 youme. All rights reserved.
//

#include "ProtocolBufferHelp.h"
#include "tsk_debug.h"
#include "NgnApplication.h"
#include "tsk_time.h"
#include "NgnEngine.h"
#include "NgnConfigurationEntry.h"
#include "NgnMemoryConfiguration.hpp"
#include "YouMeVoiceEngine.h"

YouMeProtocol::DataReportBase* CProtocolBufferHelp::CreateDataReportHead(YouMeProtocol::UserEvt evt,int evtCode )
{
    YouMeProtocol::DataReportBase* pReportHead = new YouMeProtocol::DataReportBase;
    pReportHead->set_allocated_head(CreatePacketHead(YouMeProtocol::MSG_DataReport));
    pReportHead->set_evt(evt);
    pReportHead->set_code(evtCode);
    
    //根据服务器的时间和本地时间来计算
    unsigned int iLocalNowTime = tsk_gettimeofday_ms()/1000;
    unsigned int iServerBaseTime = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::VERIFY_TIME, NgnConfigurationEntry::DEFAULT_VERIFY_TIME);
    
    pReportHead->set_reporttime(iServerBaseTime + iLocalNowTime- NgnApplication::getInstance()->getLocalBaseTime());
   
    //新增加userid
    std::string userid = CYouMeVoiceEngine::getInstance()->GetJoinUserName();
    pReportHead->set_userid(userid);
    
    return pReportHead;
}
YouMeProtocol::PacketHead* CProtocolBufferHelp::CreatePacketHead(YouMeProtocol::MSG_TYPE msgType,int iProtoVer)
{
    YouMeProtocol::PacketHead *pHead = new YouMeProtocol::PacketHead;
    pHead->set_appkey(NgnApplication::getInstance()->getAppKey());
    pHead->set_msgtype(msgType);
    pHead->set_msgversion(iProtoVer);
    SDKValidate_Platform platform = NgnApplication::getInstance()->getPlatform();
    if (platform == SDKValidate_Platform_Android) {
        pHead->set_platform(YouMeProtocol::Platform_Android);
    }
    else if (platform == SDKValidate_Platform_IOS)
    {
         pHead->set_platform(YouMeProtocol::Platform_IOS);
    }
    else if (platform == SDKValidate_Platform_OSX)
    {
        pHead->set_platform(YouMeProtocol::Platform_OSX);
    }
    else if (platform == SDKValidate_Platform_Windows)
    {
        pHead->set_platform(YouMeProtocol::Platform_Windows);
    }
    else if (platform == SDKValidate_Platform_Linux)
    {
        pHead->set_platform(YouMeProtocol::Platform_Linux);
    }
    else
    {
         pHead->set_platform(YouMeProtocol::Platform_Unknow);
    }
    pHead->set_model(NgnApplication::getInstance()->getModel());
    pHead->set_brand(NgnApplication::getInstance()->getBrand());
    pHead->set_sysversion(NgnApplication::getInstance()->getSysVersion());
    pHead->set_ymsdkversion(NgnApplication::getInstance()->getSDKVersion());
  
    std::string strIdentify =NgnApplication::getInstance()->getDeviceIMEI();
    if (strIdentify.empty()) {
        strIdentify =NgnApplication::getInstance()->getUUID();
    }
    pHead->set_identify(strIdentify);
    pHead->set_cpuarch(NgnApplication::getInstance()->getCPUArch());
    pHead->set_cpuchip(NgnApplication::getInstance()->getCPUChip());
    pHead->set_packagename(NgnApplication::getInstance()->getPackageName());
    pHead->set_sdkname("voice");
    NETWORK_TYPE netType = NgnEngine::getInstance ()->getNetworkService()->getNetworkType();
    if (netType == NETWORK_TYPE_MOBILE)
    {
        pHead->set_networktype(YouMeProtocol::NetworkType_3G);
    }
    else if(netType == NETWORK_TYPE_WIFI)
    {
        pHead->set_networktype(YouMeProtocol::NetworkType_Wifi);
    }
    else
    {
        pHead->set_networktype(YouMeProtocol::NetworkType_Unknow);
    }

    std::string userid = CYouMeVoiceEngine::getInstance()->GetJoinUserName();
    pHead->set_userid(userid);

    uint64_t businessId = CYouMeVoiceEngine::getInstance()->getBusinessId();
    pHead->set_businessid(businessId);
    
    return pHead;
}
