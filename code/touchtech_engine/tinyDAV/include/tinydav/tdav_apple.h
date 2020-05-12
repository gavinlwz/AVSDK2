/*******************************************************************
 *  Copyright(c) 2015-2020 YOUME All rights reserved.
 *
 *  简要描述:游密音频通话引擎
 *
 *  当前版本:1.0
 *  作者:brucewang
 *  日期:2015.9.30
 *  说明:对外发布接口
 *
 *  取代版本:0.9
 *  作者:brucewang
 *  日期:2015.9.15
 *  说明:内部测试接口
 ******************************************************************/

#ifndef TDAV_APPLE_H
#define TDAV_APPLE_H

#include "tinydav_config.h"

#if TDAV_UNDER_APPLE

TDAV_BEGIN_DECLS
void tdav_InitCategory(int needRecord, int outputToSpeaker);
void tdav_RestoreCategory();
int tdav_IsPlayOnlyCategory();
void tdav_SetCategory();
int tdav_getIsAudioSessionInChatMode();
int tdav_apple_init ();
int tdav_apple_enable_audio ();
int tdav_apple_deinit ();
void tdav_apple_resetOutputTarget (int isSpeakerOn);
int tdav_apple_setOutputoSpeaker (int enable);
int hasHeadset ();
int tdav_apple_getRecordPermission(); // 1: granted; 0:denied; -1: undefined
int tdav_apple_isAirPlayOn();
void tdav_apple_monitor_add(int bIsConsumer);
void tdav_apple_monitor_test_disable();

void tdav_SetTestCategory(int bCategory);

//检查是否同时有audio的输入输出设备，mac上如果不同时拥有，需要使用不同的模式
int tdav_mac_has_both_audio_device();
int tdav_mac_update_use_audio_device();
int tdav_mac_cur_record_device();
void tdav_mac_set_choose_device(const char* deviceUid);

TDAV_END_DECLS

#endif /* TDAV_UNDER_APPLE */

#endif /* TDAV_APPLE_H */
