//
//  tdav_codec_rtp_extension.c
//  youme_voice_engine
//
//  Created by peter on 7/29/16.
//  Copyright © 2016 Youme. All rights reserved.
//

#include <string.h>
#include "tinydav/codecs/rtp_extension/tdav_codec_rtp_extension.h"
#include "tsk_memory.h"
#include "tsk_debug.h"
#include "tnet_endianness.h"

// Bitwise mask to indicate if a field exist. When a byte is used up, it will wrap around to 1.
// You can add new value to the end of list.
// But, to keep backward compatibilities, DO NOT change the exising values!!!
//
static const uint8_t MASK_BANDWIDTH_CTRL      = 0x01;
static const uint8_t MASK_FEC_OPUS            = 0x02;
static const uint8_t MASK_FEC_RED             = 0x04;
static const uint8_t MASK_FEC_ULP             = 0x08;
static const uint8_t MASK_FEC_UXP             = 0x10;
static const uint8_t MASK_VAD_SEND            = 0x20;
static const uint8_t MASK_TIMESTAMP_ORIGINAL  = 0x40;
static const uint8_t MASK_TIMESTAMP_BOUNCED   = 0x80;

// Definitions of FEC types
const uint8_t RTP_HEADER_EXT_FEC_TYPE_NONE = 0;
const uint8_t RTP_HEADER_EXT_FEC_TYPE_OPUS = MASK_FEC_OPUS;
const uint8_t RTP_HEADER_EXT_FEC_TYPE_RED  = MASK_FEC_RED;
const uint8_t RTP_HEADER_EXT_FEC_TYPE_ULP  = MASK_FEC_ULP;
const uint8_t RTP_HEADER_EXT_FEC_TYPE_UXP  = MASK_FEC_UXP;

#define FEC_TYPE_ALL \
    (RTP_HEADER_EXT_FEC_TYPE_OPUS | \
     RTP_HEADER_EXT_FEC_TYPE_RED  | \
     RTP_HEADER_EXT_FEC_TYPE_ULP  | \
     RTP_HEADER_EXT_FEC_TYPE_UXP)

#define CHECK_SIZE_LEFT(size, offset, minSize) \
    if ((size - offset) < minSize) { \
        return -1; \
    }

