#pragma once
#include <string>
#include "YouMeCommon/DataReport.h"

/**
 * ReportService.h
 * Created by python on 2017/11/02.
 * Copyright © 2017年 Youme. All rights reserved.
 * 数据上报服务
 * 数据上报命令字和版本
 * 命令字和版本的值须先从服务器配置后取得，请勿随意修改！
 * 配置地址：http://t.ops.nxit.us/home
 */

/**
 * 数据上报参数
 */
namespace youmeRTC {
    class ReportParam {
    public:
        ReportParam() { }
        ~ReportParam() { }
        
    public:
        static std::string m_strIdentify;
        static unsigned int m_uiTcpPort;
        static unsigned int m_uiUdpPort;
        static XString m_strDomain;
        
    public:
        static void setParam(std::string identify, unsigned int tcpport, unsigned int uudpport, XString domain) {
            ReportParam::m_strIdentify = identify;
            ReportParam::m_uiTcpPort= tcpport;
            ReportParam::m_uiUdpPort = uudpport;
            ReportParam::m_strDomain = domain;
        }
        
        static void setIdentify(std::string identify) {
            ReportParam::m_strIdentify = identify;
        }
        
        static void setTcpPort(unsigned int tcpport) {
            ReportParam::m_uiTcpPort= tcpport;
        }
        
        static void setUdpPort(unsigned int uudpport) {
            ReportParam::m_uiUdpPort = uudpport;
        }
        
        static void setDomain(XString domain) {
            ReportParam::m_strDomain = domain;
        }
    };
    
    /**
     * 通用消息头
     */
    class ReportMessage {
    public:
        ReportMessage() { }
        ~ReportMessage() { }
        
    public:
        unsigned short cmdid = 0;   /* 命令字     */
        unsigned short version = 1; /* 版本       */
        bool bUseTcp = true;        /* 是否使用tcp*/
        
    public:  /* RTC上报公共字段 */
        std::string  appkey;
        std::string userid;
        
    public:
        virtual void LoadToRecord(youmecommon::CRecord & record) const { };
    };
    
    
    /**
     * 业务逻辑上报，客户端使用时候主动触发
     * TCP
     */
    
    /********************************************************************************/
    /* [CMD:1000] [VERSION:1] */
    class ReportCommon : public ReportMessage {
    public:
        ReportCommon()
        {
            cmdid = 1000;
            version = 1;
            bUseTcp = true;
        }
        
        ~ReportCommon() { }
        
    public:
        unsigned int  common_type=0;  /* 通用类型 */
        unsigned int  result=0;  /* 操作结果 */
        std::string  param="";  /* 额外参数 */
        unsigned int  sdk_version=0;  /* SDK版本 */
        unsigned int  platform=0;  /* 平台 */
        std::string  brand="";  /* 品牌 */
        std::string  model="";  /* 机型 */
        std::string  canal_id="";  /* 渠道 */
        std::string  package_name="";  /* app包名 */
        
        
    public:
        void LoadToRecord(youmecommon::CRecord & record) const
        {
            record.SetData(appkey.c_str());
            record.SetData(userid.c_str());
            record.SetData(common_type);
            record.SetData(result);
            record.SetData(param.c_str());
            record.SetData(sdk_version);
            record.SetData(platform);
            record.SetData(brand.c_str());
            record.SetData(model.c_str());
            record.SetData(canal_id.c_str());
            record.SetData(package_name.c_str());
            
        }
    };
    
    /**
     * 业务逻辑上报，客户端使用时候主动触发
     * TCP
     */
    
    /********************************************************************************/
    /* [CMD:1001] [VERSION:1] */
    class ReportQuit : public ReportMessage {
    public:
        ReportQuit()
        {
            cmdid = 1001;
            version = 1;
            bUseTcp = true;
        }
        
        ~ReportQuit() { }
        
