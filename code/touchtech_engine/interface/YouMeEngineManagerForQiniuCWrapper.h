//
//  YouMeEngineManagerForQiniuCWrapper.h
//  youme_voice_engine
//
//  Created by mac on 2017/8/14.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef YouMeEngineManagerForQiniuCWrapper_h
#define YouMeEngineManagerForQiniuCWrapper_h

#ifdef __cplusplus
extern "C" {
#endif
    void YM_Qiniu_onAudioFrameCallback(int sessionId, void* data, int len, uint64_t timestamp);
    void YM_Qiniu_onAudioFrameMixedCallback(void* data, int len, uint64_t timestamp);
#ifdef __cplusplus
}
#endif

#endif /* YouMeEngineManagerForQiniuCWrapper_h */
