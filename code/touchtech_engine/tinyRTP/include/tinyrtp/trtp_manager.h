/**@file trtp_manager.h
 * @brief RTP/RTCP manager.
 */
#ifndef TINYRTP_MANAGER_H
#define TINYRTP_MANAGER_H

#include "tinyrtp_config.h"
#include "tinyrtp/rtp/trtp_rtp_packet.h"
#include "tinyrtp/rtp/trtp_rtp_session.h"
#include "tinyrtp/rtcp/trtp_rtcp_session.h"
#include "tinyrtp/rtp/trtp_rtp_header.h"

#include "tinymedia/tmedia_defaults.h"

#include "tinynet.h"

TRTP_BEGIN_DECLS

#define TRTP_H264_MP 105
#define TRTP_OPUS_MP 111

#define TRTP_CUSTOM_FORMAT 126

struct trtp_rtp_packet_s;

typedef int (*video_adjust_cb_f)(const void* callback_data, int width, int height, int bitrate, int fps);
typedef int (*video_route_cb_f)(const void* callback_data, int type, void *data, int error);

typedef enum trtp_video_stream_status_s
{
    TRTP_VIDEO_STEAM_NORMAL                 = 0, // 正常发送视频大小流
    TRTP_VIDEO_STEAM_MINOR                  = 1, // 仅发送视频小流
    TRTP_VIDEO_STEAM_UNKNOW                 = 10,  // unknow status

} trtp_video_stream_status_s;

typedef enum trtp_video_adjust_mode_s
{
    TRTP_VIDEO_ADJUST_MODE_DECREASE = 0,    // decrease (bitrate/fps/resolution)
    TRTP_VIDEO_ADJUST_MODE_INCREASE = 1,    // increase (bitrate/fps/resolution)
} trtp_video_adjust_mode_s;

typedef enum trtp_video_stream_type_s
{
    TRTP_VIDEO_STREAM_TYPE_MAIN = 0,
    TRTP_VIDEO_STREAM_TYPE_MINOR = 1,
    TRTP_VIDEO_STREAM_TYPE_SHARE = 2,
} trtp_video_stream_type_s;

typedef enum trtp_trans_route_type_s
{
    TRTP_TRANS_ROUTE_TYPE_UNKNOE = 0,
    TRTP_TRANS_ROUTE_TYPE_P2P    = 1,   // p2p通路传输
    TRTP_TRANS_ROUTE_TYPE_SERVER = 2,   // server转发
}trtp_trans_route_type_t;

typedef enum trtp_route_event_type_s
{
    TRTP_ROUTE_EVENT_TYPE_UNKNOW        = 0,
    TRTP_ROUTE_EVENT_TYPE_P2P_SUCCESS   = 1,  // p2p 通路检测成功
    TRTP_ROUTE_EVENT_TYPE_P2P_FAIL      = 2,  // p2p 通路检测失败，走server转发
    TRTP_ROUTE_EVENT_TYPE_P2P_CHANGE    = 3,  // p2p 切换到server转发
} trtp_route_event_type_t;

//用来控制发包，丢包，强制发送等
typedef struct send_packet_status_s
{
    TSK_DECLARE_OBJECT;
    tsk_boolean_t bLossPacket; //上一次是否丢了包
    uint8_t LastFrameType; //上一次包类型 。 I帧或者P 帧
    uint8_t  VideoID;
    uint16_t LastFrameSerial; //上一次的帧序号
} send_packet_status_t;

// 用于数据上报统计发送端丢包率（半程）
typedef struct trtp_send_report_stat_s
{
    uint32_t audio_send_total_count;
    uint32_t audio_send_loss_count;

    uint32_t video_send_total_count;
    uint32_t video_send_loss_count;
} trtp_send_report_stat_t;