    public:
        unsigned int  total_usetime=0;  /* 使用时间 */
        unsigned int  dns_count=0;  /* DNS解析次数 */
        unsigned int  valid_count=0;  /* SDK验证次数 */
        unsigned int  redirect_count=0;  /* 重定向次数 */
        unsigned int  login_count=0;  /* 登录次数 */
        unsigned int  join_count=0;  /* 加入会话次数 */
        unsigned int  leave_count=0;  /* 离开次数 */
        unsigned int  kickout_count=0;  /* 被踢出次数 */
        unsigned int  uploadlog_count=0;  /* 日志上行次数 */
        unsigned int  sdk_version=0;  /* SDK版本 */
        unsigned int  platform=0;  /* 平台 */
        std::string  canal_id="";  /* 渠道ID */
        
        
    public:
        void LoadToRecord(youmecommon::CRecord & record) const
        {
            record.SetData(appkey.c_str());
            record.SetData(userid.c_str());
            record.SetData(total_usetime);
            record.SetData(dns_count);
            record.SetData(valid_count);
            record.SetData(redirect_count);
            record.SetData(login_count);
            record.SetData(join_count);
            record.SetData(leave_count);
            record.SetData(kickout_count);
            record.SetData(uploadlog_count);
            record.SetData(sdk_version);
            record.SetData(platform);
            record.SetData(canal_id.c_str());
            
        }
    };
    
    /**
     * 业务逻辑上报，客户端使用时候主动触发
     * TCP
     */
    
    /********************************************************************************/
    /* [CMD:1002] [VERSION:1] */
    class ReportDNSParse : public ReportMessage {
    public:
        ReportDNSParse()
        {
            cmdid = 1002;
            version = 1;
            bUseTcp = true;
        }
        
        ~ReportDNSParse() { }
        
    public:
        std::string  parse_addr="";  /* 要解析地址 */
        unsigned int  parse_usetime=0;  /* 解析使用时间 */
        unsigned int  parse_result=0;  /* 解析结果 */
        std::string  parse_iplist="";  /* 解析出来的IP列表 */
        unsigned int  sdk_version=0;  /* SDK版本 */
        unsigned int  platform=0;  /* 平台 */
        std::string  canal_id="";  /* 渠道ID */
        
        
    public:
        void LoadToRecord(youmecommon::CRecord & record) const
        {
            record.SetData(appkey.c_str());
            record.SetData(userid.c_str());
            record.SetData(parse_addr.c_str());
            record.SetData(parse_usetime);
            record.SetData(parse_result);
            record.SetData(parse_iplist.c_str());
            record.SetData(sdk_version);
            record.SetData(platform);
            record.SetData(canal_id.c_str());
            
        }
    };
    
    /**
     * 业务逻辑上报，客户端使用时候主动触发
     * TCP
     */
    
    /********************************************************************************/
    /* [CMD:1003] [VERSION:1] */
    class ReportSDKValid : public ReportMessage {
    public:
        ReportSDKValid()
        {
            cmdid = 1003;
            version = 1;
            bUseTcp = true;
        }
        
        ~ReportSDKValid() { }
        
    public:
        unsigned int  server_type=0;  /* 验证服务器类型 */
        std::string  server_addr="";  /* 验证服务器地址 */
        std::string  phone_type="";  /* 手机类型 */
        std::string  phone_num="";  /* 手机号码 */
        std::string  cpu_chip="";  /* CPU芯片 */
        std::string  cpu_version="";  /* CPU芯片版本 */
        unsigned int  cpu_core=0;  /* CPU核数 */
        std::string  os_name="";  /* 操作系统类型 */
        std::string  os_version="";  /* 操作系统版本 */
        std::string  mobi_operator="";  /* 运营商 */
        std::string  net_type="";  /* 网络类型 */
        unsigned int  retry_times=0;  /* 重试次数 */
        unsigned int  sdk_usetime=0;  /* 验证用时 */
        unsigned int  sdk_result=0;  /* 验证结果 */
        std::string  redirect_server="";  /* 重定向服务器地址 */
        unsigned int  redirect_port=0;  /* 重定向服务器端口 */
        unsigned int  sdk_version=0;  /* SDK版本 */
        unsigned int  platform=0;  /* 平台 */
        std::string  canal_id="";  /* 渠道ID */
        std::string  package_name="";  /* app包名 */
        
        
    public:
        void LoadToRecord(youmecommon::CRecord & record) const
        {
            record.SetData(appkey.c_str());
            record.SetData(userid.c_str());
            record.SetData(server_type);
            record.SetData(server_addr.c_str());
            record.SetData(phone_type.c_str());
            record.SetData(phone_num.c_str());
            record.SetData(cpu_chip.c_str());
            record.SetData(cpu_version.c_str());
            record.SetData(cpu_core);
            record.SetData(os_name.c_str());
            record.SetData(os_version.c_str());
            record.SetData(mobi_operator.c_str());
            record.SetData(net_type.c_str());
            record.SetData(retry_times);
            record.SetData(sdk_usetime);
            record.SetData(sdk_result);
            record.SetData(redirect_server.c_str());
            record.SetData(redirect_port);
            record.SetData(sdk_version);
            record.SetData(platform);
            record.SetData(canal_id.c_str());
            record.SetData(package_name.c_str());
            
        }
    };
    
