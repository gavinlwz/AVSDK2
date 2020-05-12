//
//  NgnConfigurationEntry.cpp
//  youme_voice_engine
//
//  Created by wanglei on 15/11/2.
//  Copyright © 2015年 youme. All rights reserved.
//

#include <stdio.h>
#include "NgnConfigurationEntry.h"
#include "NgnEngine.h"
string NgnConfigurationEntry::TAG = "NgnConfigurationEntry";

string NgnConfigurationEntry::CONFIG_FILE_NAME = TAG + ".ini";


// General


string NgnConfigurationEntry::GENERAL_AEC = "AEC";

string NgnConfigurationEntry::GENERAL_AEC_OPT = "AEC_OPT";

string NgnConfigurationEntry::GENERAL_VAD = "VAD";

string NgnConfigurationEntry::GENERAL_AGC = "AGC";

string NgnConfigurationEntry::GENERAL_NR = "NR";

string NgnConfigurationEntry::GENERAL_CNG = "CNG";

string NgnConfigurationEntry::GENERAL_HS = "HS";

string NgnConfigurationEntry::GENERAL_REV = "REV";

string NgnConfigurationEntry::GENERAL_ECHO_TAIL = "ECHO_TAIL";

string NgnConfigurationEntry::GENERAL_AGC_TARGET_LEVEL = "AGC_TARGET_LEVEL";

string NgnConfigurationEntry::GENERAL_AGC_COMPRESS_GAIN = "AGC_COMPRESS_GAIN";

string NgnConfigurationEntry::GENERAL_JITBUFFER_MARGIN = "JITBUFFER_MARGIN";
string NgnConfigurationEntry::GENERAL_JITBUFFER_MAXLATERATE = "GENERAL_JITBUFFER_MAXLATERATE";
string NgnConfigurationEntry::GENERAL_RECORD_SAMPLE_RATE = "RECORD_SAMPLE_RATE";
string NgnConfigurationEntry::GENERAL_PLAYBACK_SAMPLE_RATE = "PLAYBACK_SAMPLE_RATE";

string   NgnConfigurationEntry::PCM_FILE_SIZE_KB = "PCM_FILE_SIZE_KB";
uint32_t NgnConfigurationEntry::DEF_PCM_FILE_SIZE_KB = 0;

string   NgnConfigurationEntry::H264_FILE_SIZE_KB = "H264_FILE_SIZE_KB";
uint32_t NgnConfigurationEntry::DEF_H264_FILE_SIZE_KB = 0;

string NgnConfigurationEntry::VAD_LEN_MS = "VAD_LEN_MS";
uint32_t NgnConfigurationEntry::DEF_VAD_LEN_MS = 500;
string NgnConfigurationEntry::VAD_SILENCE_PERCENT = "VAD_SILENCE_PERCENT";
uint32_t NgnConfigurationEntry::DEF_VAD_SILENCE_PERCENT = 20;

string NgnConfigurationEntry::ANDROID_UNDER_OPENSLES = "ANDROID_UNDER_OPENSLES";
bool NgnConfigurationEntry::DEFAULT_ANDROID_UNDER_OPENSLES = true;

string NgnConfigurationEntry::BACKGROUND_MUSIC_DELAY = "BACKGROUND_MUSIC_DELAY";
#if defined(__APPLE__)
uint32_t NgnConfigurationEntry::DEFAULT_BACKGROUND_MUSIC_DELAY = 220;
#else
uint32_t NgnConfigurationEntry::DEFAULT_BACKGROUND_MUSIC_DELAY = 420;
#endif

string NgnConfigurationEntry::EXIT_COMMMODE_API_ENABLE = "EXIT_COMMMODE_API_ENABLE";
bool NgnConfigurationEntry::DEF_EXIT_COMMMODE_API_ENABLE = true;


// Network/*
string NgnConfigurationEntry::NETWORK_USE_WIFI = "NETWORK_USE_WIFI";
string NgnConfigurationEntry::NETWORK_USE_MOBILE = "NETWORK_USE_MOBILE";

string NgnConfigurationEntry::NETWORK_REDIRECT_ADD = "NETWORK_REDIRECT_ADD";
string NgnConfigurationEntry::NETWORK_REDIRECT_PORT = "NETWORK_REDIRECT_PORT";

