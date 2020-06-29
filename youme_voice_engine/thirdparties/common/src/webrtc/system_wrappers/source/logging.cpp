/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/system_wrappers/include/logging.h"

#include <string.h>

#include <sstream>

#include "webrtc/common_types.h"
//#include "webrtc/system_wrappers/include/trace.h"

#ifdef __cplusplus
extern "C" {
  #include "tsk_log.h"
}
#endif

namespace youme {
namespace webrtc {
namespace {


//TraceLevel WebRtcSeverity(LoggingSeverity sev) {
//  switch (sev) {
//    // TODO(ajm): SENSITIVE doesn't have a corresponding webrtc level.
//    case LS_SENSITIVE:  return kTraceInfo;
//    case LS_VERBOSE:    return kTraceInfo;
//    case LS_INFO:       return kTraceTerseInfo;
//    case LS_WARNING:    return kTraceWarning;
//    case LS_ERROR:      return kTraceError;
//    default:            return kTraceNone;
//  }
//}

int LogLevelWebrtcToTsk(LoggingSeverity sev) {
  switch (sev) {
    case LS_SENSITIVE:  return TSK_LOG_INFO;
    case LS_VERBOSE:    return TSK_LOG_VERBOSE;
    case LS_INFO:       return TSK_LOG_INFO;
    case LS_WARNING:    return TSK_LOG_WARNING;
    case LS_ERROR:      return TSK_LOG_ERROR;
    default:            return TSK_LOG_DISABLED;
  }
}

// Return the filename portion of the string (that following the last slash).
const char* FilenameFromPath(const char* file) {
  const char* end1 = ::strrchr(file, '/');
  const char* end2 = ::strrchr(file, '\\');
  if (!end1 && !end2)
    return file;
  else
    return (end1 > end2) ? end1 + 1 : end2 + 1;
}

}  // namespace

LogMessage::LogMessage(const char* file, int line, LoggingSeverity sev)
    : severity_(sev) {
  print_stream_ << "(" << FilenameFromPath(file) << ":" << line << "): ";
}

bool LogMessage::Loggable(LoggingSeverity sev) {
  // |level_filter| is a bitmask, unlike libjingle's minimum severity value.
  //  return WebRtcSeverity(sev) & Trace::level_filter() ? true : false;
    return true;
}

LogMessage::~LogMessage() {
  const std::string& str = print_stream_.str();
  int log_level = LogLevelWebrtcToTsk(severity_);
  //Trace::Add(WebRtcSeverity(severity_), kTraceUndefined, 0, "%s", str.c_str());
  tsk_log_imp ("", "", 0, log_level, str.c_str());
}

}  // namespace webrtc
}  // namespace youme
