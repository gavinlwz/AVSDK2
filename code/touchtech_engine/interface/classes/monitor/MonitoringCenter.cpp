//
//  MonitoringCenter.cpp
//  YouMeVoiceEngine
//
//  Created by 杜凯 on 15/9/26.
//  Copyright (c) 2015年 youme. All rights reserved.
//

#include "MonitoringCenter.h"
#include "NgnConfigurationEntry.h"
#include "NgnMemoryConfiguration.hpp"
#include "YoumeRunningState.pb.h"
#include "NgnApplication.h"
#include "YouMeVoiceEngine.h"
#include <assert.h>
#include "minizip/MiniZip.h"
#include "NgnApplication.h"
#include "uploadlog.pb.h"
#include "ProtocolBufferHelp.h"
#include <stdio.h>
#include "XDNSParse.h"
#include <vector>
#include "version.h"
#include "XOsWrapper.h"
#ifdef ANDROID
#include "AudioMgr.h"
extern void SaveLogcat(const std::string& strPath);
#endif

#include "ReportService.h"
#include "ReportMessageDef.h"
#include "AVStatistic.h"
//美服
#define LOG_REPORT_URL_US "us.rtclog.youme.im"
#define LOG_REPORT_URL_US_IP "107.150.100.5"
//国服
#define LOG_REPORT_URL "rtclog.youme.im"
#define LOG_REPORT_URL_IP "123.59.75.232"
//亚太
#define LOG_REPORT_URL_HK "hk.rtclog.youme.im"
#define LOG_REPORT_URL_HK_IP "103.218.243.124"

extern int g_serverMode;
extern std::string g_serverIp;
extern int g_serverPort;
extern YOUME_RTC_SERVER_REGION g_serverRegionId;

/*
 ********************MonitoringCenter************************
 */

MonitoringCenter *MonitoringCenter::sInstance = NULL;
bool MonitoringCenter::s_bIsInit = false;
MonitoringCenter *MonitoringCenter::getInstance ()
{
    if (NULL == sInstance)
    {
        sInstance = new MonitoringCenter ();
    }
    return sInstance;
}

void MonitoringCenter::destroy ()
{
    delete sInstance;
    sInstance = NULL;
}

MonitoringCenter::MonitoringCenter()
{
    m_bParseDNS = true;
}


MonitoringCenter::~MonitoringCenter()
{
    UnInit();
}

bool MonitoringCenter::Init()
{
    //判断是否需要上传，如果需要上传的话，在打开日志之前压缩一下
    if (s_bIsInit) {
        return true;
    }
    lock_guard<std::mutex> lock(m_mutex);

	/** Set Mode */
    //不再需要老上报了
	m_bReportServiceOn = false ;
    s_bIsInit = true;
    return true ;

    m_bExit = false;
    m_reportTcp.Reset();
    m_needReportList.clear();
    std::string strReportDBPaht = NgnApplication::getInstance ()->getDocumentPath () + "/report.db";
    m_SqliteDb.Open(LocalToXString(strReportDBPaht).c_str());
    
    for (int i = 0 ; i < sizeof(s_szTableName) / sizeof(s_szTableName[0]);i++)
    {
		if (!m_SqliteDb.IsTableExist(LocalToXString(s_szTableName[i]).c_str()))
        {
            youmecommon::CSqliteOperator	sqliteOperator(m_SqliteDb);
			sqliteOperator.PrepareSQL(LocalToXString(szCreateTableSQL[i]).c_str());
            sqliteOperator.Execute();
        }
    }
    m_iMaxID = 1;
    //读取表中的数据
    youmecommon::CSqliteOperator	sqliteOperator(m_SqliteDb);
    std::string strSql = "select * from report";
	sqliteOperator.PrepareSQL(LocalToXString(strSql));
    sqliteOperator.Execute();
    while (sqliteOperator.Next())
    {
        int iTmpID=0;
        youmecommon::CXSharedArray<byte> buffer;
        sqliteOperator >> iTmpID >>buffer;
        if (iTmpID > m_iMaxID) {
            m_iMaxID = iTmpID;
        }
        m_needReportList[iTmpID] = buffer;
        m_wait.Increment();
    }
    
    m_reportFuture = std::thread(&MonitoringCenter::ReportProc, this);
    s_bIsInit = true;
    return true;
}

