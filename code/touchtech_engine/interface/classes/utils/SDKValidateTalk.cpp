//
//  SDKValidate.cpp
//  YouMeVoiceEngine
//
//  Created by user on 15/9/23.
//  Copyright (c) 2015年 youme. All rights reserved.
//

#include "SDKValidateTalk.h"
#include <assert.h>
#include <sstream>
#ifdef WIN32
#else
#include <unistd.h>
#include <netdb.h>
#endif

#include "XConfigCWrapper.hpp"
#include "ProtocolBufferHelp.h"
#include "tsk_base64.h"
#include "tsk_memory.h"
#include "tsk_debug.h"
#include "tsk_uuid.h"

#include "tsk_string.h"
#include "SyncUDP.h"
#include "version.h"
#include "tsk_time.h"
#include "MonitoringCenter.h"
#include "XOsWrapper.h"
#include "NgnEngine.h"
#include "ReportService.h"
#include "NgnConfigurationEntry.h"
#include "AVStatistic.h"

//"123.59.62.126",

//中国
static std::string g_cnBackupSdkValidateIPs[] = { ("47.106.74.42"), ("120.79.5.50") };
//海外
static std::string g_overseaBackupSdkValidateIPs[] = { ("47.75.205.71"), ("47.91.152.229") };
// test env backup
static std::string g_cnBackupSdkValidateIPsForTest[] = { ("39.105.175.55") };

// 数据上报兜底ip
#define REPORT_BACKUP_IPLIST    ("123.59.150.76")

#define SDKVALIDATE_PORT            (8012)
#define SDKVALIDATE_PORT2           (8011)
#define SDKVALIDATE_PORT3           (5001)

#define REPORT_TCPPORT              (8001)
#define REPORT_UDPPORT              (8001)
#define RECONNECT_DNS_RETRY_COUNT   (2)
#define RECONNECT_DNS_WAIT_TIME     (1000) //ms

#define DNS_PARSE_TIMEOUT           (1000)  //ms

extern int g_serverMode;
extern std::string g_serverIp;
extern int g_serverPort;
extern YOUME_RTC_SERVER_REGION g_serverRegionId;
extern std::string g_extServerRegionName;


/* SDK 验证地址（生产环境）*/
#define DEFAULT_VALIDATE_SERVER "cn.rtc.youme.im";
#define SDKVALID_URL_SUFFIX  "rtc.youme.im"
#define REPORT_SERVER_URL  "dr.youme.im"

/* SDK 验证地址（测试环境）*/
const static std::map<int, std::string> g_sdkValidTestServer =
{
    {SERVER_MODE_TEST,   "t.voiceconfig.youme.im"}, /* 测试服(default) */
    {SERVER_MODE_DEV,    "d.voiceconfig.youme.im"}, /* 开发服 */
    {SERVER_MODE_DEMO,   "b.voiceconfig.youme.im"}  /* 商务服 */
};

/* 数据上报地址（生产环境） */
const static std::map<YOUME_RTC_SERVER_REGION, std::string> g_reportServer =
{
    {RTC_CN_SERVER, "cn.dr.youme.im"}, /* 中国(default) */
    {RTC_HK_SERVER, "hk.dr.youme.im"}, /* 香港 */
    {RTC_US_SERVER, "us.dr.youme.im"}, /* 美国 */
    {RTC_SG_SERVER, "sg.dr.youme.im"}, /* 新加坡 */
    {RTC_KR_SERVER, "kr.dr.youme.im"}, /* 韩国 */
    {RTC_AU_SERVER, "au.dr.youme.im"}, /* 澳洲 */
    {RTC_DE_SERVER, "de.dr.youme.im"}, /* 德国 */
    {RTC_BR_SERVER, "br.dr.youme.im"}, /* 巴西 */
    {RTC_IN_SERVER, "in.dr.youme.im"}, /* 印度 */
    {RTC_JP_SERVER, "jp.dr.youme.im"}, /* 日本 */
    {RTC_IE_SERVER, "ie.dr.youme.im"}  /* 爱尔兰 */
};

/* 数据上报地址（测试环境） */
const static std::map<int, std::string> g_reportTestServer =
{
    {SERVER_MODE_TEST,   "t.dr.youme.im"}, /* 测试服(default) */
    {SERVER_MODE_DEV,    "d.dr.youme.im"}, /* 开发服 */
    {SERVER_MODE_DEMO,   "b.dr.youme.im"}  /* 商务服 */
};


CSDKValidate *CSDKValidate::s_signle = NULL;

CSDKValidate *CSDKValidate::GetInstance ()
{
    //这里就不加锁了，简单判断一下
    if (NULL == s_signle)
    {
        s_signle = new CSDKValidate;
    }
    return s_signle;
}

//这玩意也不加锁了，开始的时候初始化一次吧。反正自家人用
bool CSDKValidate::Init ()
{
    if (m_bInit)
    {
        return true;
    }
    m_bInit = true;
    return m_bInit;
}
//说实话并没有什么用，单实例的玩意一般都不会释放，除非退出
void CSDKValidate::UnInit ()
{
    if (!m_bInit)
    {
        return;
    }
    Abort ();
    m_bInit = false;
}

