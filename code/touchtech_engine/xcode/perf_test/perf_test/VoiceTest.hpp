//
//  VoiceTest.hpp
//  perf_test
//
//  Created by wanglei on 16/1/4.
//  Copyright © 2016年 tencent. All rights reserved.
//

#ifndef VoiceTest_hpp
#define VoiceTest_hpp
#include <string>
using namespace::std;
#include "IYouMeVoiceEngine.h"
#include "IYouMeCommonCallback.h"
#include "IYouMeConferenceCallback.h"

class CVoiceTest : public IYouMeCommonCallback, public IYouMeConferenceCallback
{
public:
    CVoiceTest(IYouMeVoiceEngine* pEngine,string strRoomID)
    {
        m_pEngine = pEngine;
        m_strRoomID = strRoomID;
    }
public:
    virtual void onEvent (const YouMeEvent_t &eventType, const YouMeErrorCode &iErrorCode);
    virtual void onCallEvent (const YouMeCallEvent_t &event, const YouMeErrorCode_t &errCode);
private:
    IYouMeVoiceEngine* m_pEngine = NULL;
    string m_strRoomID;
};

#endif /* VoiceTest_hpp */