//暂时先不存db
void MonitoringCenter::UnInit()
{
    m_bExit = true;
    lock_guard<std::mutex> lock(m_mutex);
    TSK_DEBUG_INFO ("Enter");
    m_reportTcp.Abort();
    if (m_reportFuture.joinable()) {
        if (m_reportFuture.get_id() == std::this_thread::get_id()) {
            m_reportFuture.detach();
        }
        else
        {
            m_reportFuture.join();

        }
    }
    if (m_uploadThrad.joinable()) {
        if (m_uploadThrad.get_id() == std::this_thread::get_id()) {
            m_uploadThrad.detach();
        }
        else
        {
            m_uploadThrad.join();
        }
    }
    {
        lock_guard<std::mutex> lock(m_mutex);
        m_needReportList.clear();
    }
    TSK_DEBUG_INFO ("Leave");
}

bool MonitoringCenter::Report(const char* buffer,int iBufferLen)
{
	if (!m_bReportServiceOn) { /** 服务器配置关闭了老的数据上报  */
		return true;
	}

    TSK_DEBUG_INFO ("Enter");
    youmecommon::CXSharedArray<byte> pPtr(iBufferLen);
    memcpy(pPtr.Get(), buffer, iBufferLen);
    {
        lock_guard<std::mutex> lock(m_mutex);
        m_iMaxID++;
        m_needReportList[m_iMaxID] = pPtr;
        
        youmecommon::CSqliteOperator	sqliteOperator(m_SqliteDb);
        std::string strSql = "insert into report values(?1,?2)";
		sqliteOperator.PrepareSQL(LocalToXString(strSql));
        sqliteOperator << m_iMaxID;
        sqliteOperator << pPtr;
        sqliteOperator.Execute();
    }
    
    m_wait.Increment();
    return true;
}
void MonitoringCenter::UploadLogProc(UploadType type,int iErrorcode, bool needVideoSnapshot)
{
     TSK_DEBUG_INFO("Enter");
    //先进行域名解析
    if (0 == m_logReportIPVec.size()) {
        youmecommon::CXDNSParse parese;
        std::string strReportURL;
        switch (g_serverRegionId) {
            case RTC_CN_SERVER:
                strReportURL =LOG_REPORT_URL;
                break;
            case RTC_US_SERVER:
                strReportURL =LOG_REPORT_URL_US;
                break;
            case RTC_HK_SERVER:
                strReportURL =LOG_REPORT_URL_HK;
                break;
            default:
                strReportURL =LOG_REPORT_URL;
                break;
        }
        
        uint64_t ulStart = tsk_time_now();
        
        ReportQuitData::getInstance()->m_dns_count++;
        parese.ParseDomain2(LocalToXString(strReportURL), m_logReportIPVec);
        if (m_logReportIPVec.empty()) {
            std::string report_url_ip;
            switch (g_serverRegionId) {
                case RTC_CN_SERVER:
                    report_url_ip = LOG_REPORT_URL_IP;
                    break;
                case RTC_US_SERVER:
                    report_url_ip = LOG_REPORT_URL_US_IP;
                    break;
                case RTC_HK_SERVER:
                    report_url_ip = LOG_REPORT_URL_HK_IP;
                    break;
                default:
                    report_url_ip = LOG_REPORT_URL_IP;
                    break;
            }

			m_logReportIPVec.push_back(LocalToXString(report_url_ip));
        }
        
        ReportService * report = ReportService::getInstance();
        youmeRTC::ReportDNSParse dns;
        dns.parse_addr = strReportURL;
        dns.parse_usetime = (tsk_time_now() - ulStart);
        dns.parse_result = 0;
        for (int i = 0; i < m_logReportIPVec.size(); i++) {
            dns.parse_iplist.append( XStringToUTF8(m_logReportIPVec[i]) ).append(";");  // ip列表
        }
        dns.sdk_version = SDK_NUMBER;
        dns.platform = NgnApplication::getInstance()->getPlatform();
        dns.canal_id = NgnApplication::getInstance()->getCanalID();
        report->report( dns );
    }
    //去取第一个上报地址
	std::string strUploadIp = XStringToLocal(m_logReportIPVec[0]);
    int iUploadPort = 6008;
    
    bool bSuccess = false;
    FILE* fLog = NULL;
    std::string strZip =  NgnApplication::getInstance ()->getZipLogPath();
    do {
        youmecommon::CMiniZip logZip;
        if (!logZip.Open(LocalToXString(strZip)))
        {
            TSK_DEBUG_ERROR("Failed open zip file:%s",strZip.c_str());
            break;
        }
        TSK_DEBUG_INFO("Opening zip file:%s",strZip.c_str());
        //添加现在的文件到
		logZip.AddOneFileToZip(LocalToXString(NgnApplication::getInstance()->getLogPath()));
        TSK_DEBUG_INFO("Adding the current log to zip:%s",NgnApplication::getInstance()->getLogPath().c_str());
        //添加已经存在的备份文件
		logZip.AddOneFileToZip(LocalToXString(NgnApplication::getInstance()->getBackupLogPath()));
        TSK_DEBUG_INFO("Adding the backup log to zip:%s",NgnApplication::getInstance()->getBackupLogPath().c_str());
        //添加已经存在的用户自定义文件
        if (!NgnApplication::getInstance()->getUserLogPath().empty())
        {
            logZip.AddOneFileToZip(LocalToXString(NgnApplication::getInstance()->getUserLogPath()));
            TSK_DEBUG_INFO("Adding the user log to zip:%s",NgnApplication::getInstance()->getUserLogPath().c_str());
        }
        if (needVideoSnapshot)
        {
            //添加已经存在的视频快照文件
            logZip.AddOneFileToZip(LocalToXString(NgnApplication::getInstance()->getVideoSnapshotPath()));
            TSK_DEBUG_INFO("Adding the video snapshot to zip:%s",NgnApplication::getInstance()->getVideoSnapshotPath().c_str());
        }
#ifdef ANDROID
        //上传logcat
        SaveLogcat ("/sdcard/youme_logcat.txt");
        logZip.AddOneFileToZip("/sdcard/youme_logcat.txt");
        remove("/sdcard/youme_logcat.txt");
         TSK_DEBUG_INFO("Adding logcat done");
#endif

        
        
        logZip.Close();
        //打开压缩文件
        fLog = fopen(strZip.c_str(), "rb");
        if (NULL == fLog) {
            TSK_DEBUG_ERROR("Failed to open zip file:%s, error:%d",strZip.c_str(),errno);
            break;
        }
        //获取文件长度
        XFSeek(fLog,0,SEEK_END);
		long long iFileSize = XFTell(fLog);
		XFSeek(fLog, 0, SEEK_SET);
        youmertc::CSyncTCP tcp;
        if (!tcp.Init(strUploadIp, iUploadPort)) {
            TSK_DEBUG_ERROR("Failed to create socket for log server:%s:%d",strUploadIp.c_str(),iUploadPort);
            break;
        }
        if (!tcp.Connect(3000)) {
            TSK_DEBUG_ERROR("Failed to connect to the log server:%s:%d",strUploadIp.c_str(),iUploadPort);
            break;
        }
          
        //构造protobuf 数据
        YouMeProtocol::UploadLog uploadProtocol;
        uploadProtocol.set_allocated_head (CProtocolBufferHelp::CreatePacketHead (YouMeProtocol::MSG_UploadLog));
        uploadProtocol.set_uptype((YouMeProtocol::UploadType)type);
        uploadProtocol.set_errorcode(iErrorcode);
        uploadProtocol.set_filelen((int)iFileSize);
		uploadProtocol.set_userid(CYouMeVoiceEngine::getInstance()->GetJoinUserName());
        std::string strUploadData;
        uploadProtocol.SerializeToString(&strUploadData);
        
        //发送protobuf数据
        tcp.SendData(strUploadData.c_str(), strUploadData.length());
        //发送文件内容
        youmecommon::CXSharedArray<char> pFileBuffer(64*1024);
        while (true) {
            int iReadLen = fread(pFileBuffer.Get(), 1, 64*1024, fLog);
            if (iReadLen <= 0) {
                break;
            }
            tcp.SendBufferData(pFileBuffer.Get(), iReadLen);
        }
        youmecommon::CXSharedArray<char> pRecvData(1);
        tcp.RecvDataByLen(1, pRecvData);
        //到这一步就认为发送成功了
        bSuccess = true;
    } while (0);
    if (NULL != fLog) {
        fclose(fLog);
    }
    remove(strZip.c_str());
    if (bSuccess) {
        tsk_uninit_log();
        if (SERVER_MODE_FORMAL == g_serverMode) {
            remove(NgnApplication::getInstance()->getLogPath().c_str());
        }
        
        remove(NgnApplication::getInstance()->getBackupLogPath().c_str());

        if (NgnApplication::getInstance()->getUserLogPath().empty()) {
            tsk_init_log(NgnApplication::getInstance()->getLogPath().c_str(), NgnApplication::getInstance()->getBackupLogPath().c_str());
        }else {
            tsk_init_log(NgnApplication::getInstance()->getUserLogPath().c_str(), NULL);
        }
	}
    m_bUploadComplete = true;
    TSK_DEBUG_INFO("Leave");
}