/** RTP/RTCP manager */
typedef struct trtp_manager_s
{
    TSK_DECLARE_OBJECT;

    char *local_ip;
    tsk_bool_t use_ipv6;
    tsk_bool_t is_started;
	tsk_bool_t use_tcp;
    tsk_bool_t use_rtcp;
    tsk_bool_t use_rtcpmux;
    tsk_bool_t is_socket_disabled;
   
    // tsk_bool_t is_force_symetric_rtp;
    // tsk_bool_t is_symetric_rtp_checked;
    // tsk_bool_t is_symetric_rtcp_checked;
    int32_t app_bw_max_upload;   // application specific (kbps)
    int32_t app_bw_max_download; // application specific (kbps)
	float app_jitter_cng; // application specific jitter buffer congestion estimation (quality in ]0, 1], 1f meaning no congestion)

    int32_t feedback_period;     // RTP feedback period measured in packet number.
                                 // e.g. if it's 100, then the feedback will be sent for every 100 packets.
    
    tnet_transport_t *transport;        // 用于server中转

    tsk_bool_t is_p2p_socket_disabled;
    tnet_transport_t *transport_p2p;    // 用于p2p传输

    tsk_bool_t is_report_socket_disabled;
    tnet_transport_t *transport_report;     //专门用于数据上报

    xt_timer_manager_handle_t *timer_mgr_global;

    // p2p route检测结束标志，在房间仅有两人时启动检测
    uint64_t   p2p_route_check_startms;
    tsk_bool_t p2p_route_check_end;
    tsk_bool_t p2p_route_enable;
    tsk_bool_t p2p_route_change_flag;
    trtp_trans_route_type_t curr_trans_route;
    
    struct {
        tsk_bool_t  p2p_info_set_flag;
        char *      p2p_local_ip;
        uint16_t    p2p_local_port;
        char *      p2p_remote_ip;
        uint16_t    p2p_remote_port;
        struct sockaddr_storage p2p_remote_addr;

        // route check
        long route_check_timerid;
        uint32_t    p2p_ping_seq;
        uint32_t    p2p_pong_seq;
        
        uint64_t    p2p_last_send_ping_time;
        uint64_t    p2p_last_recv_pong_time;
        uint64_t    p2p_last_recv_ping_time;

        uint32_t    p2p_route_check_loss;   // pong连续丢包个数
    } p2p_connect_info;

    struct {
        tmedia_rtcweb_type_t local;
        tmedia_rtcweb_type_t remote;
    } rtcweb_type;

    struct {
        uint16_t start;
        uint16_t stop;
    } port_range;

    struct {
        uint16_t seq_num;
        uint32_t timestamp;
        uint8_t payload_type;
        int32_t dscp;

        char *remote_ip;
        tnet_port_t remote_port;
        struct sockaddr_storage remote_addr;

        char *public_ip;
        tnet_port_t public_port;

        struct {
            uint32_t local;
            uint32_t remote;
        } ssrc;

        struct {
            const void *usrdata;
            trtp_rtp_cb_f fun;
        } cb;
        
        // video rtp数据回调结构体，由于video和audio使用同个端口同个rtp对象，故不改变audio结构逻辑基础上增加video rtp回调结构
        struct
        {
            const void *usrdata;
            trtp_rtp_cb_f fun;
        } video_cb;

        // for switch video source (main/minor)
        struct 
        {
            const void *usrdata;
            video_switch_cb_f fun;
        } video_switch_cb;

        uint16_t video_seq_num;
        uint16_t video_segment_seq_num;
        uint16_t video_first_packet_seq_num;
        uint32_t video_timestamp;
        uint8_t  video_payload_type;
        
        uint16_t video_segment_seq_num_second;  //第二路流
        uint16_t video_first_packet_seq_num_second;

        uint16_t video_segment_seq_num_share;  // 共享流
        uint16_t video_first_packet_seq_num_share;

        struct {
            void *ptr;
            tsk_size_t size;
        } serial_buffer;
        
        tsk_boolean_t is_video_pause;
        tsk_boolean_t is_audio_pause;
    } rtp;
    

    //report,ctrl数据走的udp
    struct {
        char *remote_ip;
        tnet_port_t remote_port;
        struct sockaddr_storage remote_addr;
        
    }report;

    struct {
        char* cname;
        char* remote_ip;
        tnet_port_t remote_port;
        struct sockaddr_storage remote_addr;
        tnet_socket_t* local_socket;

        struct {
			char* ip;
			tnet_port_t port;
			tnet_socket_type_t type;
		} public_addr;

        struct {
            const void* usrdata;
            trtp_rtcp_cb_f fun;
        } cb;

        struct trtp_rtcp_session_s* session;
    } rtcp;
    

    struct{
        struct {
            const void* usrdata;
            trtp_rtcp_cb_f fun;
        } jb_cb;
        
    }jb;
	
    // Statictics for sending data.
    // These can only accessed by the sending thread.
    struct
    {
        uint64_t start_time_ms;     // Start time of a new round of bit rate calculation.
        uint64_t bytes_sent;        // Number of bytes sent during one round of bit rate calculation.
        uint64_t total_time_ms;     // Total time elapsed since sending the first packet.
        uint64_t total_bytes_sent;  // Total number of bytes sent since sending the first packet.
        uint64_t packet_count;      // Total number of packets sent since sending the first packet.
        
        uint64_t video_start_time_ms;   // video: Start time of a new round of bit rate calculation.
        uint64_t video_total_time_ms;   // video: Total time elapsed since sending the first packet.
        uint64_t video_bytes_sent;      // video: Number of bytes sent during one round of bit rate calculation.
        uint64_t video_total_bytes_sent;      // video: Total number of bytes sent since sending the first packet.
        uint64_t video_packet_count;    // video: Total number of packets sent since sending the first packet.

    } send_stats;
  
    struct
    {
        trtp_rtp_packet_t* head;
        trtp_rtp_packet_t* newest;
        uint32_t playing_time;
        uint64_t last_time_of_day_ms;
        uint32_t rtp_clock_rate;
        int64_t timestamp_threshold_drop;
        int64_t timestamp_threshold_wait;
        
        // for debug
        tsk_bool_t is_recording_time_set;
        tsk_bool_t got_first_packet;
        tsk_bool_t match_first_packet;
        
        TSK_DECLARE_SAFEOBJ;
    } packet_queue;
  
    int sessionid;
    uint64_t last_time_recv_packet_ms;
    uint64_t last_time_send_dummy_packet_ms;
    uint64_t last_time_print_recv_packet_ms;

    tsk_semaphore_handle_t* rtp_send_sema;
    tsk_bool_t rtp_send_thread_running;
    tsk_thread_handle_t *rtp_send_thread_handle;
    tsk_list_t* rtp_packets_list;
    
    tsk_list_t* sorts_list;
    
    //被插件引用的次数，audio，video 等等 joexie
    int iPluginRefCount;
    
    
    TSK_DECLARE_SAFEOBJ;
    
    long media_ctl_timerid;
    long lossrate_check_timerid;
    
    long packet_total;//一次统计包总数
    long packet_loss;//一次统计的丢包数
    int  iStaticCount;//统计的次数
    int  iUpBitrateStaticCount;//统计的次数
    
    int  iLastVideoBitrate;  // 大流最后设置的码率调节系数(百分比)
    int  iLastVideoFps; // 大流最后设置的帧率调节系数(百分比)
    int  iLastNoLossBitrate;//最近一次没丢包的码率
    int  iUpBitrateLevel;//每次码率提升百分比，0-100
    int  iMinBitRatePercent;//最小码率
    int  iLastVideoFrame;
    tsk_bool_t bUpBitrate;
    
    int  iLastMainVideoSize;        // 大流最后设置的分辨率调节系数(百分比)
    int  iLastvideoSendStatus;      // 上一次视频大小流发送状态
    int  ivideoSendStatus;          // 视频大小流发送状态
    int  iLastMinorVideoBitrate;    // 小流最后设置的码率调节系数(百分比)
    int  iLastMinorVideoFps;        // 小流最后的帧率调节系数(百分比)
    // int  iLastMinorNoLossBitrate;   // 小流最近一次没丢包的码率 
    uint64_t lastAdjustTimeMs;     // 上一次调整码率时间时间
    int  iForceSendStatus;          // 上层指定发送发送模式

    //为了统计发送音视频数据的通路是否畅通，即半程丢包统计
    int isSendDataToServer; // 是否有向服务器发送半程丢包统计请求
    int isSendDataReceived; // 是否有收到服务器反馈的半程统计数据，目前仅统计视频
    int iLastRtpPass;       // 之前是通的还是不通的
    
    int iHigherLossrateCount;   //上行连续高丢包次数, 大于20%
    int iLastDataRtt;
    int iLastDeltaRtt;

    int iPeakLossCount; // 突发丢包，超过70%

    // 下行带宽统计
    uint64_t recvDataStat;
    uint64_t recvLastTickStart;
    uint64_t recvInstantBitrate;

    tsk_list_t* send_packets_status_list;
    
    //控制发包间隔
    tsk_cond_handle_t*   send_thread_cond;

	//使用TCP 连接是，需要将收到的包缓存起来，然后根据头部2字节来解包
	void* tcpPacketBuffer;

	//视频加密使用的密码
	char* pszEncryptPasswd;

    // 本端视频qos调整参数回调
    struct 
    {
        const void *usrdata;
        video_adjust_cb_f fun;
    } video_encode_adjust_cb;

    struct 
    {
        const void *usrdata;
        video_route_cb_f fun;
    }trtp_route_cb;
    
    int iDataSendFlag;  // 是否发送rtp数据，目前在重连阶段停止发送数据, 默认发送

    uint64_t uBusinessId; //华为传输承载增加的消息中包含BusinessId字段

    trtp_send_report_stat_t report_stat;
} trtp_manager_t;