int tdav_codec_rtp_extension_decode(tdav_session_audio_t *audio, uint8_t* pExtData, uint32_t extSize, RtpHeaderExt_t *pDecodedExt)
{
    // Get number of mask bytes
    uint32_t offset = 0;
    uint8_t  maskByteNum = 0;
    uint8_t  maskByte1 = 0;
    
    if (!pExtData || !pDecodedExt || (extSize < 4)) {
        return -1;
    }
    
    memset(pDecodedExt, 0, sizeof(RtpHeaderExt_t));
    pDecodedExt->fecType = RTP_HEADER_EXT_FEC_TYPE_NONE;

    //////////////////////////////////////////////////////////////////////////////////
    // Get the mask byte number
    maskByteNum = pExtData[offset++];
    if (0 == maskByteNum) {
        return -1;
    }

    if (maskByteNum >= 1) {
        maskByte1 = pExtData[offset++];
        offset += 2; // skip the header extension size field(2 bytes)
        
    }
    
    CHECK_SIZE_LEFT(extSize, offset, (maskByteNum - 1));
    //
    // In the future, is the mask byte number is greater than 1, handle them here
    //
    //if (maskByteNum >= 2) {
    //    maskByte2 = pExtData[offset++];
    //}
    
    /////////////////////////////////////////////////////////////////////////////////
    // Parse the data sizes
    if (maskByte1 & MASK_BANDWIDTH_CTRL) {
        CHECK_SIZE_LEFT(extSize, offset, sizeof(uint16_t));
        pDecodedExt->bandwidthCtrlSize = tnet_ntohs(*((uint16_t*)&pExtData[offset]));
        offset += sizeof(uint16_t);
    }
    pDecodedExt->fecType = maskByte1 & FEC_TYPE_ALL;
    if (RTP_HEADER_EXT_FEC_TYPE_NONE != pDecodedExt->fecType) {
        CHECK_SIZE_LEFT(extSize, offset, sizeof(uint16_t));
        pDecodedExt->fecSize = tnet_ntohs(*((uint16_t*)&pExtData[offset]));
        offset += sizeof(uint16_t);
    }
    if (maskByte1 & MASK_TIMESTAMP_BOUNCED) {
        CHECK_SIZE_LEFT(extSize, offset, sizeof(uint8_t));
        pDecodedExt->timestampBouncedSize = pExtData[offset];
        offset += sizeof(uint8_t);
    }
    //
    // Added a new size field above ^^^^^^^^^^
    //////////////////////////////////////////////////////////////////////////////////
    
    //////////////////////////////////////////////////////////////////////////////////
    // Retrieve the data
    if (pDecodedExt->bandwidthCtrlSize > 0) {
        CHECK_SIZE_LEFT(extSize, offset, (pDecodedExt->bandwidthCtrlSize));
        pDecodedExt->pBandwidthCtrlData = &pExtData[offset];
        offset += pDecodedExt->bandwidthCtrlSize;
    }

    if (pDecodedExt->fecSize > 0) {
        CHECK_SIZE_LEFT(extSize, offset, (pDecodedExt->fecSize));
        pDecodedExt->pFecData = &pExtData[offset];
        offset += pDecodedExt->fecSize;
    }
    
    if (maskByte1 & MASK_VAD_SEND) {
        CHECK_SIZE_LEFT(extSize, offset, sizeof(uint8_t));
        pDecodedExt->hasVoiceSilence = 1;
        pDecodedExt->voiceSilence = *((uint8_t*)&pExtData[offset]);
        offset += sizeof(uint8_t);
    }

    if (maskByte1 & MASK_TIMESTAMP_ORIGINAL) {
        CHECK_SIZE_LEFT(extSize, offset, sizeof(uint32_t));
        pDecodedExt->hasTimestampOriginal = 1;
        pDecodedExt->timestampOriginal = tnet_ntohl(*((uint32_t*)&pExtData[offset]));
        offset += sizeof(uint32_t);
    }

    if (pDecodedExt->timestampBouncedSize > 0) {
        CHECK_SIZE_LEFT(extSize, offset, (pDecodedExt->timestampBouncedSize));
        pDecodedExt->pTimestampBouncedData = &pExtData[offset];
        offset += pDecodedExt->timestampBouncedSize;
    }
    //
    // Added a new data field above ^^^^^^^^^^
    //////////////////////////////////////////////////////////////////////////////////
    
    return 0;
}


