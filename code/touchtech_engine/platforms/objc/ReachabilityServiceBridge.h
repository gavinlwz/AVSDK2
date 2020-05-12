//
//  ReachabilityServiceBridge.h
//  YouMeVoiceEngine
//
//  Created by wanglei on 15/9/17.
//  Copyright (c) 2015年 youme. All rights reserved.
//

#ifndef __YouMeVoiceEngine__ReachabilityServiceBridge__
#define __YouMeVoiceEngine__ReachabilityServiceBridge__

#include "INgnNetworkService.h"
class YouMeTalkReachabilityServiceBridgeIOS
{
    public:
    static YouMeTalkReachabilityServiceBridgeIOS *getInstance ();
    static void destroy ();

    public:
    void init (INgnNetworkChangCallback *networkChangeCallback);
    void uninit();
    private:
    YouMeTalkReachabilityServiceBridgeIOS ();
    ~YouMeTalkReachabilityServiceBridgeIOS ();

    private:
    static YouMeTalkReachabilityServiceBridgeIOS *mPInstance;
};

#endif /* defined(__YouMeVoiceEngine__ReachabilityServiceBridge__) */