    /**
     * 业务逻辑上报，客户端使用时候主动触发
     * TCP
     */
    
    /********************************************************************************/
    /* [CMD:1004] [VERSION:1] */
    class ReportRedirect : public ReportMessage {
    public:
        ReportRedirect()
        {
            cmdid = 1004;
            version = 1;
            bUseTcp = true;
        }
        
        ~ReportRedirect() { }
        
    public:
        std::string  redirect_server="";  /* 重定向服务器IP */
        unsigned int  redirect_port=0;  /* 重定向服务器端口 */
        std::string  roomid="";  /* 房间号 */
        unsigned int  retry_times=0;  /* 重试次数 */
        unsigned int  redirect_usetime=0;  /* 重定向用时 */
        unsigned int  redirect_result=0;  /* 重定向结果 */
        unsigned int  redirect_status=0;  /* 重定向状态 */
        std::string  mcu_server="";  /* 登录服务器地址 */
        unsigned int  mcu_port=0;  /* 登录服务器端口 */
        unsigned int  sdk_version=0;  /* SDK版本 */
        unsigned int  server_region=0;  /* 服务器区域 */
        unsigned int  platform=0;  /* 平台 */
        std::string  canal_id="";  /* 渠道ID */
        std::string  package_name="";  /* app包名 */
        
        
    public:
        void LoadToRecord(youmecommon::CRecord & record) const
        {
            record.SetData(appkey.c_str());
            record.SetData(userid.c_str());
            record.SetData(redirect_server.c_str());
            record.SetData(redirect_port);
            record.SetData(roomid.c_str());
            record.SetData(retry_times);
            record.SetData(redirect_usetime);
            record.SetData(redirect_result);
            record.SetData(redirect_status);
            record.SetData(mcu_server.c_str());
            record.SetData(mcu_port);
            record.SetData(sdk_version);
            record.SetData(server_region);
            record.SetData(platform);
            record.SetData(canal_id.c_str());
            record.SetData(package_name.c_str());
            
        }
    };
    
    /**
     * 业务逻辑上报，客户端使用时候主动触发
     * TCP
     */
    
    /********************************************************************************/
    /* [CMD:1005] [VERSION:1] */
    class ReportLogin : public ReportMessage {
    public:
        ReportLogin()
        {
            cmdid = 1005;
            version = 1;
            bUseTcp = true;
        }
        
        ~ReportLogin() { }
        
    public:
        std::string  roomid="";  /* 房间ID */
        std::string  mcu_server="";  /* 登录服务器地址 */
        unsigned int  mcu_port=0;  /* 登录服务器端口 */
        unsigned int  retry_times=0;  /* 重试次数 */
        unsigned int  login_usetime=0;  /* 登录用时 */
        unsigned int  login_result=0;  /* 登录结果 */
        unsigned int  login_status=0;  /* 登录状态 */
        unsigned int  mcu_sessionid=0;  /* sessionid */
        unsigned int  mcu_udpport=0;  /* udpport */
        unsigned int  sdk_version=0;  /* SDK版本 */
        unsigned int  server_region=0;  /* 服务器区域 */
        unsigned int  platform=0;  /* 平台 */
        std::string  canal_id="";  /* 渠道ID */
        
        
    public:
        void LoadToRecord(youmecommon::CRecord & record) const
        {
            record.SetData(appkey.c_str());
            record.SetData(userid.c_str());
            record.SetData(roomid.c_str());
            record.SetData(mcu_server.c_str());
            record.SetData(mcu_port);
            record.SetData(retry_times);
            record.SetData(login_usetime);
            record.SetData(login_result);
            record.SetData(login_status);
            record.SetData(mcu_sessionid);
            record.SetData(mcu_udpport);
            record.SetData(sdk_version);
            record.SetData(server_region);
            record.SetData(platform);
            record.SetData(canal_id.c_str());
            
        }
    };
    
