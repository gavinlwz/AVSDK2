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
#include "webrtc/modules/video_coding/codecs/h264/h264_video_toolbox_decoder.h"
#if defined(WEBRTC_VIDEO_TOOLBOX_SUPPORTED)

#if defined(WEBRTC_IOS)
#include "RTCUIApplication.h"
#include "RTCUIApplicationStatusObserver.h"
#import <UIKit/UIKit.h>
#endif

#include "libyuv/convert.h"
#include "webrtc/base/checks.h"
#include "webrtc/system_wrappers/include/logging.h"
#include "webrtc/common_video/include/video_frame_buffer.h"
#include "webrtc/modules/video_coding/codecs/h264/h264_video_toolbox_nalu.h"
#include "webrtc/video_frame.h"
namespace youme{
    namespace internal {
        
        // Convenience function for creating a dictionary.
        inline CFDictionaryRef CreateCFDictionary(CFTypeRef* keys,
                                                  CFTypeRef* values,
                                                  size_t size) {
            return CFDictionaryCreate(nullptr, keys, values, size,
                                      &kCFTypeDictionaryKeyCallBacks,
                                      &kCFTypeDictionaryValueCallBacks);
        }
        
        // Struct that we pass to the decoder per frame to decode. We receive it again
        // in the decoder callback.
        struct FrameDecodeParams {
            FrameDecodeParams(webrtc::DecodedImageCallback* cb, int64_t ts, bool pixel)
            : callback(cb), timestamp(ts), isPixelBufferCb(pixel) {}
            webrtc::DecodedImageCallback* callback;
            int64_t timestamp;
            bool isPixelBufferCb;
        };
        
        // On decode we receive a CVPixelBuffer, which we need to convert to a frame
        // buffer for use in the rest of WebRTC. Unfortunately this involves a frame
        // copy.
        // TODO(tkchin): Stuff CVPixelBuffer into a TextureBuffer and pass that along
        // instead once the pipeline supports it.
        rtc::scoped_refptr<webrtc::VideoFrameBuffer> VideoFrameBufferForPixelBuffer(
                                                                                    CVPixelBufferRef pixel_buffer) {
            RTC_DCHECK(pixel_buffer);
            RTC_DCHECK(CVPixelBufferGetPixelFormatType(pixel_buffer) ==
                       kCVPixelFormatType_420YpCbCr8BiPlanarFullRange);
            size_t width = CVPixelBufferGetWidthOfPlane(pixel_buffer, 0);
            size_t height = CVPixelBufferGetHeightOfPlane(pixel_buffer, 0);
            // TODO(tkchin): Use a frame buffer pool.
            rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer =
            new rtc::RefCountedObject<webrtc::I420Buffer>(width, height);
            CVPixelBufferLockBaseAddress(pixel_buffer, kCVPixelBufferLock_ReadOnly);
            const uint8_t* src_y = reinterpret_cast<const uint8_t*>(
                                                                    CVPixelBufferGetBaseAddressOfPlane(pixel_buffer, 0));
            int src_y_stride = CVPixelBufferGetBytesPerRowOfPlane(pixel_buffer, 0);
            const uint8_t* src_uv = reinterpret_cast<const uint8_t*>(
                                                                     CVPixelBufferGetBaseAddressOfPlane(pixel_buffer, 1));
            int src_uv_stride = CVPixelBufferGetBytesPerRowOfPlane(pixel_buffer, 1);
            int ret = libyuv::NV12ToI420(
                                         src_y, src_y_stride, src_uv, src_uv_stride,
                                         buffer->MutableData(webrtc::kYPlane), buffer->stride(webrtc::kYPlane),
                                         buffer->MutableData(webrtc::kUPlane), buffer->stride(webrtc::kUPlane),
                                         buffer->MutableData(webrtc::kVPlane), buffer->stride(webrtc::kVPlane),
                                         width, height);
            CVPixelBufferUnlockBaseAddress(pixel_buffer, kCVPixelBufferLock_ReadOnly);
            if (ret) {
                YM_LOG(LS_ERROR) << "Error converting NV12 to I420: " << ret;
                return nullptr;
            }
            return buffer;
        }
        
