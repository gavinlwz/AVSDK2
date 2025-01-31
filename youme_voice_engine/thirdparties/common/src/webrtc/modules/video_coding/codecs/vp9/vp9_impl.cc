﻿/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 */

#include "webrtc/modules/video_coding/codecs/vp9/vp9_impl.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>

#include "vpx/vpx_encoder.h"
#include "vpx/vpx_decoder.h"
#include "vpx/vp8cx.h"
#include "vpx/vp8dx.h"

#include "webrtc/base/bind.h"
#include "webrtc/base/checks.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/trace_event.h"
#include "webrtc/common.h"
#include "webrtc/common_video/libyuv/include/webrtc_libyuv.h"
#include "webrtc/modules/include/module_common_types.h"
#include "webrtc/modules/video_coding/codecs/vp9/screenshare_layers.h"
#include "webrtc/system_wrappers/include/tick_util.h"

namespace {

// VP9DecoderImpl::ReturnFrame helper function used with WrappedI420Buffer.
static void WrappedI420BufferNoLongerUsedCb(
    webrtc::Vp9FrameBufferPool::Vp9FrameBuffer* img_buffer) {
  img_buffer->Release();
}

}  // anonymous namespace

namespace webrtc {

// Only positive speeds, range for real-time coding currently is: 5 - 8.
// Lower means slower/better quality, higher means fastest/lower quality.
int GetCpuSpeed(int width, int height) {
  // For smaller resolutions, use lower speed setting (get some coding gain at
  // the cost of increased encoding complexity).
  if (width * height <= 352 * 288)
    return 5;
  else
    return 7;
}

VP9Encoder* VP9Encoder::Create() {
  return new VP9EncoderImpl();
}

void VP9EncoderImpl::EncoderOutputCodedPacketCallback(vpx_codec_cx_pkt* pkt,
                                                      void* user_data) {
  VP9EncoderImpl* enc = (VP9EncoderImpl*)(user_data);
  enc->GetEncodedLayerFrame(pkt);
}

VP9EncoderImpl::VP9EncoderImpl()
    : encoded_image_(),
      encoded_complete_callback_(NULL),
      inited_(false),
      timestamp_(0),
      picture_id_(0),
      cpu_speed_(3),
      rc_max_intra_target_(0),
      encoder_(NULL),
      config_(NULL),
      raw_(NULL),
      input_image_(NULL),
      tl0_pic_idx_(0),
      frames_since_kf_(0),
      num_temporal_layers_(0),
      num_spatial_layers_(0),
      frames_encoded_(0),
      // Use two spatial when screensharing with flexible mode.
      spatial_layer_(new ScreenshareLayersVP9(2)) {
  memset(&codec_, 0, sizeof(codec_));
  uint32_t seed = static_cast<uint32_t>(TickTime::MillisecondTimestamp());
  srand(seed);
}

VP9EncoderImpl::~VP9EncoderImpl() {
  Release();
}

int VP9EncoderImpl::Release() {
  if (encoded_image_._buffer != NULL) {
    delete [] encoded_image_._buffer;
    encoded_image_._buffer = NULL;
  }
  if (encoder_ != NULL) {
    if (vpx_codec_destroy(encoder_)) {
      return WEBRTC_VIDEO_CODEC_MEMORY;
    }
    delete encoder_;
    encoder_ = NULL;
  }
  if (config_ != NULL) {
    delete config_;
    config_ = NULL;
  }
  if (raw_ != NULL) {
    vpx_img_free(raw_);
    raw_ = NULL;
  }
  inited_ = false;
  return WEBRTC_VIDEO_CODEC_OK;
}

bool VP9EncoderImpl::ExplicitlyConfiguredSpatialLayers() const {
  // We check target_bitrate_bps of the 0th layer to see if the spatial layers
  // (i.e. bitrates) were explicitly configured.
  return num_spatial_layers_ > 1 &&
         codec_.spatialLayers[0].target_bitrate_bps > 0;
}

bool VP9EncoderImpl::SetSvcRates() {
  uint8_t i = 0;

  if (ExplicitlyConfiguredSpatialLayers()) {
    if (num_temporal_layers_ > 1) {
      LOG(LS_ERROR) << "Multiple temporal layers when manually specifying "
                       "spatial layers not implemented yet!";
      return false;
    }
    int total_bitrate_bps = 0;
    for (i = 0; i < num_spatial_layers_; ++i)
      total_bitrate_bps += codec_.spatialLayers[i].target_bitrate_bps;
    // If total bitrate differs now from what has been specified at the
    // beginning, update the bitrates in the same ratio as before.
    for (i = 0; i < num_spatial_layers_; ++i) {
      config_->ss_target_bitrate[i] = config_->layer_target_bitrate[i] =
          static_cast<int>(static_cast<int64_t>(config_->rc_target_bitrate) *
                           codec_.spatialLayers[i].target_bitrate_bps /
                           total_bitrate_bps);
    }
  } else {
    float rate_ratio[VPX_MAX_LAYERS] = {0};
    float total = 0;

    for (i = 0; i < num_spatial_layers_; ++i) {
      if (svc_internal_.svc_params.scaling_factor_num[i] <= 0 ||
          svc_internal_.svc_params.scaling_factor_den[i] <= 0) {
        LOG(LS_ERROR) << "Scaling factors not specified!";
        return false;
      }
      rate_ratio[i] =
          static_cast<float>(svc_internal_.svc_params.scaling_factor_num[i]) /
          svc_internal_.svc_params.scaling_factor_den[i];
      total += rate_ratio[i];
    }

    for (i = 0; i < num_spatial_layers_; ++i) {
      config_->ss_target_bitrate[i] = static_cast<unsigned int>(
          config_->rc_target_bitrate * rate_ratio[i] / total);
      if (num_temporal_layers_ == 1) {
        config_->layer_target_bitrate[i] = config_->ss_target_bitrate[i];
      } else if (num_temporal_layers_ == 2) {
        config_->layer_target_bitrate[i * num_temporal_layers_] =
            config_->ss_target_bitrate[i] * 2 / 3;
        config_->layer_target_bitrate[i * num_temporal_layers_ + 1] =
            config_->ss_target_bitrate[i];
      } else if (num_temporal_layers_ == 3) {
        config_->layer_target_bitrate[i * num_temporal_layers_] =
            config_->ss_target_bitrate[i] / 2;
        config_->layer_target_bitrate[i * num_temporal_layers_ + 1] =
            config_->layer_target_bitrate[i * num_temporal_layers_] +
            (config_->ss_target_bitrate[i] / 4);
        config_->layer_target_bitrate[i * num_temporal_layers_ + 2] =
            config_->ss_target_bitrate[i];
      } else {
        LOG(LS_ERROR) << "Unsupported number of temporal layers: "
                      << num_temporal_layers_;
        return false;
      }
    }
  }

  // For now, temporal layers only supported when having one spatial layer.
  if (num_spatial_layers_ == 1) {
    for (i = 0; i < num_temporal_layers_; ++i) {
      config_->ts_target_bitrate[i] = config_->layer_target_bitrate[i];
    }
  }

  return true;
}

int VP9EncoderImpl::SetRates(uint32_t new_bitrate_kbit,
                             uint32_t new_framerate) {
  if (!inited_) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  if (encoder_->err) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  if (new_framerate < 1) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  // Update bit rate
  if (codec_.maxBitrate > 0 && new_bitrate_kbit > codec_.maxBitrate) {
    new_bitrate_kbit = codec_.maxBitrate;
  }
  config_->rc_target_bitrate = new_bitrate_kbit;
  codec_.maxFramerate = new_framerate;
  spatial_layer_->ConfigureBitrate(new_bitrate_kbit, 0);

  if (!SetSvcRates()) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }

