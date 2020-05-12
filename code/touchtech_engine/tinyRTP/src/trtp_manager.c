
/**@file trtp_manager.c
* @brief RTP/RTCP manager.
*
*/
#include "tinyrtp/trtp_manager.h"

#include "tinyrtp/rtp/trtp_rtp_packet.h"
#include "tinyrtp/rtcp/trtp_rtcp_packet.h"
#include "tinyrtp/rtcp/trtp_rtcp_session.h"

#include "tsk_base64.h"
#include "tsk_debug.h"
#include "tsk_md5.h"
#include "tsk_memory.h"
#include "tsk_string.h"
#include "tmedia_utils.h"
#include "trtp_sort.h"
#include <assert.h>
#include <limits.h> /* INT_MAX */
#if !TRTP_UNDER_WINDOWS
#include <netinet/ip.h>
#endif

#include "XConfigCWrapper.hpp"
#define MAX_SEND_LIST_COUNT 1000

#if !defined(TRTP_TRANSPORT_NAME)
#define TRTP_TRANSPORT_NAME "RTP/RTCP Manager"
#endif

#if !defined(P2P_TRTP_TRANSPORT_NAME)
#define P2P_TRTP_TRANSPORT_NAME "P2P RTP/RTCP Manager"
#endif

#if !defined(REPORT_TRTP_TRANSPORT_NAME)
#define REPORT_TRTP_TRANSPORT_NAME "Report RTP/RTCP Manager"
#endif

#if !defined(TRTP_DISABLE_SOCKETS_BEFORE_START)
#define TRTP_DISABLE_SOCKETS_BEFORE_START 0
#endif
#if !defined(TRTP_TINY_RCVBUF)
#define TRTP_TINY_RCVBUF (256 >> 1 /*Will be doubled and min on linux is 256*/) /* tiny buffer used to disable receiving */
#endif

#if !defined(TRTP_DSCP_RTP_DEFAULT)
#define TRTP_DSCP_RTP_DEFAULT /* 0x2e */ 0x00
#endif

#if !defined(TRTP_PORT_RANGE_START)
#define TRTP_PORT_RANGE_START 1024
#endif
#if !defined(TRTP_PORT_RANGE_STOP)
#define TRTP_PORT_RANGE_STOP 65535
#endif

#if !defined(TRTP_DTLS_HANDSHAKING_TIMEOUT)
#define TRTP_DTLS_HANDSHAKING_TIMEOUT 1000
#endif
#if !defined(TRTP_DTLS_HANDSHAKING_TIMEOUT_MAX)
#define TRTP_DTLS_HANDSHAKING_TIMEOUT_MAX (TRTP_DTLS_HANDSHAKING_TIMEOUT << 20)
#endif

#define TRTP_HALF_RTCP_REPORT_TIME  2000
#define TRTP_CHECK_LOSSRATE_TIME    1000

#define TRTP_P2P_ROUTE_CHECK_TIME 500
#define TRTP_P2P_PING_COUNT_QPS   5

#define DUMP_RTP_HEAD_EXTENSION 0
#define DUMP_SERIALIZE_DATA 0

#define   trtp_max(A,B)  (A)>(B)?(A):(B)

int onPacketLostRateCheckTimer( const void*arg, long timerid );
void lossRateProcessForTest(const void* arg, long timerid);
void lossRateProcessForBwe(const void* arg, long timerid);
static int _trtp_manager_recv_data (const trtp_manager_t *self, const uint8_t *data_ptr, tsk_size_t data_size, tnet_fd_t local_fd, const struct sockaddr_storage *remote_addr);
static void _trtp_manager_packet_queue_push (trtp_manager_t *self, trtp_rtp_packet_t* new_packet);
static trtp_rtp_packet_t* _trtp_manager_packet_queue_pop (trtp_manager_t *self, uint64_t time_of_day_ms);

#define _trtp_manager_is_rtcpmux_active(self) \
    ((self) && ((self)->use_rtcpmux && (!(self)->rtcp.local_socket || ((self)->transport && (self)->transport->master && (self)->transport->master->fd == (self)->rtcp.local_socket->fd))))

#define _trtp_manager_send_turn_dtls_rtp(ice_ctx, handshaking_data_ptr, handshaking_data_size) \
    _trtp_manager_send_turn_dtls ((ice_ctx), (handshaking_data_ptr), (handshaking_data_size), /*use_rtcp_channel =*/tsk_false)
#define _trtp_manager_send_turn_dtls_rtcp(ice_ctx, handshaking_data_ptr, handshaking_data_size) \
    _trtp_manager_send_turn_dtls ((ice_ctx), (handshaking_data_ptr), (handshaking_data_size), /*use_rtcp_channel =*/tsk_true)

void sendP2pRouteCheckReq( trtp_manager_t *self );
void sendP2pRouteCheckResp( trtp_manager_t *self );

/* ======================= Transport callback ========================== */
static int _trtp_transport_layer_cb (const tnet_transport_event_t *e)
{
    trtp_manager_t *manager = (trtp_manager_t *)e->callback_data;
    switch (e->type)
    {
    case event_data:
    {
		if (TNET_SOCKET_TYPE_IS_STREAM(manager->transport->type))
		{
			//TCP 需要解包
			ByteArray_Append(manager->tcpPacketBuffer, e->data, e->size);
			while (1)
			{
				int iBuffLen = ByteArray_Length(manager->tcpPacketBuffer);
				//2字节头
				if (iBuffLen < sizeof(short))
				{
					return 0;
				}
				const char* pBaseBuffer = ByteArray_Data(manager->tcpPacketBuffer);
				short sPackLen = 0;
				memcpy(&sPackLen, pBaseBuffer, sizeof(short));
				sPackLen = ntohs(sPackLen);
				//判断body 是否够了
				if (iBuffLen < sizeof(short) + sPackLen)
				{
					//不够等待下一次接收
					return 0;
				}
				//够了直接回调到上面
				_trtp_manager_recv_data(manager, pBaseBuffer + sizeof(short), sPackLen, e->local_fd, &e->remote_addr);
				ByteArray_Remove(manager->tcpPacketBuffer, 0, sizeof(short) + sPackLen);
			}
		}
		else
		{
			return _trtp_manager_recv_data(manager, e->data, e->size, e->local_fd, &e->remote_addr);
		}
    }

    case event_connected:
    case event_closed:
    {
		TSK_DEBUG_INFO("tcp mode conntectd");
        break;
    }
    default:
        break;
    }
    return 0;
}

/* =======================Report Transport callback ========================== */
static int _trtp_transport_layer_report_cb (const tnet_transport_event_t *e)
{
    trtp_manager_t *manager = (trtp_manager_t *)e->callback_data;
    
    switch (e->type)
    {
        case event_data:
        {
			if (TNET_SOCKET_TYPE_IS_STREAM(manager->transport_report->type))
			{
				//TCP 需要解包

			}
			else
			{
				onRecvMediaCtlData(manager, e->data, e->size);
				return 0;
			}
        
        }
            
        case event_connected:
        case event_closed:
        {
			TSK_DEBUG_INFO("tcp mode conntectd");
            break;
        }
        default:
            break;
    }
    return 0;
}

send_packet_status_t* trtp_manager_get_send_status(trtp_manager_t *self,uint8_t videoid)
{
    send_packet_status_t* status = tsk_null;
    const tsk_list_item_t *item;
    send_packet_status_t* pkt;
    tsk_list_lock(self->send_packets_status_list);
    tsk_list_foreach(item, self->send_packets_status_list){
        if(!(pkt = item->data)){
            continue;
        }
        if(pkt->VideoID == videoid){
            status = pkt;
            break;
        }
    }
    
    tsk_list_unlock(self->send_packets_status_list);
    
    return status;
}

 int trtp_manager_addPlugRefCount(trtp_manager_t *self)
{
   int iCurCount =  tsk_atomic_inc(&self->iPluginRefCount);
    return ++iCurCount;
}
 int trtp_manager_subPlugRefCount(trtp_manager_t *self)
{
    int iCurCount = tsk_atomic_dec(&self->iPluginRefCount);
    return --iCurCount;
}

static trtp_manager_t *_trtp_manager_create (const char *local_ip, tsk_bool_t ipv6, int sessionid)
{
    trtp_manager_t *manager;

    if ((manager = tsk_object_new (trtp_manager_def_t))) {
        manager->use_rtcp = tsk_true;
        manager->use_rtcpmux = tsk_true;
        manager->local_ip = tsk_strdup (local_ip);
        manager->use_ipv6 = ipv6;
		manager->use_tcp = Config_GetInt("rtp_use_tcp", 0);
        manager->rtp.payload_type = 127;
        manager->sessionid = sessionid;
        manager->iPluginRefCount=0;
        manager->iStaticCount = 4;
        manager->iLastVideoBitrate = 100;
        manager->iLastVideoFps = 100;
        manager->iLastVideoFrame= 100;
        manager->iLastMainVideoSize = 100;

        manager->iLastvideoSendStatus = TRTP_VIDEO_STEAM_UNKNOW;
        manager->ivideoSendStatus = TRTP_VIDEO_STEAM_NORMAL;
        manager->iLastMinorVideoBitrate = 100;
        manager->iLastMinorVideoFps = 100;
        // manager->iLastMinorNoLossBitrate = 100;
        manager->lastAdjustTimeMs = 0;
        manager->iHigherLossrateCount = 0;
        manager->iForceSendStatus = 0;
        manager->iLastDataRtt = 0;
        manager->iLastDeltaRtt = 0;

        manager->iPeakLossCount = 0;

		manager->tcpPacketBuffer = NULL;
		manager->pszEncryptPasswd = NULL;
        manager->iLastNoLossBitrate = 100;
        manager->iUpBitrateLevel = 5;
        manager->iMinBitRatePercent = 50;
        manager->bUpBitrate = tsk_false;

        manager->send_stats.start_time_ms = 0;
        manager->send_stats.bytes_sent = 0;
        manager->send_stats.total_time_ms = 0;
        manager->send_stats.total_bytes_sent = 0;
        manager->send_stats.packet_count = 0;

        manager->send_stats.video_start_time_ms = 0;
        manager->send_stats.video_total_time_ms = 0;
        manager->send_stats.video_bytes_sent = 0;
        manager->send_stats.video_total_bytes_sent = 0;
        manager->send_stats.video_packet_count = 0;
        manager->send_thread_cond = tsk_cond_create(tsk_false, tsk_false);

        manager->recvDataStat = 0;
        manager->recvLastTickStart = 0;
        manager->recvInstantBitrate = 0;

        manager->iDataSendFlag = 1;

        manager->report_stat.audio_send_total_count = 0;
        manager->report_stat.audio_send_loss_count  = 0;
        manager->report_stat.video_send_total_count = 0;
        manager->report_stat.video_send_loss_count  = 0;

    }
    return manager;
}

static void _trtp_manager_packet_queue_push (trtp_manager_t *self, trtp_rtp_packet_t* new_packet)
{
    trtp_rtp_packet_t* cur_packet;
    
    if ((tsk_false == self->packet_queue.got_first_packet) && (self->packet_queue.rtp_clock_rate > 0)) {
        TSK_DEBUG_INFO("Got first packet pts:%u(ms)", (new_packet->header->timestamp * 1000) / self->packet_queue.rtp_clock_rate);
        self->packet_queue.got_first_packet = tsk_true;
    }
    
    if (tsk_null == self->packet_queue.head) {
        self->packet_queue.head = new_packet;
        self->packet_queue.newest = new_packet;
        new_packet->prev = tsk_null;
        new_packet->next = tsk_null;
        return;
    }
 
    // Insert the packet to the queue to make it ordered by RTP timestamp
    cur_packet = self->packet_queue.newest;
    int64_t time_diff_ms = DIFF_RTP_TS(cur_packet->header->timestamp, new_packet->header->timestamp);
    if (time_diff_ms > 0) {
        while ((time_diff_ms > 0) && (tsk_null != cur_packet->next)) {
            cur_packet = cur_packet->next;
            time_diff_ms = DIFF_RTP_TS(cur_packet->header->timestamp, new_packet->header->timestamp);
        }
        if (time_diff_ms <= 0) {
            cur_packet = cur_packet->prev;
        }
    } else if (time_diff_ms < 0) {
        while ((time_diff_ms < 0) && (tsk_null != cur_packet->prev)) {
            cur_packet = cur_packet->prev;
            time_diff_ms = DIFF_RTP_TS(cur_packet->header->timestamp, new_packet->header->timestamp);
        }
        if (time_diff_ms < 0) {
            new_packet->prev = tsk_null;
            new_packet->next = cur_packet;
            cur_packet->prev = new_packet;
            cur_packet = tsk_null;
            self->packet_queue.head = new_packet;
        }
    }
    
    if (tsk_null != cur_packet) {
        if (tsk_null != cur_packet->next) {
            cur_packet->next->prev = new_packet;
        }
        new_packet->next = cur_packet->next;
        new_packet->prev = cur_packet;
        cur_packet->next = new_packet;
    }
    
    self->packet_queue.newest = new_packet;
}

static trtp_rtp_packet_t* _trtp_manager_packet_queue_pop (trtp_manager_t *self, uint64_t time_of_day_ms)
{
    trtp_rtp_packet_t* return_packet = tsk_null;
    
    // Update the playing time.
    // We need to check for the rare case that there's a jump in the system clock.
    if (time_of_day_ms > self->packet_queue.last_time_of_day_ms) {
        self->packet_queue.playing_time += (uint32_t)(((uint64_t)(time_of_day_ms - self->packet_queue.last_time_of_day_ms)
                                                      * self->packet_queue.rtp_clock_rate) / 1000);
    }
    self->packet_queue.last_time_of_day_ms = time_of_day_ms;
    
    while (tsk_null != self->packet_queue.head) {
        trtp_rtp_packet_t* cur_packet = self->packet_queue.head;
        int64_t time_diff = DIFF_RTP_TS(self->packet_queue.playing_time, cur_packet->header->timestamp);

        if (time_diff < self->packet_queue.timestamp_threshold_drop) {
            // Drop the packet since it expired
            self->packet_queue.head = cur_packet->next;
            if (self->packet_queue.newest == cur_packet) {
                self->packet_queue.newest = self->packet_queue.head;
            }
            if (tsk_null != self->packet_queue.head) {
                self->packet_queue.head->prev = tsk_null;
            }
            TSK_OBJECT_SAFE_FREE (cur_packet);
        } else if (time_diff > self->packet_queue.timestamp_threshold_wait) {
            // It's not time to play any packet yet.
            return_packet = tsk_null;
            break;
        } else {
            self->packet_queue.head = cur_packet->next;
            if (self->packet_queue.newest == cur_packet) {
                self->packet_queue.newest = self->packet_queue.head;
            }
            if (tsk_null != self->packet_queue.head) {
                self->packet_queue.head->prev = tsk_null;
            }
            return_packet = cur_packet;
            if ((tsk_false == self->packet_queue.match_first_packet) && (self->packet_queue.rtp_clock_rate > 0)) {
                TSK_DEBUG_INFO("First match pts:%u(ms)", (return_packet->header->timestamp * 1000) / self->packet_queue.rtp_clock_rate);
                self->packet_queue.match_first_packet = tsk_true;
            }
            break;
        }
    }
    
    return return_packet;
}


static int _trtp_manager_recv_data (const trtp_manager_t *self, 
                                    const uint8_t *data_ptr, 
                                    tsk_size_t data_size, 
                                    tnet_fd_t local_fd, 
                                    const struct sockaddr_storage *remote_addr) {

    // ((trtp_manager_t *)self)->isSendDataReceived = 1;
    tsk_bool_t is_rtcp = tsk_false, is_rtp = tsk_false;
    uint64_t curTimeMs = tsk_gettimeofday_ms();

    if (!self || !data_ptr) {
        TSK_DEBUG_ERROR("Invalid parameters");
        return 0;
    }

    if (!self->is_started) {
        TSK_DEBUG_INFO ("RTP manager not started yet");
        return 0;
    }


    // Once we receive a packet from the server, the UDP connection with the server is valid
    ((trtp_manager_t *)self)->last_time_recv_packet_ms = curTimeMs;

    // recv data stat
    if (!self->recvDataStat || !self->recvLastTickStart) {
        ((trtp_manager_t *)self)->recvLastTickStart = curTimeMs;
    }

    //  这里忽略TCP传输格式，TCP头长度比UDP头长度多不了几个字节
    // rtp length + Ethernet head length + IP head length + UDP head length
    ((trtp_manager_t *)self)->recvDataStat += data_size + 14 + 20 + 8;;

    // 每秒统计一次下行带宽
    int32_t tick_interval = curTimeMs - self->recvLastTickStart;
    if (tick_interval >= 1000) {
        ((trtp_manager_t *)self)->recvInstantBitrate = self->recvDataStat * 1000 / tick_interval;
        ((trtp_manager_t *)self)->recvDataStat = 0;

        // TSK_DEBUG_INFO("instant bitrate:%llu", self->recvInstantBitrate);
        // set to Avstatic
        setRecvDataStat(self->recvInstantBitrate);
    }

    // If it's a dummy packet(no payload), just ignore it.
    // Our RTP packet contains a 12-byte RTP fixed header and a 4-byte CSRC field
    // for the session ID. So if it's not more than 16 bytes, there's no valid payload.
    if (data_size <= 16) {
        if (16 == data_size) {
            int32_t sessionId = (data_ptr[12] << 24) | (data_ptr[13] << 16)
                                | (data_ptr[14] << 8) | data_ptr[15];
            if (self->sessionid != sessionId) {
                TSK_DEBUG_WARN("Got dummy packet from session:%d", sessionId);
            } else {
                // received a dummy packet echoed from the server
                if ((curTimeMs - self->last_time_print_recv_packet_ms) >= (10 * 1000)) {
                    TSK_DEBUG_INFO("Got dummy packet from me:%d", sessionId);
                    ((trtp_manager_t *)self)->last_time_print_recv_packet_ms = curTimeMs;
                }
            }
        } else {
            TSK_DEBUG_WARN("Got incomplete packet");
        }
        return 0;
    }

    // p2p route check
    if (data_size == 20) {
        int send_sessionid = (data_ptr[12] << 24) | (data_ptr[13] << 16)
                                | (data_ptr[14] << 8) | data_ptr[15];
        if (self->sessionid != send_sessionid) {
            if (data_ptr[16] == 'P' && data_ptr[17] == 'O' && data_ptr[18] == 'N' && data_ptr[19] == 'G') {
                // get resp from p2p remote
                ((trtp_manager_t *)self)->p2p_connect_info.p2p_last_recv_pong_time = curTimeMs;
            } else if (data_ptr[16] == 'P' && data_ptr[17] == 'I' && data_ptr[18] == 'N' && data_ptr[19] == 'G') {
                // get req from p2p remote
                ((trtp_manager_t *)self)->p2p_connect_info.p2p_last_recv_ping_time = curTimeMs;
                // send ping resp to remote
                sendP2pRouteCheckResp((trtp_manager_t *)self);
            }
        }
        return 0;
    }

    // defined when RTCP-MUX is disabled and RTCP port is equal to "RTP Port + 1"

    // rfc5764 - 5.1.2.  Reception
    // rfc5761 - 4.  Distinguishable RTP and RTCP Packets
    // rtcp palod type in [192, 224], dynamic rtp payload type in [96, 127]
    uint8_t pt = data_ptr[1] & 0x7F;
    is_rtcp = (63 < pt) && (pt < 96);
    is_rtp = !is_rtcp;

    // rtp/rtcp packet process
    if (is_rtcp) {
        // rtcp packet only send to rtcp session to analyse
        if (self->rtcp.session) {
            return trtp_rtcp_session_process_rtcp_in(self->rtcp.session, data_ptr, data_size);
        }
        TSK_DEBUG_WARN("No RTCP session");
        return 0;
    } else {    // rtp packet process

        if (self->rtp.cb.fun) {
            trtp_rtp_packet_t *packet_rtp = tsk_null;
            if ((packet_rtp = trtp_rtp_packet_deserialize (data_ptr, data_size))) {
                // update remote SSRC based on received RTP packet
                ((trtp_manager_t *)self)->rtp.ssrc.remote = packet_rtp->header->ssrc;
                // Get the session id.
                if (packet_rtp->header->csrc_count > 0) {
                    packet_rtp->header->session_id = (int32_t)packet_rtp->header->csrc[0];
                }  else {
                    packet_rtp->header->session_id = -1;
                }
                packet_rtp->header->my_session_id = self->sessionid;

                // forward packet to the RTCP session to calculate qos
                if (self->rtcp.session) {
                    trtp_rtcp_session_process_rtp_in(self->rtcp.session, packet_rtp, data_size);
                }
                
                if (packet_rtp->payload.size == 0)
                    TSK_DEBUG_INFO ("recv packet_rtp payload:%p ,size:%d", packet_rtp->payload.data, packet_rtp->payload.size);

                // Only when the playing timestamp has been specified, we need to queue the packets
                // according to the current timestamp.
                // This only make sense for audio only applications.
                if (TRTP_OPUS_MP == packet_rtp->header->payload_type 
                        && self->packet_queue.last_time_of_day_ms > 0) {
                    tsk_safeobj_lock (&(self->packet_queue));
                    _trtp_manager_packet_queue_push((trtp_manager_t *)self, packet_rtp);
                    // Pop all the packets that match the current playing timestamp.
                    do {
                        packet_rtp = _trtp_manager_packet_queue_pop((trtp_manager_t *)self, curTimeMs);
                        if (packet_rtp) {
                            packet_rtp->header->receive_timestamp_ms = curTimeMs;
                            self->rtp.cb.fun (self->rtp.cb.usrdata, packet_rtp);
                            TSK_OBJECT_SAFE_FREE (packet_rtp);
                        } else {
                            break;
                        }
                    } while (1);
                    tsk_safeobj_unlock (&(self->packet_queue));
                    return 0;
                }
                
                packet_rtp->header->receive_timestamp_ms = curTimeMs;

#if 0
                trtp_sort_t* sort = trtp_sort_select_by_sessionid_payloadtype(self->sorts_list, (int)packet_rtp->header->csrc[0], packet_rtp->header->payload_type);
                
                if(!sort) {
                    trtp_sort_t* sort_new = trtp_sort_create((int)packet_rtp->header->csrc[0], packet_rtp->header->payload_type, tsk_null, tsk_null);
                    trtp_sort_start(sort_new);
                    sort = sort_new;
                    tsk_list_push_back_data(self->sorts_list, (tsk_object_t**)&sort_new);
                }
                
                if(sort) {
                    trtp_sort_push_rtp_packet(sort, packet_rtp);
                }
#endif
                
                // forward to the callback function (most likely "session_av")
                // 根据rtp_packet的payload_type判断数据是音频数据还是视频数据
                if (packet_rtp->header->payload_type == TRTP_H264_MP) {
                    //TSK_DEBUG_INFO("Recv video packet:payload_type:%d sessionid:%d packet_size:%d video_cb.fun:%p",
                    //               packet_rtp->header->payload_type,
                    //               packet_rtp->header->session_id,
                    //               packet_rtp->payload.size,
                    //               self->rtp.video_cb.fun);
                    TMEDIA_I_AM_ACTIVE(3000, "video rtp packet is coming");
                    #ifndef OS_LINUX
                    // 视频数据，调用视频session注册的回调接口
                    if (self->rtp.video_cb.fun && !self->rtp.is_video_pause) {
                        self->rtp.video_cb.fun (self->rtp.video_cb.usrdata, packet_rtp);
                    }
                    #endif

                } else if (packet_rtp->header->payload_type == TRTP_CUSTOM_FORMAT) {
                    // 自定义数据, csrc[0] 为发送用户，csrc[1]为目的用户
                    NotifyCustomData(packet_rtp->payload.data, packet_rtp->payload.size, packet_rtp->header->timestampl, packet_rtp->header->csrc[0]);
                } else if (packet_rtp->header->payload_type == TRTP_OPUS_MP) {
                    TMEDIA_I_AM_ACTIVE(500, "audio rtp packet is coming");
                    // 默认为音频数据，调用音频session注册的回调接口
                    if (self->rtp.cb.fun && !self->rtp.is_audio_pause) {
                        self->rtp.cb.fun (self->rtp.cb.usrdata, packet_rtp);
                    }
                } else {
                    TMEDIA_I_AM_ACTIVE(200, "invalid rtp packet is coming");
                }

                TSK_OBJECT_SAFE_FREE (packet_rtp);
                return 0;
            } else {
                TSK_DEBUG_ERROR ("RTP packet === NOK");
                return -1;
            }
        }
        return 0;
    }
}

