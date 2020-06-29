/*
* Copyright (C) 2010-2011 Mamadou Diop.
*
* Contact: Mamadou Diop <diopmamadou(at)doubango.org>
*
* This file is part of Open Source Doubango Framework.
*
* DOUBANGO is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* DOUBANGO is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with DOUBANGO.
*
*/

/**@file tdav_codec_h264.c
* @brief H.264 codec plugin using FFmpeg for decoding and x264 for encoding
* RTP payloader/depayloader follows RFC 3984
*
* @author Mamadou Diop <diopmamadou(at)doubango.org>
*

*/
#include "tinydav/codecs/h264/tdav_codec_h264.h"


#if defined(HAVE_FFMPEG) || defined(HAVE_H264_PASSTHROUGH) || defined(HARDWARE_CODECS)

#include "tinydav/codecs/h264/tdav_codec_h264_common.h"
#include "tinydav/codecs/h264/tdav_codec_h264_rtp.h"
#include "tinydav/video/tdav_converter_video.h"

#include "tinyrtp/rtp/trtp_rtp_packet.h"

#include "tinymedia/tmedia_params.h"
#include "tinymedia/tmedia_defaults.h"

#ifdef WIN32
//#pragma comment(lib, "DXGI.lib")
//#include <Windows.h>
//#include <DXGI.h>
#include "comutil.h"
#include "Wbemcli.h"
#include "Wbemidl.h"
#endif

#include "webrtc/modules/video_coding/codecs/h264/include/h264.h"

#include "tsk.h"
#include "tsk_params.h"
#include "tsk_memory.h"
#include "tsk_debug.h"
#include <memory>
#include <map>
#include <list>
#include <mutex>
#include <vector>
#include "version.h"

#include "../../../../baseWrapper/src/XConfigCWrapper.hpp"

#if FFMPEG_SUPPORT
#include "YMVideoRecorderManager.hpp"
#ifdef __cplusplus
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
#include "libx264/x264.h"
}
#endif
#endif

#include "tinydav/tdav_session_av.h"
#include "tinydav/video/tdav_session_video.h"
#include "tinymedia/tmedia_producer.h"
#include "tmedia_utils.h"

#if HAVE_FFMPEG
#   include <libavcodec/avcodec.h>
#   if (LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(51, 35, 0))
#       include <libavutil/opt.h>
#   endif
#endif


#define   tdav_max(A,B)  ((A)>(B)?(A):(B))

#define  tdav_codec_android_hardware_codec_size_low              0  
#define  tdav_codec_h264_idr_frame_interval_num                  20  // GOP

#define  tdav_codec_video_hw_profile_level                      "video_hw_profile_level"
#define  tdav_codec_video_bitrate_level                         "video_bitrate_level"
#define  tdav_codec_video_fps_level                             "video_fps_level"
#define  tdav_codec_video_resolution_level                      "video_resolution_level"

#define  tdav_codec_video_bitrate_second_level               "video_bitrate_level_second"        //"video_bitrate_second_level"
#define  tdav_codec_video_fps_second_level                   "video_fps_level_second"            //"video_fps_second_level"
#define  tdav_codec_video_resolution_second_level            "video_resolution_level"     //"video_resolution_second_level"

// for share video stream
#define  tdav_codec_video_bitrate_share_level               "video_bitrate_share"        
#define  tdav_codec_video_fps_share_level                   "video_fps_share"            
#define  tdav_codec_video_resolution_share_level            "video_resolution_share"     

#define  tdav_codec_video_encode_gop_size					"video_encode_gop_size"

#define tdav_codec_video_scale(A,B)             (B)!=100 ? (( (int)((A)*(B)/100.0f) << 4) >> 4) : (A)
#define tdav_codec_h264_fps_base                15
#define tdav_codec_h264_fps_to_bitrate_ratio    0.5         //帧率变化对应码率变化比例 参考帧率15

#define  tdav_video_main       0
#define  tdav_video_child      1
#define  tdav_video_share      2    // 用于共享屏幕、文档等外部输入视频

#define  tdav_codec_h264_decoder_timeout        30000      //解码器空闲超时释放（ms）

// H.264 start code length.
#define H264_SC_LENGTH 4

uint8_t H264_START_CODE_PREFIX_NOFIRST[3] = { 0x00, 0x00, 0x01 };

// Maximum allowed NALUs in one output frame.
#define MAX_NALUS_PERFRAME 32

#define USE_FFMPEG_ENCODE 1

#define H264_DATA_DUMP 1

class WebRtcEncodedImageCallback;
class WebRtcDecodedImageCallback;

enum Windows_GPU_Series {
	WINDOWS_GPU_NONE = 0,
	WINDOWS_GPU_INTEL,
	WINDOWS_GPU_NVDIA,
	WINDOWS_GPU_AMD
};

typedef struct h264_decoder_manager_s
{
#if FFMPEG_SUPPORT
	AVCodec* codec;
	AVCodecContext* context;
	AVFrame* picture;
#endif
#if HARDWARE_CODECS
	youme::webrtc::VideoDecoder* webrtc_codec;
	WebRtcDecodedImageCallback* callback;
#endif
	void* accumulator;
	tsk_size_t accumulator_pos;
	tsk_size_t accumulator_size;
	uint16_t last_seq;
	uint16_t last_frame_last_pkt_seqnum;
	uint16_t last_seqnum_with_mark;
	tsk_bool_t isCurFrameCrash; // Means the current frame crash
	tsk_bool_t isCurGopCrash; // Means the current gop crash
	tsk_bool_t isSpsFrame;
	tsk_bool_t webrtc_hwcodec_issupport;   //webrtc hardware encoding support
										   //uint16_t info[2][50];
										   //uint16_t index;
	uint16_t pkt_num;
	tsk_bool_t init_idr;
	int32_t video_id;
	uint32_t width;
	uint32_t height;
	uint64_t last_frame_time;
	tsk_mutex_handle_t * mutex;
	tsk_bool_t texture_cb;

#if H264_DATA_DUMP
    char cFIlePath[1024];
    FILE *pfWrite;
    int  file_size;
    int  max_size;
#endif
} h264_decoder_manager_t;

typedef std::map<int32_t, h264_decoder_manager_t*> h264_decoder_manager_map_t;
typedef std::vector<int32_t> h264_encoder_fps_vector_t;

enum h264_encoder_bitrate_control {
	H264_ENCODER_RC_CQP = 0,
	H264_ENCODER_RC_CRF,
	H264_ENCODER_RC_ABR
};

typedef struct h264_encoder_manager_s
{
	uint64_t frame_count;
	uint16_t quality; // [1-31]
	uint16_t max_bw_kpbs;
	uint16_t width;
	uint16_t height;
	uint16_t fps;
    uint16_t lastSetedFPS;
	int16_t rotation;
	uint16_t bitrate_level;
	uint16_t resolution_level;
	uint16_t fps_level;
	uint16_t sec_frame_index;
	uint16_t video_id;
	uint16_t gop_size;
	tsk_bool_t force_idr;
	h264_encoder_fps_vector_t vec_fps;
	tsk_bool_t webrtc_hwcodec_issupport;   //webrtc hardware encoding support
	tsk_mutex_handle_t * mutex;
#if FFMPEG_SUPPORT
	h264_encoder_bitrate_control bitrate_control;
	x264_param_t *pX264Param;
	x264_t       *pX264Handle;
	x264_nal_t   *pNals;
	x264_picture_t *pPicIn;
	x264_picture_t *pPicOut;
	int32_t i4Nal;
	int64_t i8FrameNum;
	AVCodec *codec;
	AVCodecContext *context;
#endif
#if HARDWARE_CODECS
	youme::webrtc::VideoEncoder* webrtc_codec;
	WebRtcEncodedImageCallback* callback;

#endif

	// for fps change
	int32_t  outputInterval = 0;
	uint64_t outputVideoTs = 0;
	uint64_t lastInputTs = 0;
	int32_t inputInterval = 0;
	int32_t frameCount = 0;
    
#if H264_DATA_DUMP
    char    cFIlePath[1024];
    FILE    *pfWrite;
    int file_size;
    int max_size;
#endif

}h264_encoder_manager_t;
typedef std::map<int32_t, h264_encoder_manager_t*> h264_encoder_manager_map_t;

typedef struct tdav_codec_h264_s
{
	TDAV_DECLARE_CODEC_H264_COMMON;
	// Encoder
	struct {

		tsk_bool_t hw_encode_enable;
		tsk_bool_t hw_encode_force_no;  //force sw codec
		tsk_bool_t passthrough; // whether to bypass encoding
		h264_encoder_manager_map_t * video_enc_map;

		//ABR模式下的码率最大,平均值比例
		float abr_avg_bitrate_percent;
		float abr_max_bitrate_percent;
		int32_t idr_frame_interval_multiple;
	} encoder;

	// decoder
	struct {
		tsk_bool_t hw_decode_enable;
		tsk_bool_t hw_decode_force_no;  //force sw codec
		tsk_bool_t passthrough; // whether to bypass decoding
		h264_decoder_manager_map_t* video_dec_map;
		predecodeCb_t predecode_callback;
		tsk_bool_t predecode_need_decode_and_render;
		tsk_bool_t first_frame;
		videoDecodeParamCb_t decode_param_callback;	
	} decoder;
}
tdav_codec_h264_t;

#if !defined(TDAV_H264_GOP_SIZE_IN_SECONDS)
#   define TDAV_H264_GOP_SIZE_IN_SECONDS		25
#endif

#define kResetRotationTrue tsk_true
#define kResetRotationFalse tsk_false

static int tdav_codec_h264_init(tdav_codec_h264_t* self, profile_idc_t profile);
static int tdav_codec_h264_deinit(tdav_codec_h264_t* self);
static int tdav_codec_h264_open_sw_encoder(tdav_codec_h264_t* self);
static int tdav_codec_h264_open_encoder(tdav_codec_h264_t* self, uint32_t video_id, const void* context);
static int tdav_codec_h264_close_encoder(tdav_codec_h264_t* self, tsk_bool_t reset_rotation);
static void tdav_codec_h264_close_encoder(h264_encoder_manager_t *video_enc_mgr);
static int tdav_codec_h264_close_decoder(tdav_codec_h264_t* self);
static void tdav_codec_h264_adjust_bitrate(tdav_codec_h264_t* self, h264_encoder_manager_t *video_enc_mgr, int level);
static tsk_bool_t tdav_codec_h264_hardware_is_supported(int w, int h);
int tdav_codec_h264_open_hw_encoder(tdav_codec_h264_t* self, h264_encoder_manager_t *video_enc_mgr, int video_id, const void* context);
int tdav_codec_h264_open_sw_encoder(tdav_codec_h264_t* self, h264_encoder_manager_t *video_enc_mgr, bool use_hwaccel = true);
static int tdav_codec_h264_reset_decoder(tdav_codec_h264_t *self, int32_t session_id, const void* context);
static void tdav_codec_h264_free_timeout_decoder(tdav_codec_h264_t *self);
static int tdav_codec_h264_close_decoder_with_sessionid(tdav_codec_h264_t* self, int sessionId);
static int tdav_codec_h264_creat_sw_decoder(tdav_codec_h264_t *self, h264_decoder_manager_t *video_dec_mgr, bool use_hwaccel = true);

static int tdav_codec_h264_create_encode_dump_file(h264_encoder_manager_t* encoder_mgr, int type, tsk_boolean_t HWEncode);
static int tdav_codec_h264_create_decode_dump_file(h264_decoder_manager_t* decoder_mgr, int sessionid);

#ifdef WIN32
static Windows_GPU_Series tdav_codec_h264_get_windows_gpu();
#endif

//回调session
static webrtc_encode_h264_callback _gSessionEncodeCallback = NULL;
static webrtc_decode_h264_callback _gSessionDecodeCallback = NULL;

/*=======================================webrtc codec callback======================================*/
#if  HARDWARE_CODECS
class WebRtcEncodedImageCallback : public youme::webrtc::EncodedImageCallback
{
public:
	WebRtcEncodedImageCallback() :_session(NULL), _video_id(0), _video_enc_mgr(nullptr) {}
	virtual ~WebRtcEncodedImageCallback() {}

	// Callback function which is called when an image has been encoded.
	virtual int32_t Encoded(const youme::webrtc::EncodedImage& encoded_image,
		const youme::webrtc::CodecSpecificInfo* codec_specific_info,
		const youme::webrtc::RTPFragmentationHeader* fragmentation)
	{
		if (!_session || !_gSessionEncodeCallback || !_video_enc_mgr)
			return 0;
#if 0
		if (!_lasttimes)
			_lasttimes = tsk_time_now();
		_frameCount++;
		_bitrateCount += encoded_image._length;
		if (tsk_time_now() - _lasttimes > 5000)
		{
			// hardcode slice startcode len: 4byte
			_lasttimes = tsk_time_now();
			_frameCount = 0;
			_bitrateCount = 0;
		}
#endif
        
        int video_share_tpye = tmedia_get_video_share_type();
        h264_encoder_manager_t* video_enc_mgr = (h264_encoder_manager_t*)_video_enc_mgr;
        
#if H264_DATA_DUMP
        if (video_enc_mgr->pfWrite)
        {
            if (video_enc_mgr->file_size > video_enc_mgr->max_size) {
                tdav_codec_h264_create_encode_dump_file(video_enc_mgr, _video_id, true);
            }
            
            if (video_enc_mgr->pfWrite) {
                fwrite(encoded_image._buffer, 1, encoded_image._length, video_enc_mgr->pfWrite);
                video_enc_mgr->file_size += encoded_image._length;
            }
        }
#endif

        //共享流录制分支
		if (tdav_video_share == _video_id && (video_share_tpye & ENABLE_SAVE))
		{
#if FFMPEG_SUPPORT
            tdav_session_av_t* base = (tdav_session_av_t*)_session;
            int sessionId = base->producer->session_id;
            auto pRecorder = YMVideoRecorderManager::getInstance()->getRecorderBySession(sessionId);
            int nalu_type = encoded_image._buffer[4] & 0x1F; // hardcode slice startcode len: 4byte
			if (pRecorder)
			{
                uint8_t* avcc_sps = nullptr;
                int avcc_sps_size = 0;
                uint8_t* avcc_pps = nullptr;
                int avcc_pps_size = 0;
                uint8_t* avcc_idr = nullptr;
                int avcc_idr_size = 0;

                if (fragmentation->fragmentationVectorSize >= 3) //起码包含sps,pps,和IDR数据
                {
                    pRecorder->setVideoInfo(video_enc_mgr->width, video_enc_mgr->height, video_enc_mgr->fps, video_enc_mgr->max_bw_kpbs, 1000);
                    YMVideoRecorderManager::getInstance()->annexbToAvcc(encoded_image._buffer + fragmentation->fragmentationOffset[0], fragmentation->fragmentationLength[0], &avcc_sps, &avcc_sps_size);
                    YMVideoRecorderManager::getInstance()->annexbToAvcc(encoded_image._buffer + fragmentation->fragmentationOffset[1], fragmentation->fragmentationLength[1], &avcc_pps, &avcc_pps_size);
                    YMVideoRecorderManager::getInstance()->setSpsAndPps(sessionId, avcc_sps, avcc_sps_size, avcc_pps, avcc_pps_size);
					for (int index = 2; index < fragmentation->fragmentationVectorSize; index++)
					{
						int cur_nalu_type = encoded_image._buffer[fragmentation->fragmentationOffset[index]] & 0x1F;
						if (cur_nalu_type == H264_NAL_SLICE_IDR)
						{
                            YMVideoRecorderManager::getInstance()->annexbToAvcc(encoded_image._buffer + fragmentation->fragmentationOffset[index], fragmentation->fragmentationLength[index], &avcc_idr, &avcc_idr_size);
							YMVideoRecorderManager::getInstance()->inputVideoData(sessionId, avcc_idr, avcc_idr_size, encoded_image.capture_time_ms_, nalu_type);
						}
					}
                } else {
                    uint8_t* non_idr = nullptr;
                    int non_idr_size = 0;
                    YMVideoRecorderManager::getInstance()->annexbToAvcc(encoded_image._buffer+4, encoded_image._length-4, &non_idr, &non_idr_size);
    				YMVideoRecorderManager::getInstance()->inputVideoData(sessionId, non_idr, non_idr_size, encoded_image.capture_time_ms_, nalu_type);
                }
			}
#endif
		}

        //大小流，共享流发送之前处理流程
        if (_video_id != tdav_video_share || (video_share_tpye & ENABLE_SHARE)) {
            youme::webrtc::EncodedImage encoded_image_temp = encoded_image;
            encoded_image_temp._length = 0;

			// packet SPS/PPS/SEI into STAP-A
			// attention: huawei harware code has more than 2 slice in one frame
            if (fragmentation->fragmentationVectorSize >= 3) //起码包含sps,pps,和IDR数据
            {
                uint8_t *_temp_buf = (uint8_t *)malloc(1024); // mtu size
                int frag_index = 0;
                int frag_length = fragmentation->fragmentationLength[0];
                int frag_offset = fragmentation->fragmentationOffset[0];
                int payload_size_left = 1024 - 4; // hardcode slice startcode len: 4byte
                int payload_offset = 0;
                _temp_buf[payload_offset] = 0x0;
                _temp_buf[payload_offset + 1] = 0x0;
                _temp_buf[payload_offset + 2] = 0x0;
                _temp_buf[payload_offset + 3] = 0x1;
                payload_offset += 4;
                
                while (payload_size_left > 0) {
                    if (frag_index == 0) {
                        _temp_buf[payload_offset] = 0x58;    // stap-a header
                        payload_offset++;
                    }

				_temp_buf[payload_offset] = (frag_length & 0xFF00) >> 8;   // nalu1 size
				_temp_buf[payload_offset + 1] = (frag_length & 0xFF);
				payload_offset += 2;

				memcpy(&_temp_buf[payload_offset], encoded_image._buffer + frag_offset, frag_length);
				payload_offset += frag_length;

				frag_index++;
				frag_length = fragmentation->fragmentationLength[frag_index];
				frag_offset = fragmentation->fragmentationOffset[frag_index];

				payload_size_left = payload_size_left - frag_length;
				if (payload_size_left < 0 || frag_index >= (fragmentation->fragmentationVectorSize - 1)) {
					encoded_image_temp._buffer = _temp_buf;
					encoded_image_temp._length = payload_offset;
					(*_gSessionEncodeCallback)(encoded_image_temp._buffer, encoded_image_temp._length, encoded_image_temp.capture_time_ms_, _video_id, _session);

					if (frag_index <= fragmentation->fragmentationVectorSize) {
						uint8_t *remain_buf = encoded_image._buffer + frag_offset - 4;
						if (remain_buf[0] == 0x0 && remain_buf[1] == 0x0 \
							&& remain_buf[2] == 0x0 && remain_buf[3] == 0x1) {
							// do nothing, continue!
						}
						else {
							remain_buf[0] = 0x0;
							remain_buf[1] = 0x0;
							remain_buf[2] = 0x0;
							remain_buf[3] = 0x1;
						}

                            // remain fragment
                            (*_gSessionEncodeCallback)(remain_buf, encoded_image._length - frag_offset + 4, encoded_image.capture_time_ms_, _video_id, _session);
                            break;
                        }
                    }
                }
                       
                if (_temp_buf) {
                   free(_temp_buf);
                   _temp_buf = NULL;
                }
            } 
            else{ 
                (*_gSessionEncodeCallback)(encoded_image._buffer, encoded_image._length, encoded_image.capture_time_ms_, _video_id, _session);
            }
        }

		return 0;
	}
public:
	void * _session;
	int _video_id;
	int _frameCount = 0;
	int _bitrateCount = 0;
	uint64_t _lasttimes = 0;
	void * _video_enc_mgr;
};


class WebRtcDecodedImageCallback : public youme::webrtc::DecodedImageCallback {
public:
	WebRtcDecodedImageCallback() :_session(NULL), _video_id(0), _width(0), _height(0), _dest_yuv(NULL), _yuv_size(0) {}
	virtual ~WebRtcDecodedImageCallback() {
		if (_dest_yuv)
			free(_dest_yuv);
		_listHeader.clear();
	}

