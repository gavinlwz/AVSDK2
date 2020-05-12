//
//  ReportService.cpp
//  youme_voice_engine
//
//  Created by fire on 16/10/25.
//  Copyright © 2016年 Youme. All rights reserved.
//
//  数据上报服务
//

#include "NgnApplication.h"
#include "YouMeVoiceEngine.h"
#include "ReportService.h"
#include "NgnConfigurationEntry.h"
#include "tsk_debug.h"
#include "AVStatistic.h"


/** report param */
std::string youmeRTC::ReportParam::m_strIdentify;
unsigned int youmeRTC::ReportParam::m_uiTcpPort;
unsigned int youmeRTC::ReportParam::m_uiUdpPort;
XString youmeRTC::ReportParam::m_strDomain;

/** init */
ReportService * ReportService::mService = NULL;
std::mutex* ReportService::m_InitMutex = new std::mutex;
std::mutex* ReportService::m_waitListMutex = new std::mutex;

ReportService::ReportService() {
}

ReportService::~ReportService() {
    if (pDataChannel != NULL) {
        CYouMeDataChannel::DestroyInstance(pDataChannel);
        pDataChannel = NULL;
    }
}

bool ReportService::initDataChannel(){
    if( !pDataChannel ){
        std::string report_db = NgnApplication::getInstance()->getDocumentPath() + "/reportnew.db";
        pDataChannel = CYouMeDataChannel::CreateInstance(LocalToXString(report_db));
    }
    
    /** Init */
    CSDKValidate * pSDK = CSDKValidate::GetInstance();
    pSDK->ReportServiceInit();
    
    return true;
}

bool ReportService::init()
{
    if ( m_bInited == false) {
        int reportMode = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::DATAREPORT_MODE, NgnConfigurationEntry::DEFAULT_DATAREPORT_MODE);
        if (reportMode == 2 || reportMode == 3 ) {
            m_bReportServiceOn = true;
        }
        else {
            m_bReportServiceOn = false;
            return true;
        }
    }
    
    m_bInited = true;
    
    if( !pDataChannel )
    {
        return true ;
    }
    if( !m_bReportServiceOn ){
        return true;
    }
    
    std::lock_guard<std::mutex> lock(*m_waitListMutex);
    if( m_listWait.size() > 0 )
    {
        for( auto it = m_listWait.begin(); it != m_listWait.end(); ++it )
        {
            auto data = *it;
            
            data->Report();
        }
        
        m_listWait.clear();
    }
    
    return true;
}

ReportService * ReportService::getInstance() {
    std::lock_guard<std::mutex> lock(*m_InitMutex);
    if (mService == NULL) {
        mService = new ReportService();
    }
    
    return mService;
}

void ReportService::destroy() {
    std::lock_guard<std::mutex> lock(*m_InitMutex);
    if (mService != NULL) {
        delete mService;
    }
    
    mService = NULL;
}

/**
 * 数据上报
 */
void ReportService::report(youmeRTC::ReportMessage & message, bool bForceReport ) {
    if (pDataChannel == NULL) {
        return;
    }
    
    if (!m_bReportServiceOn) { /* 服务器配置关闭，不上行了 */
        return;
    }
    
    //如果还没初始化完成，先临时保存一下，等初始化完成再上报
    std::shared_ptr< CDataReport<youmeRTC::ReportMessage, youmeRTC::ReportParam> >  data(new CDataReport<youmeRTC::ReportMessage, youmeRTC::ReportParam>(pDataChannel, message.cmdid, message.version, message.bUseTcp ));
    TSK_DEBUG_INFO("Report to ip[%s], cmd[%d], version[%d], tcp[%d].", XStringToLocal(youmeRTC::ReportParam::m_strDomain).c_str(), message.cmdid, message.version, message.bUseTcp);
    
    /** common data */
    message.appkey = NgnApplication::getInstance()->getAppKey().c_str();
    message.userid = CYouMeVoiceEngine::getInstance()->GetJoinUserName();
    
    data->SetBody(message);
    
    ReportQuitData::getInstance()->m_uploadlog_count++;

    //
    if( m_bInited || bForceReport )
    {
        data->Report();
    }
    else
    {
        std::lock_guard<std::mutex> lock(*m_waitListMutex);
        //如果还没初始化完成，且不是强制上报的消息先临时保存一下，等初始化完成再上报
        //不用保留很多吧，最多保留10条
        if( m_listWait.size() >= 10 )
        {
            m_listWait.pop_front();
        }

        m_listWait.push_back( data );
        return ;
    }

   
}

