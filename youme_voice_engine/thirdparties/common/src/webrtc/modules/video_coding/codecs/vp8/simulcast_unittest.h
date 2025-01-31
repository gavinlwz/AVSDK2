﻿/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_SIMULCAST_UNITTEST_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_SIMULCAST_UNITTEST_H_

#include <algorithm>
#include <vector>

#include "webrtc/base/checks.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/common.h"
#include "webrtc/common_video/libyuv/include/webrtc_libyuv.h"
#include "webrtc/modules/video_coding/include/mock/mock_video_codec_interface.h"
#include "webrtc/modules/video_coding/codecs/vp8/include/vp8.h"
#include "webrtc/modules/video_coding/codecs/vp8/temporal_layers.h"
#include "webrtc/video_frame.h"

#include "gtest/gtest.h"

using ::testing::_;
using ::testing::AllOf;
using ::testing::Field;
using ::testing::Return;

namespace webrtc {
namespace testing {

const int kDefaultWidth = 1280;
const int kDefaultHeight = 720;
const int kNumberOfSimulcastStreams = 3;
const int kColorY = 66;
const int kColorU = 22;
const int kColorV = 33;
const int kMaxBitrates[kNumberOfSimulcastStreams] = {150, 600, 1200};
const int kMinBitrates[kNumberOfSimulcastStreams] = {50, 150, 600};
const int kTargetBitrates[kNumberOfSimulcastStreams] = {100, 450, 1000};
const int kDefaultTemporalLayerProfile[3] = {3, 3, 3};

template<typename T> void SetExpectedValues3(T value0,
                                             T value1,
                                             T value2,
                                             T* expected_values) {
  expected_values[0] = value0;
  expected_values[1] = value1;
  expected_values[2] = value2;
}

class Vp8TestEncodedImageCallback : public EncodedImageCallback {
 public:
  Vp8TestEncodedImageCallback()
       : picture_id_(-1) {
    memset(temporal_layer_, -1, sizeof(temporal_layer_));
    memset(layer_sync_, false, sizeof(layer_sync_));
  }

  ~Vp8TestEncodedImageCallback() {
    delete [] encoded_key_frame_._buffer;
    delete [] encoded_frame_._buffer;
  }

  virtual int32_t Encoded(const EncodedImage& encoded_image,
                          const CodecSpecificInfo* codec_specific_info,
                          const RTPFragmentationHeader* fragmentation) {
    // Only store the base layer.
    if (codec_specific_info->codecSpecific.VP8.simulcastIdx == 0) {
      if (encoded_image._frameType == kVideoFrameKey) {
        delete [] encoded_key_frame_._buffer;
        encoded_key_frame_._buffer = new uint8_t[encoded_image._size];
        encoded_key_frame_._size = encoded_image._size;
        encoded_key_frame_._length = encoded_image._length;
        encoded_key_frame_._frameType = kVideoFrameKey;
        encoded_key_frame_._completeFrame = encoded_image._completeFrame;
        memcpy(encoded_key_frame_._buffer,
               encoded_image._buffer,
               encoded_image._length);
      } else {
        delete [] encoded_frame_._buffer;
        encoded_frame_._buffer = new uint8_t[encoded_image._size];
        encoded_frame_._size = encoded_image._size;
        encoded_frame_._length = encoded_image._length;
        memcpy(encoded_frame_._buffer,
               encoded_image._buffer,
               encoded_image._length);
      }
    }
    picture_id_ = codec_specific_info->codecSpecific.VP8.pictureId;
    layer_sync_[codec_specific_info->codecSpecific.VP8.simulcastIdx] =
        codec_specific_info->codecSpecific.VP8.layerSync;
    temporal_layer_[codec_specific_info->codecSpecific.VP8.simulcastIdx] =
        codec_specific_info->codecSpecific.VP8.temporalIdx;
    return 0;
  }
  void GetLastEncodedFrameInfo(int* picture_id, int* temporal_layer,
                               bool* layer_sync, int stream) {
    *picture_id = picture_id_;
    *temporal_layer = temporal_layer_[stream];
    *layer_sync = layer_sync_[stream];
  }
  void GetLastEncodedKeyFrame(EncodedImage* encoded_key_frame) {
    *encoded_key_frame = encoded_key_frame_;
  }
  void GetLastEncodedFrame(EncodedImage* encoded_frame) {
    *encoded_frame = encoded_frame_;
  }

 private:
  EncodedImage encoded_key_frame_;
  EncodedImage encoded_frame_;
  int picture_id_;
  int temporal_layer_[kNumberOfSimulcastStreams];
  bool layer_sync_[kNumberOfSimulcastStreams];
};

class Vp8TestDecodedImageCallback : public DecodedImageCallback {
 public:
  Vp8TestDecodedImageCallback()
      : decoded_frames_(0) {
  }
  int32_t Decoded(VideoFrame& decoded_image) override {
    for (int i = 0; i < decoded_image.width(); ++i) {
      EXPECT_NEAR(kColorY, decoded_image.buffer(kYPlane)[i], 1);
    }

    // TODO(mikhal): Verify the difference between U,V and the original.
    for (int i = 0; i < ((decoded_image.width() + 1) / 2); ++i) {
      EXPECT_NEAR(kColorU, decoded_image.buffer(kUPlane)[i], 4);
      EXPECT_NEAR(kColorV, decoded_image.buffer(kVPlane)[i], 4);
    }
    decoded_frames_++;
    return 0;
  }
  int32_t Decoded(VideoFrame& decoded_image, int64_t decode_time_ms) override {
    RTC_NOTREACHED();
    return -1;
  }
  int DecodedFrames() {
    return decoded_frames_;
  }

