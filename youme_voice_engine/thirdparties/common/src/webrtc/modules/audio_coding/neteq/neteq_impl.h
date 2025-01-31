﻿/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_AUDIO_CODING_NETEQ_NETEQ_IMPL_H_
#define WEBRTC_MODULES_AUDIO_CODING_NETEQ_NETEQ_IMPL_H_

#include "webrtc/base/constructormagic.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/thread_annotations.h"
#include "webrtc/modules/audio_coding/neteq/audio_multi_vector.h"
#include "webrtc/modules/audio_coding/neteq/defines.h"
#include "webrtc/modules/audio_coding/neteq/include/neteq.h"
#include "webrtc/modules/audio_coding/neteq/packet.h"  // Declare PacketList.
#include "webrtc/modules/audio_coding/neteq/random_vector.h"
#include "webrtc/modules/audio_coding/neteq/rtcp.h"
#include "webrtc/modules/audio_coding/neteq/statistics_calculator.h"
#include "webrtc/typedefs.h"

namespace youme {
namespace webrtc {

// Forward declarations.
class Accelerate;
class BackgroundNoise;
class BufferLevelFilter;
class ComfortNoise;
class CriticalSectionWrapper;
class DecisionLogic;
class DecoderDatabase;
class DelayManager;
class DelayPeakDetector;
class DtmfBuffer;
class DtmfToneGenerator;
class Expand;
class Merge;
class Nack;
class Normal;
class PacketBuffer;
class PayloadSplitter;
class PostDecodeVad;
class PreemptiveExpand;
class RandomVector;
class SyncBuffer;
class TimestampScaler;
struct AccelerateFactory;
struct DtmfEvent;
struct ExpandFactory;
struct PreemptiveExpandFactory;

class NetEqImpl : public webrtc::NetEq {
 public:
  // Creates a new NetEqImpl object. The object will assume ownership of all
  // injected dependencies, and will delete them when done.
  NetEqImpl(const NetEq::Config& config,
            BufferLevelFilter* buffer_level_filter,
            DecoderDatabase* decoder_database,
            DelayManager* delay_manager,
            DelayPeakDetector* delay_peak_detector,
            DtmfBuffer* dtmf_buffer,
            DtmfToneGenerator* dtmf_tone_generator,
            PacketBuffer* packet_buffer,
            PayloadSplitter* payload_splitter,
            TimestampScaler* timestamp_scaler,
            AccelerateFactory* accelerate_factory,
            ExpandFactory* expand_factory,
            PreemptiveExpandFactory* preemptive_expand_factory,
            bool create_components = true);

  ~NetEqImpl() override;

  // Inserts a new packet into NetEq. The |receive_timestamp| is an indication
  // of the time when the packet was received, and should be measured with
  // the same tick rate as the RTP timestamp of the current payload.
  // Returns 0 on success, -1 on failure.
  int InsertPacket(const WebRtcRTPHeader& rtp_header,
                   rtc::ArrayView<const uint8_t> payload,
                   uint32_t receive_timestamp) override;

  // Inserts a sync-packet into packet queue. Sync-packets are decoded to
  // silence and are intended to keep AV-sync intact in an event of long packet
  // losses when Video NACK is enabled but Audio NACK is not. Clients of NetEq
  // might insert sync-packet when they observe that buffer level of NetEq is
  // decreasing below a certain threshold, defined by the application.
  // Sync-packets should have the same payload type as the last audio payload
  // type, i.e. they cannot have DTMF or CNG payload type, nor a codec change
  // can be implied by inserting a sync-packet.
  // Returns kOk on success, kFail on failure.
  int InsertSyncPacket(const WebRtcRTPHeader& rtp_header,
                       uint32_t receive_timestamp) override;

  // Instructs NetEq to deliver 10 ms of audio data. The data is written to
  // |output_audio|, which can hold (at least) |max_length| elements.
  // The number of channels that were written to the output is provided in
  // the output variable |num_channels|, and each channel contains
  // |samples_per_channel| elements. If more than one channel is written,
  // the samples are interleaved.
  // The speech type is written to |type|, if |type| is not NULL.
  // Returns kOK on success, or kFail in case of an error.
  int GetAudio(size_t max_length,
               int16_t* output_audio,
               size_t* samples_per_channel,
               int* num_channels,
               NetEqOutputType* type) override;

  // Associates |rtp_payload_type| with |codec| and stores the information in
  // the codec database. Returns kOK on success, kFail on failure.
  int RegisterPayloadType(NetEqDecoder codec,
                          uint8_t rtp_payload_type,
                          int32_t fs_hz = -1) override;