  // Update encoder context
  if (vpx_codec_enc_config_set(encoder_, config_)) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP9EncoderImpl::InitEncode(const VideoCodec* inst,
                               int number_of_cores,
                               size_t /*max_payload_size*/) {
  if (inst == NULL) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (inst->maxFramerate < 1) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  // Allow zero to represent an unspecified maxBitRate
  if (inst->maxBitrate > 0 && inst->startBitrate > inst->maxBitrate) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (inst->width < 1 || inst->height < 1) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (number_of_cores < 1) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (inst->codecSpecific.VP9.numberOfTemporalLayers > 3) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  // libvpx currently supports only one or two spatial layers.
  if (inst->codecSpecific.VP9.numberOfSpatialLayers > 2) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }

  int retVal = Release();
  if (retVal < 0) {
    return retVal;
  }
  if (encoder_ == NULL) {
    encoder_ = new vpx_codec_ctx_t;
  }
  if (config_ == NULL) {
    config_ = new vpx_codec_enc_cfg_t;
  }
  timestamp_ = 0;
  if (&codec_ != inst) {
    codec_ = *inst;
  }

  num_spatial_layers_ = inst->codecSpecific.VP9.numberOfSpatialLayers;
  num_temporal_layers_ = inst->codecSpecific.VP9.numberOfTemporalLayers;
  if (num_temporal_layers_ == 0)
    num_temporal_layers_ = 1;

