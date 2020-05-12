//
//  VoiceTest.cpp
//  perf_test
//
//  Created by wanglei on 16/1/4.
//  Copyright © 2016年 tencent. All rights reserved.
//

#include "VoiceTest.hpp"
#include "IYouMeVoiceEngine.h"

void CVoiceTest::onEvent (const YouMeEvent_t &eventType, const YouMeErrorCode &iErrorCode)
{
    m_pEngine->joinConference(m_strRoomID);
}
void CVoiceTest::onCallEvent (const YouMeCallEvent_t &event, const YouMeErrorCode_t &errCode)
{
    printf("event=%d\n",event);
}