	virtual int32_t Decoded(youme::webrtc::VideoFrame& decodedImage)
	{
		if (!_session || !_gSessionDecodeCallback)
			return 0;
		rtc::scoped_refptr<youme::webrtc::VideoFrameBuffer> framebuffer = decodedImage.video_frame_buffer();
		bool isTexture;
		std::unique_lock<std::mutex> l(_mutex);
		int64_t timestamp = decodedImage.render_time_ms();//decodedImage.timestamp();
		uint8_t* data = nullptr;
		int size = decodedImage.height()*decodedImage.width() * 3 / 2;
		std::list<std::unique_ptr<trtp_rtp_header_t>>::iterator iter = _listHeader.begin();
		while (iter != _listHeader.end()) {
			if ((*iter).get()->timestampl < timestamp)
			{
				_listHeader.erase(iter);
			}
			else if ((*iter).get()->timestampl == timestamp)
			{
				trtp_rtp_header_t * rtp_hdr = (trtp_rtp_header_t*)(*iter).get();
				if (rtp_hdr->video_display_width && rtp_hdr->video_display_height) {
					_width = rtp_hdr->video_display_width;
					_height = rtp_hdr->video_display_height;
				}
				if (!_width || !_height) {
					_width = framebuffer->width();
					_height = framebuffer->height();
				}
				if (framebuffer.get()->native_handle() != nullptr) {
					data = (uint8_t*)framebuffer.get()->native_handle();
					size = 0;
					isTexture = true;
					//  TSK_DEBUG_INFO("h264 decoder hw out timestmap:%lld\n", timestamp);
				}
				else {
					data = (uint8_t*)framebuffer->data(youme::webrtc::kYPlane);
					// TSK_DEBUG_INFO("h264 decoder2 sw:%d,sh:%d,dw:%d,dh:%d\n",\
					                    decodedImage.video_frame_buffer()->width(), decodedImage.video_frame_buffer()->height(),\
                    _width,_height);
					if (_width < framebuffer->width() ||
						_height < framebuffer->height()) {
						int src_w = framebuffer->width();
						int src_h = framebuffer->height();
						int dest_w = _width;
						int dest_h = _height;
						size = dest_w*dest_h * 3 / 2;
						if (_yuv_size < size)
						{
							if (_dest_yuv)
							{
								free(_dest_yuv);
								_yuv_size = 0;
							}
							_dest_yuv = (uint8_t*)malloc(size);
							if (!_dest_yuv)
								break;
							_yuv_size = size;
						}
						uint8_t* src_y = data;
						uint8_t* src_u = data + src_w*src_h;
						uint8_t* src_v = data + src_w*src_h * 5 / 4;
						uint8_t* dest_y = _dest_yuv;
						uint8_t* dest_u = _dest_yuv + dest_w*dest_h;
						uint8_t* dest_v = _dest_yuv + dest_w*dest_h * 5 / 4;
						//y
						for (int i = 0; i < dest_h; ++i) {
							memcpy(dest_y + i*dest_w, src_y + i*src_w, dest_w);
						}
						//uv
						for (int i = 0; i < dest_h / 2; ++i) {
							memcpy(dest_u + i*dest_w / 2, src_u + i*src_w / 2, dest_w / 2);
							memcpy(dest_v + i*dest_w / 2, src_v + i*src_w / 2, dest_w / 2);
						}
						data = _dest_yuv;
					}
					isTexture = false;
				}
				rtp_hdr->video_display_width = _width;
				rtp_hdr->video_display_height = _height;
				(*_gSessionDecodeCallback)(data, size, timestamp, _session, (void*)rtp_hdr, isTexture, _video_id);
				_listHeader.erase(iter);
				break;
			}
			else {
				break;
			}
			iter = _listHeader.begin();
		}

		return 0;
	}

	virtual int32_t Decoded(youme::webrtc::VideoFrame& decodedImage, int64_t decode_time_ms) {
		// The default implementation ignores custom decode time value.
		return Decoded(decodedImage);
	}

	virtual int32_t ReceivedDecodedReferenceFrame(const uint64_t pictureId) {
		return -1;
	}

	virtual int32_t ReceivedDecodedFrame(const uint64_t pictureId) {
		return -1;
	}

	void PushReserve(void* hdr)
	{
		std::unique_lock<std::mutex> l(_mutex);
		std::unique_ptr<trtp_rtp_header_t> t(new trtp_rtp_header_t);
		memcpy(t.get(), hdr, sizeof(trtp_rtp_header_t));
		_listHeader.push_back(std::move(t));
	}
public:
	int _video_id;
	int _width;
	int _height;
	uint8_t* _dest_yuv;
	uint32_t _yuv_size;
	void * _session;
	std::mutex _mutex;
	std::list<std::unique_ptr<trtp_rtp_header_t>> _listHeader;
};

#endif


class c_mutex
{
public:
	c_mutex(tsk_mutex_handle_t* h) :_h(h)
	{
		if (h)
			tsk_mutex_lock(h);
	}
	~c_mutex()
	{
		if (_h)
			tsk_mutex_unlock(_h);
	}
private:
	tsk_mutex_handle_t * _h;
};

#ifdef WIN32
static Windows_GPU_Series tdav_codec_h264_get_windows_gpu() {
	HRESULT hr;
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	IWbemLocator *pIWbemLocator = NULL;
	hr = CoCreateInstance(__uuidof(WbemLocator), NULL,
		CLSCTX_INPROC_SERVER, __uuidof(IWbemLocator),
		(LPVOID *)&pIWbemLocator);

	BSTR bstrServer = SysAllocString(L"\\\\.\\root\\cimv2");
	IWbemServices *pIWbemServices;
	hr = pIWbemLocator->ConnectServer(bstrServer, NULL, NULL, 0L, 0L, NULL,
		NULL, &pIWbemServices);

	hr = CoSetProxyBlanket(pIWbemServices, RPC_C_AUTHN_WINNT,
		RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL,
		RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
		EOAC_DEFAULT);

	BSTR bstrWQL = SysAllocString(L"WQL");
	BSTR bstrPath = SysAllocString(L"select * from Win32_VideoController");
	IEnumWbemClassObject* pEnum;
	hr = pIWbemServices->ExecQuery(bstrWQL, bstrPath, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnum);

	IWbemClassObject* pObj = NULL;
	ULONG uReturned;
	VARIANT var;
	Windows_GPU_Series gpu = WINDOWS_GPU_NONE;
	do {
		hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &uReturned);
		if (uReturned)
		{
			hr = pObj->Get(L"ConfigManagerErrorCode", 0, &var, NULL, NULL);
			if (SUCCEEDED(hr))
			{
				if (var.lVal != 0) {
					TSK_DEBUG_WARN("WMI VideoController ConfigManagerErrorCode, continue");
					continue;
				}
			}

			hr = pObj->Get(L"Name", 0, &var, NULL, NULL);
			if (SUCCEEDED(hr))
			{
				char sString_name[256];
				WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, sString_name, sizeof(sString_name), NULL, NULL);
				std::string name(sString_name);
				TSK_DEBUG_INFO("Windows GPU Name:%s", name.c_str());

				if (name.find("Intel") != -1) {
					hr = pObj->Get(L"DriverVersion", 0, &var, NULL, NULL);
					if (SUCCEEDED(hr))
					{
						char sString_version[256];
						WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, sString_version, sizeof(sString_version), NULL, NULL);
						std::string driverVersion = sString_version;
						// 25.20.100.6373版本的Intel核显驱动会导致crash
						if (driverVersion.find("25.20.100.6373") != -1) {
							TSK_DEBUG_WARN("Intel Graphics driver version:25.20.100.6373 would crash, continue!!", name.c_str());
							continue;
						}
					}
					gpu = WINDOWS_GPU_INTEL;
					TSK_DEBUG_INFO("find Intel hardware accelerator");
					break;
				}
				if (name.find("GeForce") != -1) {
					gpu = WINDOWS_GPU_NVDIA;
					TSK_DEBUG_INFO("find Nvdia hardware accelerator");
					break;
				}
				if (name.find("Radeon") != -1) {
					// todu aeron need support AMD !!!
					TSK_DEBUG_WARN("not support Amd hardware accelerator, switch to software!!");
					gpu = WINDOWS_GPU_NONE;
					continue;
				}
			}
		}
		else {
			break;
		}
	} while (uReturned);

	pEnum->Release();
	SysFreeString(bstrPath);
	SysFreeString(bstrWQL);
	pIWbemServices->Release();
	SysFreeString(bstrServer);
	//CoUninitialize();

	return gpu;
}

//static Windows_GPU_Series tdav_codec_h264_get_windows_gpu() {
//	IDXGIFactory * pFactory;
//	IDXGIAdapter * pAdapter;
//	std::vector <IDXGIAdapter*> vAdapters;
//
//	int iAdapterNum = 0;
//
//	HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFactory));
//
//	if (FAILED(hr)) {
//		TSK_DEBUG_INFO("can not find available GPU accelerator");
//		return WINDOWS_GPU_NONE;
//	}
//
//	while (pFactory->EnumAdapters(iAdapterNum, &pAdapter) != DXGI_ERROR_NOT_FOUND)
//	{
//		vAdapters.push_back(pAdapter);
//		++iAdapterNum;
//	}
//
//	for (size_t i = 0; i < vAdapters.size(); i++)
//	{
//		DXGI_ADAPTER_DESC adapterDesc;
//		vAdapters[i]->GetDesc(&adapterDesc);
//
//		std::wstring wstr(adapterDesc.Description);
//		std::string str(wstr.length(), ' ');
//		std::copy(wstr.begin(), wstr.end(), str.begin());
//
//		TSK_DEBUG_INFO("Windows GPU Name:%s", str.c_str());
//
//		if (str.find("Intel") != -1) {
//			return WINDOWS_GPU_INTEL;
//		}
//		if (str.find("GeForce") != -1) {
//			return WINDOWS_GPU_NVDIA;
//		}
//		if (str.find("Radeon") != -1) {
//			return WINDOWS_GPU_AMD;
//		}
//	}
//	vAdapters.clear();
//	TSK_DEBUG_INFO("can not find available GPU accelerator");
//	return WINDOWS_GPU_NONE;
//}
#endif

/* ============ H.264 Base/Main Profile X.X Plugin interface functions ================= */

static int tdav_codec_h264_set(tmedia_codec_t* self, const tmedia_param_t* param)
{
	tdav_codec_h264_t* h264 = (tdav_codec_h264_t*)self;
	if (param->value_type == tmedia_pvt_int32) {
		if (tsk_striequals(param->key, "action")) {
			tmedia_codec_action_t action = (tmedia_codec_action_t)TSK_TO_INT32((uint8_t*)param->value);
			switch (action) {
			case tmedia_codec_action_encode_idr:
			{
                //因为ssrc标示没用，所以所有源都重编i帧 (包括camera大小流和共享流)
                if( h264->encoder.video_enc_map){
                    auto iter = h264->encoder.video_enc_map->begin();
                    for (; iter != h264->encoder.video_enc_map->end(); iter++) {
                        iter->second->force_idr = true;
                    }
                }
                //TSK_DEBUG_INFO("rtcp request idr frame..");
				//h264->encoder.force_idr = tsk_true;
				break;
			}
			case tmedia_codec_action_bw_down:
			{
				//h264->encoder.quality = TSK_CLAMP(1, (h264->encoder.quality + 1), 31);

				break;
			}
			case tmedia_codec_action_bw_up:
			{
				//h264->encoder.quality = TSK_CLAMP(1, (h264->encoder.quality - 1), 31);

				break;
			}
			}
			return 0;
		}
		else if (tsk_striequals(param->key, "bw_kbps")) {
			int32_t max_bw_userdefine = self->bandwidth_max_upload;
			//int32_t max_bw_new = *((int32_t*)param->value);
			if (max_bw_userdefine > 0) {
				// do not use more than what the user defined in it's configuration
				//h264->encoder.max_bw_kpbs = TSK_MIN(max_bw_new, max_bw_userdefine);
			}
			else {
				//h264->encoder.max_bw_kpbs = max_bw_new;
			}
			return 0;
		}
		else if (tsk_striequals(param->key, "rotation")) {
			//int32_t rotation = *((int32_t*)param->value);

			return 0;
		}
		else if (tsk_striequals(param->key, "close_decoder"))
		{
			int sessionId = *((int32_t*)param->value);
			tdav_codec_h264_close_decoder_with_sessionid(h264, sessionId);
			return 0;
		}
		else if (tsk_striequals(param->key, TMEDIA_PARAM_KEY_VIDEO_PREDECODE_NEED_DECODE_AND_RENDER))
		{
			h264->decoder.predecode_need_decode_and_render = *((tsk_bool_t*)param->value);
            TSK_DEBUG_INFO("set predecode_need_decode_and_render:%d", h264->decoder.predecode_need_decode_and_render);
		}
	}
	else if (param->value_type == tmedia_pvt_pvoid)
	{
		if (tsk_striequals(param->key, "session_encode_callback"))
		{
			_gSessionEncodeCallback = (webrtc_encode_h264_callback)param->value;
		}
		else if (tsk_striequals(param->key, "session_decode_callback"))
		{
			_gSessionDecodeCallback = (webrtc_decode_h264_callback)param->value;

		} 
		else if (tsk_striequals(param->key, TMEDIA_PARAM_KEY_VIDEO_PREDECODE_CB))
		{
			h264->decoder.predecode_callback = (predecodeCb_t)param->value;
            TSK_DEBUG_INFO("set predecode_callback:%p", h264->decoder.predecode_callback);
		}
		else if (tsk_striequals(param->key, TMEDIA_PARAM_KEY_VIDEO_DECODE_PARAM_CB))
		{
			h264->decoder.decode_param_callback = (videoDecodeParamCb_t)param->value;
            TSK_DEBUG_INFO("set decode_param_callback:%p", h264->decoder.decode_param_callback);
		}
	}
	return -1;
}


static int tdav_codec_h264_open(tmedia_codec_t* self, const void* context)
{
	int ret;
	tdav_codec_h264_t* h264 = (tdav_codec_h264_t*)self;

	if (!h264) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	/* the caller (base class) already checked that the codec is not opened */

	//	Encoder
	if ((ret = tdav_codec_h264_open_encoder(h264, tdav_video_main, context))) {
		return ret;
	}

	unsigned width = 0, height = 0;
	tmedia_defaults_get_video_size_child(&width, &height);
	if (width && height) {
		if ((ret = tdav_codec_h264_open_encoder(h264, tdav_video_child, context))) {
			return ret;
		}
	}

	width = 0, height = 0;
	tmedia_defaults_get_video_size_share(&width, &height);
	if (width && height) {
		if ((ret = tdav_codec_h264_open_encoder(h264, tdav_video_share, context))) {
			return ret;
		}
	}
    
	return 0;
}

static int tdav_codec_h264_close(tmedia_codec_t* self)
{
	tdav_codec_h264_t* h264 = (tdav_codec_h264_t*)self;

	if (!h264) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	/* the caller (base class) alreasy checked that the codec is opened */

	//	Encoder
	tdav_codec_h264_close_encoder(h264, kResetRotationTrue);

	//	Decoder
	tdav_codec_h264_close_decoder(h264);
	TSK_DEBUG_INFO("tdav_codec_h264_close ");
	return 0;
}

static int tdav_codec_h264_reset_encoder(tmedia_codec_t* self, h264_encoder_manager_t * video_enc_mgr, const void* context)
{
	tdav_codec_h264_t* h264 = (tdav_codec_h264_t*)self;
	int video_id = video_enc_mgr->video_id;
	tdav_codec_h264_close_encoder(video_enc_mgr);
#if HARDWARE_CODECS
	if (!tdav_codec_h264_open_hw_encoder(h264, video_enc_mgr, video_id, context)) {
        TSK_DEBUG_INFO("tdav_codec_h264_open_hw_encoder success");
        video_enc_mgr->webrtc_hwcodec_issupport = tsk_true;
#if H264_DATA_DUMP
        if (tmedia_defaults_get_h264_file_size_kb() > 0 && !video_enc_mgr->pfWrite) {
            tdav_codec_h264_create_encode_dump_file(video_enc_mgr, video_id, true);
        }
#endif
        return 0;
	}
    TSK_DEBUG_INFO("tdav_codec_h264_open_hw_encoder fail or not");
#endif
	if (!tdav_codec_h264_open_sw_encoder(h264, video_enc_mgr)) {
        video_enc_mgr->webrtc_hwcodec_issupport = tsk_false;
#if H264_DATA_DUMP
        if (tmedia_defaults_get_h264_file_size_kb() > 0 && !video_enc_mgr->pfWrite) {
            tdav_codec_h264_create_encode_dump_file(video_enc_mgr, video_id, false);
        }
#endif
		return 0;
	}

	return 0;
}


static int tdav_codec_h264_encode_check_param(tmedia_codec_t* self, h264_encoder_manager_t * video_enc_mgr, const void* context)
{
	int fps;
	int bitrate_level, fps_leveL, resolution_level;
	tdav_codec_h264_t* h264 = (tdav_codec_h264_t*)self;
	int video_id = video_enc_mgr->video_id;
	unsigned int width = video_enc_mgr->width;
	unsigned int height = video_enc_mgr->height;
	unsigned width_local = 0, height_local = 0, size_local = 0;
	unsigned int new_gop_size = 1;

	if (tdav_video_main == video_id) {
		bitrate_level = Config_GetInt(tdav_codec_video_bitrate_level, 100);
		fps_leveL = Config_GetInt(tdav_codec_video_fps_level, 100);
		resolution_level = Config_GetInt(tdav_codec_video_resolution_level, 100);
		fps = tmedia_defaults_get_video_fps();

		tmedia_defaults_get_video_size(&width_local, &height_local);
		size_local = width_local * height_local;
	}
	else if (tdav_video_child == video_id) {
		bitrate_level = Config_GetInt(tdav_codec_video_bitrate_second_level, 100);
		fps_leveL = Config_GetInt(tdav_codec_video_fps_second_level, 100);
		resolution_level = Config_GetInt(tdav_codec_video_resolution_second_level, 100);
		fps = tmedia_defaults_get_video_fps_child();

		tmedia_defaults_get_video_size_child(&width_local, &height_local);
		size_local = width_local * height_local;
	}
	else {
		bitrate_level = Config_GetInt(tdav_codec_video_bitrate_share_level, 100);
		fps_leveL = Config_GetInt(tdav_codec_video_fps_share_level, 100);
		resolution_level = Config_GetInt(tdav_codec_video_resolution_share_level, 100);
		fps = tmedia_defaults_get_video_fps_share();

		tmedia_defaults_get_video_size_share(&width_local, &height_local);
		size_local = width_local * height_local;
	}

	// 默认GOP大小为1s
	new_gop_size = Config_GetInt(tdav_codec_video_encode_gop_size, 1);

	if (fps != video_enc_mgr->lastSetedFPS
        || fps_leveL != video_enc_mgr->fps_level
		|| resolution_level != video_enc_mgr->resolution_level
		|| (size_local != 0 && (width_local != width || height_local != height))
		|| (new_gop_size != video_enc_mgr->gop_size)) {

		// sanity check fps_level and resolution_level
		fps_leveL = (fps_leveL > 100) ? 100 : fps_leveL;
		resolution_level = (resolution_level > 100) ? 100 : resolution_level;

		TSK_DEBUG_INFO("h264 encoder reset video_id:%d, fps_level:%d, new_fps_level:%d width:%u height:%u, gop:%d\n"
			, video_id, video_enc_mgr->fps_level, fps_leveL, width_local, height_local, new_gop_size);
		int fps_def = fps;
		video_enc_mgr->fps_level = fps_leveL;
		video_enc_mgr->resolution_level = resolution_level;
		video_enc_mgr->bitrate_level = bitrate_level;
		video_enc_mgr->width = width_local;
		video_enc_mgr->height = height_local;
		video_enc_mgr->gop_size = new_gop_size;
		video_enc_mgr->sec_frame_index = 0;
		video_enc_mgr->vec_fps.clear();
		video_enc_mgr->vec_fps.resize(fps_def);
		for (int i = 0; i < fps_def; ++i) {
			video_enc_mgr->vec_fps[i] = 0;
		}

		if (100 != fps_leveL) {
			int fps_new = fps_def * fps_leveL / 100.0f;
			float offset = fps_def / (float)fps_new;
			int num = 0;
			for (float i = 0.0f; i < fps_def; ) {
				video_enc_mgr->vec_fps[(int)i] = 1;
				i += offset;
				num++;
			}
			if (num < fps_new)
				video_enc_mgr->vec_fps[fps_def - 1] = 1;
		}

		return tdav_codec_h264_reset_encoder(self, video_enc_mgr, context);

	}
	else if (bitrate_level != video_enc_mgr->bitrate_level)
	{
		video_enc_mgr->bitrate_level = bitrate_level;
		tdav_codec_h264_adjust_bitrate(h264, video_enc_mgr, bitrate_level);
	}

	return 0;
}

//000000000000000
//111111110000000
static int tdav_codec_h264_fps_lose(h264_encoder_manager_t * video_enc_mgr)
{
	if (video_enc_mgr->fps_level == 100)
		return 0;

	//int fps_def = video_enc_mgr->video_id == tdav_video_main ? tmedia_defaults_get_video_fps() : tmedia_defaults_get_video_fps_child();
	int fps_def = 0;
	if (tdav_video_main == video_enc_mgr->video_id) {
		fps_def = tmedia_defaults_get_video_fps();
	}
	else if (tdav_video_child == video_enc_mgr->video_id) {
		fps_def = tmedia_defaults_get_video_fps_child();
	}
	else {
		fps_def = tmedia_defaults_get_video_fps_share();
	}

	if (video_enc_mgr->sec_frame_index >= fps_def) {
		video_enc_mgr->sec_frame_index = 0;
	}

	if (video_enc_mgr->vec_fps[video_enc_mgr->sec_frame_index++] == 1) {
		return 0;
	}
	//printf("tdav_codec_h264_fps_lose ---%d\n",self->encoder.sec_frame_index);

	return -1;
}