  // Random start 16 bits is enough.
  picture_id_ = static_cast<uint16_t>(rand()) & 0x7FFF;
  // Allocate memory for encoded image
  if (encoded_image_._buffer != NULL) {
    delete [] encoded_image_._buffer;
  }
  encoded_image_._size = CalcBufferSize(kI420, codec_.width, codec_.height);
  encoded_image_._buffer = new uint8_t[encoded_image_._size];
  encoded_image_._completeFrame = true;
  // Creating a wrapper to the image - setting image data to NULL. Actual
  // pointer will be set in encode. Setting align to 1, as it is meaningless
  // (actual memory is not allocated).
  raw_ = vpx_img_wrap(NULL, VPX_IMG_FMT_I420, codec_.width, codec_.height,
                      1, NULL);
  // Populate encoder configuration with default values.
  if (vpx_codec_enc_config_default(vpx_codec_vp9_cx(), config_, 0)) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  config_->g_w = codec_.width;
  config_->g_h = codec_.height;
  config_->rc_target_bitrate = inst->startBitrate;  // in kbit/s
  config_->g_error_resilient = 1;
  // Setting the time base of the codec.
  config_->g_timebase.num = 1;
  config_->g_timebase.den = 90000;
  config_->g_lag_in_frames = 0;  // 0- no frame lagging
  config_->g_threads = 1;
  // Rate control settings.
  config_->rc_dropframe_thresh = inst->codecSpecific.VP9.frameDroppingOn ?
      30 : 0;
  config_->rc_end_usage = VPX_CBR;
  config_->g_pass = VPX_RC_ONE_PASS;
  config_->rc_min_quantizer = 2;
  config_->rc_max_quantizer = 52;
  config_->rc_undershoot_pct = 50;
  config_->rc_overshoot_pct = 50;
  config_->rc_buf_initial_sz = 500;
  config_->rc_buf_optimal_sz = 600;
  config_->rc_buf_sz = 1000;
  // Set the maximum target size of any key-frame.
  rc_max_intra_target_ = MaxIntraTarget(config_->rc_buf_optimal_sz);
  if (inst->codecSpecific.VP9.keyFrameInterval  > 0) {
    config_->kf_mode = VPX_KF_AUTO;
    config_->kf_max_dist = inst->codecSpecific.VP9.keyFrameInterval;
  } else {
    config_->kf_mode = VPX_KF_DISABLED;
  }
  config_->rc_resize_allowed = inst->codecSpecific.VP9.automaticResizeOn ?
      1 : 0;
  // Determine number of threads based on the image size and #cores.
  config_->g_threads = NumberOfThreads(config_->g_w,
                                       config_->g_h,
                                       number_of_cores);

  cpu_speed_ = GetCpuSpeed(config_->g_w, config_->g_h);

  // TODO(asapersson): Check configuration of temporal switch up and increase
  // pattern length.
  is_flexible_mode_ = inst->codecSpecific.VP9.flexibleMode;
  if (is_flexible_mode_) {
    config_->temporal_layering_mode = VP9E_TEMPORAL_LAYERING_MODE_BYPASS;
    config_->ts_number_layers = num_temporal_layers_;
    if (codec_.mode == kScreensharing)
      spatial_layer_->ConfigureBitrate(inst->startBitrate, 0);
  } else if (num_temporal_layers_ == 1) {
    gof_.SetGofInfoVP9(kTemporalStructureMode1);
    config_->temporal_layering_mode = VP9E_TEMPORAL_LAYERING_MODE_NOLAYERING;
    config_->ts_number_layers = 1;
    config_->ts_rate_decimator[0] = 1;
    config_->ts_periodicity = 1;
    config_->ts_layer_id[0] = 0;
  } else if (num_temporal_layers_ == 2) {
    gof_.SetGofInfoVP9(kTemporalStructureMode2);
    config_->temporal_layering_mode = VP9E_TEMPORAL_LAYERING_MODE_0101;
    config_->ts_number_layers = 2;
    config_->ts_rate_decimator[0] = 2;
    config_->ts_rate_decimator[1] = 1;
    config_->ts_periodicity = 2;
    config_->ts_layer_id[0] = 0;
    config_->ts_layer_id[1] = 1;
  } else if (num_temporal_layers_ == 3) {
    gof_.SetGofInfoVP9(kTemporalStructureMode3);
    config_->temporal_layering_mode = VP9E_TEMPORAL_LAYERING_MODE_0212;
    config_->ts_number_layers = 3;
    config_->ts_rate_decimator[0] = 4;
    config_->ts_rate_decimator[1] = 2;
    config_->ts_rate_decimator[2] = 1;
    config_->ts_periodicity = 4;
    config_->ts_layer_id[0] = 0;
    config_->ts_layer_id[1] = 2;
    config_->ts_layer_id[2] = 1;
    config_->ts_layer_id[3] = 2;
  } else {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }

  tl0_pic_idx_ = static_cast<uint8_t>(rand());

  return InitAndSetControlSettings(inst);
}

int VP9EncoderImpl::NumberOfThreads(int width,
                                    int height,
                                    int number_of_cores) {
  // Keep the number of encoder threads equal to the possible number of column
  // tiles, which is (1, 2, 4, 8). See comments below for VP9E_SET_TILE_COLUMNS.
  if (width * height >= 1280 * 720 && number_of_cores > 4) {
    return 4;
  } else if (width * height >= 640 * 480 && number_of_cores > 2) {
    return 2;
  } else {
    // 1 thread less than VGA.
    return 1;
  }
}