TINYRTP_API send_packet_status_t* trtp_manager_get_send_status(trtp_manager_t *self,uint8_t videoid);

TINYRTP_API int trtp_manager_addPlugRefCount(trtp_manager_t *self);
TINYRTP_API int trtp_manager_subPlugRefCount(trtp_manager_t *self);


TINYRTP_API trtp_manager_t *trtp_manager_create ( const char *local_ip, tsk_bool_t use_ipv6, int sessionid);

TINYRTP_API int trtp_manager_prepare (trtp_manager_t *self);
TINYRTP_API tsk_bool_t trtp_manager_is_ready (trtp_manager_t *self);
TINYRTP_API int trtp_manager_set_rtp_callback (trtp_manager_t *self, trtp_rtp_cb_f fun, const void *usrdata);

// video接口相关, video和audio使用同个rtp对象，在不改变audio逻辑基础上增加video相关接口
// 设置rtp video回调接口
TINYRTP_API int trtp_manager_set_rtp_video_callback(trtp_manager_t *self, trtp_rtp_cb_f fun, const void *usrdata);
TINYRTP_API int trtp_manager_set_rtcp_callback(trtp_manager_t* self, trtp_rtcp_cb_f fun, const void* usrdata);
TINYRTP_API int trtp_manager_set_switch_video_callback(trtp_manager_t *self, video_switch_cb_f fun, const void *usrdata);
TINYRTP_API int trtp_manager_set_video_adjust_callback(trtp_manager_t *self, video_adjust_cb_f fun, const void *usrdata);
TINYRTP_API int trtp_manager_set_video_route_callback(trtp_manager_t *self,  video_route_cb_f fun, const void *usrdata);

