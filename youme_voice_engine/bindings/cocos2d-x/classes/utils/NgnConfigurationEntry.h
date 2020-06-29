
#ifndef __NGNCONFIGURATIONENTRY__
#define __NGNCONFIGURATIONENTRY__
#include <string>
using namespace std;
#include "tinymedia.h"
#include "tinydav.h"


/** 录音和设置CommMode顺序 */
enum {
    COMMMODE_BEFORE = 0x00,    /** 启动录音前设置CommMode */
    COMMMODE_AFTER  = 0x01,  /** 启动录音后设置CommMode */
    COMMMODE_TWO_SIDES = 0x02, /** 启动录音前后都设置CommMode */
    COMMMODE_NONE = 0x03 /** 启动录音前后都不设置CommMode */
};


class NgnConfigurationEntry
{
    public:
    static string TAG;

    static string CONFIG_FILE_NAME;

    // General
  
    static string GENERAL_AEC;
    
    static string GENERAL_AEC_OPT;

    static string GENERAL_VAD;

    static string GENERAL_AGC;

    static string GENERAL_NR;
    
    static string GENERAL_HS;
    
    static string GENERAL_REV;

    static string GENERAL_CNG;

    static string GENERAL_MCU_MODE;
    
    static string GENERAL_ECHO_TAIL;
    
    static string GENERAL_AGC_TARGET_LEVEL;
    
    static string GENERAL_AGC_COMPRESS_GAIN;
   
    static string GENERAL_JITBUFFER_MARGIN;
    
    static string GENERAL_JITBUFFER_MAXLATERATE;
    
    static string GENERAL_RECORD_SAMPLE_RATE;
    
    static string GENERAL_PLAYBACK_SAMPLE_RATE;

    // Enable dumping PCM files
    static string   PCM_FILE_SIZE_KB;
    static uint32_t DEF_PCM_FILE_SIZE_KB;
    
    // Enable dumping H264 files
    static string   H264_FILE_SIZE_KB;
    static uint32_t DEF_H264_FILE_SIZE_KB;
 
    // Enable sending vad result by rtp
    static string VAD_LEN_MS;
    static uint32_t DEF_VAD_LEN_MS;
    static string VAD_SILENCE_PERCENT;
    static uint32_t DEF_VAD_SILENCE_PERCENT;


    // Android audio path use opensles or not
    static string ANDROID_UNDER_OPENSLES;
    static bool DEFAULT_ANDROID_UNDER_OPENSLES;
    
    // Network
    static string NETWORK_USE_WIFI;
     static string NETWORK_USE_MOBILE;
   
    static string NETWORK_REDIRECT_ADD;

    static string NETWORK_REDIRECT_PORT;

    static string MAX_VIDEO_BITRATE_LEVEL_SERVER;
    static string MIN_VIDEO_BITRATE_LEVEL_SERVER;
    static string MAX_VIDEO_BITRATE_LEVEL_P2P;
  
    static string MAX_RTCP_MEMBER_COUNT;
    static string VIDEO_NACK_FLAG;

    // Media
    static string MEDIA_PROFILE;

    static string DEFAULT_NETWORK_REDIRECT_ADD;

    // General

    static bool DEFAULT_GENERAL_AEC;
    
    static uint32_t DEFAULT_GENERAL_AEC_OPT;

    static bool DEFAULT_GENERAL_VAD;

    static bool DEFAULT_GENERAL_NR;

    static bool DEFAULT_GENERAL_AGC;

    static bool DEFAULT_GENERAL_CNG;
    
    static bool DEFAULT_GENERAL_HS;
    
    static bool DEFAULT_GENERAL_REV;

    static int DEFAULT_GENERAL_ECHO_TAIL;
    
    static int DEFAULT_GENERAL_AGC_TARGET_LEVEL;
    
    static int DEFAULT_GENERAL_AGC_COMPRESS_GAIN;

    static bool DEFAULT_GENERAL_USE_ECHO_TAIL_ADAPTIVE;

    static int DEFAULT_SAMPLE_RATE;
    
    static int DEFAULT_RECORD_SAMPLE_RATE;
    
    static int DEFAULT_PLAYBACK_SAMPLE_RATE;
    
    static int DEFAULT_JITBUFFER_MARGIN;
    
    static int DEFAULT_JITBUFFER_MAXLATERATE;
    
    static string   MAX_CHANNEL_NUM;
    static uint32_t DEF_MAX_CHANNEL_NUM;
    
    //渠道号
    static string CANAL_ID;
    static string DEFAULT_CANAL_ID;
    
    static string  VIDEO_STAT_REPORT_PERIOD_MS;
    static int  DEFAULT_VIDEO_STAT_REPORT_PERIOD_MS ;

   
    // Network
    static bool DEFAULT_NETWORK_USE_WIFI;
    static bool DEFAULT_NETWORK_USE_3G;
    static int DEFAULT_NETWORK_REDIRECT_PORT;
    static uint32_t DEFAULT_MAX_VIDEO_BITRATE_LEVEL_SERVER;
    static uint32_t DEFAULT_MIN_VIDEO_BITRATE_LEVEL_SERVER;
    static uint32_t DEFAULT_MAX_VIDEO_BITRATE_LEVEL_P2P;