static void tdav_codec_h264_get_nalu_type(uint8_t * buf, int * nalu_type, int * start_len)
{
	if (!buf) return;

	int start_code_len = 0;

	if (buf[0] == 0x0 && buf[1] == 0x0 && buf[2] == 0x0 && buf[3] == 0x1)
	{
		start_code_len = 4;
	}
	else if (buf[0] == 0x0 && buf[1] == 0x0 && buf[2] == 0x1)
	{
		start_code_len = 3;
	}

	*nalu_type = buf[start_code_len] & 0x1F;
	*start_len = start_code_len;

}

int32_t tdav_codec_h264_get_nextNaluPosition(uint8_t *buffer, size_t buffer_size, size_t *start_code_length) {
	if (buffer_size < H264_SC_LENGTH) {
		return -1;
	}
	uint8_t *head = buffer;
	// Set end buffer pointer to 4 bytes before actual buffer end so we can
	// access head[1], head[2] and head[3] in a loop without buffer overrun.
	uint8_t *end = buffer + buffer_size - H264_SC_LENGTH;

	while (head < end) {
		if (head[0]) {
			head++;
			continue;
		}
		if (head[1]) { // got 00xx
			head += 2;
			continue;
		}
		if (head[2] && head[2] != 0x01) { // got 0000xx
			head += 3;
			continue;
		}
		if (head[2] == 0x01) { // case 000001
			*start_code_length = 3;
			return (int32_t)(head - buffer);
		}
		if (head[3] != 0x01) { // got 000000xx
			head++; // xx != 1, continue searching.
			continue;
		}
		*start_code_length = 4;
		return (int32_t)(head - buffer);
	}
	return -1;
}

tsk_boolean_t tdav_codec_h264_get_fragmentationHeader_by_payload(youme::webrtc::RTPFragmentationHeader* header, uint8_t *payload, int32_t payload_size)
{
	// For H.264 search for start codes.
	int32_t scPositions[MAX_NALUS_PERFRAME + 1] = {};
	int32_t scStartCode_lengths[MAX_NALUS_PERFRAME + 1] = {};
	int32_t scPositionsLength = 0;
	int32_t scPosition = 0;
	int last_type = 0; // unknown
	int current_type = 0;
	while (scPositionsLength < MAX_NALUS_PERFRAME) {
		size_t start_code_length = 0;
		int32_t naluPosition = tdav_codec_h264_get_nextNaluPosition(
			payload + scPosition, payload_size - scPosition, &start_code_length);
		if (naluPosition < 0) {
			break;
		}
		current_type = *(payload + scPosition + naluPosition + start_code_length) & 0x1F;
		if (current_type == H264_NAL_AUD || (last_type == 0 && current_type == H264_NAL_SEI)) {
			// 适配qsv编码的两特点：1. 每个nalu都有AUD 2. 每个P帧前都会有SEI
			scPosition += start_code_length;
			continue;
		}
		scPosition += naluPosition;
		scStartCode_lengths[scPositionsLength] = start_code_length;
		scPositions[scPositionsLength++] = scPosition;
		scPosition += start_code_length;
		last_type = current_type;
	}
	if (scPositionsLength == 0) {
		TSK_DEBUG_ERROR("Start code is not found!");
		return tsk_false;
	}
	scPositions[scPositionsLength] = payload_size;
	header->VerifyAndAllocateFragmentationHeader(scPositionsLength);
	for (size_t i = 0; i < scPositionsLength; i++) {
		// size_t lentemp = scStartCode_lengths[i];
		header->fragmentationOffset[i] = scPositions[i] + scStartCode_lengths[i];
		header->fragmentationLength[i] =
			scPositions[i + 1] - header->fragmentationOffset[i];
		header->fragmentationPlType[i] = 0;
		header->fragmentationTimeDiff[i] = 0;
	}
	return tsk_true;
}

bool tdav_check_video_fps(h264_encoder_manager_t * video_enc_mgr, int64_t timestamp)
{

#if 0  // use extern video timestamp instead of extern timestamp
	if (!timestamp) {
		TSK_DEBUG_INFO("@@ doCheckVideoTimestampMobile input invalid");
		return true;
	}

	// fist step
	// 1. fot first frame, output ts is input ts
	// 2. if caller stop inputframe, reset varible when inputframe again
	if (!video_enc_mgr->outputVideoTs || (timestamp > video_enc_mgr->lastInputTs + video_enc_mgr->inputInterval * 2)) {
		video_enc_mgr->lastInputTs = timestamp;
		video_enc_mgr->outputVideoTs = timestamp + video_enc_mgr->outputInterval;
		video_enc_mgr->inputInterval = 0;
		return true;
	}

	// sencond call, m_delatIOOffset = |m_outputInterval - m_inputIntervalMobile|
	// Assume:
	//      1. that the input frame rate is fixed
	//      2. input fps > output fps
	//      3. timestamp is increasing

	if (video_enc_mgr->inputInterval) {
		video_enc_mgr->inputInterval = (timestamp - video_enc_mgr->lastInputTs)*0.2 + video_enc_mgr->inputInterval*0.8;
	}
	else {
		video_enc_mgr->inputInterval = (timestamp - video_enc_mgr->lastInputTs);
	}

	int m_delatIOOffset = abs(video_enc_mgr->outputInterval - video_enc_mgr->inputInterval);
	video_enc_mgr->lastInputTs = timestamp;

	// If the input and output frame rate intervals are close,
	// the frame rate is considered to be the same
	if (m_delatIOOffset <= 10 || video_enc_mgr->inputInterval > video_enc_mgr->outputInterval + 10) {
		video_enc_mgr->outputVideoTs += video_enc_mgr->outputInterval;
		return true;
	}

	int deltaInputIntrval = video_enc_mgr->outputVideoTs > timestamp
		? (video_enc_mgr->outputVideoTs - timestamp)
		: (timestamp - video_enc_mgr->outputVideoTs);

	TSK_DEBUG_INFO("@@ doCheckVideoTimestampMobile deltaInputIntrval:%d, m_delatIOOffset:%d", deltaInputIntrval, m_delatIOOffset);
	if (deltaInputIntrval < m_delatIOOffset) {
		video_enc_mgr->outputVideoTs += video_enc_mgr->outputInterval;
		return true;
	}
	else {
		return false;
	}
#else 
	// sanity check
	if (!video_enc_mgr) {
		return false;
	}

	// use local timestamp to check video fps
	bool is_drop = true;
	int video_id = video_enc_mgr->video_id;
	int fps = tdav_codec_h264_fps_base;

	if (tdav_video_main == video_id) {
		fps = tmedia_defaults_get_video_fps();
	} else if (tdav_video_child == video_id) {
		fps = tmedia_defaults_get_video_fps_child();
	} else {
		fps = tmedia_defaults_get_video_fps_share();
	}
	
	uint64_t newTime = tsk_time_now();
	if ((newTime - video_enc_mgr->outputVideoTs + 5) / (1000.0f / fps) >= video_enc_mgr->frameCount && video_enc_mgr->frameCount < fps)
	{
		video_enc_mgr->frameCount++;
		is_drop = false;
	}

	if (newTime - video_enc_mgr->outputVideoTs >= 1000)
	{
		video_enc_mgr->outputVideoTs = newTime;
		video_enc_mgr->frameCount = 0;
	}
	return is_drop;
#endif    
}

static tsk_size_t tdav_codec_h264_encode(tmedia_codec_t* self, const void* in_data, tsk_size_t in_size, void** out_data, tsk_size_t* out_max_size, int64_t timestamp, int video_id, tsk_bool_t force_key_frame, const void* context)
{
	int ret = 0;
	int k;
	int out_size = 0;

#if HAVE_FFMPEG
	int size;
	tsk_bool_t send_idr, send_hdr;
#endif

	tdav_codec_h264_t* h264 = (tdav_codec_h264_t*)self;
	if (!self || !in_data || !out_data) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return 0;
	}

	if (!self->opened) {
		TSK_DEBUG_ERROR("Codec not opened");
		return 0;
	}

	h264_encoder_manager_map_t::iterator iter = h264->encoder.video_enc_map->find(video_id);
	if (iter == h264->encoder.video_enc_map->end()) {
		TMEDIA_I_AM_ACTIVE_ERROR(100, "not found encoder video_id:%d", video_id);
		return -1;
	}

	h264_encoder_manager_t * video_enc_mgr = iter->second;
	c_mutex _mutex(video_enc_mgr->mutex);

    // 当采集(包括外部输入)帧率大于编码帧率时，根据编码帧率做视频抽帧处理
	if (tdav_check_video_fps(video_enc_mgr, timestamp)) {
		return 0;
	}

	// 编码参数检查，编码参数变化时重启编码器
	if (tdav_codec_h264_encode_check_param(self, video_enc_mgr, context) != 0)
		return 0;

	AddVideoFrame(1 , 0 ); //0表示自己吧

	// 在弱网环境下，帧率下调时，需要对视频帧做丢帧处理
	if (tdav_codec_h264_fps_lose(video_enc_mgr) != 0) {
		return 0;
	}

   if(force_key_frame){
        video_enc_mgr->force_idr = true;
    }
	int size = video_enc_mgr->width * video_enc_mgr->height;
	tsk_bool_t switch2SwCoder = tsk_false; // 目前仅针对Android判断
#ifdef ANDROID
	switch2SwCoder = ((video_enc_mgr->width % 16) || (video_enc_mgr->height % 16));
#endif

	if (h264->encoder.hw_encode_enable && video_enc_mgr->webrtc_hwcodec_issupport) {
#if HARDWARE_CODECS
		youme::webrtc::CodecSpecificInfo codec_info;
		std::vector<youme::webrtc::FrameType> next_frame_types(1, youme::webrtc::kVideoFrameDelta);
		if (video_enc_mgr->force_idr) {
			video_enc_mgr->force_idr = false;
			next_frame_types[0] = youme::webrtc::kVideoFrameKey;
		}
		if (in_size == 0) {
			rtc::scoped_refptr<youme::webrtc::VideoFrameBuffer> buffer =
				new rtc::RefCountedObject<youme::webrtc::NativeHandleBuffer>((void*)in_data, video_enc_mgr->width, video_enc_mgr->height);
			youme::webrtc::VideoFrame video_frame(buffer, timestamp, timestamp, youme::webrtc::kVideoRotation_0);
			ret = video_enc_mgr->webrtc_codec->Encode(video_frame, &codec_info, &next_frame_types);
		}
		else {
			if (switch2SwCoder) {
				TSK_DEBUG_INFO("resolution not meet(16x16) switch to ---> sw encode\n");
				ret = WEBRTC_VIDEO_CODEC_FALLBACK_SOFTWARE;
			} else {
				rtc::scoped_refptr<youme::webrtc::VideoFrameBuffer> buffer =
					new rtc::RefCountedObject<youme::webrtc::I420Buffer>(video_enc_mgr->width, video_enc_mgr->height);
				memcpy((uint8_t *)buffer->MutableData(youme::webrtc::kYPlane), (uint8_t *)in_data, size);
				memcpy((uint8_t *)buffer->MutableData(youme::webrtc::kUPlane), (uint8_t *)in_data + size, size / 4);
				memcpy((uint8_t *)buffer->MutableData(youme::webrtc::kVPlane), (uint8_t *)in_data + size * 5 / 4, size / 4);
				youme::webrtc::VideoFrame video_frame(buffer, timestamp, timestamp, youme::webrtc::kVideoRotation_0);
				ret = video_enc_mgr->webrtc_codec->Encode(video_frame, &codec_info, &next_frame_types);
			}
		}
		if (ret == WEBRTC_VIDEO_CODEC_FALLBACK_SOFTWARE ||
			ret == WEBRTC_VIDEO_CODEC_UNINITIALIZED)
		{

#ifdef ANDROID
			TSK_DEBUG_INFO("h264 hw encode fail ---> sw encode\n");
			//切换软编码
			video_enc_mgr->webrtc_codec->Release();
			delete video_enc_mgr->webrtc_codec;
			video_enc_mgr->webrtc_codec = NULL;
			delete video_enc_mgr->callback;
			video_enc_mgr->callback = NULL;
			video_enc_mgr->webrtc_hwcodec_issupport = false;
			h264->encoder.hw_encode_force_no = true;
			if (tdav_codec_h264_open_sw_encoder(h264, video_enc_mgr) != 0) {
				return 0;
			}
#else
			tdav_codec_h264_reset_encoder(self, video_enc_mgr, context);
			return 0;
#endif
		}
		else {
			return ret;
		}
#if H264_DATA_DUMP
		//fwrite((uint8_t *)*out_data + out_size, 1, h264->encoder.pNals[k].i_payload, h264->encoder.pfWrite);
#endif
#endif
	}
    
	// 重启编码器，强制输出I帧
	if (force_key_frame) {
      //  tdav_codec_h264_reset_encoder(self, video_enc_mgr, context);
    }

    int video_share_tpye = tmedia_get_video_share_type();
	if (h264->encoder.passthrough) {
#if FFMPEG_SUPPORT
#if !USE_FFMPEG_ENCODE

		{
			// default is YUV420
			if (size != (in_size * 2 / 3)) {
				TSK_DEBUG_ERROR("[H.264] Encoder size not match! size[%d] in_size[%d]", size, in_size);
				if (in_size == 0) //oepngl codec
					return 1;
				return 0;
			}
			memcpy((uint8_t *)(video_enc_mgr->pPicIn->img.plane[0]), (uint8_t *)in_data, size);
			memcpy((uint8_t *)(video_enc_mgr->pPicIn->img.plane[1]), (uint8_t *)in_data + size, size / 4);
			memcpy((uint8_t *)(video_enc_mgr->pPicIn->img.plane[2]), (uint8_t *)in_data + size * 5 / 4, size / 4);

			video_enc_mgr->pPicIn->i_pts = video_enc_mgr->i8FrameNum;
			video_enc_mgr->i8FrameNum++;
			ret = x264_encoder_encode(video_enc_mgr->pX264Handle,
				&(video_enc_mgr->pNals),
				&(video_enc_mgr->i4Nal),
				video_enc_mgr->pPicIn,
				video_enc_mgr->pPicOut);

			if (ret == 0) {
				int frames = x264_encoder_delayed_frames(video_enc_mgr->pX264Handle);
				TSK_DEBUG_INFO("x264_encoder_encode delayed frames:%d", frames);
				return 0;
			}
			if (ret < 0) {
				TSK_DEBUG_ERROR("x264_encoder_encode NOT OK: ret=%d", ret);
				return 0;
			}
			if (*out_max_size < in_size / 2) {
				if (!(*out_data = tsk_realloc(*out_data, in_size))) {
					TSK_DEBUG_ERROR("[H.264] Failed to allocate buffer with size = %zu", in_size);
					*out_max_size = 0;
					return 0;
				}
				*out_max_size = in_size;
			}

			uint8_t *_temp_buf = NULL;
			int start_len = 0, nalu_type = 0;
			int payload_size_left = 1024; // slice startcode len
			int payload_offset = 0;

			if (video_id == tdav_video_share)
			{
				tdav_session_av_t* base = (tdav_session_av_t*)context;
				int sessionId = base->producer->session_id;

				auto pRecorder = YMVideoRecorderManager::getInstance()->getRecorderBySession(sessionId);
				if (pRecorder)
				{
					if (video_enc_mgr->i4Nal >= 3)//起码包含sps,pps,和IDR数据
					{
						//时间戳单位是ms
						pRecorder->setVideoInfo(video_enc_mgr->width, video_enc_mgr->height, video_enc_mgr->fps, video_enc_mgr->max_bw_kpbs, 1000);

						YMVideoRecorderManager::getInstance()->setSpsAndPps(sessionId, video_enc_mgr->pNals[0].p_payload, video_enc_mgr->pNals[0].i_payload,
							video_enc_mgr->pNals[1].p_payload, video_enc_mgr->pNals[1].i_payload);

						for (int index = 2; index < video_enc_mgr->i4Nal; index++)
						{
							tdav_codec_h264_get_nalu_type(video_enc_mgr->pNals[index].p_payload, &nalu_type, &start_len);
							if (nalu_type == H264_NAL_SLICE_IDR)
							{
								YMVideoRecorderManager::getInstance()->inputVideoData(sessionId, video_enc_mgr->pNals[index].p_payload, video_enc_mgr->pNals[index].i_payload, timestamp, H264_NAL_SPS);
							}
						}
					}
					else {
						tdav_codec_h264_get_nalu_type(video_enc_mgr->pNals[0].p_payload, &nalu_type, &start_len);
						YMVideoRecorderManager::getInstance()->inputVideoData(sessionId, video_enc_mgr->pNals[0].p_payload, video_enc_mgr->pNals[0].i_payload, timestamp, nalu_type);
					}
				}
			}

			for (k = 0; k < video_enc_mgr->i4Nal; k++) {
				//            TSK_DEBUG_ERROR ("[X264] encode nal[%d] payload len[%d]", k, video_enc_mgr->pNals[k].i_payload);
				//            TSK_DEBUG_ERROR ("[X264] payload[0x%2x %2x %2x %2x %2x]", video_enc_mgr->pNals[k].p_payload[0], video_enc_mgr->pNals[k].p_payload[1], video_enc_mgr->pNals[k].p_payload[2], video_enc_mgr->pNals[k].p_payload[3], video_enc_mgr->pNals[k].p_payload[4]);
#if H264_DATA_DUMP
                if (video_enc_mgr->pfWrite)
                {
                    if (video_enc_mgr->file_size > video_enc_mgr->max_size) {
                        tdav_codec_h264_create_encode_dump_file(video_enc_mgr, video_id, false);
                    }
                    
                    if (video_enc_mgr->pfWrite) {
                        fwrite(video_enc_mgr->pNals[k].p_payload, 1, video_enc_mgr->pNals[k].i_payload, video_enc_mgr->pfWrite);
                        video_enc_mgr->file_size += video_enc_mgr->pNals[k].i_payload;
                    }
                }
#endif

				// packet SPS/PPS/SEI into STAP-A
				tdav_codec_h264_get_nalu_type(video_enc_mgr->pNals[k].p_payload, &nalu_type, &start_len);
				if (nalu_type == H264_NAL_SPS || nalu_type == H264_NAL_PPS || nalu_type == H264_NAL_SEI) {
					if (k == 0) {
						_temp_buf = (uint8_t *)malloc(1024); // mtu size

						_temp_buf[payload_offset] = 0x0;
						_temp_buf[payload_offset + 1] = 0x0;
						_temp_buf[payload_offset + 2] = 0x0;
						_temp_buf[payload_offset + 3] = 0x1;
						payload_offset += 4;
						_temp_buf[payload_offset] = 0x58;    // stap-a header
						payload_offset++;
					}

					if (payload_size_left - payload_offset > video_enc_mgr->pNals[k].i_payload - start_len) {
						_temp_buf[payload_offset] = ((video_enc_mgr->pNals[k].i_payload - start_len) & 0xFF00) >> 8;   // nalu1 size
						_temp_buf[payload_offset + 1] = (video_enc_mgr->pNals[k].i_payload - start_len) & 0xFF;
						payload_offset += 2;
						memcpy(_temp_buf + payload_offset, video_enc_mgr->pNals[k].p_payload + start_len, video_enc_mgr->pNals[k].i_payload - start_len);
						payload_offset += video_enc_mgr->pNals[k].i_payload - start_len;
					}

					int next_nalu_type = 0, next_start_len = 0;
					tdav_codec_h264_get_nalu_type(video_enc_mgr->pNals[k + 1].p_payload, &next_nalu_type, &next_start_len);
					if (next_nalu_type == 5 || next_nalu_type == 1) {
						(*_gSessionEncodeCallback)(_temp_buf, payload_offset, timestamp, video_id, context);
					}

					if (_temp_buf) {
						free(_temp_buf);
						_temp_buf = NULL;
					}
				}
				else {
					memcpy((uint8_t *)*out_data + out_size, video_enc_mgr->pNals[k].p_payload, video_enc_mgr->pNals[k].i_payload);
					out_size += video_enc_mgr->pNals[k].i_payload;
				}
			}
			video_enc_mgr->frame_count++;
		}
#else //!USE_FFMPEG_ENCODE
		{
			// default is YUV420
			if (size != (in_size * 2 / 3)) {
				TSK_DEBUG_ERROR("[H.264] Encoder size not match! size[%d] in_size[%d]", size, in_size);
				if (in_size == 0) //oepngl codec
					return 1;
				return 0;
			}

			if (video_enc_mgr->context == NULL) {
				return 0;
			}

			AVFrame *av_frame = av_frame_alloc();
			int picture_size = 0;
			if (!strcmp(video_enc_mgr->context->codec->name, "h264_qsv")) {
				// qsv不支持YUV420P，需要转成NV12
				picture_size = av_image_get_buffer_size(AV_PIX_FMT_NV12, video_enc_mgr->width, video_enc_mgr->height, 1);

				av_frame->width = video_enc_mgr->width;
				av_frame->height = video_enc_mgr->height;
				av_frame->format = AV_PIX_FMT_NV12;

				int yLen = av_frame->width * av_frame->height;
				int uLen = yLen / 4;

				uint8_t *nv12_uvData = (uint8_t *)av_malloc(2 * uLen);
				uint8_t *i420_uData = (uint8_t *)in_data + yLen;
				uint8_t *i420_vData = (uint8_t *)in_data + yLen + uLen;

				int index = 0;
				for (index = 0; index < uLen; index++) {
					*(nv12_uvData + 2 * index) = *(i420_uData + index);
					*(nv12_uvData + 2 * index + 1) = *(i420_vData + index);
				}

				memcpy((uint8_t *)in_data + yLen, (uint8_t *)nv12_uvData, 2 * uLen);

				av_image_fill_arrays(av_frame->data,
					av_frame->linesize,
					(uint8_t *)in_data,
					video_enc_mgr->context->pix_fmt,
					video_enc_mgr->context->width,
					video_enc_mgr->context->height,
					1);
				av_frame->pts = av_frame->pkt_dts = video_enc_mgr->i8FrameNum;
				av_free(nv12_uvData);
				nv12_uvData = NULL;
			}
			else {
				picture_size = av_image_get_buffer_size(video_enc_mgr->context->pix_fmt, video_enc_mgr->width, video_enc_mgr->height, 1);

				av_frame->width = video_enc_mgr->width;
				av_frame->height = video_enc_mgr->height;
				av_frame->format = AV_PIX_FMT_YUV420P;
				av_image_fill_arrays(av_frame->data,
					av_frame->linesize,
					(uint8_t *)in_data,
					video_enc_mgr->context->pix_fmt,
					video_enc_mgr->context->width,
					video_enc_mgr->context->height,
					1);
				av_frame->pts = av_frame->pkt_dts = video_enc_mgr->i8FrameNum;
			}

			// 强制输出I帧
			if (video_enc_mgr->force_idr) {
                av_frame->pict_type = AV_PICTURE_TYPE_I;
                av_frame->key_frame = 1;
                video_enc_mgr->force_idr = false;
			}
             

			AVPacket av_packet;
			av_new_packet(&av_packet, picture_size);

			video_enc_mgr->i8FrameNum++;
			int got_packet = 0;
			int ret = avcodec_encode_video2(video_enc_mgr->context, &av_packet, av_frame, &got_packet);
			if (ret < 0) {
#ifdef WIN32
				// close h264_qsv encoder
				if (strcmp(video_enc_mgr->context->codec->name, "libx264")) {
					TSK_DEBUG_INFO("h264_qsv hw accelerate encode fail ---> sw encode\n");
					// close h264_qsv encoder
					if (video_enc_mgr->context) {
						avcodec_close(video_enc_mgr->context);
						av_free(video_enc_mgr->context);
						video_enc_mgr->context = tsk_null;
					}

					if (tdav_codec_h264_open_sw_encoder(h264, video_enc_mgr, false) != 0) {
						TSK_DEBUG_INFO("[H.264] Failed create new sw encoder fail");
						return -1;
					}
				}
#endif
				TSK_DEBUG_ERROR("x264_encoder_encode NOT OK: ret=%d", ret);
				av_free_packet(&av_packet);
				av_frame_free(&av_frame);
				return 0;
			}
            
			if (*out_max_size < in_size / 2) {
				if (!(*out_data = tsk_realloc(*out_data, in_size))) {
					TSK_DEBUG_ERROR("[H.264] Failed to allocate buffer with size = %zu", in_size);
					*out_max_size = 0;
					av_free_packet(&av_packet);
					av_frame_free(&av_frame);
					return 0;
				}
				*out_max_size = in_size;
			}

			if (got_packet == 1) {
				youme::webrtc::RTPFragmentationHeader header;
				memset(&header, 0, sizeof(header));
				tdav_codec_h264_get_fragmentationHeader_by_payload(&header, av_packet.data, av_packet.size);
				int nalu_type = 0;
#if H264_DATA_DUMP
                if (video_enc_mgr->pfWrite)
                {
                    if (video_enc_mgr->file_size > video_enc_mgr->max_size) {
                        tdav_codec_h264_create_encode_dump_file(video_enc_mgr, video_id, false);
                    }
                    
                    if (video_enc_mgr->pfWrite) {
                        fwrite(av_packet.data, 1, av_packet.size, video_enc_mgr->pfWrite);
                        video_enc_mgr->file_size += av_packet.size;
                    }
                }
#endif

				// for save share video stream to file
				if (video_id == tdav_video_share && (video_share_tpye & ENABLE_SAVE))
				{
					tdav_session_av_t* base = (tdav_session_av_t*)context;
					int sessionId = base->producer->session_id;

					auto pRecorder = YMVideoRecorderManager::getInstance()->getRecorderBySession(sessionId);
					if (pRecorder)
					{
                        uint8_t* avcc_sps = nullptr;
                        int avcc_sps_size = 0;
                        uint8_t* avcc_pps = nullptr;
                        int avcc_pps_size = 0;
                        uint8_t* avcc_idr = nullptr;
                        int avcc_idr_size = 0;
						if (header.fragmentationVectorSize >= 3)//起码包含sps,pps,和IDR数据
						{
							//时间戳单位是ms
							YMVideoRecorderManager::getInstance()->setVideoInfo(sessionId, video_enc_mgr->width, video_enc_mgr->height, video_enc_mgr->fps, video_enc_mgr->max_bw_kpbs, 1000);
                            YMVideoRecorderManager::getInstance()->annexbToAvcc(av_packet.data + header.fragmentationOffset[0], header.fragmentationLength[0], &avcc_sps, &avcc_sps_size);
                            YMVideoRecorderManager::getInstance()->annexbToAvcc(av_packet.data + header.fragmentationOffset[1], header.fragmentationLength[1], &avcc_pps, &avcc_pps_size);
							YMVideoRecorderManager::getInstance()->setSpsAndPps(sessionId, avcc_sps, avcc_sps_size, avcc_pps, avcc_pps_size);

							for (int index = 2; index < header.fragmentationVectorSize; index++)
							{
								uint8_t* tempbuffer = av_packet.data + header.fragmentationOffset[index];
								nalu_type = tempbuffer[0] & 0x1F;
								if (nalu_type == H264_NAL_SLICE_IDR)
								{
                                    YMVideoRecorderManager::getInstance()->annexbToAvcc(av_packet.data + header.fragmentationOffset[index], header.fragmentationLength[index], &avcc_idr, &avcc_idr_size);
									YMVideoRecorderManager::getInstance()->inputVideoData(sessionId, avcc_idr, avcc_idr_size, timestamp, H264_NAL_SPS);
								}
							}
						}
						else {
							uint8_t* tempbuffer = av_packet.data + header.fragmentationOffset[0];
							nalu_type = tempbuffer[0] & 0x1F;
                            uint8_t* non_idr = nullptr;
                            int non_idr_size = 0;
                            YMVideoRecorderManager::getInstance()->annexbToAvcc(av_packet.data + header.fragmentationOffset[0], header.fragmentationLength[0], &non_idr, &non_idr_size);
							YMVideoRecorderManager::getInstance()->inputVideoData(sessionId, non_idr, non_idr_size, timestamp, nalu_type);
						}
					}  
				}

                //大小流和共享流发送逻辑处理
                if (video_id != tdav_video_share || video_share_tpye & ENABLE_SHARE) {
                    int k = 0;
    				uint8_t *_temp_buf = (uint8_t *)malloc(1024); // mtu size
    				int payload_size_left = 1024; // slice startcode len
    				int payload_offset = 0;
    				for (k = 0; k < header.fragmentationVectorSize; k++) {
    					uint8_t* nal_buffer_without_startcode = av_packet.data + header.fragmentationOffset[k];
    					// TSK_DEBUG_ERROR ("[FFmpeg] encode nal[%d] payload len[%d]", k, header.fragmentationLength[k]);
    					// TSK_DEBUG_ERROR ("[FFmpeg] payload[0x%2x %2x %2x %2x %2x]", nal_buffer[0], nal_buffer[1], nal_buffer[2], nal_buffer[3], nal_buffer[4]);
    					// packet SPS/PPS/SEI into STAP-A
    					nalu_type = nal_buffer_without_startcode[0] & 0x1F;
    					if (nalu_type == H264_NAL_SPS || nalu_type == H264_NAL_PPS || nalu_type == H264_NAL_SEI) {
    						if (k == 0) {
    							_temp_buf[payload_offset] = 0x0;
    							_temp_buf[payload_offset + 1] = 0x0;
    							_temp_buf[payload_offset + 2] = 0x0;
    							_temp_buf[payload_offset + 3] = 0x1;
    							payload_offset += 4;
    							_temp_buf[payload_offset] = 0x58;    // stap-a header
    							payload_offset++;
    						}

    						if (payload_size_left - payload_offset > header.fragmentationLength[k]) {
    							_temp_buf[payload_offset] = ((header.fragmentationLength[k]) & 0xFF00) >> 8;   // nalu1 size
    							_temp_buf[payload_offset + 1] = (header.fragmentationLength[k]) & 0xFF;
    							payload_offset += 2;
    							memcpy(_temp_buf + payload_offset, nal_buffer_without_startcode, header.fragmentationLength[k]);
    							payload_offset += header.fragmentationLength[k];
    						}

    						uint8_t* nal_buffer_next = av_packet.data + header.fragmentationOffset[k + 1];
    						int next_nalu_next_type = nal_buffer_next[0] & 0x1F;
    						if (next_nalu_next_type == 5 || next_nalu_next_type == 1) {
    							(*_gSessionEncodeCallback)(_temp_buf, payload_offset, timestamp, video_id, context);
    						}
    					}
    					else {
    						memcpy((uint8_t *)*out_data + out_size, H264_START_CODE_PREFIX, 4);
    						out_size += 4;
    						memcpy((uint8_t *)*out_data + out_size, nal_buffer_without_startcode, header.fragmentationLength[k]);
    						out_size += header.fragmentationLength[k];
    					}
    				}
    				video_enc_mgr->frame_count++;
                    
                    if (_temp_buf) {
    					free(_temp_buf);
    					_temp_buf = NULL;
				    }
                }
			}

			av_free_packet(&av_packet);
			av_frame_free(&av_frame);
			av_frame = NULL;
		}
#endif //!USE_FFMPEG_ENCODE
#endif //FFMPEG_SUPPORT
       return out_size;
    }

    return 0;
}

