//
//  tadv_codec_bandwidth_ctrl_unittest.cpp
//  youme_voice_engine
//
//  Created by peter on 8/3/16.
//  Copyright © 2016 Youme. All rights reserved.
//

#include "tdav_codec_bandwidth_ctrl_unittest.h"
#ifdef _UNIT_TEST_

#include "tinydav/codecs/bandwidth_ctrl/tdav_codec_bandwidth_ctrl.h"

#include "tsk_debug.h"


static void dump_msg(BandwidthCtrlMsg_t *pMsg, const char* pTitle)
{
    char testStr[1024];
    testStr[0] = '\0';
    TSK_DEBUG_INFO("===================================================");
    TSK_DEBUG_INFO("%s:", pTitle);
    if (BWCtrlMsgServerCommand == pMsg->msgType) {
        TSK_DEBUG_INFO("msgType: serverCmd");
        if (BWCTRL_FEC_TYPE_NONE == pMsg->msg.server.fecToUse) {
            TSK_DEBUG_INFO("fecToUse: BWCTRL_FEC_TYPE_NONE");
        } else if (BWCTRL_FEC_TYPE_OPUS == pMsg->msg.server.fecToUse) {
            TSK_DEBUG_INFO("fecToUse: BWCTRL_FEC_TYPE_OPUS");
        } else if (BWCTRL_FEC_TYPE_RED == pMsg->msg.server.fecToUse) {
            TSK_DEBUG_INFO("fecToUse: BWCTRL_FEC_TYPE_RED");
        } else if (BWCTRL_FEC_TYPE_ULP == pMsg->msg.server.fecToUse) {
            TSK_DEBUG_INFO("fecToUse: BWCTRL_FEC_TYPE_ULP");
        } else if (BWCTRL_FEC_TYPE_UXP == pMsg->msg.server.fecToUse) {
            TSK_DEBUG_INFO("fecToUse: BWCTRL_FEC_TYPE_UXP");
        }
        TSK_DEBUG_INFO("redunantPercent:%d", pMsg->msg.server.redunantPercent);
        TSK_DEBUG_INFO("redunantGroupSize:%d", pMsg->msg.server.redunantGroupSize);
        TSK_DEBUG_INFO("targetBitrateBps:%d", pMsg->msg.server.targetBitrateBps);
        TSK_DEBUG_INFO("originalTimestamp:%d", pMsg->msg.server.originalTimestamp);
        TSK_DEBUG_INFO("loopbackTimestamp:%d", pMsg->msg.server.loopbackTimestamp);
    } else {
        TSK_DEBUG_INFO("msgType: clientReport");
        TSK_DEBUG_INFO("sessionId:%d", pMsg->msg.client.sessionId);
        TSK_DEBUG_INFO("packetLossRateQ8:%d", pMsg->msg.client.packetLossRateQ8);
        TSK_DEBUG_INFO("packetLossRateAfterFecQ8:%d", pMsg->msg.client.packetLossRateAfterFecQ8);
        TSK_DEBUG_INFO("maxConsecutiveLostPackets:%d", pMsg->msg.client.maxConsecutiveLostPackets);
        TSK_DEBUG_INFO("loopbackTimestamp:%d", pMsg->msg.client.loopbackTimestamp);
        if (BWCTRL_FEC_TYPE_NONE == pMsg->msg.client.highestFecSupported) {
            TSK_DEBUG_INFO("highestFec: BWCTRL_FEC_TYPE_NONE");
        } else if (BWCTRL_FEC_TYPE_OPUS == pMsg->msg.client.highestFecSupported) {
            TSK_DEBUG_INFO("highestFec: BWCTRL_FEC_TYPE_OPUS");
        } else if (BWCTRL_FEC_TYPE_RED == pMsg->msg.client.highestFecSupported) {
            TSK_DEBUG_INFO("highestFec: BWCTRL_FEC_TYPE_RED");
        } else if (BWCTRL_FEC_TYPE_ULP == pMsg->msg.client.highestFecSupported) {
            TSK_DEBUG_INFO("highestFec: BWCTRL_FEC_TYPE_ULP");
        } else if (BWCTRL_FEC_TYPE_UXP == pMsg->msg.client.highestFecSupported) {
            TSK_DEBUG_INFO("highestFec: BWCTRL_FEC_TYPE_UXP");
        }
        TSK_DEBUG_INFO("originalTimestamp:%d", pMsg->msg.client.originalTimestamp);
    }
    TSK_DEBUG_INFO("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
}

static void test_one_round(BandwidthCtrlMsg_t *pMsg)
{
    void *pEncoded = NULL;
    uint32_t encodedSize = 0;
    tsk_size_t out_size;
    BandwidthCtrlMsg_t msgDecoded;
    dump_msg(pMsg, "Original data:");
    out_size = tdav_codec_bandwidth_ctrl_encode(pMsg, &pEncoded, &encodedSize);
    TSK_DEBUG_INFO("pEncoded:%p, out_size:%u, encodedSize:%d", pEncoded, out_size, encodedSize);
    tdav_codec_bandwidth_ctrl_decode(pEncoded, encodedSize, &msgDecoded);
    dump_msg(&msgDecoded, "Decoded data:");
    
}

void tdav_codec_bandwidth_ctrl_unittest_do_test()
{
    BandwidthCtrlMsg_t msg;
    
    msg.msgType = BWCtrlMsgServerCommand;
    msg.msg.server.fecToUse = BWCTRL_FEC_TYPE_NONE;
    msg.msg.server.redunantPercent = 20;
    msg.msg.server.redunantGroupSize = 4;
    msg.msg.server.targetBitrateBps = 40000;
    msg.msg.server.originalTimestamp = 2140;
    msg.msg.server.loopbackTimestamp = 7710;
    test_one_round(&msg);

    msg.msgType = BWCtrlMsgServerCommand;
    msg.msg.server.fecToUse = BWCTRL_FEC_TYPE_RED;
    msg.msg.server.redunantPercent = 20;
    msg.msg.server.redunantGroupSize = 4;
    msg.msg.server.targetBitrateBps = 40000;
    msg.msg.server.originalTimestamp = -1;
    msg.msg.server.loopbackTimestamp = -1;
    test_one_round(&msg);

    msg.msgType = BWCtrlMsgServerCommand;
    msg.msg.server.fecToUse = BWCTRL_FEC_TYPE_ULP;
    msg.msg.server.redunantPercent = 20;
    msg.msg.server.redunantGroupSize = -1;
    msg.msg.server.targetBitrateBps = 40000;
    msg.msg.server.originalTimestamp = 3224;
    msg.msg.server.loopbackTimestamp = -1;
    test_one_round(&msg);

    msg.msgType = BWCtrlMsgServerCommand;
    msg.msg.server.fecToUse = BWCTRL_FEC_TYPE_UXP;
    msg.msg.server.redunantPercent = 20;
    msg.msg.server.redunantGroupSize = -1;
    msg.msg.server.targetBitrateBps = 40000;
    msg.msg.server.originalTimestamp = -1;
    msg.msg.server.loopbackTimestamp = 7710;
    test_one_round(&msg);

    msg.msgType = BWCtrlMsgServerCommand;
    msg.msg.server.fecToUse = BWCTRL_FEC_TYPE_OPUS;
    msg.msg.server.redunantPercent = 20;
    msg.msg.server.redunantGroupSize = -1;
    msg.msg.server.targetBitrateBps = 40000;
    msg.msg.server.originalTimestamp = -1;
    msg.msg.server.loopbackTimestamp = 7710;
    test_one_round(&msg);

    msg.msgType = BWCtrlMsgServerCommand;
    msg.msg.server.fecToUse = -1;
    msg.msg.server.redunantPercent = -1;
    msg.msg.server.redunantGroupSize = -1;
    msg.msg.server.targetBitrateBps = -1;
    msg.msg.server.originalTimestamp = -1;
    msg.msg.server.loopbackTimestamp = 117710;
    test_one_round(&msg);

    msg.msgType = BWCtrlMsgClientReport;
    msg.msg.client.sessionId = 201;
    msg.msg.client.packetLossRateQ8 = 15;
    msg.msg.client.packetLossRateAfterFecQ8 = 5;
    msg.msg.client.maxConsecutiveLostPackets = 8;
    msg.msg.client.loopbackTimestamp = 7000041;
    msg.msg.client.highestFecSupported = BWCTRL_FEC_TYPE_UXP;
    msg.msg.client.originalTimestamp = 2000071;
    test_one_round(&msg);

    msg.msgType = BWCtrlMsgClientReport;
    msg.msg.client.sessionId = -1;
    msg.msg.client.packetLossRateQ8 = 15;
    msg.msg.client.packetLossRateAfterFecQ8 = 5;
    msg.msg.client.maxConsecutiveLostPackets = 8;
    msg.msg.client.loopbackTimestamp = -1;
    msg.msg.client.highestFecSupported = BWCTRL_FEC_TYPE_ULP;
    msg.msg.client.originalTimestamp = 2000071;
    test_one_round(&msg);

    msg.msgType = BWCtrlMsgClientReport;
    msg.msg.client.sessionId = 201;
    msg.msg.client.packetLossRateQ8 = 15;
    msg.msg.client.packetLossRateAfterFecQ8 = 5;
    msg.msg.client.maxConsecutiveLostPackets = 8;
    msg.msg.client.loopbackTimestamp = 7000041;
    msg.msg.client.highestFecSupported = BWCTRL_FEC_TYPE_RED;
    msg.msg.client.originalTimestamp = -1;
    test_one_round(&msg);

    msg.msgType = BWCtrlMsgClientReport;
    msg.msg.client.sessionId = -1;
    msg.msg.client.packetLossRateQ8 = 15;
    msg.msg.client.packetLossRateAfterFecQ8 = 5;
    msg.msg.client.maxConsecutiveLostPackets = 8;
    msg.msg.client.loopbackTimestamp = -1;
    msg.msg.client.highestFecSupported = BWCTRL_FEC_TYPE_NONE;
    msg.msg.client.originalTimestamp = -1;
    test_one_round(&msg);


    msg.msgType = BWCtrlMsgClientReport;
    msg.msg.client.sessionId = -1;
    msg.msg.client.packetLossRateQ8 = 15;
    msg.msg.client.packetLossRateAfterFecQ8 = 5;
    msg.msg.client.maxConsecutiveLostPackets = 8;
    msg.msg.client.loopbackTimestamp = -1;
    msg.msg.client.highestFecSupported = BWCTRL_FEC_TYPE_OPUS;
    msg.msg.client.originalTimestamp = -1;
    test_one_round(&msg);

    msg.msgType = BWCtrlMsgClientReport;
    msg.msg.client.sessionId = -1;
    msg.msg.client.packetLossRateQ8 = -1;
    msg.msg.client.packetLossRateAfterFecQ8 = -1;
    msg.msg.client.maxConsecutiveLostPackets = -1;
    msg.msg.client.loopbackTimestamp = -1;
    msg.msg.client.highestFecSupported = -1;
    msg.msg.client.originalTimestamp = 90000009;
    test_one_round(&msg);
}

#endif //_UNIT_TEST_