 private:
  int decoded_frames_;
};

class SkipEncodingUnusedStreamsTest {
 public:
  std::vector<unsigned int> RunTest(VP8Encoder* encoder,
                                    VideoCodec* settings,
                                    uint32_t target_bitrate) {
    Config options;
    SpyingTemporalLayersFactory* spy_factory =
        new SpyingTemporalLayersFactory();
    options.Set<TemporalLayers::Factory>(spy_factory);
    settings->extra_options = &options;
    EXPECT_EQ(0, encoder->InitEncode(settings, 1, 1200));

    encoder->SetRates(target_bitrate, 30);

    std::vector<unsigned int> configured_bitrates;
    for (std::vector<TemporalLayers*>::const_iterator it =
             spy_factory->spying_layers_.begin();
         it != spy_factory->spying_layers_.end();
         ++it) {
      configured_bitrates.push_back(
          static_cast<SpyingTemporalLayers*>(*it)->configured_bitrate_);
    }
    return configured_bitrates;
  }

  class SpyingTemporalLayers : public TemporalLayers {
   public:
    explicit SpyingTemporalLayers(TemporalLayers* layers)
        : configured_bitrate_(0), layers_(layers) {}

    virtual ~SpyingTemporalLayers() { delete layers_; }

    virtual int EncodeFlags(uint32_t timestamp) {
      return layers_->EncodeFlags(timestamp);
    }

    bool ConfigureBitrates(int bitrate_kbit,
                           int max_bitrate_kbit,
                           int framerate,
                           vpx_codec_enc_cfg_t* cfg) override {
      configured_bitrate_ = bitrate_kbit;
      return layers_->ConfigureBitrates(
          bitrate_kbit, max_bitrate_kbit, framerate, cfg);
    }

    void PopulateCodecSpecific(bool base_layer_sync,
                               CodecSpecificInfoVP8* vp8_info,
                               uint32_t timestamp) override {
      layers_->PopulateCodecSpecific(base_layer_sync, vp8_info, timestamp);
    }

    void FrameEncoded(unsigned int size, uint32_t timestamp, int qp) override {
      layers_->FrameEncoded(size, timestamp, qp);
    }

    int CurrentLayerId() const override { return layers_->CurrentLayerId(); }

    bool UpdateConfiguration(vpx_codec_enc_cfg_t* cfg) override {
      return false;
    }

    int configured_bitrate_;
    TemporalLayers* layers_;
  };

  class SpyingTemporalLayersFactory : public TemporalLayers::Factory {
   public:
    virtual ~SpyingTemporalLayersFactory() {}
    TemporalLayers* Create(int temporal_layers,
                           uint8_t initial_tl0_pic_idx) const override {
      SpyingTemporalLayers* layers =
          new SpyingTemporalLayers(TemporalLayers::Factory::Create(
              temporal_layers, initial_tl0_pic_idx));
      spying_layers_.push_back(layers);
      return layers;
    }

    mutable std::vector<TemporalLayers*> spying_layers_;
  };
};

class TestVp8Simulcast : public ::testing::Test {
 public:
  TestVp8Simulcast(VP8Encoder* encoder, VP8Decoder* decoder)
     : encoder_(encoder),
       decoder_(decoder) {}

  // Creates an VideoFrame from |plane_colors|.
  static void CreateImage(VideoFrame* frame, int plane_colors[kNumOfPlanes]) {
    for (int plane_num = 0; plane_num < kNumOfPlanes; ++plane_num) {
      int width = (plane_num != kYPlane ? (frame->width() + 1) / 2 :
        frame->width());
      int height = (plane_num != kYPlane ? (frame->height() + 1) / 2 :
        frame->height());
      PlaneType plane_type = static_cast<PlaneType>(plane_num);
      uint8_t* data = frame->buffer(plane_type);
      // Setting allocated area to zero - setting only image size to
      // requested values - will make it easier to distinguish between image
      // size and frame size (accounting for stride).
      memset(frame->buffer(plane_type), 0, frame->allocated_size(plane_type));
      for (int i = 0; i < height; i++) {
        memset(data, plane_colors[plane_num], width);
        data += frame->stride(plane_type);
      }
    }
  }