        // This is the callback function that VideoToolbox calls when decode is
        // complete.
        void VTDecompressionOutputCallback(void* decoderRef,
                                           void* params,
                                           OSStatus status,
                                           VTDecodeInfoFlags info_flags,
                                           CVImageBufferRef image_buffer,
                                           CMTime timestamp,
                                           CMTime duration) {
            rtc::scoped_ptr<FrameDecodeParams> decode_params(
                                                             reinterpret_cast<FrameDecodeParams*>(params));
            if (status != noErr) {
                webrtc::H264VideoToolboxDecoder *decoder = (webrtc::H264VideoToolboxDecoder *)decoderRef;
                decoder->setError(status);
                YM_LOG(LS_ERROR) << "Failed to decode frame. Status: " << status;
                return;
            }
            
            if (!image_buffer) {
                YM_LOG(LS_ERROR) << "Failed to decode frame. image buffer is null ";
                return;
            }
            // TODO(tkchin): Handle CVO properly.
            
            if(decode_params->isPixelBufferCb){
                size_t width =  (size_t)CVPixelBufferGetWidth((CVPixelBufferRef)image_buffer);
                size_t height = (size_t)CVPixelBufferGetHeight((CVPixelBufferRef)image_buffer);
                //size_t width = CVPixelBufferGetWidthOfPlane(image_buffer, 0);
                //size_t height = CVPixelBufferGetHeightOfPlane(image_buffer, 0);
                rtc::scoped_refptr<youme::webrtc::VideoFrameBuffer> buffer =
                new rtc::RefCountedObject<youme::webrtc::NativeHandleBuffer>((void*)image_buffer, width, height);
                youme::webrtc::VideoFrame decoded_frame(buffer, decode_params->timestamp, decode_params->timestamp, youme::webrtc::kVideoRotation_0);
                decode_params->callback->Decoded(decoded_frame);
                //NSLog(@"Decoded w:%d, h:%d, ts:%lld\n", width, height, decode_params->timestamp);
            }
            else{
                rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer = VideoFrameBufferForPixelBuffer(image_buffer);
                webrtc::VideoFrame decoded_frame(buffer, decode_params->timestamp, decode_params->timestamp,  webrtc::kVideoRotation_0);
                decode_params->callback->Decoded(decoded_frame);
            }
        }
        
    }  // namespace internal
    
    namespace webrtc {
        
        H264VideoToolboxDecoder::H264VideoToolboxDecoder()
        : callback_(nullptr),
        video_format_(nullptr),
        decompression_session_(nullptr),
        _isPixelBufferCb(true){
            h264Buffer = nullptr;
            h264BufferSize = 0;
            _error = noErr;
            
#if defined(WEBRTC_IOS)
            [RTCUIApplicationStatusObserver prepareForUse];
#endif
        }
        
        H264VideoToolboxDecoder::~H264VideoToolboxDecoder() {
            DestroyDecompressionSession();
            SetVideoFormat(nullptr);
            if(h264Buffer)
                delete [] h264Buffer;
        }
        
        int H264VideoToolboxDecoder::InitDecode(const VideoCodec* video_codec,
                                                int number_of_cores,
                                                bool texturecb) {
            _isPixelBufferCb = texturecb;
            return WEBRTC_VIDEO_CODEC_OK;
        }
        
