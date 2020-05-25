//
//  tdav_codec_rtp_extension.h
//  youme_voice_engine
//
//  Created by peter on 7/29/16.
//  Copyright © 2016 Youme. All rights reserved.
//

#ifndef _TDAV_CODEC_RTP_EXTENSION_H_
#define _TDAV_CODEC_RTP_EXTENSION_H_

#ifdef __cplusplus
extern "C" {
#endif
    
#include <stdint.h>
#include "tsk_common.h"
#include "tinydav/audio/tdav_session_audio.h"
    
// FEC type definitions
extern const uint8_t RTP_HEADER_EXT_FEC_TYPE_NONE;
extern const uint8_t RTP_HEADER_EXT_FEC_TYPE_OPUS;
extern const uint8_t RTP_HEADER_EXT_FEC_TYPE_RED;
extern const uint8_t RTP_HEADER_EXT_FEC_TYPE_ULP;
extern const uint8_t RTP_HEADER_EXT_FEC_TYPE_UXP;
    
typedef struct RtpHeaderExt_s {
    uint8_t*  pBandwidthCtrlData;
    uint16_t  bandwidthCtrlSize;
    uint8_t   fecType;
    uint8_t*  pFecData;
    uint16_t  fecSize;
    uint8_t   hasVoiceSilence;
    uint8_t   voiceSilence;
    uint8_t   hasTimestampOriginal;
    uint32_t  timestampOriginal;
    uint8_t*  pTimestampBouncedData;
    uint8_t   timestampBouncedSize;
} RtpHeaderExt_t;
    

// Decode the RTP header extension to the struct RtpHeaderExt_t.
int tdav_codec_rtp_extension_decode(tdav_session_audio_t *audio, uint8_t* pExtData, uint32_t extSize, RtpHeaderExt_t *pDecodedExt);

// Encode the pHeaderExt into the RTP header extension buffer ppEncodedExt.
// If *ppEncodedExt is NULL or *pEncodedExtSize is less than the exact output size, the buffer will be
// reallocated. On return, *pEncodedExtSize will contain the exact output size. The caller is responsible
// free the space allocated for *ppEncodedExt.
// The data was padded to be 4 bytes aligned.
// If successful, the exact size of encoded data was returned. Otherwise, 0 was returned.
tsk_size_t tdav_codec_rtp_extension_encode(tdav_session_audio_t *audio, RtpHeaderExt_t *pHeaderExt, void **ppEncodedExt, tsk_size_t *pEncodedExtSize);
    
    
typedef struct VideoRtpHeaderExt_s{
    uint8_t   hasTimestampOriginal;
    uint32_t  timestampOriginal;
    uint8_t*  pTimestampBouncedData;
    uint8_t   timestampBouncedSize;
} VideoRtpHeaderExt_t;
    
    
#ifdef __cplusplus
}
#endif

#endif /* _TDAV_CODEC_RTP_EXTENSION_H_ */