//服务器验证的http 请求
YouMeErrorCode CSDKValidate::ServerLoginIn (bool bReconnect, std::string& appkeySuffix,
                                            RedirectServerInfoVec_t& redirectServerInfoVec, bool& canRefreshRedirect)
{
    TSK_DEBUG_INFO ("## serverRegionId:%d, extServerRegionName:%s", g_serverRegionId, g_extServerRegionName.c_str());
    
    YouMeErrorCode errCode = YOUME_SUCCESS;
    std::string strProbufData;
    char szRandString[TSK_UUID_STRING_SIZE+1];
    youmecommon::CXSharedArray<unsigned char>  result;
    uint64_t ulStart = 0;
    std::string strValidURL;
    YouMeErrorCode ipRet;
    YouMeProtocol::ServerValid validProtocol;
    youmeRTC::ReportDNSParse dns;
    m_serverPort = SDKVALIDATE_PORT;
    
    m_vecServerPort.push_back(  SDKVALIDATE_PORT );
    m_vecServerPort.push_back(  SDKVALIDATE_PORT2 );
    m_vecServerPort.push_back(  SDKVALIDATE_PORT3 );

    
    m_strIPList.clear();
    YouMeProtocol::PacketHead* head = nullptr;

    if (!m_bInit)
    {
        TSK_DEBUG_ERROR ("Not initialized");
        errCode = YOUME_ERROR_NOT_INIT;
        goto sdk_validate_end;
    }
    
    // init dns parse instance
    m_dnsHandle = youmecommon::DNSUtil::Instance();
    if (!m_dnsHandle) {
        TSK_DEBUG_ERROR ("Not get dns parse instance");
        errCode = YOUME_ERROR_UNKNOWN;
        goto sdk_validate_end;
    }
    
    redirectServerInfoVec.clear();
    canRefreshRedirect = false;
    m_condHandle.Reset();
    m_configurations.clear();
    if ((int)g_serverRegionId < (int)RTC_EXT_SERVER) {
        // For RTC_DEFAULT_SERVER, the server cannot recognize it, and will return a default server list.
        // So trickily, we just got what we want(the default list). For others, the server will recognize
        // them and will return the relevant server list.
        validProtocol.set_idc((uint32_t)g_serverRegionId);
    } else if (RTC_EXT_SERVER == g_serverRegionId) {
        validProtocol.set_idc_area(g_extServerRegionName);
    }
    //else : do not send any area
    
    //sdvalidate可能用不同的key，所以这里要把head里的改了
    head = CProtocolBufferHelp::CreatePacketHead (YouMeProtocol::MSG_SDK_AUTH);
    validProtocol.set_allocated_head (head );

    validProtocol.set_bussines(YouMeProtocol::Bussiness_AV);
    //生成一个64位的随机字符串
    
    tsk_uuidgenerate(&szRandString);
    //对数据进行RSA加密
    
    if (!m_rsa.EncryptByPublicKey ((const unsigned char *)szRandString, (int)strlen(szRandString), result))
    {
        TSK_DEBUG_ERROR ("Failed to encrypt");
        errCode = YOUME_ERROR_ILLEGAL_SDK;
        goto sdk_validate_end;
    }
    
    
    validProtocol.set_data(result.Get(), result.GetBufferLen());
    validProtocol.SerializeToString (&strProbufData);
 
    ulStart = tsk_time_now();
    
    // Get Validate domain name
    if (g_serverMode != SERVER_MODE_FORMAL) {
        std::map<int, std::string>::const_iterator it = g_sdkValidTestServer.find(g_serverMode);
        
        if (it != g_sdkValidTestServer.end()) {
            strValidURL = it->second;
        } else if (SERVER_MODE_FIXED_IP_VALIDATE == g_serverMode) {
            strValidURL = g_serverIp;
        } else if (SERVER_MODE_FIXED_IP_PRIVATE_SERVICE == g_serverMode) {
            // sanity check
            tsk_bool_t privateServerFlag = Config_GetBool("PRIVATE_SERVER", 1);
            if (privateServerFlag) {
                TSK_DEBUG_INFO ("private server mode set, server:%s", g_serverIp.c_str());
                strValidURL = g_serverIp;
            } else {
                TSK_DEBUG_ERROR ("private server mode is not set, server:%s", g_serverIp.c_str());
                errCode = YOUME_ERROR_INVALID_PARAM;
                goto sdk_validate_end;
            }
        } else {
            strValidURL = g_sdkValidTestServer.find(SERVER_MODE_TEST)->second; // default
        }
    } else {
        strValidURL = appkeySuffix;
        strValidURL.append(".");
//        if (!g_extServerRegionName.empty()) {
//            strValidURL.append(g_extServerRegionName);
//            strValidURL.append(".");
//        }
        strValidURL.append(SDKVALID_URL_SUFFIX);
    }
    
    // Parse the domain name
    if (g_serverMode != SERVER_MODE_FIXED_IP_VALIDATE && g_serverMode != SERVER_MODE_FIXED_IP_PRIVATE_SERVICE) {
        ipRet = getValidateIPList(strValidURL, m_strIPList, bReconnect);
    } else {
        TSK_DEBUG_INFO ("#### private server mode:%d, Parsing SDKValidate, g_serverIp:%s", g_serverMode, g_serverIp.c_str());
        m_strIPList.push_back(LocalToXString(g_serverIp));
        ipRet = YOUME_SUCCESS;
    }
    
    if (SERVER_MODE_FIXED_IP_VALIDATE == g_serverMode || SERVER_MODE_FIXED_IP_PRIVATE_SERVICE == g_serverMode) {
        m_serverPort = g_serverPort;
        
        m_vecServerPort.clear();
        m_vecServerPort.push_back(  g_serverPort );
    }

    
    if (ipRet != YOUME_SUCCESS) {
        errCode = ipRet;
        goto sdk_validate_end;
    }
    
    
    // 新的数据上报
    
    dns.parse_addr = strValidURL;
    dns.parse_usetime = (tsk_time_now() - ulStart);
    dns.parse_result = 0;
    for (int i = 0; i < m_strIPList.size(); i++) {
        dns.parse_iplist.append(XStringToLocal(m_strIPList[i])).append(";");  // ip列表
    }
    dns.sdk_version = SDK_NUMBER;
    dns.platform = NgnApplication::getInstance()->getPlatform();

    errCode = ValidateWithTcp(m_strIPList, m_vecServerPort, strProbufData, redirectServerInfoVec, canRefreshRedirect);
    if (YOUME_SUCCESS != errCode  ) {
        errCode = ValidateWithUdp(m_strIPList, m_serverPort, strProbufData, redirectServerInfoVec, canRefreshRedirect);
    }
    
    if( YOUME_SUCCESS == errCode )
    {
        map<string, youmecommon::CXAny> configurations = CSDKValidate::GetInstance ()->getConfigurations ();
        updateConfigurations (configurations);
        
//        string canalID = CNgnMemoryConfiguration::getInstance()->GetConfiguration( NgnConfigurationEntry::CANAL_ID, "");
        
        
        string canalID = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::CANAL_ID, NgnConfigurationEntry::DEFAULT_CANAL_ID);
        NgnApplication::getInstance()->setCanalID( canalID );
    }
    
