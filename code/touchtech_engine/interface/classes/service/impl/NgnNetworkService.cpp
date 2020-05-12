//
//  NgnNetworkService.cpp
//  cocos2d-x sdk
//
//  Created by wanglei on 15/8/7.
//  Copyright (c) 2015年 youme. All rights reserved.
//

#include "NgnConfigurationEntry.h"
#include "NgnMemoryConfiguration.hpp"
#include "NgnEngine.h"
#include "NgnNetworkService.h"
#if defined(__APPLE__)
#include "ReachabilityServiceBridge.h"
#endif


NgnNetworkService::NgnNetworkService ()
{
    mUseWifi = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::NETWORK_USE_WIFI, NgnConfigurationEntry::DEFAULT_NETWORK_USE_WIFI);
    mUseMobile = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::NETWORK_USE_MOBILE, NgnConfigurationEntry::DEFAULT_NETWORK_USE_3G);
}

NgnNetworkService::~NgnNetworkService ()
{
}

bool NgnNetworkService::start ()
{
#if defined(__APPLE__)
    YouMeTalkReachabilityServiceBridgeIOS::getInstance ()->init (this);
#endif
    return true;
}

bool NgnNetworkService::stop ()
{
#if defined(__APPLE__)
    YouMeTalkReachabilityServiceBridgeIOS::getInstance()->uninit();
    YouMeTalkReachabilityServiceBridgeIOS::destroy ();
    TSK_DEBUG_INFO ("YouMeTalkReachabilityServiceBridgeIOS stoped");
#endif
    return true;
}

NETWORK_TYPE NgnNetworkService::getNetworkType ()
{
    return mNetWorkType;
}

bool NgnNetworkService::hasNetwork ()
{
    if (mNetWorkType != NETWORK_TYPE_NO)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool NgnNetworkService::isMobileNetwork()
{
    return mNetWorkType == NETWORK_TYPE_MOBILE;
}

bool NgnNetworkService::isWIFI ()
{
    return mNetWorkType == NETWORK_TYPE_WIFI;
}
/*
string NgnNetworkService::getDnsServer (DNS_TYPE type)
{
    tnet_addresses_L_t *pDnsServers = NULL;
    string strDnsServer = "";
    pDnsServers = tnet_dns_resolvconf_parse ("/etc/resolv.conf");
    if (!pDnsServers || TSK_LIST_IS_EMPTY (pDnsServers))
    {
        return strDnsServer;
    }
    const tsk_list_item_t *pItem = NULL;
    const tnet_address_t *pAddress = NULL;

    tsk_list_foreach (pItem, pDnsServers)
    {
        if (!(pAddress = (tnet_address_t *)(pItem->data)))
        {
            continue;
        }

        // Skip loopback address to avoid problems :)
        if ((pAddress->family == AF_INET && tsk_striequals (pAddress->ip, "127.0.0.1")) || (pAddress->family == AF_INET6 && tsk_striequals (pAddress->ip, "::1")))
        {
            continue;
        }
        strDnsServer = pAddress->ip;
        TSK_OBJECT_SAFE_FREE (pDnsServers);
        pAddress = NULL;
        break;
    }
    return strDnsServer;
}*/

string NgnNetworkService::getLocalIP (bool ipv6)
{
    tsk_bool_t unicast = tsk_true;
    tsk_bool_t anycast = tsk_false;
    tsk_bool_t multicast = tsk_false;
    long if_index = -1;

    tnet_set_usemobilenetWork (mUseMobile);
    tnet_addresses_L_t *pAddresses = tnet_get_addresses ((ipv6 ? AF_INET6 : AF_INET), unicast, anycast, multicast,  if_index);
    if (!pAddresses || TSK_LIST_IS_EMPTY (pAddresses))
    {
        return "";
    }
//    const tsk_list_item_t *pItem = NULL;
//    const tnet_address_t *pAddress = NULL;
    string strLocalIP = "";

    // 在某些设备上，如Samsung S7, 有几个模拟网卡，一旦选择了这些模拟网卡，网络通信就会出问题，所以使用通配地址
    if (!ipv6) {
        strLocalIP = "0.0.0.0";
        TSK_OBJECT_SAFE_FREE (pAddresses);
    } else {
//        tsk_list_foreach (pItem, pAddresses)
//        {
//            if (!(pAddress = (tnet_address_t *)(pItem->data)))
//            {
//                continue;
//            }
//
//            // Skip loopback address to avoid problems :)
//            if ((pAddress->family == AF_INET && tsk_striequals (pAddress->ip, "127.0.0.1")) || (pAddress->family == AF_INET6 && tsk_striequals (pAddress->ip, "::1")))
//            {
//                continue;
//            }
//            strLocalIP = pAddress->ip;
//            TSK_OBJECT_SAFE_FREE (pAddresses);
//            pAddress = NULL;
//            break;
//        }
        strLocalIP = "::";
    }

    TSK_DEBUG_INFO ("local ip is %s", strLocalIP.c_str ());
    return strLocalIP;
}
bool NgnNetworkService::release ()
{
   
    return true;
}

void NgnNetworkService::onNetWorkChanged (NETWORK_TYPE type)
{
    TSK_DEBUG_INFO ("%d ",type);
    mNetWorkType = type;
    if (mPCallback != NULL)
    {
        mPCallback->onNetWorkChanged (type);
    }
}

void NgnNetworkService::registerCallback (INgnNetworkChangCallback *pCallback)
{
    mPCallback = pCallback;
}

