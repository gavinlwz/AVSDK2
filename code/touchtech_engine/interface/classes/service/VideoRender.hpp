//
//  VideoRender.hpp
//  youme_voice_engine
//
//  Created by mac on 2017/6/8.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef VideoRender_hpp
#define VideoRender_hpp

#include "IYouMeVideoCallback.h"
#include <YouMeCommon/CrossPlatformDefine/PlatformDef.h>
#include <string>

class VideoRender {
    
public:
    std::string userId;
    int renderId;
    int sessionId;

    VideoRender(std::string userId, int renderId, int sessionId);
    std::string toString();

};

#endif /* VideoRender_hpp */