string NgnConfigurationEntry::MAX_VIDEO_BITRATE_LEVEL_SERVER = "MAX_VIDEO_BITRATE_LEVEL_SERVER";
string NgnConfigurationEntry::MIN_VIDEO_BITRATE_LEVEL_SERVER = "MIN_VIDEO_BITRATE_LEVEL_SERVER";
string NgnConfigurationEntry::MAX_VIDEO_BITRATE_LEVEL_P2P = " MAX_VIDEO_BITRATE_LEVEL_P2P";

string NgnConfigurationEntry::MAX_RTCP_MEMBER_COUNT = "MAX_RTCP_MEMBER_COUNT";
uint32_t NgnConfigurationEntry::DEFAULT_MAX_RTCP_MEMBER_COUNT = 10;



// Media
string NgnConfigurationEntry::MEDIA_PROFILE = "MEDIA_PROFILE";// Security

//
// Default values
//

// Server
// 测试：120.24.68.210
// 运营：123.59.69.198
// 阿里测试：112.74.86.10
string NgnConfigurationEntry::DEFAULT_NETWORK_REDIRECT_ADD = "120.24.68.210";


// General

// For iOS, the system built-in ACE is already very good, no extra AEC needed
// For Android, our software AEC is quite bad, so close it by default.
#if defined(__APPLE__)
bool NgnConfigurationEntry::DEFAULT_GENERAL_AEC = false;
#else
bool NgnConfigurationEntry::DEFAULT_GENERAL_AEC = true;
#endif

uint32_t NgnConfigurationEntry::DEFAULT_GENERAL_AEC_OPT = 0;

bool NgnConfigurationEntry::DEFAULT_GENERAL_VAD = false;

bool NgnConfigurationEntry::DEFAULT_GENERAL_NR = true;

bool NgnConfigurationEntry::DEFAULT_GENERAL_AGC = true;

bool NgnConfigurationEntry::DEFAULT_GENERAL_CNG = false;

bool NgnConfigurationEntry::DEFAULT_GENERAL_HS = false;

bool NgnConfigurationEntry::DEFAULT_GENERAL_REV = false;

int NgnConfigurationEntry::DEFAULT_GENERAL_ECHO_TAIL = 100;

bool NgnConfigurationEntry::DEFAULT_GENERAL_USE_ECHO_TAIL_ADAPTIVE = true;

int NgnConfigurationEntry::DEFAULT_GENERAL_AGC_TARGET_LEVEL = 12;

int NgnConfigurationEntry::DEFAULT_GENERAL_AGC_COMPRESS_GAIN = 18;

int NgnConfigurationEntry::DEFAULT_SAMPLE_RATE = 16000;
int NgnConfigurationEntry::DEFAULT_RECORD_SAMPLE_RATE = 16000;
int NgnConfigurationEntry::DEFAULT_PLAYBACK_SAMPLE_RATE = 16000;

int NgnConfigurationEntry::DEFAULT_JITBUFFER_MARGIN = 100;

int NgnConfigurationEntry::DEFAULT_JITBUFFER_MAXLATERATE = 5;

string   NgnConfigurationEntry::MAX_CHANNEL_NUM = "MAX_CHANNEL_NUM";
uint32_t NgnConfigurationEntry::DEF_MAX_CHANNEL_NUM = 5;

string   NgnConfigurationEntry::CANAL_ID = "CANAL_ID";
string   NgnConfigurationEntry::DEFAULT_CANAL_ID = "YOUME";

string   NgnConfigurationEntry::VIDEO_STAT_REPORT_PERIOD_MS = "VIDEO_STAT_REPORT_PERIOD_MS";
int   NgnConfigurationEntry::DEFAULT_VIDEO_STAT_REPORT_PERIOD_MS = 60000;    //默认一分钟

// Network
bool NgnConfigurationEntry::DEFAULT_NETWORK_USE_WIFI = true;
bool NgnConfigurationEntry::DEFAULT_NETWORK_USE_3G = true;
int NgnConfigurationEntry::DEFAULT_NETWORK_REDIRECT_PORT = 5574;
string NgnConfigurationEntry::GENERAL_MCU_MODE = "MCU_MODE";