    /**
     * 业务逻辑上报，客户端使用时候主动触发
     * TCP
     */
    
    /********************************************************************************/
    /* [CMD:1006] [VERSION:1] */
    class ReportChannel : public ReportMessage {
    public:
        ReportChannel()
        {
            cmdid = 1006;
            version = 1;
            bUseTcp = true;
        }
        
        ~ReportChannel() { }
        
    public:
        std::string  roomid="";  /* 房间号 */
        unsigned int  sessionid=0;  /* 会话id */
        unsigned int  lost_heart=0;  /* seesionid */
        unsigned int  heart_interval=0;  /* 心跳间隔 */
        unsigned int  heart_count=0;  /* 心跳次数 */
        unsigned int  on_disconnect=0;  /* 是否启动重连 */
        unsigned int  need_userlist=0;  /* 是否需要用户列表 */
        unsigned int  need_mic=0;  /* 是否需要麦克风 */
        unsigned int  need_speaker=0;  /* 是否打开扬声器 */
        unsigned int  auto_sendstatus=0;  /* 是否自动发送状态 */
        unsigned int  user_role=0;  /* 用户角色 */
        unsigned int  api_from=0;  /* 哪个接口调用的 */
        unsigned int  rtc_usetime=0;  /* 使用时间 */
        unsigned int  operate_type=0;  /* 操作类型 */
        unsigned int  sdk_version=0;  /* SDK版本 */
        unsigned int  video_duration=0;  /* 视频采集时长 */
        unsigned int  platform=0;  /* 平台 */
        std::string  canal_id="";  /* 渠道ID */
        std::string  package_name="";  /* app包名 */
        
        
    public:
        void LoadToRecord(youmecommon::CRecord & record) const
        {
            record.SetData(appkey.c_str());
            record.SetData(userid.c_str());
            record.SetData(roomid.c_str());
            record.SetData(sessionid);
            record.SetData(lost_heart);
            record.SetData(heart_interval);
            record.SetData(heart_count);
            record.SetData(on_disconnect);
            record.SetData(need_userlist);
            record.SetData(need_mic);
            record.SetData(need_speaker);
            record.SetData(auto_sendstatus);
            record.SetData(user_role);
            record.SetData(api_from);
            record.SetData(rtc_usetime);
            record.SetData(operate_type);
            record.SetData(sdk_version);
            record.SetData(video_duration);
            record.SetData(platform);
            record.SetData(canal_id.c_str());
            record.SetData(package_name.c_str());
            
        }
    };
    
    /**
     * 业务逻辑上报，客户端使用时候主动触发
     * TCP
     */
    
    /********************************************************************************/
    /* [CMD:1007] [VERSION:1] */
    class ReportMixAudioTrack : public ReportMessage {
    public:
        ReportMixAudioTrack()
        {
            cmdid = 1007;
            version = 1;
            bUseTcp = true;
        }
        
        ~ReportMixAudioTrack() { }
        
    public:
        unsigned int  but_size=0;  /* 缓冲区数据大小 */
        unsigned int  channel_num=0;  /* 声道数量 */
        unsigned int  sample_rate=0;  /* 采样率 */
        unsigned int  bytes_persample=0;  /* 一个声道里面每个采样的字节数 */
        unsigned char  isfloat=0;  /* 是否采用浮点数 */
        unsigned char  little_endian=0;  /* 大小端存储 */
        unsigned char  inter_leaved=0;  /* 多声道存储方式 */
        unsigned char  for_speaker=0;  /* 是否输出到扬声器 */
        unsigned int  sdk_version=0;  /* SDK版本 */
        unsigned int  platform=0;  /* 平台 */
        std::string  canal_id="";  /* 渠道ID */
        
        
    public:
        void LoadToRecord(youmecommon::CRecord & record) const
        {
            record.SetData(appkey.c_str());
            record.SetData(userid.c_str());
            record.SetData(but_size);
            record.SetData(channel_num);
            record.SetData(sample_rate);
            record.SetData(bytes_persample);
            record.SetData(isfloat);
            record.SetData(little_endian);
            record.SetData(inter_leaved);
            record.SetData(for_speaker);
            record.SetData(sdk_version);
            record.SetData(platform);
            record.SetData(canal_id.c_str());
            
        }
    };
    
