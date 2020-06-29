//
//  MonitoringCenter.h
//  YouMeVoiceEngine
//
//  Created by 杜凯 on 15/9/28.
//  Copyright (c) 2015年 youme. All rights reserved.
//

#ifndef __YouMeVoiceEngine__MonitoringCenter__
#define __YouMeVoiceEngine__MonitoringCenter__

#include <stdio.h>
#include "tsk_thread.h"

//using namespace std;
//using namespace YouMeProtocol;

#include "tsk_debug.h"
#include "YouMeCommon/XSharedArray.h"
#include <map>
#include <vector>
#include <mutex>
#include "YouMeCommon/XSemaphore.h"
#include "SyncTCPTalk.h"
#include "YouMeCommon/SqliteOperator.h"
#include <future>

enum UploadType
{
    UploadType_SDKValidFail=1,
    UploadType_RedirectFail=2,
    UploadType_JoinRoomFail=4,
    UploadType_HeartFail=8,
	UploadType_Notify = 16,
};


/************************************************************************/
/*    保存了手机的照片缩略图的format信息                                                                     */
/************************************************************************/
const std::string s_szTableName[] ={
    "report"
    }; //一对多的配置


const std::string CREATE_TABLE_SINGLECONFIG = \
    "create table report (id int, \
     value  blob);";



const std::string szCreateTableSQL[] = {
    CREATE_TABLE_SINGLECONFIG
};




class MonitoringCenter
{
public:

    static MonitoringCenter *getInstance ();
    static void destroy();
    
    
    bool Init();
    void UnInit();
    
    
    bool Report(const char* buffer,int iBufferLen);
    //触发日志上传
    void UploadLog(UploadType type,int iErrorcodeCode, bool needVideoSnapshot);
    
    //提供这个玩意给sdk 验证调用
    void NotifyDataReportParseDNS();
    static bool s_bIsInit;

    void setSdkConfigAvail();

private:
    void ReportProc();
    void UploadLogProc(UploadType type,int iErrorcode, bool needVideoSnapshot);
    MonitoringCenter();
    ~MonitoringCenter();
    std::map<int, youmecommon::CXSharedArray<byte> >m_needReportList;
     static MonitoringCenter *sInstance;
    std::thread m_reportFuture;
    //启动一个线程用来上报日志,初始化成功之后才能上传，上传完成之后线程退出，只有再下一次启动的时候再上报
    std::thread m_uploadThrad;
    youmecommon::CXSemaphore m_wait;
    std::mutex m_mutex;
    bool m_bExit;
	bool m_bReportServiceOn = true; /** 数据上报是否打开（老的） */

    int m_iMaxID;
    
    youmecommon::CSqliteDb m_SqliteDb;
    //只需要初始化一次
    bool m_bUploadComplete=true;
    std::vector<XString> m_logReportIPVec;
    //数据上报的IP地址
	std::vector<XString> m_dataReportIPVec;
    //数据上报的域名是在SDK 验证之后才有的。所以SDK 验证之后，需要通知一下这里重新解析一遍
    bool m_bParseDNS;
    bool m_bSdkConfigAvail = false;
    youmertc::CSyncTCP m_reportTcp;
  };



#endif /* defined(__YouMeVoiceEngine__MonitoringCenter__) */
