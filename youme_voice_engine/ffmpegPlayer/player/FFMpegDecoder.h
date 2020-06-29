//
//  FFMpegAudioDecoder.hpp
//  youme_voice_engine
//
//  Created by peter on 8/13/16.
//  Copyright © 2016 Youme. All rights reserved.
//

#ifndef _FFMPEG_DECODER_
#define _FFMPEG_DECODER_

#include "version.h"
#if FFMPEG_SUPPORT

#ifdef __cplusplus
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
}
#endif

#include "IFFMpegDecoder.h"

typedef enum {
    DecoderOpening,
    DecoderOpened,
    DecoderClosing,
    DecoderClosed
} DecoderState_e;

class CFFMpegDecoder : public IFFMpegDecoder
{
public:
    CFFMpegDecoder();
    ~CFFMpegDecoder();
    
    bool open(const char* pFilePath) override;
    void close() override;
    int32_t getNextFrame(void** ppOutBuf, uint32_t *pMaxSize, AudioMeta_t *pAudioMeta, VideoMeta_s *pVideoMeta) override;
    
private:
    AVFormatContext     *m_pFmtContext;
    AVCodecContext      *m_pCodecContext;
    AVFrame             *m_pAvFrame;
    int32_t              m_nStreamIndex;
    DecoderState_e  m_state;
};

#endif //FFMPEG_SUPPORT

#endif /* _FFMPEG_DECODER_ */
