//
//  ReachabilityServiceBridge.cpp
//  YouMeVoiceEngine
//
//  Created by wanglei on 15/9/17.
//  Copyright (c) 2015年 youme. All rights reserved.
//

#include "ReachabilityServiceBridge.h"
#import "ReachabilityService.h"

YouMeTalkReachabilityServiceBridgeIOS *YouMeTalkReachabilityServiceBridgeIOS::mPInstance = NULL;

YouMeTalkReachabilityServiceBridgeIOS *YouMeTalkReachabilityServiceBridgeIOS::getInstance ()
{
    if (mPInstance == NULL)
    {
        mPInstance = new YouMeTalkReachabilityServiceBridgeIOS ();
    }
    return mPInstance;
}
void YouMeTalkReachabilityServiceBridgeIOS::destroy ()
{
    delete mPInstance;
    mPInstance = NULL;
}


void YouMeTalkReachabilityServiceBridgeIOS::init (INgnNetworkChangCallback *networkChangeCallbakc)
{
    YouMeTalkReachabilityServiceIOS* pService = [YouMeTalkReachabilityServiceIOS getInstance] ;
    [pService start:networkChangeCallbakc];
}
void YouMeTalkReachabilityServiceBridgeIOS::uninit()
{
    YouMeTalkReachabilityServiceIOS* pService = [YouMeTalkReachabilityServiceIOS getInstance] ;
    [pService uninit];
}

YouMeTalkReachabilityServiceBridgeIOS::YouMeTalkReachabilityServiceBridgeIOS ()
{
}
YouMeTalkReachabilityServiceBridgeIOS::~YouMeTalkReachabilityServiceBridgeIOS ()
{
    [YouMeTalkReachabilityServiceIOS destroy];
}