/** Create RTP/RTCP manager */
trtp_manager_t *trtp_manager_create ( const char *local_ip, tsk_bool_t ipv6, int sessionid)
{
    trtp_manager_t *manager;
    if ((manager = _trtp_manager_create ( local_ip, ipv6,sessionid)))
    {
    }

    /** Sets p2p remote parameters for rtp session */
    char * strBuf = tsk_malloc(128);
    Config_GetString("p2p_local_ip", "", strBuf);
    tsk_strupdate (&manager->p2p_connect_info.p2p_local_ip, strBuf);
    manager->p2p_connect_info.p2p_local_port = Config_GetInt("p2p_local_port", 0);

    Config_GetString("p2p_remote_ip", "", strBuf);
    tsk_strupdate (&manager->p2p_connect_info.p2p_remote_ip, strBuf);
    manager->p2p_connect_info.p2p_remote_port = Config_GetInt("p2p_remote_port", 0);;

    if (manager->p2p_connect_info.p2p_local_ip != "" && manager->p2p_connect_info.p2p_local_port != 0 \
        && manager->p2p_connect_info.p2p_remote_ip != "" && manager->p2p_connect_info.p2p_remote_port != 0) {
        manager->p2p_connect_info.p2p_info_set_flag = tsk_true;
        // 等p2p通路检测之后再决定走哪个通路
        manager->curr_trans_route = TRTP_TRANS_ROUTE_TYPE_UNKNOE;
    } else {
        manager->p2p_connect_info.p2p_info_set_flag = tsk_false;
        manager->p2p_route_check_end = tsk_true;
        // 未设置p2p info, 直接走server转发
        manager->curr_trans_route = TRTP_TRANS_ROUTE_TYPE_SERVER;
    }
    
    manager->p2p_route_enable = tsk_false;
    manager->p2p_route_change_flag = Config_GetInt("route_change_flag", 0);

    manager->p2p_connect_info.p2p_last_recv_pong_time = 0;
    manager->p2p_connect_info.p2p_last_recv_ping_time = 0;
    manager->p2p_connect_info.p2p_route_check_loss = 0;
    if (strBuf) {
        tsk_free(&strBuf);
    }
    
    TSK_DEBUG_INFO ("[P2P] local:[%s:%u] remote[%s:%u] flag:%d", manager->p2p_connect_info.p2p_local_ip, manager->p2p_connect_info.p2p_local_port,
                    manager->p2p_connect_info.p2p_remote_ip, manager->p2p_connect_info.p2p_remote_port, manager->p2p_connect_info.p2p_info_set_flag);

    return manager;
}

/** Prepares the RTP/RTCP manager */
/** create transport and set data and report callback**/
int trtp_manager_prepare (trtp_manager_t *self)
{
    const char *rtp_local_ip = tsk_null, *rtcp_local_ip = tsk_null;
    tnet_port_t rtp_local_port = 0, rtcp_local_port = 0;

    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    if (self->transport)
    {
        TSK_DEBUG_ERROR ("RTP manager already prepared");
        return -2;
    }
    
    if (self->transport_p2p)
    {
        TSK_DEBUG_ERROR ("P2P RTP manager already prepared");
        return -2;
    }

    if( self->transport_report )
    {
        TSK_DEBUG_ERROR ("REPORT RTP manager already prepared");
        return -2;
    }
    
#define __retry_count_max 5
#define __retry_count_max_minus1 (__retry_count_max - 1)
    uint8_t retry_count = __retry_count_max;
    tnet_socket_type_t socket_type;
    if (self->use_ipv6)
    {
        if (self->use_tcp)
        {
            socket_type = tnet_socket_type_tcp_ipv6;
        }
        else
        {
            socket_type = tnet_socket_type_udp_ipv6;
        }
    }
    else
    {
        if (self->use_tcp)
        {
            socket_type = tnet_socket_type_tcp_ipv4;
        }
        else
        {
            socket_type = tnet_socket_type_udp_ipv4;
        }
    }

    tnet_port_t local_port;
    /* Creates local rtp and rtcp sockets */
    while (retry_count--)
    {
        /* random number in the range 1024 to 65535 */
        static int counter = 0;

        // first check => try to use port from latest active session if exist
        local_port = (retry_count == __retry_count_max_minus1 && (self->port_range.start <= self->rtp.public_port && self->rtp.public_port <= self->port_range.stop)) ?
                                    self->rtp.public_port :
                                    (((rand () ^ ++counter) % (self->port_range.stop - self->port_range.start)) + self->port_range.start);
        local_port = (local_port & 0xFFFE); /* turn to even number */

        /* beacuse failure will cause errors in the log, print a message to alert that there is
        * nothing to worry about */
        TSK_DEBUG_INFO ("RTP/RTCP manager[Begin]: Trying to bind to random ports, local_port:%u", local_port);

        /* RTP */
        if (!(self->transport = tnet_transport_create (self->local_ip, local_port, socket_type, TRTP_TRANSPORT_NAME)))
        {
            TSK_DEBUG_ERROR ("Failed to create RTP/RTCP Transport");
            TSK_OBJECT_SAFE_FREE (self->transport);
            if(retry_count>0) {
                continue;
            }else{
                return -3;
            }
        }
        self->tcpPacketBuffer = Create_ByteArray();
        TSK_DEBUG_INFO ("RTP/RTCP manager[End]: Trying to bind to random ports");
        break;
    } // end-of-while(retry_count)

    rtp_local_ip = self->transport->master->ip;
    rtp_local_port = self->transport->master->port;

    if(self->rtcp.local_socket) {
        rtcp_local_ip = self->rtcp.local_socket->ip;
        rtcp_local_port = self->rtcp.local_socket->port;
    }

    // create p2p socket
    if (self->p2p_connect_info.p2p_info_set_flag) {
        uint8_t p2p_retry_count = __retry_count_max;
        while (p2p_retry_count--) {
            // 目前p2p仅支持IPv4
            if (self->use_tcp) {
                socket_type = tnet_socket_type_tcp_ipv4;
            } else {
                socket_type = tnet_socket_type_udp_ipv4;
            }

            /* creat p2p socket */
            TSK_DEBUG_INFO ("P2P RTP/RTCP manager[Begin]: Trying to bind to local ip:%s, local_port:%u"
                            , self->p2p_connect_info.p2p_local_ip, self->p2p_connect_info.p2p_local_port);
            if (!(self->transport_p2p = tnet_transport_create ("0.0.0.0", self->p2p_connect_info.p2p_local_port
                                                            , socket_type, P2P_TRTP_TRANSPORT_NAME)))
            {
                TSK_DEBUG_ERROR ("Failed to create P2P RTP/RTCP Transport");
                TSK_OBJECT_SAFE_FREE (self->transport_p2p);
                if(retry_count>0) {
                    continue;
                }else{
                    return -3;
                }
            }
            self->tcpPacketBuffer = Create_ByteArray();
            TSK_DEBUG_INFO ("P2P RTP/RTCP manager[End]");
            break;
        }
    }

    //create report socket
	if (!self->use_tcp)
	{
		uint8_t report_retry_count = __retry_count_max;
		while (report_retry_count--)
		{
			// todo:随便找了个port，这个要跟服务器商量吧
			local_port += 3;

			//local_port = 54532;

			/* beacuse failure will cause errors in the log, print a message to alert that there is
			* nothing to worry about */
            /* creat p2p socket */
			TSK_DEBUG_INFO("Report RTP/RTCP manager[Begin]: Trying to bind to random ports, local_port:%u", local_port);
			if (!(self->transport_report = tnet_transport_create(self->local_ip, local_port, socket_type, REPORT_TRTP_TRANSPORT_NAME)))
			{
				TSK_DEBUG_ERROR("Failed to create Report RTP/RTCP Transport");
				TSK_OBJECT_SAFE_FREE(self->transport_report);
			}

			TSK_DEBUG_INFO("Report RTP/RTCP manager[End]: Trying to bind to random ports");
			break;
		} 
	}
  
    tsk_strupdate (&self->rtp.public_ip, rtp_local_ip);
    self->rtp.public_port = rtp_local_port;

    tsk_strupdate(&self->rtcp.public_addr.ip, rtcp_local_ip);
    self->rtcp.public_addr.port = rtcp_local_port;
	self->rtcp.public_addr.type = socket_type;
	
    if (self->transport)
    {
        /* set callback function */
        tnet_transport_set_callback (self->transport, _trtp_transport_layer_cb, self);
/* Disable receiving until we start the transport (To avoid buffering) */
#if TRTP_DISABLE_SOCKETS_BEFORE_START
        if (!self->is_socket_disabled)
        {
            int err, optval = TRTP_TINY_RCVBUF;
            if ((err = setsockopt (self->transport->master->fd, SOL_SOCKET, SO_RCVBUF, (char *)&optval, sizeof (optval))))
            {
                TNET_PRINT_LAST_ERROR ("setsockopt(SOL_SOCKET, SO_RCVBUF) has failed with error code %d", err);
            }
            self->is_socket_disabled = (err == 0);
        }
#endif
    }
    
    // 用于p2p传输
    if (self->transport_p2p)
    {
        /* set callback function */
        tnet_transport_set_callback (self->transport_p2p, _trtp_transport_layer_cb, self);
/* Disable receiving until we start the transport (To avoid buffering) */
#if TRTP_DISABLE_SOCKETS_BEFORE_START
        if (!self->socket_disabled)
        {
            int err, optval = TRTP_TINY_RCVBUF;
            if ((err = setsockopt (self->transport_p2p->master->fd, SOL_SOCKET, SO_RCVBUF, (char *)&optval, sizeof (optval))))
            {
                TNET_PRINT_LAST_ERROR ("setsockopt(SOL_SOCKET, SO_RCVBUF) has failed with error code %d", err);
            }
            self->socket_disabled = (err == 0);
        }
#endif
    }

    if( self->transport_report )
    {
        /* set callback function */
        tnet_transport_set_callback (self->transport_report, _trtp_transport_layer_report_cb, self);
        /* Disable receiving until we start the transport (To avoid buffering) */
#if TRTP_DISABLE_SOCKETS_BEFORE_START
        if (!self->is_report_socket_disabled)
        {
            int err, optval = TRTP_TINY_RCVBUF;
            if ((err = setsockopt (self->transport_report->master->fd, SOL_SOCKET, SO_RCVBUF, (char *)&optval, sizeof (optval))))
            {
                TNET_PRINT_LAST_ERROR ("setsockopt(SOL_SOCKET, SO_RCVBUF) has failed with error code %d", err);
            }
            self->is_report_socket_disabled = (err == 0);
        }
#endif

        
    }

    return 0;
}


/** Indicates whether the manager is already ready or not */
tsk_bool_t trtp_manager_is_ready (trtp_manager_t *self)
{
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return tsk_false;
    }
    return (self->transport != tsk_null);
}
/** Sets RTP callback */
int trtp_manager_set_rtp_callback (trtp_manager_t *self, trtp_rtp_cb_f fun, const void *usrdata)
{
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    self->rtp.cb.fun = fun;
    self->rtp.cb.usrdata = usrdata;

    return 0;
}

// 设置rtp video回调
int trtp_manager_set_rtp_video_callback(trtp_manager_t *self, trtp_rtp_cb_f fun, const void *usrdata)
{
    if (!self)
    {
        TSK_DEBUG_ERROR("Invalid parameter");
        
        return -1;
    }
    
    self->rtp.video_cb.fun = fun;
    self->rtp.video_cb.usrdata = usrdata;
    
    return 0;
}