//设置jb回调接口
TINYRTP_API int trtp_manager_set_jb_callback(trtp_manager_t* self, trtp_rtcp_jb_cb_f fun, const void* usrdata);

// 设置video playload type
TINYRTP_API int trtp_manager_set_video_payload_type (trtp_manager_t *self, uint8_t payload_type);
TINYRTP_API trtp_rtp_packet_t* trtp_manager_create_packet(trtp_manager_t *self, const void *data, tsk_size_t size,
                                                          const void *extension, tsk_size_t ext_size,
                                                          uint32_t duration, tsk_bool_t iFrame, tsk_bool_t marker);
TINYRTP_API trtp_rtp_packet_t* trtp_manager_create_packet_new(trtp_manager_t *self, const void *data, tsk_size_t size,
                                                              const void *extension, tsk_size_t ext_size, uint64_t timestamp, tsk_bool_t iFrame, tsk_bool_t marker);
// video数据发送接口
TINYRTP_API tsk_size_t trtp_manager_send_video_rtp_with_extension(trtp_manager_t *self,
                                                                  const void *data,
                                                                  tsk_size_t size,
                                                                  uint32_t duration,
                                                                  tsk_bool_t marker,
                                                                  tsk_bool_t last_packet,
                                                                  const void *extension,
                                                                  tsk_size_t ext_size,
                                                                  tsk_bool_t iFrame,
                                                                  tsk_bool_t repeat);

TINYRTP_API int trtp_manager_set_rtp_dscp (trtp_manager_t *self, int32_t dscp);
TINYRTP_API int trtp_manager_set_payload_type (trtp_manager_t *self, uint8_t payload_type);
TINYRTP_API int trtp_manager_set_rtp_remote (trtp_manager_t *self, const char *remote_ip, tnet_port_t remote_port);
TINYRTP_API int trtp_manager_set_p2p_remote (trtp_manager_t *self, const char *local_ip, tnet_port_t local_port, const char *remote_ip, tnet_port_t remote_port);
TINYRTP_API int trtp_manager_set_rtcp_remote (trtp_manager_t *self, const char *remote_ip, tnet_port_t remote_port);

TINYRTP_API int trtp_manager_set_port_range (trtp_manager_t *self, uint16_t start, uint16_t stop);
TINYRTP_API int trtp_manager_set_rtcweb_type_remote (trtp_manager_t *self, tmedia_rtcweb_type_t rtcweb_type);
TINYRTP_API int trtp_manager_start (trtp_manager_t *self);
TINYRTP_API tsk_bool_t trtp_manager_send_rtp_dummy (trtp_manager_t *self);
TINYRTP_API tsk_size_t trtp_manager_send_rtp_with_extension (trtp_manager_t *self, const void *data, tsk_size_t size, uint32_t duration,
                                                        tsk_bool_t marker, tsk_bool_t last_packet, const void *extension, tsk_size_t ext_size);