int VP9EncoderImpl::InitAndSetControlSettings(const VideoCodec* inst) {
  config_->ss_number_layers = num_spatial_layers_;

  if (ExplicitlyConfiguredSpatialLayers()) {
    for (int i = 0; i < num_spatial_layers_; ++i) {
      const auto& layer = codec_.spatialLayers[i];
      svc_internal_.svc_params.max_quantizers[i] = config_->rc_max_quantizer;
      svc_internal_.svc_params.min_quantizers[i] = config_->rc_min_quantizer;
      svc_internal_.svc_params.scaling_factor_num[i] = layer.scaling_factor_num;
      svc_internal_.svc_params.scaling_factor_den[i] = layer.scaling_factor_den;
    }
  } else {
    int scaling_factor_num = 256;
    for (int i = num_spatial_layers_ - 1; i >= 0; --i) {
      svc_internal_.svc_params.max_quantizers[i] = config_->rc_max_quantizer;
      svc_internal_.svc_params.min_quantizers[i] = config_->rc_min_quantizer;
      // 1:2 scaling in each dimension.
      svc_internal_.svc_params.scaling_factor_num[i] = scaling_factor_num;
      svc_internal_.svc_params.scaling_factor_den[i] = 256;
      if (codec_.mode != kScreensharing)
        scaling_factor_num /= 2;
    }
  }

  if (!SetSvcRates()) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }

  if (vpx_codec_enc_init(encoder_, vpx_codec_vp9_cx(), config_, 0)) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  vpx_codec_control(encoder_, VP8E_SET_CPUUSED, cpu_speed_);
  vpx_codec_control(encoder_, VP8E_SET_MAX_INTRA_BITRATE_PCT,
                    rc_max_intra_target_);
  vpx_codec_control(encoder_, VP9E_SET_AQ_MODE,
                    inst->codecSpecific.VP9.adaptiveQpMode ? 3 : 0);

  vpx_codec_control(
      encoder_, VP9E_SET_SVC,
      (num_temporal_layers_ > 1 || num_spatial_layers_ > 1) ? 1 : 0);
  if (num_temporal_layers_ > 1 || num_spatial_layers_ > 1) {
    vpx_codec_control(encoder_, VP9E_SET_SVC_PARAMETERS,
                      &svc_internal_.svc_params);
  }
  // Register callback for getting each spatial layer.
  vpx_codec_priv_output_cx_pkt_cb_pair_t cbp = {
      VP9EncoderImpl::EncoderOutputCodedPacketCallback, (void*)(this)};
  vpx_codec_control(encoder_, VP9E_REGISTER_CX_CALLBACK, (void*)(&cbp));

  // Control function to set the number of column tiles in encoding a frame, in
  // log2 unit: e.g., 0 = 1 tile column, 1 = 2 tile columns, 2 = 4 tile columns.
  // The number tile columns will be capped by the encoder based on image size
  // (minimum width of tile column is 256 pixels, maximum is 4096).
  vpx_codec_control(encoder_, VP9E_SET_TILE_COLUMNS, (config_->g_threads >> 1));
#if !defined(WEBRTC_ARCH_ARM) && !defined(WEBRTC_ARCH_ARM64)
  // Note denoiser is still off by default until further testing/optimization,
  // i.e., codecSpecific.VP9.denoisingOn == 0.
  vpx_codec_control(encoder_, VP9E_SET_NOISE_SENSITIVITY,
                    inst->codecSpecific.VP9.denoisingOn ? 1 : 0);
#endif
  if (codec_.mode == kScreensharing) {
    // Adjust internal parameters to screen content.
    vpx_codec_control(encoder_, VP9E_SET_TUNE_CONTENT, 1);
  }
  // Enable encoder skip of static/low content blocks.
  vpx_codec_control(encoder_, VP8E_SET_STATIC_THRESHOLD, 1);
  inited_ = true;
  return WEBRTC_VIDEO_CODEC_OK;
}

uint32_t VP9EncoderImpl::MaxIntraTarget(uint32_t optimal_buffer_size) {
  // Set max to the optimal buffer level (normalized by target BR),
  // and scaled by a scale_par.
  // Max target size = scale_par * optimal_buffer_size * targetBR[Kbps].
  // This value is presented in percentage of perFrameBw:
  // perFrameBw = targetBR[Kbps] * 1000 / framerate.
  // The target in % is as follows:
  float scale_par = 0.5;
  uint32_t target_pct =
      optimal_buffer_size * scale_par * codec_.maxFramerate / 10;
  // Don't go below 3 times the per frame bandwidth.
  const uint32_t min_intra_size = 300;
  return (target_pct < min_intra_size) ? min_intra_size: target_pct;
}

