//
//  NgnNetworkService.h
//  cocos2d-x sdk
//
//  Created by wanglei on 15/8/7.
//  Copyright (c) 2015年 youme. All rights reserved.
//

#ifndef __cocos2d_x_sdk__NgnNetworkService__
#define __cocos2d_x_sdk__NgnNetworkService__

#include <string>
using namespace std;
#include "INgnNetworkService.h"


/**
 * Network service.
 */
class NgnNetworkService :public INgnNetworkService
{
    private:
    bool mUseWifi = true;

    bool mUseMobile = false;

    bool mStarted = false;

    NETWORK_TYPE mNetWorkType = NETWORK_TYPE_WIFI;

    INgnNetworkChangCallback *mPCallback = NULL;

    public:
    NgnNetworkService ();

    virtual ~NgnNetworkService ();

    public:
    virtual bool start ();

    virtual bool stop ();

    virtual NETWORK_TYPE getNetworkType ();

    virtual bool hasNetwork ();

    virtual bool isWIFI ();
    virtual bool isMobileNetwork() ;
   // virtual string getDnsServer (DNS_TYPE type);

    virtual string getLocalIP (bool ipv6);

    virtual bool release ();

    virtual void onNetWorkChanged (NETWORK_TYPE type);

    virtual void registerCallback (INgnNetworkChangCallback *pCallback);

    
};


#endif /* defined(__cocos2d_x_sdk__NgnNetworkService__) */