sdk_validate_end:
    // Before creating a new thread, check if the previous thread was stopped.
    // If not, stop it first.
    if (m_reportInitThread.joinable()) {
        if (std::this_thread::get_id() != m_reportInitThread.get_id()) {
            TSK_DEBUG_INFO("Start to join the InitReport thread");
            m_reportInitThread.join();
            TSK_DEBUG_INFO("Join the InitReport thread OK");
        } else {
            m_reportInitThread.detach();
        }
    }
    m_reportInitThread = std::thread(&CSDKValidate::InitReportProc, this, dns, strValidURL, ulStart, errCode);
    m_reportInitThread.detach();
    TSK_DEBUG_INFO ("SDK Validate exit");
    return errCode;
}

void CSDKValidate::InitReportProc(youmeRTC::ReportDNSParse dns, const std::string strValidURL, uint64_t ulStart, YouMeErrorCode errCode)
{
    // 新的数据上报
    ReportService * report = ReportService::getInstance();
    report->init(); /* 拉完配置再初始化数据上报，确保后面使用的服务器配置 */
    MonitoringCenter::getInstance()->setSdkConfigAvail();
    
    dns.canal_id = NgnApplication::getInstance()->getCanalID();
    report->report(dns);
    
    youmeRTC::ReportSDKValid sdk;
    sdk.server_type = g_serverRegionId;
    sdk.server_addr = strValidURL;
    sdk.sdk_usetime = tsk_time_now() - ulStart;
    sdk.sdk_result = -errCode; //errcode是负的
    sdk.redirect_server = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::NETWORK_REDIRECT_ADD, NgnConfigurationEntry::DEFAULT_NETWORK_REDIRECT_ADD);
    sdk.redirect_port = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::NETWORK_REDIRECT_PORT, NgnConfigurationEntry::DEFAULT_NETWORK_REDIRECT_PORT);
    sdk.sdk_version = SDK_NUMBER;
    
    sdk.os_name = NgnApplication::getInstance()->getSysName();
    sdk.os_version = NgnApplication::getInstance()->getSysVersion();
    sdk.cpu_chip = NgnApplication::getInstance()->getCPUChip();
    sdk.platform = NgnApplication::getInstance()->getPlatform();
    sdk.canal_id = NgnApplication::getInstance()->getCanalID();
    sdk.package_name = NgnApplication::getInstance()->getPackageName();
    
    //SDK valid 不成功的话，要强制上报
    report->report(sdk, errCode != YOUME_SUCCESS );
    TSK_DEBUG_INFO ("SDK Validate Report:%d", errCode);
}

YouMeErrorCode CSDKValidate::ValidateWithUdp(std::vector<XString> &strIPList, int serverPort, std::string &strProbufData,
                                             RedirectServerInfoVec_t& redirectServerInfoVec, bool& canRefreshRedirect)
{
    YouMeErrorCode errCode = YOUME_ERROR_NETWORK_ERROR;
    for (int i = 0; i < 3 ; i++)
    {
        for (int ServerCount=0; ServerCount < strIPList.size(); ServerCount++)
        {
            youmecommon::CSyncUDP udp;
            TSK_DEBUG_INFO ("--UDP Validate SDK with %s:%d", XStringToLocal(strIPList[ServerCount]).c_str(), serverPort);
            if (!udp.Init(strIPList[ServerCount], serverPort))
            {
                TSK_DEBUG_ERROR ("Failed to create socket");
                errCode = YOUME_ERROR_UNKNOWN;
                goto err_exit;
            }

            udp.SendData(strProbufData.c_str (), strProbufData.length());
            //i=0 的时候并没有wait，所以第一次发包几乎是不可能立即收到的，这个地方需要修改下
            if (youmecommon::WaitResult_Timeout != m_condHandle.WaitTime(0)) {
                //要退出了，赶紧退出
                TSK_DEBUG_INFO ("==UDP validate aborted");
                errCode = YOUME_ERROR_USER_ABORT;
                goto err_exit;
            }

            youmecommon::CXSharedArray<char> recvBuffer;
            int num = udp.RecvData(recvBuffer,i*1000,&m_condHandle);
            TSK_DEBUG_INFO ("SDKValidate returns length:%d",num);
            if (num <= 0)
            {
                TSK_DEBUG_WARN ("recvfrom() error :%s", m_strIPList[ServerCount].c_str());
                m_condHandle.WaitTime(2*1000);
                continue;
            }
            else
            {
                //成功退出
                YouMeProtocol::ServerValidResponse response;
                if (!response.ParseFromArray(recvBuffer.Get(),num))
                {
                    TSK_DEBUG_ERROR ("protobuf parsing failed, switch to the next server:%s",m_strIPList[ServerCount].c_str());
                    errCode = YOUME_ERROR_SERVER_INTER_ERROR;
                    goto err_exit;
                }

                youmecommon::CXSharedArray<unsigned char> rsaDeBuffer ;
                if (!m_rsa.DecryptByPublicKey ((const unsigned char *)response.data().c_str(), response.data().length(), rsaDeBuffer))
                {
                    TSK_DEBUG_ERROR ("Failed to decrypt http RSA public key");
                    errCode = YOUME_ERROR_ILLEGAL_SDK;
                    goto err_exit;
                }
                
                int iStatus = response.status ();
                TSK_DEBUG_INFO("status:%d : %d :%s", iStatus,response.timeout(),m_strIPList[ServerCount].c_str());
                if (0 == iStatus)
                {
                    parseValidateServerResponse(response, redirectServerInfoVec, canRefreshRedirect);
                    MonitoringCenter::getInstance()->NotifyDataReportParseDNS();
                    errCode = YOUME_SUCCESS;
                    goto err_exit;
                }
                else
                {
                    if (1 ==  iStatus)
                    {
                        if (youmecommon::WaitResult_Timeout != m_condHandle.WaitTime(response.timeout())) {
                            //要退出了，赶紧退出
                            TSK_DEBUG_INFO ("==UDP validarte aborted");
                            errCode = YOUME_ERROR_USER_ABORT;
                            goto err_exit;
                        }
                        continue;
                    }
                    errCode = YOUME_ERROR_ILLEGAL_SDK;
                    goto err_exit;
                }
            }
        }
    }
    
err_exit:
    return errCode;
}