TINYRTP_API trtp_rtp_packet_t* trtp_manager_gen_rtp_with_extension(trtp_manager_t *self, const void *data, tsk_size_t size, uint32_t duration,
                                                                tsk_bool_t marker, tsk_bool_t last_packet, const void *extension, tsk_size_t ext_size);

TINYRTP_API tsk_bool_t trtp_manager_remove_member(trtp_manager_t *self, uint32_t sessionid);
//发送自定义的数据
TINYRTP_API tsk_size_t trtp_manager_send_custom_data(trtp_manager_t *self, const void *data, tsk_size_t size,uint64_t timeSpan, uint32_t uRecvSessionId);


TINYRTP_API tsk_size_t trtp_manager_send_rtp (trtp_manager_t *self, const void *data, tsk_size_t size, uint32_t duration, tsk_bool_t marker, tsk_bool_t last_packet);
TINYRTP_API tsk_size_t trtp_manager_send_rtp_packet (trtp_manager_t *self, const struct trtp_rtp_packet_s *packet, tsk_bool_t bypass_encrypt);
TINYRTP_API tsk_size_t trtp_manager_send_rtp_raw (trtp_manager_t *self, const void *data, tsk_size_t size);
TINYRTP_API uint32_t   trtp_manager_get_current_timestamp (trtp_manager_t *self);
TINYRTP_API void       trtp_manager_set_recording_time_ms (trtp_manager_t *self, uint32_t time_ms, uint32_t rtp_clock_rate);
TINYRTP_API void       trtp_manager_set_playing_time_ms (trtp_manager_t *self, uint32_t time_ms, uint32_t rtp_clock_rate);
TINYRTP_API int trtp_manager_set_app_bandwidth_max (trtp_manager_t *self, int32_t bw_upload_kbps, int32_t bw_download_kbps);
TINYRTP_API int trtp_manager_signal_pkt_loss (trtp_manager_t *self, uint32_t ssrc_media, const uint16_t *seq_nums, tsk_size_t count);
TINYRTP_API int trtp_manager_signal_frame_corrupted (trtp_manager_t *self, uint32_t ssrc_media);
TINYRTP_API int trtp_manager_signal_jb_error (trtp_manager_t *self, uint32_t ssrc_media);
TINYRTP_API int trtp_manager_stop (trtp_manager_t *self);
TINYRTP_API int trtp_manager_push_rtp_packet (trtp_manager_t *self, trtp_rtp_packet_t *packet);
TINYRTP_API uint64_t trtp_manager_get_video_duration (trtp_manager_t *self);
TINYRTP_GEXTERN const tsk_object_def_t *trtp_manager_def_t;

TINYRTP_API tsk_size_t trtp_manager_send_rtcp_data (trtp_manager_t *self, const void *data, tsk_size_t size);

TINYRTP_API int trtp_manager_get_source_info(trtp_manager_t* self, uint32_t ssrc, uint32_t* sendstatus, uint32_t* source_type);
TINYRTP_API int trtp_manager_set_source_type(trtp_manager_t* self, uint32_t ssrc, uint32_t source_type);

TINYRTP_API int trtp_manager_set_send_status(trtp_manager_t* self, uint32_t sendstatus);

TINYRTP_API void trtp_manager_set_send_flag(trtp_manager_t* self, uint32_t send_flag);

TINYRTP_API void trtp_manager_get_recv_stat(trtp_manager_t* self, uint64_t* recvStat);

TINYRTP_API void start_media_ctl( trtp_manager_t *self );
TINYRTP_API void stop_media_ctl(  trtp_manager_t *self );
int onMediaCtlTimer(const void*arg,long timerid);
void onRecvMediaCtlData( trtp_manager_t *self, char* buff, int len );

void trtp_manager_start_route_check_timer( trtp_manager_t *self );
void trtp_manager_stop_route_check_timer( trtp_manager_t *self );

void trtp_manager_set_business_id(trtp_manager_t* self, uint64_t business_id);

int trtp_manager_get_report_stat(trtp_manager_t *self, trtp_send_report_stat_t* stat);
int trtp_manager_reset_report_stat(trtp_manager_t *self);

TRTP_END_DECLS

#endif /* TINYRTP_MANAGER_H */