  // Provides an externally created decoder object |decoder| to insert in the
  // decoder database. The decoder implements a decoder of type |codec| and
  // associates it with |rtp_payload_type|. The decoder will produce samples
  // at the rate |sample_rate_hz|. Returns kOK on success, kFail on failure.
  int RegisterExternalDecoder(AudioDecoder* decoder,
                              NetEqDecoder codec,
                              uint8_t rtp_payload_type,
                              int sample_rate_hz) override;

  // Removes |rtp_payload_type| from the codec database. Returns 0 on success,
  // -1 on failure.
  int RemovePayloadType(uint8_t rtp_payload_type) override;

  bool SetMinimumDelay(int delay_ms) override;

  bool SetMaximumDelay(int delay_ms) override;

  int LeastRequiredDelayMs() const override;

  int SetTargetDelay() override;

  int TargetDelay() override;

  int CurrentDelayMs() const override;

  // Sets the playout mode to |mode|.
  // Deprecated.
  // TODO(henrik.lundin) Delete.
  void SetPlayoutMode(NetEqPlayoutMode mode) override;

  // Returns the current playout mode.
  // Deprecated.
  // TODO(henrik.lundin) Delete.
  NetEqPlayoutMode PlayoutMode() const override;

  // Writes the current network statistics to |stats|. The statistics are reset
  // after the call.
  int NetworkStatistics(NetEqNetworkStatistics* stats) override;

  // Writes the current RTCP statistics to |stats|. The statistics are reset
  // and a new report period is started with the call.
  void GetRtcpStatistics(RtcpStatistics* stats) override;

  // Same as RtcpStatistics(), but does not reset anything.
  void GetRtcpStatisticsNoReset(RtcpStatistics* stats) override;

  // Enables post-decode VAD. When enabled, GetAudio() will return
  // kOutputVADPassive when the signal contains no speech.
  void EnableVad() override;

  // Disables post-decode VAD.
  void DisableVad() override;

  bool GetPlayoutTimestamp(uint32_t* timestamp) override;

  int SetTargetNumberOfChannels() override;

  int SetTargetSampleRate() override;

  // Returns the error code for the last occurred error. If no error has
  // occurred, 0 is returned.
  int LastError() const override;

  // Returns the error code last returned by a decoder (audio or comfort noise).
  // When LastError() returns kDecoderErrorCode or kComfortNoiseErrorCode, check
  // this method to get the decoder's error code.
  int LastDecoderError() override;

  // Flushes both the packet buffer and the sync buffer.
  void FlushBuffers() override;

  void PacketBufferStatistics(int* current_num_packets,
                              int* max_num_packets) const override;

  void EnableNack(size_t max_nack_list_size) override;

  void DisableNack() override;

  std::vector<uint16_t> GetNackList(int64_t round_trip_time_ms) const override;

  // This accessor method is only intended for testing purposes.
  const SyncBuffer* sync_buffer_for_test() const;

 protected:
  static const int kOutputSizeMs = 20;
  static const size_t kMaxFrameSize = 2880;  // 60 ms @ 48 kHz.
  // TODO(hlundin): Provide a better value for kSyncBufferSize.
  static const size_t kSyncBufferSize = 2 * kMaxFrameSize;

  // Inserts a new packet into NetEq. This is used by the InsertPacket method
  // above. Returns 0 on success, otherwise an error code.
  // TODO(hlundin): Merge this with InsertPacket above?
  int InsertPacketInternal(const WebRtcRTPHeader& rtp_header,
                           rtc::ArrayView<const uint8_t> payload,
                           uint32_t receive_timestamp,
                           bool is_sync_packet)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Delivers 10 ms of audio data. The data is written to |output|, which can
  // hold (at least) |max_length| elements. The number of channels that were
  // written to the output is provided in the output variable |num_channels|,
  // and each channel contains |samples_per_channel| elements. If more than one
  // channel is written, the samples are interleaved.
  // Returns 0 on success, otherwise an error code.
  int GetAudioInternal(size_t max_length,
                       int16_t* output,
                       size_t* samples_per_channel,
                       int* num_channels) EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Provides a decision to the GetAudioInternal method. The decision what to
  // do is written to |operation|. Packets to decode are written to
  // |packet_list|, and a DTMF event to play is written to |dtmf_event|. When
  // DTMF should be played, |play_dtmf| is set to true by the method.
  // Returns 0 on success, otherwise an error code.
  int GetDecision(Operations* operation,
                  PacketList* packet_list,
                  DtmfEvent* dtmf_event,
                  bool* play_dtmf) EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Decodes the speech packets in |packet_list|, and writes the results to
  // |decoded_buffer|, which is allocated to hold |decoded_buffer_length|
  // elements. The length of the decoded data is written to |decoded_length|.
  // The speech type -- speech or (codec-internal) comfort noise -- is written
  // to |speech_type|. If |packet_list| contains any SID frames for RFC 3389
  // comfort noise, those are not decoded.
  int Decode(PacketList* packet_list,
             Operations* operation,
             int* decoded_length,
             AudioDecoder::SpeechType* speech_type)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Sub-method to Decode(). Performs codec internal CNG.
  int DecodeCng(AudioDecoder* decoder, int* decoded_length,
                AudioDecoder::SpeechType* speech_type)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Sub-method to Decode(). Performs the actual decoding.
  int DecodeLoop(PacketList* packet_list,
                 const Operations& operation,
                 AudioDecoder* decoder,
                 int* decoded_length,
                 AudioDecoder::SpeechType* speech_type)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Sub-method which calls the Normal class to perform the normal operation.
  void DoNormal(const int16_t* decoded_buffer,
                size_t decoded_length,
                AudioDecoder::SpeechType speech_type,
                bool play_dtmf) EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Sub-method which calls the Merge class to perform the merge operation.
  void DoMerge(int16_t* decoded_buffer,
               size_t decoded_length,
               AudioDecoder::SpeechType speech_type,
               bool play_dtmf) EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Sub-method which calls the Expand class to perform the expand operation.
  int DoExpand(bool play_dtmf) EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Sub-method which calls the Expand class to perform the expand operation.
  int DoPlc() EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);
  