int VP9EncoderImpl::Encode(const VideoFrame& input_image,
                           const CodecSpecificInfo* codec_specific_info,
                           const std::vector<FrameType>* frame_types) {
  if (!inited_) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  if (input_image.IsZeroSize()) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (encoded_complete_callback_ == NULL) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  FrameType frame_type = kVideoFrameDelta;
  // We only support one stream at the moment.
  if (frame_types && frame_types->size() > 0) {
    frame_type = (*frame_types)[0];
  }
  RTC_DCHECK_EQ(input_image.width(), static_cast<int>(raw_->d_w));
  RTC_DCHECK_EQ(input_image.height(), static_cast<int>(raw_->d_h));

  // Set input image for use in the callback.
  // This was necessary since you need some information from input_image.
  // You can save only the necessary information (such as timestamp) instead of
  // doing this.
  input_image_ = &input_image;

  // Image in vpx_image_t format.
  // Input image is const. VPX's raw image is not defined as const.
  raw_->planes[VPX_PLANE_Y] = const_cast<uint8_t*>(input_image.buffer(kYPlane));
  raw_->planes[VPX_PLANE_U] = const_cast<uint8_t*>(input_image.buffer(kUPlane));
  raw_->planes[VPX_PLANE_V] = const_cast<uint8_t*>(input_image.buffer(kVPlane));
  raw_->stride[VPX_PLANE_Y] = input_image.stride(kYPlane);
  raw_->stride[VPX_PLANE_U] = input_image.stride(kUPlane);
  raw_->stride[VPX_PLANE_V] = input_image.stride(kVPlane);

  vpx_enc_frame_flags_t flags = 0;
  bool send_keyframe = (frame_type == kVideoFrameKey);
  if (send_keyframe) {
    // Key frame request from caller.
    flags = VPX_EFLAG_FORCE_KF;
  }

  if (is_flexible_mode_) {
    SuperFrameRefSettings settings;

    // These structs are copied when calling vpx_codec_control,
    // therefore it is ok for them to go out of scope.
    vpx_svc_ref_frame_config enc_layer_conf;
    vpx_svc_layer_id layer_id;

    if (codec_.mode == kRealtimeVideo) {
      // Real time video not yet implemented in flexible mode.
      RTC_NOTREACHED();
    } else {
      settings = spatial_layer_->GetSuperFrameSettings(input_image.timestamp(),
                                                       send_keyframe);
    }
    enc_layer_conf = GenerateRefsAndFlags(settings);
    layer_id.temporal_layer_id = 0;
    layer_id.spatial_layer_id = settings.start_layer;
    vpx_codec_control(encoder_, VP9E_SET_SVC_LAYER_ID, &layer_id);
    vpx_codec_control(encoder_, VP9E_SET_SVC_REF_FRAME_CONFIG, &enc_layer_conf);
  }

  assert(codec_.maxFramerate > 0);
  uint32_t duration = 90000 / codec_.maxFramerate;
  if (vpx_codec_encode(encoder_, raw_, timestamp_, duration, flags,
                       VPX_DL_REALTIME)) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  timestamp_ += duration;

  return WEBRTC_VIDEO_CODEC_OK;
}

void VP9EncoderImpl::PopulateCodecSpecific(CodecSpecificInfo* codec_specific,
                                       const vpx_codec_cx_pkt& pkt,
                                       uint32_t timestamp) {
  assert(codec_specific != NULL);
  codec_specific->codecType = kVideoCodecVP9;
  CodecSpecificInfoVP9 *vp9_info = &(codec_specific->codecSpecific.VP9);
  // TODO(asapersson): Set correct values.
  vp9_info->inter_pic_predicted =
      (pkt.data.frame.flags & VPX_FRAME_IS_KEY) ? false : true;
  vp9_info->flexible_mode = codec_.codecSpecific.VP9.flexibleMode;
  vp9_info->ss_data_available = ((pkt.data.frame.flags & VPX_FRAME_IS_KEY) &&
                                 !codec_.codecSpecific.VP9.flexibleMode)
                                    ? true
                                    : false;

  vpx_svc_layer_id_t layer_id = {0};
  vpx_codec_control(encoder_, VP9E_GET_SVC_LAYER_ID, &layer_id);

  assert(num_temporal_layers_ > 0);
  assert(num_spatial_layers_ > 0);
  if (num_temporal_layers_ == 1) {
    assert(layer_id.temporal_layer_id == 0);
    vp9_info->temporal_idx = kNoTemporalIdx;
  } else {
    vp9_info->temporal_idx = layer_id.temporal_layer_id;
  }
  if (num_spatial_layers_ == 1) {
    assert(layer_id.spatial_layer_id == 0);
    vp9_info->spatial_idx = kNoSpatialIdx;
  } else {
    vp9_info->spatial_idx = layer_id.spatial_layer_id;
  }
  if (layer_id.spatial_layer_id != 0) {
    vp9_info->ss_data_available = false;
  }

  // TODO(asapersson): this info has to be obtained from the encoder.
  vp9_info->temporal_up_switch = false;

  bool is_first_frame = false;
  if (is_flexible_mode_) {
    is_first_frame =
        layer_id.spatial_layer_id == spatial_layer_->GetStartLayer();
  } else {
    is_first_frame = layer_id.spatial_layer_id == 0;
  }

  if (is_first_frame) {
    picture_id_ = (picture_id_ + 1) & 0x7FFF;
    // TODO(asapersson): this info has to be obtained from the encoder.
    vp9_info->inter_layer_predicted = false;
    ++frames_since_kf_;
  } else {
    // TODO(asapersson): this info has to be obtained from the encoder.
    vp9_info->inter_layer_predicted = true;
  }

  if (pkt.data.frame.flags & VPX_FRAME_IS_KEY) {
    frames_since_kf_ = 0;
  }

  vp9_info->picture_id = picture_id_;

  if (!vp9_info->flexible_mode) {
    if (layer_id.temporal_layer_id == 0 && layer_id.spatial_layer_id == 0) {
      tl0_pic_idx_++;
    }
    vp9_info->tl0_pic_idx = tl0_pic_idx_;
  }

  // Always populate this, so that the packetizer can properly set the marker
  // bit.
  vp9_info->num_spatial_layers = num_spatial_layers_;

  vp9_info->num_ref_pics = 0;
  if (vp9_info->flexible_mode) {
    vp9_info->gof_idx = kNoGofIdx;
    vp9_info->num_ref_pics = num_ref_pics_[layer_id.spatial_layer_id];
    for (int i = 0; i < num_ref_pics_[layer_id.spatial_layer_id]; ++i) {
      vp9_info->p_diff[i] = p_diff_[layer_id.spatial_layer_id][i];
    }
  } else {
    vp9_info->gof_idx =
        static_cast<uint8_t>(frames_since_kf_ % gof_.num_frames_in_gof);
    vp9_info->temporal_up_switch = gof_.temporal_up_switch[vp9_info->gof_idx];
  }

  if (vp9_info->ss_data_available) {
    vp9_info->spatial_layer_resolution_present = true;
    for (size_t i = 0; i < vp9_info->num_spatial_layers; ++i) {
      vp9_info->width[i] = codec_.width *
                           svc_internal_.svc_params.scaling_factor_num[i] /
                           svc_internal_.svc_params.scaling_factor_den[i];
      vp9_info->height[i] = codec_.height *
                            svc_internal_.svc_params.scaling_factor_num[i] /
                            svc_internal_.svc_params.scaling_factor_den[i];
    }
    if (!vp9_info->flexible_mode) {
      vp9_info->gof.CopyGofInfoVP9(gof_);
    }
  }
}