uint32_t NgnConfigurationEntry::DEFAULT_MAX_VIDEO_BITRATE_LEVEL_SERVER = 100; 
uint32_t NgnConfigurationEntry::DEFAULT_MIN_VIDEO_BITRATE_LEVEL_SERVER = 50; 
uint32_t NgnConfigurationEntry::DEFAULT_MAX_VIDEO_BITRATE_LEVEL_P2P = 110;

// Media
tmedia_profile_t NgnConfigurationEntry::DEFAULT_MEDIA_PROFILE = tmedia_profile_default;

tmedia_codec_id_t NgnConfigurationEntry::DEFAULT_MEDIA_CODECS = tmedia_codec_id_opus;


//C里面会用到下面的东西，直接通过服务器下发的

// OPUS, FEC, NetEQ
string NgnConfigurationEntry::OPUS_COMPLEXITY                   = "OPUS_COMPLEXITY";
int    NgnConfigurationEntry::DEFAULT_OPUS_COMPLEXITY           = 5;
string NgnConfigurationEntry::OPUS_ENABLE_VBR                   = "OPUS_ENABLE_VBR";
int    NgnConfigurationEntry::DEFAULT_OPUS_ENABLE_VBR           = 1;
string NgnConfigurationEntry::OPUS_MAX_BANDWIDTH                = "OPUS_MAX_BANDWIDTH";
int    NgnConfigurationEntry::DEFAULT_OPUS_MAX_BANDWIDTH        = 1105;
string NgnConfigurationEntry::OPUS_ENABLE_INBANDFEC             = "OPUS_ENABLE_INBANDFEC";
int    NgnConfigurationEntry::DEFAULT_OPUS_ENABLE_INBANDFEC     = 1;
string NgnConfigurationEntry::OPUS_ENABLE_OUTBANDFEC            = "OPUS_ENABLE_OUTBANDFEC";
int    NgnConfigurationEntry::DEFAULT_OPUS_ENABLE_OUTBANDFEC    = 0;
string NgnConfigurationEntry::OPUS_PACKET_LOSS_PERC             = "OPUS_PACKET_LOSS_PERC";
int    NgnConfigurationEntry::DEFAULT_OPUS_PACKET_LOSS_PERC     = 0;
string NgnConfigurationEntry::OPUS_DTX_PEROID                   = "OPUS_DTX_PEROID";
int    NgnConfigurationEntry::DEFAULT_OPUS_DTX_PEROID           = 100;
string NgnConfigurationEntry::OPUS_BITRATE                      = "OPUS_BITRATE";
int    NgnConfigurationEntry::DEFAULT_OPUS_BITRATE              = 25000;
string NgnConfigurationEntry::RTP_FEEDBACK_PERIOD               = "RTP_FEEDBACK_PERIOD";
int    NgnConfigurationEntry::DEF_RTP_FEEDBACK_PERIOD           = 0; // 0 means "do not send feedback" ,
                                                                     // 250 means sending feedback every 250 RTP packets

string   NgnConfigurationEntry::MAX_JITTER_BUFFER_NUM = "MAX_JITTER_BUFFER_NUM";
uint32_t NgnConfigurationEntry::DEF_MAX_JITTER_BUFFER_NUM = 10;
string   NgnConfigurationEntry::MAX_NETEQ_DELAY_MS = "MAX_NETEQ_DELAY_MS";
uint32_t NgnConfigurationEntry::DEF_MAX_NETEQ_DELAY_MS = 1000;



//数据上报配置
 string NgnConfigurationEntry::DATAREPORT_ADDR="DATAREPORT_ADDR";
 string NgnConfigurationEntry::DEFAULT_DATAREPORT_ADDR="192.168.0.1";
 string NgnConfigurationEntry::DATAREPORT_PORT="DATAREPORT_PORT";
 int NgnConfigurationEntry::DEFAULT_DATAREPORT_PORT=5574;

string NgnConfigurationEntry::DATAREPORT_MODE = "DATAREPORT_MODE";
int NgnConfigurationEntry::DEFAULT_DATAREPORT_MODE = 2;


//心跳配置
string NgnConfigurationEntry::HEART_TIMEOUT="HEART_TIMEOUT";
string NgnConfigurationEntry::HEART_TIMEOUT_COUNT="HEART_TIMEOUT_COUNT";
int NgnConfigurationEntry::DEFAULT_HEART_TIMEOUT=10;
int NgnConfigurationEntry::DEFAULT_TIMEOUT_COUNT=10;