static int tdav_codec_h264_creat_sw_decoder(tdav_codec_h264_t *self, h264_decoder_manager_t *video_dec_mgr, bool use_hwaccel)
{
	static bool force_downgrade_to_x264decode = false;
	int width = TMEDIA_CODEC_VIDEO(self)->in.width;
	int height = TMEDIA_CODEC_VIDEO(self)->in.height;
	int ret = -1;
#if FFMPEG_SUPPORT
	video_dec_mgr->context = avcodec_alloc_context3(NULL);
	video_dec_mgr->context->codec_id = AV_CODEC_ID_H264;
	video_dec_mgr->context->codec_type = AVMEDIA_TYPE_VIDEO;
	video_dec_mgr->context->pix_fmt = AV_PIX_FMT_YUV420P;
	video_dec_mgr->context->flags2 |= CODEC_FLAG2_FAST;
	video_dec_mgr->context->width = width;
	video_dec_mgr->context->height = height;

	// Picture (YUV 420)
	if (!(video_dec_mgr->picture = /*avcodec_alloc_frame()*/av_frame_alloc())) {
		TSK_DEBUG_ERROR("[H.264] Failed to create decoder picture");
		return -1;
	}
	//avcodec_get_frame_defaults(video_dec_mgr->picture);
	av_frame_unref(video_dec_mgr->picture);

	AVDictionary *param = NULL;
	// 适配qsv编码
	av_dict_set(&param, "async_depth", "0", 0);

#ifdef WIN32
	bool use_h264 = force_downgrade_to_x264decode ? true : false;
	// 目前硬解码还没有测试通过，还需测试硬解码延迟，测试通过可以注释掉这行
	use_h264 = true;

	std::string codec_name = "h264";
	if (use_hwaccel && (!use_h264)) {
		Windows_GPU_Series gpu = tdav_codec_h264_get_windows_gpu();
		switch (gpu)
		{
		case WINDOWS_GPU_NONE:
			codec_name = "h264";
			use_h264 = true;
			break;
		case WINDOWS_GPU_INTEL:
			video_dec_mgr->context->pix_fmt = AV_PIX_FMT_NV12;
			codec_name = "h264_qsv";
			break;
		case WINDOWS_GPU_NVDIA:
			codec_name = "h264_cuvid";
			break;
		case WINDOWS_GPU_AMD:
			codec_name = "h264_amf";
			break;
		default:
			break;
		}
	}
	else {
		use_h264 = true;
	}

	if (!use_h264) {
		video_dec_mgr->codec = avcodec_find_decoder_by_name(codec_name.c_str());
		if (!video_dec_mgr->codec) {
			TSK_DEBUG_WARN("tdav codec decoder cannot find %s, switch to h264", codec_name.c_str());
			force_downgrade_to_x264decode = true;
		}
		else {
			ret = avcodec_open2(video_dec_mgr->context, video_dec_mgr->codec, &param);
			if (ret < 0) {
				TSK_DEBUG_WARN("tdav codec decoder open %s failed, switch to h264", codec_name.c_str());
				force_downgrade_to_x264decode = true;
			}
			else {
				TSK_DEBUG_INFO("tdav codec open decoder %s success !!", video_dec_mgr->codec->name);
				force_downgrade_to_x264decode = true;
				video_dec_mgr->last_frame_time = tsk_time_now();
				av_dict_free(&param);
				param = tsk_null;
				return 0;
			}
		}
	}
#endif
	video_dec_mgr->context->pix_fmt = AV_PIX_FMT_YUV420P; // use h264 software decode
	video_dec_mgr->codec = avcodec_find_decoder_by_name("h264");
	if (!video_dec_mgr->codec) {
		TSK_DEBUG_WARN("tdav codec decoder cannot find h264");
		av_dict_free(&param);
		param = tsk_null;
		return -1;
	}
	ret = avcodec_open2(video_dec_mgr->context, video_dec_mgr->codec, &param);
	if (ret < 0) {
		if (video_dec_mgr->context) {
			avcodec_close(video_dec_mgr->context);
			av_free(video_dec_mgr->context);
			video_dec_mgr->context = tsk_null;
		}
		if (video_dec_mgr->picture) {
			av_frame_free(&video_dec_mgr->picture);
			video_dec_mgr->picture = tsk_null;
		}
		av_dict_free(&param);
		param = tsk_null;
		return -1;
	}
	else {
		TSK_DEBUG_INFO("tdav codec open decoder %s success !!", video_dec_mgr->codec->name);
		video_dec_mgr->last_frame_time = tsk_time_now();
		av_dict_free(&param);
		param = tsk_null;
		return 0;
	}
#endif
	return -1;
}

static int tdav_codec_h264_creat_hw_decoder(tdav_codec_h264_t *self, h264_decoder_manager_t *video_dec_mgr, const void* context)
{
	int width = TMEDIA_CODEC_VIDEO(self)->in.width;
	int height = TMEDIA_CODEC_VIDEO(self)->in.height;
#if HARDWARE_CODECS
	if (tmedia_defaults_get_video_hwcodec_enabled() &&
		!self->decoder.hw_decode_force_no &&
		self->decoder.hw_decode_enable &&
		tdav_codec_h264_hardware_is_supported(width, height) &&
		youme::webrtc::H264Decoder::IsSupported()) {

		//create hw codecs
		tsk_bool_t video_frame_raw_enabled = tmedia_defaults_get_video_frame_raw_cb_enabled();
		tsk_bool_t video_decode_raw_enabled = tmedia_defaults_get_video_decode_raw_cb_enabled();
		if (!video_frame_raw_enabled && !video_decode_raw_enabled) {
			youme::webrtc::H264Decoder::SetEGLContext(tmedia_get_video_egl_context());
		}

		youme::webrtc::VideoDecoder* codec = youme::webrtc::H264Decoder::Create();
		youme::webrtc::VideoCodec codecSetting;
		codecSetting.codecType = youme::webrtc::kVideoCodecH264;
		codecSetting.width = width;
		codecSetting.height = height;
		codecSetting.maxFramerate = tdav_codec_h264_fps_base;
		if (codec->InitDecode(&codecSetting, 1, true) == 0) {
			video_dec_mgr->webrtc_codec = codec;
			video_dec_mgr->callback = new WebRtcDecodedImageCallback();
			video_dec_mgr->callback->_session = (void*)context;
			codec->RegisterDecodeCompleteCallback(video_dec_mgr->callback);
			video_dec_mgr->webrtc_hwcodec_issupport = true;
			if (!video_frame_raw_enabled && !video_decode_raw_enabled)
				video_dec_mgr->texture_cb = (tmedia_get_video_egl_context() != nullptr);
			video_dec_mgr->last_frame_time = tsk_time_now();
			return 0;
		}
		else {
			codec->Release();
			delete codec;
		}
	}
#endif

	return -1;
}


static int tdav_codec_h264_creat_new_decoder(tdav_codec_h264_t *self, int32_t session_id, const void* context, int video_id)
{
	TSK_DEBUG_INFO("tdav_codec_h264_creat_new_decoder sessionid:%d\n", session_id);
	h264_decoder_manager_t *video_dec_mgr;
	h264_decoder_manager_map_t::iterator iter = self->decoder.video_dec_map->find(session_id);
	if (iter != self->decoder.video_dec_map->end())
	{
		return 0;
	}

	video_dec_mgr = new h264_decoder_manager_t;
	if (NULL == video_dec_mgr) {
		return -1;
	}
	// Initial
	memset(video_dec_mgr, 0, sizeof(h264_decoder_manager_t));
	video_dec_mgr->accumulator = tsk_null;
	video_dec_mgr->last_seq = 0;
	video_dec_mgr->last_frame_last_pkt_seqnum = 0;
	video_dec_mgr->last_seqnum_with_mark = 0;
	video_dec_mgr->isCurFrameCrash = tsk_false;
	video_dec_mgr->isCurGopCrash = tsk_false;
	video_dec_mgr->pkt_num = 0;
	video_dec_mgr->init_idr = false;
	video_dec_mgr->video_id = video_id;
	video_dec_mgr->width = 0;
	video_dec_mgr->height = 0;
	video_dec_mgr->last_frame_time = 0;
	video_dec_mgr->pfWrite = nullptr;
	video_dec_mgr->file_size = 0;
	video_dec_mgr->max_size = 0;

	do {
		// if predecode_need_decode_and_render is false, no need to create video decoder
		if (!self->decoder.predecode_need_decode_and_render) {
			TSK_DEBUG_INFO("h264_create_decoder, no need to create video decoder");
			break;
		}

		if (!tdav_codec_h264_creat_hw_decoder(self, video_dec_mgr, context)) {
			break;
		}

		TSK_DEBUG_ERROR("tdav_codec_h264_creat_hw_decoder fail, switch to sw decoder");
		if (!tdav_codec_h264_creat_sw_decoder(self, video_dec_mgr)) {
			break;
		}

		TSK_DEBUG_ERROR("tdav_codec_h264_creat_sw_decoder fail--");
		delete video_dec_mgr;
		return -1;
	} while (0);

#if H264_DATA_DUMP
	if (tmedia_defaults_get_h264_file_size_kb() > 0 && !video_dec_mgr->pfWrite) {
		video_dec_mgr->max_size = tmedia_defaults_get_h264_file_size_kb() * 1024;
		if (video_dec_mgr->max_size > 0)
		{
			tdav_codec_h264_create_decode_dump_file(video_dec_mgr, session_id);
		}
		
	}
#endif

	video_dec_mgr->mutex = tsk_mutex_create();
	self->decoder.video_dec_map->insert(h264_decoder_manager_map_t::value_type(session_id, video_dec_mgr));
	return 0;

}

//释放空闲解码器
static void tdav_codec_h264_free_timeout_decoder(tdav_codec_h264_t *self) {
	uint64_t time_now = tsk_time_now();
	h264_decoder_manager_map_t::iterator iter = self->decoder.video_dec_map->begin();
	while (iter != self->decoder.video_dec_map->end()) {
		if (time_now - iter->second->last_frame_time > tdav_codec_h264_decoder_timeout) {
			TSK_DEBUG_INFO("h264 codec timeout free sessionid:%d, now:%lld, last_frame_time:%lld", iter->first, time_now, iter->second->last_frame_time);
			h264_decoder_manager_t *video_dec_mgr = iter->second;
			if (video_dec_mgr) {
				if (video_dec_mgr->mutex)
					tsk_mutex_lock(video_dec_mgr->mutex);
#if FFMPEG_SUPPORT
				if (video_dec_mgr->context) {
					avcodec_close(video_dec_mgr->context);
					av_free(video_dec_mgr->context);
					video_dec_mgr->context = tsk_null;
				}
				if (video_dec_mgr->picture) {
					av_frame_free(&video_dec_mgr->picture);
					video_dec_mgr->picture = tsk_null;
				}
#endif

#if HARDWARE_CODECS
				if (video_dec_mgr->webrtc_codec) {
					video_dec_mgr->webrtc_codec->Release();
					delete video_dec_mgr->webrtc_codec;
					video_dec_mgr->webrtc_codec = NULL;
				}

				if (video_dec_mgr->callback) {
					delete video_dec_mgr->callback;
					video_dec_mgr->callback = NULL;
				}
#endif
				if (video_dec_mgr->accumulator) {
					TSK_FREE(video_dec_mgr->accumulator);
					video_dec_mgr->accumulator = tsk_null;
				}
				video_dec_mgr->accumulator_pos = 0;
				if (video_dec_mgr->mutex) {
					tsk_mutex_unlock(video_dec_mgr->mutex);
					tsk_mutex_destroy(&(video_dec_mgr->mutex));
				}
				delete video_dec_mgr;
				video_dec_mgr = NULL;
			}
			iter = self->decoder.video_dec_map->erase(iter);
			continue;
		}
		++iter;
	}
}