YouMeErrorCode CSDKValidate::ValidateWithTcp(std::vector<XString> &strIPList, std::vector<int>& vecServerPort, std::string &strProbufData,
                                             RedirectServerInfoVec_t& redirectServerInfoVec, bool& canRefreshRedirect)
{
    YouMeErrorCode errCode = YOUME_ERROR_NETWORK_ERROR;
    for( int portCount = 0 ; portCount < vecServerPort.size(); portCount++ ){
        int serverPort = vecServerPort[portCount];
        for (int ServerCount = 0; ServerCount < strIPList.size(); ServerCount++)
        {
            if (m_isAborting) {
                TSK_DEBUG_INFO ("==TCP validate aborted");
                errCode = YOUME_ERROR_USER_ABORT;
                goto err_exit;
            }
            
            TSK_DEBUG_INFO ("--TCP Validate SDK with %s:%d", XStringToLocal(strIPList[ServerCount]).c_str(), serverPort);
            m_validateTcp.UnInit();
            
            if (!m_validateTcp.Init(XStringToLocal(strIPList[ServerCount]), serverPort, 25))
            {
                TSK_DEBUG_ERROR ("Failed to create socket");
                errCode = YOUME_ERROR_UNKNOWN;
                goto err_exit;
            }
            
            if (!m_validateTcp.Connect(10)) {
                TSK_DEBUG_ERROR ("Failed to connect to the validate server");
                XSleep(1000);
                continue;
            }
            TSK_DEBUG_INFO ("--TCP Validate connected");
            
            int sentBytes = m_validateTcp.SendData(strProbufData.c_str (), strProbufData.length());
            if (strProbufData.length() != sentBytes) {
                continue;
            }
            
            youmecommon::CXSharedArray<char> recvBuffer;
            int recvNum = m_validateTcp.RecvData (recvBuffer);
            TSK_DEBUG_INFO ("SDKValidate returns length:%d",recvNum);
            if (recvNum <= 0)
            {
                TSK_DEBUG_ERROR ("RecvData error");
                continue;
            }
            else
            {
                //成功退出
                YouMeProtocol::ServerValidResponse response;
                if (!response.ParseFromArray(recvBuffer.Get(),recvNum))
                {
                    TSK_DEBUG_ERROR ("protobuf parsing failed");
                    errCode = YOUME_ERROR_SERVER_INTER_ERROR;
                    goto err_exit;
                }
                
                youmecommon::CXSharedArray<unsigned char> rsaDeBuffer ;
                if (!m_rsa.DecryptByPublicKey ((const unsigned char *)response.data().c_str(), response.data().length(), rsaDeBuffer))
                {
                    TSK_DEBUG_ERROR ("Failed to decrypt http RSA public key");
                    errCode = YOUME_ERROR_ILLEGAL_SDK;
                    goto err_exit;
                }
                
                int iStatus = response.status ();
                TSK_DEBUG_INFO("status:%d : %d :%s", iStatus,response.timeout(),strIPList[ServerCount].c_str());
                if (0 == iStatus)
                {
                    parseValidateServerResponse(response, redirectServerInfoVec, canRefreshRedirect);
                    MonitoringCenter::getInstance()->NotifyDataReportParseDNS();
                    errCode = YOUME_SUCCESS;
                    goto err_exit;
                }
                else
                {
                    if (1 ==  iStatus)
                    {
                        continue;
                    }
                    errCode = YOUME_ERROR_ILLEGAL_SDK;
                    goto err_exit;
                }
            }
        }

    }
    
err_exit:
    m_validateTcp.UnInit();
    return errCode;
}

void CSDKValidate::updateConfigurations (map<string, youmecommon::CXAny>& configurations)
{
    CNgnMemoryConfiguration::getInstance()->Clear();
    for (map<string, youmecommon::CXAny>::iterator it = configurations.begin (); it != configurations.end (); it++)
    {
        CNgnMemoryConfiguration::getInstance()->SetConfiguration(it->first,it->second);
    }
}

/**
 * Parse the response from the validate server.
 */