  static void DefaultSettings(VideoCodec* settings,
                              const int* temporal_layer_profile) {
    assert(settings);
    memset(settings, 0, sizeof(VideoCodec));
    strncpy(settings->plName, "VP8", 4);
    settings->codecType = kVideoCodecVP8;
    // 96 to 127 dynamic payload types for video codecs
    settings->plType = 120;
    settings->startBitrate = 300;
    settings->minBitrate = 30;
    settings->maxBitrate = 0;
    settings->maxFramerate = 30;
    settings->width = kDefaultWidth;
    settings->height = kDefaultHeight;
    settings->numberOfSimulcastStreams = kNumberOfSimulcastStreams;
    ASSERT_EQ(3, kNumberOfSimulcastStreams);
    ConfigureStream(kDefaultWidth / 4, kDefaultHeight / 4,
                    kMaxBitrates[0],
                    kMinBitrates[0],
                    kTargetBitrates[0],
                    &settings->simulcastStream[0],
                    temporal_layer_profile[0]);
    ConfigureStream(kDefaultWidth / 2, kDefaultHeight / 2,
                    kMaxBitrates[1],
                    kMinBitrates[1],
                    kTargetBitrates[1],
                    &settings->simulcastStream[1],
                    temporal_layer_profile[1]);
    ConfigureStream(kDefaultWidth, kDefaultHeight,
                    kMaxBitrates[2],
                    kMinBitrates[2],
                    kTargetBitrates[2],
                    &settings->simulcastStream[2],
                    temporal_layer_profile[2]);
    settings->codecSpecific.VP8.resilience = kResilientStream;
    settings->codecSpecific.VP8.denoisingOn = true;
    settings->codecSpecific.VP8.errorConcealmentOn = false;
    settings->codecSpecific.VP8.automaticResizeOn = false;
    settings->codecSpecific.VP8.feedbackModeOn = false;
    settings->codecSpecific.VP8.frameDroppingOn = true;
    settings->codecSpecific.VP8.keyFrameInterval = 3000;
  }

  static void ConfigureStream(int width,
                              int height,
                              int max_bitrate,
                              int min_bitrate,
                              int target_bitrate,
                              SimulcastStream* stream,
                              int num_temporal_layers) {
    assert(stream);
    stream->width = width;
    stream->height = height;
    stream->maxBitrate = max_bitrate;
    stream->minBitrate = min_bitrate;
    stream->targetBitrate = target_bitrate;
    stream->numberOfTemporalLayers = num_temporal_layers;
    stream->qpMax = 45;
  }

 protected:
  virtual void SetUp() {
    SetUpCodec(kDefaultTemporalLayerProfile);
  }

  virtual void SetUpCodec(const int* temporal_layer_profile) {
    encoder_->RegisterEncodeCompleteCallback(&encoder_callback_);
    decoder_->RegisterDecodeCompleteCallback(&decoder_callback_);
    DefaultSettings(&settings_, temporal_layer_profile);
    EXPECT_EQ(0, encoder_->InitEncode(&settings_, 1, 1200));
    EXPECT_EQ(0, decoder_->InitDecode(&settings_, 1));
    int half_width = (kDefaultWidth + 1) / 2;
    input_frame_.CreateEmptyFrame(kDefaultWidth, kDefaultHeight,
                                  kDefaultWidth, half_width, half_width);
    memset(input_frame_.buffer(kYPlane), 0,
        input_frame_.allocated_size(kYPlane));
    memset(input_frame_.buffer(kUPlane), 0,
        input_frame_.allocated_size(kUPlane));
    memset(input_frame_.buffer(kVPlane), 0,
        input_frame_.allocated_size(kVPlane));
  }

  virtual void TearDown() {
    encoder_->Release();
    decoder_->Release();
  }

  void ExpectStreams(FrameType frame_type, int expected_video_streams) {
    ASSERT_GE(expected_video_streams, 0);
    ASSERT_LE(expected_video_streams, kNumberOfSimulcastStreams);
    if (expected_video_streams >= 1) {
      EXPECT_CALL(encoder_callback_, Encoded(
          AllOf(Field(&EncodedImage::_frameType, frame_type),
                Field(&EncodedImage::_encodedWidth, kDefaultWidth / 4),
                Field(&EncodedImage::_encodedHeight, kDefaultHeight / 4)), _, _)
                  )
          .Times(1)
          .WillRepeatedly(Return(0));
    }
    if (expected_video_streams >= 2) {
      EXPECT_CALL(encoder_callback_, Encoded(
          AllOf(Field(&EncodedImage::_frameType, frame_type),
                Field(&EncodedImage::_encodedWidth, kDefaultWidth / 2),
                Field(&EncodedImage::_encodedHeight, kDefaultHeight / 2)), _, _)
                  )
          .Times(1)
          .WillRepeatedly(Return(0));
    }
    if (expected_video_streams >= 3) {
      EXPECT_CALL(encoder_callback_, Encoded(
          AllOf(Field(&EncodedImage::_frameType, frame_type),
                Field(&EncodedImage::_encodedWidth, kDefaultWidth),
                Field(&EncodedImage::_encodedHeight, kDefaultHeight)), _, _))
          .Times(1)
          .WillRepeatedly(Return(0));
    }
  }

