//
//  IYouMeEventCallback.h
//  youme_voice_engine
//
//  Created by fire on 2016/12/14.
//  Copyright © 2016年 Youme. All rights reserved.
//

#ifndef IYouMeEventCallback_h
#define IYouMeEventCallback_h

#include "YouMeConstDefine.h"
#include <list>
#include <string>

class IYouMeEventCallback
{
public:
    virtual void onEvent(const YouMeEvent event, const YouMeErrorCode error, const char * channel, const char * param) = 0;
    
    
    // virtual void onRequestRestAPI( int requestID, const YouMeErrorCode &iErrorCode, const char* strQuery, const char*  strResult ) = 0;

    virtual void onMemberChange( const char* channel, const char* listMemberChangeJson, bool bUpdate ) = 0 ;
    // virtual void onMemberChange( const char*  channel, const char* userID,bool isJoin, bool bUpdate) { } ;

	// virtual void onBroadcast(const YouMeBroadcast bc, const char* channel, const char* param1, const char* param2, const char* strContent) = 0;

	// virtual void OnCustomDataNotify(const void* pData, int iDataLen, unsigned long long ulTimeSpan) = 0;

   virtual void onAVStatistic( YouMeAVStatisticType type,  const char* userID,  int value ) = 0 ;

    //需要先调用setVideoPreDecodeCallBackEnable
//     virtual void onVideoPreDecode(const char* userId, void* data, int dataSizeInByte, unsigned long long timestamp) = 0;

    /*
     * 功能：文本翻译回调
     * @param errorcode：错误码
     * @param requestID：请求ID（与translateText接口输出参数requestID一致）
     * @param text：翻译结果
     * @param srcLangCode：源语言编码
     * @param destLangCode：目标语言编码
     */
//     virtual void onTranslateTextComplete(YouMeErrorCode errorcode, unsigned int requestID, const std::string& text, YouMeLanguageCode srcLangCode, YouMeLanguageCode destLangCode) = 0 ;

};


struct MemberChange{
    const char* userID;
    bool isJoin;
};

#endif /* IYouMeEventCallback_h */
