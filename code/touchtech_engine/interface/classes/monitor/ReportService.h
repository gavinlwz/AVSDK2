//
//  ReportService.h
//  youme_voice_engine
//
//  Created by fire on 16/10/25.
//  Copyright © 2016年 Youme. All rights reserved.
//
//  数据上报服务
//

#pragma once
#include "ReportMessageDef.h"
#include "version.h"
#include <list>


enum {
	REPORT_COMMON_INIT = 0,		/* 初始化 */
	REPORT_COMMON_UNINIT = 1,	/* 反初始化 */
};


enum {
	REPORT_CHANNEL_JOIN = 0,	/* 加入会议 */
	REPORT_CHANNEL_LEAVE,		/* 离开会议 */
	REPORT_CHANNEL_RECONNECT,	/* 重连 */
	REPORT_CHANNEL_LOSTHEART,	/* 心跳丢失 */
	REPORT_CHANNEL_KICKOUT		/* 被踢出房间 */
};

enum {
    BG_OP_START = 0 ,       //打开背景音乐
    BG_OP_PAUSE,            //暂停
    BG_OP_RESUME,           //恢复
    BG_OP_STOP,             //停止
    
};

enum {
    REPORT_VIDEO_OPEN_CAMERA = 0,	/* 打开摄像头 */
    REPORT_VIDEO_CLOSE_CAMERA,		/* 关闭摄像头 */
    REPORT_VIDEO_BLOCK_OTHERS,	    /* 屏蔽他人 */
    REPORT_VIDEO_START_RENDER,	    /* 开始渲染 */
    REPORT_VIDEO_STOP_RENDER,	    /* 停止渲染 */
    REPORT_VIDEO_NORMAL,            /* 视频正常 */
    REPORT_VIDEO_SHUTDOWN,          /* 视频停止 */
    REPORT_VIDEO_DATA_ERROR,        /* 数据错误 */
    REPORT_VIDEO_NETWORK_BAD,       /* 网络错误 */
    REPORT_VIDEO_BLACK_FULL,        /* 全屏黑色 */
    REPORT_VIDEO_GREEN_FULL,        /* 全屏绿色 */
    REPORT_VIDEO_BLACK_BORDER,      /* 边框黑色 */
    REPORT_VIDEO_GREEN_BORDER,      /* 边框绿色 */
    REPORT_VIDEO_BLURRED_SCREEN,    /* 尴尬花屏 */
    REPORT_VIDEO_ENCODER_ERROR,     /* 编码错误 */
    REPORT_VIDEO_DECODER_ERROR,     /* 解码错误 */
    REPORT_VIDEO_SELF_LEAVE_ROOM,   /* 自己离开房间 */
};

/******
 * 数据上报服务
 */
class ReportService {
private:
	ReportService();
	~ReportService();

protected:
	CYouMeDataChannel * pDataChannel = NULL;
	static ReportService * mService;

private:
    static std::mutex* m_InitMutex;
	bool m_bReportServiceOn = true; /** 数据上报是否打开（新的） */
    
    bool m_bInited = false ;

    static std::mutex* m_waitListMutex;
    //初始化完成之前就有数据要发送，没办法，先保存起来吧
    std::list< std::shared_ptr< CDataReport<youmeRTC::ReportMessage, youmeRTC::ReportParam> > > m_listWait;
public:
	static ReportService * getInstance();
	void destroy();
    bool initDataChannel();
    bool init();
    
    //只有sdk验证失败了要强制上报
	void report(youmeRTC::ReportMessage & message , bool bForceReport = false );
    
};