        int H264VideoToolboxDecoder::Decode(
                                            const EncodedImage& input_image,
                                            bool missing_frames,
                                            const RTPFragmentationHeader* fragmentation,
                                            const CodecSpecificInfo* codec_specific_info,
                                            int64_t render_time_ms) {
            RTC_DCHECK(input_image._buffer);
            
            if (_error != noErr) {
                YM_LOG(LS_WARNING) << "Last frame decode failed.";
                _error = noErr;
                if(input_image._frameType == kVideoFrameDelta) return WEBRTC_VIDEO_CODEC_ERROR;
            }
            
#if defined(WEBRTC_IOS)
            if (![[RTCUIApplicationStatusObserver sharedInstance] isApplicationActive]) {
                // Ignore all decode requests when app isn't active. In this state, the
                // hardware decoder has been invalidated by the OS.
                // Reset video format so that we won't process frames until the next
                // keyframe.
                SetVideoFormat(nullptr);
                return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
            }
#endif
            
            CMVideoFormatDescriptionRef input_format = nullptr;
            if (H264AnnexBBufferHasVideoFormatDescription(input_image._buffer,
                                                          input_image._length)) {
                input_format = CreateVideoFormatDescription(input_image._buffer,
                                                            input_image._length);
                if (input_format) {
                    // Check if the video format has changed, and reinitialize decoder if
                    // needed.
                    if (!CMFormatDescriptionEqual(input_format, video_format_)) {
                        SetVideoFormat(input_format);
                        ResetDecompressionSession();
                    }
                    CFRelease(input_format);
                }
            }
            if (!video_format_) {
                // We received a frame but we don't have format information so we can't
                // decode it.
                // This can happen after backgrounding. We need to wait for the next
                // sps/pps before we can resume so we request a keyframe by returning an
                // error.
                YM_LOG(LS_WARNING) << "Missing video format. Frame with sps/pps required.";
                return WEBRTC_VIDEO_CODEC_ERROR;
            }
            
            if (input_image._length + 128 > h264BufferSize) {
                if(h264Buffer)
                    delete []  h264Buffer;
                h264Buffer = new uint8_t[input_image._length + 128];
                h264BufferSize = input_image._length + 128;
            }
            
            CMSampleBufferRef sample_buffer = nullptr;
            if (!H264AnnexBBufferToCMSampleBuffer(input_image._buffer,
                                                  input_image._length,
                                                  video_format_,
                                                  &sample_buffer,
                                                  h264Buffer,
                                                  h264BufferSize)) {
                return WEBRTC_VIDEO_CODEC_ERROR;
            }
            RTC_DCHECK(sample_buffer);
            
            
            // Check if the video format has changed, and reinitialize decoder if needed.
            CMVideoFormatDescriptionRef description =
            CMSampleBufferGetFormatDescription(sample_buffer);
            if (!CMFormatDescriptionEqual(description, video_format_)) {
                SetVideoFormat(description);
                ResetDecompressionSession();
            }
            VTDecodeFrameFlags decode_flags =
            kVTDecodeFrame_EnableAsynchronousDecompression;
            rtc::scoped_ptr<internal::FrameDecodeParams> frame_decode_params;
            frame_decode_params.reset(new internal::FrameDecodeParams(callback_, input_image.capture_time_ms_, _isPixelBufferCb));
            OSStatus status = VTDecompressionSessionDecodeFrame(
                                                                decompression_session_, sample_buffer, decode_flags,
                                                                frame_decode_params.release(), nullptr);
#if defined(WEBRTC_IOS)
            // Re-initialize the decoder if we have an invalid session while the app is
            // active and retry the decode request.
            if (status == kVTInvalidSessionErr &&
                ResetDecompressionSession() == WEBRTC_VIDEO_CODEC_OK) {
                frame_decode_params.reset(new internal::FrameDecodeParams(callback_, input_image._timeStamp, _isPixelBufferCb));
                status = VTDecompressionSessionDecodeFrame(
                                                           decompression_session_, sample_buffer, decode_flags,
                                                           frame_decode_params.release(), nullptr);
            }
#endif
            CFRelease(sample_buffer);
            if (status != noErr) {
                YM_LOG(LS_ERROR) << "Failed to decode frame with code: " << status;
                return WEBRTC_VIDEO_CODEC_ERROR;
            }
            return WEBRTC_VIDEO_CODEC_OK;
        }
        
        int H264VideoToolboxDecoder::RegisterDecodeCompleteCallback(
                                                                    DecodedImageCallback* callback) {
            RTC_DCHECK(!callback_);
            callback_ = callback;
            return WEBRTC_VIDEO_CODEC_OK;
        }
        
        int H264VideoToolboxDecoder::Release() {
            callback_ = nullptr;
            return WEBRTC_VIDEO_CODEC_OK;
        }
        
        int H264VideoToolboxDecoder::Reset() {
            ResetDecompressionSession();
            return WEBRTC_VIDEO_CODEC_OK;
        }
        