  // Sub-method which calls the Accelerate class to perform the accelerate
  // operation.
  int DoAccelerate(int16_t* decoded_buffer,
                   size_t decoded_length,
                   AudioDecoder::SpeechType speech_type,
                   bool play_dtmf,
                   bool fast_accelerate) EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Sub-method which calls the PreemptiveExpand class to perform the
  // preemtive expand operation.
  int DoPreemptiveExpand(int16_t* decoded_buffer,
                         size_t decoded_length,
                         AudioDecoder::SpeechType speech_type,
                         bool play_dtmf) EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Sub-method which calls the ComfortNoise class to generate RFC 3389 comfort
  // noise. |packet_list| can either contain one SID frame to update the
  // noise parameters, or no payload at all, in which case the previously
  // received parameters are used.
  int DoRfc3389Cng(PacketList* packet_list, bool play_dtmf)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Calls the audio decoder to generate codec-internal comfort noise when
  // no packet was received.
  void DoCodecInternalCng(const int16_t* decoded_buffer, size_t decoded_length)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Calls the DtmfToneGenerator class to generate DTMF tones.
  int DoDtmf(const DtmfEvent& dtmf_event, bool* play_dtmf)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Produces packet-loss concealment using alternative methods. If the codec
  // has an internal PLC, it is called to generate samples. Otherwise, the
  // method performs zero-stuffing.
  void DoAlternativePlc(bool increase_timestamp)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Overdub DTMF on top of |output|.
  int DtmfOverdub(const DtmfEvent& dtmf_event,
                  size_t num_channels,
                  int16_t* output) const EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Extracts packets from |packet_buffer_| to produce at least
  // |required_samples| samples. The packets are inserted into |packet_list|.
  // Returns the number of samples that the packets in the list will produce, or
  // -1 in case of an error.
  int ExtractPackets(size_t required_samples, PacketList* packet_list)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Resets various variables and objects to new values based on the sample rate
  // |fs_hz| and |channels| number audio channels.
  void SetSampleRateAndChannels(int fs_hz, size_t channels)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Returns the output type for the audio produced by the latest call to
  // GetAudio().
  NetEqOutputType LastOutputType() EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Updates Expand and Merge.
  virtual void UpdatePlcComponents(int fs_hz, size_t channels)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);

  // Creates DecisionLogic object with the mode given by |playout_mode_|.
  virtual void CreateDecisionLogic() EXCLUSIVE_LOCKS_REQUIRED(crit_sect_);
  
  // Convert Operations to its name
  const char* OperationToName(Operations op);

