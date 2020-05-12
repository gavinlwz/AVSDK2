//
//  Log.cpp
//  MultiScreen
//
//  Created by brucewang on 12-12-18.
//  Copyright (c) 2012年 brucewang. All rights reserved.
//


#include "stdarg.h"
#include "stdio.h"
#include "tsk_debug.h"
#include "tsk_log.h"
#include "tsk_string.h"
#include "tsk_time.h"
#include "tsk_memory.h"
#include <time.h>
#ifdef WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif

#if ANDROID
#include <android/log.h>
#endif
#include "XOsWrapper.h"

#define TSK_LOG_BUFSIZE 2048

#define BUILDTIME __TIME__
#define BUILDDATE __DATE__
// example: "May 30 2019 12:05:30"
#define BUILDINFO BUILDDATE " " BUILDTIME

static FILE *gLogFile = NULL;
static tsk_mutex_handle_t* gLogFileMutex = tsk_null;
static uint64_t gLogFileSize = 0;
static uint64_t gMaxLogFileSize = 10*1024*1024; // 10MB by default
static char *gLogFilePath = NULL;
static char *gLogFilePathBak = NULL;
static uint32_t gMaxLogLevelConsole = TSK_LOG_INFO;
static uint32_t gMaxLogLevelFile = TSK_LOG_INFO;


void tsk_init_log (const char *szfilePath, const char *szfilePathBak)
{
    if (NULL != gLogFile)
    {
        TSK_DEBUG_INFO("tsk log no need to init");
        return;
    }
    if (NULL == szfilePath)
    {
        TSK_DEBUG_ERROR("tsk log invalid param: szfilePath:%p", szfilePath);
        return;
    }
    int pathLen = strlen(szfilePath) + 1;
    int pathLenBak = 0;
    gLogFilePath = tsk_calloc(1, pathLen);
    if (!gLogFilePath) {
        // If this happens, the system is actually not usable.
        return;
    }
    if (szfilePathBak) {
        pathLenBak = strlen(szfilePathBak) + 1;
        gLogFilePathBak = tsk_calloc(1, pathLenBak);
        if (!gLogFilePathBak) {
            // If this happens, the system is actually not usable.
            return;
        }
    }

    strncpy(gLogFilePath, szfilePath, pathLen);
    if (gLogFilePathBak) {
        strncpy(gLogFilePathBak, szfilePathBak, pathLenBak);
    }

    gLogFileMutex = tsk_mutex_create(tsk_true);
    if (gMaxLogLevelFile){
        //文件不存在的时候就w+
        gLogFile = fopen (gLogFilePath, "r+");
        if (NULL == gLogFile) {
            gLogFile = fopen (gLogFilePath, "w+");
        }
        if (NULL != gLogFile) {
            XFSeek(gLogFile, 0, SEEK_END);
            gLogFileSize = (uint64_t)XFTell(gLogFile);
            TSK_DEBUG_INFO("-------------------------------------------------------------------------");
            TSK_DEBUG_INFO("Build info: %s", BUILDINFO);
        }
    }
}

void tsk_uninit_log ()
{
    if (NULL != gLogFile) {
        fclose (gLogFile);
        gLogFile = NULL;
    }
    
    if (gLogFileMutex) {
        tsk_mutex_destroy(&gLogFileMutex);
        gLogFileMutex = tsk_null;
    }

    if (gLogFilePathBak) {
        tsk_free((void**)&gLogFilePathBak);
    }
    if (gLogFilePath) {
        tsk_free((void**)&gLogFilePath);
    }
}

void tsk_set_log_maxFileSizeKb(uint32_t maxLogFileSizeKb)
{
    gMaxLogFileSize = (uint64_t)maxLogFileSizeKb * 1024;
}

void tsk_set_log_maxLevelConsole(uint32_t maxLogLevelConsole)
{
    gMaxLogLevelConsole = maxLogLevelConsole;
}

void tsk_set_log_maxLevelFile(uint32_t maxLogLevelFile)
{
    gMaxLogLevelFile = maxLogLevelFile;
}

const char *LogLevelToString (int dLevel)
{
    switch (dLevel)
    {
    case TSK_LOG_ERROR:
        return "ERROR";

    case TSK_LOG_WARNING:
        return "WARNING";

    case TSK_LOG_FATAL:
        return "FATAL";

    case TSK_LOG_DEBUG:
        return "DEBUG";

    case TSK_LOG_INFO:
        return "INFO";

    case TSK_LOG_VERBOSE:
        return "VERBOSE";

    default:
        return "UNDEFINED";
    }
}
#if ANDROID
int LogLevelToAndroidLevel (int dLevel)
{
    switch (dLevel)
    {
    case TSK_LOG_ERROR:
        return ANDROID_LOG_ERROR;

    case TSK_LOG_WARNING:
        return ANDROID_LOG_WARN;

    case TSK_LOG_FATAL:
        return ANDROID_LOG_FATAL;

    case TSK_LOG_DEBUG:
        return ANDROID_LOG_DEBUG;

    case TSK_LOG_INFO:
        return ANDROID_LOG_INFO;

    case TSK_LOG_VERBOSE:
        return ANDROID_LOG_VERBOSE;

    default:
        return ANDROID_LOG_INFO;
    }
}
#endif


