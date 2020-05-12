//
//  XOsWrapper.h
//  youme_voice_engine
//
//  Created by peterzhuang on 09/19/2016.
//  Copyright © 2016年 YOUME. All rights reserved.
//
 
#ifndef _X_OS_Wrapper_h_
#define _X_OS_Wrapper_h_

#ifdef WIN32

#define XFSeek _fseeki64
#define XFTell _ftelli64
#define XSleep(x) Sleep(x)
#define  XPreferredSeparator "\\"
#else

#include <unistd.h>

#define XSleep(x) usleep(x*1000)
#define XFSeek fseeko
#define XFTell ftello
#define  XPreferredSeparator "/"
#endif // not WIN32

#endif /* _X_OS_Wrapper_h_ */