void CSDKValidate::parseValidateServerResponse(YouMeProtocol::ServerValidResponse &response, RedirectServerInfoVec_t &redirectServerInfoVec, bool &canRefreshRedirect)
{
    int i;
    for (i = 0; i < response.configurations_size (); i++)
    {
        TSK_DEBUG_INFO("type:%d key:%s value:%s",response.configurations(i).type(),response.configurations (i).name ().c_str(), response.configurations (i).value ().c_str());
        switch (response.configurations(i).type()) {
            case YouMeProtocol::NAME_BOOL:
            {
                m_configurations.insert (std::map<std::string, youmecommon::CXAny>::value_type (response.configurations (i).name (), (bool)tsk_atoll(response.configurations (i).value ().c_str())));
            }
                break;
            case YouMeProtocol::NAME_INT32:
            {
                m_configurations.insert (std::map<std::string, youmecommon::CXAny>::value_type (response.configurations (i).name (), (int)tsk_atoll(response.configurations (i).value ().c_str())));
            }
                break;
            case YouMeProtocol::NAME_INT64:
            {
                m_configurations.insert (std::map<std::string, youmecommon::CXAny>::value_type (response.configurations (i).name (), (long)tsk_atoll(response.configurations (i).value ().c_str())));
            }
                break;
            case YouMeProtocol::NAME_UIN32:
            {
                m_configurations.insert (std::map<std::string, youmecommon::CXAny>::value_type (response.configurations (i).name (), (unsigned int)tsk_atoll(response.configurations (i).value ().c_str())));
            }
                break;
            case YouMeProtocol::NAME_UINT64:
            {
                m_configurations.insert (std::map<std::string, youmecommon::CXAny>::value_type (response.configurations (i).name (), (unsigned long)tsk_atoll(response.configurations (i).value ().c_str())));
            }
                break;
            case YouMeProtocol::NAME_STRING:
            {
                m_configurations.insert (std::map<std::string, youmecommon::CXAny>::value_type (response.configurations (i).name (), response.configurations (i).value ()));
            }
                break;
            default:
                break;
        }
        
    }
    
    if (response.has_get_redirect_flag()) {
        canRefreshRedirect = response.get_redirect_flag();
    }

    // Reserve enough space for the default redirect server and the full redirect server list.
    redirectServerInfoVec.reserve(response.redirect_info_list_size());
    for (i = 0; i < response.redirect_info_list_size(); i++) {
        RedirectServerInfo_t redirectServerInfo;
        
        if (response.redirect_info_list(i).has_host()) {
            redirectServerInfo.host = response.redirect_info_list(i).host();
        } else {
            redirectServerInfo.host = "";
        }
        
        if (response.redirect_info_list(i).has_port()) {
            redirectServerInfo.port = response.redirect_info_list(i).port();
        } else {
            redirectServerInfo.port = 0;
        }
        
        redirectServerInfoVec.push_back(redirectServerInfo);
        TSK_DEBUG_INFO("--Redirect server %s:%d", redirectServerInfo.host.c_str(), redirectServerInfo.port);
    }
}

YouMeErrorCode CSDKValidate::GetRedirectList(ServerRegionNameMap_t& regionNameMap, RedirectServerInfoVec_t& redirectServerInfoVec)
{
    TSK_DEBUG_INFO ("## GetRedirectList UDP");
    
    if (!m_bInit)
    {
        TSK_DEBUG_ERROR ("Need to init first");
        return YOUME_ERROR_NOT_INIT;
    }
    
    if (regionNameMap.empty()) {
        return YOUME_ERROR_UNKNOWN;
    }
    
    redirectServerInfoVec.clear();
    m_condHandle.Reset();
    YouMeProtocol::GetRedirectRequest redirectRequest;
    redirectRequest.set_allocated_head (CProtocolBufferHelp::CreatePacketHead (YouMeProtocol::MSG_GET_REDIRECT));
    ServerRegionNameMap_t::iterator it;
    for (it = regionNameMap.begin(); it != regionNameMap.end(); it++) {
        redirectRequest.add_idc_area(it->first);
    }
    std::string strProbufData;
    redirectRequest.SerializeToString (&strProbufData);
    
    YouMeErrorCode errCode = GetRedirectListTcp(strProbufData, redirectServerInfoVec);
    if (YOUME_SUCCESS != errCode) {
        errCode = GetRedirectListUdp(strProbufData, redirectServerInfoVec);
    }
    return errCode;
}