    static uint32_t DEFAULT_MAX_RTCP_MEMBER_COUNT;
    static uint32_t DEFAULT_VIDEO_NACK_FLAG;

    // Media
    static tmedia_profile_t DEFAULT_MEDIA_PROFILE;

    static tmedia_codec_id_t DEFAULT_MEDIA_CODECS;

    // OPUS, FEC, and NetEQ
    // TODO: 将所有 Enabled 配置改为bool型
    static string OPUS_COMPLEXITY;
    static int    DEFAULT_OPUS_COMPLEXITY;
    static string OPUS_ENABLE_VBR;
    static int    DEFAULT_OPUS_ENABLE_VBR;
    static string OPUS_MAX_BANDWIDTH;
    static int    DEFAULT_OPUS_MAX_BANDWIDTH;
    static string OPUS_ENABLE_INBANDFEC;
    static int    DEFAULT_OPUS_ENABLE_INBANDFEC;
    static string OPUS_ENABLE_OUTBANDFEC;
    static int    DEFAULT_OPUS_ENABLE_OUTBANDFEC;
    static string OPUS_DTX_PEROID;
    static int    DEFAULT_OPUS_DTX_PEROID;
    static string OPUS_PACKET_LOSS_PERC;
    static int    DEFAULT_OPUS_PACKET_LOSS_PERC;
    static string OPUS_ENABLE_DTX;
    static int    DEFAULT_OPUS_ENABLE_DTX;
    static string OPUS_BITRATE;
    static int    DEFAULT_OPUS_BITRATE;
    static string RTP_FEEDBACK_PERIOD; // in packet number
    static int    DEF_RTP_FEEDBACK_PERIOD;
    //
    static string MAX_JITTER_BUFFER_NUM;
    static uint32_t DEF_MAX_JITTER_BUFFER_NUM;
    static string  MAX_NETEQ_DELAY_MS;
    static uint32_t DEF_MAX_NETEQ_DELAY_MS;
    
    //数据上报地址和端口
    static string DATAREPORT_ADDR;
    static string DEFAULT_DATAREPORT_ADDR;
    static string DATAREPORT_PORT;
    static int DEFAULT_DATAREPORT_PORT;
    
    static string DATAREPORT_MODE;
    static int DEFAULT_DATAREPORT_MODE;
    
    //心跳包配置
    static string HEART_TIMEOUT;
    static string HEART_TIMEOUT_COUNT;
    static int DEFAULT_HEART_TIMEOUT;
    static int DEFAULT_TIMEOUT_COUNT;
    
    
    //断线重连次数
    static string RECONNECT_COUNT;
    static int DEFAULT_RECONNECT_COUNT;
    
    //是否启动功能
    static string ENABLE;
    static bool DEFAULT_ENABLE;
    
    //是否启动自动重连
    static string AUTO_RECONNECT;
    static bool DEFAULT_AUTO_RECONNECT;
    
    //回声消除
    static string AGC_MIN;
    static int DEFAULT_AGC_MIN;
    
    
    static string AGC_MAX;
    static int DEFAULT_AGC_MAX;
    
    static string ECHO_SKEW;
    static int DEF_ECHO_SKEW;
    
    static string FADEIN_TIME_MS;
    static uint32_t DEF_FADEIN_TIME_MS;
    
    //变音处理
    static string SOUNDTOUCH_ENABLED;
    static int DEFAULT_SOUNDTOUCH_ENABLED;
    
    static string SOUNDTOUCH_TEMPO;
    static int DEFAULT_SOUNDTOUCH_TEMPO;
    
    static string SOUNDTOUCH_RATE;
    static int DEFAULT_SOUNDTOUCH_RATE;
    
    static string SOUNDTOUCH_PITCH;
    static int DEFAULT_SOUNDTOUCH_PITCH;
    
    //是否启动communication
    static string COMMUNICATION_MODE;
    static bool DEFAULT_COMMUNICATION_MODE;
    
    //录音和设置CommMode顺序
    static string COMMUNICATION_MODE_ORDER;
    static int DEFAULT_COMMUNICATION_MODE_ORDER;
    
    //android 动态库更新
    static string UPDATE;
    static bool DEFAULT_UPDATE;
    
    static string UPDATE_URL;
    static string DEFAULT_UPDATE_URL;
    
    static string UPDATE_MD5;
    static string DEFAULT_UPDATE_MD5;
    
    static string VERIFY_TIME;
    static unsigned int DEFAULT_VERIFY_TIME;
    
    // Android specific configurations.
    static string ANDROID_OUTPUT_VOLUME_GAIN;
    static int    DEFAULT_ANDROID_OUTPUT_VOLUME_GAIN;
    static string MIC_VOLUME_GAIN;
    static int    DEF_MIC_VOLUME_GAIN;
    