int VP9EncoderImpl::GetEncodedLayerFrame(const vpx_codec_cx_pkt* pkt) {
  encoded_image_._length = 0;
  encoded_image_._frameType = kVideoFrameDelta;
  RTPFragmentationHeader frag_info;
  // Note: no data partitioning in VP9, so 1 partition only. We keep this
  // fragmentation data for now, until VP9 packetizer is implemented.
  frag_info.VerifyAndAllocateFragmentationHeader(1);
  int part_idx = 0;
  CodecSpecificInfo codec_specific;

  assert(pkt->kind == VPX_CODEC_CX_FRAME_PKT);
  memcpy(&encoded_image_._buffer[encoded_image_._length], pkt->data.frame.buf,
         pkt->data.frame.sz);
  frag_info.fragmentationOffset[part_idx] = encoded_image_._length;
  frag_info.fragmentationLength[part_idx] =
      static_cast<uint32_t>(pkt->data.frame.sz);
  frag_info.fragmentationPlType[part_idx] = 0;
  frag_info.fragmentationTimeDiff[part_idx] = 0;
  encoded_image_._length += static_cast<uint32_t>(pkt->data.frame.sz);

  vpx_svc_layer_id_t layer_id = {0};
  vpx_codec_control(encoder_, VP9E_GET_SVC_LAYER_ID, &layer_id);
  if (is_flexible_mode_ && codec_.mode == kScreensharing)
    spatial_layer_->LayerFrameEncoded(
        static_cast<unsigned int>(encoded_image_._length),
        layer_id.spatial_layer_id);

  assert(encoded_image_._length <= encoded_image_._size);

  // End of frame.
  // Check if encoded frame is a key frame.
  if (pkt->data.frame.flags & VPX_FRAME_IS_KEY) {
    encoded_image_._frameType = kVideoFrameKey;
  }
  PopulateCodecSpecific(&codec_specific, *pkt, input_image_->timestamp());

  if (encoded_image_._length > 0) {
    TRACE_COUNTER1("webrtc", "EncodedFrameSize", encoded_image_._length);
    encoded_image_._timeStamp = input_image_->timestamp();
    encoded_image_.capture_time_ms_ = input_image_->render_time_ms();
    encoded_image_._encodedHeight = raw_->d_h;
    encoded_image_._encodedWidth = raw_->d_w;
    encoded_complete_callback_->Encoded(encoded_image_, &codec_specific,
                                        &frag_info);
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

vpx_svc_ref_frame_config VP9EncoderImpl::GenerateRefsAndFlags(
    const SuperFrameRefSettings& settings) {
  static const vpx_enc_frame_flags_t kAllFlags =
      VP8_EFLAG_NO_REF_ARF | VP8_EFLAG_NO_REF_GF | VP8_EFLAG_NO_REF_LAST |
      VP8_EFLAG_NO_UPD_LAST | VP8_EFLAG_NO_UPD_ARF | VP8_EFLAG_NO_UPD_GF;
  vpx_svc_ref_frame_config sf_conf = {};
  if (settings.is_keyframe) {
    // Used later on to make sure we don't make any invalid references.
    memset(buffer_updated_at_frame_, -1, sizeof(buffer_updated_at_frame_));
    for (int layer = settings.start_layer; layer <= settings.stop_layer;
         ++layer) {
      num_ref_pics_[layer] = 0;
      buffer_updated_at_frame_[settings.layer[layer].upd_buf] = frames_encoded_;
      // When encoding a keyframe only the alt_fb_idx is used
      // to specify which layer ends up in which buffer.
      sf_conf.alt_fb_idx[layer] = settings.layer[layer].upd_buf;
    }
  } else {
    for (int layer_idx = settings.start_layer; layer_idx <= settings.stop_layer;
         ++layer_idx) {
      vpx_enc_frame_flags_t layer_flags = kAllFlags;
      num_ref_pics_[layer_idx] = 0;
      int8_t refs[3] = {settings.layer[layer_idx].ref_buf1,
                        settings.layer[layer_idx].ref_buf2,
                        settings.layer[layer_idx].ref_buf3};

      for (unsigned int ref_idx = 0; ref_idx < kMaxVp9RefPics; ++ref_idx) {
        if (refs[ref_idx] == -1)
          continue;

        RTC_DCHECK_GE(refs[ref_idx], 0);
        RTC_DCHECK_LE(refs[ref_idx], 7);
        // Easier to remove flags from all flags rather than having to
        // build the flags from 0.
        switch (num_ref_pics_[layer_idx]) {
          case 0: {
            sf_conf.lst_fb_idx[layer_idx] = refs[ref_idx];
            layer_flags &= ~VP8_EFLAG_NO_REF_LAST;
            break;
          }
          case 1: {
            sf_conf.gld_fb_idx[layer_idx] = refs[ref_idx];
            layer_flags &= ~VP8_EFLAG_NO_REF_GF;
            break;
          }
          case 2: {
            sf_conf.alt_fb_idx[layer_idx] = refs[ref_idx];
            layer_flags &= ~VP8_EFLAG_NO_REF_ARF;
            break;
          }
        }
        // Make sure we don't reference a buffer that hasn't been
        // used at all or hasn't been used since a keyframe.
        RTC_DCHECK_NE(buffer_updated_at_frame_[refs[ref_idx]], -1);

        p_diff_[layer_idx][num_ref_pics_[layer_idx]] =
            frames_encoded_ - buffer_updated_at_frame_[refs[ref_idx]];
        num_ref_pics_[layer_idx]++;
      }

      bool upd_buf_same_as_a_ref = false;
      if (settings.layer[layer_idx].upd_buf != -1) {
        for (unsigned int ref_idx = 0; ref_idx < kMaxVp9RefPics; ++ref_idx) {
          if (settings.layer[layer_idx].upd_buf == refs[ref_idx]) {
            switch (ref_idx) {
              case 0: {
                layer_flags &= ~VP8_EFLAG_NO_UPD_LAST;
                break;
              }
              case 1: {
                layer_flags &= ~VP8_EFLAG_NO_UPD_GF;
                break;
              }
              case 2: {
                layer_flags &= ~VP8_EFLAG_NO_UPD_ARF;
                break;
              }
            }
            upd_buf_same_as_a_ref = true;
            break;
          }
        }
        if (!upd_buf_same_as_a_ref) {
          // If we have three references and a buffer is specified to be
          // updated, then that buffer must be the same as one of the
          // three references.
          RTC_CHECK_LT(num_ref_pics_[layer_idx], kMaxVp9RefPics);

          sf_conf.alt_fb_idx[layer_idx] = settings.layer[layer_idx].upd_buf;
          layer_flags ^= VP8_EFLAG_NO_UPD_ARF;
        }

        int updated_buffer = settings.layer[layer_idx].upd_buf;
        buffer_updated_at_frame_[updated_buffer] = frames_encoded_;
        sf_conf.frame_flags[layer_idx] = layer_flags;
      }
    }
  }
  ++frames_encoded_;
  return sf_conf;
}

int VP9EncoderImpl::SetChannelParameters(uint32_t packet_loss, int64_t rtt) {
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP9EncoderImpl::RegisterEncodeCompleteCallback(
    EncodedImageCallback* callback) {
  encoded_complete_callback_ = callback;
  return WEBRTC_VIDEO_CODEC_OK;
}

VP9Decoder* VP9Decoder::Create() {
  return new VP9DecoderImpl();
}

VP9DecoderImpl::VP9DecoderImpl()
    : decode_complete_callback_(NULL),
      inited_(false),
      decoder_(NULL),
      key_frame_required_(true) {
  memset(&codec_, 0, sizeof(codec_));
}

VP9DecoderImpl::~VP9DecoderImpl() {
  inited_ = true;  // in order to do the actual release
  Release();
  int num_buffers_in_use = frame_buffer_pool_.GetNumBuffersInUse();
  if (num_buffers_in_use > 0) {
    // The frame buffers are reference counted and frames are exposed after
    // decoding. There may be valid usage cases where previous frames are still
    // referenced after ~VP9DecoderImpl that is not a leak.
    LOG(LS_INFO) << num_buffers_in_use << " Vp9FrameBuffers are still "
                 << "referenced during ~VP9DecoderImpl.";
  }
}

int VP9DecoderImpl::Reset() {
  if (!inited_) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  InitDecode(&codec_, 1);
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP9DecoderImpl::InitDecode(const VideoCodec* inst, int number_of_cores) {
  if (inst == NULL) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  int ret_val = Release();
  if (ret_val < 0) {
    return ret_val;
  }
  if (decoder_ == NULL) {
    decoder_ = new vpx_codec_ctx_t;
  }
  vpx_codec_dec_cfg_t  cfg;
  // Setting number of threads to a constant value (1)
  cfg.threads = 1;
  cfg.h = cfg.w = 0;  // set after decode
  vpx_codec_flags_t flags = 0;
  if (vpx_codec_dec_init(decoder_, vpx_codec_vp9_dx(), &cfg, flags)) {
    return WEBRTC_VIDEO_CODEC_MEMORY;
  }
  if (&codec_ != inst) {
    // Save VideoCodec instance for later; mainly for duplicating the decoder.
    codec_ = *inst;
  }

  if (!frame_buffer_pool_.InitializeVpxUsePool(decoder_)) {
    return WEBRTC_VIDEO_CODEC_MEMORY;
  }

  inited_ = true;
  // Always start with a complete key frame.
  key_frame_required_ = true;
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP9DecoderImpl::Decode(const EncodedImage& input_image,
                           bool missing_frames,
                           const RTPFragmentationHeader* fragmentation,
                           const CodecSpecificInfo* codec_specific_info,
                           int64_t /*render_time_ms*/) {
  if (!inited_) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  if (decode_complete_callback_ == NULL) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  // Always start with a complete key frame.
  if (key_frame_required_) {
    if (input_image._frameType != kVideoFrameKey)
      return WEBRTC_VIDEO_CODEC_ERROR;
    // We have a key frame - is it complete?
    if (input_image._completeFrame) {
      key_frame_required_ = false;
    } else {
      return WEBRTC_VIDEO_CODEC_ERROR;
    }
  }
  vpx_codec_iter_t iter = NULL;
  vpx_image_t* img;
  uint8_t* buffer = input_image._buffer;
  if (input_image._length == 0) {
    buffer = NULL;  // Triggers full frame concealment.
  }
  // During decode libvpx may get and release buffers from |frame_buffer_pool_|.
  // In practice libvpx keeps a few (~3-4) buffers alive at a time.
  if (vpx_codec_decode(decoder_,
                       buffer,
                       static_cast<unsigned int>(input_image._length),
                       0,
                       VPX_DL_REALTIME)) {
    return WEBRTC_VIDEO_CODEC_ERROR;
  }
  // |img->fb_priv| contains the image data, a reference counted Vp9FrameBuffer.
  // It may be released by libvpx during future vpx_codec_decode or
  // vpx_codec_destroy calls.
  img = vpx_codec_get_frame(decoder_, &iter);
  int ret = ReturnFrame(img, input_image._timeStamp);
  if (ret != 0) {
    return ret;
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP9DecoderImpl::ReturnFrame(const vpx_image_t* img, uint32_t timestamp) {
  if (img == NULL) {
    // Decoder OK and NULL image => No show frame.
    return WEBRTC_VIDEO_CODEC_NO_OUTPUT;
  }

  // This buffer contains all of |img|'s image data, a reference counted
  // Vp9FrameBuffer. Performing AddRef/Release ensures it is not released and
  // recycled during use (libvpx is done with the buffers after a few
  // vpx_codec_decode calls or vpx_codec_destroy).
  Vp9FrameBufferPool::Vp9FrameBuffer* img_buffer =
      static_cast<Vp9FrameBufferPool::Vp9FrameBuffer*>(img->fb_priv);
  img_buffer->AddRef();
  // The buffer can be used directly by the VideoFrame (without copy) by
  // using a WrappedI420Buffer.
  rtc::scoped_refptr<WrappedI420Buffer> img_wrapped_buffer(
      new rtc::RefCountedObject<webrtc::WrappedI420Buffer>(
          img->d_w, img->d_h,
          img->planes[VPX_PLANE_Y], img->stride[VPX_PLANE_Y],
          img->planes[VPX_PLANE_U], img->stride[VPX_PLANE_U],
          img->planes[VPX_PLANE_V], img->stride[VPX_PLANE_V],
          // WrappedI420Buffer's mechanism for allowing the release of its frame
          // buffer is through a callback function. This is where we should
          // release |img_buffer|.
          rtc::Bind(&WrappedI420BufferNoLongerUsedCb, img_buffer)));

  VideoFrame decoded_image;
  decoded_image.set_video_frame_buffer(img_wrapped_buffer);
  decoded_image.set_timestamp(timestamp);
  int ret = decode_complete_callback_->Decoded(decoded_image);
  if (ret != 0)
    return ret;
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP9DecoderImpl::RegisterDecodeCompleteCallback(
    DecodedImageCallback* callback) {
  decode_complete_callback_ = callback;
  return WEBRTC_VIDEO_CODEC_OK;
}

int VP9DecoderImpl::Release() {
  if (decoder_ != NULL) {
    // When a codec is destroyed libvpx will release any buffers of
    // |frame_buffer_pool_| it is currently using.
    if (vpx_codec_destroy(decoder_)) {
      return WEBRTC_VIDEO_CODEC_MEMORY;
    }
    delete decoder_;
    decoder_ = NULL;
  }
  // Releases buffers from the pool. Any buffers not in use are deleted. Buffers
  // still referenced externally are deleted once fully released, not returning
  // to the pool.
  frame_buffer_pool_.ClearPool();
  inited_ = false;
  return WEBRTC_VIDEO_CODEC_OK;
}
}  // namespace webrtc