YouMeErrorCode CSDKValidate::GetRedirectListUdp(std::string &strProbufData, RedirectServerInfoVec_t& redirectServerInfoVec)
{
    YouMeErrorCode errCode = YOUME_ERROR_NETWORK_ERROR;
    
    for (int retryCnt = 0; retryCnt < 3 ; retryCnt++)
    {
        for (int ServerCount=0; ServerCount<m_strIPList.size(); ServerCount++)
        {
            youmecommon::CSyncUDP udp;
            TSK_DEBUG_INFO ("--UDP GetRedirectList with %s:%d", m_strIPList[ServerCount].c_str(), m_serverPort);
            if (!udp.Init(m_strIPList[ServerCount], m_serverPort))
            {
                TSK_DEBUG_ERROR ("Failed to create socket");
                errCode = YOUME_ERROR_UNKNOWN;
                goto err_exit;
            }
            
            udp.SendData(strProbufData.c_str (), strProbufData.length());
            //i=0 的时候并没有wait，所以第一次发包几乎是不可能立即收到的，这个地方需要修改下
            if (youmecommon::WaitResult_Timeout != m_condHandle.WaitTime(0)) {
                //要退出了，赶紧退出
                TSK_DEBUG_ERROR ("==UDP get redirect aborted");
                errCode = YOUME_ERROR_USER_ABORT;
                goto err_exit;
            }
            
            youmecommon::CXSharedArray<char> recvBuffer;
            int num = udp.RecvData(recvBuffer, retryCnt*1000, &m_condHandle);
            TSK_DEBUG_INFO ("!!GetRedirectList returns data length:%d",num);
            if (num <= 0)
            {
                TSK_DEBUG_WARN ("recvfrom() error :%s",m_strIPList[ServerCount].c_str());
                m_condHandle.WaitTime(2*1000);
                continue;
            }
            else
            {
                //成功退出
                YouMeProtocol::GetRedirectResponse response;
                if (!response.ParseFromArray(recvBuffer.Get(),num))
                {
                    TSK_DEBUG_ERROR ("Failed to parse protobuf header, switch to the next server:%s",m_strIPList[ServerCount].c_str());
                    errCode = YOUME_ERROR_SERVER_INTER_ERROR;
                    continue;
                }
                
                int iStatus = response.status ();
                TSK_DEBUG_INFO("UDP GetRedirectList status:%d : %s", iStatus, m_strIPList[ServerCount].c_str());
                if (0 == iStatus)
                {
                    RedirectServerInfo_t redirectServerInfo;
                    // Reserver enough space for the default redirect server and the full redirect server list.
                    redirectServerInfoVec.reserve(response.redirect_info_list_size());
                    for (int i = 0; i < response.redirect_info_list_size(); i++) {
                        if (response.redirect_info_list(i).has_host()) {
                            redirectServerInfo.host = response.redirect_info_list(i).host();
                        } else {
                            redirectServerInfo.host = "";
                        }
                        
                        if (response.redirect_info_list(i).has_port()) {
                            redirectServerInfo.port = response.redirect_info_list(i).port();
                        } else {
                            redirectServerInfo.port = 0;
                        }
                        
                        redirectServerInfoVec.push_back(redirectServerInfo);
                        TSK_DEBUG_INFO("--Redirect server %s:%d", redirectServerInfo.host.c_str(), redirectServerInfo.port);
                    }
                    errCode = YOUME_SUCCESS;
                    goto err_exit;
                }
                else
                {
                    if (1 ==  iStatus)
                    {
                        if (youmecommon::WaitResult_Timeout != m_condHandle.WaitTime(500)) {
                            //要退出了，赶紧退出
                            TSK_DEBUG_INFO ("==UDP get reidrect aborted");
                            errCode = YOUME_ERROR_USER_ABORT;
                            goto err_exit;
                        }
                        continue;
                    }
                    errCode = YOUME_ERROR_ILLEGAL_SDK;
                    goto err_exit;
                }
            }
        }
    }
    
err_exit:
    return errCode;
}

YouMeErrorCode CSDKValidate::GetRedirectListTcp(std::string &strProbufData, RedirectServerInfoVec_t& redirectServerInfoVec)
{
    YouMeErrorCode errCode = YOUME_ERROR_NETWORK_ERROR;
    for( int portCount = 0; portCount < m_vecServerPort.size(); portCount++ ){
        for (int ServerCount=0; ServerCount<m_strIPList.size(); ServerCount++)
        {
            if (m_isAborting) {
                TSK_DEBUG_INFO ("==TCP get redirect aborted");
                errCode = YOUME_ERROR_USER_ABORT;
                goto err_exit;
            }
            
            TSK_DEBUG_INFO("--TCP GetRedirectList with %s:%d", XStringToLocal(m_strIPList[ServerCount]).c_str(), m_vecServerPort[portCount]);
            m_getRedirectTcp.UnInit();
            if (!m_getRedirectTcp.Init(XStringToLocal(m_strIPList[ServerCount]), m_vecServerPort[portCount], 25))
            {
                TSK_DEBUG_ERROR ("Failed to create socket");
                continue;
            }
            
            if (!m_getRedirectTcp.Connect(25)) {
                TSK_DEBUG_ERROR ("Failed to connect to the validate server");
                continue;
            }
            
            int sentBytes = m_getRedirectTcp.SendData(strProbufData.c_str (), strProbufData.length());
            if (strProbufData.length() != sentBytes) {
                TSK_DEBUG_ERROR ("Failed to send data to the validate server");
                continue;
            }
            
            youmecommon::CXSharedArray<char> recvBuffer;
            int recvNum = m_getRedirectTcp.RecvData (recvBuffer);
            TSK_DEBUG_INFO ("!!GetRedirectList returns data length:%d", recvNum);
            if (recvNum <= 0)
            {
                TSK_DEBUG_ERROR ("RecvData error");
                continue;
            }
            else
            {
                //成功退出
                YouMeProtocol::GetRedirectResponse response;
                if (!response.ParseFromArray(recvBuffer.Get(),recvNum))
                {
                    TSK_DEBUG_ERROR ("Failed to parse protobuf header");
                    errCode = YOUME_ERROR_SERVER_INTER_ERROR;
                    continue;
                }
                
                int iStatus = response.status ();
                TSK_DEBUG_INFO("TCP GetRedirectList status:%d : %s", iStatus, m_strIPList[ServerCount].c_str());
                if (0 == iStatus)
                {
                    RedirectServerInfo_t redirectServerInfo;
                    // Reserver enough space for the default redirect server and the full redirect server list.
                    redirectServerInfoVec.reserve(response.redirect_info_list_size());
                    for (int i = 0; i < response.redirect_info_list_size(); i++) {
                        if (response.redirect_info_list(i).has_host()) {
                            redirectServerInfo.host = response.redirect_info_list(i).host();
                        } else {
                            redirectServerInfo.host = "";
                        }
                        
                        if (response.redirect_info_list(i).has_port()) {
                            redirectServerInfo.port = response.redirect_info_list(i).port();
                        } else {
                            redirectServerInfo.port = 0;
                        }
                        
                        redirectServerInfoVec.push_back(redirectServerInfo);
                        TSK_DEBUG_INFO("--Redirect server %s:%d", redirectServerInfo.host.c_str(), redirectServerInfo.port);
                    }
                    errCode = YOUME_SUCCESS;
                    goto err_exit;
                }
                else
                {
                    if (1 ==  iStatus)
                    {
                        continue;
                    }
                    errCode = YOUME_ERROR_ILLEGAL_SDK;
                    goto err_exit;
                }
            }
        }
    }
    
err_exit:
    m_getRedirectTcp.UnInit();
    return errCode;
}

