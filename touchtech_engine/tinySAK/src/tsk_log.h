//
//  MTsLog.h
//  MultiScreen
//
//  Created by brucewang on 12-12-18.
//  Copyright (c) 2012年 brucewang. All rights reserved.
//

#ifndef _TSK_LOG_H_
#define _TSK_LOG_H_

#define TSK_LOG(level, ...) tsk_log_imp (__FUNCTION__, __FILE__, __LINE__, (level), __VA_ARGS__)
#define TSK_LOG_HEX(level, data, len) \
    MTSLogHexImp (__FUNCTION__, __FILE__, __LINE__, (level), data, len)
#define TSK_TRACE TSK_LOG (MTSLOG_DEBUG, "Enter ")

enum TSK_LOG_LEVEL
{
    TSK_LOG_DISABLED = 0,
    TSK_LOG_FATAL = 1,
    TSK_LOG_ERROR = 10,
    TSK_LOG_WARNING = 20,
    TSK_LOG_INFO = 40,
    TSK_LOG_DEBUG = 50,
    TSK_LOG_VERBOSE = 60
};



void tsk_init_log (const char *szfilePath, const char *szBakFilePath);
void tsk_uninit_log ();
void tsk_set_log_maxFileSizeKb(uint32_t maxLogFileSizeKb);
void tsk_set_log_maxLevelConsole(uint32_t maxLogLevelConsole);
void tsk_set_log_maxLevelFile(uint32_t maxLogLevelFile);

void tsk_log_imp (const char *pszFun, const char *pszFile, int dLine, int dLevel, const char *pszFormat, ...);
void tsk_log_hex_imp (const char *pszFun, const char *pszFile, int dLine, int level, const uint8_t *data, unsigned long len);

#endif