    // iOS specific configurations.
    static string IOS_OUTPUT_VOLUME_GAIN;
    static int    DEFAULT_IOS_OUTPUT_VOLUME_GAIN;
    static string IOS_NO_PERMISSION_NO_REC;
    static bool   DEF_IOS_NO_PERMISSION_NO_REC;
    static string IOS_AIRPLAY_NO_REC;
    static bool   DEF_IOS_AIRPLAY_NO_REC;
    static string IOS_ALWAYS_PLAY_CATEGORY;
    static bool   DEF_IOS_ALWAYS_PLAY_CATEGORY;
    static string IOS_NEED_SET_MODE;
    static bool   DEF_IOS_NEED_SET_MODE;
    static string IOS_RECOVERY_DELAY_FROM_CALL;
    static uint32_t DEF_IOS_RECOVERY_DELAY_FROM_CALL;
    
    // Background music delay configuration.
    static string BACKGROUND_MUSIC_DELAY;
    static unsigned int DEFAULT_BACKGROUND_MUSIC_DELAY;
    
    // Whether exit comm-mode when headset is plugin
    static string   EXIT_COMMMODE_API_ENABLE;
    static bool     DEF_EXIT_COMMMODE_API_ENABLE;
    
    // Log
    static string   UPLOAD_LOG;
    static bool     DEF_UPLOAD_LOG;
    static string   UPLOAD_LOG_FOR_USER;
    static string   DEF_UPLOAD_LOG_FOR_USER;
    static string   LOG_LEVEL_CONSOLE;
    static uint32_t DEF_LOG_LEVEL_CONSOLE;
    static string   LOG_LEVEL_FILE;
    static uint32_t DEF_LOG_LEVEL_FILE;
    static string   LOG_FILE_SIZE_KB;
    static uint32_t DEF_LOG_FILE_SIZE_KB;
    // Period for printing packet statistics info, 0 for not printing anything.
    static string   PACKET_STAT_LOG_PERIOD_MS;
    static uint32_t DEF_PACKET_STAT_LOG_PERIOD_MS;
    // Period for calculating/reporting packet statistics info, 0 for not calculating/reporting
    static string   PACKET_STAT_REPORT_PERIOD_MS;
    static uint32_t DEF_PACKET_STAT_REPORT_PERIOD_MS;
    
    static string   AV_QOS_STAT_REPORT_PERIOD_MS;
    static uint32_t DEF_AV_QOS_STAT_REPORT_PERIOD_MS;

    // Video part
    static string   CAMERA_ROTATION;
    static int DEFAULT_CAMERA_ROTATION;
    static string   VIDEO_SIZE_INDEX;
    static int DEFAULT_VIDEO_SIZE_INDEX;
    
    static string VIDEO_ALLOW_FIX_QUALITY;
    static bool   DEF_VIDEO_ALLOW_FIX_QUALITY;
    
    static string CAMERA_CAPTURE_MODE;
    static int   DEF_CAMERA_CAPTURE_MODE;

    static string   VIDEO_BITRATE_MAX_MULTIPLE;  
    static uint32_t DEFAULT_VIDEO_BITRATE_MAX_MULTIPLE;  // 外部设置码率范围，默认码率的倍数
 
    static string   VIDEO_FRAME_REFREASH_TIME;  
    static uint32_t DEFAULT_VIDEO_FRAME_REFREASH_TIME;  // 共享流，强刷最后一帧的超时时间，默认500ms

    // AVRunTimeMonitor
    static string   AVMONITOR_CONSUMER_PEROID_MS;
    static uint32_t DEF_AVMONITOR_CONSUMER_PEROID_MS;
    static string   AVMONITOR_PRODUCER_PEROID_MS;
    static uint32_t DEF_AVMONITOR_PRODUCER_PEROID_MS;
    
    //translate
    static string CONFIG_TRANSLATE_ENABLE;
    static int TRANSLATE_SWITCH_DEFAULT;
    static int TRANSLATE_METHOD_DEFAULT;
    static string CONFIG_TRANSLATE_HOST;
    static string TRANSLATE_HOST_DEFAULT;
    static string CONFIG_TRANSLATE_CGI;
    static string TRANSLATE_CGI_PATH_DEFAULT;
    
    static string CONFIG_TRANSLATE_REGULAR;
    static string TRANSLATE_REGULAR_DEFAULT;
    
    static string CONFIG_TRANSLATE_METHOD;
    static string CONFIG_TRANSLATE_HOST_V2;
    static string TRANSLATE_HOST_DEFAULT_V2;
    static string CONFIG_TRANSLATE_CGI_V2;
    static string TRANSLATE_CGI_PATH_DEFAULT_V2;
    
    static string CONFIG_TRANSLATE_GOOGLE_APIKEY;
    static string TRANSLATE_GOOGLE_APIKEY_DEFAULT;
    
};
#endif