//断线重连配置
string NgnConfigurationEntry::RECONNECT_COUNT = "RECONNECT_COUNT";
int NgnConfigurationEntry::DEFAULT_RECONNECT_COUNT=5;

string NgnConfigurationEntry::ENABLE = "Enable";
bool NgnConfigurationEntry::DEFAULT_ENABLE = true;

//自动重连
string NgnConfigurationEntry::AUTO_RECONNECT="AutoReconnect";
bool NgnConfigurationEntry::DEFAULT_AUTO_RECONNECT=true;


//回声消除配置参数
string NgnConfigurationEntry::AGC_MIN = "AGC_MIN";
int NgnConfigurationEntry::DEFAULT_AGC_MIN = 0;

string NgnConfigurationEntry::AGC_MAX = "AGC_MAX";
int NgnConfigurationEntry::DEFAULT_AGC_MAX = 255;

string NgnConfigurationEntry::ECHO_SKEW = "ECHO_SKEW";
int NgnConfigurationEntry::DEF_ECHO_SKEW = 0;

string NgnConfigurationEntry::COMMUNICATION_MODE = "COMM_MODE";
bool NgnConfigurationEntry::DEFAULT_COMMUNICATION_MODE = true;

string NgnConfigurationEntry::COMMUNICATION_MODE_ORDER = "COMM_ORDER";
int NgnConfigurationEntry::DEFAULT_COMMUNICATION_MODE_ORDER = COMMMODE_BEFORE;

//变音处理
string NgnConfigurationEntry::SOUNDTOUCH_ENABLED = "Soundtouch_Enabled";
int NgnConfigurationEntry::DEFAULT_SOUNDTOUCH_ENABLED = 0;

string NgnConfigurationEntry::SOUNDTOUCH_TEMPO = "Soundtouch_Tempo";
int NgnConfigurationEntry::DEFAULT_SOUNDTOUCH_TEMPO = 100;

string NgnConfigurationEntry::SOUNDTOUCH_RATE = "Soundtouch_Rate";
int NgnConfigurationEntry::DEFAULT_SOUNDTOUCH_RATE = 100;

string NgnConfigurationEntry::SOUNDTOUCH_PITCH = "Soundtouch_Pitch";
int NgnConfigurationEntry::DEFAULT_SOUNDTOUCH_PITCH = 70;



string NgnConfigurationEntry::UPDATE="Update";
bool NgnConfigurationEntry::DEFAULT_UPDATE=false;

string NgnConfigurationEntry::UPDATE_URL="UpdateURL";
string NgnConfigurationEntry::DEFAULT_UPDATE_URL="";

string NgnConfigurationEntry::UPDATE_MD5="UpdateMD5";
string NgnConfigurationEntry::DEFAULT_UPDATE_MD5="";

/*
string NgnConfigurationEntry::UPLOAD_LOG="UploadLog";
int NgnConfigurationEntry::DEFAULT_UPLOAD_LOG=0;

 string NgnConfigurationEntry::UPLOAD_LOG_IP="UploadLogIP";
 string NgnConfigurationEntry::DEFAULT_UPLOAD_LOG_IP="123.59.51.6";

 string NgnConfigurationEntry::UPLOAD_LOG_PORT="UploadLogPort";
 int NgnConfigurationEntry::DEFAULT_UPLOAD_LOG_PORT=6008;

*/

string NgnConfigurationEntry::VERIFY_TIME="VERIFY_TIME";
unsigned int NgnConfigurationEntry::DEFAULT_VERIFY_TIME=tsk_gettimeofday_ms()/1000;

// Apply an extra gain on output audio volume.
// The gain is a real number(0.1 - 10.0), and is scaled to integer by a factor 100.
// For example, a number 110 here actually stands for 110/100 = 1.1
string NgnConfigurationEntry::ANDROID_OUTPUT_VOLUME_GAIN = "ANDROID_OUT_VOL_GAIN";
int    NgnConfigurationEntry::DEFAULT_ANDROID_OUTPUT_VOLUME_GAIN = 100;

