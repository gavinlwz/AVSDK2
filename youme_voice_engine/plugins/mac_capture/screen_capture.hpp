//
//  screen_capture.hpp
//  youme_voice_engine
//
//  Created by pinky on 2020/5/25.
//  Copyright © 2020 Youme. All rights reserved.
//

#ifndef screen_capture_hpp
#define screen_capture_hpp

#include <stdio.h>


/* callback func: send screen/window capture frame to youmevoiceengine.cpp */
typedef void (*ScreenCb)(char *data, uint32_t len, int width, int height, uint64_t timestamp);

void *InitScreenMedia( int screenid );

void DestroyScreenMedia(void *data);

int SetScreenCaptureFramerate(void *data, int fps);

void SetScreenCaptureVideoCallback(void *data, ScreenCb cb);

int StartScreenStream(void * data);

int StopScreenStream(void * data);

int SetScreenExclusiveWnd( void * data, int wnd );





#endif /* screen_capture_hpp */
