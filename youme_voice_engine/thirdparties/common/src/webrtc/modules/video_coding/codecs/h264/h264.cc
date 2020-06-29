/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 */

#include "webrtc/modules/video_coding/codecs/h264/include/h264.h"

#if defined(WEBRTC_IOS) || defined(WEBRTC_MAC)
#include "webrtc/modules/video_coding/codecs/h264/h264_video_toolbox_decoder.h"
#include "webrtc/modules/video_coding/codecs/h264/h264_video_toolbox_encoder.h"
#endif

#ifdef ANDROID

#include "webrtc/api/java/jni/androidmediaencoder_jni.h"
#include "webrtc/api/java/jni/androidmediadecoder_jni.h"
#endif

#ifdef HARDWARE_CODECS
#include "webrtc/common_video/h264/sps_parser.h"
#endif

#include "webrtc/base/checks.h"
namespace youme{
    namespace webrtc {
        
        // We need this file to be C++ only so it will compile properly for all
        // platforms. In order to write ObjC specific implementations we use private
        // externs. This function is defined in h264.mm.
#if defined(WEBRTC_IOS) || defined(WEBRTC_MAC)
        extern bool IsH264CodecSupportedObjC();
#endif
        
        bool IsH264CodecSupported() {
            
#if defined(WEBRTC_IOS) || defined(WEBRTC_MAC)
            return IsH264CodecSupportedObjC();
#elif defined(ANDROID)
            return MediaCodecVideoEncoderFactory::IsSupported();
#else
            return false;
#endif
        }
        
        VideoEncoder* H264Encoder::Create() {
            RTC_DCHECK(H264Encoder::IsSupported());
#if (defined(WEBRTC_IOS) || defined(WEBRTC_MAC)) && defined(WEBRTC_VIDEO_TOOLBOX_SUPPORTED)
            return new H264VideoToolboxEncoder();
#elif defined(ANDROID)
            return MediaCodecVideoEncoderFactory::CreateVideoEncoder();
#else
            RTC_NOTREACHED();
            return nullptr;
#endif
        }
        
        bool H264Encoder::IsSupported() {
            
#if defined(WEBRTC_IOS) || defined(WEBRTC_MAC)
            return IsH264CodecSupported();
#elif defined(ANDROID)
            return MediaCodecVideoEncoderFactory::IsSupported();
#else
            return false;
#endif
        }
        
        bool H264Encoder::IsSupportedGLES()
        {
#if defined(WEBRTC_IOS) || defined(WEBRTC_MAC)
            return IsH264CodecSupported();
#elif defined(ANDROID)
            return MediaCodecVideoEncoderFactory::IsSupportedGLES();
#else
            return false;
#endif
        }
        
        void H264Encoder::SetEGLContext(void* eglContext)
        {
#if defined(ANDROID)
            MediaCodecVideoEncoderFactory::SetEGLContext((jobject)eglContext);
#endif
        }
        
        VideoDecoder* H264Decoder::Create() {
            RTC_DCHECK(H264Decoder::IsSupported());
#if (defined(WEBRTC_IOS) || defined(WEBRTC_MAC)) && defined(WEBRTC_VIDEO_TOOLBOX_SUPPORTED)
            return new H264VideoToolboxDecoder();
#elif defined(ANDROID)
            return MediaCodecVideoDecoderFactory::CreateVideoDecoder();
#else
            RTC_NOTREACHED();
            return nullptr;
#endif
        }
        
        bool H264Decoder::IsSupported() {
            
#if defined(WEBRTC_IOS) || defined(WEBRTC_MAC)
            return IsH264CodecSupported();
#elif defined(ANDROID)
            return MediaCodecVideoDecoderFactory::IsSupported();
#else
            return false;
#endif
        }
        void H264Decoder::SetEGLContext(void* eglContext)
        {
#if defined(ANDROID)
            MediaCodecVideoDecoderFactory::SetEGLContext((jobject)eglContext);
#endif
        }
        bool H264SpsParser::SpsParser(uint8_t* buff, int buffLen, uint32_t* width, uint32_t* height)
        {
            if ((buff[4]&0x1f) != 7) {
                return false;
            }
            int len = 0;
            for (int i = 4; i < buffLen - 4; ++i) {
                if (buff[i] == 0 &&
                    buff[i+1] == 0 &&
                    buff[i+2] == 0 &&
                    buff[i+3] == 1 &&
                    (buff[i+4]&0x1f) == 8) {
                    len = i;
                    break;
                }
            }
            if (!len)
                return false;
            uint8_t* sps = &buff[5];
            uint32_t sps_len = len - 5;
            if(sps_len <= 0)
            {
                printf("h264 parse sps len == 0 !\n");
                 return false;
            }
            rtc::Optional<SpsParser::SpsState> spsState = SpsParser::ParseSps(sps, sps_len);
            if (spsState->width == 0 || spsState->height == 0) {
                return false;
            }
            *width = spsState->width;// - (spsState->frame_crop_left_offset + spsState->frame_crop_right_offset)*2;
            *height = spsState->height;// - (spsState->frame_crop_top_offset + spsState->frame_crop_bottom_offset)*2;
            return true;
        }
        
    }
}  // namespace webrtc