  const rtc::scoped_ptr<CriticalSectionWrapper> crit_sect_;
  const rtc::scoped_ptr<BufferLevelFilter> buffer_level_filter_
      GUARDED_BY(crit_sect_);
  const rtc::scoped_ptr<DecoderDatabase> decoder_database_
      GUARDED_BY(crit_sect_);
  const rtc::scoped_ptr<DelayManager> delay_manager_ GUARDED_BY(crit_sect_);
  const rtc::scoped_ptr<DelayPeakDetector> delay_peak_detector_
      GUARDED_BY(crit_sect_);
  const rtc::scoped_ptr<DtmfBuffer> dtmf_buffer_ GUARDED_BY(crit_sect_);
  const rtc::scoped_ptr<DtmfToneGenerator> dtmf_tone_generator_
      GUARDED_BY(crit_sect_);
  const rtc::scoped_ptr<PacketBuffer> packet_buffer_ GUARDED_BY(crit_sect_);
  const rtc::scoped_ptr<PayloadSplitter> payload_splitter_
      GUARDED_BY(crit_sect_);
  const rtc::scoped_ptr<TimestampScaler> timestamp_scaler_
      GUARDED_BY(crit_sect_);
  const rtc::scoped_ptr<PostDecodeVad> vad_ GUARDED_BY(crit_sect_);
  const rtc::scoped_ptr<ExpandFactory> expand_factory_ GUARDED_BY(crit_sect_);
  const rtc::scoped_ptr<AccelerateFactory> accelerate_factory_
      GUARDED_BY(crit_sect_);
  const rtc::scoped_ptr<PreemptiveExpandFactory> preemptive_expand_factory_
      GUARDED_BY(crit_sect_);

  rtc::scoped_ptr<BackgroundNoise> background_noise_ GUARDED_BY(crit_sect_);
  rtc::scoped_ptr<DecisionLogic> decision_logic_ GUARDED_BY(crit_sect_);
  rtc::scoped_ptr<AudioMultiVector> algorithm_buffer_ GUARDED_BY(crit_sect_);
  rtc::scoped_ptr<SyncBuffer> sync_buffer_ GUARDED_BY(crit_sect_);
  rtc::scoped_ptr<Expand> expand_ GUARDED_BY(crit_sect_);
  rtc::scoped_ptr<Normal> normal_ GUARDED_BY(crit_sect_);
  rtc::scoped_ptr<Merge> merge_ GUARDED_BY(crit_sect_);
  rtc::scoped_ptr<Accelerate> accelerate_ GUARDED_BY(crit_sect_);
  rtc::scoped_ptr<PreemptiveExpand> preemptive_expand_ GUARDED_BY(crit_sect_);
  RandomVector random_vector_ GUARDED_BY(crit_sect_);
  rtc::scoped_ptr<ComfortNoise> comfort_noise_ GUARDED_BY(crit_sect_);
  Rtcp rtcp_ GUARDED_BY(crit_sect_);
  StatisticsCalculator stats_ GUARDED_BY(crit_sect_);
  int fs_hz_ GUARDED_BY(crit_sect_);
  int fs_mult_ GUARDED_BY(crit_sect_);
  size_t output_size_samples_ GUARDED_BY(crit_sect_);
  size_t decoder_frame_length_ GUARDED_BY(crit_sect_);
  Modes last_mode_ GUARDED_BY(crit_sect_);
  rtc::scoped_ptr<int16_t[]> mute_factor_array_ GUARDED_BY(crit_sect_);
  size_t decoded_buffer_length_ GUARDED_BY(crit_sect_);
  rtc::scoped_ptr<int16_t[]> decoded_buffer_ GUARDED_BY(crit_sect_);
  uint32_t playout_timestamp_ GUARDED_BY(crit_sect_);
  bool new_codec_ GUARDED_BY(crit_sect_);
  uint32_t timestamp_ GUARDED_BY(crit_sect_);
  bool reset_decoder_ GUARDED_BY(crit_sect_);
  uint8_t current_rtp_payload_type_ GUARDED_BY(crit_sect_);
  uint8_t current_cng_rtp_payload_type_ GUARDED_BY(crit_sect_);
  uint32_t ssrc_ GUARDED_BY(crit_sect_);
  bool first_packet_ GUARDED_BY(crit_sect_);
  bool first_decoded_packet_ GUARDED_BY(crit_sect_);
  int32_t output_silence_packet_count_ GUARDED_BY(crit_sect_);
  int error_code_ GUARDED_BY(crit_sect_);  // Store last error code.
  int decoder_error_code_ GUARDED_BY(crit_sect_);
  const BackgroundNoiseMode background_noise_mode_ GUARDED_BY(crit_sect_);
  NetEqPlayoutMode playout_mode_ GUARDED_BY(crit_sect_);
  bool enable_fast_accelerate_ GUARDED_BY(crit_sect_);
  rtc::scoped_ptr<Nack> nack_ GUARDED_BY(crit_sect_);
  bool nack_enabled_ GUARDED_BY(crit_sect_);
  Operations last_operation_ GUARDED_BY(crit_sect_);
  int32_t    last_operation_count_ GUARDED_BY(crit_sect_);
  
 private:
  RTC_DISALLOW_COPY_AND_ASSIGN(NetEqImpl);
};

}  // namespace webrtc
}  // namespace youme
#endif  // WEBRTC_MODULES_AUDIO_CODING_NETEQ_NETEQ_IMPL_H_