    /**
     * 业务逻辑上报，客户端使用时候主动触发
     * TCP
     */
    
    /********************************************************************************/
    /* [CMD:1008] [VERSION:1] */
    class ReportBackgroundMusic : public ReportMessage {
    public:
        ReportBackgroundMusic()
        {
            cmdid = 1008;
            version = 1;
            bUseTcp = true;
        }
        
        ~ReportBackgroundMusic() { }
        
    public:
        unsigned char  operate_type=0;  /* 操作类型 */
        std::string  file_path="";  /* 文件路径 */
        std::string  file_type="";  /* 文件类型 */
        unsigned int  file_size=0;  /* 文件大小 */
        unsigned int  play_result=0;  /* 播放结果 */
        unsigned int  play_repeat=0;  /* 是否循环 */
        unsigned int  sdk_version=0;  /* SDK版本 */
        unsigned int  platform=0;  /* 平台 */
        std::string  canal_id="";  /* 渠道ID */
        
        
    public:
        void LoadToRecord(youmecommon::CRecord & record) const
        {
            record.SetData(appkey.c_str());
            record.SetData(userid.c_str());
            record.SetData(operate_type);
            record.SetData(file_path.c_str());
            record.SetData(file_type.c_str());
            record.SetData(file_size);
            record.SetData(play_result);
            record.SetData(play_repeat);
            record.SetData(sdk_version);
            record.SetData(platform);
            record.SetData(canal_id.c_str());
            
        }
    };
    
    /**
     * 业务逻辑上报，客户端使用时候主动触发
     * TCP
     */
    
    /********************************************************************************/
    /* [CMD:1009] [VERSION:1] */
    class ReportSetting : public ReportMessage {
    public:
        ReportSetting()
        {
            cmdid = 1009;
            version = 1;
            bUseTcp = true;
        }
        
        ~ReportSetting() { }
        
    public:
        std::string  operate_type="";  /* 操作类型 */
        unsigned int  operate_mode=0;  /* 操作方式 */
        unsigned int  status=0;  /* 状态 */
        unsigned int  value=0;  /* 值 */
        std::string  param="";  /* 其它参数 */
        std::string  other_userid="";  /* 他人用户ID */
        unsigned int  sdk_version=0;  /* SDK版本 */
        unsigned int  platform=0;  /* 平台 */
        std::string  canal_id="";  /* 渠道ID */
        
        
    public:
        void LoadToRecord(youmecommon::CRecord & record) const
        {
            record.SetData(appkey.c_str());
            record.SetData(userid.c_str());
            record.SetData(operate_type.c_str());
            record.SetData(operate_mode);
            record.SetData(status);
            record.SetData(value);
            record.SetData(param.c_str());
            record.SetData(other_userid.c_str());
            record.SetData(sdk_version);
            record.SetData(platform);
            record.SetData(canal_id.c_str());
            
        }
    };
    
    /**
     * 业务逻辑上报，客户端使用时候主动触发
     * TCP
     */
    
    /********************************************************************************/
    /* [CMD:1010] [VERSION:1] */
    class ReportPacketStatOneUser : public ReportMessage {
    public:
        ReportPacketStatOneUser()
        {
            cmdid = 1010;
            version = 1;
            bUseTcp = true;
        }
        
        ~ReportPacketStatOneUser() { }
        
    public:
        unsigned int  packetLossRate10000th=0;  /*  */
        unsigned int  networkDelayMs=0;  /*  */
        unsigned int  averagePacketTimeMs=0;  /*  */
        unsigned int  serverRegion=0;  /* 服务器区域 */
        std::string  other_userid="";  /* 上报的谁的数据 */
        unsigned int  platform=0;  /* 平台 */
        unsigned int  sdk_version=0;  /* SDK版本 */
        std::string  canal_id="";  /* 渠道ID */
        
        
    public:
        void LoadToRecord(youmecommon::CRecord & record) const
        {
            record.SetData(appkey.c_str());
            record.SetData(userid.c_str());
            record.SetData(packetLossRate10000th);
            record.SetData(networkDelayMs);
            record.SetData(averagePacketTimeMs);
            record.SetData(serverRegion);
            record.SetData(other_userid.c_str());
            record.SetData(platform);
            record.SetData(sdk_version);
            record.SetData(canal_id.c_str());
            
        }
    };
    
