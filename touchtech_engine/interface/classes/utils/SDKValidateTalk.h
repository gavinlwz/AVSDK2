//
//  SDKValidate.h
//  YouMeVoiceEngine
//
//  Created by user on 15/9/23.
//  Copyright (c) 2015年 youme. All rights reserved.
//

#ifndef __YouMeVoiceEngine__SDKValidate__
#define __YouMeVoiceEngine__SDKValidate__

#include <stdio.h>
#include <string>
#include <vector>
#include <thread>
#include <map>
#include "YouMeCommon/RSAUtil.h"
#include "DNSUtil.h"
#include "XCondWait.h"
#include "XAny.h"
#include "YouMeConstDefine.h"
#include "ReportMessageDef.h"
#include "YouMeServerValidProtocol.pb.h"
#include "SyncTCPTalk.h"


enum SDKValidate_Platform
{
    SDKValidate_Platform_Unknow,
    SDKValidate_Platform_Android,
    SDKValidate_Platform_IOS,
    SDKValidate_Platform_Windows,
    SDKValidate_Platform_OSX,
    SDKValidate_Platform_Linux
};

typedef struct RedirectServerInfo_s
{
    std::string host;
    uint32_t    port;
} RedirectServerInfo_t;

typedef std::vector<RedirectServerInfo_t> RedirectServerInfoVec_t;

// Mappping between "Region Name" and "Number of players in this region"
typedef std::map<std::string, int> ServerRegionNameMap_t;

class CSDKValidate
{
private:
    CSDKValidate ()
    {
        m_bInit = false;
        m_devicePlatform = SDKValidate_Platform_Unknow;
    }

public:
    static CSDKValidate *GetInstance ();
    bool Init ();
    void UnInit ();
    void Reset ();
    void Abort ();

    //先要设置好这4个参数，然后才能真正调用
    void SetPackageName (const std::string &strPackageName);
    void SetPlatform (SDKValidate_Platform platform);
    void SetAppKey (const std::string &strAppKey);
    bool SetAppSecret (const std::string &strAppSecret);
    void SetAppKeySuffix( const std::string& strAppKeySuffix );

    //开始验证
    YouMeErrorCode ServerLoginIn (bool bReconnect, std::string& appkeySuffix,
                                  RedirectServerInfoVec_t& redirectServerInfoVec, bool& canRefreshRedirect);
    
    YouMeErrorCode GetRedirectList(ServerRegionNameMap_t& regionNameMap, RedirectServerInfoVec_t& redirectServerInfoVec);

    void InitReportProc(youmeRTC::ReportDNSParse dns, const std::string strValidURL, uint64_t ulStart, YouMeErrorCode errCode);
    // 数据上报初始化
    void ReportServiceInit();

    //提供获取各种参数函数
    std::map<std::string, youmecommon::CXAny> getConfigurations ();
    //必须要先调用反初始化才能调用
    static void Destroy ();


private:
    bool ParseAppSecret ();
    YouMeErrorCode getValidateIPList(std::string url, std::vector<XString> & ipList, bool bReconnect);
    
    /* 获取上报地址 */
    std::string getReportURL( std::string& strAppKeySuffix );
    YouMeErrorCode getReportIPList(std::string url, std::vector<XString> & ipList, bool bReconnect);
    
    void updateConfigurations (std::map<std::string, youmecommon::CXAny>& configurations);
    void parseValidateServerResponse(YouMeProtocol::ServerValidResponse &response, RedirectServerInfoVec_t &redirectServerInfoVec, bool &canRefreshRedirect);
    YouMeErrorCode ValidateWithUdp(std::vector<XString> &strIPList, int serverPort, std::string &strProbufData,
                                   RedirectServerInfoVec_t& redirectServerInfoVec, bool& canRefreshRedirect);
    YouMeErrorCode ValidateWithTcp(std::vector<XString> &strIPList, std::vector<int>& serverPort, std::string &strProbufData,
                                   RedirectServerInfoVec_t& redirectServerInfoVec, bool& canRefreshRedirect);
    YouMeErrorCode GetRedirectListUdp(std::string &strProbufData, RedirectServerInfoVec_t& redirectServerInfoVec);
    YouMeErrorCode GetRedirectListTcp(std::string &strProbufData, RedirectServerInfoVec_t& redirectServerInfoVec);
    
    private:
    static CSDKValidate *s_signle;
    bool m_bInit;
    std::string m_strPackageName;
    SDKValidate_Platform m_devicePlatform;
    std::string m_strAppKey;
    std::string m_strAppSecret;
    std::string m_strAppKeySuffix;
    // RSA 的公钥和模数
    youmecommon::CRSAUtil m_rsa;
  
    std::thread m_reportInitThread;

    std::map<std::string, youmecommon::CXAny> m_configurations;
    youmecommon::CXCondWait m_condHandle;
    std::vector<XString> m_strIPList;
    std::vector<int>  m_vecServerPort;  //Tcp用多端口
    int m_serverPort = 0;       //udp用单端口
    
    youmertc::CSyncTCP m_validateTcp;
    youmertc::CSyncTCP m_getRedirectTcp;
    bool m_isAborting = false;
    
    youmecommon::DNSUtil* m_dnsHandle;
};


#endif /* defined(__YouMeVoiceEngine__SDKValidate__) */