void MonitoringCenter::NotifyDataReportParseDNS()
{
    m_bParseDNS = true;
}
void MonitoringCenter::ReportProc()
{
    bool bFirstReport = true;
    while (true) {
        m_wait.Decrement();
        if (m_bExit) {
            break;
        }
        if (m_needReportList.empty()) {
            continue;
        }
        youmecommon::CXSharedArray<byte> pBuffer;
        int iReportID = 0;
        {
            lock_guard<std::mutex> lock(m_mutex);
            std::map<int, youmecommon::CXSharedArray<byte> >::iterator begin = m_needReportList.begin();
            pBuffer = begin->second;
            iReportID = begin->first;
            m_needReportList.erase(begin);
        }
        if (0 ==pBuffer.GetBufferLen())
        {
            continue;
        }
        
        if (m_bParseDNS) {
            m_dataReportIPVec.clear();
            m_bParseDNS = false;
        }
        
        if (0 == m_dataReportIPVec.size()) {
            TSK_DEBUG_INFO ("DataReport waits for SDK config to be available");
            for (int i = 0;  i < 50; i++) {
                if (m_bSdkConfigAvail) {
                    break;
                }
                XSleep(100);
            }
            youmecommon::CXDNSParse parese;
            std::string reportAddr = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::DATAREPORT_ADDR, NgnConfigurationEntry::DEFAULT_DATAREPORT_ADDR);
            // If the data report addr has not been configured(that is, we are going to use the default one), do not
            // perform a DNS parsing, instead, take the default addr directly.
            if (reportAddr.compare(NgnConfigurationEntry::DEFAULT_DATAREPORT_ADDR) != 0) {
                TSK_DEBUG_INFO ("#### Parsing DataReport server:%s", reportAddr.c_str());
                
                uint64_t ulStart = tsk_time_now();
                parese.ParseDomain2(LocalToXString(reportAddr), m_dataReportIPVec);
                ReportQuitData::getInstance()->m_dns_count++;
                
                ReportService * report = ReportService::getInstance();
                youmeRTC::ReportDNSParse dns;
                dns.parse_addr = reportAddr;
                dns.parse_usetime = (tsk_time_now() - ulStart);
                dns.parse_result = 0;
                for (int i = 0; i < m_dataReportIPVec.size(); i++) {
                    dns.parse_iplist.append( XStringToUTF8(m_dataReportIPVec[i]) ).append(";");  // ip列表
                }
                dns.sdk_version = SDK_NUMBER;
                dns.platform = NgnApplication::getInstance()->getPlatform();
                dns.canal_id = NgnApplication::getInstance()->getCanalID();
                report->report( dns );

            }
            if (m_dataReportIPVec.empty()) {
                if ((RTC_CN_SERVER == g_serverRegionId) || (RTC_DEFAULT_SERVER == g_serverRegionId)) {
					reportAddr = "123.59.62.126";
                } else {
					reportAddr = "47.89.13.3";
                }
				m_dataReportIPVec.push_back(LocalToXString(reportAddr));
                TSK_DEBUG_INFO ("#### DataReport using default server:%s", reportAddr.c_str());
			}
        }
        
        std::string strDataReportAddr = XStringToLocal(m_dataReportIPVec[0]);
        
        m_reportTcp.UnInit();
        if (!m_reportTcp.Init(strDataReportAddr, CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::DATAREPORT_PORT, NgnConfigurationEntry::DEFAULT_DATAREPORT_PORT), 60))
        {
            //这里是不会失败的，如果真失败了，就等下一次上报
            m_reportTcp.UnInit();
            continue;
        }
        if (!m_reportTcp.Connect(60))
        {
            //这里是不会失败的，如果真失败了，就等下一次上报
            TSK_DEBUG_WARN("Failed to connect the DataReport server:%s:%d",CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::DATAREPORT_ADDR, NgnConfigurationEntry::DEFAULT_DATAREPORT_ADDR).c_str(),errno);
            m_reportTcp.UnInit();
            continue;
        }
       
        int iSendLen = m_reportTcp.SendData((const char *)pBuffer.Get(), pBuffer.GetBufferLen());
        if (iSendLen != pBuffer.GetBufferLen()) {
            TSK_DEBUG_WARN("DataReport error, wrong length");
            m_reportTcp.UnInit();
            continue;
        }
        
        youmecommon::CXSharedArray<char> recvBuf;
        int iRecvBuf = m_reportTcp.RecvData(recvBuf);
        if (iRecvBuf <=0) {
            TSK_DEBUG_WARN("DataReport error, wrong length:%d",iRecvBuf);
            //后面需要保存到数据库的操作了
        }
        else
        {
            YouMeProtocol::DataReport_Response response;
            response.ParseFromArray(recvBuf.Get(), iRecvBuf);
            //后面是别的动作了
            if (response.code() == 0) {
                if (bFirstReport) {
                    TSK_DEBUG_INFO("DataReport first report success");
                    bFirstReport = false;
                }

                //成功，从数据库删除记录
                lock_guard<std::mutex> lock(m_mutex);
                youmecommon::CSqliteOperator	sqliteOperator(m_SqliteDb);
                std::string strSql = "delete from report where id=?1";
				sqliteOperator.PrepareSQL(LocalToXString(strSql));
                sqliteOperator << iReportID;
                sqliteOperator.Execute();
            } else {
                TSK_DEBUG_INFO("DataReport returns error:%d",response.code());
            }
        }
        m_reportTcp.UnInit();
    }
    TSK_DEBUG_INFO("DataReport thread exit");
}

void MonitoringCenter::UploadLog(UploadType type,int iErrorcodeCode, bool needVideoSnapshot)
{
    //正在上传
    if (!m_bUploadComplete) {
        return;
    }
   
    if (m_uploadThrad.joinable()) {
        m_uploadThrad.join();
    }
    
    
    m_bUploadComplete = false;
    m_uploadThrad = std::thread(&MonitoringCenter::UploadLogProc, this,type,iErrorcodeCode, needVideoSnapshot);
}

/*
 * 通知SDK配置已经拉取到了（或者拉取失败了）
 */
void MonitoringCenter::setSdkConfigAvail()
{
    m_bReportServiceOn = false;

    m_bSdkConfigAvail = true;
}