    /**
     * 业务逻辑上报，客户端使用时候主动触发
     * TCP
     */
    
    /********************************************************************************/
    /* [CMD:1013] [VERSION:1] */
    class ReportTranslateInfo : public ReportMessage {
    public:
        ReportTranslateInfo()
        {
            cmdid = 1013;
            version = 1;
            bUseTcp = true;
        }
        
        ~ReportTranslateInfo() { }
        
    public:
        unsigned int  SDKVersion=0;  /* SDK版本 */
        unsigned short  status=0;  /* 状态 */
        std::string  srcLanguage="";  /* 源语言 */
        std::string  destLanguage="";  /* 目标语言 */
        unsigned long long  characterCount=0;  /* 字符数 */
        unsigned short  translateVersion=0;  /* 翻译使用版本 */
        
        
    public:
        void LoadToRecord(youmecommon::CRecord & record) const
        {
            record.SetData(appkey.c_str());
            record.SetData(userid.c_str());
            record.SetData(SDKVersion);
            record.SetData(status);
            record.SetData(srcLanguage.c_str());
            record.SetData(destLanguage.c_str());
            record.SetData(characterCount);
            record.SetData(translateVersion);
            
        }
    };
    
    /**
     * 业务逻辑上报，客户端使用时候主动触发
     * TCP
     */
    
    /********************************************************************************/
    /* [CMD:3001] [VERSION:1] */
    class ReportVideoEvent : public ReportMessage {
    public:
        ReportVideoEvent()
        {
            cmdid = 3001;
            version = 1;
            bUseTcp = true;
        }
        
        ~ReportVideoEvent() { }
        
    public:
        unsigned int  sessionid=0;  /* session id */
        unsigned int  event=0;  /* 事件(0:打开摄像头 1:关闭摄像头 2:屏蔽他人 3:开始渲染 4:停止渲染) */
        unsigned int  result=0;  /* result(0:成功 非0:失败) */
        unsigned int  sdk_version=0;  /* sdk版本 */
        unsigned int  platform=0;  /* 平台 */
        std::string  canal_id="";  /* 渠道 */
        
        
    public:
        void LoadToRecord(youmecommon::CRecord & record) const
        {
            record.SetData(appkey.c_str());
            record.SetData(userid.c_str());
            record.SetData(sessionid);
            record.SetData(event);
            record.SetData(result);
            record.SetData(sdk_version);
            record.SetData(platform);
            record.SetData(canal_id.c_str());
            
        }
    };
    
    /**
     * 业务逻辑上报，客户端使用时候主动触发
     * TCP
     */
    
    /********************************************************************************/
    /* [CMD:3002] [VERSION:1] */
    class ReportVideoInfo : public ReportMessage {
    public:
        ReportVideoInfo()
        {
            cmdid = 3002;
            version = 1;
            bUseTcp = true;
        }
        
        ~ReportVideoInfo() { }
        
    public:
        unsigned int  sessionid=0;  /* 会话 id */
        unsigned int  width=0;  /* 宽 */
        unsigned int  height=0;  /* 高 */
        unsigned int  fps=0;  /* 帧率 */
        unsigned int  smooth=0;  /* 是否流畅 */
        unsigned int  delay=0;  /* 延时 */
        unsigned int  sdk_version=0;  /* sdk版本 */
        unsigned int  platform=0;  /* 平台 */
        std::string  canal_id="";  /* 渠道 */
        
        
    public:
        void LoadToRecord(youmecommon::CRecord & record) const
        {
            record.SetData(appkey.c_str());
            record.SetData(userid.c_str());
            record.SetData(sessionid);
            record.SetData(width);
            record.SetData(height);
            record.SetData(fps);
            record.SetData(smooth);
            record.SetData(delay);
            record.SetData(sdk_version);
            record.SetData(platform);
            record.SetData(canal_id.c_str());
            
        }
    };
    
    /**
     * 业务逻辑上报，客户端使用时候主动触发
     * TCP
     */
    
    /********************************************************************************/
    /* [CMD:3003] [VERSION:1] */
    class ReportRecvVideoInfo : public ReportMessage {
    public:
        ReportRecvVideoInfo()
        {
            cmdid = 3003;
            version = 1;
            bUseTcp = true;
        }
        
        ~ReportRecvVideoInfo() { }
        
