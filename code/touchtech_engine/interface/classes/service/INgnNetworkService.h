//
//  INgnNetworkService.h
//  cocos2d-x sdk
//
//  Created by wanglei on 15/8/5.
//  Copyright (c) 2015年 youme. All rights reserved.
//

#ifndef cocos2d_x_sdk_INgnNetworkService_h
#define cocos2d_x_sdk_INgnNetworkService_h
#include <string>
using namespace std;

#include "INgnBaseService.h"

enum DNS_TYPE
{
    DNS_1,
    DNS_2,
    DNS_3,
    DNS_4
};


enum NETWORK_TYPE
{
    NETWORK_TYPE_NO = -1,
    /** Current network is WIFI */
    NETWORK_TYPE_WIFI = 1,
    NETWORK_TYPE_MOBILE=2
};

class INgnNetworkChangCallback
{
    public:
    virtual void onNetWorkChanged (NETWORK_TYPE type) = 0;
};

class INgnNetworkService : public INgnBaseService, public INgnNetworkChangCallback
{
    public:
    virtual ~INgnNetworkService ()
    {
    }

    public:
    virtual NETWORK_TYPE getNetworkType () = 0;

    virtual bool hasNetwork () = 0;

    virtual bool isWIFI () = 0;
    virtual bool isMobileNetwork() = 0;
   // virtual string getDnsServer (DNS_TYPE type) = 0;

    virtual string getLocalIP (bool ipv6) = 0;


    virtual bool release () = 0;
    virtual void registerCallback (INgnNetworkChangCallback *pCallback) = 0;
};


#endif