void tsk_log_imp (const char *pszFun, const char *pszFile, int dLine, int dLevel, const char *pszFormat, ...)
{
    tsk_bool_t writeToConsole = tsk_false;
    tsk_bool_t writeToFile = tsk_false;
    if (dLevel <= gMaxLogLevelConsole) {
        writeToConsole = tsk_true;
    }
    if (dLevel <= gMaxLogLevelFile) {
        writeToFile = tsk_true;
    }
    
    if (!writeToConsole && !writeToFile) {
        return;
    }

    char strtime[20] = { 0 };
    time_t timep = time (NULL);
    struct tm *p_tm = localtime (&timep);
    strftime (strtime, sizeof (strtime), "%Y-%m-%d %H:%M:%S", p_tm);
    char buffer[TSK_LOG_BUFSIZE + 1] = {0};
#ifdef WIN32
	int dSize = _snprintf(buffer, TSK_LOG_BUFSIZE, "thread: %d %s %03d %s ",
		GetCurrentThreadId(), strtime, (int)(tsk_gettimeofday_ms() % 1000), LogLevelToString (dLevel));

	if (dSize < TSK_LOG_BUFSIZE)
	{
		va_list args;
		va_start (args, pszFormat);
		dSize += vsnprintf (buffer + dSize, TSK_LOG_BUFSIZE - dSize, pszFormat, args);
		va_end (args);
	}
	int iFileNameBegin = tsk_strLastIndexOf(pszFile, strlen(pszFile), "\\");

	const char *szFileName = pszFile + iFileNameBegin + 1;
	if (dSize < TSK_LOG_BUFSIZE)
	{
		_snprintf(buffer + dSize, TSK_LOG_BUFSIZE - dSize, " [%s#%s:%d]\n", pszFun, szFileName, dLine);
	}
#else
	int dSize = snprintf(buffer, TSK_LOG_BUFSIZE, "thread：%lu %s.%03d %-8s ",
		(unsigned long)pthread_self(), strtime, (int)(tsk_gettimeofday_ms() % 1000), LogLevelToString(dLevel));

	if (dSize < TSK_LOG_BUFSIZE)
	{
		va_list args;
		va_start(args, pszFormat);
		dSize += vsnprintf(buffer + dSize, TSK_LOG_BUFSIZE - dSize, pszFormat, args);
		va_end(args);
	}

	int iFileNameBegin = tsk_strLastIndexOf(pszFile, strlen(pszFile), "/");
	const char *szFileName = pszFile + iFileNameBegin + 1;
	if (dSize < TSK_LOG_BUFSIZE)
	{
		snprintf(buffer + dSize, TSK_LOG_BUFSIZE - dSize, " [%s#%s:%d]\n", pszFun, szFileName, dLine);
	}
#endif // WIN32

  
    if (writeToConsole) {
#if ANDROID
        int iAndroidLogLevel = LogLevelToAndroidLevel (dLevel);
        __android_log_write (iAndroidLogLevel, "YOUME", buffer);
#elif defined(WIN32)
	    OutputDebugString(buffer);
#else
        printf ("%s", buffer);
#endif
    }
    
    if (gLogFileMutex) {
        tsk_mutex_lock(gLogFileMutex);
        if (gLogFile && writeToFile) {
            if (gLogFileSize >= gMaxLogFileSize) {
                if (gLogFilePathBak) {
                    fclose(gLogFile);
                    remove(gLogFilePathBak);
                    rename(gLogFilePath, gLogFilePathBak);
                    gLogFile = fopen (gLogFilePath, "w+");
                    gLogFileSize = 0;
                }else {
                    fclose(gLogFile);
                    remove(gLogFilePath);
                    gLogFile = fopen (gLogFilePath, "w+");
                    gLogFileSize = 0;
                }
            }
            if (gLogFile) {
                int logInfoLen = strlen (buffer);
                gLogFileSize += logInfoLen;
                fwrite (buffer, 1, logInfoLen, gLogFile);
                fflush (gLogFile);
            }
        }
        tsk_mutex_unlock(gLogFileMutex);
    }
}

static const char hexdig[] = "0123456789abcdef";
void tsk_log_hex_imp (const char *pszFun, const char *pszFile, int dLine, int dLevel, const uint8_t *data, unsigned long len)
{
    unsigned long i;
    char line[50], *ptr;

    ptr = line;

    for (i = 0; i < len; i++)
    {
        *ptr++ = hexdig[0x0f & (data[i] >> 4)];
        *ptr++ = hexdig[0x0f & data[i]];
        if ((i & 0x0f) == 0x0f)
        {
            *ptr = '\0';
            ptr = line;
            tsk_log_imp (pszFun, pszFile, dLine, dLevel, "%s", line);
        }
        else
        {
            *ptr++ = ' ';
        }
    }
    if (i & 0x0f)
    {
        *ptr = '\0';
        tsk_log_imp (pszFun, pszFile, dLine, dLevel, "%s", line);
    }
}
