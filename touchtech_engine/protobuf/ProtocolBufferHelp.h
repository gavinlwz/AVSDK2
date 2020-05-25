//
//  ProtocolBufferHelp.h
//  YouMeVoiceEngine
//
//  Created by user on 15/9/30.
//  Copyright (c) 2015年 youme. All rights reserved.
//

#ifndef __YouMeVoiceEngine__ProtocolBufferHelp__
#define __YouMeVoiceEngine__ProtocolBufferHelp__

#include <stdio.h>
#include "common.pb.h"
#include "YoumeRunningState.pb.h"
class CProtocolBufferHelp
{
public:
    static YouMeProtocol::PacketHead* CreatePacketHead(YouMeProtocol::MSG_TYPE msgType,int iProtoVer=11);
    
    
    static YouMeProtocol::DataReportBase* CreateDataReportHead(YouMeProtocol::UserEvt evt,int evtCode =0);
};
#endif /* defined(__YouMeVoiceEngine__ProtocolBufferHelp__) */