/**
 * 获取上报服务器地址
 */
std::string CSDKValidate::getReportURL( std::string& strAppKeySuffix)
{
    string reportURL = "";
    if (g_serverMode != SERVER_MODE_FORMAL) {
        std::map<int, std::string>::const_iterator it = g_reportTestServer.find(g_serverMode);
        
		if (it != g_reportTestServer.end()) {
            reportURL = it->second;
        } else if (SERVER_MODE_FIXED_IP_VALIDATE == g_serverMode || SERVER_MODE_FIXED_IP_PRIVATE_SERVICE == g_serverMode) {
            reportURL = g_serverIp;
        } else {
            reportURL = g_reportTestServer.find(SERVER_MODE_TEST)->second; // default
        }
    } else {
        reportURL = strAppKeySuffix;
        reportURL.append(".");

        reportURL.append(REPORT_SERVER_URL);
    }
    
    return reportURL;
}


/**
 * 获取上报服务器ipList
 */
YouMeErrorCode CSDKValidate::getReportIPList(std::string url, std::vector<XString> & ipList, bool bReconnect)
{
    ipList.clear();
    std::vector<std::string> ipStrList;
    TSK_DEBUG_INFO ("#### Parsing Report server:%s, isReconnect:%d", url.c_str(), bReconnect);
    if (bReconnect) {
        for (int i = 0; i < RECONNECT_DNS_RETRY_COUNT; i++) {
            ReportQuitData::getInstance()->m_dns_count++;
            m_dnsHandle->GetHostByNameAsync(url, ipStrList, DNS_PARSE_TIMEOUT);
            if (!ipStrList.empty()) {
                break;
            }
            // Wait for a while, the network may be ready. But if the user cancel it, return immediately.
            if (youmecommon::WaitResult_Timeout != m_condHandle.WaitTime(RECONNECT_DNS_WAIT_TIME)) {
                TSK_DEBUG_INFO("User interruption, stop report DNS parsing");
                return YOUME_ERROR_UNKNOWN;
            }
        }
    } else {
        ReportQuitData::getInstance()->m_dns_count++;
        m_dnsHandle->GetHostByNameAsync(url, ipStrList, DNS_PARSE_TIMEOUT);
    }

    int ipStrListLen = ipStrList.size();
    if (ipStrListLen > 0) {
        for (int i = 0; i < ipStrListLen; ++i) {
            ipList.push_back(LocalToXString(ipStrList[i]));
        }
    }    

    if (ipList.empty()) {
        TSK_DEBUG_ERROR ("####Parse Report DNS failed, use defaut ip instead.");
        if (g_serverMode != SERVER_MODE_FORMAL) {
            std::string backup_iplist = REPORT_BACKUP_IPLIST;
            ipList.push_back(LocalToXString(backup_iplist));
        } else {
            std::string backup_iplist = REPORT_BACKUP_IPLIST;
            ipList.push_back(LocalToXString(backup_iplist));
        }
    }
    
    return YOUME_SUCCESS;
}

/**
 * 获取验证服务器ip列表
 */
YouMeErrorCode CSDKValidate::getValidateIPList(std::string url, std::vector<XString> & ipList, bool bReconnect)
{
    // DNS parsing is first step accessing the network. For reconnecting, we should try more
    // time than normal, because the network may be unstable at the first several seconds.
    ipList.clear();
    std::vector<std::string> ipStrList;
    TSK_DEBUG_INFO ("#### Parsing SDKValidate server:%s, isReconnect:%d", url.c_str(), bReconnect);
    if (bReconnect) {
        for (int i = 0; i < RECONNECT_DNS_RETRY_COUNT; i++) {
            // check ip list
            ReportQuitData::getInstance()->m_dns_count++;
            m_dnsHandle->GetHostByNameAsync(url, ipStrList, DNS_PARSE_TIMEOUT);
            if (!ipStrList.empty()) {
                continue;
            }
            // Wait for a while, the network may be ready. But if the user cancel it, return immediately.
            if (youmecommon::WaitResult_Timeout != m_condHandle.WaitTime(RECONNECT_DNS_WAIT_TIME)) {
                TSK_DEBUG_INFO("User interruption, stop DNS parsing");
                return YOUME_ERROR_UNKNOWN;
            }
        }
    } else {
        ReportQuitData::getInstance()->m_dns_count++;
        m_dnsHandle->GetHostByNameAsync(url, ipStrList, DNS_PARSE_TIMEOUT);
    } 

    int ipStrListLen = ipStrList.size();
    if (ipStrListLen > 0) {
        for (int i = 0; i < ipStrListLen; ++i) {
            ipList.push_back(LocalToXString(ipStrList[i]));
        }
    } else {
        TSK_DEBUG_WARN("DNS parsing get zero result");
    }

    // Added the backup IPs to the IP list.
    if (SERVER_MODE_FORMAL == g_serverMode) {
        const std::string* tmpIPArray;
        int ipNum=0;
        switch (g_serverRegionId) {
            case RTC_CN_SERVER:
            case RTC_DEFAULT_SERVER:
                tmpIPArray = g_cnBackupSdkValidateIPs;
                ipNum = sizeof(g_cnBackupSdkValidateIPs)/sizeof(std::string);
                break;
            default:
                tmpIPArray = g_overseaBackupSdkValidateIPs;
                ipNum = sizeof(g_overseaBackupSdkValidateIPs)/sizeof(std::string);
                break;
        }
        
        for (int i = 0; i < ipNum; i++) {
            TSK_DEBUG_INFO("Adding backup IP:%s", tmpIPArray[i].c_str());
            ipList.push_back(LocalToXString(tmpIPArray[i]));
        }
    } else if (SERVER_MODE_TEST == g_serverMode) {
        const std::string* tmpIPArray;
        int ipNum=0;

        tmpIPArray = g_cnBackupSdkValidateIPsForTest;
        ipNum = sizeof(g_cnBackupSdkValidateIPsForTest)/sizeof(std::string);
        
        for (int i = 0; i < ipNum; i++) {
            TSK_DEBUG_INFO("Test env--Adding backup IP:%s", tmpIPArray[i].c_str());
            ipList.push_back(LocalToXString(tmpIPArray[i]));
        }
    }

    return YOUME_SUCCESS;
}

