//
//  tdav_codec_bandwidth_ctrl.cpp
//  youme_voice_engine
//
//  Created by peter on 7/29/16.
//  Copyright © 2016 Youme. All rights reserved.
//

#include "BandwidthControl.pb.h"
#include "tsk_memory.h"
#include "tsk_debug.h"
#include "tinydav/codecs/bandwidth_ctrl/tdav_codec_bandwidth_ctrl.h"

// Control message type
const int32_t BWCtrlMsgServerCommand = YmBwCtrlProto::CtrlMsgServerCommand;
const int32_t BWCtrlMsgClientReport  = YmBwCtrlProto::CtrlMsgClientReport;

// FEC type definitions
const int32_t BWCTRL_FEC_TYPE_NONE  = YmBwCtrlProto::FecNone;
const int32_t BWCTRL_FEC_TYPE_OPUS  = YmBwCtrlProto::FecOpus;
const int32_t BWCTRL_FEC_TYPE_RED   = YmBwCtrlProto::FecRed;
const int32_t BWCTRL_FEC_TYPE_ULP   = YmBwCtrlProto::FecUlp;
const int32_t BWCTRL_FEC_TYPE_UXP   = YmBwCtrlProto::FecUxp;


void tdav_codec_bandwidth_ctrl_clear_msg(BandwidthCtrlMsg_t *pMsg)
{
    if (!pMsg) {
        return;
    }
    
    pMsg->isValid = 0;
    if (BWCtrlMsgServerCommand == pMsg->msgType) {
        pMsg->msg.server.fecToUse           = -1;
        pMsg->msg.server.redunantPercent    = -1;
        pMsg->msg.server.redunantGroupSize  = -1;
        pMsg->msg.server.targetBitrateBps   = -1;
        pMsg->msg.server.originalTimestamp  = -1;
        pMsg->msg.server.loopbackTimestamp  = -1;
    } else if (BWCtrlMsgClientReport == pMsg->msgType) {
        pMsg->msg.client.forSessionId                = -1;
        pMsg->msg.client.packetLossRateQ8            = -1;
        pMsg->msg.client.packetLossRateAfterFecQ8    = -1;
        pMsg->msg.client.maxConsecutiveLostPackets   = -1;
        pMsg->msg.client.loopbackTimestamp           = -1;
        pMsg->msg.client.highestFecSupported         = -1;
        pMsg->msg.client.originalTimestamp           = -1;
    }
}

int tdav_codec_bandwidth_ctrl_decode(void* pToDecode, uint32_t toDecodeSize, BandwidthCtrlMsg_t *pDecoded)
{
    if (!pToDecode || !toDecodeSize || !pDecoded) {
        return -1;
    }
    
    YmBwCtrlProto::CtrlMsgMain msgMain;
    if (!msgMain.ParseFromArray(pToDecode, toDecodeSize)) {
        TSK_DEBUG_ERROR("Failed to parse bandwidth control data");
        return -1;
    }
    
    if (msgMain.has_msgtype() &&
        (BWCtrlMsgServerCommand == msgMain.msgtype()) &&
        msgMain.has_msgserver())
    {
        pDecoded->msgType = BWCtrlMsgServerCommand;
        tdav_codec_bandwidth_ctrl_clear_msg(pDecoded);

        const YmBwCtrlProto::CtrlMsgServer &msgServer = msgMain.msgserver();
        if (msgServer.has_fectouse()) {
            pDecoded->msg.server.fecToUse = msgServer.fectouse();
        }
        if (msgServer.has_redunantpercent()) {
            pDecoded->msg.server.redunantPercent = msgServer.redunantpercent();
        }
        if (msgServer.has_redunantgroupsize()) {
            pDecoded->msg.server.redunantGroupSize = msgServer.redunantgroupsize();
        }
        if (msgServer.has_targetbitratebps()) {
            pDecoded->msg.server.targetBitrateBps = msgServer.targetbitratebps();
        }
        if (msgServer.has_originaltimestamp()) {
            pDecoded->msg.server.originalTimestamp = msgServer.originaltimestamp();
        }
        if (msgServer.has_loopbacktimestamp()) {
            pDecoded->msg.server.loopbackTimestamp = msgServer.loopbacktimestamp();
        }
    } else if (msgMain.has_msgtype() &&
               (BWCtrlMsgClientReport == msgMain.msgtype()) &&
               msgMain.has_msgclient())
    {
        pDecoded->msgType = BWCtrlMsgClientReport;
        tdav_codec_bandwidth_ctrl_clear_msg(pDecoded);
        
        const YmBwCtrlProto::CtrlMsgClient &msgClient = msgMain.msgclient();
        if (msgClient.has_forsessionid()) {
            pDecoded->msg.client.forSessionId = msgClient.forsessionid();
        }
        if (msgClient.has_packetlossrateq8()) {
            pDecoded->msg.client.packetLossRateQ8 = msgClient.packetlossrateq8();
        }
        if (msgClient.has_packetlossrateafterfecq8()) {
            pDecoded->msg.client.packetLossRateAfterFecQ8 = msgClient.packetlossrateafterfecq8();
        }
        if (msgClient.has_maxconsecutivelostpackets()) {
            pDecoded->msg.client.maxConsecutiveLostPackets = msgClient.maxconsecutivelostpackets();
        }
        if (msgClient.has_loopbacktimestamp()) {
            pDecoded->msg.client.loopbackTimestamp = msgClient.loopbacktimestamp();
        }
        if (msgClient.has_highestfecsupported()) {
            pDecoded->msg.client.highestFecSupported = msgClient.highestfecsupported();
        }
        if (msgClient.has_originaltimestamp()) {
            pDecoded->msg.client.originalTimestamp = msgClient.originaltimestamp();
        }
    }
    return 0;
}

