//
//  sps_vui_rewriter_extend.h
//  youme_voice_engine
//
//  Created by bhb on 2017/9/20.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef sps_vui_rewriter_extend_h
#define sps_vui_rewriter_extend_h
#include "webrtc/base/bitbuffer.h"
#include "webrtc/base/buffer.h"
#include "webrtc/base/fileutils.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/pathutils.h"
#include "webrtc/base/stream.h"
#include "webrtc/common_video/h264/sps_parser.h"


namespace youme {
    namespace webrtc {
        
        enum SpsMode {
            kNoRewriteRequired_PocCorrect,
            kNoRewriteRequired_VuiOptimal,
            kRewriteRequired_NoVui,
            kRewriteRequired_NoBitstreamRestriction,
            kRewriteRequired_VuiSuboptimal,
        };

        class SpsVuiRewriterExtend
        {
        public:
           static  void GenerateFakeSps(SpsMode mode, rtc::Optional<SpsParser::SpsState> sps, rtc::Buffer* out_buffer);
        };
    }
}
#endif /* sps_vui_rewriter_extend_h */