        int H264VideoToolboxDecoder::ResetDecompressionSession() {
            DestroyDecompressionSession();
            OSStatus status;
            // Need to wait for the first SPS to initialize decoder.
            if (!video_format_) {
                return WEBRTC_VIDEO_CODEC_OK;
            }
            
            // Set keys for OpenGL and IOSurface compatibilty, which makes the encoder
            // create pixel buffers with GPU backed memory. The intent here is to pass
            // the pixel buffers directly so we avoid a texture upload later during
            // rendering. This currently is moot because we are converting back to an
            // I420 frame after decode, but eventually we will be able to plumb
            // CVPixelBuffers directly to the renderer.
            // TODO(tkchin): Maybe only set OpenGL/IOSurface keys if we know that that
            // we can pass CVPixelBuffers as native handles in decoder output.
            static size_t const attributes_size = 3;
            CFTypeRef keys[attributes_size] = {
#if defined(WEBRTC_IOS)
                kCVPixelBufferOpenGLESCompatibilityKey,
#elif defined(WEBRTC_MAC)
                kCVPixelBufferOpenGLCompatibilityKey,
#endif
                kCVPixelBufferIOSurfacePropertiesKey,
                kCVPixelBufferPixelFormatTypeKey
            };
            CFDictionaryRef io_surface_value =
            internal::CreateCFDictionary(nullptr, nullptr, 0);
            int64_t nv12type = _isPixelBufferCb ? kCVPixelFormatType_32BGRA : kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
            CFNumberRef pixel_format =
            CFNumberCreate(nullptr, kCFNumberLongType, &nv12type);
            CFTypeRef values[attributes_size] = {
                kCFBooleanTrue,
                io_surface_value,
                pixel_format
            };
            CFDictionaryRef attributes =
            internal::CreateCFDictionary(keys, values, attributes_size);
            if (io_surface_value) {
                CFRelease(io_surface_value);
                io_surface_value = nullptr;
            }
            if (pixel_format) {
                CFRelease(pixel_format);
                pixel_format = nullptr;
            }
            VTDecompressionOutputCallbackRecord record = {
                internal::VTDecompressionOutputCallback, this,
            };
            //NSLog(@"vdt enter\n");
            CFMutableDictionaryRef decoderSpecifications = NULL;
#if !TARGET_OS_IPHONE
            /** iOS is always hardware-accelerated **/
            CFStringRef bkey = kVTVideoDecoderSpecification_RequireHardwareAcceleratedVideoDecoder;
            CFBooleanRef bvalue = kCFBooleanTrue;
            
            CFStringRef ckey = kVTVideoDecoderSpecification_EnableHardwareAcceleratedVideoDecoder;
            CFBooleanRef cvalue = kCFBooleanTrue;
            decoderSpecifications = CFDictionaryCreateMutable(
                                                              kCFAllocatorDefault,
                                                              2,
                                                              &kCFTypeDictionaryKeyCallBacks,
                                                              &kCFTypeDictionaryValueCallBacks);
            
            CFDictionaryAddValue(decoderSpecifications, bkey, bvalue);
            CFDictionaryAddValue(decoderSpecifications, ckey, cvalue);
            //  CFDictionaryAddValue(encoderSpecifications, key, value);
            status = VTDecompressionSessionCreate(kCFAllocatorDefault, video_format_, decoderSpecifications, attributes,
                                         &record, &decompression_session_);
            CFRelease(decoderSpecifications);
            if (status != noErr) //macos 小分辨率硬解会失败！
            {
                status = VTDecompressionSessionCreate(kCFAllocatorDefault, video_format_, NULL, attributes,
                                                      &record, &decompression_session_);
                if (status != noErr) {
                    CFRelease(attributes);
                    DestroyDecompressionSession();
                    return WEBRTC_VIDEO_CODEC_ERROR;
                }
            }
            CFRelease(attributes);
#else
            status = VTDecompressionSessionCreate(kCFAllocatorDefault, video_format_, decoderSpecifications, attributes,
                                         &record, &decompression_session_);
            CFRelease(attributes);
            if (status != noErr) {
                DestroyDecompressionSession();
                return WEBRTC_VIDEO_CODEC_ERROR;
            }
#endif
            //NSLog(@"vdt leave");
            ConfigureDecompressionSession();
            
            return WEBRTC_VIDEO_CODEC_OK;
        }
        
        void H264VideoToolboxDecoder::ConfigureDecompressionSession() {
            RTC_DCHECK(decompression_session_);
#if defined(WEBRTC_IOS)
            VTSessionSetProperty(decompression_session_,
                                 kVTDecompressionPropertyKey_RealTime, kCFBooleanTrue);
#endif
        }
        
        void H264VideoToolboxDecoder::DestroyDecompressionSession() {
            if (decompression_session_) {
                VTDecompressionSessionWaitForAsynchronousFrames(decompression_session_);
                VTDecompressionSessionInvalidate(decompression_session_);
                CFRelease(decompression_session_);
                decompression_session_ = nullptr;
            }
        }
        
        void H264VideoToolboxDecoder::SetVideoFormat(
                                                     CMVideoFormatDescriptionRef video_format) {
            if (video_format_ == video_format) {
                return;
            }
            if (video_format_) {
                CFRelease(video_format_);
            }
            video_format_ = video_format;
            if (video_format_) {
                CFRetain(video_format_);
            }
        }
        
        void H264VideoToolboxDecoder::setError(OSStatus error) {
            _error = error;
        }
    }  // namespace webrtc
}
#endif  // defined(WEBRTC_VIDEO_TOOLBOX_SUPPORTED)