  void VerifyTemporalIdxAndSyncForAllSpatialLayers(
      Vp8TestEncodedImageCallback* encoder_callback,
      const int* expected_temporal_idx,
      const bool* expected_layer_sync,
      int num_spatial_layers) {
    int picture_id = -1;
    int temporal_layer = -1;
    bool layer_sync = false;
    for (int i = 0; i < num_spatial_layers; i++) {
      encoder_callback->GetLastEncodedFrameInfo(&picture_id, &temporal_layer,
                                                &layer_sync, i);
      EXPECT_EQ(expected_temporal_idx[i], temporal_layer);
      EXPECT_EQ(expected_layer_sync[i], layer_sync);
    }
  }

  // We currently expect all active streams to generate a key frame even though
  // a key frame was only requested for some of them.
  void TestKeyFrameRequestsOnAllStreams() {
    encoder_->SetRates(kMaxBitrates[2], 30);  // To get all three streams.
    std::vector<FrameType> frame_types(kNumberOfSimulcastStreams,
                                       kVideoFrameDelta);
    ExpectStreams(kVideoFrameKey, kNumberOfSimulcastStreams);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));

    ExpectStreams(kVideoFrameDelta, kNumberOfSimulcastStreams);
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));

    frame_types[0] = kVideoFrameKey;
    ExpectStreams(kVideoFrameKey, kNumberOfSimulcastStreams);
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));

    std::fill(frame_types.begin(), frame_types.end(), kVideoFrameDelta);
    frame_types[1] = kVideoFrameKey;
    ExpectStreams(kVideoFrameKey, kNumberOfSimulcastStreams);
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));

    std::fill(frame_types.begin(), frame_types.end(), kVideoFrameDelta);
    frame_types[2] = kVideoFrameKey;
    ExpectStreams(kVideoFrameKey, kNumberOfSimulcastStreams);
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));

    std::fill(frame_types.begin(), frame_types.end(), kVideoFrameDelta);
    ExpectStreams(kVideoFrameDelta, kNumberOfSimulcastStreams);
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));
  }

  void TestPaddingAllStreams() {
    // We should always encode the base layer.
    encoder_->SetRates(kMinBitrates[0] - 1, 30);
    std::vector<FrameType> frame_types(kNumberOfSimulcastStreams,
                                       kVideoFrameDelta);
    ExpectStreams(kVideoFrameKey, 1);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));

    ExpectStreams(kVideoFrameDelta, 1);
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));
  }

  void TestPaddingTwoStreams() {
    // We have just enough to get only the first stream and padding for two.
    encoder_->SetRates(kMinBitrates[0], 30);
    std::vector<FrameType> frame_types(kNumberOfSimulcastStreams,
                                       kVideoFrameDelta);
    ExpectStreams(kVideoFrameKey, 1);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));

    ExpectStreams(kVideoFrameDelta, 1);
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));
  }

  void TestPaddingTwoStreamsOneMaxedOut() {
    // We are just below limit of sending second stream, so we should get
    // the first stream maxed out (at |maxBitrate|), and padding for two.
    encoder_->SetRates(kTargetBitrates[0] + kMinBitrates[1] - 1, 30);
    std::vector<FrameType> frame_types(kNumberOfSimulcastStreams,
                                       kVideoFrameDelta);
    ExpectStreams(kVideoFrameKey, 1);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));

    ExpectStreams(kVideoFrameDelta, 1);
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));
  }

  void TestPaddingOneStream() {
    // We have just enough to send two streams, so padding for one stream.
    encoder_->SetRates(kTargetBitrates[0] + kMinBitrates[1], 30);
    std::vector<FrameType> frame_types(kNumberOfSimulcastStreams,
                                       kVideoFrameDelta);
    ExpectStreams(kVideoFrameKey, 2);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));

    ExpectStreams(kVideoFrameDelta, 2);
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));
  }

  void TestPaddingOneStreamTwoMaxedOut() {
    // We are just below limit of sending third stream, so we should get
    // first stream's rate maxed out at |targetBitrate|, second at |maxBitrate|.
    encoder_->SetRates(kTargetBitrates[0] + kTargetBitrates[1] +
                       kMinBitrates[2] - 1, 30);
    std::vector<FrameType> frame_types(kNumberOfSimulcastStreams,
                                       kVideoFrameDelta);
    ExpectStreams(kVideoFrameKey, 2);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));

    ExpectStreams(kVideoFrameDelta, 2);
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));
  }

  void TestSendAllStreams() {
    // We have just enough to send all streams.
    encoder_->SetRates(kTargetBitrates[0] + kTargetBitrates[1] +
                       kMinBitrates[2], 30);
    std::vector<FrameType> frame_types(kNumberOfSimulcastStreams,
                                       kVideoFrameDelta);
    ExpectStreams(kVideoFrameKey, 3);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));

    ExpectStreams(kVideoFrameDelta, 3);
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));
  }

  void TestDisablingStreams() {
    // We should get three media streams.
    encoder_->SetRates(kMaxBitrates[0] + kMaxBitrates[1] +
                       kMaxBitrates[2], 30);
    std::vector<FrameType> frame_types(kNumberOfSimulcastStreams,
                                       kVideoFrameDelta);
    ExpectStreams(kVideoFrameKey, 3);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));

    ExpectStreams(kVideoFrameDelta, 3);
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));

    // We should only get two streams and padding for one.
    encoder_->SetRates(kTargetBitrates[0] + kTargetBitrates[1] +
                       kMinBitrates[2] / 2, 30);
    ExpectStreams(kVideoFrameDelta, 2);
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));

    // We should only get the first stream and padding for two.
    encoder_->SetRates(kTargetBitrates[0] + kMinBitrates[1] / 2, 30);
    ExpectStreams(kVideoFrameDelta, 1);
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));

    // We don't have enough bitrate for the thumbnail stream, but we should get
    // it anyway with current configuration.
    encoder_->SetRates(kTargetBitrates[0] - 1, 30);
    ExpectStreams(kVideoFrameDelta, 1);
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));

    // We should only get two streams and padding for one.
    encoder_->SetRates(kTargetBitrates[0] + kTargetBitrates[1] +
                       kMinBitrates[2] / 2, 30);
    // We get a key frame because a new stream is being enabled.
    ExpectStreams(kVideoFrameKey, 2);
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));

    // We should get all three streams.
    encoder_->SetRates(kTargetBitrates[0] + kTargetBitrates[1] +
                       kTargetBitrates[2], 30);
    // We get a key frame because a new stream is being enabled.
    ExpectStreams(kVideoFrameKey, 3);
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));
  }

  void SwitchingToOneStream(int width, int height) {
    // Disable all streams except the last and set the bitrate of the last to
    // 100 kbps. This verifies the way GTP switches to screenshare mode.
    settings_.codecSpecific.VP8.numberOfTemporalLayers = 1;
    settings_.maxBitrate = 100;
    settings_.startBitrate = 100;
    settings_.width = width;
    settings_.height = height;
    for (int i = 0; i < settings_.numberOfSimulcastStreams - 1; ++i) {
      settings_.simulcastStream[i].maxBitrate = 0;
      settings_.simulcastStream[i].width = settings_.width;
      settings_.simulcastStream[i].height = settings_.height;
    }
    // Setting input image to new resolution.
    int half_width = (settings_.width + 1) / 2;
    input_frame_.CreateEmptyFrame(settings_.width, settings_.height,
                                  settings_.width, half_width, half_width);
    memset(input_frame_.buffer(kYPlane), 0,
        input_frame_.allocated_size(kYPlane));
    memset(input_frame_.buffer(kUPlane), 0,
        input_frame_.allocated_size(kUPlane));
    memset(input_frame_.buffer(kVPlane), 0,
        input_frame_.allocated_size(kVPlane));

    // The for loop above did not set the bitrate of the highest layer.
    settings_.simulcastStream[settings_.numberOfSimulcastStreams - 1].
        maxBitrate = 0;
    // The highest layer has to correspond to the non-simulcast resolution.
    settings_.simulcastStream[settings_.numberOfSimulcastStreams - 1].
        width = settings_.width;
    settings_.simulcastStream[settings_.numberOfSimulcastStreams - 1].
        height = settings_.height;
    EXPECT_EQ(0, encoder_->InitEncode(&settings_, 1, 1200));

    // Encode one frame and verify.
    encoder_->SetRates(kMaxBitrates[0] + kMaxBitrates[1], 30);
    std::vector<FrameType> frame_types(kNumberOfSimulcastStreams,
                                       kVideoFrameDelta);
    EXPECT_CALL(encoder_callback_,
                Encoded(AllOf(Field(&EncodedImage::_frameType, kVideoFrameKey),
                              Field(&EncodedImage::_encodedWidth, width),
                              Field(&EncodedImage::_encodedHeight, height)),
                        _, _))
        .Times(1)
        .WillRepeatedly(Return(0));
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));

    // Switch back.
    DefaultSettings(&settings_, kDefaultTemporalLayerProfile);
    // Start at the lowest bitrate for enabling base stream.
    settings_.startBitrate = kMinBitrates[0];
    EXPECT_EQ(0, encoder_->InitEncode(&settings_, 1, 1200));
    encoder_->SetRates(settings_.startBitrate, 30);
    ExpectStreams(kVideoFrameKey, 1);
    // Resize |input_frame_| to the new resolution.
    half_width = (settings_.width + 1) / 2;
    input_frame_.CreateEmptyFrame(settings_.width, settings_.height,
                                  settings_.width, half_width, half_width);
    memset(input_frame_.buffer(kYPlane), 0,
        input_frame_.allocated_size(kYPlane));
    memset(input_frame_.buffer(kUPlane), 0,
        input_frame_.allocated_size(kUPlane));
    memset(input_frame_.buffer(kVPlane), 0,
        input_frame_.allocated_size(kVPlane));
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, &frame_types));
  }

  void TestSwitchingToOneStream() {
    SwitchingToOneStream(1024, 768);
  }

  void TestSwitchingToOneOddStream() {
    SwitchingToOneStream(1023, 769);
  }

  void TestRPSIEncoder() {
    Vp8TestEncodedImageCallback encoder_callback;
    encoder_->RegisterEncodeCompleteCallback(&encoder_callback);

    encoder_->SetRates(kMaxBitrates[2], 30);  // To get all three streams.

    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));
    int picture_id = -1;
    int temporal_layer = -1;
    bool layer_sync = false;
    encoder_callback.GetLastEncodedFrameInfo(&picture_id, &temporal_layer,
                                             &layer_sync, 0);
    EXPECT_EQ(0, temporal_layer);
    EXPECT_TRUE(layer_sync);
    int key_frame_picture_id = picture_id;

    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));
    encoder_callback.GetLastEncodedFrameInfo(&picture_id, &temporal_layer,
                                             &layer_sync, 0);
    EXPECT_EQ(2, temporal_layer);
    EXPECT_TRUE(layer_sync);

    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));
    encoder_callback.GetLastEncodedFrameInfo(&picture_id, &temporal_layer,
                                             &layer_sync, 0);
    EXPECT_EQ(1, temporal_layer);
    EXPECT_TRUE(layer_sync);

    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));
    encoder_callback.GetLastEncodedFrameInfo(&picture_id, &temporal_layer,
                                             &layer_sync, 0);
    EXPECT_EQ(2, temporal_layer);
    EXPECT_FALSE(layer_sync);

    CodecSpecificInfo codec_specific;
    codec_specific.codecType = kVideoCodecVP8;
    codec_specific.codecSpecific.VP8.hasReceivedRPSI = true;

    // Must match last key frame to trigger.
    codec_specific.codecSpecific.VP8.pictureIdRPSI = key_frame_picture_id;

    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, &codec_specific, NULL));
    encoder_callback.GetLastEncodedFrameInfo(&picture_id, &temporal_layer,
                                             &layer_sync, 0);

    EXPECT_EQ(0, temporal_layer);
    EXPECT_TRUE(layer_sync);

    // Must match last key frame to trigger, test bad id.
    codec_specific.codecSpecific.VP8.pictureIdRPSI = key_frame_picture_id + 17;

    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, &codec_specific, NULL));
    encoder_callback.GetLastEncodedFrameInfo(&picture_id, &temporal_layer,
                                             &layer_sync, 0);

    EXPECT_EQ(2, temporal_layer);
    // The previous frame was a base layer sync (since it was a frame that
    // only predicts from key frame and hence resets the temporal pattern),
    // so this frame (the next one) must have |layer_sync| set to true.
    EXPECT_TRUE(layer_sync);
  }

  void TestRPSIEncodeDecode() {
    Vp8TestEncodedImageCallback encoder_callback;
    Vp8TestDecodedImageCallback decoder_callback;
    encoder_->RegisterEncodeCompleteCallback(&encoder_callback);
    decoder_->RegisterDecodeCompleteCallback(&decoder_callback);

    encoder_->SetRates(kMaxBitrates[2], 30);  // To get all three streams.

    // Set color.
    int plane_offset[kNumOfPlanes];
    plane_offset[kYPlane] = kColorY;
    plane_offset[kUPlane] = kColorU;
    plane_offset[kVPlane] = kColorV;
    CreateImage(&input_frame_, plane_offset);

    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));
    int picture_id = -1;
    int temporal_layer = -1;
    bool layer_sync = false;
    encoder_callback.GetLastEncodedFrameInfo(&picture_id, &temporal_layer,
                                             &layer_sync, 0);
    EXPECT_EQ(0, temporal_layer);
    EXPECT_TRUE(layer_sync);
    int key_frame_picture_id = picture_id;

    // Change color.
    plane_offset[kYPlane] += 1;
    plane_offset[kUPlane] += 1;
    plane_offset[kVPlane] += 1;
    CreateImage(&input_frame_, plane_offset);
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));

    // Change color.
    plane_offset[kYPlane] += 1;
    plane_offset[kUPlane] += 1;
    plane_offset[kVPlane] += 1;
    CreateImage(&input_frame_, plane_offset);

    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));

    // Change color.
    plane_offset[kYPlane] += 1;
    plane_offset[kUPlane] += 1;
    plane_offset[kVPlane] += 1;
    CreateImage(&input_frame_, plane_offset);

    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));

    CodecSpecificInfo codec_specific;
    codec_specific.codecType = kVideoCodecVP8;
    codec_specific.codecSpecific.VP8.hasReceivedRPSI = true;
    // Must match last key frame to trigger.
    codec_specific.codecSpecific.VP8.pictureIdRPSI = key_frame_picture_id;

    // Change color back to original.
    plane_offset[kYPlane] = kColorY;
    plane_offset[kUPlane] = kColorU;
    plane_offset[kVPlane] = kColorV;
    CreateImage(&input_frame_, plane_offset);

    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, &codec_specific, NULL));

    EncodedImage encoded_frame;
    encoder_callback.GetLastEncodedKeyFrame(&encoded_frame);
    decoder_->Decode(encoded_frame, false, NULL);
    encoder_callback.GetLastEncodedFrame(&encoded_frame);
    decoder_->Decode(encoded_frame, false, NULL);
    EXPECT_EQ(2, decoder_callback.DecodedFrames());
  }

  // Test the layer pattern and sync flag for various spatial-temporal patterns.
  // 3-3-3 pattern: 3 temporal layers for all spatial streams, so same
  // temporal_layer id and layer_sync is expected for all streams.
  void TestSaptioTemporalLayers333PatternEncoder() {
    Vp8TestEncodedImageCallback encoder_callback;
    encoder_->RegisterEncodeCompleteCallback(&encoder_callback);
    encoder_->SetRates(kMaxBitrates[2], 30);  // To get all three streams.

    int expected_temporal_idx[3] = { -1, -1, -1};
    bool expected_layer_sync[3] = {false, false, false};

    // First frame: #0.
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));
    SetExpectedValues3<int>(0, 0, 0, expected_temporal_idx);
    SetExpectedValues3<bool>(true, true, true, expected_layer_sync);
    VerifyTemporalIdxAndSyncForAllSpatialLayers(&encoder_callback,
                                                expected_temporal_idx,
                                                expected_layer_sync,
                                                3);

    // Next frame: #1.
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));
    SetExpectedValues3<int>(2, 2, 2, expected_temporal_idx);
    SetExpectedValues3<bool>(true, true, true, expected_layer_sync);
    VerifyTemporalIdxAndSyncForAllSpatialLayers(&encoder_callback,
                                                expected_temporal_idx,
                                                expected_layer_sync,
                                                3);

    // Next frame: #2.
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));
    SetExpectedValues3<int>(1, 1, 1, expected_temporal_idx);
    SetExpectedValues3<bool>(true, true, true, expected_layer_sync);
    VerifyTemporalIdxAndSyncForAllSpatialLayers(&encoder_callback,
                                                expected_temporal_idx,
                                                expected_layer_sync,
                                                3);

    // Next frame: #3.
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));
    SetExpectedValues3<int>(2, 2, 2, expected_temporal_idx);
    SetExpectedValues3<bool>(false, false, false, expected_layer_sync);
    VerifyTemporalIdxAndSyncForAllSpatialLayers(&encoder_callback,
                                                expected_temporal_idx,
                                                expected_layer_sync,
                                                3);

    // Next frame: #4.
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));
    SetExpectedValues3<int>(0, 0, 0, expected_temporal_idx);
    SetExpectedValues3<bool>(false, false, false, expected_layer_sync);
    VerifyTemporalIdxAndSyncForAllSpatialLayers(&encoder_callback,
                                                expected_temporal_idx,
                                                expected_layer_sync,
                                                3);

    // Next frame: #5.
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));
    SetExpectedValues3<int>(2, 2, 2, expected_temporal_idx);
    SetExpectedValues3<bool>(false, false, false, expected_layer_sync);
    VerifyTemporalIdxAndSyncForAllSpatialLayers(&encoder_callback,
                                                expected_temporal_idx,
                                                expected_layer_sync,
                                                3);
  }

  // Test the layer pattern and sync flag for various spatial-temporal patterns.
  // 3-2-1 pattern: 3 temporal layers for lowest resolution, 2 for middle, and
  // 1 temporal layer for highest resolution.
  // For this profile, we expect the temporal index pattern to be:
  // 1st stream: 0, 2, 1, 2, ....
  // 2nd stream: 0, 1, 0, 1, ...
  // 3rd stream: -1, -1, -1, -1, ....
  // Regarding the 3rd stream, note that a stream/encoder with 1 temporal layer
  // should always have temporal layer idx set to kNoTemporalIdx = -1.
  // Since CodecSpecificInfoVP8.temporalIdx is uint8_t, this will wrap to 255.
  // TODO(marpan): Although this seems safe for now, we should fix this.
  void TestSpatioTemporalLayers321PatternEncoder() {
    int temporal_layer_profile[3] = {3, 2, 1};
    SetUpCodec(temporal_layer_profile);
    Vp8TestEncodedImageCallback encoder_callback;
    encoder_->RegisterEncodeCompleteCallback(&encoder_callback);
    encoder_->SetRates(kMaxBitrates[2], 30);  // To get all three streams.

    int expected_temporal_idx[3] = { -1, -1, -1};
    bool expected_layer_sync[3] = {false, false, false};

    // First frame: #0.
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));
    SetExpectedValues3<int>(0, 0, 255, expected_temporal_idx);
    SetExpectedValues3<bool>(true, true, false, expected_layer_sync);
    VerifyTemporalIdxAndSyncForAllSpatialLayers(&encoder_callback,
                                                expected_temporal_idx,
                                                expected_layer_sync,
                                                3);

    // Next frame: #1.
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));
    SetExpectedValues3<int>(2, 1, 255, expected_temporal_idx);
    SetExpectedValues3<bool>(true, true, false, expected_layer_sync);
    VerifyTemporalIdxAndSyncForAllSpatialLayers(&encoder_callback,
                                                expected_temporal_idx,
                                                expected_layer_sync,
                                                3);

    // Next frame: #2.
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));
    SetExpectedValues3<int>(1, 0, 255, expected_temporal_idx);
    SetExpectedValues3<bool>(true, false, false, expected_layer_sync);
    VerifyTemporalIdxAndSyncForAllSpatialLayers(&encoder_callback,
                                                expected_temporal_idx,
                                                expected_layer_sync,
                                                3);

    // Next frame: #3.
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));
    SetExpectedValues3<int>(2, 1, 255, expected_temporal_idx);
    SetExpectedValues3<bool>(false, false, false, expected_layer_sync);
    VerifyTemporalIdxAndSyncForAllSpatialLayers(&encoder_callback,
                                                expected_temporal_idx,
                                                expected_layer_sync,
                                                3);

    // Next frame: #4.
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));
    SetExpectedValues3<int>(0, 0, 255, expected_temporal_idx);
    SetExpectedValues3<bool>(false, false, false, expected_layer_sync);
    VerifyTemporalIdxAndSyncForAllSpatialLayers(&encoder_callback,
                                                expected_temporal_idx,
                                                expected_layer_sync,
                                                3);

    // Next frame: #5.
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));
    SetExpectedValues3<int>(2, 1, 255, expected_temporal_idx);
    SetExpectedValues3<bool>(false, false, false, expected_layer_sync);
    VerifyTemporalIdxAndSyncForAllSpatialLayers(&encoder_callback,
                                                expected_temporal_idx,
                                                expected_layer_sync,
                                                3);
  }

  void TestStrideEncodeDecode() {
    Vp8TestEncodedImageCallback encoder_callback;
    Vp8TestDecodedImageCallback decoder_callback;
    encoder_->RegisterEncodeCompleteCallback(&encoder_callback);
    decoder_->RegisterDecodeCompleteCallback(&decoder_callback);

    encoder_->SetRates(kMaxBitrates[2], 30);  // To get all three streams.
    // Setting two (possibly) problematic use cases for stride:
    // 1. stride > width 2. stride_y != stride_uv/2
    int stride_y = kDefaultWidth + 20;
    int stride_uv = ((kDefaultWidth + 1) / 2) + 5;
    input_frame_.CreateEmptyFrame(kDefaultWidth, kDefaultHeight,
                                 stride_y, stride_uv, stride_uv);
    // Set color.
    int plane_offset[kNumOfPlanes];
    plane_offset[kYPlane] = kColorY;
    plane_offset[kUPlane] = kColorU;
    plane_offset[kVPlane] = kColorV;
    CreateImage(&input_frame_, plane_offset);

    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));

    // Change color.
    plane_offset[kYPlane] += 1;
    plane_offset[kUPlane] += 1;
    plane_offset[kVPlane] += 1;
    CreateImage(&input_frame_, plane_offset);
    input_frame_.set_timestamp(input_frame_.timestamp() + 3000);
    EXPECT_EQ(0, encoder_->Encode(input_frame_, NULL, NULL));

    EncodedImage encoded_frame;
    // Only encoding one frame - so will be a key frame.
    encoder_callback.GetLastEncodedKeyFrame(&encoded_frame);
    EXPECT_EQ(0, decoder_->Decode(encoded_frame, false, NULL));
    encoder_callback.GetLastEncodedFrame(&encoded_frame);
    decoder_->Decode(encoded_frame, false, NULL);
    EXPECT_EQ(2, decoder_callback.DecodedFrames());
  }

  void TestSkipEncodingUnusedStreams() {
    SkipEncodingUnusedStreamsTest test;
    std::vector<unsigned int> configured_bitrate =
        test.RunTest(encoder_.get(),
                     &settings_,
                     1);    // Target bit rate 1, to force all streams but the
                            // base one to be exceeding bandwidth constraints.
    EXPECT_EQ(static_cast<size_t>(kNumberOfSimulcastStreams),
              configured_bitrate.size());

    unsigned int min_bitrate =
        std::max(settings_.simulcastStream[0].minBitrate, settings_.minBitrate);
    int stream = 0;
    for (std::vector<unsigned int>::const_iterator it =
             configured_bitrate.begin();
         it != configured_bitrate.end();
         ++it) {
      if (stream == 0) {
        EXPECT_EQ(min_bitrate, *it);
      } else {
        EXPECT_EQ(0u, *it);
      }
      ++stream;
    }
  }

  rtc::scoped_ptr<VP8Encoder> encoder_;
  MockEncodedImageCallback encoder_callback_;
  rtc::scoped_ptr<VP8Decoder> decoder_;
  MockDecodedImageCallback decoder_callback_;
  VideoCodec settings_;
  VideoFrame input_frame_;
};

}  // namespace testing
}  // namespace webrtc

#endif  // WEBRTC_MODULES_VIDEO_CODING_CODECS_VP8_SIMULCAST_UNITTEST_H_