    public:
        std::string  roomid="";  /* 房间ID */
        unsigned int  report_type=0;  /* 报告类型 */
        unsigned int  width=0;  /* 宽 */
        unsigned int  height=0;  /* 高 */
        unsigned int  video_render_duration=0;  /* 渲染持续时间 */
        std::string  other_userid="";  /* 发送视频的用户id */
        unsigned int  sdk_version=0;  /* sdk版本 */
        unsigned int  platform=0;  /* 平台 */
        unsigned int  num_camera_stream=0;  /* 接收视频流的路数 */
        unsigned int  sum_resolution=0;  /* 集合分辨率(=宽*高*路数) */
        std::string  canal_id="";  /* 渠道 */
        
        
    public:
        void LoadToRecord(youmecommon::CRecord & record) const
        {
            record.SetData(appkey.c_str());
            record.SetData(userid.c_str());
            record.SetData(roomid.c_str());
            record.SetData(report_type);
            record.SetData(width);
            record.SetData(height);
            record.SetData(video_render_duration);
            record.SetData(other_userid.c_str());
            record.SetData(sdk_version);
            record.SetData(platform);
            record.SetData(num_camera_stream);
            record.SetData(sum_resolution);
            record.SetData(canal_id.c_str());
            
        }
    };
    
    /**
     * 业务逻辑上报，客户端使用时候主动触发
     * TCP
     */
    
    /********************************************************************************/
    /* [CMD:3004] [VERSION:1] */
    class ReportVideoStatisticInfo : public ReportMessage {
    public:
        ReportVideoStatisticInfo()
        {
            cmdid = 3004;
            version = 1;
            bUseTcp = true;
        }
        
        ~ReportVideoStatisticInfo() { }
        
    public:
        unsigned int  sdk_version=0;  /* sdk版本 */
        unsigned int  platform=0;  /* 平台 */
        std::string  canal_id="";  /* 渠道 */
        std::string  user_name="";  /* 数据所属用户名 */
        unsigned int  user_type=0;  /* 0表示自己，1表示远端 */
        unsigned int  fps=0;  /* 视频帧率 */
        unsigned int  bit_rate=0;  /* 视频码率(kbps) */
        unsigned int  lost_packet_rate=0;  /* 视频丢包率 */
        unsigned int  block_count=0;  /* 卡顿次数 */
        unsigned int  report_period=0;  /* 上报的时间间隔(ms) */
        std::string  room_name="";  /* 房间号 */
        std::string  brand="";  /* 品牌 */
        std::string  model="";  /* 型号 */
        unsigned int  block_time=0;  /* 卡顿时长，两帧间隔超过200ms */
        unsigned int  networkDelayMs=0;  /* 收包延时 */
        unsigned int  averagePacketTimeMs=0;  /* 收包间隔 */
        unsigned int  firstFrameTime=0;  /* 首帧时间，从调用开启摄像头到回调渲染数据 */
        
        
    public:
        void LoadToRecord(youmecommon::CRecord & record) const
        {
            record.SetData(appkey.c_str());
            record.SetData(userid.c_str());
            record.SetData(sdk_version);
            record.SetData(platform);
            record.SetData(canal_id.c_str());
            record.SetData(user_name.c_str());
            record.SetData(user_type);
            record.SetData(fps);
            record.SetData(bit_rate);
            record.SetData(lost_packet_rate);
            record.SetData(block_count);
            record.SetData(report_period);
            record.SetData(room_name.c_str());
            record.SetData(brand.c_str());
            record.SetData(model.c_str());
            record.SetData(block_time);
            record.SetData(networkDelayMs);
            record.SetData(averagePacketTimeMs);
            record.SetData(firstFrameTime);
            
        }
    };


    /**
     * 业务逻辑上报，客户端使用时候主动触发，用于音视频qos上报
     * TCP
     */
    
    /********************************************************************************/
    /* [CMD:3005] [VERSION:1] */
    class ReportAVQosStatisticInfo : public ReportMessage {
    public:
        ReportAVQosStatisticInfo()
        {
            cmdid = 3005;
            version = 1;
            bUseTcp = true;
        }
        
        ~ReportAVQosStatisticInfo() { }
        
