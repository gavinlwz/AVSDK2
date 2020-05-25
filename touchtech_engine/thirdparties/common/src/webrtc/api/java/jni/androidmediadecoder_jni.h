/*
 * libjingle
 * Copyright 2015 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef TALK_APP_WEBRTC_JAVA_JNI_ANDROIDMEDIADECODER_JNI_H_
#define TALK_APP_WEBRTC_JAVA_JNI_ANDROIDMEDIADECODER_JNI_H_

//#include "webrtc/media/engine/webrtcvideodecoderfactory.h"
#include "webrtc/video_decoder.h"
#include "webrtc/api/java/jni/jni_helpers.h"



namespace youme {
    namespace webrtc {
        
        // Implementation of Android MediaCodec based encoder factory.
        class MediaCodecVideoDecoderFactory{
        public:
            MediaCodecVideoDecoderFactory(){};
             ~MediaCodecVideoDecoderFactory(){};
            static bool IsSupported();
            static void SetEGLContext(jobject egl_context);
            static webrtc::VideoDecoder* CreateVideoDecoder();

        private:
            static jobject eglbase_;
            static jobject egl_context_;
        };
        
    }  // namespace webrtc
}  // namespace youme


#endif  // TALK_APP_WEBRTC_JAVA_JNI_ANDROIDMEDIADECODER_JNI_H_