tsk_size_t tdav_codec_bandwidth_ctrl_encode(BandwidthCtrlMsg_t *pToEncode, void **ppEncoded, tsk_size_t *pEncodedSize)
{
    if (!pToEncode || !ppEncoded || !pEncodedSize || !pToEncode->isValid) {
        return 0;
    }
    
    YmBwCtrlProto::CtrlMsgMain msgMain;

    if (BWCtrlMsgClientReport == pToEncode->msgType) {
        YmBwCtrlProto::CtrlMsgClient *pMsgClient = new YmBwCtrlProto::CtrlMsgClient;
        if (NULL == pMsgClient) {
            return 0;
        }
        if (-1 !=  pToEncode->msg.client.forSessionId) {
            pMsgClient->set_forsessionid(pToEncode->msg.client.forSessionId);
        }
        if (-1 !=  pToEncode->msg.client.packetLossRateQ8) {
            pMsgClient->set_packetlossrateq8(pToEncode->msg.client.packetLossRateQ8);
        }
        if (-1 !=  pToEncode->msg.client.packetLossRateAfterFecQ8) {
            pMsgClient->set_packetlossrateafterfecq8(pToEncode->msg.client.packetLossRateAfterFecQ8);
        }
        if (-1 !=  pToEncode->msg.client.maxConsecutiveLostPackets) {
            pMsgClient->set_maxconsecutivelostpackets(pToEncode->msg.client.maxConsecutiveLostPackets);
        }
        if (-1 !=  pToEncode->msg.client.loopbackTimestamp) {
            pMsgClient->set_loopbacktimestamp(pToEncode->msg.client.loopbackTimestamp);
        }
        if (-1 != pToEncode->msg.client.highestFecSupported) {
            pMsgClient->set_highestfecsupported((YmBwCtrlProto::FecType)pToEncode->msg.client.highestFecSupported);
        }
        if (-1 !=  pToEncode->msg.client.originalTimestamp) {
            pMsgClient->set_originaltimestamp(pToEncode->msg.client.originalTimestamp);
        }
        msgMain.set_msgtype(YmBwCtrlProto::CtrlMsgClientReport);
        msgMain.set_msgversion(1);
        msgMain.set_allocated_msgclient(pMsgClient);
    } else if (BWCtrlMsgServerCommand == pToEncode->msgType) {
        YmBwCtrlProto::CtrlMsgServer *pMsgServer = new YmBwCtrlProto::CtrlMsgServer;
        if (NULL == pMsgServer) {
            return 0;
        }
        if (-1 != pToEncode->msg.server.fecToUse) {
            pMsgServer->set_fectouse((YmBwCtrlProto::FecType)pToEncode->msg.server.fecToUse);
        }
        if (-1 != pToEncode->msg.server.redunantPercent) {
            pMsgServer->set_redunantpercent(pToEncode->msg.server.redunantPercent);
        }
        if (-1 != pToEncode->msg.server.redunantGroupSize) {
            pMsgServer->set_redunantgroupsize(pToEncode->msg.server.redunantGroupSize);
        }
        if (-1 != pToEncode->msg.server.targetBitrateBps) {
            pMsgServer->set_targetbitratebps(pToEncode->msg.server.targetBitrateBps);
        }
        if (-1 != pToEncode->msg.server.originalTimestamp) {
            pMsgServer->set_originaltimestamp(pToEncode->msg.server.originalTimestamp);
        }
        if (-1 != pToEncode->msg.server.loopbackTimestamp) {
            pMsgServer->set_loopbacktimestamp(pToEncode->msg.server.loopbackTimestamp);
        }
        
        msgMain.set_msgtype(YmBwCtrlProto::CtrlMsgServerCommand);
        msgMain.set_msgversion(1);
        msgMain.set_allocated_msgserver(pMsgServer);
    }

    // Allocate or enlarge the output buffer if necessary
    tsk_size_t totalSize = (tsk_size_t)msgMain.ByteSize();
    if ((NULL == *ppEncoded) || (*pEncodedSize < totalSize)) {
        *ppEncoded = tsk_realloc(*ppEncoded, totalSize);
        if (NULL == *ppEncoded) {
            TSK_DEBUG_ERROR ("Failed to allocate rtp header extension buffer with size = %zu", totalSize);
            *pEncodedSize = 0;
            return 0;
        }
        *pEncodedSize = totalSize;
    }
    if (!msgMain.SerializeToArray(*ppEncoded, *pEncodedSize)) {
        TSK_DEBUG_ERROR("Failed to serialize bandwidth control data");
        return 0;
    }
                             
    return totalSize;
}
