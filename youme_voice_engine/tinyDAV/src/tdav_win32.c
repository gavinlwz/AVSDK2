﻿/*******************************************************************
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
/**@file tdav_win32.c
 * @brief tinyDAV WIN32 helper functions.
 *
 */
#include "tinydav/tdav_win32.h"

#if TDAV_UNDER_WINDOWS

#include "tsk_string.h"
#include "tsk_memory.h"
#include "tsk_debug.h"

#include <windows.h>
#include <MMSystem.h>
#if !TDAV_UNDER_WINDOWS_RT
#include <Shlwapi.h> /* PathRemoveFileSpec */
#endif

/* https://msdn.microsoft.com/en-us/library/windows/desktop/ms724834%28v=vs.85%29.aspx
Version Number    Description
6.2				  Windows 8 / Windows Server 2012
6.1               Windows 7     / Windows 2008 R2
6.0               Windows Vista / Windows 2008
5.2               Windows 2003 
5.1               Windows XP
5.0               Windows 2000
*/
static DWORD dwMajorVersion = -1;
static DWORD dwMinorVersion = -1;

#if (TDAV_UNDER_WINDOWS_RT || TDAV_UNDER_WINDOWS_CE)
const HMODULE GetCurrentModule()
{
	return NULL;
}
#else
const HMODULE GetCurrentModule()
{
	HMODULE hm = {0};
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)GetCurrentModule, &hm);   
    return hm;
}
#endif /* !(TDAV_UNDER_WINDOWS_RT || TDAV_UNDER_WINDOWS_CE) */

int tdav_win32_init()
{
#if !TDAV_UNDER_WINDOWS_RT
	MMRESULT result;
	
#if !TDAV_UNDER_WINDOWS_CE
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
#endif

	// Timers accuracy
	result = timeBeginPeriod(1);
	if (result) {
		TSK_DEBUG_ERROR("timeBeginPeriod(1) returned result=%u", result);
	}

	// Get OS version
	if(dwMajorVersion == -1 || dwMinorVersion == -1){
		OSVERSIONINFO osvi;
		ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&osvi);
		dwMajorVersion = osvi.dwMajorVersion;
		dwMinorVersion = osvi.dwMinorVersion;
#if TDAV_UNDER_WINDOWS_CE && (BUILD_TYPE_GE && SIN_CITY)
		TSK_DEBUG_INFO("Windows dwMajorVersion=%ld, dwMinorVersion=%ld\n", dwMajorVersion, dwMinorVersion);
#else
		fprintf(stdout, "Windows dwMajorVersion=%ld, dwMinorVersion=%ld\n", dwMajorVersion, dwMinorVersion);
#endif
	}
#endif

	return 0;
}

int tdav_win32_get_osversion(unsigned long* version_major, unsigned long* version_minor)
{
	if (version_major) {
		*version_major = dwMajorVersion;
	}
	if (version_minor) {
		*version_minor = dwMinorVersion;
	}
	return 0;
}

tsk_bool_t tdav_win32_is_win8_or_later()
{
	if (dwMajorVersion == -1 || dwMinorVersion == -1) {
		TSK_DEBUG_ERROR("Version numbers are invalid");
		return tsk_false;
	}
	return ((dwMajorVersion > 6) || ((dwMajorVersion == 6) && (dwMinorVersion >= 2)));
}

tsk_bool_t tdav_win32_is_win7_or_later()
{
	if (dwMajorVersion == -1 || dwMinorVersion == -1) {
		TSK_DEBUG_ERROR("Version numbers are invalid");
		return tsk_false;
	}
	return ( (dwMajorVersion > 6) || ( (dwMajorVersion == 6) && (dwMinorVersion >= 1) ) );
}

tsk_bool_t tdav_win32_is_winvista_or_later()
{
	if (dwMajorVersion == -1 || dwMinorVersion == -1) {
		TSK_DEBUG_ERROR("Version numbers are invalid");
		return tsk_false;
	}
	return (dwMajorVersion >= 6);
}

tsk_bool_t tdav_win32_is_winxp_or_later()
{
	if (dwMajorVersion == -1 || dwMinorVersion == -1) {
		TSK_DEBUG_ERROR("Version numbers are invalid");
		return tsk_false;
	}
	return ( (dwMajorVersion > 5) || ( (dwMajorVersion == 5) && (dwMinorVersion >= 1) ) );
}