/** 上报服务初始化 */
void CSDKValidate::ReportServiceInit()
{
    std::string domain = getReportURL( m_strAppKeySuffix );
    std::string uuid = NgnApplication::getInstance()->getUUID();
    std::string imei = NgnApplication::getInstance()->getDeviceIMEI();
    
    XString strDomain = UTF8TOXString( domain );
    
    TSK_DEBUG_INFO("####Report service init domain[%s], tcp[%d], udp[%d], uuid[%s] imei[%s].", domain.c_str(), REPORT_TCPPORT, REPORT_UDPPORT, uuid.c_str(), imei.c_str());
    if(uuid.empty()) {
        youmeRTC::ReportParam::setParam(imei, REPORT_TCPPORT, REPORT_UDPPORT, strDomain);
    } else {
        youmeRTC::ReportParam::setParam(uuid, REPORT_TCPPORT, REPORT_UDPPORT, strDomain);
    }
}

//设置三个参数
void CSDKValidate::SetPackageName (const std::string &strPackageName)
{
    m_strPackageName = strPackageName;
}
void CSDKValidate::SetPlatform (SDKValidate_Platform platform)
{
    m_devicePlatform = platform;
}
void CSDKValidate::SetAppKey (const std::string &strAppKey)
{
    m_strAppKey = strAppKey;
}

void CSDKValidate::SetAppKeySuffix( const std::string& strAppKeySuffix )
{
    m_strAppKeySuffix = strAppKeySuffix;
}
bool CSDKValidate::SetAppSecret (const std::string &strAppSecret)
{
    m_strAppSecret = strAppSecret;
    return ParseAppSecret ();
}
//将appkey 解析成public 和module
bool CSDKValidate::ParseAppSecret ()
{
    char *pDecode = NULL;
    bool bParseSuccess = false;
    do
    {
        int iBufferLen = (int)tsk_base64_decode ((const uint8_t *)m_strAppSecret.c_str (), m_strAppSecret.length (), &pDecode);
        if (131 != iBufferLen)
        {
            TSK_DEBUG_ERROR ("Failed to decode base64:%s length:%d", m_strAppKey.c_str (),iBufferLen);
            break;
        }
        youmecommon::CXSharedArray<unsigned char> publicKey;
        youmecommon::CXSharedArray<unsigned char>  module;

        //模数
        module.Allocate(128);
        memcpy(module.Get(), pDecode, 128);
   
        // publickey
        publicKey.Allocate(3);
        memcpy(publicKey.Get(), pDecode+128, 3);
  
        bParseSuccess = m_rsa.SetPublicKey (publicKey, module);
    } while (0);

    tsk_free ((void **)&pDecode);
    return bParseSuccess;
}

std::map<std::string, youmecommon::CXAny> CSDKValidate::getConfigurations ()
{
    /*
    m_configurations["GENERAL_AEC"] = true;
    m_configurations["GENERAL_ECHO_TAIL"] = 100;
    m_configurations["GENERAL_NR"] = true;
    m_configurations["GENERAL_AGC"] = false;
    m_configurations["GENERAL_VAD"] = true;
    m_configurations["GENERAL_CNG"] = false;
    m_configurations["GENERAL_SPEAKERON"] = 1;
    m_configurations["GENERAL_JITBUFFER_MARGIN"] = 200;
    m_configurations["OPUS_COMPLEXITY"] = 5;
    m_configurations["OPUS_ENABLE_VBR"] = 1;
    m_configurations["OPUS_MAX_BANDWIDTH"] = 1105;
    m_configurations["OPUS_ENABLE_INBANDFEC"] = 0;
    m_configurations["OPUS_ENABLE_OUTBANDFEC"] = 0;
    m_configurations["OPUS_DTX_PEROID"] = 0;
    m_configurations["DATAREPORT_ADDR"] = std::string("123.59.62.126");
    m_configurations["DATAREPORT_PORT"] = 5574;
    m_configurations["HEART_TIMEOUT"] = 5;
    m_configurations["HEART_TIMEOUT_COUNT"] = 4;
    m_configurations["RECONNECT_COUNT"] = 5;
*/
    
    
    return m_configurations;
}

void CSDKValidate::Destroy ()
{
    delete s_signle;
    s_signle = NULL;
}
void CSDKValidate::Reset ()
{
    if (!m_bInit)
    {
        return;
    }
    m_isAborting = false;
    m_condHandle.Reset();
    m_validateTcp.Reset();
    m_getRedirectTcp.Reset();
}

void CSDKValidate::Abort ()
{
    if (!m_bInit)
    {
        return;
    }
    m_isAborting = true;
    m_condHandle.SetSignal();
    m_validateTcp.Abort();
    m_getRedirectTcp.Abort();
}
