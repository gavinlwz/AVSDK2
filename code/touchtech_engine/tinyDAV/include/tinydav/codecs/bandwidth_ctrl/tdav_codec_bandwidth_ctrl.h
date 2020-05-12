//
//  tdav_codec_bandwidth_ctrl.h
//  youme_voice_engine
//
//  Created by peter on 7/29/16.
//  Copyright © 2016 Youme. All rights reserved.
//

#ifndef _TDAV_CODEC_BANDWIDTH_CTRL_H_
#define _TDAV_CODEC_BANDWIDTH_CTRL_H_

#ifdef __cplusplus
extern "C" {
#endif 
    
#include <stdint.h>
#include "tinysak_config.h"
    
// Control message type
extern const int32_t BWCtrlMsgServerCommand;
extern const int32_t BWCtrlMsgClientReport;

// FEC type definitions
extern const int32_t BWCTRL_FEC_TYPE_NONE;
extern const int32_t BWCTRL_FEC_TYPE_OPUS;
extern const int32_t BWCTRL_FEC_TYPE_RED;
extern const int32_t BWCTRL_FEC_TYPE_ULP;
extern const int32_t BWCTRL_FEC_TYPE_UXP;
    
    
typedef struct BandwidthCtrlMsg_s {
    uint8_t isValid; // Indicate if the data in this block is valid or obsolete
    int32_t msgType;
    union {
        struct {
            int32_t fecToUse;
            int32_t redunantPercent;
            int32_t redunantGroupSize;
            int32_t targetBitrateBps;
            int32_t originalTimestamp;
            int32_t loopbackTimestamp;
        } server;
        
        struct {
            int32_t forSessionId;
            int32_t packetLossRateQ8;
            int32_t packetLossRateAfterFecQ8;
            int32_t maxConsecutiveLostPackets;
            int32_t loopbackTimestamp;
            int32_t highestFecSupported;
            int32_t originalTimestamp;
        } client;
    } msg;
} BandwidthCtrlMsg_t;
    
// Set all the fields to be invalid. Before calling this function, msgType should be set.
void tdav_codec_bandwidth_ctrl_clear_msg(BandwidthCtrlMsg_t *pMsg);
    
// Decode the bandwidth control block to the struct BandwidthCtrlMsg_t.
int tdav_codec_bandwidth_ctrl_decode(void* pToDecode, uint32_t toDecodeSize, BandwidthCtrlMsg_t *pDecoded);

// Encode the pToEncode into the bandwidth control data buffer ppEncoded.
// If *ppEncoded is NULL or *pEncodedSize is less than the exact output size, the buffer will be
// reallocated. On return, *pEncodedSize will contain the exact output size. The caller is responsible
// free the space allocated for *ppEncoded.
// If successful, the exact size of encoded data was returned. Otherwise, 0 was returned.
tsk_size_t tdav_codec_bandwidth_ctrl_encode(BandwidthCtrlMsg_t *pToEncode, void **ppEncoded, tsk_size_t *pEncodedSize);

#ifdef __cplusplus
}
#endif

#endif /* _TDAV_CODEC_BANDWIDTH_CTRL_H_ */