static int tdav_codec_h264_reset_decoder(tdav_codec_h264_t *self, int32_t session_id, const void* context)
{
	TSK_DEBUG_INFO("videocodec tdav_codec_h264_reset_decoder sessionid:%d\n", session_id);
	h264_decoder_manager_t *video_dec_mgr;
	h264_decoder_manager_map_t::iterator iter = self->decoder.video_dec_map->find(session_id);
	if (iter != self->decoder.video_dec_map->end())
	{
		video_dec_mgr = iter->second;
#if FFMPEG_SUPPORT
		if (video_dec_mgr->context) {
			avcodec_close(video_dec_mgr->context);
			av_free(video_dec_mgr->context);
			video_dec_mgr->context = tsk_null;
		}
		if (video_dec_mgr->picture) {
			av_frame_free(&video_dec_mgr->picture);
			video_dec_mgr->picture = tsk_null;
		}
#endif

#if HARDWARE_CODECS
		if (video_dec_mgr->webrtc_codec) {
			video_dec_mgr->webrtc_codec->Release();
			delete video_dec_mgr->webrtc_codec;
			video_dec_mgr->webrtc_codec = NULL;
		}

		if (video_dec_mgr->callback) {
			delete video_dec_mgr->callback;
			video_dec_mgr->callback = NULL;
		}
#endif
	}
	else {
		return -1;
	}

	if (!self->decoder.predecode_need_decode_and_render) {
		TSK_DEBUG_INFO("h264_reset_decoder, no need to create video decoder");
		return 0;
	}

	if (!tdav_codec_h264_creat_hw_decoder(self, video_dec_mgr, context)) {
		return 0;
	}
	TSK_DEBUG_ERROR("tdav_codec_h264_reset_decoder hw fail--");
	if (!tdav_codec_h264_creat_sw_decoder(self, video_dec_mgr)) {
		return 0;
	}
	TSK_DEBUG_ERROR("tdav_codec_h264_reset_decoder sw fail--");
	return -1;
}

//FILE* h264_dump = NULL;

static tsk_size_t tdav_codec_h264_decode(tmedia_codec_t* self, const void* in_data, tsk_size_t in_size, void** out_data, tsk_size_t* out_max_size, tsk_object_t* proto_hdr, const void* context)
{
	tdav_codec_h264_t* h264 = (tdav_codec_h264_t*)self;
	trtp_rtp_header_t* rtp_hdr = (trtp_rtp_header_t *)proto_hdr;
//    tdav_session_video_t* session_video = TDAV_SESSION_VIDEO(context);
    
//    void** out_predecode_data, tsk_size_t* out_predecode_max_size, tsk_size_t* out_predecode_size, tsk_bool_t predecode_need_decode_and_render
    
	const uint8_t* pay_ptr = tsk_null;
	tsk_size_t pay_size = 0;
	int ret = 0;
	tsk_bool_t sps_or_pps, append_scp, end_of_unit;
	tsk_size_t retsize = 0, size_to_copy = 0;
	static const tsk_size_t xmax_size = (3840 * 2160 * 3) >> 3; // >>3 instead of >>1 (not an error)
	static tsk_size_t start_code_prefix_size = sizeof(H264_START_CODE_PREFIX);
	int got_picture_ptr = 0;

	if (!h264 || !in_data || !in_size || !out_data || !rtp_hdr)
	{
		TSK_DEBUG_ERROR("[H.264] Invalid parameter");
		return 0;
	}

	// // TSK_DEBUG_ERROR("tdav_codec_h264_decode videoid[%d] insize[%d] outsize[%d]", rtp_hdr->video_id, in_size, *out_max_size);
	// // for share video stream by mark
	// if (2 == rtp_hdr->video_id) {
	//     rtp_hdr->session_id = -2;   // set share video sessionid to -2
	// }

	h264_decoder_manager_map_t::iterator it = h264->decoder.video_dec_map->find(rtp_hdr->session_id);
	if (it == h264->decoder.video_dec_map->end()) {
		tdav_codec_h264_free_timeout_decoder(h264);
		if (tdav_codec_h264_creat_new_decoder(h264, rtp_hdr->session_id, context, rtp_hdr->video_id) != 0) {

			TSK_DEBUG_ERROR("[H.264] Failed to create h264 decoder for session(%d)", rtp_hdr->session_id);
			return 0;
		}
		it = h264->decoder.video_dec_map->find(rtp_hdr->session_id);
		if (it == h264->decoder.video_dec_map->end()) {
			TSK_DEBUG_ERROR("[H.264] Impossible");
			return 0;
		}
		TSK_DEBUG_INFO("[H.264] Sucessfully created h264 decoder for session(%d)", rtp_hdr->session_id);
	}
	h264_decoder_manager_t* video_dec_mgr = it->second;
	if (video_dec_mgr->webrtc_hwcodec_issupport && video_dec_mgr->texture_cb && !tmedia_get_video_egl_context()) {
		tdav_codec_h264_reset_decoder(h264, rtp_hdr->session_id, context);
	}
    
	c_mutex _mutex(video_dec_mgr->mutex);
	//    if (!video_dec_mgr->codec || !video_dec_mgr->context) {
	//        TSK_DEBUG_ERROR("[H.264] Decoder not ready");
	//    }

	// Check the current frame crash
	/*
	if((((video_dec_mgr->last_seq + 1) != rtp_hdr->seq_num) && (rtp_hdr->seq_num > video_dec_mgr->last_frame_last_pkt_seqnum)) // 1. 序列号不连续
	|| (rtp_hdr->is_last_pkts && !rtp_hdr->marker)) { // 2. 已经是最后的pkt，但是没有携带marker，表示最后一个pkt丢掉了
	TSK_DEBUG_INFO("[H.264] Frame crash since packet loss, seq_num=%d", (video_dec_mgr->last_seq + 1));
	video_dec_mgr->isCurFrameCrash = tsk_true;
	video_dec_mgr->isCurGopCrash   = tsk_true;
	}
	*/
	uint16_t first_packet_seq_num = rtp_hdr->csrc[2] >> 16;
	uint16_t packet_num = rtp_hdr->csrc[2] & 0x0000ffff;
	if (packet_num != 1) {
		if (first_packet_seq_num + (video_dec_mgr->pkt_num++) != rtp_hdr->seq_num) {
			video_dec_mgr->isCurFrameCrash = tsk_true;
			video_dec_mgr->isCurGopCrash = tsk_true;
			video_dec_mgr->pkt_num = 0;
			//TSK_DEBUG_INFO("[H.264] Frame crash since packet loss, seq_num=%d", first_packet_seq_num + video_dec_mgr->pkt_num);
		}
		if (rtp_hdr->is_last_pkts && !rtp_hdr->marker) {
			video_dec_mgr->isCurFrameCrash = tsk_true;
			video_dec_mgr->isCurGopCrash = tsk_true;
			video_dec_mgr->pkt_num = 0;
			//TSK_DEBUG_INFO("[H.264] Frame crash since packet loss, seq_num=%d", first_packet_seq_num + video_dec_mgr->pkt_num);
		}
		if ((video_dec_mgr->pkt_num == packet_num) && rtp_hdr->marker) {
			// 表示是完整的packet list
			video_dec_mgr->pkt_num = 0;
			video_dec_mgr->isCurFrameCrash = tsk_false;
		}
	}

	video_dec_mgr->last_seq = rtp_hdr->seq_num;
	if (rtp_hdr->marker) {
		video_dec_mgr->last_seqnum_with_mark = rtp_hdr->seq_num;
	}

	//video_dec_mgr->info[0][video_dec_mgr->index] = rtp_hdr->seq_num;
	//video_dec_mgr->info[1][video_dec_mgr->index] = rtp_hdr->marker;
	//video_dec_mgr->index ++;

	/* 5.3. NAL Unit Octet Usage
	+---------------+
	|0|1|2|3|4|5|6|7|
	+-+-+-+-+-+-+-+-+
	|F|NRI|  Type   |
	+---------------+
	*/
	if (*((uint8_t*)in_data) & 0x80) {
		TSK_DEBUG_WARN("F=1");
		/* reset accumulator */
		video_dec_mgr->accumulator_pos = 0;
		return 0;
	}

	/* get payload */
	if ((ret = tdav_codec_h264_get_pay(in_data, in_size, (const void**)&pay_ptr, &pay_size, &append_scp, &end_of_unit)) || !pay_ptr || !pay_size) {
		TSK_DEBUG_ERROR("[H.264] Depayloader failed to get H.264 content");
		return 0;
	}

	//append_scp = tsk_true;
	size_to_copy = pay_size + (append_scp ? start_code_prefix_size : 0);
	// whether it's SPS or PPS (append_scp is false for subsequent FUA chuncks)
	sps_or_pps = append_scp && pay_ptr && ((pay_ptr[0] & 0x1F) == 7 || (pay_ptr[0] & 0x1F) == 8);
	if (sps_or_pps) {
		// http://libav-users.943685.n4.nabble.com/Decode-H264-streams-how-to-fill-AVCodecContext-from-SPS-PPS-td2484472.html
		// SPS and PPS should be bundled with IDR
		//TSK_DEBUG_INFO("Receiving SPS or PPS ...to be tied to an IDR");
		video_dec_mgr->isSpsFrame = tsk_true; // mark this frame is sps (I)frame
	}

	// start-accumulator
	if (!video_dec_mgr->accumulator) {
		if (size_to_copy > xmax_size) {
			TSK_DEBUG_ERROR("[H.264] %lu too big to contain valid encoded data. xmax_size=%u", size_to_copy, xmax_size);
			return 0;
		}
		if (!(video_dec_mgr->accumulator = tsk_calloc(size_to_copy, sizeof(uint8_t)))) {
			TSK_DEBUG_ERROR("[H.264] Failed to allocated new buffer");
			return 0;
		}
		video_dec_mgr->accumulator_size = size_to_copy;
	}
	if ((video_dec_mgr->accumulator_pos + size_to_copy) >= xmax_size) {
		TSK_DEBUG_ERROR("[H.264] BufferOverflow");
		video_dec_mgr->accumulator_pos = 0;
		return 0;
	}
	if ((video_dec_mgr->accumulator_pos + size_to_copy) > video_dec_mgr->accumulator_size) {
		if (!(video_dec_mgr->accumulator = tsk_realloc(video_dec_mgr->accumulator, (video_dec_mgr->accumulator_pos + size_to_copy)))) {
			TSK_DEBUG_ERROR("[H.264] Failed to reallocated new buffer");
			video_dec_mgr->accumulator_pos = 0;
			video_dec_mgr->accumulator_size = 0;
			return 0;
		}
		video_dec_mgr->accumulator_size = (video_dec_mgr->accumulator_pos + size_to_copy);
	}

	if (append_scp) {
        // 针对nalu起始码，每帧的开始是4字节，sps和pps是4字节，其余的都是3字节
        if (video_dec_mgr->accumulator_pos) {
            memcpy(&((uint8_t*)video_dec_mgr->accumulator)[video_dec_mgr->accumulator_pos], H264_START_CODE_PREFIX_NOFIRST, sizeof(H264_START_CODE_PREFIX_NOFIRST));
            video_dec_mgr->accumulator_pos += sizeof(H264_START_CODE_PREFIX_NOFIRST);
        } else
        {
            memcpy(&((uint8_t*)video_dec_mgr->accumulator)[video_dec_mgr->accumulator_pos], H264_START_CODE_PREFIX, start_code_prefix_size);
            video_dec_mgr->accumulator_pos += start_code_prefix_size;
        }
	}
	memcpy(&((uint8_t*)video_dec_mgr->accumulator)[video_dec_mgr->accumulator_pos], pay_ptr, pay_size);
	video_dec_mgr->accumulator_pos += pay_size;
	// end-accumulator
	if ((rtp_hdr->marker) || (rtp_hdr->is_last_pkts)) { // 表示可以结算了
		if (video_dec_mgr->isSpsFrame && !video_dec_mgr->isCurFrameCrash) {
			video_dec_mgr->isCurGopCrash = tsk_false; // reset gop crash flag
		}

		if (!video_dec_mgr->isCurGopCrash && !video_dec_mgr->isCurFrameCrash) {
            
//            if( h264_dump==NULL){
//                h264_dump = fopen("/tmp/h264_dump.h264", "wb+");
//            }
            
			int frameType = ((uint8_t*)video_dec_mgr->accumulator)[4] & 0x1f;

			int video_id = rtp_hdr->csrc[3];
			//检测大小流
			if (video_dec_mgr->video_id == -1) {
				video_dec_mgr->video_id = video_id;
			}
			if (video_dec_mgr->video_id != video_id && video_dec_mgr->init_idr) {
				//重置解码器
				TSK_DEBUG_INFO("video decodec reset video_id:%d\n", video_dec_mgr->video_id);
				if (tdav_codec_h264_reset_decoder(h264, rtp_hdr->session_id, context) != 0) {
					TSK_DEBUG_INFO("video decodec reset fail-\n");
					goto suc;
				}
				video_dec_mgr->video_id = video_id;
				video_dec_mgr->init_idr = false;
			}

			//The first frame must be frame IDR
			if (!video_dec_mgr->init_idr) {
				if (frameType == 7) {
					video_dec_mgr->init_idr = true;
				}
				else
				{
					TSK_DEBUG_INFO("mediacodec not found IDR frame!!! %d\n", frameType);
					goto suc;
				}
			}

			uint64_t curTime = tsk_time_now();
			//这里只是提交帧间隔给上层判断是否卡顿
			AddVideoBlockTime(curTime - video_dec_mgr->last_frame_time, rtp_hdr->session_id);
			video_dec_mgr->last_frame_time = curTime;


//            TSK_DEBUG_INFO("--- before fwrite type:%d", frameType);
//            fwrite((void*)video_dec_mgr->accumulator, 1, video_dec_mgr->accumulator_pos, h264_dump);
            if( h264->decoder.predecode_callback ) {
				if (h264->decoder.first_frame && frameType == 0x07){
            		TSK_DEBUG_INFO("predecode_callback first frame, len:%d, ts:%llu", video_dec_mgr->accumulator_pos, rtp_hdr->timestampl);
            		h264->decoder.first_frame = tsk_false;
            	} 
                
            	if (!h264->decoder.first_frame) {
            		// TSK_DEBUG_INFO("predecode_callback frame type:%d, len:%d", frameType, video_dec_mgr->accumulator_pos );
					h264->decoder.predecode_callback((void*)video_dec_mgr->accumulator, video_dec_mgr->accumulator_pos, rtp_hdr->session_id, rtp_hdr->timestampl);
            	} else {
            		TSK_DEBUG_WARN("predecode_callback wait, frame type:%d, len:%d", frameType, video_dec_mgr->accumulator_pos );
            	}
            }

			if (video_dec_mgr->pfWrite)
			{
				if (video_dec_mgr->file_size > video_dec_mgr->max_size) {
					tdav_codec_h264_create_decode_dump_file(video_dec_mgr, rtp_hdr->session_id);
				}
				
				if (video_dec_mgr->pfWrite) {
					fwrite(video_dec_mgr->accumulator, 1, video_dec_mgr->accumulator_pos, video_dec_mgr->pfWrite);
					video_dec_mgr->file_size += video_dec_mgr->accumulator_pos;
				}
			}

#if HARDWARE_CODECS
			if (h264->decoder.hw_decode_enable && video_dec_mgr->webrtc_hwcodec_issupport && h264->decoder.predecode_need_decode_and_render) {
				//检测分辨率改变
				if (frameType == 0x07) {
					//sps get resolution
					uint32_t width = 0;
					uint32_t height = 0;
					if (youme::webrtc::H264SpsParser::SpsParser((uint8_t*)video_dec_mgr->accumulator, video_dec_mgr->accumulator_pos, &width, &height))
					{
						((trtp_rtp_header_t *)proto_hdr)->video_display_width = width;
						((trtp_rtp_header_t *)proto_hdr)->video_display_height = height;
						if (!video_dec_mgr->width || !video_dec_mgr->height) {
							video_dec_mgr->width = width;
							video_dec_mgr->height = height;
						}
						if (video_dec_mgr->width != width || video_dec_mgr->height != height) {
							//重置解码器
							if (video_dec_mgr->init_idr) {
								TSK_DEBUG_INFO("video decodec reset video size w:%d, h:%d\n", width, height);
								if (tdav_codec_h264_reset_decoder(h264, rtp_hdr->session_id, context) != 0) {
									TSK_DEBUG_INFO("video decodec reset fail-\n");
									goto suc;
								}
							}
							video_dec_mgr->width = width;
							video_dec_mgr->height = height;

							// report decode resolution
							if (h264->decoder.decode_param_callback) {
								h264->decoder.decode_param_callback(rtp_hdr->session_id, video_dec_mgr->width, video_dec_mgr->height);
							}
						}

					}

				}

				youme::webrtc::EncodedImage encoded_image;
				encoded_image._buffer = (uint8_t*)video_dec_mgr->accumulator;
				encoded_image._length = video_dec_mgr->accumulator_pos;
				encoded_image._size = video_dec_mgr->accumulator_pos;
				encoded_image.capture_time_ms_ = rtp_hdr->timestampl;
				if (frameType == 7 || frameType == 5) {
					encoded_image._frameType = youme::webrtc::kVideoFrameKey;
				}
				else {
					encoded_image._frameType = youme::webrtc::kVideoFrameDelta;
				}

				//TSK_DEBUG_INFO("hw decode intput -----------key frame:%d, pts:%u\n", frameType, rtp_hdr->timestamp);
				encoded_image._completeFrame = true;
				youme::webrtc::RTPFragmentationHeader frag_info;
				int ret = video_dec_mgr->webrtc_codec->Decode(encoded_image, false, &frag_info);
				if (ret == WEBRTC_VIDEO_CODEC_FALLBACK_SOFTWARE)
				{
					TSK_DEBUG_INFO("h264 hw decode fail ---> sw decode\n");
					//切换软解
					video_dec_mgr->webrtc_codec->Release();
					delete video_dec_mgr->webrtc_codec;
					video_dec_mgr->webrtc_codec = NULL;
					delete video_dec_mgr->callback;
					video_dec_mgr->callback = NULL;
					video_dec_mgr->webrtc_hwcodec_issupport = false;
					//h264->decoder.hw_decode_force_no = true;
					if (tdav_codec_h264_creat_sw_decoder(h264, video_dec_mgr) != 0) {
						TSK_DEBUG_INFO("[H.264] Failed to decode the buffer with error code =%d, size=%lu, append=%s", ret, video_dec_mgr->accumulator_pos, append_scp ? "yes" : "no");
						if (TMEDIA_CODEC_VIDEO(self)->in.callback) {
							TMEDIA_CODEC_VIDEO(self)->in.result.type = tmedia_video_decode_result_type_error;
							TMEDIA_CODEC_VIDEO(self)->in.result.proto_hdr = proto_hdr;
							TMEDIA_CODEC_VIDEO(self)->in.callback(&TMEDIA_CODEC_VIDEO(self)->in.result);
						}
						goto suc;
					}

				}
				else if (ret != 0)
				{
					goto suc;
				}
				else {
					/* IDR ? */
					if ((frameType == 0x07 || frameType == 0x05) && TMEDIA_CODEC_VIDEO(self)->in.callback) {
						TMEDIA_CODEC_VIDEO(self)->in.result.type = tmedia_video_decode_result_type_idr;
						TMEDIA_CODEC_VIDEO(self)->in.result.proto_hdr = proto_hdr;
						TMEDIA_CODEC_VIDEO(self)->in.callback(&TMEDIA_CODEC_VIDEO(self)->in.result);
					}
					retsize = 1;
					video_dec_mgr->callback->PushReserve(proto_hdr);
					goto suc;
				}

			}

#endif
            if (h264->decoder.predecode_need_decode_and_render) {
#if FFMPEG_SUPPORT
                AVPacket packet;
                av_init_packet(&packet);
                packet.dts = packet.pts = AV_NOPTS_VALUE;
                packet.size = (int)video_dec_mgr->accumulator_pos;
                packet.data = (uint8_t *)video_dec_mgr->accumulator;
#if 0
                ret = avcodec_send_packet(video_dec_mgr->context, &packet);
                while (avcodec_receive_frame(video_dec_mgr->context, video_dec_mgr->picture) == 0)
                {
                    //avpicture_layout((AVPicture *)video_dec_mgr->picture, video_dec_mgr->context->pix_fmt, (int)video_dec_mgr->context->width, (int)video_dec_mgr->context->height,
                    //                (uint8_t *)*out_data, (int)retsize);
                    av_image_copy_to_buffer((uint8_t *)*out_data,
                                            (int)retsize,
                                            (const uint8_t * const *)video_dec_mgr->picture->data,
                                            video_dec_mgr->picture->linesize,
                                            video_dec_mgr->context->pix_fmt,
                                            video_dec_mgr->context->width,
                                            video_dec_mgr->context->height,
                                            1);
                    
                    
                }
#else
                ret = avcodec_decode_video2(video_dec_mgr->context, video_dec_mgr->picture, &got_picture_ptr, &packet);
                if (ret < 0) {
#ifdef WIN32
                    // close h264_qsv decoder
                    if (strcmp(video_dec_mgr->context->codec->name, "libx264")) {
                        TSK_DEBUG_INFO("h264_qsv hw accelerate decode fail ---> sw decode\n");
                        // close h264_qsv decoder
                        if (video_dec_mgr->context) {
                            avcodec_close(video_dec_mgr->context);
                            av_free(video_dec_mgr->context);
                            video_dec_mgr->context = tsk_null;
                        }
                        if (video_dec_mgr->picture) {
                            av_frame_free(&video_dec_mgr->picture);
                            video_dec_mgr->picture = tsk_null;
                        }
                        
                        //h264->decoder.hw_decode_force_no = true;
                        if (tdav_codec_h264_creat_sw_decoder(h264, video_dec_mgr, false) != 0) {
                            TSK_DEBUG_INFO("[H.264] Failed create new sw decoder fail");
                            if (TMEDIA_CODEC_VIDEO(self)->in.callback) {
                                TMEDIA_CODEC_VIDEO(self)->in.result.type = tmedia_video_decode_result_type_error;
                                TMEDIA_CODEC_VIDEO(self)->in.result.proto_hdr = proto_hdr;
                                TMEDIA_CODEC_VIDEO(self)->in.callback(&TMEDIA_CODEC_VIDEO(self)->in.result);
                            }
                        }
                        
                        goto suc;
                    }
#endif
                    TSK_DEBUG_INFO("[H.264] Failed to decode the buffer with error code =%d, size=%lu, append=%s", ret, video_dec_mgr->accumulator_pos, append_scp ? "yes" : "no");
                    if (TMEDIA_CODEC_VIDEO(self)->in.callback) {
                        TMEDIA_CODEC_VIDEO(self)->in.result.type = tmedia_video_decode_result_type_error;
                        TMEDIA_CODEC_VIDEO(self)->in.result.proto_hdr = proto_hdr;
                        TMEDIA_CODEC_VIDEO(self)->in.callback(&TMEDIA_CODEC_VIDEO(self)->in.result);
                    }
                }
                else if (got_picture_ptr) {
                    int xsize = 0;
                    /* IDR ? */
                    if ((frameType == 7) && TMEDIA_CODEC_VIDEO(self)->in.callback) {
                        // TSK_DEBUG_INFO("[H.264] Decoded H.264 IDR");
                        TMEDIA_CODEC_VIDEO(self)->in.result.type = tmedia_video_decode_result_type_idr;
                        TMEDIA_CODEC_VIDEO(self)->in.result.proto_hdr = proto_hdr;
                        TMEDIA_CODEC_VIDEO(self)->in.callback(&TMEDIA_CODEC_VIDEO(self)->in.result);
                    }
                    
                    xsize = avpicture_get_size(video_dec_mgr->context->pix_fmt, video_dec_mgr->context->width, video_dec_mgr->context->height);
                    if (*out_max_size < xsize) {
                        if ((*out_data = tsk_realloc(*out_data, (xsize + FF_INPUT_BUFFER_PADDING_SIZE)))) {
                            *out_max_size = xsize;
                        }
                        else {
                            *out_max_size = 0;
                            av_packet_unref(&packet);
                            goto suc;
                            //return 0;
                        }
                    }
                    retsize = xsize;
                    TMEDIA_CODEC_VIDEO(h264)->in.width = video_dec_mgr->context->width;
                    TMEDIA_CODEC_VIDEO(h264)->in.height = video_dec_mgr->context->height;
                    rtp_hdr->video_display_width = TMEDIA_CODEC_VIDEO(h264)->in.width;
                    rtp_hdr->video_display_height = TMEDIA_CODEC_VIDEO(h264)->in.height;

                	if (!video_dec_mgr->width || !video_dec_mgr->height) {
						video_dec_mgr->width = video_dec_mgr->context->width;
						video_dec_mgr->height = video_dec_mgr->context->height;
					}

					if ((video_dec_mgr->context->width && video_dec_mgr->context->height) && (video_dec_mgr->width != video_dec_mgr->context->width || video_dec_mgr->height != video_dec_mgr->context->height)) {
						video_dec_mgr->width = video_dec_mgr->context->width;
	                    video_dec_mgr->height = video_dec_mgr->context->height;

	                    // report decode resolution
						if (h264->decoder.decode_param_callback) {
							h264->decoder.decode_param_callback(rtp_hdr->session_id, video_dec_mgr->width, video_dec_mgr->height);
						}
					}

                    if (((video_dec_mgr->context->pix_fmt == AV_PIX_FMT_YUV420P || video_dec_mgr->context->pix_fmt == AV_PIX_FMT_YUVJ420P)) &&
                        (retsize == video_dec_mgr->context->width * video_dec_mgr->context->height * 3 / 2)) {
                        avpicture_layout((AVPicture *)video_dec_mgr->picture, video_dec_mgr->context->pix_fmt, (int)video_dec_mgr->context->width, (int)video_dec_mgr->context->height,
                                         (uint8_t *)*out_data, (int)retsize);
                        
                    }
                    else if (video_dec_mgr->context->pix_fmt == AV_PIX_FMT_NV12) {
                        avpicture_layout((AVPicture *)video_dec_mgr->picture, video_dec_mgr->context->pix_fmt, (int)video_dec_mgr->context->width, (int)video_dec_mgr->context->height,
                                         (uint8_t *)*out_data, (int)retsize);
                        
                        // transfer from nv12 to yuv420p
                        int w = video_dec_mgr->context->width;
                        int h = video_dec_mgr->context->height;
                        int yLen = w * h;
                        int vLen = yLen / 4;
                        int index = 0;
                        uint8_t *uv_data = (uint8_t*)*out_data + yLen;
                        uint8_t *vData = (uint8_t*)malloc(w * h / 4);
                        
                        for (index = 0; index < vLen; index++) {
                            *(vData + index) = *(uv_data + index * 2 + 1);
                            *(uv_data + index) = *(uv_data + index * 2);
                        }
                        memcpy(uv_data + vLen, vData, vLen);
                        free(vData);
                        
                    }
                    else {
                        TSK_DEBUG_WARN("[H.264] H264 decoder output size not match, fmt[%d], dec_size[%d],size2[%d]"
                                       , video_dec_mgr->context->pix_fmt, retsize, video_dec_mgr->context->width * video_dec_mgr->context->height * 3 / 2);
                    }
                }
#endif
                av_packet_unref(&packet);
                // Special case, the marker frame is lose
                if ((rtp_hdr->is_last_pkts) && (!rtp_hdr->marker)) {
                    video_dec_mgr->last_seq += 1; // Add 1 to avoid the next packet of the next frame check, TODO
                }
#endif
            }
            
		}
	suc:
		video_dec_mgr->accumulator_pos = 0;
		video_dec_mgr->last_frame_last_pkt_seqnum = rtp_hdr->seq_num;
		video_dec_mgr->isSpsFrame = tsk_false;
		//video_dec_mgr->index = 0;
		//memset(video_dec_mgr->info, 0, sizeof(video_dec_mgr->info));
	}

	return retsize;
}