// 设置rtp video switch回调
int trtp_manager_set_switch_video_callback(trtp_manager_t *self, video_switch_cb_f fun, const void *usrdata)
{
    if (!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    self->rtp.video_switch_cb.fun = fun;
    self->rtp.video_switch_cb.usrdata = usrdata;
    if (self->rtcp.session) {
         return trtp_rtcp_session_set_video_swicth_callback(self->rtcp.session, self->rtp.video_switch_cb.fun, self->rtp.video_switch_cb.usrdata);
    }
    return 0;
}

int trtp_manager_set_video_adjust_callback(trtp_manager_t *self, video_adjust_cb_f fun, const void *usrdata)
{
    if (!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    self->video_encode_adjust_cb.fun = fun;
    self->video_encode_adjust_cb.usrdata = usrdata;

    return 0;
}

int trtp_manager_set_video_route_callback(trtp_manager_t *self,  video_route_cb_f fun, const void *usrdata)
{
    if (!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    
    self->trtp_route_cb.fun = fun;
    self->trtp_route_cb.usrdata = usrdata;
    
    return 0;
}

// 设置 video playload type
int trtp_manager_set_video_payload_type (trtp_manager_t *self, uint8_t payload_type)
{
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    
    TSK_DEBUG_INFO("video:payload_type:%d", payload_type);
    
    self->rtp.video_payload_type = payload_type;
    
    return 0;
}

/** Sets RTCP callback */
int trtp_manager_set_rtcp_callback(trtp_manager_t* self, trtp_rtcp_cb_f fun, const void* usrdata)
{
    if(!self) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }

    self->rtcp.cb.fun = fun;
    self->rtcp.cb.usrdata = usrdata;
    if(self->rtcp.session) {
        return trtp_rtcp_session_set_callback(self->rtcp.session, fun, usrdata);
    }

    return 0;
}

/** Sets the payload type */
int trtp_manager_set_payload_type (trtp_manager_t *self, uint8_t payload_type)
{
    if (!self) {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    self->rtp.payload_type = payload_type;
    return 0;
}

int trtp_manager_set_rtp_dscp (trtp_manager_t *self, int32_t dscp)
{
    if (!self) {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    self->rtp.dscp = dscp;
    return 0;
}

/** Sets remote parameters for rtp session */
int trtp_manager_set_rtp_remote (trtp_manager_t *self, const char *remote_ip, tnet_port_t remote_port)
{
    if (!self) {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    // if ICE is enabled then, these values will be updated when the manager start()s and call
    // ice_init()
    tsk_strupdate (&self->rtp.remote_ip, remote_ip);
    self->rtp.remote_port = remote_port;

    return 0;
}

int trtp_manager_set_report_remote( trtp_manager_t *self, const char *remote_ip, tnet_port_t remote_port )
{
    if (!self) {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    
    tsk_strupdate (&self->report.remote_ip, remote_ip);
    self->report.remote_port = remote_port;
    return 0;
}
int trtp_manager_set_port_range (trtp_manager_t *self, uint16_t start, uint16_t stop)
{
    if (!self) {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    self->port_range.start = start;
    self->port_range.stop = stop;
    return 0;
}

int trtp_manager_set_rtcweb_type_remote (trtp_manager_t *self, tmedia_rtcweb_type_t rtcweb_type)
{
    if (!self) {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }
    self->rtcweb_type.remote = rtcweb_type;
    return 0;
}

// 这个线程只是针对视频数据做pacing处理，防止流量尖刺
static void* TSK_STDCALL trtp_manager_rtp_send_thread_func(void *arg) {
    trtp_manager_t* self = (trtp_manager_t*)arg;
    self->rtp_send_thread_running = tsk_true;
    
    TSK_DEBUG_INFO("trtp send thread enters.");
    
    unsigned int sendCounter = 0;

    unsigned int oneLoopPktCount = 5;
	int forbidenReducePktCount = 0;
    int useTCP = Config_GetInt("rtp_use_tcp", 0);
    int sleepTime = 5;//ms

    while (self->rtp_send_thread_running) {
        
        if (0 != tsk_semaphore_decrement(self->rtp_send_sema)) {
            TSK_DEBUG_ERROR("Fatal error: rtp_send_sema failed");
            break;
        }
        
        if (!self->rtp_send_thread_running) {
            break;
        }
        //tsk_thread_sleep(5);
        
        tsk_list_item_t* item = tsk_null;
        tsk_list_lock(self->rtp_packets_list);
        int list_count = tsk_list_count_all(self->rtp_packets_list);
        item = tsk_list_pop_first_item(self->rtp_packets_list);
        tsk_list_unlock(self->rtp_packets_list);

        if(item) {
//            TMEDIA_I_AM_ACTIVE(300,"trtp send thread is running. list count:%d", list_count);
            /* debug
            if (((trtp_rtp_packet_t *) item->data)->header->payload_type == TRTP_H264_MP) {
                TMEDIA_DUMP_RTP_TO_FILE("/send_video_rtp.txt", ((trtp_rtp_packet_t *) item->data));
            } else {
                TMEDIA_DUMP_RTP_TO_FILE("/send_audio_rtp.txt", ((trtp_rtp_packet_t *) item->data));
            }
             */
            trtp_rtp_packet_t * packet = (trtp_rtp_packet_t *) item->data;
            
            // 根据videoid来区分大小流，然后根据大小流状态来是否发送相应的数据包
            int video_id = 0;
            if (packet->header->csrc_count >= 5) {
                video_id = packet->header->csrc[4]; // open rscode
            } else {
                video_id = packet->header->csrc[3]; // close rscode
            }
            
            //int mode = tmedia_defaults_get_video_net_adjustmode();

            // notice: 
            // videoid: TRTP_VIDEO_STREAM_TYPE_SHARE must send in any scenes
            if (self->iDataSendFlag && (TRTP_VIDEO_STEAM_NORMAL == self->ivideoSendStatus
                || (TRTP_VIDEO_STEAM_MINOR == self->ivideoSendStatus && 1 == video_id)
                || TRTP_VIDEO_STREAM_TYPE_SHARE == video_id))
            {
                {
                    trtp_manager_send_rtp_packet(self, (trtp_rtp_packet_t *) item->data, tsk_false);
                    if ( (!useTCP) && ((trtp_rtp_packet_t *)item->data)->header->payload_type == TRTP_H264_MP) {
                        if ( ++sendCounter % oneLoopPktCount == 0) {
                            if (list_count < 400) {
                                tsk_cond_wait(self->send_thread_cond, sleepTime );
                            }
                            
                            if(list_count < 200) {
                                sleepTime = 5;
                            } else {
                                oneLoopPktCount = list_count / 10; //预计 50ms 发送完成
                                if (list_count > 500) {
                                    TSK_DEBUG_INFO("pkg more %d",list_count);
                                }

                                if (oneLoopPktCount >= 20) {
                                    sleepTime = 3;
                                } else {
                                    sleepTime = 5;
                                }
                            }
                            sendCounter = 0;
                        }
                    }
                }
            }           

            TSK_OBJECT_SAFE_FREE(item);
            
            //tsk_thread_sleep(10); //测试积累
            
        } else {
            //tsk_thread_sleep(5);
        }

        if(!self->rtp_send_thread_running) {
            break;
        }
    }

    TSK_DEBUG_WARN("trtp send thread exits.");
    return tsk_null;
}

static void OnCustomInputCallback(const void*pData, int iDataLen, uint64_t uTimeSpa, void* pParam, uint32_t uSessionId)
{
	trtp_manager_t *self = (trtp_manager_t *)pParam;
	trtp_manager_send_custom_data(self, pData, iDataLen, uTimeSpa, uSessionId);
}

/** Starts the RTP/RTCP manager */
int trtp_manager_start (trtp_manager_t *self)
{
    int ret = 0;
    int rcv_buf = (int)tmedia_defaults_get_rtpbuff_size ();
    int snd_buf = (int)tmedia_defaults_get_rtpbuff_size ();
    int report_rcv_buf = 102400;
    int report_snd_buf = 102400;
#if !TRTP_UNDER_WINDOWS_CE
    int32_t dscp_rtp;
#endif

    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    tsk_safeobj_lock (self);

    if (self->is_started)
    {
        goto bail;
    }

    if (!self->transport && (ret = trtp_manager_prepare (self)))
    {
        TSK_DEBUG_ERROR ("Failed to prepare RTP/RTCP mamanger");
        goto bail;
    }

    if (!self->transport || !self->transport->master)
    {
        TSK_DEBUG_ERROR ("RTP/RTCP manager not prepared");
        ret = -2;
        goto bail;
    }

    /* Flush buffers and re-enable sockets */
    if (self->transport->master && self->is_socket_disabled)
    {
        static char buff[1024];
        tsk_size_t guard_count = 0;

        TSK_DEBUG_INFO ("Start flushing RTP socket...");
        // Buffer should be empty ...but who know?
        // rcv() should never block() as we are always using non-blocking sockets
        while ((ret = (int)recv (self->transport->master->fd, buff, sizeof (buff), 0)) > 0 && ++guard_count < 0xF0)
        {
            TSK_DEBUG_INFO ("Flushing RTP Buffer %d", ret);
        }
        TSK_DEBUG_INFO ("End flushing RTP socket");
    }

    /* Flush buffers and re-enable sockets */
    if (self->transport_p2p && self->transport_p2p->master && self->is_p2p_socket_disabled)
    {
        static char buff[1024];
        tsk_size_t guard_count = 0;

        TSK_DEBUG_INFO ("Start flushing P2P RTP socket...");
        // Buffer should be empty ...but who know?
        // rcv() should never block() as we are always using non-blocking sockets
        while ((ret = (int)recv (self->transport_p2p->master->fd, buff, sizeof (buff), 0)) > 0 && ++guard_count < 0xF0)
        {
            TSK_DEBUG_INFO ("Flushing P2P RTP Buffer %d", ret);
        }
        TSK_DEBUG_INFO ("End flushing P2P RTP socket");
    }

    /* Flush buffers and re-enable sockets */
	if (self->transport_report && self->transport_report->master && self->is_report_socket_disabled)
    {
        static char buff[1024];
        tsk_size_t guard_count = 0;
        
        TSK_DEBUG_INFO ("Start flushing Report RTP socket...");
        // Buffer should be empty ...but who know?
        // rcv() should never block() as we are always using non-blocking sockets
        while ((ret = (int)recv (self->transport_report->master->fd, buff, sizeof (buff), 0)) > 0 && ++guard_count < 0xF0)
        {
            TSK_DEBUG_INFO ("Flushing Report RTP Buffer %d", ret);
        }
        TSK_DEBUG_INFO ("End flushing Report RTP socket");
    }

/* enlarge socket buffer */
#if !TRTP_UNDER_WINDOWS_CE
    TSK_DEBUG_INFO ("SO_RCVBUF = %d, SO_SNDBUF = %d", rcv_buf, snd_buf);
    if ((ret = setsockopt (self->transport->master->fd, SOL_SOCKET, SO_RCVBUF, (char *)&rcv_buf, sizeof (rcv_buf))))
    {
        TNET_PRINT_LAST_ERROR ("setsockopt(SOL_SOCKET, SO_RCVBUF, %d) has failed with error code %d", rcv_buf, ret);
    }
    if ((ret = setsockopt (self->transport->master->fd, SOL_SOCKET, SO_SNDBUF, (char *)&snd_buf, sizeof (snd_buf))))
    {
        TNET_PRINT_LAST_ERROR ("setsockopt(SOL_SOCKET, SO_SNDBUF, %d) has failed with error code %d", snd_buf, ret);
    }
#endif

/* enlarge p2p socket buffer */
#if !TRTP_UNDER_WINDOWS_CE
    if (self->transport_p2p) {
        TSK_DEBUG_INFO ("SO_RCVBUF = %d, SO_SNDBUF = %d", rcv_buf, snd_buf);
        if ((ret = setsockopt (self->transport_p2p->master->fd, SOL_SOCKET, SO_RCVBUF, (char *)&rcv_buf, sizeof (rcv_buf))))
        {
            TNET_PRINT_LAST_ERROR ("P2P setsockopt(SOL_SOCKET, SO_RCVBUF, %d) has failed with error code %d", rcv_buf, ret);
        }
        if ((ret = setsockopt (self->transport_p2p->master->fd, SOL_SOCKET, SO_SNDBUF, (char *)&snd_buf, sizeof (snd_buf))))
        {
            TNET_PRINT_LAST_ERROR ("P2P setsockopt(SOL_SOCKET, SO_SNDBUF, %d) has failed with error code %d", snd_buf, ret);
        }
    }
#endif

// #if !TRTP_UNDER_WINDOWS
//     dscp_rtp = IPTOS_RELIABILITY;
//     if ((ret = setsockopt (self->transport->master->fd, IPPROTO_IP, IP_TOS, (char *)&dscp_rtp, sizeof (dscp_rtp))))
//     {
//         TNET_PRINT_LAST_ERROR ("setsockopt(IPPROTO_IP, IP_TOS) has failed with error code %d", ret);
//     }
// #endif /* !TRTP_UNDER_WINDOWS_CE */
    
    /* enlarge report socket buffer */
#if !TRTP_UNDER_WINDOWS_CE
    if( self->transport_report && self->transport_report->master )
    {
        TSK_DEBUG_INFO ("Report SO_RCVBUF = %d, SO_SNDBUF = %d", report_rcv_buf, report_snd_buf);
        if ((ret = setsockopt (self->transport_report->master->fd, SOL_SOCKET, SO_RCVBUF, (char *)&report_rcv_buf, sizeof (report_rcv_buf))))
        {
            TNET_PRINT_LAST_ERROR ("Report setsockopt(SOL_SOCKET, SO_RCVBUF, %d) has failed with error code %d", report_rcv_buf, ret);
        }
        if ((ret = setsockopt (self->transport_report->master->fd, SOL_SOCKET, SO_SNDBUF, (char *)&report_snd_buf, sizeof (report_snd_buf))))
        {
            TNET_PRINT_LAST_ERROR ("Report setsockopt(SOL_SOCKET, SO_SNDBUF, %d) has failed with error code %d", report_snd_buf, ret);
        }
    }
   #endif /* !TRTP_UNDER_WINDOWS_CE */

    /* RTP */
    // check remote IP address validity
    if ((tsk_striequals (self->rtp.remote_ip, "0.0.0.0") || tsk_striequals (self->rtp.remote_ip, "::")))
    { // most likely loopback testing
        tnet_ip_t source = { 0 };
        tsk_bool_t updated = tsk_false;
        if (self->transport && self->transport->master)
        {
            updated = (tnet_getbestsource (self->transport->master->ip, self->transport->master->port, self->transport->master->type, &source) == 0);
        }
        // Not allowed to send data to "0.0.0.0"
        TSK_DEBUG_INFO ("RTP remote IP contains not allowed value ...changing to '%s'", updated ? source : "oops");
        if (updated)
        {
            tsk_strupdate (&self->rtp.remote_ip, source);
        }
    }
    
    if ((ret = tnet_sockaddr_init (self->rtp.remote_ip, self->rtp.remote_port, self->transport->master->type, &self->rtp.remote_addr)))
    {
        tnet_transport_shutdown (self->transport);
        TSK_OBJECT_SAFE_FREE (self->transport);
        TSK_DEBUG_ERROR ("Invalid RTP host:port [%s:%u]", self->rtp.remote_ip, self->rtp.remote_port);
        goto bail;
    }
    TSK_DEBUG_INFO ("rtp.remote_ip=%s, rtp.remote_port=%d, rtp.local_fd=%d", self->rtp.remote_ip, self->rtp.remote_port, self->transport->master->fd);

   
    /* start the transport if TURN is not active (otherwise TURN data will be received directly on
     * RTP manager with channel headers) */
	if ((ret = tnet_transport_start(self->transport, self->rtp.remote_ip, self->rtp.remote_port)))
    {
        TSK_DEBUG_ERROR ("Failed to start the RTP/RTCP transport");
        goto bail;
    }
    self->is_started = tsk_true;

    if(!self->rtp_send_thread_handle) {
        self->rtp_send_sema = tsk_semaphore_create();
        int ret = tsk_thread_create(&self->rtp_send_thread_handle, trtp_manager_rtp_send_thread_func, self);
        if((0 != ret) && (!self->rtp_send_thread_handle)) {
            TSK_DEBUG_ERROR ("Failed to create rtp send thread");
            goto bail;
        }
        TSK_DEBUG_INFO ("Success to create rtp send thread");
        ret = tsk_thread_set_priority(self->rtp_send_thread_handle, TSK_THREAD_PRIORITY_TIME_CRITICAL);
    }
    
    // p2p rtp
    if (self->transport_p2p){
        if ((ret = tnet_sockaddr_init (self->p2p_connect_info.p2p_remote_ip, self->p2p_connect_info.p2p_remote_port, self->transport_p2p->master->type, &self->p2p_connect_info.p2p_remote_addr)))
        {
            tnet_transport_shutdown (self->transport_p2p);
            TSK_OBJECT_SAFE_FREE (self->transport_p2p);
            TSK_DEBUG_ERROR ("Invalid RTP host:port [%s:%u]", self->p2p_connect_info.p2p_remote_ip, self->p2p_connect_info.p2p_remote_port);
            goto bail;
        }

        TSK_DEBUG_INFO ("p2p remote info[%s:%u] rtp.local_fd=%d", self->p2p_connect_info.p2p_remote_ip, self->p2p_connect_info.p2p_remote_port, self->transport_p2p->master->fd);
        /* start the p2p transport */
        if ((ret = tnet_transport_start(self->transport_p2p, self->p2p_connect_info.p2p_remote_ip, self->p2p_connect_info.p2p_remote_port)))
        {
            TSK_DEBUG_ERROR ("Failed to start P2P RTP/RTCP transport");
            goto bail;
        }
    }
    
    //设置半程rtcp远端IP，端口
    int reportPort = Config_GetInt( CONFIG_MediaCtlPort, 0 );
    TSK_DEBUG_INFO("set ctl report UDP port:%d", reportPort );
    trtp_manager_set_report_remote( self, self->rtp.remote_ip, reportPort );
    
    //启动半程rtcp
	if (self->transport_report)
	{
		if ((ret = tnet_sockaddr_init(self->report.remote_ip, self->report.remote_port, self->transport_report->master->type, &self->report.remote_addr)))
		{
			tnet_transport_shutdown(self->transport_report);
			TSK_OBJECT_SAFE_FREE(self->transport_report);
			TSK_DEBUG_ERROR("Invalid Report RTP host:port [%s:%u]", self->report.remote_ip, self->report.remote_port);
		}
		else{
			TSK_DEBUG_INFO("report.remote_ip=%s, report.remote_port=%d, report=%d", self->report.remote_ip, self->report.remote_port, self->transport_report->master->fd);

			if ((ret = tnet_transport_start(self->transport_report, self->report.remote_ip, self->report.remote_port)))
			{
				tnet_transport_shutdown(self->transport_report);
				TSK_OBJECT_SAFE_FREE(self->transport_report);

				TSK_DEBUG_ERROR("Failed to start the Report RTP/RTCP transport");
			}
		}

		start_media_ctl(self);
	}
   
	// 设置一个回调
	SetCustonInputCallback(OnCustomInputCallback, self);

    /* 全程RTCP */
    if(self->use_rtcp) {
        tnet_fd_t local_rtcp_fd = self->rtcp.local_socket ? self->rtcp.local_socket->fd : -1;
        if(local_rtcp_fd < 0 || self->use_rtcpmux) {    // use RTP local port to send RTCP packets
            local_rtcp_fd = self->transport->master->fd;
        }

        if(!self->rtcp.remote_ip) {
            self->rtcp.remote_ip = tsk_strdup(self->rtcp.remote_ip ? self->rtcp.remote_ip : self->rtp.remote_ip);
        }
        if(!self->rtcp.remote_port) {
            self->rtcp.remote_port = self->rtcp.remote_port ? self->rtcp.remote_port : (self->use_rtcpmux ? self->rtp.remote_port : (self->rtp.remote_port + 1));
        }

        if((ret = tnet_sockaddr_init(self->rtcp.remote_ip, self->rtcp.remote_port, self->transport->master->type, &self->rtcp.remote_addr))) {
            TSK_DEBUG_ERROR("Invalid RTCP host:port [%s:%u]", self->rtcp.remote_ip, self->rtcp.remote_port);
            /* do not exit */
        }
        
        TSK_DEBUG_INFO("rtcp.remote_ip=%s, rtcp.remote_port=%d, rtcp.local_fd=%d", self->rtcp.remote_ip, self->rtcp.remote_port, local_rtcp_fd);
        if (!self->use_rtcpmux) {
            /* add RTCP socket to the transport */
            if(self->rtcp.local_socket) {
                TSK_DEBUG_INFO("rtcp.local_ip=%s, rtcp.local_port=%d, rtcp.local_fd=%d", self->rtcp.local_socket->ip, self->rtcp.local_socket->port, self->rtcp.local_socket->fd);
                if(ret == 0 && (ret = tnet_transport_add_socket(self->transport, self->rtcp.local_socket->fd, self->rtcp.local_socket->type, tsk_false/* do not take ownership */, tsk_true/* only Meaningful for tls*/))) {
                    TSK_DEBUG_ERROR("Failed to add RTCP socket");
                    /* do not exit */
                }
            }
        }

        /* create and start RTCP session */
        if(!self->rtcp.session && ret == 0) {
            // use sessionid as local ssrc by mark
            self->rtcp.session = trtp_rtcp_session_create(self->sessionid, self->rtcp.cname);
        }
        
        if(self->rtcp.session) {
            ret = trtp_rtcp_session_set_callback(self->rtcp.session, self->rtcp.cb.fun, self->rtcp.cb.usrdata);
            ret = trtp_rtcp_session_set_video_swicth_callback(self->rtcp.session, self->rtp.video_switch_cb.fun, self->rtp.video_switch_cb.usrdata);
            ret = trtp_rtcp_session_set_app_bw_and_jcng(self->rtcp.session, self->app_bw_max_upload, self->app_bw_max_download, self->app_jitter_cng);
            ret = trtp_rtcp_session_set_net_transport(self->rtcp.session, self->transport);
            if (self->transport_p2p) {
                ret = trtp_rtcp_session_set_p2p_transport(self->rtcp.session, self->transport_p2p, self->transport_p2p->master->fd, (const struct sockaddr *)&self->p2p_connect_info.p2p_remote_addr);
            }

            // for test by mark
            // trtp_rtcp_session_enable_p2p_transport(self->rtcp.session, tsk_true);
            if((ret = trtp_rtcp_session_start(self->rtcp.session, local_rtcp_fd, (const struct sockaddr *)&self->rtcp.remote_addr))) {
                TSK_DEBUG_ERROR("Failed to start RTCP session");
                goto bail;
            }
        }
    }

    // start timer for check lossrate
    self->lossrate_check_timerid = xt_timer_mgr_global_schedule_loop( TRTP_CHECK_LOSSRATE_TIME, lossRateProcessForBwe, self );    
bail:

    tsk_safeobj_unlock (self);

    return ret;
}


/**
* Send a dummy packet with null payload to keep the UDP connection with the server.
* @param [in] self - rtp_manager object
* @return true if a dummy packet was sent successfully
*         false if it failed to send a dummy packet
*/
tsk_bool_t trtp_manager_send_rtp_dummy (trtp_manager_t *self)
{
    uint8_t buf[16];
    uint64_t curTimeMs = tsk_gettimeofday_ms();

    if (!self) {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return tsk_false;
    }
    
    
    if ((self->packet_queue.last_time_of_day_ms > 0)
        && ((curTimeMs - self->last_time_recv_packet_ms) > 20) ){
        trtp_rtp_packet_t *packet_rtp = tsk_null;

        tsk_safeobj_lock (&(self->packet_queue));
        do {
            packet_rtp = _trtp_manager_packet_queue_pop(self, curTimeMs);
            if (packet_rtp) {
                packet_rtp->header->receive_timestamp_ms = curTimeMs;
                self->rtp.cb.fun (self->rtp.cb.usrdata, packet_rtp);
                TSK_OBJECT_SAFE_FREE (packet_rtp);
            } else {
                break;
            }
        } while (1);
        tsk_safeobj_unlock (&(self->packet_queue));
    }

    // send a dummy packet every 2000ms,
    if ( (curTimeMs - self->last_time_send_dummy_packet_ms) < 5000 ){
            return tsk_true;
    }

    // Compose an RTP header with 1 CSRC (the session ID)
    memset(buf, 0, sizeof(buf));
    buf[0] = 0x01; // 1 CSRC
    if (self->packet_queue.last_time_of_day_ms > 0) {
        buf[4] = (uint8_t)((self->packet_queue.playing_time >> 24) & 0xFF);
        buf[5] = (uint8_t)((self->packet_queue.playing_time >> 16) & 0xFF);
        buf[6] = (uint8_t)((self->packet_queue.playing_time >> 8) & 0xFF);
        buf[7] = (uint8_t)(self->packet_queue.playing_time & 0xFF);
    }
    buf[12] = (uint8_t)((self->sessionid >> 24) & 0xFF);
    buf[13] = (uint8_t)((self->sessionid >> 16) & 0xFF);
    buf[14] = (uint8_t)((self->sessionid >> 8) & 0xFF);
    buf[15] = (uint8_t)(self->sessionid & 0xFF);
    tsk_size_t size_sent = trtp_manager_send_rtp_raw (self, buf, sizeof(buf));
    if (size_sent != sizeof(buf)) {
        TMEDIA_I_AM_ACTIVE (200,"Failed to send a dummy packet, size_sent:%lu", size_sent);
        return tsk_false;
    }
    TSK_DEBUG_INFO ("Sent a dummy packet");
    self->last_time_send_dummy_packet_ms = curTimeMs;
    
    return tsk_true;
}

/* Encapsulate raw data into RTP packet and send it over the network
* Very IMPORTANT: For voice packets, the marker bits indicates the beginning of a talkspurt */
tsk_size_t trtp_manager_send_rtp (trtp_manager_t *self, const void *data, tsk_size_t size, uint32_t duration, tsk_bool_t marker, tsk_bool_t last_packet)
{
    return trtp_manager_send_rtp_with_extension(self, data, size, duration, marker, last_packet, tsk_null, 0);
}

TINYRTP_API tsk_bool_t trtp_manager_remove_member(trtp_manager_t *self, uint32_t sessionid)
{
    tsk_bool_t ret = tsk_false;

    // sanity check
    if (!self || !sessionid)
    {
        TSK_DEBUG_ERROR("Invalid parameter to remove member in rtcp session");
        return tsk_false;
    }

    if (self->rtcp.session) 
    {
        // youme sessionid is rtp/rtcp ssrc
        ret = trtp_rtcp_session_remove_member(self->rtcp.session, sessionid);
    }
    else
    {
        TSK_DEBUG_ERROR("rtcp session is null when remove member");
    }
    
    return ret;
}

trtp_rtp_packet_t* trtp_manager_gen_rtp_with_extension(trtp_manager_t *self, const void *data, tsk_size_t size, uint32_t duration,
	tsk_bool_t marker, tsk_bool_t last_packet, const void *extension, tsk_size_t ext_size)
{
	trtp_rtp_packet_t *packet;
	tsk_size_t ret = 0;

	if (!self || !self->transport || !data || !size)
	{
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_null;
	}

	/* check if transport is started */
	if (!self->is_started || !self->transport->master)
	{
		TSK_DEBUG_WARN("RTP engine not ready yet");
		return tsk_null;
	}

	/* create packet with header */
	if (!(packet = trtp_rtp_packet_create(self->rtp.ssrc.local, ++self->rtp.seq_num, self->rtp.timestamp, self->rtp.payload_type, marker)))
	{
		return tsk_null;
	}

	self->isSendDataToServer++;

	if (packet->header->csrc_count >= 15) {
		assert(0);
		TSK_DEBUG_WARN("RTP header cannot hold the sessiong ID");
	}
	else
	{
		packet->header->csrc[0] = self->sessionid;
		if (packet->header->csrc_count < 1) {
			packet->header->csrc_count = 1;
		}
	}
	if (last_packet)
	{
		self->rtp.timestamp += duration;
	}

	//packet->payload.data_const = data;
	packet->payload.data = tsk_malloc(size);
	memcpy(packet->payload.data, data, size);
	packet->payload.size = size;
	// If the header extension is available(at least 4 fixed bytes + 1 word(4 bytes of payload)
	if (extension && (ext_size >= 8)) {
        //packet->extension.data_const = extension;
        packet->extension.data = tsk_malloc(ext_size);
        memcpy(packet->extension.data, extension, ext_size);
		packet->extension.size = ext_size;
		packet->header->extension = 1;
	}


	// trtp_manager_push_rtp_packet(self, packet);

	/* Calculate the statistics data and print them perodically */
	self->send_stats.bytes_sent += ret;
	if ((self->send_stats.packet_count % 1000) == 0) {
		uint64_t cur_time_ms = tsk_gettimeofday_ms();
		uint32_t avg_bit_rate = 0;
		uint32_t cur_bit_rate = 0;
		// The time validity needs to be checked carefully. Otherwise it may cause
		// a divided-by-0 exception.
		if ((self->send_stats.start_time_ms > 0) && (cur_time_ms > self->send_stats.start_time_ms)) {
			uint64_t time_diff = cur_time_ms - self->send_stats.start_time_ms;
			self->send_stats.total_bytes_sent += self->send_stats.bytes_sent;
			self->send_stats.total_time_ms += time_diff;
			avg_bit_rate = (uint32_t)((self->send_stats.total_bytes_sent * 8 * 1000) / self->send_stats.total_time_ms);
			cur_bit_rate = (uint32_t)((self->send_stats.bytes_sent * 8 * 1000) / time_diff);
		}
		// start another around of statistics
		self->send_stats.bytes_sent = 0;
		self->send_stats.start_time_ms = cur_time_ms;
		TSK_DEBUG_INFO("Sent the %lld(th) RTP packet with result:%lu, avg bitrate:%d(bps), cur bitrate:%d(bps)",
			self->send_stats.packet_count, ret, avg_bit_rate, cur_bit_rate);
	}
	self->send_stats.packet_count++;

	return packet;
}

#if DUMP_RTP_HEAD_EXTENSION
uint8_t get8bit(uint8_t **data) {
    uint8_t * tmp = *data;
    uint16_t result = 0;

    result = *tmp;
    *data = tmp + 1;
    return result;
}

uint16_t get16bit(uint8_t **data) {
    uint8_t * tmp = *data;
    uint16_t result = 0;

    result = *tmp << 8 | *(tmp+1);
    *data = tmp + 2;
    return result;
}

uint64_t get64bit(uint8_t **data) {
    uint8_t * tmp = *data;
    uint64_t result = 0;

    result = (*tmp << 56 & 0xff00000000000000) | 
             (*(tmp+1) << 48 & 0x00ff000000000000) |
             (*(tmp+2) << 40 & 0x0000ff0000000000) |
             (*(tmp+3) << 32 & 0x000000ff00000000) |
             (*(tmp+4) << 24 & 0x00000000ff000000) |
             (*(tmp+5) << 16 & 0x0000000000ff0000) |
             (*(tmp+6) << 8 & 0x000000000000ff00) |
             (*(tmp+7) & 0x00000000000000ff);

    *data = tmp + 8;
    return result;
}

void dump_extension_data(void *data, size_t size) {
    uint16_t extension_len = 0;
    uint8_t rtp_extension_type = 0;
    uint16_t business_id_size = 0;
    uint64_t business_id = 0;
    uint8_t **tmp = &data;
    extension_len = get16bit(tmp);
    rtp_extension_type = get8bit(tmp);
    business_id_size = get8bit(tmp);
    business_id = get64bit(tmp);
    TSK_DEBUG_INFO("bruce >>> after accemble rtp extension, extension_len=%d, rtp_extension_type=%d, business_id_size=%d, business_id=%llu", 
        extension_len, rtp_extension_type, business_id_size, business_id);
}
#endif

TINYRTP_API tsk_size_t trtp_manager_send_custom_data(trtp_manager_t *self, const void *data, tsk_size_t size, uint64_t timeSpan, uint32_t uRecvSessionId)
{
	trtp_rtp_packet_t *packet;
	tsk_size_t ret = 0;
    int offset = 0;
    static uint16_t seq_num = 0;

	if (!self || !self->transport || !data || !size)
	{
		TSK_DEBUG_ERROR("Invalid parameter");
		return tsk_null;
	}

	/* check if transport is started */
	if (!self->is_started || !self->transport->master)
	{
		TSK_DEBUG_WARN("RTP engine not ready yet");
		return tsk_null;
	}

	/* create packet with header */
	if (!(packet = trtp_rtp_packet_create(0, seq_num, timeSpan, TRTP_CUSTOM_FORMAT, 0)))
	{
		return tsk_null;
	}
    seq_num++;
	
	if (packet->header->csrc_count >= 15) {
		assert(0);
		TSK_DEBUG_WARN("RTP header cannot hold the sessiong ID");
	}
	else
	{
		packet->header->csrc[0] = self->sessionid;
        if (uRecvSessionId != 0) {
            packet->header->csrc[1] = uRecvSessionId;
        }
        
		if (packet->header->csrc_count < 2) {
            if (uRecvSessionId != 0) {
                packet->header->csrc_count = 2;
            } else {
                packet->header->csrc_count = 1;
            }
		}
	}
    
    if (self->uBusinessId != 0) {
        /* | extension magic num（2byte） |    extension lenth（2byte）          |
           |    data type（2byte）        |    data size（2byte）                |
           |  data(以4字节对齐，数据不是4的整数倍，在data高位第一个4字节前面补0x00)      |
        */
        // BusinessID is put into rtp extension
        // 16bit magic num + 16bit extension lenth + 16bit type lenth + 16bit business id len + business id size
        packet->extension.size = RTP_EXTENSION_MAGIC_NUM_LEN_PROFILE+RTP_EXTENSION_LEN_PROFILE + RTP_EXTENSION_TYPE_PROFILE + BUSINESS_ID_LEN_PROFILE + sizeof(uint64_t);
        packet->extension.data = (unsigned char *)malloc(packet->extension.size);
        memset(packet->extension.data, 0, packet->extension.size);

        //往rtp head extension中写入魔法数字
        offset = 0;
        uint16_t magic_num_net_order = htons(RTP_EXTENSION_MAGIC_NUM);
        memcpy((unsigned char *)packet->extension.data+offset, &magic_num_net_order, RTP_EXTENSION_MAGIC_NUM_LEN_PROFILE);
        offset+=RTP_EXTENSION_MAGIC_NUM_LEN_PROFILE;

        //往rtp head extension中写入扩展内容占用的空间大小
        uint16_t rtp_extension_len = (RTP_EXTENSION_TYPE_PROFILE + BUSINESS_ID_LEN_PROFILE + sizeof(uint64_t)) / 4;
        uint16_t rtp_extension_len_net_order = htons(rtp_extension_len);
        memcpy((unsigned char *)packet->extension.data+offset, &rtp_extension_len_net_order, RTP_EXTENSION_LEN_PROFILE);
        offset+=RTP_EXTENSION_LEN_PROFILE;

        //往rtp head extension中写入内容数据类型
        uint16_t rtp_extension_type_net_order = htons(BUSINESSID);
        memcpy((unsigned char *)packet->extension.data+offset, &rtp_extension_type_net_order, RTP_EXTENSION_TYPE_PROFILE);
        offset+=RTP_EXTENSION_TYPE_PROFILE;

        //往rtp head extension中写入内容数据空间大小
        uint16_t rtp_extension_type_len_net_order = htons(sizeof(uint64_t));
        memcpy((unsigned char *)packet->extension.data+offset, &rtp_extension_type_len_net_order, BUSINESS_ID_LEN_PROFILE);
        offset+=BUSINESS_ID_LEN_PROFILE;

        //往rtp head extension中写入内容数据
        uint32_t business_id_net_order_high = htonl((self->uBusinessId >> 32) & 0xffffffff);
        memcpy((unsigned char *)packet->extension.data+offset, &business_id_net_order_high, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        uint32_t business_id_net_order_low = htonl(self->uBusinessId & 0xffffffff);
        memcpy((unsigned char *)packet->extension.data+offset, &business_id_net_order_low, sizeof(uint32_t));
        
        packet->header->extension = 1;
        
    #if DUMP_RTP_HEAD_EXTENSION
        dump_extension_data(packet->extension.data, packet->extension.size);
    #endif
    }
  
	//packet->payload.data_const = data;
	packet->payload.data = tsk_malloc(size);
	memcpy(packet->payload.data, data, size);
	packet->payload.size = size;

	ret = trtp_manager_send_rtp_packet(self, packet, tsk_false);
	TSK_OBJECT_SAFE_FREE(packet);
	return ret;
}

/* Encapsulate raw data into RTP packet and send it over the network.
 * Return the last feedback information from the receiver if available. 
 * Very IMPORTANT: For voice packets, the marker bits indicates the beginning of a talkspurt */
tsk_size_t trtp_manager_send_rtp_with_extension (trtp_manager_t *self, const void *data, tsk_size_t size, uint32_t duration,
                                                tsk_bool_t marker, tsk_bool_t last_packet, const void *extension, tsk_size_t ext_size)
{
    trtp_rtp_packet_t *packet;
    tsk_size_t ret = 0;

    if (!self || !self->transport || !data || !size)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return 0;
    }

    /* check if transport is started */
    if (!self->is_started || !self->transport->master)
    {
        TSK_DEBUG_WARN ("RTP engine not ready yet");
        return 0;
    }
    
    /* create packet with header */
    if (!(packet = trtp_rtp_packet_create (self->rtp.ssrc.local, ++self->rtp.seq_num, self->rtp.timestamp, self->rtp.payload_type, marker)))
    {
        return 0;
    }
    
    self->isSendDataToServer++;
    
    if (packet->header->csrc_count >= 15) {
        assert(0);
        TSK_DEBUG_WARN ("RTP header cannot hold the sessiong ID");
    }
    else
    {
        packet->header->csrc[0] = self->sessionid;
        if (packet->header->csrc_count <1) {
            packet->header->csrc_count=1;
        }
    }
    if (last_packet)
    {
        self->rtp.timestamp += duration;
    }

    packet->payload.data_const = data;
    packet->payload.size = size;
    // If the header extension is available(at least 4 fixed bytes + 1 word(4 bytes of payload)
    if (extension && (ext_size >= 8)) {
        packet->extension.data_const = extension;
        packet->extension.size = ext_size;
        packet->header->extension = 1;
    }
    
    ret = trtp_manager_send_rtp_packet (self, packet, tsk_false);
   // trtp_manager_push_rtp_packet(self, packet);

    /* Calculate the statistics data and print them perodically */
    self->send_stats.bytes_sent += ret;
    if ((self->send_stats.packet_count % 1000) == 0) {
        uint64_t cur_time_ms = tsk_gettimeofday_ms();
        uint32_t avg_bit_rate = 0;
        uint32_t cur_bit_rate = 0;
        // The time validity needs to be checked carefully. Otherwise it may cause
        // a divided-by-0 exception.
        if ((self->send_stats.start_time_ms > 0) && (cur_time_ms > self->send_stats.start_time_ms)) {
            uint64_t time_diff = cur_time_ms - self->send_stats.start_time_ms;
            self->send_stats.total_bytes_sent += self->send_stats.bytes_sent;
            self->send_stats.total_time_ms += time_diff;
            avg_bit_rate = (uint32_t)((self->send_stats.total_bytes_sent * 8 * 1000) / self->send_stats.total_time_ms);
            cur_bit_rate = (uint32_t)((self->send_stats.bytes_sent * 8 * 1000) / time_diff);
        }
        // start another around of statistics
        self->send_stats.bytes_sent = 0;
        self->send_stats.start_time_ms = cur_time_ms;
        TSK_DEBUG_INFO ("Sent the %lld(th) RTP packet with result:%lu, avg bitrate:%d(bps), cur bitrate:%d(bps)",
                        self->send_stats.packet_count, ret, avg_bit_rate, cur_bit_rate);
    }
    self->send_stats.packet_count++;
    
    
    TSK_OBJECT_SAFE_FREE (packet);

    return ret;
}

trtp_rtp_packet_t* trtp_manager_create_packet_new(trtp_manager_t *self, const void *data, tsk_size_t size,
                                              const void *extension, tsk_size_t ext_size, uint64_t timestamp, tsk_bool_t iFrame, tsk_bool_t marker)
{
    trtp_rtp_packet_t *packet = tsk_null;
    int offset = 0;

    packet = trtp_rtp_packet_create (self->rtp.ssrc.local, self->rtp.video_seq_num, timestamp, self->rtp.video_payload_type, marker);
    if (!packet)
    {
        return tsk_null;
    }
    if (packet->header->csrc_count >= 15) {
        assert(0);
        TSK_DEBUG_WARN ("RTP header cannot hold the sessiong ID");
    }
    else
    {
        packet->header->csrc[0] = self->sessionid;
        if (packet->header->csrc_count < 1) {
            packet->header->csrc_count = 1;
        }
        packet->header->csrc[1] = iFrame ? 1 : 0;
        packet->header->csrc_count++;
        
        packet->header->session_id = self->sessionid;
    }
    
    packet->payload.data = tsk_calloc(1, size);
    packet->payload.size = size;
    memcpy((void*)packet->payload.data, data, size);

    if (self->uBusinessId != 0) {
        // If the header extension is available(at least 4 fixed bytes + 1 word(4 bytes of payload)
        int head_ext_Len_only_business_id = RTP_EXTENSION_MAGIC_NUM_LEN_PROFILE + RTP_EXTENSION_LEN_PROFILE + RTP_EXTENSION_TYPE_PROFILE + BUSINESS_ID_LEN_PROFILE + sizeof(uint64_t);
        int head_ext_len = head_ext_Len_only_business_id;
        if (extension && (ext_size >= 8)) {
            head_ext_len = ext_size + RTP_EXTENSION_TYPE_PROFILE + BUSINESS_ID_LEN_PROFILE + sizeof(uint64_t);
        }  

        packet->extension.data = (unsigned char *)tsk_malloc(head_ext_len);
        memset(packet->extension.data, 0, head_ext_len);
        packet->extension.size = head_ext_len;
        packet->header->extension = 1;

        offset = 0;
        if (head_ext_len > head_ext_Len_only_business_id) {
            // 原来extension的长度 + 2byte extension type + 2byte data len + data size
            uint16_t rtp_extension_len = ((((const char *)extension)[2] & 0xff) << 8) | (((const char *)extension)[3] & 0xff) << 2 + RTP_EXTENSION_TYPE_PROFILE + BUSINESS_ID_LEN_PROFILE + sizeof(uint64_t);
            uint16_t rtp_extension_len_net_order = htons(rtp_extension_len / 4);
            //原来extension的长度替换掉
            //如果传进来的extension有数据，那么在extension中增加businessId时，需要更改extension中的长度，由于传下来的extension是const类型，所以有冲突。为解决编译问题，暂时注释掉。
            // memcpy((const unsigned char *)extension + 2, &rtp_extension_len_net_order, sizeof(uint16_t));
            memcpy((unsigned char *)packet->extension.data, extension, ext_size);
            offset += ext_size;
        } else {
            //往rtp head extension中写入魔法数字
            uint16_t magic_num_net_order = htons(RTP_EXTENSION_MAGIC_NUM);
            memcpy((unsigned char *)packet->extension.data+offset, &magic_num_net_order, RTP_EXTENSION_MAGIC_NUM_LEN_PROFILE);
            offset += RTP_EXTENSION_MAGIC_NUM_LEN_PROFILE;

            //往rtp head extension中写入扩展内容占用的空间大小
            uint16_t rtp_extension_len = (RTP_EXTENSION_TYPE_PROFILE + BUSINESS_ID_LEN_PROFILE + sizeof(uint64_t)) / 4;
            uint16_t rtp_extension_len_net_order = htons(rtp_extension_len);
            memcpy((unsigned char *)packet->extension.data+offset, &rtp_extension_len_net_order, RTP_EXTENSION_LEN_PROFILE);
            offset += RTP_EXTENSION_LEN_PROFILE;
        }

        //往rtp head extension中写入内容数据类型
        uint16_t rtp_extension_type_net_order = htons(BUSINESSID);
        memcpy((unsigned char *)packet->extension.data+offset, &rtp_extension_type_net_order, RTP_EXTENSION_TYPE_PROFILE);
        offset += RTP_EXTENSION_TYPE_PROFILE;

        //往rtp head extension中写入内容数据空间大小
        uint16_t rtp_extension_type_len_net_order = htons(sizeof(uint64_t));
        memcpy((unsigned char *)packet->extension.data+offset, &rtp_extension_type_len_net_order, BUSINESS_ID_LEN_PROFILE);
        offset += BUSINESS_ID_LEN_PROFILE;

        //往rtp head extension中写入内容数据
        uint32_t business_id_net_order_high = htonl((self->uBusinessId >> 32) & 0xffffffff);
        memcpy((unsigned char *)packet->extension.data+offset, &business_id_net_order_high, sizeof(uint32_t));
        offset += sizeof(uint32_t);
        uint32_t business_id_net_order_low = htonl(self->uBusinessId & 0xffffffff);
        memcpy((unsigned char *)packet->extension.data+offset, &business_id_net_order_low, sizeof(uint32_t));
    } else {
        if (extension && (ext_size >= 8)) {
            packet->extension.data_const = extension;
            packet->extension.size = ext_size;
            packet->header->extension = 1;
        }
    }
    
    return packet;
}

trtp_rtp_packet_t* trtp_manager_create_packet(trtp_manager_t *self, const void *data, tsk_size_t size,
                                              const void *extension, tsk_size_t ext_size,
                                              uint32_t duration, tsk_bool_t iFrame, tsk_bool_t marker) {
    trtp_rtp_packet_t *packet = tsk_null;
    
    packet = trtp_rtp_packet_create (self->rtp.ssrc.local, self->rtp.video_seq_num, self->rtp.video_timestamp, self->rtp.video_payload_type, marker);
    if (!packet)
    {
        return tsk_null;
    }
    
    if (packet->header->csrc_count >= 15) {
        assert(0);
        TSK_DEBUG_WARN ("RTP header cannot hold the sessiong ID");
    }
    else
    {
        packet->header->csrc[0] = self->sessionid;
        if (packet->header->csrc_count < 1) {
            packet->header->csrc_count = 1;
        }
        packet->header->csrc[1] = iFrame ? 1 : 0;
        packet->header->csrc_count++;
        
        packet->header->session_id = self->sessionid;
    }
    
    if(marker) {
        self->rtp.video_timestamp += duration;
    }
    
    packet->payload.data = tsk_calloc(1, size);
    packet->payload.size = size;
    memcpy((void*)packet->payload.data, data, size);
    // If the header extension is available(at least 4 fixed bytes + 1 word(4 bytes of payload)
    if (extension && (ext_size >= 8)) {
        packet->extension.data_const = extension;
        packet->extension.size = ext_size;
        packet->header->extension = 1;
    }
    
    return packet;
}

// VIDEO rtp数据发送接口
tsk_size_t trtp_manager_send_video_rtp_with_extension (trtp_manager_t *self,
                                                       const void *data,
                                                       tsk_size_t size,
                                                       uint32_t duration,
                                                       tsk_bool_t marker,
                                                       tsk_bool_t last_packet,
                                                       const void *extension,
                                                       tsk_size_t ext_size,
                                                       tsk_bool_t iFrame,
                                                       tsk_bool_t repeat)
{
    trtp_rtp_packet_t *packet;
    tsk_size_t ret;

    if (!self || !self->transport || !data || !size)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return 0;
    }

    /* check if transport is started */
    if (!self->is_started || !self->transport->master)
    {
        TSK_DEBUG_WARN ("RTP engine not ready yet");
        return 0;
    }

    /* create packet with header */
    if (iFrame && repeat) {
        packet = trtp_rtp_packet_create (self->rtp.ssrc.local, self->rtp.video_seq_num, self->rtp.video_timestamp, self->rtp.video_payload_type, marker);
    } else {
        packet = trtp_rtp_packet_create (self->rtp.ssrc.local, ++self->rtp.video_seq_num, self->rtp.video_timestamp, self->rtp.video_payload_type, marker);
    }
    if (!packet)
    {
        return 0;
    }

    if (packet->header->csrc_count >= 15) {
        assert(0);
        TSK_DEBUG_WARN ("RTP header cannot hold the sessiong ID");
    }
    else
    {
        packet->header->csrc[0] = self->sessionid;
        if (packet->header->csrc_count <1) {
            packet->header->csrc_count=1;
        }
        packet->header->session_id = self->sessionid;
    }
    if (last_packet)
    {
        //if (!iFrame || repeat) {
            self->rtp.video_timestamp += duration;
        //}
    }

    packet->payload.data_const = data;
    packet->payload.size = size;
    // If the header extension is available(at least 4 fixed bytes + 1 word(4 bytes of payload)
    if (extension && (ext_size >= 8)) {
        packet->extension.data_const = extension;
        packet->extension.size = ext_size;
        packet->header->extension = 1;
    }

    ret = trtp_manager_send_rtp_packet (self, packet, tsk_false);

    /* Calculate the statistics data and print them perodically */
    self->send_stats.video_bytes_sent += ret;
    if ((self->send_stats.video_packet_count % 100) == 0)
    {
        // 打印视频日志
        TSK_DEBUG_INFO("video:local(%s:%d)-->remote(%s:%d) sessionId:%d ssrc:%u seq_num:%d timestamp:%d payload_type:%d sent_count:%lld sent_bytes:%lld",
                       self->rtp.public_ip, self->rtp.public_port,
                       self->rtp.remote_ip, self->rtp.remote_port,
                       self->sessionid,
                       self->rtp.ssrc.local, self->rtp.video_seq_num,
                       self->rtp.video_timestamp, self->rtp.video_payload_type,
                       self->send_stats.video_packet_count, self->send_stats.video_bytes_sent);
    }

    self->send_stats.video_packet_count++;

    TSK_OBJECT_SAFE_FREE (packet);

    return ret;
}

#if DUMP_SERIALIZE_DATA
    trtp_rtp_header_t* dumpHead(uint8_t *data, int size) {
        trtp_rtp_header_t *header = tsk_null;
        header = trtp_rtp_header_deserialize(data, size);
        TSK_DEBUG_INFO("bruce >>> header->seq_num=%d", header->seq_num);
        TSK_DEBUG_INFO("bruce >>> header->extension=%d", header->extension);
        TSK_DEBUG_INFO("bruce >>> header->csrc[0]=%d", header->csrc[0]);
        TSK_DEBUG_INFO("bruce >>> header->csrc[1]=%d", header->csrc[1]);
        return header;
    }

    void dumpExtension(void *data, int size) {
        uint8_t *buff = (uint8_t *)data;
        uint16_t magic_num = buff[0] << 8 | buff[1];
        TSK_DEBUG_INFO("bruce >>> extension magic code=0x%x", magic_num);
        buff += 2;
        uint16_t extension_len = (buff[0] << 8 | buff[1]) << 2;
        TSK_DEBUG_INFO("bruce >>> extension len=%d", extension_len);
        buff += 2;
        while (extension_len > 0) {
            uint16_t extension_type = buff[0] << 8 | buff[1];
            TSK_DEBUG_INFO("bruce >>> extension type=%d", extension_type);
            buff += 2;
            uint16_t extension_data_len = buff[0] << 8 | buff[1];
            TSK_DEBUG_INFO("bruce >>> extension data len=%d", extension_data_len);
            buff += 2;
            switch (extension_type) {
                uint32_t extension_data_high = 0;
                uint32_t extension_data_low = 0;
                case BUSINESSID:
                    memcpy(&extension_data_high, buff, extension_data_len/2);
                    buff += extension_data_len/2;
                    memcpy(&extension_data_low, buff, extension_data_len/2);
                    uint64_t businessId = (uint64_t)ntohl(extension_data_high) << 32 | ntohl(extension_data_low);
                    TSK_DEBUG_INFO("bruce >>> extension data=%lld", businessId);
                    extension_len -= 4+extension_data_len;
                    buff += extension_data_len/2;
                    break;
                default:
                    TSK_DEBUG_ERROR("bruce >>> unkonw extension type");
                    break;
            }
        }
    }

    void dumpData(void *data, int size) {
        uint8_t *buff = (uint8_t *)data;
        TSK_DEBUG_INFO("bruce >>> data=%s", buff);
    }
#endif

// serialize, encrypt then send the data
tsk_size_t trtp_manager_send_rtp_packet (trtp_manager_t *self, const struct trtp_rtp_packet_s *packet, tsk_bool_t bypass_encrypt)
{
    int ret = 0;
    tsk_size_t rtp_buff_pad_count = 0;
    tsk_size_t xsize;

    /* check validity */
    if (!self || !packet)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return 0;
    }

    tsk_safeobj_lock (self);

    /* check if transport is started */
    if (!self->is_started || !self->transport || !self->transport->master)
    {
        TSK_DEBUG_WARN ("RTP engine not ready yet");
        ret = 0;
        goto bail;
    }

    xsize = (trtp_rtp_packet_guess_serialbuff_size (packet) + rtp_buff_pad_count);
    if (self->rtp.serial_buffer.size < xsize)
    {
        if (!(self->rtp.serial_buffer.ptr = tsk_realloc (self->rtp.serial_buffer.ptr, xsize)))
        {
            TSK_DEBUG_ERROR ("Failed to allocate buffer with size = %d", (int)xsize);
            self->rtp.serial_buffer.size = 0;
            goto bail;
        }
        self->rtp.serial_buffer.size = xsize;
    }

    /* serialize and send over the network */
    if ((ret = (int)trtp_rtp_packet_serialize_to (packet, self->rtp.serial_buffer.ptr, xsize)))
    {
        void *data_ptr = self->rtp.serial_buffer.ptr;
        int data_size = ret;
		//这里是发送音视频数据，只有视频需要加密
		int bIsEncrypt = 0;
		if ((packet->header->payload_type == TRTP_H264_MP) && (self->pszEncryptPasswd != NULL))
		{
			data_size = Encrypt_Data(data_ptr, data_size,self->pszEncryptPasswd, &data_ptr);
			bIsEncrypt = 1;
		}

#if DUMP_SERIALIZE_DATA
        int remain = data_size;
        uint8_t *tmp = (uint8_t *)data_ptr;
        if (remain < TRTP_RTP_HEADER_MIN_SIZE + sizeof(uint64_t)) {
            TSK_DEBUG_ERROR("bruce >>> header is error");
            goto fail;
        }

        trtp_rtp_header_t* header = dumpHead(tmp, remain);
        tmp += TRTP_RTP_HEADER_MIN_SIZE + sizeof(uint64_t);
        remain -= TRTP_RTP_HEADER_MIN_SIZE + sizeof(uint64_t);
        
        if (header->extension == 1) {
            int extension_len = 4 + ((tmp[2] << 8 | tmp[3]) << 2);
            TSK_DEBUG_INFO("bruce >>> all extension len=%d", extension_len);
            if (remain > extension_len) {
                dumpExtension(tmp, remain);
                remain -= extension_len;
                tmp += extension_len;
            }
        }

        if (remain > 0) {
            dumpData(tmp, remain);
        }
fail:
    free((void *)header);
    header = NULL;
#endif

        if (/* number of bytes sent */ (ret = (int)trtp_manager_send_rtp_raw (self, data_ptr, data_size)) > 0)
        {
            // for rtcp stat
            if (self->rtcp.session) {
                trtp_rtcp_session_process_rtp_out(self->rtcp.session, packet, data_size);
            }
            // stat send packet counts/bytes (audio & video)
            if (TRTP_H264_MP == packet->header->payload_type) {
                /* Calculate the statistics video data and print them perodically */
                self->send_stats.video_bytes_sent += ret;
                if ((self->send_stats.video_packet_count % 5000) == 0) {
                    uint64_t cur_time_ms = tsk_gettimeofday_ms();
                    uint32_t avg_bit_rate = 0;
                    uint32_t cur_bit_rate = 0;
                    // The time validity needs to be checked carefully. Otherwise it may cause
                    // a divided-by-0 exception.
                    if ((self->send_stats.video_start_time_ms > 0) && (cur_time_ms > self->send_stats.video_start_time_ms)) {
                        uint64_t time_diff = cur_time_ms - self->send_stats.video_start_time_ms;
                        self->send_stats.video_total_bytes_sent += self->send_stats.video_bytes_sent;
                        self->send_stats.video_total_time_ms += time_diff;
                        avg_bit_rate = (uint32_t)((self->send_stats.video_total_bytes_sent * 8) / self->send_stats.video_total_time_ms);
                        cur_bit_rate = (uint32_t)((self->send_stats.video_bytes_sent * 8) / time_diff);
                    }
                    // start another around of statistics
                    self->send_stats.video_bytes_sent = 0;
                    self->send_stats.video_start_time_ms = cur_time_ms;
                    TSK_DEBUG_INFO ("Sent video %lld(th) RTP packet, avg bitrate:%u(kbps), cur bitrate:%u(kbps)",
                                    self->send_stats.video_packet_count, avg_bit_rate, cur_bit_rate);
                }
                self->send_stats.video_packet_count++;

            } else if (TRTP_OPUS_MP == packet->header->payload_type) {
                /* Calculate the statistics audio data and print them perodically */
                self->send_stats.bytes_sent += ret;
                if ((self->send_stats.packet_count % 1000) == 0) {
                    uint64_t cur_time_ms = tsk_gettimeofday_ms();
                    uint32_t avg_bit_rate = 0;
                    uint32_t cur_bit_rate = 0;
                    // The time validity needs to be checked carefully. Otherwise it may cause
                    // a divided-by-0 exception.
                    if ((self->send_stats.start_time_ms > 0) && (cur_time_ms > self->send_stats.start_time_ms)) {
                        uint64_t time_diff = cur_time_ms - self->send_stats.start_time_ms;
                        self->send_stats.total_bytes_sent += self->send_stats.bytes_sent;
                        self->send_stats.total_time_ms += time_diff;
                        avg_bit_rate = (uint32_t)((self->send_stats.total_bytes_sent * 8 * 1000) / self->send_stats.total_time_ms);
                        cur_bit_rate = (uint32_t)((self->send_stats.bytes_sent * 8 * 1000) / time_diff);
                    }
                    // start another around of statistics
                    self->send_stats.bytes_sent = 0;
                    self->send_stats.start_time_ms = cur_time_ms;
                    TSK_DEBUG_INFO ("Sent audio %lld(th) RTP packet avg bitrate:%d(bps), cur bitrate:%d(bps)",
                                    self->send_stats.packet_count, avg_bit_rate, cur_bit_rate);
                }
                self->send_stats.packet_count++;
            }
        } else {
            ret = 0;
        }

		if (bIsEncrypt)
		{
			//加密了的s
			TSK_FREE(data_ptr);
		}

    }
    else
    {
        TSK_DEBUG_ERROR ("Failed to serialize RTP packet");
    }

bail:
    tsk_safeobj_unlock (self);
    return ret;
}

// send raw data "as is" without adding any RTP header or SRTP encryption
tsk_size_t trtp_manager_send_rtp_raw (trtp_manager_t *self, const void *data, tsk_size_t size)
{
    tsk_size_t ret = 0;
    
    if (!self || !data || !size) {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return 0;
    }
    
    tsk_safeobj_lock (self);

    if (!self->transport || !self->transport->master)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        tsk_safeobj_unlock (self);
        return 0;
    }


	if (TNET_SOCKET_TYPE_IS_STREAM(self->transport->type))
	{
		ret = tnet_transport_send_wrapper(self->transport, self->transport->master->fd, data, size);
	}
	else
	{
        if (self->p2p_route_check_end) {
            if (self->p2p_route_enable) {
                ret = tnet_sockfd_sendto(self->transport_p2p->master->fd, (const struct sockaddr *)&self->p2p_connect_info.p2p_remote_addr, data, size);
            } else if (!self->p2p_connect_info.p2p_info_set_flag || (self->p2p_connect_info.p2p_info_set_flag && self->p2p_route_change_flag) ) {
                ret = tnet_sockfd_sendto(self->transport->master->fd, (const struct sockaddr *)&self->rtp.remote_addr, data, size); 
            }
        } else {
            // for p2p route check
            if (self->transport_p2p) {
                ret = tnet_sockfd_sendto(self->transport_p2p->master->fd, (const struct sockaddr *)&self->p2p_connect_info.p2p_remote_addr, data, size);
            }
        }
	}

    tsk_safeobj_unlock (self);
    return ret;
}

// send raw data "as is" without adding any RTP header or SRTP encryption
tsk_size_t trtp_manager_send_rtcp_data (trtp_manager_t *self, const void *data, tsk_size_t size)
{
    tsk_size_t ret = 0;
    
    if (!self || !data || !size) {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return 0;
    }
    
    tsk_safeobj_lock (self);
    
    if (!self->transport_report || !self->transport_report->master)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        tsk_safeobj_unlock (self);
        return 0;
    }
	if (TNET_SOCKET_TYPE_IS_STREAM(self->transport_report->type))
	{
		ret = tnet_transport_send_wrapper(self->transport_report, self->transport_report->master->fd, data, size);
	}
	else
	{
        // send half-loss req to mcu server
		ret = tnet_sockfd_sendto(self->transport_report->master->fd, (const struct sockaddr *)&self->report.remote_addr, data, size); // returns number of sent bytes
	}
    
    tsk_safeobj_unlock (self);
    return ret;
}

int trtp_manager_get_source_info(trtp_manager_t* self, uint32_t ssrc, uint32_t* sendstatus, uint32_t* source_type)
{
    int ret = 0;

    if (!self || !self->rtcp.session) {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return 0;
    }

    return trtp_rtcp_session_get_source_info(self->rtcp.session, ssrc, sendstatus, source_type);
}

int trtp_manager_set_source_type(trtp_manager_t* self, uint32_t ssrc, uint32_t source_type)
{
    int ret = 0;

    if (!self || !self->rtcp.session) {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return 0;
    }

    return trtp_rtcp_session_set_source_type(self->rtcp.session, ssrc, source_type);
}

int trtp_manager_set_send_status(trtp_manager_t* self, uint32_t sendstatus)
{
    if (!self) {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return 0;
    }

    unsigned width = 0, height = 0;
    tmedia_defaults_get_video_size_child(&width, &height);
    int minor_enable = (width*height) > 0 ? 1: 0;

    if (!minor_enable &&  TRTP_VIDEO_STEAM_MINOR == sendstatus) {
        TSK_DEBUG_ERROR ("minor stream disable, not support sendstatus:%d", sendstatus);
        return 0;
    }

    TSK_DEBUG_INFO ("set video origin status:%d, sendstatus:%u", self->ivideoSendStatus, sendstatus);
    self->ivideoSendStatus = sendstatus;
    self->iForceSendStatus = (sendstatus == TRTP_VIDEO_STEAM_MINOR) ? 1 : 0;

    return 0;
}

void trtp_manager_set_send_flag(trtp_manager_t* self, uint32_t send_flag)
{
    if (!self) {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return;
    }

    if (send_flag == self->iDataSendFlag) {
        return;
    }

    int souceCount = trtp_rtcp_session_get_source_count(self->rtcp.session);
    if (0 == send_flag && souceCount > 0) {
        TSK_DEBUG_INFO ("souce count[%d] is more than 0, send data continue", souceCount);
        return;
    }

    TSK_DEBUG_INFO ("set video send flag:%u", send_flag);
    self->iDataSendFlag = send_flag;
}

void trtp_manager_get_recv_stat(trtp_manager_t* self, uint64_t* recvStat)
{
    if (!self) {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return;
    }

    uint64_t now_timestamp = tsk_time_now();
    int32_t tick_interval = now_timestamp - self->recvLastTickStart;
    if (self->recvLastTickStart && tick_interval > 0) {
        *recvStat = self->recvDataStat * 1000 / tick_interval;
        self->recvDataStat = 0;
    }
}

int trtp_manager_set_app_bandwidth_max (trtp_manager_t *self, int32_t bw_upload_kbps, int32_t bw_download_kbps)
{
    if (self)
    {
        self->app_bw_max_upload = bw_upload_kbps;
        self->app_bw_max_download = bw_download_kbps;
        return 0;
    }
    return -1;
}
int trtp_manager_signal_pkt_loss (trtp_manager_t *self, uint32_t ssrc_media, const uint16_t *seq_nums, tsk_size_t count)
{

    return -1;
}
int trtp_manager_signal_frame_corrupted (trtp_manager_t *self, uint32_t ssrc_media)
{
    if(self && self->rtcp.session) {
        return trtp_rtcp_session_signal_frame_corrupted(self->rtcp.session, ssrc_media);
    }
    return -1;
}

int trtp_manager_signal_jb_error (trtp_manager_t *self, uint32_t ssrc_media)
{
    
    return -1;
}

uint32_t trtp_manager_get_current_timestamp (trtp_manager_t *self)
{
    if (self) {
        return self->rtp.timestamp;
    }
    
    return 0;
}

void trtp_manager_set_recording_time_ms (trtp_manager_t *self, uint32_t time_ms, uint32_t rtp_clock_rate)
{
    if (self) {
        self->rtp.timestamp = (uint32_t)(((uint64_t)time_ms * rtp_clock_rate) / 1000);
        if (tsk_false == self->packet_queue.is_recording_time_set) {
            TSK_DEBUG_INFO("Set first recording time:%u(ms)", time_ms);
            self->packet_queue.is_recording_time_set = tsk_true;
        }
    }
}

void trtp_manager_set_playing_time_ms (trtp_manager_t *self, uint32_t time_ms, uint32_t rtp_clock_rate)
{
    if (self) {
        if (0 == self->packet_queue.last_time_of_day_ms) {
            TSK_DEBUG_INFO("Set first playing time:%u(ms)", time_ms);
        }
        self->packet_queue.rtp_clock_rate = rtp_clock_rate;
        // Add a latency to the playing time. This latency is for a packet to be transferred from the RTP manager to the audio driver.
        time_ms += 600;
        // Convert the time from milli-second to RTP clock
        self->packet_queue.playing_time = (uint32_t)(((uint64_t)time_ms * rtp_clock_rate) / 1000);
        self->packet_queue.timestamp_threshold_drop = -(((int64_t)200 * rtp_clock_rate) / 1000);
        self->packet_queue.timestamp_threshold_wait = ((int64_t)40 * rtp_clock_rate) / 1000;
        self->packet_queue.last_time_of_day_ms = tsk_gettimeofday_ms();
    }
}


/** Stops the RTP/RTCP manager */
int trtp_manager_stop (trtp_manager_t *self)
{
    int ret = 0;
    stop_media_ctl( self );
    
    if (self->lossrate_check_timerid != 0) {
        xt_timer_mgr_global_cancel( self->lossrate_check_timerid );
        self->lossrate_check_timerid = 0;
    }

    if (!self) {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    TSK_DEBUG_INFO ("trtp_manager_stop():self=%d, is_start:%d", self, self->is_started);

    tsk_safeobj_lock (self);

    // We haven't started the ICE context which means we must not stop it
    // if(self->ice_ctx){
    //	ret = tnet_ice_ctx_stop(self->ice_ctx);
    //}
    
    // 判断is_start和is_socket_disabled标志位是否置为false
    if (!self->is_started)
    {
        // 已经释放资源，解锁返回
        TSK_DEBUG_INFO("trtp_manager_stop():freed..is_started:%d is_socket_disabled:%d",
                       self->is_started, self->is_socket_disabled);
        
        tsk_safeobj_unlock (self);
        
        return ret;
    }

    self->is_started = tsk_false;
    self->rtp_send_thread_running = tsk_false;
    tsk_safeobj_unlock (self);
    
    if(self->send_thread_cond){
        tsk_cond_set(self->send_thread_cond);
    }
    //下面不能加锁
    if(self->rtp_send_thread_handle) {
        tsk_semaphore_increment(self->rtp_send_sema);
        tsk_thread_join(&self->rtp_send_thread_handle);
        tsk_semaphore_destroy(&self->rtp_send_sema);
    }
    
    if(self->send_thread_cond){
        tsk_cond_destroy(&self->send_thread_cond);
    }
    
    tsk_safeobj_lock (self);
    
    tsk_list_lock(self->sorts_list);
    tsk_list_item_t* item = tsk_null;
    tsk_list_foreach(item, self->sorts_list) {
        trtp_sort_stop((TRTP_SORT(item->data)));
    }
    tsk_list_unlock(self->sorts_list);

    // Stop the RTCP session first (will send BYE)
    if(self->rtcp.session) {
        ret = trtp_rtcp_session_stop(self->rtcp.session);
        ret = trtp_rtcp_session_set_net_transport(self->rtcp.session, tsk_null);
    }
	
    // Free transport to force next call to start() to create new one with new sockets
    if (self->transport)
    {
        // unregister callbacks
        ret = tnet_transport_set_callback (self->transport, tsk_null, tsk_null);

        tnet_socket_t *master_copy = tsk_object_ref (self->transport->master); // "tnet_transport_shutdown" will free the master
        tnet_transport_shutdown (self->transport);
        TSK_OBJECT_SAFE_FREE (master_copy);
        TSK_OBJECT_SAFE_FREE (self->transport);
    }

    // Free p2p transport to force next call to start() to create new one with new sockets
    if (self->transport_p2p)
    {
        // unregister callbacks
        ret = tnet_transport_set_callback (self->transport_p2p, tsk_null, tsk_null);

        tnet_socket_t *master_copy = tsk_object_ref (self->transport_p2p->master); 
        tnet_transport_shutdown (self->transport_p2p);
        TSK_OBJECT_SAFE_FREE (master_copy);
        TSK_OBJECT_SAFE_FREE (self->transport_p2p);
    }

    // Free rtp half report info to make sure these values will be updated in next start()
    if( self->transport_report )
    {
        tnet_transport_set_callback (self->transport_report, tsk_null, tsk_null);
        
        tnet_socket_t *master_copy = tsk_object_ref (self->transport_report->master);
        tnet_transport_shutdown (self->transport_report);
        TSK_OBJECT_SAFE_FREE (master_copy);
        TSK_OBJECT_SAFE_FREE (self->transport_report);
    }

    TSK_OBJECT_SAFE_FREE(self->rtcp.local_socket);
    TSK_OBJECT_SAFE_FREE(self->rtcp.session);
    self->rtcp.public_addr.port = self->rtcp.remote_port = 0;
    TSK_FREE(self->rtcp.public_addr.ip);
    TSK_FREE(self->rtcp.remote_ip);
  
    // reset default values
    self->is_socket_disabled = tsk_false;

    TSK_FREE(self->p2p_connect_info.p2p_local_ip);
    self->p2p_connect_info.p2p_local_port = 0;
    TSK_FREE(self->p2p_connect_info.p2p_remote_ip);
    self->p2p_connect_info.p2p_remote_port = 0;   
    self->p2p_connect_info.p2p_info_set_flag = tsk_false;

	Destroy_ByteArray(self->tcpPacketBuffer);
	TSK_FREE(self->pszEncryptPasswd);
    // self->is_started = tsk_false;
    
    Config_SetInt("video_bitrate_level", 100);
    Config_SetInt("video_bitrate_level_second", 100);
    
    tsk_safeobj_unlock (self);
    TSK_DEBUG_INFO ("trtp_manager_stop end, ret:%d", ret);
    return ret;
}



static const tsk_object_def_t trtp_rtp_send_status_def_s =
{
    sizeof(send_packet_status_t),
};
const tsk_object_def_t *trtp_rtp_send_packet_t = &trtp_rtp_send_status_def_s;


int trtp_manager_push_rtp_packet (trtp_manager_t *self, trtp_rtp_packet_t *packet) {
    if(!self || !packet) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return -1;
    }
    self->isSendDataToServer++;
  
    //会存在多路的情况，所以还需要解出videoid
    send_packet_status_t* status = trtp_manager_get_send_status(self, packet->header->video_id);
    if (tsk_null == status) {
        //第一次直接创建一个就行了
        status = tsk_object_new(trtp_rtp_send_packet_t);
        status->VideoID = packet->header->video_id;
        status->bLossPacket = tsk_false;
        status->LastFrameSerial = packet->header->frameSerial;
        status->LastFrameType = packet->header->frameType;
        tsk_list_lock(self->send_packets_status_list);
        tsk_list_push_data(self->send_packets_status_list, &status,tsk_false);
        tsk_list_unlock(self->send_packets_status_list);
    }
    
    tsk_list_lock(self->rtp_packets_list);
    //总的原则，如果上一次丢过了一帧中的任何一个包，这一帧都必须丢掉。无论是I 还是P
    tsk_boolean_t bFirstInsert = tsk_true;
    if (status != tsk_null) {
        if ((status->bLossPacket) && (status->LastFrameSerial == packet->header->frameSerial )){
            bFirstInsert = tsk_false;
        }
    }
    if (bFirstInsert) {
         if(tsk_list_count_all(self->rtp_packets_list) < MAX_SEND_LIST_COUNT) {
             //先判断是否要塞进去
             tsk_boolean_t bSecondInsert = tsk_true;
             //判断一下，忽略第一个包
             if (tsk_null != status) {
                 if ((status->bLossPacket) && (packet->header->frameType == 0)) {
                     //上一次丢过，并且这次不是I 帧，那也丢掉,一直到出现一个I 帧
                     bSecondInsert = tsk_false;
                 }
                 status->bLossPacket=!bSecondInsert;
                 status->LastFrameType=packet->header->frameType;
                 status->LastFrameSerial = packet->header->frameSerial;
             }
             
             if (bSecondInsert) {
                 tsk_object_ref(TSK_OBJECT(packet));
                 tsk_list_push_back_data(self->rtp_packets_list, (void **)&packet);
             } else {
                //装不下了，上次丢过p帧，这次继续丢
                TSK_DEBUG_WARN("pkt list full, drop frame %d ",(int)packet->header->frameSerial);
             }
          }
        else
        {
            tsk_boolean_t bThirdInsert = tsk_false;
            if (packet->header->frameType == 0) {
                //如果是P 帧的话，需要判断之前是否已经插入了帧序号相同的P 帧，如果已经插入了，那这属于半截的，还是要放进去
                if ((packet->header->frameSerial == status->LastFrameSerial) && (!status->bLossPacket)) {
                    bThirdInsert=tsk_true;//还是要放进去
                } else {
                    TSK_DEBUG_WARN("pkt list full, drop P frame %d ",(int)packet->header->frameSerial);
                } 
            } else {
                //I 帧 需要特殊处理，最大保证不丢，但是也要控制，否则延迟会越来越大
                if (tsk_list_count_all(self->rtp_packets_list) > 2* MAX_SEND_LIST_COUNT) {
                    //塞的过程中达到了最大延迟了，确保上一次塞进去的I 帧都要继续塞进去，否则就白塞了
                    if (status->LastFrameSerial == packet->header->frameSerial) {
                        bThirdInsert=tsk_true;
                    } else {
                         TSK_DEBUG_WARN("pkt list full, drop I frame %d ",(int)packet->header->frameSerial);
                    }
                } else {
                    //没有达到最大延迟
                    bThirdInsert = tsk_true;
                }
            }
            
            status->bLossPacket=!bThirdInsert;
            status->LastFrameType=packet->header->frameType;
            status->LastFrameSerial = packet->header->frameSerial;
            if (bThirdInsert) {
                tsk_object_ref(TSK_OBJECT(packet));
                tsk_list_push_back_data(self->rtp_packets_list, (void **)&packet);
            } else {
                //装不下了 丢掉一个
                TSK_DEBUG_WARN("pkt list full,drop frame %d ",(int)packet->header->frameSerial);
            }
        }
    }
    else
    {
        //上一次丢过，这次也要丢掉
        TSK_DEBUG_WARN("pkt list full ,drop frame %d ",(int)packet->header->frameSerial);
    }
    
    tsk_list_unlock(self->rtp_packets_list);
    tsk_semaphore_increment(self->rtp_send_sema);
        
    return 0;
}

uint64_t trtp_manager_get_video_duration (trtp_manager_t *self) {
    uint64_t duration = 0;
    if(!self->sorts_list) {
        TSK_DEBUG_ERROR("Invalid parameter");
        return 0;
    }
    
    tsk_list_item_t *item = tsk_null;
    tsk_list_lock(self->sorts_list);
    tsk_list_foreach(item, self->sorts_list) {
        if(TRTP_H264_MP == TRTP_SORT(item->data)->payload_type) {
            duration += trtp_sort_get_duration(TRTP_SORT(item->data));
        }
    }
    tsk_list_unlock(self->sorts_list);
    
    TSK_DEBUG_INFO("the total duration is %lld ms", duration);
    return duration;
}

//=================================================================================================
//	RTP manager object definition
//
static tsk_object_t *trtp_manager_ctor (tsk_object_t *self, va_list *app)
{
    trtp_manager_t *manager = (trtp_manager_t *)self;
    if (manager)
    {
        manager->port_range.start = tmedia_defaults_get_rtp_port_range_start ();
        manager->port_range.stop = tmedia_defaults_get_rtp_port_range_stop ();
        // manager->is_force_symetric_rtp = tmedia_defaults_get_rtp_symetric_enabled ();
        manager->app_bw_max_upload = INT_MAX;   // INT_MAX or <=0 means undefined
        manager->app_bw_max_download = INT_MAX; // INT_MAX or <=0 means undefined
        manager->app_jitter_cng = 1.f; // Within [0, 1], in quality metric unit: 1 is best, 0 worst

        /* rtp */
        // Audio part
        manager->rtp.timestamp = rand () ^ rand ();
        manager->rtp.seq_num = rand () ^ rand ();
        // Video part
        manager->rtp.video_timestamp = manager->rtp.timestamp; // 保证用同一个源，做AV sync
        manager->rtp.video_seq_num = rand () ^ rand ();
        manager->rtp.video_segment_seq_num = rand () ^ rand ();
        manager->rtp.video_first_packet_seq_num = 0;
        
        manager->rtp.video_segment_seq_num_second = rand() ^ rand();
        manager->rtp.video_first_packet_seq_num_second = 0;

        manager->rtp.video_segment_seq_num_share = rand() ^ rand();
        manager->rtp.video_first_packet_seq_num_share = 0;

        manager->rtp.ssrc.local = rand () ^ rand () ^ (int)tsk_time_epoch ();
        manager->rtp.dscp = TRTP_DSCP_RTP_DEFAULT;
        manager->sessionid = -1;
        manager->last_time_recv_packet_ms = 0;
        manager->last_time_send_dummy_packet_ms = 0;
        manager->last_time_print_recv_packet_ms = 0;
        
        manager->rtp_send_thread_running = tsk_false;
		
        /* rtcp */
        {
            // use MD5 string to avoid padding issues
            tsk_md5string_t md5 = { 0 };
            tsk_sprintf(&manager->rtcp.cname, "doubango.%llu", (tsk_time_now() + rand()));
            tsk_md5compute(manager->rtcp.cname, tsk_strlen(manager->rtcp.cname), &md5);
            tsk_strupdate(&manager->rtcp.cname, md5);
        }
		
        if(!(manager->rtp_packets_list = tsk_list_create())) {
            TSK_DEBUG_ERROR("Failed to create rtp packets list.");
        }
        
        manager->send_packets_status_list = tsk_list_create();
        if(!(manager->sorts_list = tsk_list_create())) {
            TSK_DEBUG_ERROR("Failed to create rtp sorts list.");
        }
        
        manager->media_ctl_timerid = 0;
        manager->lossrate_check_timerid = 0;
        
        manager->packet_total = 0 ;
        manager->packet_loss = 0 ;
        manager->iStaticCount = 4 ;
        manager->iLastVideoBitrate=100;
        manager->iLastVideoFrame= 100;
        manager->iLastNoLossBitrate = 100;
        manager->iUpBitrateLevel = 5;
        manager->iMinBitRatePercent = 50;
        manager->bUpBitrate = tsk_false;
        
        manager->isSendDataToServer = 0;
        manager->isSendDataReceived = 0;
        manager->iLastRtpPass = 0;
        
        manager->iUpBitrateStaticCount = 0;
        
        manager->iLastDataRtt = 0;
        manager->iLastDeltaRtt = 0;

        // p2p connect info
        manager->p2p_connect_info.p2p_info_set_flag = tsk_false;
        manager->p2p_connect_info.p2p_local_ip = tsk_null;
        manager->p2p_connect_info.p2p_local_port = 0;
        manager->p2p_connect_info.p2p_remote_ip = tsk_null;
        manager->p2p_connect_info.p2p_remote_port = 0;

        // packet queue
        tsk_safeobj_init (&(manager->packet_queue));

        tsk_safeobj_init (manager);
    }
    return self;
}

static tsk_object_t *trtp_manager_dtor (tsk_object_t *self)
{
    trtp_manager_t *manager = self;
    if (manager)
    {
        if (manager->transport)
        {
        }

        /* stop */
        if (manager->is_started)
        {
            trtp_manager_stop (manager);
        }

        TSK_OBJECT_SAFE_FREE (manager->transport);

        TSK_FREE (manager->local_ip);

        /* rtp */
        TSK_FREE (manager->rtp.remote_ip);
        TSK_FREE (manager->rtp.public_ip);
        TSK_FREE (manager->rtp.serial_buffer.ptr);
        TSK_FREE (manager->report.remote_ip);
        
        TSK_OBJECT_SAFE_FREE(manager->rtp_packets_list);
        TSK_OBJECT_SAFE_FREE(manager->send_packets_status_list);
        
        
        TSK_OBJECT_SAFE_FREE(manager->sorts_list);

        /* rtcp */
        TSK_OBJECT_SAFE_FREE(manager->rtcp.session);
        TSK_FREE(manager->rtcp.remote_ip);
        TSK_FREE(manager->rtcp.public_addr.ip);
        TSK_FREE(manager->rtcp.cname);
        TSK_OBJECT_SAFE_FREE(manager->rtcp.local_socket);
		
        // packet queue
        while (tsk_null != manager->packet_queue.head) {
            trtp_rtp_packet_t* cur_packet = manager->packet_queue.head;
            manager->packet_queue.head = cur_packet->next;
            TSK_OBJECT_SAFE_FREE (cur_packet);
        }
        

        tsk_safeobj_deinit (&(manager->packet_queue));

        tsk_safeobj_deinit (manager);
        
       
        TSK_DEBUG_INFO ("*** RTP manager destroyed ***");
    }

    return self;
}

static const tsk_object_def_t trtp_manager_def_s = {
    sizeof (trtp_manager_t), 
	trtp_manager_ctor, 
	trtp_manager_dtor, 
	tsk_null,
};
const tsk_object_def_t *trtp_manager_def_t = &trtp_manager_def_s;

void send_media_ctl_req( trtp_manager_t *self,  int isStart )
{
    int buffLen = 1000;
    char buff[1000] = {0};

    unsigned width = 0, height = 0;
    tmedia_defaults_get_video_size_child(&width, &height);
    int minor_enable = (width*height) > 0 ? 1: 0;

    int stat_type = 0;
    if (TRTP_VIDEO_STEAM_NORMAL == self->ivideoSendStatus) {
        if (minor_enable) {
            stat_type = 3; // main and minor
        } else {
            stat_type = 1; // main
        }
        
    } else if (TRTP_VIDEO_STEAM_MINOR == self->ivideoSendStatus) {
        stat_type = 2; // minor
    }

    TMEDIA_I_AM_ACTIVE(5, "send_media_ctl_req isStart[%d] stat_type[%d]", isStart, stat_type);
    if (self->iLastvideoSendStatus != self->ivideoSendStatus) {
        self->iLastvideoSendStatus = self->ivideoSendStatus;
    }
    
    int ret = GetMediaCtlReqBuffer( isStart, self->sessionid, stat_type, buff, &buffLen);
    tsk_size_t  result = 0;
    if( ret == 0 )
    {
         result = trtp_manager_send_rtcp_data( self , buff, buffLen );
    }
}

void start_media_ctl( trtp_manager_t *self )
{
#ifdef YOUME_VIDEO
    send_media_ctl_req( self ,  1 );
    
    if( self->media_ctl_timerid != 0 )
    {
        xt_timer_mgr_global_cancel( self->media_ctl_timerid );
        self->media_ctl_timerid = 0;
    }
    
    //如果是启动，要保证服务器收到这个包
    self->media_ctl_timerid = xt_timer_mgr_global_schedule_loop( TRTP_HALF_RTCP_REPORT_TIME, onMediaCtlTimer, self );
#endif
}

void stop_media_ctl( trtp_manager_t *self )
{
#ifdef YOUME_VIDEO
    if( self->media_ctl_timerid != 0 )
    {
        xt_timer_mgr_global_cancel( self->media_ctl_timerid );
        self->media_ctl_timerid = 0;
    }
    
    send_media_ctl_req( self ,  0 );
#endif
}

// 主要用于调整视频码率
// self: trtp manager实例
// video_id: 0: main stream; 1: minor stream; 2: share stream
// mode: 0: decrease; 1:increase
// lossrate: 当前丢包率（根据实际情况可能是全程或者半程丢包率）
static int adjustVideoBitrate(trtp_manager_t* self, int video_id, int mode, int lossRate) {
    
    // sanity check
    if (!self) {
        TSK_DEBUG_ERROR("adjustVideoBitrate input invalid");
        return -1;
    }

    if (0 == video_id) {
        if (TRTP_VIDEO_ADJUST_MODE_DECREASE == mode) {
            // decrease main
            if (lossRate >= 25) {
                self->iLastVideoBitrate -= 50;
            } else if (lossRate >= 20) {
                self->iLastVideoBitrate -= 30;
            } else if (lossRate >= 10) {
                self->iLastVideoBitrate -= 20;
            } else {
                self->iLastVideoBitrate -= 10;
            }        
        } else {
            // increase main
            self->iLastVideoBitrate += 15;
        }
    } else if (1 == video_id) {
        if (TRTP_VIDEO_ADJUST_MODE_DECREASE == mode) {
            // decrease minor
            if (lossRate >= 25) {
                self->iLastMinorVideoBitrate -= 50;
            } else if (lossRate >= 20) {
                self->iLastMinorVideoBitrate -= 30;
            } else if (lossRate >= 10) {
                self->iLastMinorVideoBitrate -= 20;
            } else {
                self->iLastMinorVideoBitrate -= 10;
            }
        } else {
            // increse minor
            self->iLastMinorVideoBitrate += 15;
        }
    }

    return 0;
}

// 主要用于调整视频帧率
// self: trtp manager实例
// video_id: 0: main stream; 1: minor stream; 2: share stream
// mode: 0: decrease; 1:increase
// lossrate: 当前丢包率（根据实际情况可能是全程或者半程丢包率）
static int adjustVideofps(trtp_manager_t* self, int video_id, int mode, int lossRate) {
    // sanity check
    if (!self) {
        TSK_DEBUG_ERROR("adjustVideofps input invalid");
        return -1;
    }

    if (0 == video_id) {
        if (TRTP_VIDEO_ADJUST_MODE_DECREASE == mode) {
            // decrease main
            if (lossRate >= 25) {
                self->iLastVideoFps -= 50;
            } else if (lossRate >= 20) {
                self->iLastVideoFps -= 30;
            } else if (lossRate >= 10) {
                self->iLastVideoFps -= 20;
            } else {
                self->iLastVideoFps -= 10;
            }        
        } else {
            // increase main
            self->iLastVideoFps += 10;
        }
    } else if (1 == video_id) {
        if (TRTP_VIDEO_ADJUST_MODE_DECREASE == mode) {
            // decrease minor
            if (lossRate >= 25) {
                self->iLastMinorVideoFps -= 50;
            } else if (lossRate >= 20) {
                self->iLastMinorVideoFps -= 30;
            } else if (lossRate >= 10) {
                self->iLastMinorVideoFps -= 20;
            } else {
                self->iLastMinorVideoFps -= 10;
            }
        } else {
            // increse minor
            self->iLastMinorVideoFps += 15;
        }
    }

    return 0;
}

// 主要用于调整视频
// self: trtp manager实例
// video_id: 0: main stream; 1: minor stream; 2: share stream
// mode: 0: decrease; 1:increase
// lossrate: 当前丢包率（根据实际情况可能是全程或者半程丢包率）
static int adjustVideoResolution(trtp_manager_t* self, int video_id, int mode, int lossRate) {
    // sanity check
    if (!self) {
        TSK_DEBUG_ERROR("adjustVideoResolution input invalid");
        return -1;
    }

    if (0 == video_id) {
        if (TRTP_VIDEO_ADJUST_MODE_DECREASE == mode) {
            // decrease main
            self->iLastMainVideoSize = 50;
        } else {
            // increase main
            self->iLastMainVideoSize = 100;
        }
    } else {
        TSK_DEBUG_ERROR("adjustVideoResolution not support minor video stream");
    }

    return 0;
}

#define TRTP_RTCP_LOSS_RESUME_TIME_MS       3000

//static int trtp_video_send_status = TRTP_VIDEO_STEAM_NORMAL;
void lossRateProcessForTest(const void* arg,long timerid)
{
    unsigned width = 0, height = 0;
    tmedia_defaults_get_video_size_child(&width, &height);
    int minor_enable = (width*height) > 0 ? 1: 0;
    
    trtp_manager_t *self = (trtp_manager_t*)arg;
    int half_lossRate = Config_GetInt("video_lossRate", 0 );
    int full_lossrate = Config_GetInt("video_up_lossRate", 0);
    int share_stream_bitrate = 100;

    // camera大小流不发送时认为半程丢包为0
    if (!trtp_rtcp_session_get_video_sendstat(self->rtcp.session)) {
        half_lossRate = 0;
    }

    static int iMinAdjustBitratePercent = 50;
    static int iMinAdjustFrameratePercent = 60;

    // for different up lossrate set min bitrate and min framerate
    int max_up_lossrate = full_lossrate > half_lossRate ? full_lossrate : half_lossRate;
    if (max_up_lossrate >= 50) {
        iMinAdjustBitratePercent = 30;
        iMinAdjustFrameratePercent = 50;
    } else if (max_up_lossrate >= 40) {
        iMinAdjustBitratePercent = 40;
        iMinAdjustFrameratePercent = 60;
    } else if (max_up_lossrate >= 30) {
        iMinAdjustBitratePercent = 55;
        iMinAdjustFrameratePercent = 70; 
    } else if (max_up_lossrate >= 20) {
        iMinAdjustBitratePercent = 60;
        iMinAdjustFrameratePercent = 80; 
    } else if (max_up_lossrate > 10) {
        iMinAdjustBitratePercent = 70;
        iMinAdjustFrameratePercent = 70; 
    } else {
        iMinAdjustBitratePercent = 80;
        iMinAdjustFrameratePercent = 80;  
    }

    if (max_up_lossrate >= 20) {
        self->iHigherLossrateCount++;
    } else {
        self->iHigherLossrateCount = 0;
    }

    // 半程丢包和全程丢包都正常的话 每5s记录一次
    if (!half_lossRate && !full_lossrate) {
        TMEDIA_I_AM_ACTIVE(5, "lossRateProcessForTest-loss half-loss[%d] full-loss[%d]", half_lossRate, full_lossrate);
    } else {
        TSK_DEBUG_INFO("lossRateProcessForTest half-loss[%d] full-loss[%d]", half_lossRate, full_lossrate);
    }
    

    uint64_t curTimeMs = tsk_gettimeofday_ms();

    // half_lossRate = (half_lossRate > 5) ? half_lossRate : 0;
    // full_lossrate = (full_lossrate > 5) ? full_lossrate : 0;
    int mode = tmedia_defaults_get_video_net_adjustmode();

    // check stream status and (half-circle/full-circle) loss rate
    do {
        if (TRTP_VIDEO_STEAM_NORMAL == self->ivideoSendStatus) {
            if (minor_enable && !mode && self->iHigherLossrateCount >= 3) {
                self->ivideoSendStatus = TRTP_VIDEO_STEAM_MINOR;
                break;
            }
            
            TSK_DEBUG_INFO ("max up loss:%d, half:%d, full:%d" , max_up_lossrate, half_lossRate, full_lossrate);

            if (max_up_lossrate > 0 && self->iLastVideoBitrate <= iMinAdjustBitratePercent
                && self->iLastVideoFps <= iMinAdjustFrameratePercent) {
                break;
            }
            
            if (!full_lossrate) {
                if (!half_lossRate) {
                    if (self->iLastVideoBitrate < 100 || self->iLastVideoFps < 100) {
                        if (curTimeMs - self->lastAdjustTimeMs < TRTP_RTCP_LOSS_RESUME_TIME_MS) {
                            // TSK_DEBUG_INFO ("lossRateProcessForTest (main&minor) wait another tick to increase minor bitRate");
                            break;
                        } else {
                            adjustVideofps(self, TRTP_VIDEO_STREAM_TYPE_MAIN, TRTP_VIDEO_ADJUST_MODE_INCREASE, 0);
                            adjustVideoBitrate(self, TRTP_VIDEO_STREAM_TYPE_MAIN, TRTP_VIDEO_ADJUST_MODE_INCREASE, 0);
                            // adjustVideoResolution(self, TRTP_VIDEO_STREAM_TYPE_MAIN, TRTP_VIDEO_ADJUST_MODE_INCREASE, 0);
                            
                            TSK_DEBUG_INFO ("resume bitrate:%d, fps:%d" , self->iLastVideoBitrate, self->iLastVideoFps);
                        }
                    } else {
                        // TSK_DEBUG_INFO ("lossRateProcessForTest (main&minor) network is good");
                        break;
                    }
                } else {
                    // do nothing
                }
            } else {
                if (!half_lossRate) {
                    // reduce main stream net resolution
                    if (!minor_enable 
                        && iMinAdjustBitratePercent == self->iLastVideoBitrate 
                        && iMinAdjustFrameratePercent == self->iLastVideoFps) {
                        //adjustVideoResolution(self, TRTP_VIDEO_STREAM_TYPE_MAIN, TRTP_VIDEO_ADJUST_MODE_DECREASE, full_lossrate);
                    }

                    // reduce main stream bitrate
                    if (self->iLastVideoBitrate > iMinAdjustBitratePercent) {
                        adjustVideoBitrate(self, TRTP_VIDEO_STREAM_TYPE_MAIN, TRTP_VIDEO_ADJUST_MODE_DECREASE, full_lossrate);
                        // 码率下限
                        self->iLastVideoBitrate = (self->iLastVideoBitrate < iMinAdjustBitratePercent) ? iMinAdjustBitratePercent : self->iLastVideoBitrate;
                    }
                
                    // reduce main stream fps
                    if (self->iLastVideoFps > iMinAdjustFrameratePercent
                        && self->iLastVideoBitrate <= iMinAdjustBitratePercent) {
                        adjustVideofps(self, TRTP_VIDEO_STREAM_TYPE_MAIN, TRTP_VIDEO_ADJUST_MODE_DECREASE, full_lossrate);
                        
                        // 帧率下限
                        self->iLastVideoFps = (self->iLastVideoFps < iMinAdjustFrameratePercent) ? iMinAdjustFrameratePercent : self->iLastVideoFps;
                    }
                } else {
                    // close main video stream when
                    // 1. minor video enable
                    // 2. network adjustmode is not auto
                    // 3. lossrate for 3 count
                    if (minor_enable && !mode && self->iHigherLossrateCount >= 3) {
                        self->ivideoSendStatus = TRTP_VIDEO_STEAM_MINOR;
                        // TSK_DEBUG_INFO ("lossRateProcessForTest (minor) switch to minor stream and decrease minor bitrate");
                        
                        // reduce minor stream fps
                        if (iMinAdjustBitratePercent == self->iLastVideoBitrate) {
                            adjustVideofps(self, TRTP_VIDEO_STREAM_TYPE_MINOR, TRTP_VIDEO_ADJUST_MODE_DECREASE, full_lossrate);
                        }

                        // reduce main stream bitrate
                        adjustVideoBitrate(self, TRTP_VIDEO_STREAM_TYPE_MINOR, TRTP_VIDEO_ADJUST_MODE_DECREASE, full_lossrate);

                    } else {
                        // reduce main stream net resolution
                        if (!minor_enable 
                            && iMinAdjustBitratePercent == self->iLastVideoBitrate 
                            && iMinAdjustFrameratePercent == self->iLastVideoFps) {
                            // adjustVideoResolution(self, TRTP_VIDEO_STREAM_TYPE_MAIN, TRTP_VIDEO_ADJUST_MODE_DECREASE, full_lossrate);
                        }
                        //优先降低码率
                        // reduce main stream bitrate
                        if (self->iLastVideoBitrate > iMinAdjustBitratePercent) {
                            adjustVideoBitrate(self, TRTP_VIDEO_STREAM_TYPE_MAIN, TRTP_VIDEO_ADJUST_MODE_DECREASE, full_lossrate);
                            
                            // 下限
                            self->iLastVideoBitrate = (self->iLastVideoBitrate < iMinAdjustBitratePercent) ? iMinAdjustBitratePercent : self->iLastVideoBitrate;
                        }
                        
                        // reduce main stream fps
                        if (self->iLastVideoFps > iMinAdjustFrameratePercent
                            && self->iLastVideoBitrate <= iMinAdjustBitratePercent) {
                            adjustVideofps(self, TRTP_VIDEO_STREAM_TYPE_MAIN, TRTP_VIDEO_ADJUST_MODE_DECREASE, full_lossrate);
                            
                            // 帧率下限
                            self->iLastVideoFps = (self->iLastVideoFps < iMinAdjustFrameratePercent) ? iMinAdjustFrameratePercent : self->iLastVideoFps;
                        }
                    }
                }
            }
        } else if (TRTP_VIDEO_STEAM_MINOR == self->ivideoSendStatus) {
            if (max_up_lossrate > 0 && self->iLastMinorVideoBitrate <= iMinAdjustBitratePercent
                && self->iLastMinorVideoFps <= iMinAdjustFrameratePercent) {
                
                break;
            }
            
            if (!full_lossrate && !half_lossRate) {
                if (self->iLastMinorVideoBitrate  < 100 || self->iLastMinorVideoFps < 100) {
                    if (curTimeMs - self->lastAdjustTimeMs < TRTP_RTCP_LOSS_RESUME_TIME_MS) {
                        // TSK_DEBUG_INFO ("lossRateProcessForTest (minor) wait another tick to increase bitRate");
                        break;
                    } else {
                        // resume bitrate and framerate
                        adjustVideofps(self, TRTP_VIDEO_STREAM_TYPE_MINOR, TRTP_VIDEO_ADJUST_MODE_INCREASE, 0);
                        adjustVideoBitrate(self, TRTP_VIDEO_STREAM_TYPE_MINOR, TRTP_VIDEO_ADJUST_MODE_INCREASE, 0);
                    }
                } else {
                    if (curTimeMs - self->lastAdjustTimeMs < TRTP_RTCP_LOSS_RESUME_TIME_MS) {
                        // TSK_DEBUG_INFO ("lossRateProcessForTest (minor) wait another tick to switch main stream");
                        break;
                    } else {
                        if (!mode && !self->iForceSendStatus) {
                            self->ivideoSendStatus = TRTP_VIDEO_STEAM_NORMAL;
                        }
                    }
                }
            } else {
                // 优先降低码率，再降低帧率，保证流畅性
                // reduce minor stream fps
                if (self->iLastMinorVideoFps >= iMinAdjustFrameratePercent
                    && self->iLastMinorVideoBitrate <= iMinAdjustBitratePercent) {
                    adjustVideofps(self, TRTP_VIDEO_STREAM_TYPE_MINOR, TRTP_VIDEO_ADJUST_MODE_DECREASE, full_lossrate);
                }

                // reduce minor stream bitrate
                if (self->iLastMinorVideoBitrate >= iMinAdjustBitratePercent) {
                    adjustVideoBitrate(self, TRTP_VIDEO_STREAM_TYPE_MINOR, TRTP_VIDEO_ADJUST_MODE_DECREASE, full_lossrate);
                }
            }
        } else {
            TSK_DEBUG_WARN ("lossRateProcessForTest invalid status[%d]", self->ivideoSendStatus);
            break;
        }
    
        // check main/minor video stream adjust percennt
        self->iLastVideoBitrate = (self->iLastVideoBitrate > 100) ? 100 : self->iLastVideoBitrate;
//        self->iLastVideoBitrate = (self->iLastVideoBitrate < iMinAdjustBitratePercent) ? iMinAdjustBitratePercent : self->iLastVideoBitrate;

        self->iLastVideoFps = (self->iLastVideoFps > 100) ? 100 : self->iLastVideoFps;
//        self->iLastVideoFps = (self->iLastVideoFps < iMinAdjustFrameratePercent) ? iMinAdjustFrameratePercent : self->iLastVideoFps;

        self->iLastMinorVideoBitrate = (self->iLastMinorVideoBitrate > 100) ? 100 : self->iLastMinorVideoBitrate;
//        self->iLastMinorVideoBitrate = (self->iLastMinorVideoBitrate < iMinAdjustBitratePercent) ? iMinAdjustBitratePercent : self->iLastMinorVideoBitrate;

        self->iLastMinorVideoFps = (self->iLastMinorVideoFps > 100) ? 100 : self->iLastMinorVideoFps;
//        self->iLastMinorVideoFps = (self->iLastMinorVideoFps < iMinAdjustFrameratePercent) ? iMinAdjustFrameratePercent : self->iLastMinorVideoFps;

        self->lastAdjustTimeMs = curTimeMs;

        // adjust video bitrate
        Config_SetInt("video_bitrate_level", self->iLastVideoBitrate);
        Config_SetInt("video_bitrate_level_second", self->iLastMinorVideoBitrate);

        Config_SetInt("video_fps_level", self->iLastVideoFps);
        Config_SetInt("video_fps_level_second", self->iLastMinorVideoFps);

        // for share video stream
        if (TRTP_VIDEO_STEAM_NORMAL == self->ivideoSendStatus) {
            Config_SetInt("video_bitrate_share", self->iLastVideoBitrate);
            share_stream_bitrate = self->iLastVideoBitrate;
        } else {
            Config_SetInt("video_bitrate_share", self->iLastMinorVideoBitrate);
            share_stream_bitrate = self->iLastMinorVideoBitrate;
        }

        Config_SetInt("video_size_level", self->iLastMainVideoSize);

        TSK_DEBUG_INFO ("lossRateProcessForTest status:%d, main:%d, fps:%d, size:%d, minor:%d, fps:%d, share:%d"
            , self->ivideoSendStatus, self->iLastVideoBitrate, self->iLastVideoFps, self->iLastMainVideoSize
            , self->iLastMinorVideoBitrate, self->iLastMinorVideoFps, share_stream_bitrate);

        // send query media ctrol request(report req)
        // send_media_ctl_req(self, 1);

        // send qos parameter to caller
        if (self->video_encode_adjust_cb.fun) {
            unsigned videoWidth = 0, videoHeight = 0;
            tmedia_defaults_get_video_size(&videoWidth, &videoHeight);
            videoWidth = videoWidth * self->iLastMainVideoSize / 100;
            videoHeight = videoHeight * self->iLastMainVideoSize / 100;

            int videoBitrate = tmedia_defaults_size_to_bitrate(videoWidth, videoHeight);
            videoBitrate = videoBitrate * self->iLastVideoBitrate / 100;
            
            int videoFps = tmedia_defaults_get_video_fps();
            videoFps = trtp_max(videoFps * (self->iLastVideoFps / 100.0f), 10);
            self->video_encode_adjust_cb.fun(self->video_encode_adjust_cb.usrdata, videoWidth, videoHeight, videoBitrate, videoFps);
        }

    } while(0);

    //Config_SetInt("video_up_lossRate", 0);
}

// 利用上行丢包率做带宽预测，目前仅使用全程丢包率
// currRate:当前码率（单位:kbps）
// 当loss 大于10%时, 下调码率: newRate = currRate * (1 - 0.5*lossRate);
// 当loss 在2%～10%之间，不做调整
// 当loss 小于2%时，上调码率: newRate = currRate * 1.08 + 0.5
void lossRateProcessForBwe(const void* arg,long timerid)
{
    unsigned width = 0, height = 0;
    tmedia_defaults_get_video_size_child(&width, &height);
    int minor_enable = (width*height) > 0 ? 1: 0;
    
    trtp_manager_t *self = (trtp_manager_t*)arg;
    int half_lossRate = Config_GetInt("video_lossRate", 0 );
    int full_lossrate = Config_GetInt("video_up_lossRate", 0);
    int data_rtt = Config_GetInt("data_rtt", 0);
    int curr_deltaRtt = data_rtt - self->iLastDataRtt;

    static int iMinAdjustBitratePercent = 50;
    static int iMinAdjustFrameratePercent = 60;

//    int max_up_lossrate = full_lossrate > half_lossRate ? full_lossrate : half_lossRate;
    int max_up_lossrate = full_lossrate;
    if (max_up_lossrate >= 50) {
        iMinAdjustBitratePercent = 30;
        iMinAdjustFrameratePercent = 50;
    } else if (max_up_lossrate >= 40) {
        iMinAdjustBitratePercent = 40;
        iMinAdjustFrameratePercent = 60;
    } else if (max_up_lossrate >= 30) {
        iMinAdjustBitratePercent = 55;
        iMinAdjustFrameratePercent = 70; 
    } else if (max_up_lossrate >= 20) {
        iMinAdjustBitratePercent = 60;
        iMinAdjustFrameratePercent = 80; 
    } else if (max_up_lossrate > 10) {
        iMinAdjustBitratePercent = 70;
        iMinAdjustFrameratePercent = 70; 
    } else if (max_up_lossrate > 0) {
        iMinAdjustBitratePercent = 80;
        iMinAdjustFrameratePercent = 80;  
    } else {
        iMinAdjustBitratePercent = 100;
        iMinAdjustFrameratePercent = 100;  
    }

    // 半程丢包和全程丢包都正常的话 每10s记录一次, 否则异常丢包率都会打印
    if (!half_lossRate && !full_lossrate) {
        TMEDIA_I_AM_ACTIVE(10, "lossRateProcessForBwe half-loss[%d] full-loss[%d]", half_lossRate, full_lossrate);
    } else {
        TSK_DEBUG_INFO("lossRateProcessForBwe half-loss[%d] full-loss[%d]", half_lossRate, full_lossrate);
    }

    if (max_up_lossrate >= 20) {
        self->iHigherLossrateCount++;
    } else {
        self->iHigherLossrateCount = 0;
    }
    
    if (max_up_lossrate >= 70) {
        self->iPeakLossCount++;
    } else {
        self->iPeakLossCount = 0;
    }
    
    // 突发丢包，连续三次以上才处理
    if (max_up_lossrate >= 70 && self->iPeakLossCount < 3) {
        return;
    }

    uint64_t curTimeMs = tsk_gettimeofday_ms();

    // 大小流是否自动调整
    int mode = tmedia_defaults_get_video_net_adjustmode();
    int max_bitrate_level = 100;
    int min_bitrate_level = 50;

    if (self->p2p_route_enable) {
        max_bitrate_level = tmedia_get_max_video_bitrate_level_for_p2p();
        min_bitrate_level = 70;

        // 调整video GOP, P2P通路下GOP调整为2
        Config_SetInt("video_encode_gop_size", 2);
    } else {
        max_bitrate_level = tmedia_get_max_video_bitrate_level_for_sfu();
        min_bitrate_level = tmedia_get_min_video_bitrate_level_for_sfu();

        // 合法性保护，下限优先
        if (max_bitrate_level < min_bitrate_level) {
            max_bitrate_level = min_bitrate_level;
        }
        Config_SetInt("video_encode_gop_size", 1);
    }

    do {
        if (TRTP_VIDEO_STEAM_NORMAL == self->ivideoSendStatus) {
            /*
            if (minor_enable && !mode && self->iHigherLossrateCount >= 3) {
                self->ivideoSendStatus = TRTP_VIDEO_STEAM_MINOR;
                break;
            }
             */
            
            // TSK_DEBUG_INFO ("max up loss:%d, half:%d, full:%d" , max_up_lossrate, half_lossRate, full_lossrate);

            if (max_up_lossrate > 0 && self->iLastVideoBitrate <= iMinAdjustBitratePercent
                && self->iLastVideoFps <= iMinAdjustFrameratePercent) {
                break;
            }
            
            // reduce main stream bitrate
            if (max_up_lossrate <= 2) {
                if (curTimeMs - self->lastAdjustTimeMs < TRTP_RTCP_LOSS_RESUME_TIME_MS) {
                    break;
                }

                // 当前delartt延时比上一个delta延时增加，不上调码率
                if (curr_deltaRtt > 0 || self->iLastDeltaRtt > 0) {
                    break;
                }

                // increase bitrate
                self->iLastVideoBitrate *= 1.08;

                // resume video fps
                if (self->iLastVideoFps < 100) {
                    adjustVideofps(self, TRTP_VIDEO_STREAM_TYPE_MAIN, TRTP_VIDEO_ADJUST_MODE_INCREASE, 0);
                }
            } else if (max_up_lossrate <= 8) {
                // do nothing
            } else {
                self->iLastVideoBitrate *= (1.0 - 0.5 * max_up_lossrate / 100.0);
            }

            // reduce main stream fps
            if (self->iLastVideoFps > iMinAdjustFrameratePercent
                && self->iLastVideoBitrate <= iMinAdjustBitratePercent) {
                adjustVideofps(self, TRTP_VIDEO_STREAM_TYPE_MAIN, TRTP_VIDEO_ADJUST_MODE_DECREASE, full_lossrate);
                
                // 帧率下限
                self->iLastVideoFps = (self->iLastVideoFps < iMinAdjustFrameratePercent) ? iMinAdjustFrameratePercent : self->iLastVideoFps;
            }
        } else if (TRTP_VIDEO_STEAM_MINOR == self->ivideoSendStatus) {
            if (max_up_lossrate > 0 && self->iLastMinorVideoBitrate <= iMinAdjustBitratePercent
                && self->iLastMinorVideoFps <= iMinAdjustFrameratePercent) {
                
                break;
            }

            // reduce main stream bitrate
            if (max_up_lossrate <= 2) {
                self->iLastMinorVideoBitrate *= 1.08;
            } else if (max_up_lossrate <= 8) {
                // do nothing
            } else {
                self->iLastMinorVideoBitrate *= (1.0 - 0.5 * max_up_lossrate);
            }

            // reduce minor stream fps
            if (self->iLastMinorVideoFps >= iMinAdjustFrameratePercent
                && self->iLastMinorVideoBitrate <= iMinAdjustBitratePercent) {
                adjustVideofps(self, TRTP_VIDEO_STREAM_TYPE_MINOR, TRTP_VIDEO_ADJUST_MODE_DECREASE, full_lossrate);
                
                // 帧率下限
                self->iLastMinorVideoFps = (self->iLastMinorVideoFps < iMinAdjustFrameratePercent) ? iMinAdjustFrameratePercent : self->iLastMinorVideoFps;
            }
        } else {
            TSK_DEBUG_WARN ("lossRateProcessForBwe invalid status[%d]", self->ivideoSendStatus);
            break;
        }

//        if(max_up_lossrate >= 30){
//            // do nothing
//            // self->iLastVideoBitrate /= 2.0;
//        }else if(max_up_lossrate >= 25){
//            self->iLastVideoBitrate /= 2.1;
//        }else if(max_up_lossrate >= 20){
//            self->iLastVideoBitrate /= 2.0;
//        }else if(max_up_lossrate >= 15){
//            self->iLastVideoBitrate /= 1.9;
//        }else if(max_up_lossrate >= 8){
//            self->iLastVideoBitrate /= 1.7;
//        }else if(max_up_lossrate >= 2){
//            self->iLastVideoBitrate /= 1.4;
//        }else if (max_up_lossrate > 0) {
//            // do nothing
//            self->iLastVideoBitrate /= 1.1;
//        }

        // check main/minor video stream adjust percennt
        self->iLastVideoBitrate = (self->iLastVideoBitrate > max_bitrate_level) ? max_bitrate_level : self->iLastVideoBitrate;
        self->iLastVideoBitrate = (self->iLastVideoBitrate < min_bitrate_level) ? min_bitrate_level : self->iLastVideoBitrate;

        self->iLastVideoFps = (self->iLastVideoFps > 100) ? 100 : self->iLastVideoFps;
//        self->iLastVideoFps = (self->iLastVideoFps < iMinAdjustFrameratePercent) ? iMinAdjustFrameratePercent : self->iLastVideoFps;

        self->iLastMinorVideoBitrate = (self->iLastMinorVideoBitrate > max_bitrate_level) ? max_bitrate_level : self->iLastMinorVideoBitrate;
        self->iLastMinorVideoBitrate = (self->iLastMinorVideoBitrate < min_bitrate_level) ? min_bitrate_level : self->iLastMinorVideoBitrate;

        self->iLastMinorVideoFps = (self->iLastMinorVideoFps > 100) ? 100 : self->iLastMinorVideoFps;
//        self->iLastMinorVideoFps = (self->iLastMinorVideoFps < iMinAdjustFrameratePercent) ? iMinAdjustFrameratePercent : self->iLastMinorVideoFps;

        self->lastAdjustTimeMs = curTimeMs;

        // adjust video bitrate
        Config_SetInt("video_bitrate_level", self->iLastVideoBitrate);
        Config_SetInt("video_bitrate_level_second", self->iLastMinorVideoBitrate);

        // 1. server转发场景下
        // 2. p2p 场景下，丢包率小于50%
        if ((!self->p2p_route_enable) || (self->p2p_route_enable && max_up_lossrate <= 50)) {
            Config_SetInt("video_bitrate_share", self->iLastVideoBitrate);
        }
        

        Config_SetInt("video_fps_level", self->iLastVideoFps);
        Config_SetInt("video_fps_level_second", self->iLastMinorVideoFps);

        Config_SetInt("video_size_level", self->iLastMainVideoSize);

        TSK_DEBUG_INFO ("lossRateProcessForBwe status:%d, main:%d, fps:%d, size:%d, minor:%d, fps:%d"
            , self->ivideoSendStatus, self->iLastVideoBitrate, self->iLastVideoFps, self->iLastMainVideoSize
            , self->iLastMinorVideoBitrate, self->iLastMinorVideoFps);

        // send query media ctrol request(report req)
        // send_media_ctl_req(self, 1);

        // send qos parameter to caller
        if (self->video_encode_adjust_cb.fun) {
            unsigned videoWidth = 0, videoHeight = 0;
            tmedia_defaults_get_video_size(&videoWidth, &videoHeight);
            videoWidth = videoWidth * self->iLastMainVideoSize / 100;
            videoHeight = videoHeight * self->iLastMainVideoSize / 100;

            int videoBitrate = tmedia_defaults_size_to_bitrate(videoWidth, videoHeight);
            videoBitrate = videoBitrate * self->iLastVideoBitrate / 100;
            
            int videoFps = tmedia_defaults_get_video_fps();
            videoFps = trtp_max(videoFps * (self->iLastVideoFps / 100.0f), 10);
            self->video_encode_adjust_cb.fun(self->video_encode_adjust_cb.usrdata, videoWidth, videoHeight, videoBitrate, videoFps);
        }
    } while (0);
    
    self->iLastDataRtt = data_rtt;
    self->iLastDeltaRtt = curr_deltaRtt;
}

void lossRateProcess( trtp_manager_t *self, int lossRate)
{
    int iMaxBitRate =0;
    int iMinBitRate=0;
    int iMinBitRatePercent = self->iMinBitRatePercent; //视频默认情况下最小50百分比
    static int iMinFrameRatePercent = 50; //视频默认情况下最小50百分比
    
    /*
    if( lossRate !=0 && TSK_ABS(lossRate - Config_GetInt("video_lossRate", 0)) < 3 )
    {
        if(self->iLastNoLossBitrate != 100){
            self->iLastVideoBitrate = self->iLastNoLossBitrate - 5;
            Config_SetInt("video_bitrate_level", self->iLastVideoBitrate );
            TSK_DEBUG_INFO ("loss rate revert:%d",self->iLastVideoBitrate);
        }
        self->iStaticCount = 0;
        self->packet_total = 0;
        self->packet_loss  = 0;
        if(self->bUpBitrate == tsk_true)
        {
            self->iUpBitrateLevel = 2;
        }
        return;
    }
     */
    iMaxBitRate =Config_GetUInt("max_bitrate",0);
    iMinBitRate =Config_GetUInt("min_bitrate",0);
    if ((iMaxBitRate !=0) && (iMinBitRate !=0))
    {
        iMinBitRatePercent = iMinBitRate * 100 / iMaxBitRate;
        self->iMinBitRatePercent = iMinBitRatePercent;
    }else if(lossRate >= 28){
        iMinBitRatePercent = 40;
        iMinFrameRatePercent = 40;
        self->iMinBitRatePercent = iMinBitRatePercent;
    }
    
    if(lossRate == 0){
        //记录最近一次不丢包的码率设置
        self->iLastNoLossBitrate = self->iLastVideoBitrate;
    }
    if(lossRate >5 && self->bUpBitrate == tsk_true){
        //如果提高码率后出现丢包，那么减慢码率提升速度
        self->iUpBitrateLevel = 2;
    }
    Config_SetInt("video_lossRate", lossRate );
    if(lossRate > 2) TSK_DEBUG_INFO ("loss rate %d",lossRate);
    if (lossRate >= 28) {
        //直接降50的码率
        if (self->iLastVideoBitrate ==iMinBitRatePercent) {
            //码率到头了降帧率
            self->iLastVideoFrame-=20;
        }
        else
        {
            self->iLastVideoBitrate-=50;
        }
    }else if (lossRate >= 20) {
        //直接降50的码率
        if (self->iLastVideoBitrate ==iMinBitRatePercent) {
            //码率到头了降帧率
            self->iLastVideoFrame-=15;
            
        }
        else
        {
            self->iLastVideoBitrate-=25;
        }
    }
    else if(lossRate >=10)
    {
        if (self->iLastVideoBitrate ==iMinBitRatePercent) {
            //码率到头了降帧率
            self->iLastVideoFrame-=10;
            
        }
        else
        {
            self->iLastVideoBitrate-=10;
        }
    }
    else if(lossRate >5)
    {
        if (self->iLastVideoBitrate ==iMinBitRatePercent) {
            //码率到头了降帧率
            self->iLastVideoFrame-=5;
            
        }
        else
        {
            self->iLastVideoBitrate-=5;
        }
    }
    else if(lossRate == 0)
    {
        if(self->iUpBitrateStaticCount >= 5){
            self->iUpBitrateStaticCount = 0;
            if (self->iLastVideoBitrate == 100) {
                self->iLastVideoFrame += 10;
            }
            else
            {
                self->iLastVideoBitrate += self->iUpBitrateLevel;
                self->bUpBitrate = (self->iLastVideoBitrate < 100);
                if(self->bUpBitrate == tsk_false){
                    self->iUpBitrateLevel = 5;
                }
                TSK_DEBUG_INFO ("loss rate up: %d",self->iLastVideoBitrate);
            }
        }else{
            self->iUpBitrateStaticCount++;
        }
    }
    
    if (self->iLastVideoBitrate <iMinBitRatePercent) {
        self->iLastVideoBitrate=iMinBitRatePercent;
    }
    if (self->iLastVideoBitrate >100) {
        self->iLastVideoBitrate=100;
    }
    
    if (self->iLastVideoFrame <iMinFrameRatePercent) {
        self->iLastVideoFrame=iMinFrameRatePercent;
    }
    if (self->iLastVideoFrame >100) {
        self->iLastVideoFrame=100;
    }
    
    // TSK_DEBUG_INFO("JOE_Log 丢包率:%d total:%d 设置码率:%d 帧率:%d",lossRate,self->packet_total,self->iLastVideoBitrate,self->iLastVideoFrame);
    Config_SetInt("video_bitrate_level", self->iLastVideoBitrate );
    //Config_SetInt("video_fps_level", self->iLastVideoFrame);
    self->iStaticCount = 0;
    self->packet_total = 0;
    self->packet_loss  = 0;
}

int onMediaCtlTimer(const void* arg,long timerid)
{
    static long long lossRateCheckCount = 0;
    trtp_manager_t *self = (trtp_manager_t*)arg;
    // send_media_ctl_req( self ,  1 );
    
    // 发送的包太少就别检测了，怕不准误报
    // 检测间隔调整为5s，快速反应网络状况
    if( self->isSendDataToServer > 10 ){
        if( self->isSendDataReceived == 1 && self->iLastRtpPass == 0 )
        {
            SendNotifyEvent( "media_road_pass",  0 );
            self->iLastRtpPass = 1 ;
            
        }
        else if( self->isSendDataReceived == 0 && self->iLastRtpPass == 1 ){
            SendNotifyEvent( "media_road_block",  0 );
            self->iLastRtpPass = 0 ;
        }
        if(self->isSendDataReceived == 0 && ((++lossRateCheckCount) % 3 == 0 )){
            send_media_ctl_req( self ,  1 );
            TSK_DEBUG_WARN ("not received reply,maybe lossrate");
            //lossRateProcess(self, 28);
            // lossRateProcessForTest(self, 30);
            //Config_SetInt("video_lossRate", 30 );
        }
    }
    
    self->isSendDataToServer = 0;
    self->isSendDataReceived = 0;
    return 0;
}


void onRecvMediaCtlData( trtp_manager_t *self, char* buff, int len )
{
    struct MediaCtlRsp rspData;
    int cmd = 0;
    int ret = GetMediaCtlRspFromBuffer( buff , len , &rspData , &cmd);
    if( ret == 0 )
    {
        TMEDIA_I_AM_ACTIVE(5, "onRecvMediaCtlData receive cmd[%d] type[%d], total[%d], max[%d], min[%d]"
            , cmd, rspData.media_type, rspData.total_recv_num, rspData.max_seq, rspData.min_seq);

        // 仅针对视频统计
        if( MEDIA_TYPE_VIDEO == rspData.media_type && rspData.total_recv_num != 0 )
        {
            self->isSendDataReceived = 1;
        }
        
        int total = rspData.max_seq - rspData.min_seq + 1 ;
        //计算是否发生序号翻转
        if( total < rspData.total_recv_num )
        {
            return ;
        }
        
        if (total == 0) {
            return;
        }
        int lost = total - rspData.total_recv_num;
       // TSK_DEBUG_INFO("收到服务器回应:%d %d max:%d min:%d",total,lost,rspData.max_seq,rspData.min_seq);
        
        if( MEDIA_TYPE_VIDEO == rspData.media_type )
        {
            // video loss stat
            self->iStaticCount++;
            self->packet_total = total;

            self->packet_loss = lost;
            
            int lossRate = 0;
            if (self->packet_loss > 0 && self->packet_total > 0) {
                lossRate  = self->packet_loss * 100 / self->packet_total;
                lossRate = lossRate > 100 ? 100 : lossRate;
            }
            
            Config_SetInt("video_lossRate", lossRate );

            // 现在采用定时器触发丢包率检查
            // if (self->iStaticCount >= 4) {   
            //     //lossRateProcess(self, lossRate); 
            //     // lossRateProcessForTest(self, lossRate);            
            // }
            
            AddSelfVideoPacket( lost,total, self->sessionid );

            self->report_stat.video_send_total_count += total;
            self->report_stat.video_send_loss_count += lost;
        }
        else{
            // audio loss stat
            AddSelfAudioPacket( lost, total, self->sessionid );

            self->report_stat.audio_send_total_count += total;
            self->report_stat.audio_send_loss_count += lost;
        }
    }
    else
    {
        TSK_DEBUG_INFO ("onRecvMediaCtlData receive cmd[%d] ", cmd);
    }
}

// p2p传输相关接口
int onP2PRouteCheckTimer(const void* arg,long timerid)
{
    trtp_manager_t *self = (trtp_manager_t*)arg;
    // send_media_ctl_req( self ,  1 );
    sendP2pRouteCheckReq( self );
    tsk_bool_t p2p_loss_flag = tsk_false;

    uint64_t curTimeMs = tsk_gettimeofday_ms();
    if (curTimeMs - self->p2p_connect_info.p2p_last_recv_pong_time >= 1000
        || curTimeMs - self->p2p_connect_info.p2p_last_recv_ping_time >= 1000) {
        // self->p2p_route_enable = tsk_false;
        p2p_loss_flag = tsk_false;
    } else {
        // self->p2p_route_enable = tsk_true;
        p2p_loss_flag = tsk_true;
    }
    
    if (curTimeMs - self->p2p_route_check_startms >= 2000 
        || (curTimeMs - self->p2p_route_check_startms < 2000 && tsk_true == p2p_loss_flag)) {
        self->p2p_route_check_end = tsk_true;
    }

    if (tsk_false == p2p_loss_flag) {
        if (self->p2p_connect_info.p2p_route_check_loss >= 8) {
            self->p2p_route_enable = tsk_false;

            // p2p 检测失败，此时有两种情况：开始检测和传输过程中，需要分开处理
            trtp_manager_stop_route_check_timer(self);
            if (TRTP_TRANS_ROUTE_TYPE_UNKNOE == self->curr_trans_route) {
                // p2p通路检测失败，走server转发，上报事件
                
                if (self->trtp_route_cb.fun) {
                    TSK_DEBUG_INFO("onP2PRouteCheckTimer p2p fail:2");
                    self->trtp_route_cb.fun(self->trtp_route_cb.usrdata, TRTP_ROUTE_EVENT_TYPE_P2P_FAIL, NULL, 0);
                }
            } else {
                // p2p传输过程中ping失败，切换到server转发
                if (self->trtp_route_cb.fun) {
                    TSK_DEBUG_INFO("onP2PRouteCheckTimer p2p change:3");
                    self->trtp_route_cb.fun(self->trtp_route_cb.usrdata, TRTP_ROUTE_EVENT_TYPE_P2P_CHANGE, NULL, 0);
                }
            }
            self->curr_trans_route = TRTP_TRANS_ROUTE_TYPE_SERVER;
        }
    } else {
        self->p2p_route_enable = tsk_true;
        // P2P 检测成功，走p2p转发，上报事件
        if (self->trtp_route_cb.fun && self->curr_trans_route != TRTP_TRANS_ROUTE_TYPE_P2P) {
            TSK_DEBUG_INFO("onP2PRouteCheckTimer p2p sucess:1");
            self->trtp_route_cb.fun(self->trtp_route_cb.usrdata, TRTP_ROUTE_EVENT_TYPE_P2P_SUCCESS, NULL, 0);
        }
        self->curr_trans_route = TRTP_TRANS_ROUTE_TYPE_P2P;
    }

    if (tsk_false == p2p_loss_flag) {
        self->p2p_connect_info.p2p_route_check_loss++;
    } else {
        self->p2p_connect_info.p2p_route_check_loss = 0;
    }

    TSK_DEBUG_INFO("p2p route check [%d] p2p trans[%d] loss[%u]", self->p2p_route_check_end, self->p2p_route_enable, self->p2p_connect_info.p2p_route_check_loss);
    // 通知rtcp
    trtp_rtcp_session_enable_p2p_transport(self->rtcp.session, self->p2p_route_enable);

    self->p2p_connect_info.p2p_last_recv_pong_time = 0;
    // self->p2p_connect_info.p2p_last_recv_ping_time = 0;
    return 0;
}

void sendP2pRouteCheckReq( trtp_manager_t *self )
{
    uint8_t buf[20];

    if (!self) {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return;
    }

    // Compose an RTP header with 2 CSRC (the session ID)
    memset(buf, 0, sizeof(buf));
    buf[0] = 0x02; // 1 CSRC

    buf[12] = (uint8_t)((self->sessionid >> 24) & 0xFF);
    buf[13] = (uint8_t)((self->sessionid >> 16) & 0xFF);
    buf[14] = (uint8_t)((self->sessionid >> 8) & 0xFF);
    buf[15] = (uint8_t)(self->sessionid & 0xFF);

    buf[16] = (uint8_t)'P';
    buf[17] = (uint8_t)'I';
    buf[18] = (uint8_t)'N';
    buf[19] = (uint8_t)'G';

    tsk_size_t size_sent = trtp_manager_send_rtp_raw (self, buf, sizeof(buf));
    if (size_sent != sizeof(buf)) {
        return;
    }
    
    self->p2p_connect_info.p2p_ping_seq++;
    TSK_DEBUG_INFO ("sent p2p ping resp, cnt:%u", self->p2p_connect_info.p2p_ping_seq);
    
    return;
}

void sendP2pRouteCheckResp( trtp_manager_t *self )
{
    uint8_t buf[20];

    if (!self) {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return;
    }

    // Compose an RTP header with 2 CSRC (the session ID)
    memset(buf, 0, sizeof(buf));
    buf[0] = 0x02; // 1 CSRC

    buf[12] = (uint8_t)((self->sessionid >> 24) & 0xFF);
    buf[13] = (uint8_t)((self->sessionid >> 16) & 0xFF);
    buf[14] = (uint8_t)((self->sessionid >> 8) & 0xFF);
    buf[15] = (uint8_t)(self->sessionid & 0xFF);

    buf[16] = (uint8_t)'P';
    buf[17] = (uint8_t)'O';
    buf[18] = (uint8_t)'N';
    buf[19] = (uint8_t)'G';

    tsk_size_t size_sent = trtp_manager_send_rtp_raw (self, buf, sizeof(buf));
    if (size_sent != sizeof(buf)) {
        TSK_DEBUG_ERROR ("Failed to send a p2p pong packet, size_sent:%lu", size_sent);
        return;
    }

    self->p2p_connect_info.p2p_pong_seq++;
    TSK_DEBUG_INFO ("sent p2p pong resp, cnt:%u", self->p2p_connect_info.p2p_pong_seq);
    
    return;
}

void trtp_manager_start_route_check_timer( trtp_manager_t *self )
{
    if ( !self || !self->p2p_connect_info.p2p_info_set_flag ) {
        TSK_DEBUG_INFO("@ trtp_manager_start_route_check_timer fail, not set p2p info");
        return;
    }
    
    TSK_DEBUG_INFO("@ trtp_manager_start_route_check_timer");
    sendP2pRouteCheckReq( self );
    
    if( self->p2p_connect_info.route_check_timerid != 0 )
    {
        xt_timer_mgr_global_cancel( self->p2p_connect_info.route_check_timerid );
        self->p2p_connect_info.route_check_timerid = 0;
    }
    self->p2p_route_check_startms = tsk_gettimeofday_ms();

    //如果是启动，要保证服务器收到这个包
    self->p2p_connect_info.route_check_timerid = xt_timer_mgr_global_schedule_loop( TRTP_P2P_ROUTE_CHECK_TIME, onP2PRouteCheckTimer, self );
}

void trtp_manager_stop_route_check_timer( trtp_manager_t *self )
{
    TSK_DEBUG_INFO("@ trtp_manager_stop_route_check_timer");
    if( !self || self->p2p_connect_info.route_check_timerid != 0 )
    {
        xt_timer_mgr_global_cancel( self->p2p_connect_info.route_check_timerid );
        self->p2p_connect_info.route_check_timerid = 0;

        self->p2p_connect_info.p2p_route_check_loss = 0;
        self->curr_trans_route = TRTP_TRANS_ROUTE_TYPE_UNKNOE;
    }
    
}

void trtp_manager_set_business_id(trtp_manager_t* self, uint64_t business_id)
{
    TSK_DEBUG_INFO ("set business id:%llu", business_id);
    self->uBusinessId = business_id;
}

int trtp_manager_get_report_stat(trtp_manager_t *self, trtp_send_report_stat_t* stat)
{
    if (!self || !stat) {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    stat->audio_send_total_count = self->report_stat.audio_send_total_count;
    stat->audio_send_loss_count = self->report_stat.audio_send_loss_count;
    stat->video_send_total_count = self->report_stat.video_send_total_count;
    stat->video_send_loss_count = self->report_stat.video_send_loss_count;

    // memset(&(self->report_stat), 0, sizeof(trtp_send_report_stat_t));
    return 0;
}

int trtp_manager_reset_report_stat(trtp_manager_t *self)
{
    if (!self) {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return -1;
    }

    memset(&(self->report_stat), 0, sizeof(trtp_send_report_stat_t));
    return 0;
}