const char* tdav_get_current_directory_const()
{
#if TDAV_UNDER_WINDOWS_RT
	TSK_DEBUG_ERROR("Not supported");
	return tsk_null;
#else
	static char CURRENT_DIR_PATH[MAX_PATH] = { 0 };
	static DWORD CURRENT_DIR_PATH_LEN = 0;
	if (CURRENT_DIR_PATH_LEN == 0) {
		// NULL HMODULE will get the path to the executable not the DLL. When runing the code in Internet Explorer this is a BIG issue as the path is where IE.exe is installed.
#if TDAV_UNDER_WINDOWS_CE
		static wchar_t TMP_CURRENT_DIR_PATH[MAX_PATH] = { 0 };
		if ((CURRENT_DIR_PATH_LEN = GetModuleFileName(GetCurrentModule(), TMP_CURRENT_DIR_PATH, sizeof(TMP_CURRENT_DIR_PATH)))) {
			if ((CURRENT_DIR_PATH_LEN = wcstombs(CURRENT_DIR_PATH, TMP_CURRENT_DIR_PATH, sizeof(CURRENT_DIR_PATH) - 1))) {
				int idx = tsk_strLastIndexOf(CURRENT_DIR_PATH, CURRENT_DIR_PATH_LEN, "\\");
				if (idx > -1) {
					CURRENT_DIR_PATH[idx] = '\0';
					CURRENT_DIR_PATH_LEN = idx;
				}
			}
		}
#else
		if ((CURRENT_DIR_PATH_LEN = GetModuleFileNameA(GetCurrentModule(), CURRENT_DIR_PATH, sizeof(CURRENT_DIR_PATH)))) {
			if (!PathRemoveFileSpecA(CURRENT_DIR_PATH)) {
				TSK_DEBUG_ERROR("PathRemoveFileSpec(%s) failed: %x", CURRENT_DIR_PATH, GetLastError());
				memset(CURRENT_DIR_PATH, 0, sizeof(CURRENT_DIR_PATH));
				CURRENT_DIR_PATH_LEN = 0;
			}
		}
#endif /* TDAV_UNDER_WINDOWS_CE */
		if (!CURRENT_DIR_PATH_LEN) {
			TSK_DEBUG_ERROR("GetModuleFileNameA() failed: %x", GetLastError());
		}
	}
	return CURRENT_DIR_PATH;
#endif /* TDAV_UNDER_WINDOWS_RT */
}

TINYDAV_API void tdav_win32_print_error(const char* func, HRESULT hr)
{
	CHAR message[1024] = {0};

#if (TDAV_UNDER_WINDOWS_RT || TDAV_UNDER_WINDOWS_CE)
#if !defined(WC_ERR_INVALID_CHARS)
#define WC_ERR_INVALID_CHARS 0
#endif
	// FormatMessageA not allowed on the Store
	static WCHAR wBuff[1024] = {0};
	FormatMessageW(
		  FORMAT_MESSAGE_FROM_SYSTEM, 
		  tsk_null,
		  hr,
		  0,
		  wBuff, 
		  sizeof(wBuff)-1,
		  tsk_null);
	WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wBuff, wcslen(wBuff), message, sizeof(message) - 1, NULL, NULL);
#else
	FormatMessageA
	(
#if !TDAV_UNDER_WINDOWS_RT
	  FORMAT_MESSAGE_ALLOCATE_BUFFER | 
#endif
	  FORMAT_MESSAGE_FROM_SYSTEM, 
	  tsk_null,
	  hr,
	  0,
	  message, 
	  sizeof(message) - 1,
	  tsk_null);
#endif

	TSK_DEBUG_ERROR("%s(): %s", func, message);
}

int tdav_win32_deinit()
{
#if !(TDAV_UNDER_WINDOWS_RT || TDAV_UNDER_WINDOWS_CE)
	MMRESULT result;

	// Timers accuracy
	result = timeEndPeriod(1);
	if(result){
		TSK_DEBUG_ERROR("timeEndPeriod(1) returned result=%u", result);
	}
	CoUninitialize();
#endif

	return 0;
}

#endif /* TDAV_UNDER_WINDOWS */