static tsk_bool_t tdav_codec_h264_sdp_att_match(const tmedia_codec_t* self, const char* att_name, const char* att_value)
{
	return tdav_codec_h264_common_sdp_att_match((tdav_codec_h264_common_t*)self, att_name, att_value);
}

static char* tdav_codec_h264_sdp_att_get(const tmedia_codec_t* self, const char* att_name)
{
	char* att = tdav_codec_h264_common_sdp_att_get((const tdav_codec_h264_common_t*)self, att_name);
	if (att && tsk_striequals(att_name, "fmtp")) {
		tsk_strcat_2(&att, "; impl=%s",
#if  defined(HAVE_FFMPEG)
			"FFMPEG"
#elif defined(HAVE_H264_PASSTHROUGH)
			"PASSTHROUGH"
#endif
		);
	}
	return att;
}

/* ============ H.264 Main Profile Plugin interface ================= */

/* constructor */
static tsk_object_t* tdav_codec_h264_main_ctor(tsk_object_t * self, va_list * app)
{
	tdav_codec_h264_t *h264 = (tdav_codec_h264_t*)self;
	if (h264) {
		/* init base: called by tmedia_codec_create() */
		/* init self */
		if (tdav_codec_h264_init(h264, profile_idc_baseline) != 0) {
			return tsk_null;
		}
	}
	return self;
}
/* destructor */
static tsk_object_t* tdav_codec_h264_main_dtor(tsk_object_t * self)
{
	tdav_codec_h264_t *h264 = (tdav_codec_h264_t*)self;
	if (h264) {
		/* deinit base */
		tdav_codec_h264_common_deinit((tdav_codec_h264_common_t*)self);
		/* deinit self */
		tdav_codec_h264_deinit(h264);

	}

	return self;
}
/* object definition */
static const tsk_object_def_t tdav_codec_h264_main_def_s =
{
	sizeof(tdav_codec_h264_t),
	tdav_codec_h264_main_ctor,
	tdav_codec_h264_main_dtor,
	tmedia_codec_cmp,
};
/* plugin definition*/
static const tmedia_codec_plugin_def_t tdav_codec_h264_main_plugin_def_s =
{
	&tdav_codec_h264_main_def_s,

	tmedia_video,
	tmedia_codec_id_h264_mp,
	"H264",
	"H264 Main Profile (FFmpeg, x264)",
	TMEDIA_CODEC_FORMAT_H264_MP,
	tsk_true,
	90000, // rate

		   /* audio */
	{ 0 },

	/* video (width, height, fps)*/
	{ 176, 144, 0 },// fps is @deprecated

	tdav_codec_h264_set,
	tdav_codec_h264_open,
	tdav_codec_h264_close,
	tdav_codec_h264_encode,
	tdav_codec_h264_decode,
	tsk_null,
	tsk_null,
	tdav_codec_h264_sdp_att_match,
	tdav_codec_h264_sdp_att_get,
	false
};
const tmedia_codec_plugin_def_t *tdav_codec_h264_main_plugin_def_t = &tdav_codec_h264_main_plugin_def_s;




void tdac_codec_h264_get_encoder_info(h264_encoder_manager_t *video_enc_mgr, unsigned& width, unsigned& height, unsigned& bitrate, unsigned& fps,
	int& max_quality, int&min_quality)
{
	int bitrate_x;
	int fps_base = tdav_codec_h264_fps_base;
	uint32_t multiple = tmedia_get_max_video_bitrate_multiple();

	if (tdav_video_main == video_enc_mgr->video_id) {
		fps = tmedia_defaults_get_video_fps();
        video_enc_mgr->lastSetedFPS = fps;
		tmedia_defaults_get_video_size(&width, &height);
		width = tdav_codec_video_scale(width, video_enc_mgr->resolution_level);
		height = tdav_codec_video_scale(height, video_enc_mgr->resolution_level);
		//TMEDIA_CODEC_VIDEO(self)->out.width = TMEDIA_CODEC_VIDEO(self)->in.width = width;
		//TMEDIA_CODEC_VIDEO(self)->out.height = TMEDIA_CODEC_VIDEO(self)->in.height = height;
		bitrate = tmedia_defaults_get_video_codec_bitrate();
		bitrate_x = tmedia_defaults_size_to_bitrate(width, height);
		if (bitrate)
			fps_base = fps;

		if (!bitrate || bitrate > bitrate_x * multiple)
			bitrate = bitrate_x;

		bitrate = tdav_max(bitrate  * (video_enc_mgr->bitrate_level / 100.0f), bitrate *0.3f);
		fps = tdav_max(fps * (video_enc_mgr->fps_level / 100.0f), 3);
		bitrate += bitrate*(fps / (float)fps_base - 1)*tdav_codec_h264_fps_to_bitrate_ratio;

		max_quality = tmedia_defaults_get_video_codec_quality_max();
		min_quality = tmedia_defaults_get_video_codec_quality_min();

	}
	else if (tdav_video_child == video_enc_mgr->video_id) {
		fps = tmedia_defaults_get_video_fps_child();
        video_enc_mgr->lastSetedFPS = fps;
		tmedia_defaults_get_video_size_child(&width, &height);
		width = tdav_codec_video_scale(width, video_enc_mgr->resolution_level);
		height = tdav_codec_video_scale(height, video_enc_mgr->resolution_level);
		//TMEDIA_CODEC_VIDEO(self)->out.width = TMEDIA_CODEC_VIDEO(self)->in.width = width;
		//TMEDIA_CODEC_VIDEO(self)->out.height = TMEDIA_CODEC_VIDEO(self)->in.height = height;
		bitrate = tmedia_defaults_get_video_codec_bitrate_child();
		bitrate_x = tmedia_defaults_size_to_bitrate(width, height);
		if (bitrate)
			fps_base = fps;

		if (!bitrate || bitrate > bitrate_x * multiple)
			bitrate = bitrate_x;
		bitrate = tdav_max(bitrate  * (video_enc_mgr->bitrate_level / 100.0f), bitrate *0.3f);
		fps = tdav_max(fps * (video_enc_mgr->fps_level / 100.0f), 3);
		bitrate += bitrate*(fps / (float)fps_base - 1)*tdav_codec_h264_fps_to_bitrate_ratio;

		max_quality = tmedia_defaults_get_video_codec_quality_max_child();
		min_quality = tmedia_defaults_get_video_codec_quality_min_child();
	}
	else {
		fps = tmedia_defaults_get_video_fps_share();
        video_enc_mgr->lastSetedFPS = fps;
		tmedia_defaults_get_video_size_share(&width, &height);
		width = tdav_codec_video_scale(width, video_enc_mgr->resolution_level);
		height = tdav_codec_video_scale(height, video_enc_mgr->resolution_level);
		//TMEDIA_CODEC_VIDEO(self)->out.width = TMEDIA_CODEC_VIDEO(self)->in.width = width;
		//TMEDIA_CODEC_VIDEO(self)->out.height = TMEDIA_CODEC_VIDEO(self)->in.height = height;
		bitrate = tmedia_defaults_get_video_codec_bitrate_share();
		bitrate_x = tmedia_defaults_size_to_bitrate(width, height);
		if (bitrate)
			fps_base = fps;

		if (!bitrate || bitrate > bitrate_x * multiple)
			bitrate = bitrate_x;
		bitrate = tdav_max(bitrate  * (video_enc_mgr->bitrate_level / 100.0f), bitrate *0.3f);
		fps = tdav_max(fps * (video_enc_mgr->fps_level / 100.0f), 3);
		bitrate += bitrate*(fps / (float)fps_base - 1)*tdav_codec_h264_fps_to_bitrate_ratio;

		max_quality = tmedia_defaults_get_video_codec_quality_max_share();
		min_quality = tmedia_defaults_get_video_codec_quality_min_share();
	}

	video_enc_mgr->width = width;
	video_enc_mgr->height = height;
	video_enc_mgr->fps = fps;
	video_enc_mgr->max_bw_kpbs = bitrate;
	video_enc_mgr->outputInterval = 1000 / fps;
}