tsk_size_t tdav_codec_rtp_extension_encode(tdav_session_audio_t *audio, RtpHeaderExt_t *pHeaderExt, void **ppEncodedExt, tsk_size_t *pEncodedExtSize)
{
    tsk_size_t totalSize = 4; // the fixed 4 bytes per RFC3550
    uint32_t byteNumForSizes = 0; // number of bytes for the payload sizes
    uint8_t  maskByte1 = 0;
    uint8_t  *pEncodedExt = NULL; // pointer to the output buffer
    uint32_t sizeOff = 0; // offset for the payload size fields
    uint32_t payloadOff = 0; // offset for the payload
    
    
    if (!pHeaderExt || !ppEncodedExt || !pEncodedExtSize) {
        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Prepare the mask byte and calculate the total output size
    if (pHeaderExt->pBandwidthCtrlData && (pHeaderExt->bandwidthCtrlSize > 0)) {
        maskByte1 |= MASK_BANDWIDTH_CTRL;
        byteNumForSizes += sizeof(uint16_t);
        totalSize += (tsk_size_t)pHeaderExt->bandwidthCtrlSize;
    }
    if (pHeaderExt->pFecData && (pHeaderExt->fecSize > 0) && (RTP_HEADER_EXT_FEC_TYPE_NONE != pHeaderExt->fecType)) {
        maskByte1 |= pHeaderExt->fecType;
        byteNumForSizes += sizeof(uint16_t);
        totalSize += (tsk_size_t)pHeaderExt->fecSize;
    }
    if (pHeaderExt->hasVoiceSilence) {
        maskByte1 |= MASK_VAD_SEND;
        totalSize += sizeof(uint8_t);
    }
    if (pHeaderExt->hasTimestampOriginal) {
        // The timestamp field sizes are fixed, so no size field is needed
        maskByte1 |= MASK_TIMESTAMP_ORIGINAL;
        totalSize += sizeof(uint32_t);
    }
    if (pHeaderExt->pTimestampBouncedData && (pHeaderExt->timestampBouncedSize > 0)) {
        maskByte1 |= MASK_TIMESTAMP_BOUNCED;
        byteNumForSizes += sizeof(uint8_t);
        totalSize += (tsk_size_t)pHeaderExt->timestampBouncedSize;
    }
    //
    // Added a new size field above ^^^^^^^^^^
    //////////////////////////////////////////////////////////////////////////////////
    
    totalSize += (tsk_size_t)byteNumForSizes;
    totalSize = ((totalSize + 3) / 4) * 4; // word(4 bytes) algined
    
    // If no payload is going to be carried in the header extension, just return 0
    if (totalSize <= 4) {
        return 0;
    }
    
    // Allocate or enlarge the output buffer if necessary
    if ((NULL == *ppEncodedExt) || (*pEncodedExtSize < totalSize)) {
        *ppEncodedExt = tsk_realloc(*ppEncodedExt, totalSize);
        if (NULL == *ppEncodedExt) {
            TSK_DEBUG_ERROR ("Failed to allocate rtp header extension buffer with size = %zu", totalSize);
            *pEncodedExtSize = 0;
            return 0;
        }
        *pEncodedExtSize = totalSize;
    }
    pEncodedExt = (uint8_t*)*ppEncodedExt;
    
    // Populate the output buffer
    pEncodedExt[0] = 1; // set the mask byte number, currently only one mask byte
    pEncodedExt[1] = maskByte1;
    *((uint16_t*)&pEncodedExt[2]) = tnet_htons((uint16_t)((totalSize / 4) - 1)); // number of words(4 octets) except the first 1 word
    
    sizeOff = 4; // starts after the 4 fixed bytes per RFC3550
    payloadOff = sizeOff + byteNumForSizes; // payloads starts after the fields for the sizes
    
    //////////////////////////////////////////////////////////////////////////////////////////////
    // Set the sizes and copy the payload
    if ( (maskByte1 & MASK_BANDWIDTH_CTRL) && pHeaderExt->pBandwidthCtrlData != NULL) {
        *((uint16_t*)&pEncodedExt[sizeOff]) = tnet_htons(pHeaderExt->bandwidthCtrlSize);
        sizeOff += sizeof(uint16_t);
        memcpy(&pEncodedExt[payloadOff], pHeaderExt->pBandwidthCtrlData, pHeaderExt->bandwidthCtrlSize);
        payloadOff += pHeaderExt->bandwidthCtrlSize;
    }
    if (maskByte1 & FEC_TYPE_ALL) {
        *((uint16_t*)&pEncodedExt[sizeOff]) = tnet_htons(pHeaderExt->fecSize);
        sizeOff += sizeof(uint16_t);
        memcpy(&pEncodedExt[payloadOff], pHeaderExt->pFecData, pHeaderExt->fecSize);
        payloadOff += pHeaderExt->fecSize;
    }
    if (maskByte1 & MASK_VAD_SEND) {
        *((uint8_t*)&pEncodedExt[payloadOff]) = pHeaderExt->voiceSilence;
        payloadOff += sizeof(uint8_t);
    }
    if (maskByte1 & MASK_TIMESTAMP_ORIGINAL) {
        *((uint32_t*)&pEncodedExt[payloadOff]) = tnet_htonl(pHeaderExt->timestampOriginal);
        payloadOff += sizeof(uint32_t);
    }
    if ( (maskByte1 & MASK_TIMESTAMP_BOUNCED ) && (pHeaderExt->pTimestampBouncedData != NULL) ) {
        pEncodedExt[sizeOff] = pHeaderExt->timestampBouncedSize;
        sizeOff += sizeof(uint8_t);
        memcpy(&pEncodedExt[payloadOff], pHeaderExt->pTimestampBouncedData, pHeaderExt->timestampBouncedSize);
        payloadOff += pHeaderExt->timestampBouncedSize;
    }
    //
    // Added a new data field above ^^^^^^^^^^
    //////////////////////////////////////////////////////////////////////////////////
    
    return totalSize;
}
