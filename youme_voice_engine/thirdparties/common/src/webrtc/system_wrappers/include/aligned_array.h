﻿/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_SYSTEM_WRAPPERS_INCLUDE_ALIGNED_ARRAY_
#define WEBRTC_SYSTEM_WRAPPERS_INCLUDE_ALIGNED_ARRAY_

#include "webrtc/base/checks.h"
#include "webrtc/system_wrappers/include/aligned_malloc.h"


namespace youme {

namespace webrtc {

// Wrapper class for aligned arrays. Every row (and the first dimension) are
// aligned to the given byte alignment.
template<typename T> class AlignedArray {
 public:
  AlignedArray(int rows, size_t cols, int alignment)
      : rows_(rows),
        cols_(cols),
        alignment_(alignment) {
    RTC_CHECK_GT(alignment_, 0);
    head_row_ = static_cast<T**>(AlignedMalloc(rows_ * sizeof(*head_row_),
                                               alignment_));
    for (int i = 0; i < rows_; ++i) {
      head_row_[i] = static_cast<T*>(AlignedMalloc(cols_ * sizeof(**head_row_),
                                                   alignment_));
    }
  }

  ~AlignedArray() {
    for (int i = 0; i < rows_; ++i) {
      AlignedFree(head_row_[i]);
    }
    AlignedFree(head_row_);
  }

  T* const* Array() {
    return head_row_;
  }

  const T* const* Array() const {
    return head_row_;
  }

  T* Row(int row) {
    RTC_CHECK_LE(row, rows_);
    return head_row_[row];
  }

  const T* Row(int row) const {
    RTC_CHECK_LE(row, rows_);
    return head_row_[row];
  }

  T& At(int row, size_t col) {
    RTC_CHECK_LE(col, cols_);
    return Row(row)[col];
  }

  const T& At(int row, size_t col) const {
    RTC_CHECK_LE(col, cols_);
    return Row(row)[col];
  }

  int rows() const {
    return rows_;
  }

  size_t cols() const {
    return cols_;
  }

 private:
  int rows_;
  size_t cols_;
  int alignment_;
  T** head_row_;
};

}  // namespace webrtc
}  // namespace youme

#endif  // WEBRTC_SYSTEM_WRAPPERS_INCLUDE_ALIGNED_ARRAY_