/* ============ Common To all H264 codecs ================= */
int tdav_codec_h264_open_sw_encoder(tdav_codec_h264_t* self, h264_encoder_manager_t *video_enc_mgr, bool use_hwaccel)
{
	static bool force_downgrade_to_libx264 = false;
	unsigned width, height, bitrate, fps;
	int max_quality, min_quality;

	tdac_codec_h264_get_encoder_info(video_enc_mgr, width, height, bitrate, fps, max_quality, min_quality);
	TSK_DEBUG_INFO("tdav_codec_h264 open sw encoder fps:%d, bitrate:%d, w:%d, h:%d, hw_enable:%d, force_sw:%d", 
		fps, bitrate, width, height, self->encoder.hw_encode_enable, force_downgrade_to_libx264);
#if defined(HAVE_H264_PASSTHROUGH)
#if FFMPEG_SUPPORT

#if !USE_FFMPEG_ENCODE
	{
		// X264 encoder config.
		video_enc_mgr->pX264Handle = NULL;
		video_enc_mgr->pNals = NULL;
		video_enc_mgr->i4Nal = 0;
		video_enc_mgr->i8FrameNum = 0;
		video_enc_mgr->pPicIn = new x264_picture_t;
		video_enc_mgr->pPicOut = new x264_picture_t;
		video_enc_mgr->pX264Param = new x264_param_t;

		x264_param_default_preset(video_enc_mgr->pX264Param, "veryfast", "zerolatency");
		video_enc_mgr->pX264Param->i_threads = 1;   //关闭slice并行编码，兼容safari

													// 视频选项
		video_enc_mgr->pX264Param->i_width = width;
		video_enc_mgr->pX264Param->i_height = height;
		video_enc_mgr->pX264Param->i_keyint_max = tdav_max(fps * self->encoder.idr_frame_interval_multiple, 1); // = 0.33ms
		video_enc_mgr->pX264Param->i_keyint_min = tdav_max(fps * self->encoder.idr_frame_interval_multiple, 1); // = 0.33ms
		video_enc_mgr->pX264Param->i_csp = X264_CSP_I420; // Todo
		video_enc_mgr->pX264Param->i_frame_total = 0;

		if (video_enc_mgr->gop_size > 1) {
			video_enc_mgr->pX264Param->i_keyint_max = video_enc_mgr->gop_size * tdav_max(fps * self->encoder.idr_frame_interval_multiple, 1);
		}
#if 0
		// 流参数
		video_enc_mgr->pX264Param->i_bframe = 5; // 如果用的zerolatency，这个值不能设或者设置成0，目的是不让encoder有缓冲帧
		video_enc_mgr->pX264Param->b_open_gop = 0;
		video_enc_mgr->pX264Param->i_bframe_pyramid = 0;
		video_enc_mgr->pX264Param->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;
#endif
		// 速率控制参数
		//self->encoder.pX264Param->rc.f_rf_constant = 25;     // 实际图像质量，越大图像越花，越小越清晰
		//self->encoder.pX264Param->rc.f_rf_constant_max = 45; // 图像质量最大值

		if (max_quality != -1 && min_quality != -1)
		{
			video_enc_mgr->bitrate_control = H264_ENCODER_RC_ABR;
			video_enc_mgr->pX264Param->rc.i_rc_method = X264_RC_ABR; // 码率控制：CQP（恒定质量），CRF（恒定码率），ARR（平均码率）
			video_enc_mgr->pX264Param->rc.i_bitrate = bitrate * self->encoder.abr_avg_bitrate_percent;
			video_enc_mgr->pX264Param->rc.i_vbv_max_bitrate = bitrate * self->encoder.abr_max_bitrate_percent;
            video_enc_mgr->pX264Param->rc.i_vbv_buffer_size = bitrate * self->encoder.abr_max_bitrate_percent;
            video_enc_mgr->pX264Param->rc.f_ip_factor = 0.8;    // default: 0.9
            
		}
		else {
			video_enc_mgr->bitrate_control = H264_ENCODER_RC_CRF;
			video_enc_mgr->pX264Param->rc.i_rc_method = X264_RC_CRF; // 码率控制：CQP（恒定质量），CRF（恒定码率），ARR（平均码率）
			video_enc_mgr->pX264Param->rc.i_bitrate = bitrate;
			video_enc_mgr->pX264Param->rc.i_vbv_max_bitrate = bitrate;
            video_enc_mgr->pX264Param->rc.i_vbv_buffer_size = bitrate;
		}
		/* CQP 码率完全不可控，弃用
		else{
		video_enc_mgr->pX264Param->rc.i_rc_method   = X264_RC_CQP; // 码率控制：CQP（恒定质量），CRF（恒定码率），ARR（平均码率）
		video_enc_mgr->pX264Param->rc.i_qp_constant = (min_quality + max_quality)/2;   //qp
		video_enc_mgr->pX264Param->rc.i_qp_min = min_quality;
		video_enc_mgr->pX264Param->rc.i_qp_max = max_quality;
		}
		*/
        
        video_enc_mgr->pX264Param->rc.i_qp_step = 2;
        video_enc_mgr->pX264Param->rc.i_qp_min = 23;
        video_enc_mgr->pX264Param->rc.i_qp_max = 30;
        
		video_enc_mgr->pX264Param->analyse.inter = X264_ANALYSE_I4x4;//X264_ANALYSE_I8x8|X264_ANALYSE_I4x4;
		video_enc_mgr->pX264Param->analyse.i_me_method = X264_ME_HEX;
		video_enc_mgr->pX264Param->analyse.i_subpel_refine = 1; //4
		video_enc_mgr->pX264Param->analyse.i_me_range = 16;
		video_enc_mgr->pX264Param->i_level_idc = width*height > 1080 * 2048 ? 50 : (width*height>720 * 1280 ? 41 : 31);
		// video_enc_mgr->pX264Param->analyse.i_noise_reduction = 80;

		video_enc_mgr->pX264Param->i_fps_den = 1;
		video_enc_mgr->pX264Param->i_fps_num = fps; // = fps
		video_enc_mgr->pX264Param->i_timebase_den = video_enc_mgr->pX264Param->i_fps_num;
		video_enc_mgr->pX264Param->i_timebase_num = video_enc_mgr->pX264Param->i_fps_den;
		video_enc_mgr->pX264Param->b_repeat_headers = 1;
		video_enc_mgr->pX264Param->b_vfr_input = 0;
		// 设置profile
		if (TDAV_CODEC_H264_COMMON(self)->profile == profile_idc_baseline) {
			x264_param_apply_profile(video_enc_mgr->pX264Param, x264_profile_names[0]);
		}
		else if (TDAV_CODEC_H264_COMMON(self)->profile == profile_idc_main) {
			//x264_param_apply_profile(self->encoder.pX264Param, x264_profile_names[1]);
			x264_param_apply_profile(video_enc_mgr->pX264Param, x264_profile_names[2]);
		}
		else if (TDAV_CODEC_H264_COMMON(self)->profile == profile_idc_high) {
			x264_param_apply_profile(video_enc_mgr->pX264Param, x264_profile_names[2]);
		}
		// 编码辅助变量
		x264_picture_init(video_enc_mgr->pPicIn);
		x264_picture_alloc(video_enc_mgr->pPicIn, X264_CSP_I420, video_enc_mgr->pX264Param->i_width, video_enc_mgr->pX264Param->i_height);
		video_enc_mgr->pPicIn->img.i_csp = X264_CSP_I420;
		video_enc_mgr->pPicIn->img.i_plane = 3;
		// 打开编码器句柄
		video_enc_mgr->pX264Handle = x264_encoder_open(video_enc_mgr->pX264Param);
		if (video_enc_mgr->pX264Handle) {
			tmedia_set_video_current_bitrate(bitrate);
			return 0;
		}
	}
#else //!USE_FFMPEG_ENCODE
	{
		int ret = 0;

		video_enc_mgr->context = avcodec_alloc_context3(NULL);
		video_enc_mgr->context->codec_id = AV_CODEC_ID_H264;
		video_enc_mgr->context->codec_type = AVMEDIA_TYPE_VIDEO;
		video_enc_mgr->context->pix_fmt = AV_PIX_FMT_YUV420P;

		// 视频选项
		video_enc_mgr->context->width = width;
		video_enc_mgr->context->height = height;
		//    video_enc_mgr->context->thread_type = FF_THREAD_SLICE;
		video_enc_mgr->context->thread_count = 1;
		video_enc_mgr->context->keyint_min = tdav_max(fps * self->encoder.idr_frame_interval_multiple, 1); // = 0.33ms
		video_enc_mgr->context->gop_size = tdav_max(fps * self->encoder.idr_frame_interval_multiple, 1); // = 0.33ms
		if (video_enc_mgr->gop_size > 1) {
			video_enc_mgr->context->gop_size = video_enc_mgr->gop_size * tdav_max(fps * self->encoder.idr_frame_interval_multiple, 1);
		}
		
		video_enc_mgr->context->max_b_frames = 0;
		video_enc_mgr->context->b_frame_strategy = 0;

        video_enc_mgr->context->i_quant_factor = 0.8;
        video_enc_mgr->context->i_quant_offset = 0.0;
        
		video_enc_mgr->context->level = width*height > 1080 * 2048 ? 50 : (width*height>720 * 1280 ? 41 : 31);
		// 设置profile
		if (TDAV_CODEC_H264_COMMON(self)->profile == profile_idc_baseline) {
			video_enc_mgr->context->profile = FF_PROFILE_H264_BASELINE;
		}
		else if (TDAV_CODEC_H264_COMMON(self)->profile == profile_idc_main) {
			video_enc_mgr->context->profile = FF_PROFILE_H264_MAIN;
		}
		else if (TDAV_CODEC_H264_COMMON(self)->profile == profile_idc_high) {
			video_enc_mgr->context->profile = FF_PROFILE_H264_HIGH;
		}

		// 速率控制参数
		// todo abr/crf
		if (max_quality != -1 && min_quality != -1)
		{
			video_enc_mgr->bitrate_control = H264_ENCODER_RC_ABR;; // 码率控制：CQP（恒定质量），CRF（恒定码率），ARR（平均码率）
			video_enc_mgr->context->bit_rate = bitrate * 1000 * self->encoder.abr_avg_bitrate_percent;
			//video_enc_mgr->context->rc_min_rate = bitrate * 1000 * self->encoder.abr_avg_bitrate_percent * 0.3;
			video_enc_mgr->context->rc_max_rate = bitrate * 1000 * self->encoder.abr_avg_bitrate_percent;
			video_enc_mgr->context->rc_buffer_size = bitrate * 1000 * self->encoder.abr_max_bitrate_percent;
			//video_enc_mgr->context->rc_max_available_vbv_use = bitrate * 1000 * self->encoder.abr_avg_bitrate_percent * 0.1;
			//video_enc_mgr->context->rc_min_vbv_overflow_use = bitrate * 1000 * self->encoder.abr_avg_bitrate_percent * 0.05;
		}
		else {
			video_enc_mgr->bitrate_control = H264_ENCODER_RC_CRF;; // 码率控制：CQP（恒定质量），CRF（恒定码率），ARR（平均码率）
			video_enc_mgr->context->bit_rate = bitrate * 1000;
			video_enc_mgr->context->rc_max_rate = bitrate * 1000;
			video_enc_mgr->context->rc_buffer_size = bitrate * 1000;
		}

        video_enc_mgr->context->max_qdiff = 2;
		video_enc_mgr->context->qmin = 21;
		video_enc_mgr->context->qmax = 36;

		video_enc_mgr->context->time_base.num = 1;
		video_enc_mgr->context->time_base.den = fps;// *self->encoder.idr_frame_interval_multiple;// /tdav_codec_h264_idr_frame_interval_num;

		video_enc_mgr->context->me_method = ME_HEX;
		video_enc_mgr->context->me_subpel_quality = 1;
		video_enc_mgr->context->me_range = 16;

		//video_enc_mgr->pX264Param->analyse.inter = X264_ANALYSE_I4x4;//X264_ANALYSE_I8x8|X264_ANALYSE_I4x4;
		//video_enc_mgr->pX264Param->analyse.i_me_method = X264_ME_HEX;
		//video_enc_mgr->pX264Param->analyse.i_subpel_refine = 1; //4
		//video_enc_mgr->pX264Param->analyse.i_me_range = 16;

		video_enc_mgr->context->strict_std_compliance = FF_COMPLIANCE_NORMAL;

		bool use_x264 = force_downgrade_to_libx264 ? true : false;
        AVDictionary *param = NULL;
        // profile
        if (TDAV_CODEC_H264_COMMON(self)->profile == profile_idc_baseline) {
            av_dict_set(&param, "profile", "baseline", 0);
        }
        else if (TDAV_CODEC_H264_COMMON(self)->profile == profile_idc_main) {
            av_dict_set(&param, "profile", "main", 0);
        }
        else if (TDAV_CODEC_H264_COMMON(self)->profile == profile_idc_high) {
            av_dict_set(&param, "profile", "high", 0);
        }
        av_dict_set(&param, "preset", "veryfast", 0);
        av_dict_set(&param, "tune", "zerolatency", 0);
        av_dict_set(&param, "partitions", "i4x4", 0);
		//av_dict_set(&param, "vbv-maxrate", "1000k", 0);
		//av_dict_set(&param, "vbv-bufsize", "2000k", 0);
        if (video_enc_mgr->bitrate_control == H264_ENCODER_RC_CRF) {
            av_dict_set(&param, "crf", "23", 0);
            av_dict_set(&param, "crf_max", "23", 0);
		}
		else if (video_enc_mgr->bitrate_control == H264_ENCODER_RC_ABR) {
			av_dict_set(&param, "pass", "1", 0);
		}

#ifdef WIN32
		std::string codec_name = "libx264";
		Windows_GPU_Series gpu = WINDOWS_GPU_NONE;
		if (self->encoder.hw_encode_enable && use_hwaccel && (!use_x264)) {
			gpu = tdav_codec_h264_get_windows_gpu();
			switch (gpu)
			{
			case WINDOWS_GPU_NONE:
				codec_name = "libx264";
				use_x264 = true;
				force_downgrade_to_libx264 = true;
				break;
			case WINDOWS_GPU_INTEL:
				video_enc_mgr->context->pix_fmt = AV_PIX_FMT_NV12;
				codec_name = "h264_qsv";
				break;
			case WINDOWS_GPU_NVDIA:
				codec_name = "h264_nvenc";
				break;
			case WINDOWS_GPU_AMD:
				codec_name = "h264_amf";
				break;
			default:
				break;
			}
		}
		else {
			use_x264 = true;
		}

		if (!use_x264) {
			switch (gpu)
			{
			case WINDOWS_GPU_NONE:
				break;
			case WINDOWS_GPU_INTEL:
				av_dict_set(&param, "look_ahead", "0", 0);
				av_dict_set(&param, "repeat_pps", "0", 0);
				av_dict_set(&param, "single_sei_nal_unit", "0", 0);
				av_dict_set(&param, "recovery_point_sei", "0", 0);
				av_dict_set(&param, "async_depth", "0", 0);
				av_dict_set(&param, "pic_timing_sei", "0", 0);
				av_dict_set(&param, "max_dec_frame_buffering", "0", 0);
				break;
			case WINDOWS_GPU_NVDIA:
				// level
				if (width*height > 1080 * 2048) {
					av_dict_set(&param, "level", "5.0", 0);
				}
				else if (width*height > 720 * 1280) {
					av_dict_set(&param, "level", "4.1", 0);
				}
				else {
					av_dict_set(&param, "level", "3.1", 0);
				}
				av_dict_set(&param, "preset", "ll", 0);
				av_dict_set(&param, "delay", "0", 0);
				av_dict_set(&param, "rc-lookahead", "0", 0);
				av_dict_set(&param, "zerolatency", "1", 0);
				av_dict_set(&param, "strict_gop", "1", 0);
				if (video_enc_mgr->bitrate_control == H264_ENCODER_RC_CRF) {
					av_dict_set(&param, "rc", "cbr", 0);
				}
				else {
					av_dict_set(&param, "rc", "vbr", 0);
				}
				break;
			case WINDOWS_GPU_AMD:
				// todu
				break;
			default:
				break;
			}
			video_enc_mgr->codec = avcodec_find_encoder_by_name(codec_name.c_str());
			if (!video_enc_mgr->codec) {
				TSK_DEBUG_WARN("tdav codec encoder cannot find %s, switch to libx264", codec_name.c_str());
				force_downgrade_to_libx264 = true;
			}
			else {
				ret = avcodec_open2(video_enc_mgr->context, video_enc_mgr->codec, &param);
				if (ret < 0) {
					TSK_DEBUG_WARN("tdav codec encoder open %s failed, switch to libx264", codec_name.c_str());
					force_downgrade_to_libx264 = true;
				}
				else {
					TSK_DEBUG_INFO("tdav codec open encoder %s success !!", video_enc_mgr->codec->name);
					force_downgrade_to_libx264 = false;
					av_dict_free(&param);
					param = tsk_null;
					return 0;
				}
			}
		}
#endif

		video_enc_mgr->context->pix_fmt = AV_PIX_FMT_YUV420P; // use libx264
		video_enc_mgr->codec = avcodec_find_encoder_by_name("libx264");
		if (!video_enc_mgr->codec) {
			TSK_DEBUG_WARN("tdav codec encoder cannot find libx264");
			av_dict_free(&param);
			param = tsk_null;
			return -1;
		}
		ret = avcodec_open2(video_enc_mgr->context, video_enc_mgr->codec, &param);
		if (ret < 0) {
			if (video_enc_mgr->context) {
				avcodec_close(video_enc_mgr->context);
				av_free(video_enc_mgr->context);
				video_enc_mgr->context = tsk_null;
			}
			av_dict_free(&param);
			param = tsk_null;
			return -1;
		}
		else {
			TSK_DEBUG_INFO("tdav codec open encoder %s success !!", video_enc_mgr->codec->name);
			av_dict_free(&param);
			param = tsk_null;
			return 0;
		}
	}

#endif //!USE_FFMPEG_ENCODE
#endif //FFMPEG_SUPPORT
#endif //defined(HAVE_H264_PASSTHROUGH)
	return -1;
}



int tdav_codec_h264_open_hw_encoder(tdav_codec_h264_t* self, h264_encoder_manager_t *video_enc_mgr, int video_id, const void* context)
{
#ifndef WIN32	
	unsigned width, height, bitrate, fps;
	int max_quality, min_quality;
	tdac_codec_h264_get_encoder_info(video_enc_mgr, width, height, bitrate, fps, max_quality, min_quality);
	TSK_DEBUG_INFO("tdav_codec_h264 open hw encoder fps:%d, bitrate:%d, w:%d, h:%d, enable:%d, gop:%d\n", fps, bitrate, width, height, self->encoder.hw_encode_enable, video_enc_mgr->gop_size);

#if HARDWARE_CODECS
	if (tmedia_defaults_get_video_hwcodec_enabled() &&
		self->encoder.hw_encode_enable &&
		!self->encoder.hw_encode_force_no &&
		tdav_codec_h264_hardware_is_supported(width, height) &&
		youme::webrtc::H264Encoder::IsSupported()) {
		youme::webrtc::VideoCodec	h264_;
		youme::webrtc::VideoEncoder * codec;
		h264_.codecType = youme::webrtc::kVideoCodecH264;
		h264_.mode = youme::webrtc::kRealtimeVideo;
		h264_.startBitrate = bitrate;
		h264_.targetBitrate = bitrate;
		h264_.maxBitrate = bitrate;
		h264_.maxFramerate = fps;
		h264_.width = width;
		h264_.height = height;
		h264_.codecSpecific.H264.profile = youme::webrtc::kProfileBase;
		h264_.codecSpecific.H264.frameDroppingOn = true;
		h264_.codecSpecific.H264.keyFrameInterval = tdav_max(fps * self->encoder.idr_frame_interval_multiple, 1);	//android min 1s
		if (video_enc_mgr->gop_size > 1) {
			h264_.codecSpecific.H264.keyFrameInterval = video_enc_mgr->gop_size * tdav_max(fps * self->encoder.idr_frame_interval_multiple, 1);
		}

		h264_.codecSpecific.H264.spsData = nullptr;
		h264_.codecSpecific.H264.spsLen = 0;
		h264_.codecSpecific.H264.ppsData = nullptr;
		h264_.codecSpecific.H264.ppsLen = 0;
		h264_.max_quality = max_quality;
		h264_.min_quality = min_quality;
		h264_.profile_level = Config_GetInt(tdav_codec_video_hw_profile_level, 0);
		h264_.abr_avg_bitrate_percent = self->encoder.abr_avg_bitrate_percent;
		h264_.abr_max_bitrate_percent = self->encoder.abr_max_bitrate_percent;

		if (tmedia_defaults_get_video_frame_raw_cb_enabled())
			youme::webrtc::H264Encoder::SetEGLContext(nullptr);
		else
		{	
			if(video_id == 2 && tmedia_get_video_egl_share_context() != nullptr){
				youme::webrtc::H264Encoder::SetEGLContext(tmedia_get_video_egl_share_context());
			}else{
				youme::webrtc::H264Encoder::SetEGLContext(tmedia_get_video_egl_context());
			}
		}
		codec = youme::webrtc::H264Encoder::Create();
		if (codec->InitEncode(&h264_, 1, 0) == WEBRTC_VIDEO_CODEC_OK)
		{
			video_enc_mgr->callback = new WebRtcEncodedImageCallback();
			video_enc_mgr->callback->_session = (void*)context;
			video_enc_mgr->callback->_video_enc_mgr = video_enc_mgr;
			codec->RegisterEncodeCompleteCallback(video_enc_mgr->callback);
			video_enc_mgr->webrtc_codec = codec;
			video_enc_mgr->force_idr = true;
			video_enc_mgr->webrtc_hwcodec_issupport = true;
			video_enc_mgr->callback->_video_id = video_id;
			//这里只能用最大quality
			//            float quality = (51- min_quality)/51.0; //51是质量的最大值
			//            codec->SetQuality( quality );
			tmedia_set_video_current_bitrate(bitrate);
			return 0;

		}
		else {
			codec->Release();
			delete codec;
		}
	}

#endif
#endif	
	return -1;
}

int tdav_codec_h264_open_encoder(tdav_codec_h264_t* self, uint32_t video_id, const void* context)
{
#ifdef OS_LINUX
	// linux version no need h264 encode so far
	return 0;
#else    
	h264_encoder_manager_t *video_enc_mgr = new h264_encoder_manager_t;
	if (NULL == video_enc_mgr) {
		return -1;
	}
	// Initial
	memset(video_enc_mgr, 0, sizeof(h264_encoder_manager_t));
	video_enc_mgr->frame_count = 0;
	video_enc_mgr->sec_frame_index = 0;
	video_enc_mgr->bitrate_level = 100;
	video_enc_mgr->fps_level = 100;
	video_enc_mgr->resolution_level = 100;
	video_enc_mgr->video_id = video_id;

	video_enc_mgr->outputInterval = 0;
	video_enc_mgr->outputVideoTs = 0;
	video_enc_mgr->lastInputTs = 0;
	video_enc_mgr->inputInterval = 0;

	video_enc_mgr->gop_size = Config_GetInt(tdav_codec_video_encode_gop_size, 1);	// 默认GOP大小为1s
    tsk_boolean_t HWEncode = false;
	if (!tdav_codec_h264_open_hw_encoder(self, video_enc_mgr, video_id, context)) {
        HWEncode = true;
		goto suc;
	}
	if (!tdav_codec_h264_open_sw_encoder(self, video_enc_mgr)) {
        HWEncode = false;
		goto suc;
	}
	delete video_enc_mgr;
	TSK_DEBUG_ERROR("open h264 encoder Not expected code called");
	return -1;
suc:
#if H264_DATA_DUMP
	video_enc_mgr->file_size = 0;
    video_enc_mgr->max_size = tmedia_defaults_get_h264_file_size_kb() * 1024;
    if (video_enc_mgr->max_size > 0)
    {
        tdav_codec_h264_create_encode_dump_file(video_enc_mgr, video_id, HWEncode);
    }
#endif

	video_enc_mgr->mutex = tsk_mutex_create();
	self->encoder.video_enc_map->insert(std::pair<int32_t, h264_encoder_manager_t*>(video_id, video_enc_mgr));

	//    if( video_id == 2 )
	//    {
	//        tdav_session_av_t* base = (tdav_session_av_t*)context;
	//        int sessionId = base->producer->session_id ;
	//        auto pRecorder = YMVideoRecorderManager::getInstance()->getRecorderBySession( sessionId );
	//
	//        if( pRecorder )
	//        {
	//            //时间戳单位是ms
	//            pRecorder->setVideoInfo( video_enc_mgr->width, video_enc_mgr->height, video_enc_mgr->fps, video_enc_mgr->max_bw_kpbs,  1000 );
	//
	//        }
	//    }

	return 0;
#endif    
}

#if H264_DATA_DUMP
int tdav_codec_h264_create_encode_dump_file(h264_encoder_manager_t* encoder_mgr, int type, tsk_boolean_t HWEncode) {
    if (!encoder_mgr) {
        return -1;
    }
    
    int  baseLen = 0;
    const char *pBaseDir = tmedia_defaults_get_app_document_path();
    strncpy(encoder_mgr->cFIlePath, pBaseDir, sizeof(encoder_mgr->cFIlePath) - 1);
    baseLen = strlen(encoder_mgr->cFIlePath) + 1;
    
    switch (type) {
        case tdav_video_main:
            if (encoder_mgr->pfWrite) {
                fclose(encoder_mgr->pfWrite);
                encoder_mgr->pfWrite = nullptr;
            }
            
            if (HWEncode) {
                strncat(encoder_mgr->cFIlePath, "/dump_h264_main_hw.h264", sizeof(encoder_mgr->cFIlePath) - baseLen);
            } else {
                strncat(encoder_mgr->cFIlePath, "/dump_h264_main_sw.h264", sizeof(encoder_mgr->cFIlePath) - baseLen);
            }
            
            encoder_mgr->pfWrite = fopen(encoder_mgr->cFIlePath, "wb");
            if (!encoder_mgr->pfWrite) {
                TSK_DEBUG_ERROR("bruce >>> create dump h264 main file failed!");
                return -1;
            }
            TSK_DEBUG_INFO("bruce >>> create dump h264 main file successfully!");
            break;
        case tdav_video_child:
            if (encoder_mgr->pfWrite) {
                fclose(encoder_mgr->pfWrite);
                encoder_mgr->pfWrite = nullptr;
            }
            
            if (HWEncode) {
                strncat(encoder_mgr->cFIlePath, "/dump_h264_child_hw.h264", sizeof(encoder_mgr->cFIlePath) - baseLen);
            } else {
                strncat(encoder_mgr->cFIlePath, "/dump_h264_child_sw.h264", sizeof(encoder_mgr->cFIlePath) - baseLen);
            }
            
            encoder_mgr->pfWrite = fopen(encoder_mgr->cFIlePath, "wb");
            if (!encoder_mgr->pfWrite) {
                TSK_DEBUG_ERROR("bruce >>> create dump h264 child file failed!");
                return -1;
            }
            TSK_DEBUG_INFO("bruce >>> create dump h264 child file successfully!");
            break;
        case tdav_video_share:
            if (encoder_mgr->pfWrite) {
                fclose(encoder_mgr->pfWrite);
                encoder_mgr->pfWrite = nullptr;
            }
            
            if (HWEncode) {
                strncat(encoder_mgr->cFIlePath, "/dump_h264_share_hw.h264", sizeof(encoder_mgr->cFIlePath) - baseLen);
            } else {
                strncat(encoder_mgr->cFIlePath, "/dump_h264_share_sw.h264", sizeof(encoder_mgr->cFIlePath) - baseLen);
            }
            
            encoder_mgr->pfWrite = fopen(encoder_mgr->cFIlePath, "wb");
            if (!encoder_mgr->pfWrite) {
                TSK_DEBUG_ERROR("bruce >>> create dump h264 share file failed!");
                return -1;
            }
            TSK_DEBUG_INFO("bruce >>> create dump h264 share file successfully!");
            break;
        default:
            TSK_DEBUG_WARN("bruce >>> invalid type!");
            break;
    }
    
    encoder_mgr->file_size = 0;
    
    return 0;
}

