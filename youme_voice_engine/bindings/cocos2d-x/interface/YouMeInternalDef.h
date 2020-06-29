//
//  YouMeInternalDef.h
//  youme_voice_engine
//
//  Created by peter on 10/01/2017.
//  Copyright © 2017 Youme. All rights reserved.
//

#ifndef _YouMeInternalDef_h_
#define _YouMeInternalDef_h_


typedef enum ConferenceState {
    STATE_INITIALIZING,         /* 初始化中 */
    STATE_INIT_FAILED,          /* 初始化失败，允许再次初始化或反初始化 */
    STATE_INITIALIZED,          /* 初始化完成 */
    STATE_UNINITIALIZED         /* 反初始化完成 */
} ConferenceState_t;

typedef enum COMMON_STATUS_EVENT_TYPE {
    MIC_STATUS = 0,
    SPEAKER_STATUS = 1,
    MIC_CTR_STATUS = 2,
    SPEAKER_CTR_STATUS = 3,
    AVOID_STATUS = 4,
    IDENTITY_MODIFY = 5
}STATUS_EVENT_TYPE_t;



#endif /* _YouMeInternalDef_h_ */