string NgnConfigurationEntry::IOS_OUTPUT_VOLUME_GAIN = "IOS_OUT_VOL_GAIN";
int    NgnConfigurationEntry::DEFAULT_IOS_OUTPUT_VOLUME_GAIN = 100;

string NgnConfigurationEntry::MIC_VOLUME_GAIN = "MIC_VOL_GAIN";
int    NgnConfigurationEntry::DEF_MIC_VOLUME_GAIN = 100;

string NgnConfigurationEntry::FADEIN_TIME_MS = "FADEIN_TIME_MS";
uint32_t NgnConfigurationEntry::DEF_FADEIN_TIME_MS = 3000;

////////////////////////////////////////////////////////////////////////////
// iOS specific configurations.
string NgnConfigurationEntry::IOS_NO_PERMISSION_NO_REC = "IOS_NO_PERM_NO_REC";
bool   NgnConfigurationEntry::DEF_IOS_NO_PERMISSION_NO_REC = true;
string NgnConfigurationEntry::IOS_AIRPLAY_NO_REC = "IOS_AIRPLAY_NO_REC";
bool   NgnConfigurationEntry::DEF_IOS_AIRPLAY_NO_REC = true;
// In iOS, set the AVAudioSession category to AVAudioSessionCategoryPlayback on init(),
// so that the ring/silent switch has the same behavior during the whole app life time.
string NgnConfigurationEntry::IOS_ALWAYS_PLAY_CATEGORY = "IOS_ALWAYS_PLAY_CATEGORY";
bool   NgnConfigurationEntry::DEF_IOS_ALWAYS_PLAY_CATEGORY = true;
string NgnConfigurationEntry::IOS_NEED_SET_MODE = "IOS_NEED_SET_MODE";
bool NgnConfigurationEntry::DEF_IOS_NEED_SET_MODE = true;
string NgnConfigurationEntry::IOS_RECOVERY_DELAY_FROM_CALL = "IOS_RECOVERY_DELAY_FROM_CALL";
uint32_t NgnConfigurationEntry::DEF_IOS_RECOVERY_DELAY_FROM_CALL = 0;


// Log files
// Avialable since build number #2407
string NgnConfigurationEntry::UPLOAD_LOG = "UPLOAD_LOG";
bool   NgnConfigurationEntry::DEF_UPLOAD_LOG = false;
string NgnConfigurationEntry::UPLOAD_LOG_FOR_USER = "UPLOAD_LOG_FOR_USER";
string NgnConfigurationEntry::DEF_UPLOAD_LOG_FOR_USER = "";
// Available since 2.4.3.2465
string   NgnConfigurationEntry::LOG_LEVEL_CONSOLE = "LOG_LEVEL_CONSOLE";
uint32_t NgnConfigurationEntry::DEF_LOG_LEVEL_CONSOLE = TSK_LOG_INFO;
string   NgnConfigurationEntry::LOG_LEVEL_FILE = "LOG_LEVEL_FILE";
uint32_t NgnConfigurationEntry::DEF_LOG_LEVEL_FILE = TSK_LOG_INFO;
string   NgnConfigurationEntry::LOG_FILE_SIZE_KB = "LOG_FILE_SIZE_KB";
uint32_t NgnConfigurationEntry::DEF_LOG_FILE_SIZE_KB = 10240; //Default to 10MB
string   NgnConfigurationEntry::PACKET_STAT_LOG_PERIOD_MS = "PACKET_STAT_LOG_PERIOD_MS";
uint32_t NgnConfigurationEntry::DEF_PACKET_STAT_LOG_PERIOD_MS = (30*1000);
string   NgnConfigurationEntry::PACKET_STAT_REPORT_PERIOD_MS = "PACKET_STAT_REPORT_PERIOD_MS";
uint32_t NgnConfigurationEntry::DEF_PACKET_STAT_REPORT_PERIOD_MS = (0*1000);

string   NgnConfigurationEntry::AV_QOS_STAT_REPORT_PERIOD_MS = "AV_QOS_STAT_REPORT_PERIOD_MS";
uint32_t NgnConfigurationEntry::DEF_AV_QOS_STAT_REPORT_PERIOD_MS = (10*1000);