    public:
        unsigned long   business_id;        /* business id*/
        unsigned int    sdk_version=0;      /* sdk版本 */
        unsigned int    platform=0;         /* 平台 */
        std::string     canal_id="";        /* 渠道 */
        unsigned int    user_type=0;        /* 0表示自己，1表示远端 */
        unsigned int    report_period=0;    /* 上报的时间间隔(ms) */
        unsigned int    join_sucess_count=0;   /* 加入房间成功次数 */
        unsigned int    join_count=0;       /* 加入房间总次数 */
        unsigned int    stream_id_def;      /* 上行流说明，按bit位与，0: 大流; 1: 小流; 2: 共享流*/

        unsigned int    audio_up_lost_cnt=0;    /* 音频丢包数，半程统计，上行*/
        unsigned int    audio_up_total_cnt=0;   /* 音频总包数，半程统计, 上行 */
        unsigned int    audio_dn_lossrate=0;    /* 下行音频丢包率(取多路最大值)，dn_loss / 256 */
        unsigned int    audio_rtt=0;            /* 音频延时(ms)，全程统计*/
        unsigned int    video_up_lost_cnt=0;    /* 视频丢包数，半程统计，上行*/
        unsigned int    video_up_total_cnt=0;   /* 视频总包数，半程统计, 上行*/
        unsigned int    video_dn_lossrate=0;    /* 下行视频丢包率(取多路最大值)，dn_loss / 256 */
        unsigned int    video_rtt=0;            /* 视频延时(ms)，全程统计*/

        unsigned int    audio_up_bitrate=0; /*音频码率(kbps)，上行*/
        unsigned int    audio_dn_bitrate=0; /*音频码率(kbps)，下行，包括多路下行音频总和*/
        unsigned int    video_up_camera_bitrate=0; /* 视频码率(kbps), 上行, 大小流 */
        unsigned int    video_up_share_bitrate=0;  /* 视频码率(kbps), 上行, share */
        unsigned int    video_dn_camera_bitrate=0; /* 视频码率(kbps), 下行带宽统计(kbps), 大小流 */
        unsigned int    video_dn_share_bitrate=0;  /* 视频码率(kbps), 下行带宽统计(kbps), share */
        unsigned int    video_up_fps=0;     /* 视频帧率，上行，编码前 */
        unsigned int    video_dn_fps=0;     /* 视频帧率，下行，解码后 */
        unsigned int    block_count=0;      /* 视频卡顿次数 */
        unsigned int    block_time=0;       /* 卡顿时长，两帧间隔超过200ms */
        std::string     room_name="";       /* 房间号 */
        std::string     brand="";           /* 品牌 */
        std::string     model="";           /* 型号 */
        // unsigned int    networkDelayMs=0;  /* 收包延时 */
        // unsigned int    averagePacketTimeMs=0;  /* 收包间隔 */
        // unsigned int    firstFrameTime=0;  /* 首帧时间，从调用开启摄像头到回调渲染数据 */
        
        
    public:
        void LoadToRecord(youmecommon::CRecord & record) const
        {
            record.SetData(appkey.c_str());
            record.SetData(userid.c_str());
            record.SetData(business_id);
            record.SetData(sdk_version);
            record.SetData(platform);
            record.SetData(canal_id.c_str());
            record.SetData(user_type);
            record.SetData(report_period);
            record.SetData(join_sucess_count);
            record.SetData(join_count);
            record.SetData(stream_id_def);
            record.SetData(audio_up_lost_cnt);
            record.SetData(audio_up_total_cnt);
            record.SetData(audio_dn_lossrate);
            record.SetData(audio_rtt);
            record.SetData(video_up_lost_cnt);
            record.SetData(video_up_total_cnt);
            record.SetData(video_dn_lossrate);
            record.SetData(video_rtt);
            record.SetData(audio_up_bitrate);
            record.SetData(audio_dn_bitrate);
            record.SetData(video_up_camera_bitrate);
            record.SetData(video_up_share_bitrate);
            record.SetData(video_dn_camera_bitrate);
            record.SetData(video_dn_share_bitrate);
            record.SetData(video_up_fps);
            record.SetData(video_dn_fps);
            record.SetData(block_count);
            record.SetData(block_time);
            record.SetData(room_name.c_str());
            record.SetData(brand.c_str());
            record.SetData(model.c_str());

            // record.SetData(networkDelayMs);
            // record.SetData(averagePacketTimeMs);
            // record.SetData(firstFrameTime);
            
        }
    };
}
