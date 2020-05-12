//
//  tdav_codec_rtp_extension_unittest.cpp
//  youme_voice_engine
//
//  Created by peter on 8/3/16.
//  Copyright © 2016 Youme. All rights reserved.
//

#include "tdav_codec_rtp_extension_unittest.h"

#ifdef _UNIT_TEST_

#include "tinydav/codecs/rtp_extension/tdav_codec_rtp_extension.h"
#include "tsk_debug.h"
#include <assert.h>

static char *g_bandwidthCtrlData = (char*)"Bandwidth control test data";
static char *g_fecOpusData = (char*)"FEC opus in-band test data";
static char *g_fecRedData = (char*)"FEC red test data";
static char *g_fecUlpData = (char*)"FEC ulp test data";
static char *g_fecUxpData = (char*)"FEC uxp test data";


static void dump_rtp_header_extension(RtpHeaderExt_t *pRtpHeaderExt, const char* pTitle)
{
    char testStr[1024];
    testStr[0] = '\0';
    TSK_DEBUG_INFO("===================================================");
    TSK_DEBUG_INFO("%s:", pTitle);
    if (pRtpHeaderExt->pBandwidthCtrlData) {
        memcpy(testStr, pRtpHeaderExt->pBandwidthCtrlData, pRtpHeaderExt->bandwidthCtrlSize);
        testStr[pRtpHeaderExt->bandwidthCtrlSize] = '\0';
    }
    TSK_DEBUG_INFO("pBandwidthCtrlData:%s", testStr);
    TSK_DEBUG_INFO("bandwidthCtrlSize:%d", pRtpHeaderExt->bandwidthCtrlSize);
    if (RTP_HEADER_EXT_FEC_TYPE_NONE == pRtpHeaderExt->fecType) {
        TSK_DEBUG_INFO("FEC_TYPE_NONE");
    } else if (RTP_HEADER_EXT_FEC_TYPE_OPUS == pRtpHeaderExt->fecType) {
        TSK_DEBUG_INFO("FEC_TYPE_OPUS");
    } else if (RTP_HEADER_EXT_FEC_TYPE_RED == pRtpHeaderExt->fecType) {
        TSK_DEBUG_INFO("FEC_TYPE_RED");
    } else if (RTP_HEADER_EXT_FEC_TYPE_ULP == pRtpHeaderExt->fecType) {
        TSK_DEBUG_INFO("FEC_TYPE_ULP");
    } else if (RTP_HEADER_EXT_FEC_TYPE_UXP == pRtpHeaderExt->fecType) {
        TSK_DEBUG_INFO("FEC_TYPE_UXP");
    } else {
        TSK_DEBUG_INFO("Unknown FEC type");
    }
    
    memcpy(testStr, pRtpHeaderExt->pFecData, pRtpHeaderExt->fecSize);
    testStr[pRtpHeaderExt->fecSize] = '\0';
    TSK_DEBUG_INFO("pFecData:%s", testStr);
    TSK_DEBUG_INFO("fecSize:%d", pRtpHeaderExt->fecSize);
    TSK_DEBUG_INFO("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
}

static void test_one_round(RtpHeaderExt_t *pHeaderExtOri)
{
    void* pEncodedExt = NULL;
    RtpHeaderExt_t headerExtDecoded = { 0 };
    uint32_t encodedExtSize = 0;
    int ret = 0;

    dump_rtp_header_extension(pHeaderExtOri, "Data before encoding:");
    ret = tdav_codec_rtp_extension_encode(pHeaderExtOri, &pEncodedExt, &encodedExtSize);
    if (0 != ret) {
        TSK_DEBUG_ERROR("Encoding returns error:%d", ret);
    }
    TSK_DEBUG_INFO("pEncodedExt:%p, encodedExtSize:%u", pEncodedExt, encodedExtSize);
    ret = tdav_codec_rtp_extension_decode((uint8_t*)pEncodedExt, encodedExtSize, &headerExtDecoded);
    dump_rtp_header_extension(&headerExtDecoded, "Decoded data:");
}

void tdav_codec_rtp_extension_unittest_do_test()
{
    RtpHeaderExt_t headerExtOri = { 0 };
    
    headerExtOri.pBandwidthCtrlData = (uint8_t*)g_bandwidthCtrlData;
    headerExtOri.bandwidthCtrlSize = strlen(g_bandwidthCtrlData);
    headerExtOri.fecType = RTP_HEADER_EXT_FEC_TYPE_OPUS;
    headerExtOri.pFecData = (uint8_t*)g_fecOpusData;
    headerExtOri.fecSize = strlen(g_fecOpusData);
    test_one_round(&headerExtOri);

    headerExtOri.pBandwidthCtrlData = NULL;
    headerExtOri.bandwidthCtrlSize = 0;
    headerExtOri.fecType = RTP_HEADER_EXT_FEC_TYPE_NONE;
    headerExtOri.pFecData = NULL;
    headerExtOri.fecSize = 0;
    test_one_round(&headerExtOri);

    headerExtOri.pBandwidthCtrlData = (uint8_t*)g_bandwidthCtrlData;
    headerExtOri.bandwidthCtrlSize = strlen(g_bandwidthCtrlData);
    headerExtOri.fecType = RTP_HEADER_EXT_FEC_TYPE_NONE;
    headerExtOri.pFecData = NULL;
    headerExtOri.fecSize = 0;
    test_one_round(&headerExtOri);
    
    headerExtOri.pBandwidthCtrlData = NULL;
    headerExtOri.bandwidthCtrlSize = 0;
    headerExtOri.fecType = RTP_HEADER_EXT_FEC_TYPE_ULP;
    headerExtOri.pFecData = (uint8_t*)g_fecUlpData;
    headerExtOri.fecSize = strlen(g_fecUlpData);
    test_one_round(&headerExtOri);

    headerExtOri.pBandwidthCtrlData = NULL;
    headerExtOri.bandwidthCtrlSize = strlen(g_bandwidthCtrlData);
    headerExtOri.fecType = RTP_HEADER_EXT_FEC_TYPE_RED;
    headerExtOri.pFecData = (uint8_t*)g_fecRedData;
    headerExtOri.fecSize = strlen(g_fecRedData);
    test_one_round(&headerExtOri);

    headerExtOri.pBandwidthCtrlData = (uint8_t*)g_bandwidthCtrlData;
    headerExtOri.bandwidthCtrlSize = 0;
    headerExtOri.fecType = RTP_HEADER_EXT_FEC_TYPE_UXP;
    headerExtOri.pFecData = (uint8_t*)g_fecUxpData;
    headerExtOri.fecSize = strlen(g_fecUxpData);
    test_one_round(&headerExtOri);
}



#endif //_UNIT_TEST_