int tdav_codec_h264_create_decode_dump_file(h264_decoder_manager_t* decoder_mgr, int sessionid) {
    if (!decoder_mgr) {
        return -1;
    }
    
    int  baseLen = 0;
    const char *pBaseDir = tmedia_defaults_get_app_document_path();
    strncpy(decoder_mgr->cFIlePath, pBaseDir, sizeof(decoder_mgr->cFIlePath) - 1);
    baseLen = strlen(decoder_mgr->cFIlePath) + 1;
    
	if (decoder_mgr->pfWrite) {
		fclose(decoder_mgr->pfWrite);
		decoder_mgr->pfWrite = nullptr;
	}

	char datadir[256];
#ifdef WIN32
	 _snprintf(datadir, sizeof(datadir), "/dump_h264_recv_%d_%s.h264",
			abs(sessionid), 
			(decoder_mgr->video_id == tdav_video_main ? "main" : (decoder_mgr->video_id == tdav_video_child ? "minor" : "share")));
#else
	 snprintf(datadir, sizeof(datadir), "/dump_h264_recv_%d_%s.h264",
			abs(sessionid), 
			(decoder_mgr->video_id == tdav_video_main ? "main" : (decoder_mgr->video_id == tdav_video_child ? "minor" : "share")));
#endif
	strncat(decoder_mgr->cFIlePath, datadir, sizeof(decoder_mgr->cFIlePath) - baseLen);	

	decoder_mgr->pfWrite = fopen(decoder_mgr->cFIlePath, "wb");
	if (!decoder_mgr->pfWrite) {
		TSK_DEBUG_ERROR("create decode dump h264 file failed, sessionid:%d, videoid:%d", sessionid, decoder_mgr->video_id);
		return -1;
	}
	TSK_DEBUG_INFO("create decode dump h264 file successfully, sessionid:%d, videoid:%d", sessionid, decoder_mgr->video_id);

    decoder_mgr->file_size = 0;
    
    return 0;
}

#endif

void tdav_codec_h264_close_encoder(h264_encoder_manager_t *video_enc_mgr)
{

#if FFMPEG_SUPPORT
#if !USE_FFMPEG_ENCODE
	// 清除图像区域
	if (video_enc_mgr->pPicIn)
	{
		x264_picture_clean(video_enc_mgr->pPicIn);
		delete video_enc_mgr->pPicIn;
		video_enc_mgr->pPicIn = NULL;
	}
	if (video_enc_mgr->pPicOut)
	{
		delete video_enc_mgr->pPicOut;
		video_enc_mgr->pPicOut = NULL;
	}
	if (video_enc_mgr->pX264Handle) {
		x264_encoder_close(video_enc_mgr->pX264Handle);
		video_enc_mgr->pX264Handle = NULL;
	}

	if (video_enc_mgr->pX264Param) {
		delete  video_enc_mgr->pX264Param;
		video_enc_mgr->pX264Param = NULL;
	}
#else //!USE_FFMPEG_ENCODE
	if (video_enc_mgr->context) {
		avcodec_close(video_enc_mgr->context);
		avcodec_free_context(&video_enc_mgr->context);
		video_enc_mgr->context = tsk_null;
	}
#endif //!USE_FFMPEG_ENCODE
#endif

#if HARDWARE_CODECS
	if (video_enc_mgr->webrtc_codec) {
		video_enc_mgr->webrtc_codec->Release();
		delete video_enc_mgr->webrtc_codec;
		video_enc_mgr->webrtc_codec = NULL;
	}

	if (video_enc_mgr->callback) {
		delete  video_enc_mgr->callback;
		video_enc_mgr->callback = NULL;
	}
#endif
    
#if H264_DATA_DUMP
    if (video_enc_mgr->pfWrite) {
        fclose(video_enc_mgr->pfWrite);
        video_enc_mgr->pfWrite = NULL;
        video_enc_mgr->file_size = 0;
    }
#endif
}

int tdav_codec_h264_close_encoder(tdav_codec_h264_t* self, tsk_bool_t reset_rotation)
{
	h264_encoder_manager_map_t::iterator it;
	for (it = self->encoder.video_enc_map->begin(); it != self->encoder.video_enc_map->end(); it++) {
		h264_encoder_manager_t *video_enc_mgr = it->second;
		if (video_enc_mgr) {
			if (video_enc_mgr->mutex)
				tsk_mutex_lock(video_enc_mgr->mutex);

			tdav_codec_h264_close_encoder(video_enc_mgr);
			tmedia_set_video_current_bitrate(0);
			if (video_enc_mgr->mutex) {
				tsk_mutex_unlock(video_enc_mgr->mutex);
				tsk_mutex_destroy(&video_enc_mgr->mutex);
				video_enc_mgr->mutex = tsk_null;
			}
			delete video_enc_mgr;
			video_enc_mgr = NULL;
		}
	}
	return 0;
}

int tdav_codec_h264_close_decoder(tdav_codec_h264_t* self)
{
	h264_decoder_manager_map_t::iterator it;
	for (it = self->decoder.video_dec_map->begin(); it != self->decoder.video_dec_map->end(); it++) {
		h264_decoder_manager_t *video_dec_mgr = it->second;
		if (video_dec_mgr) {
			if (video_dec_mgr->last_frame_time != 0)
			{
				uint64_t curTime = tsk_time_now();
				AddVideoBlockTime(curTime - video_dec_mgr->last_frame_time, it->first);
			}

			if (video_dec_mgr->mutex)
				tsk_mutex_lock(video_dec_mgr->mutex);
#if FFMPEG_SUPPORT
			if (video_dec_mgr->context) {
				avcodec_close(video_dec_mgr->context);
				av_free(video_dec_mgr->context);
				video_dec_mgr->context = tsk_null;
			}
			if (video_dec_mgr->picture) {
				av_frame_free(&video_dec_mgr->picture);
				video_dec_mgr->picture = tsk_null;
			}
#endif

#if HARDWARE_CODECS
			if (video_dec_mgr->webrtc_codec) {
				video_dec_mgr->webrtc_codec->Release();
				delete video_dec_mgr->webrtc_codec;
				video_dec_mgr->webrtc_codec = NULL;
			}

			if (video_dec_mgr->callback) {
				delete video_dec_mgr->callback;
				video_dec_mgr->callback = NULL;
			}
#endif
			if (video_dec_mgr->accumulator) {
				TSK_FREE(video_dec_mgr->accumulator);
				video_dec_mgr->accumulator = tsk_null;
			}
			video_dec_mgr->accumulator_pos = 0;
			
#if H264_DATA_DUMP
			if (video_dec_mgr->pfWrite) {
				fclose(video_dec_mgr->pfWrite);
				video_dec_mgr->pfWrite = NULL;
				video_dec_mgr->file_size = 0;
			}
#endif
			if (video_dec_mgr->mutex) {
				tsk_mutex_unlock(video_dec_mgr->mutex);
				tsk_mutex_destroy(&(video_dec_mgr->mutex));
			}
			delete video_dec_mgr;
			video_dec_mgr = NULL;
		}
	}

	return 0;
}

static int tdav_codec_h264_close_decoder_with_sessionid(tdav_codec_h264_t* self, int sessionId)
{
	TSK_DEBUG_INFO("tdav_codec_h264_close_decoder_with_sessionid sessionId:%d", sessionId);

	h264_decoder_manager_map_t::iterator iter = self->decoder.video_dec_map->find(sessionId);
	if (iter == self->decoder.video_dec_map->end())
	{
		TSK_DEBUG_INFO("video decoder not exist with sessionId:%d", sessionId);
		return 0;
	}

	h264_decoder_manager_t *video_dec_mgr = iter->second;
    
    // first remove video_dec_mgr from manager
    self->decoder.video_dec_map->erase(iter);
    
	if (video_dec_mgr) {
		if (video_dec_mgr->last_frame_time != 0)
		{
			uint64_t curTime = tsk_time_now();
			AddVideoBlockTime(curTime - video_dec_mgr->last_frame_time, sessionId);
		}

		if (video_dec_mgr->mutex)
			tsk_mutex_lock(video_dec_mgr->mutex);
#if FFMPEG_SUPPORT
		if (video_dec_mgr->context) {
			avcodec_close(video_dec_mgr->context);
			av_free(video_dec_mgr->context);
			video_dec_mgr->context = tsk_null;
		}
		if (video_dec_mgr->picture) {
			av_frame_free(&video_dec_mgr->picture);
			video_dec_mgr->picture = tsk_null;
		}
#endif

#if HARDWARE_CODECS
		if (video_dec_mgr->webrtc_codec) {
			video_dec_mgr->webrtc_codec->Release();
			delete video_dec_mgr->webrtc_codec;
			video_dec_mgr->webrtc_codec = NULL;
		}

		if (video_dec_mgr->callback) {
			delete video_dec_mgr->callback;
			video_dec_mgr->callback = NULL;
		}
#endif
		if (video_dec_mgr->accumulator) {
			TSK_FREE(video_dec_mgr->accumulator);
			video_dec_mgr->accumulator = tsk_null;
		}
		video_dec_mgr->accumulator_pos = 0;

#if H264_DATA_DUMP
		if (video_dec_mgr->pfWrite) {
			fclose(video_dec_mgr->pfWrite);
			video_dec_mgr->pfWrite = NULL;
			video_dec_mgr->file_size = 0;
		}
#endif		
		if (video_dec_mgr->mutex) {
			tsk_mutex_unlock(video_dec_mgr->mutex);
			tsk_mutex_destroy(&(video_dec_mgr->mutex));
		}
		delete video_dec_mgr;
		video_dec_mgr = NULL;
	}

	

	return 0;
}

static char tdav_codec_h264_ffmpeg_gBuf[2048] = "";
static void tdav_codec_h264_ffmpeg_log_delegate(void *context, int level, const char *fmt, va_list ap)
{
	int ret = vsnprintf(tdav_codec_h264_ffmpeg_gBuf, sizeof(tdav_codec_h264_ffmpeg_gBuf), fmt, ap);
	if (ret > 0) {
		TSK_DEBUG_INFO("[FFmpeg log] %s", tdav_codec_h264_ffmpeg_gBuf);
	}
}

int tdav_codec_h264_init(tdav_codec_h264_t* self, profile_idc_t profile)
{
	int ret = 0;
	level_idc_t level;

	if (!self) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	if ((ret = tdav_codec_h264_common_init(TDAV_CODEC_H264_COMMON(self)))) {
		TSK_DEBUG_ERROR("tdav_codec_h264_common_init() faile with error code=%d", ret);
		return ret;
	}

	if ((ret = tdav_codec_h264_common_level_from_size(TMEDIA_CODEC_VIDEO(self)->out.width, TMEDIA_CODEC_VIDEO(self)->out.height, &level))) {
		TSK_DEBUG_ERROR("Failed to find level for size=[%u, %u]", TMEDIA_CODEC_VIDEO(self)->out.width, TMEDIA_CODEC_VIDEO(self)->out.height);
		return ret;
	}

	TDAV_CODEC_H264_COMMON(self)->pack_mode_local = H264_PACKETIZATION_MODE;
	TDAV_CODEC_H264_COMMON(self)->profile = profile;
	TDAV_CODEC_H264_COMMON(self)->level = level;
	TMEDIA_CODEC_VIDEO(self)->in.max_mbps = TMEDIA_CODEC_VIDEO(self)->out.max_mbps = H264_MAX_MBPS * 1000;
	TMEDIA_CODEC_VIDEO(self)->in.max_br = TMEDIA_CODEC_VIDEO(self)->out.max_br = H264_MAX_BR * 1000;

#if FFMPEG_SUPPORT
	//av_log_set_callback(tdav_codec_h264_ffmpeg_log_delegate);
	av_register_all();

#endif

#if HAVE_FFMPEG
	// register all formats and codecs
	//av_register_all();
	if (!(self->encoder.codec = avcodec_find_encoder(CODEC_ID_H264))) {
		TSK_DEBUG_ERROR("Failed to find H.264 encoder");
		ret = -2;
	}
#endif
    self->encoder.hw_encode_enable = Config_GetBool("HW_ENCODE", 0);
    self->decoder.hw_decode_enable = Config_GetBool("HW_DECODE", 0);

	self->encoder.abr_avg_bitrate_percent = Config_GetInt("VIDEO_ABR_AVG_BITRATE_PERCENT", 60) / 100.0;
	self->encoder.abr_max_bitrate_percent = Config_GetInt("VIDEO_ABR_MAX_BITRATE_PERCENT", 100) / 100.0;
    self->encoder.idr_frame_interval_multiple = Config_GetInt("VIDEO_IDR_INTERVAL", 1); // idr 帧间隔秒数

#if defined(HAVE_H264_PASSTHROUGH)
	TMEDIA_CODEC(self)->passthrough = tsk_true;
	self->decoder.passthrough = tsk_true;
	self->encoder.passthrough = tsk_true;
#endif

	self->decoder.video_dec_map = new h264_decoder_manager_map_t;
	self->encoder.video_enc_map = new h264_encoder_manager_map_t;
    
    self->decoder.predecode_callback = tsk_null;
    self->decoder.predecode_need_decode_and_render = tsk_true;
	self->decoder.first_frame = tsk_true;
	self->decoder.decode_param_callback = tsk_null;

	/* allocations MUST be done by open() */
	return ret;
}

int tdav_codec_h264_deinit(tdav_codec_h264_t* self)
{
	if (!self) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	self->decoder.video_dec_map->clear();
	delete self->decoder.video_dec_map;

	self->encoder.video_enc_map->clear();
	delete self->encoder.video_enc_map;

	TSK_DEBUG_INFO("tdav_codec_h264_deinit ");
	return 0;
}

tsk_bool_t tdav_codec_ffmpeg_h264_is_supported()
{
#if HAVE_FFMPEG
	return (avcodec_find_encoder(CODEC_ID_H264) && avcodec_find_decoder(CODEC_ID_H264));
#else
	return tsk_false;
#endif
}

tsk_bool_t tdav_codec_passthrough_h264_is_supported()
{
#if defined(HAVE_H264_PASSTHROUGH)
	return tsk_true;
#else
	return tsk_false;
#endif
}

static tsk_bool_t tdav_codec_h264_hardware_is_supported(int w, int h)
{
#ifdef ANDROID
	if (w*h < tdav_codec_android_hardware_codec_size_low)
		return false;
#else
	return true;
#endif
	return true;
}


//调节码率
static void tdav_codec_h264_adjust_bitrate(tdav_codec_h264_t* self, h264_encoder_manager_t * video_enc_mgr, int level)
{
	int bitrate, bitrate_x;
	int fps;
	int fps_base = tdav_codec_h264_fps_base;
	uint32_t multiple = tmedia_get_max_video_bitrate_multiple();

	if (tdav_video_main == video_enc_mgr->video_id) {
		fps = tmedia_defaults_get_video_fps();
		bitrate = tmedia_defaults_get_video_codec_bitrate();
		bitrate_x = tmedia_defaults_size_to_bitrate(video_enc_mgr->width, video_enc_mgr->height);
		if (bitrate)
			fps_base = fps;
		if (!bitrate || bitrate > bitrate_x * multiple)
			bitrate = bitrate_x;

		bitrate = tdav_max(bitrate * (level / 100.0f), bitrate*0.1f);

		bitrate += bitrate*(fps / (float)fps_base - 1)*tdav_codec_h264_fps_to_bitrate_ratio;

		video_enc_mgr->max_bw_kpbs = bitrate;
		tmedia_set_video_current_bitrate(bitrate);
	}
	else if (tdav_video_child == video_enc_mgr->video_id) {
		fps = tmedia_defaults_get_video_fps_child();
		bitrate = tmedia_defaults_get_video_codec_bitrate_child();
		bitrate_x = tmedia_defaults_size_to_bitrate(video_enc_mgr->width, video_enc_mgr->height);
		if (bitrate)
			fps_base = fps;
		if (!bitrate || bitrate > bitrate_x * multiple)
			bitrate = bitrate_x;

		bitrate = tdav_max(bitrate * (level / 100.0f), bitrate*0.1f);
		bitrate += bitrate*(fps / (float)fps_base - 1)*tdav_codec_h264_fps_to_bitrate_ratio;

		video_enc_mgr->max_bw_kpbs = bitrate;
	}
	else {
		fps = tmedia_defaults_get_video_fps_share();
		bitrate = tmedia_defaults_get_video_codec_bitrate_share();
		bitrate_x = tmedia_defaults_size_to_bitrate(video_enc_mgr->width, video_enc_mgr->height);
		if (bitrate)
			fps_base = fps;
		if (!bitrate || bitrate > bitrate_x * multiple)
			bitrate = bitrate_x;

		bitrate = tdav_max(bitrate * (level / 100.0f), bitrate*0.1f);
		bitrate += bitrate*(fps / (float)fps_base - 1)*tdav_codec_h264_fps_to_bitrate_ratio;

		video_enc_mgr->max_bw_kpbs = bitrate;
	}
	TSK_DEBUG_INFO("video codec set new bitrate:%d, fps:%d, width:%d, height:%d\n", bitrate, fps, video_enc_mgr->width, video_enc_mgr->height);
#if HARDWARE_CODECS
	if (self->encoder.hw_encode_enable && video_enc_mgr->webrtc_codec) {
		video_enc_mgr->webrtc_codec->SetRates(bitrate, video_enc_mgr->fps);
		TSK_DEBUG_INFO("codec h264 AdjustBitrate level:%d suc, fps:%d, bitrae:%d\n", level, video_enc_mgr->fps, bitrate);
		return;
	}
#endif

#ifdef HAVE_H264_PASSTHROUGH
#if FFMPEG_SUPPORT
#if !USE_FFMPEG_ENCODE
	if (video_enc_mgr->pX264Handle)
	{
		//        video_enc_mgr->pX264Param->rc.i_rc_method = X264_RC_ABR;
		if (video_enc_mgr->pX264Param->rc.i_rc_method == X264_RC_ABR)
		{
			video_enc_mgr->pX264Param->rc.i_vbv_max_bitrate = bitrate * self->encoder.abr_max_bitrate_percent;
			video_enc_mgr->pX264Param->rc.i_bitrate = bitrate * self->encoder.abr_avg_bitrate_percent;
		}
		else {
			video_enc_mgr->pX264Param->rc.i_vbv_max_bitrate = bitrate;
			video_enc_mgr->pX264Param->rc.i_bitrate = bitrate;
		}

		int nRes = x264_encoder_reconfig(video_enc_mgr->pX264Handle, video_enc_mgr->pX264Param);
		if (nRes != 0) {
			TSK_DEBUG_ERROR("Failed to set x264 bitrate");
		}
	}
#else
	if (video_enc_mgr->context) {
		// FFmpeg 内部会自动调节的
		if (video_enc_mgr->bitrate_control == H264_ENCODER_RC_ABR)
		{
		    video_enc_mgr->context->bit_rate = bitrate * 1000 * self->encoder.abr_avg_bitrate_percent;
			video_enc_mgr->context->rc_max_rate = bitrate * 1000 * self->encoder.abr_avg_bitrate_percent;
		}
		else {
			video_enc_mgr->context->bit_rate = bitrate * 1000;
			video_enc_mgr->context->rc_max_rate = bitrate * 1000;
		}
		//            video_enc_mgr->context->rc_buffer_size = bitrate * 1000 * 0.85;
	}
#endif //!USE_FFMPEG_ENCODE
#endif
#endif

}

#endif /* HAVE_FFMPEG || HAVE_H264_PASSTHROUGH */
