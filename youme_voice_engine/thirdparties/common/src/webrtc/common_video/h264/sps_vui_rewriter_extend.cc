/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include <vector>

//#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/common_video/h264/sps_vui_rewriter_extend.h"
#include "webrtc/base/bitbuffer.h"
#include "webrtc/base/buffer.h"
#include "webrtc/base/fileutils.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/pathutils.h"
#include "webrtc/base/stream.h"

#include "webrtc/common_video/h264/sps_vui_rewriter.h"
#include "webrtc/common_video/h264/h264_common.h"

namespace youme {
    namespace webrtc {


static const size_t kSpsBufferMaxSize = 256;

// Generates a fake SPS with basically everything empty and with characteristics
// based off SpsMode.
// Pass in a buffer of at least kSpsBufferMaxSize.
// The fake SPS that this generates also always has at least one emulation byte
// at offset 2, since the first two bytes are always 0, and has a 0x3 as the
// level_idc, to make sure the parser doesn't eat all 0x3 bytes.
void SpsVuiRewriterExtend::GenerateFakeSps(SpsMode mode, rtc::Optional<SpsParser::SpsState> sps, rtc::Buffer* out_buffer) {
  uint8_t rbsp[kSpsBufferMaxSize] = {0};
  rtc::BitBufferWriter writer(rbsp, kSpsBufferMaxSize);
  // Profile byte.
  writer.WriteUInt8(sps->profile_idc);
  // Constraint sets and reserved zero bits.
  writer.WriteUInt8(0);
  // level_idc.
  writer.WriteUInt8(sps->level_idc);
  // seq_paramter_set_id.
  writer.WriteExponentialGolomb(0);
  // Profile is not special, so we skip all the chroma format settings.

  // Now some bit magic.
  // log2_max_frame_num_minus4: ue(v). 0 is fine.
  writer.WriteExponentialGolomb(sps->log2_max_frame_num_minus4);
  // pic_order_cnt_type: ue(v).
  // POC type 2 is the one that doesn't need to be rewritten.
  if (mode == kNoRewriteRequired_PocCorrect) {
    writer.WriteExponentialGolomb(2);
  } else {
    writer.WriteExponentialGolomb(0);
    // log2_max_pic_order_cnt_lsb_minus4: ue(v). 0 is fine.
    writer.WriteExponentialGolomb(sps->log2_max_pic_order_cnt_lsb_minus4);
  }
  // max_num_ref_frames: ue(v). Use 1, to make optimal/suboptimal more obvious.
  writer.WriteExponentialGolomb(sps->max_num_ref_frames);
  // gaps_in_frame_num_value_allowed_flag: u(1).
  writer.WriteBits(0, 1);
  // Next are width/height. First, calculate the mbs/map_units versions.
  uint16_t width_in_mbs_minus1 = (sps->width + 15) / 16 - 1;

  // For the height, we're going to define frame_mbs_only_flag, so we need to
  // divide by 2. See the parser for the full calculation.
  uint16_t height_in_map_units_minus1 = ((sps->height + 15) / 16 - 1) ;
  // Write each as ue(v).
  writer.WriteExponentialGolomb(width_in_mbs_minus1);
  writer.WriteExponentialGolomb(height_in_map_units_minus1);
  // frame_mbs_only_flag: u(1). Needs to be false.
  writer.WriteBits(sps->frame_mbs_only_flag, 1);
  // mb_adaptive_frame_field_flag: u(1).
  //writer.WriteBits(0, 1);
  // direct_8x8_inferene_flag: u(1).
  writer.WriteBits(1, 1);
  // frame_cropping_flag: u(1). 1, so we can supply crop.
  writer.WriteBits(sps->frame_cropping_flag, 1);
  // Now we write the left/right/top/bottom crop.
  if(sps->frame_cropping_flag) {
      // Left/right.
      writer.WriteExponentialGolomb(sps->frame_crop_left_offset);
      writer.WriteExponentialGolomb(sps->frame_crop_right_offset);
      // Top/bottom.
      writer.WriteExponentialGolomb(sps->frame_crop_top_offset);
      writer.WriteExponentialGolomb(sps->frame_crop_bottom_offset);
  }

  // Finally! The VUI.
  // vui_parameters_present_flag: u(1)
  if (mode == kNoRewriteRequired_PocCorrect || mode == kRewriteRequired_NoVui) {
    writer.WriteBits(0, 1);
  } else {
    writer.WriteBits(1, 1);
    // VUI time. 8 flags to ignore followed by the bitstream restriction flag.
    writer.WriteBits(0, 8);
    if (mode == kRewriteRequired_NoBitstreamRestriction) {
      writer.WriteBits(0, 1);
    } else {
      writer.WriteBits(1, 1);
      // Write some defaults. Shouldn't matter for parsing, though.
      // motion_vectors_over_pic_boundaries_flag: u(1)
      writer.WriteBits(1, 1);
      // max_bytes_per_pic_denom: ue(v)
      writer.WriteExponentialGolomb(2);
      // max_bits_per_mb_denom: ue(v)
      writer.WriteExponentialGolomb(1);
      // log2_max_mv_length_horizontal: ue(v)
      // log2_max_mv_length_vertical: ue(v)
      writer.WriteExponentialGolomb(8);
      writer.WriteExponentialGolomb(8);

      // Next are the limits we care about.
      // max_num_reorder_frames: ue(v)
      // max_dec_frame_buffering: ue(v)
      if (mode == kRewriteRequired_VuiSuboptimal) {
        writer.WriteExponentialGolomb(4);
        writer.WriteExponentialGolomb(4);
      } else if (kNoRewriteRequired_VuiOptimal) {
        writer.WriteExponentialGolomb(0);
        writer.WriteExponentialGolomb(1);
      }
    }
  }

  // Get the number of bytes written (including the last partial byte).
  size_t byte_count, bit_offset;
  writer.GetCurrentOffset(&byte_count, &bit_offset);
  if (bit_offset > 0) {
    byte_count++;
  }
//    for (int i = 0; i < byte_count; ++i) {
//        printf(" 0x%X", rbsp[i]);
//    }

  // Write the NALU header and type; {0 0 0 1} and 7 for the SPS header type.
  uint8_t header[] = {0, 0, 0, 1, 0x27};
  out_buffer->AppendData(header, sizeof(header));
 
  H264::WriteRbsp(rbsp, byte_count, out_buffer);
}

}  // namespace webrtc
}