// Video part
string NgnConfigurationEntry::CAMERA_ROTATION = "CAMERA_ROTATION";
int NgnConfigurationEntry::DEFAULT_CAMERA_ROTATION = 90;
string NgnConfigurationEntry::VIDEO_SIZE_INDEX = "VIDEO_SIZE_INDEX";
int NgnConfigurationEntry::DEFAULT_VIDEO_SIZE_INDEX = 2;
string NgnConfigurationEntry::VIDEO_ALLOW_FIX_QUALITY = "VIDEO_ALLOW_FIX_QUALITY";
bool   NgnConfigurationEntry::DEF_VIDEO_ALLOW_FIX_QUALITY = false;

string NgnConfigurationEntry::CAMERA_CAPTURE_MODE = "CAMERA_CAPTURE_MODE";
int NgnConfigurationEntry::DEF_CAMERA_CAPTURE_MODE = 1;

string NgnConfigurationEntry::VIDEO_BITRATE_MAX_MULTIPLE = "VIDEO_BITRATE_MULTIPLE";
uint32_t NgnConfigurationEntry::DEFAULT_VIDEO_BITRATE_MAX_MULTIPLE = 5;

string NgnConfigurationEntry::VIDEO_FRAME_REFREASH_TIME = "VIDEO_FRAME_REFRESH_TIME";
uint32_t NgnConfigurationEntry::DEFAULT_VIDEO_FRAME_REFREASH_TIME = 500;

// AVRunTimeMonitor
string   NgnConfigurationEntry::AVMONITOR_CONSUMER_PEROID_MS = "AVMONITOR_CONSUMER_PEROID_MS";
uint32_t NgnConfigurationEntry::DEF_AVMONITOR_CONSUMER_PEROID_MS = 0;
string   NgnConfigurationEntry::AVMONITOR_PRODUCER_PEROID_MS = "AVMONITOR_PRODUCER_PEROID_MS";
uint32_t NgnConfigurationEntry::DEF_AVMONITOR_PRODUCER_PEROID_MS = 0;

//translate
string NgnConfigurationEntry::CONFIG_TRANSLATE_ENABLE = "TRANSLATE_ENABLE";
string NgnConfigurationEntry::CONFIG_TRANSLATE_HOST = "TRANSLATE_HOST";
string NgnConfigurationEntry::CONFIG_TRANSLATE_CGI = "TRANSLATE_CGI";
string NgnConfigurationEntry::CONFIG_TRANSLATE_REGULAR = "TRANSLATE_REGULAR";

int NgnConfigurationEntry::TRANSLATE_SWITCH_DEFAULT = 0 ;
int NgnConfigurationEntry::TRANSLATE_METHOD_DEFAULT = 0 ; // google翻译调用方法，0 - 破解版， 1 - 付费版

string NgnConfigurationEntry::CONFIG_TRANSLATE_METHOD = "TRANSLATE_METHOD" ;
string NgnConfigurationEntry::CONFIG_TRANSLATE_HOST_V2 ="TRANSLATE_HOST_V2";
string NgnConfigurationEntry::CONFIG_TRANSLATE_CGI_V2 = "TRANSLATE_CGI_V2";
string NgnConfigurationEntry::CONFIG_TRANSLATE_GOOGLE_APIKEY = "TRANSLATE_GOOGLE_APIKEY" ;

string NgnConfigurationEntry::TRANSLATE_HOST_DEFAULT = "https://translate.google.com";
string NgnConfigurationEntry::TRANSLATE_CGI_PATH_DEFAULT = "/translate_a/";
string NgnConfigurationEntry::TRANSLATE_HOST_DEFAULT_V2 = "https://translation.googleapis.com";
string NgnConfigurationEntry::TRANSLATE_CGI_PATH_DEFAULT_V2 = "/language/translate/";

string NgnConfigurationEntry::TRANSLATE_GOOGLE_APIKEY_DEFAULT = "AIzaSyDv8POwQOtuIp6dIMxRmqL4RVNvMicaGy0";
string NgnConfigurationEntry::TRANSLATE_REGULAR_DEFAULT = ".*TKK\\=eval\\(\\'\\(\\(function\\(\\)\\{var\\s+a\\\\x3d(-?\\d+);var\\s+b\\\\x3d(-?\\d+);return\\s+(\\d+)\\+.*";




