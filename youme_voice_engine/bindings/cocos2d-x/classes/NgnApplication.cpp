//
//  NgnApplication.cpp
//  cocos2d-x sdk
//
//  Created by wanglei on 15/8/7.
//  Copyright (c) 2015年 youme. All rights reserved.
//
#include "tsk_time.h"
#include "XOsWrapper.h"
#if TARGET_OS_IPHONE
#include "Application.h"
#endif

#if WIN32
#include <win_config.h>
#elif ANDROID
#include "android_config.h"
#else
#include "ios_config.h"
#endif

#include <stdlib.h>
#include "NgnApplication.h"
#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#include <io.h>
#include <direct.h>
#include "YouMeCommon/CrossPlatformDefine/Windows/YouMeSystemInfo.h"

#include "comutil.h"
#include "Wbemcli.h"
#include "Wbemidl.h"
#endif

#include <YouMeCommon/CrossPlatformDefine/IYouMeSystemProvider.h>


#ifdef WIN32
#include <YouMeCommon/CrossPlatformDefine/Windows/YouMeApplication_Win.h>
#elif (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR )
#include <YouMeCommon/CrossPlatformDefine/IOS/YouMeApplication_iOS.h>
#elif TARGET_OS_OSX
#include <YouMeCommon/CrossPlatformDefine/OSX/YouMeApplication_OSX.h>
#elif ANDROID
#include <YouMeApplication_Android.h>
extern YouMeApplication_Android* g_AndroidSystemProvider;
#endif

IYouMeSystemProvider* NgnApplication::s_pSystemProvider = NULL;

bool NgnApplication::s_bYouMeStartup = false;

void NgnApplication::Startup(){
    if (s_bYouMeStartup)
    {
        return;
    }
    
#ifdef WIN32
    s_pSystemProvider = new YouMeApplication_Win;
#elif (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR )
    s_pSystemProvider = new YouMeApplication_iOS;
#elif MAC_OS
    s_pSystemProvider = new  YouMeApplication_OSX;
#elif ANDROID
    s_pSystemProvider = g_AndroidSystemProvider;
#endif
    
    s_bYouMeStartup = true;
    
}


NgnApplication *NgnApplication::sInstance = NULL;
NgnApplication *NgnApplication::getInstance ()
{
    if (NULL == sInstance)
    {
        sInstance = new NgnApplication ();
        Startup();
    }
    return sInstance;
}

void NgnApplication::initPara ()
{
#ifdef __APPLE__
#if TARGET_OS_IPHONE
    this->setPackageName (YouMeApplication::getInstance ()->getPackageName ());

    this->setDocumentPath (YouMeApplication::getInstance ()->getCachePath ());
    this->setUUID (YouMeApplication::getInstance ()->getUUID ());

    this->setSysName (YouMeApplication::getInstance ()->getSysName ());
    this->setScreenOrientation(YouMeApplication::getInstance()->getScreenOrientation());
#else   //mac
    this->setPackageName ( s_pSystemProvider->getPackageName ());

    this->setDocumentPath ( s_pSystemProvider->getCachePath ());
    this->setUUID ( s_pSystemProvider->getUUID ());

    this->setSysName ( s_pSystemProvider->getSystemVersion ());
#endif
#endif
    
#ifdef WIN32
	char szPath[MAX_PATH] = {0};
#if (0) // use tmp as log file dir
    GetTempPathA(MAX_PATH, szPath);
#endif
    
	SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, szPath);
    std::string strPath(szPath);
    this->setPackageName("com.youme.rtc.windows");
	std::string strLogPath = creatLogDir(strPath);
    this->setDocumentPath(strLogPath);
	ULONG ErrorCode;
	std::wstring uuid = GenerateSysUUID(&ErrorCode);
	std::string uuidA = Unicode_to_UTF8(uuid.c_str(), uuid.length());
	this->setUUID(uuidA);
#endif // WIN32

}

#ifdef WIN32
std::string NgnApplication::creatLogDir(std::string dir)
{
	std::string first = "\\rtc"; /* default */
	std::string second = "\\default"; /* default */
	std::string first_dir;
	std::string second_dir;

	first_dir = dir + first;
	if (access(first_dir.c_str(), 6) == -1) {
		mkdir(first_dir.c_str());
	}

	CHAR exeFullPath[MAX_PATH]; // Full path  
	GetModuleFileNameA(NULL, exeFullPath, MAX_PATH);
	std::string strPath(exeFullPath);    // Get full path of the file  
	int start = strPath.find_last_of(L'\\', strPath.length());
	int end = strPath.find_last_of(L'.', strPath.length());

	if (start < end) {
		std::string module_name = strPath.substr(start, (end - start));

		if (module_name.empty() || module_name.length() < 1) {
			second_dir = first_dir + second;
		}
		else {
			second_dir = first_dir + module_name;
		}
	}
	else {
		second_dir = first_dir + second;
	}

	if (access(second_dir.c_str(), 6) == -1) {
		mkdir(second_dir.c_str());
	}

	return second_dir;
}

std::string NgnApplication::getGPUname()
{
    HRESULT hr;
    std::string gpuName;
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    IWbemLocator *pIWbemLocator = NULL;
    hr = CoCreateInstance(__uuidof(WbemLocator), NULL,
        CLSCTX_INPROC_SERVER, __uuidof(IWbemLocator),
        (LPVOID *)&pIWbemLocator);

    BSTR bstrServer = SysAllocString(L"\\\\.\\root\\cimv2");
    IWbemServices *pIWbemServices;
    hr = pIWbemLocator->ConnectServer(bstrServer, NULL, NULL, 0L, 0L, NULL,
        NULL, &pIWbemServices);

    hr = CoSetProxyBlanket(pIWbemServices, RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
        EOAC_DEFAULT);

    BSTR bstrWQL = SysAllocString(L"WQL");
    BSTR bstrPath = SysAllocString(L"select * from Win32_VideoController");
    IEnumWbemClassObject* pEnum;
    hr = pIWbemServices->ExecQuery(bstrWQL, bstrPath, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnum);

    IWbemClassObject* pObj = NULL;
    ULONG uReturned;
    VARIANT var;
    //Windows_GPU_Series gpu = WINDOWS_GPU_NONE;
    do {
        hr = pEnum->Next(WBEM_INFINITE, 1, &pObj, &uReturned);
        if (uReturned)
        {
            hr = pObj->Get(L"ConfigManagerErrorCode", 0, &var, NULL, NULL);
            if (SUCCEEDED(hr))
            {
                if (var.lVal != 0) {
                    //TSK_DEBUG_WARN("WMI VideoController ConfigManagerErrorCode, continue");
                    continue;
                }
            }

            hr = pObj->Get(L"Name", 0, &var, NULL, NULL);
            if (SUCCEEDED(hr))
            {
                char sString_name[256];
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, sString_name, sizeof(sString_name), NULL, NULL);
                gpuName = std::string(sString_name);
                //TSK_DEBUG_INFO("get Windows GPU Name:%s", name.c_str());

				if (gpuName.find("Intel") != -1) {
                    hr = pObj->Get(L"DriverVersion", 0, &var, NULL, NULL);
                    if (SUCCEEDED(hr))
                    {
                        char sString_version[256];
                        WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, sString_version, sizeof(sString_version), NULL, NULL);
                        std::string driverVersion = sString_version;
                        // 25.20.100.6373版本的Intel核显驱动会导致crash
                        if (driverVersion.find("25.20.100.6373") != -1) {
                            //TSK_DEBUG_WARN("Intel Graphics driver version:25.20.100.6373 would crash, continue!!", name.c_str());
                            continue;
                        }
                    }
                    // gpu = WINDOWS_GPU_INTEL;
                    //TSK_DEBUG_INFO("find Intel hardware accelerator");
                    break;
                }
				if (gpuName.find("GeForce") != -1) {
                    // gpu = WINDOWS_GPU_NVDIA;
                    //TSK_DEBUG_INFO("find Nvdia hardware accelerator");
                    break;
                }
				if (gpuName.find("Radeon") != -1) {
                    // todu aeron need support AMD !!!
                    //TSK_DEBUG_WARN("not support Amd hardware accelerator, switch to software!!");
                    // gpu = WINDOWS_GPU_NONE;
                    continue;
                }
            }
        }
        else {
            break;
        }
    } while (uReturned);

    pEnum->Release();
    SysFreeString(bstrPath);
    SysFreeString(bstrWQL);
    pIWbemServices->Release();
    SysFreeString(bstrServer);
    //CoUninitialize();

    return gpuName;    
}
#endif


unsigned int NgnApplication::getLocalBaseTime()
{
    return m_iLocalBaseTime;
}



std::string NgnApplication::CombinePath(const std::string& strDir, const std::string& strFileName)
{
	std::string dir = strDir;
	if (dir.size() == 0)
	{
		return strFileName;
	}
	char last_char = dir[dir.size() - 1];
	if (last_char == '\\' || last_char == '/')
	{
		// 末尾就是 '\\'，处理之前先去掉末尾的 '\\'
		dir.resize(dir.size() - 1);
	}
	XString separator = XPreferredSeparator;
	dir += XStringToLocal(separator);
	dir += strFileName;
	return dir;
}
NgnApplication::NgnApplication ()
{

    sDeviceIMEI = "";
    m_iLocalBaseTime = tsk_gettimeofday_ms()/1000;

#ifndef TARGET_OS_IPHONE
    sGlEsVersion = 0;
#endif
    sCanalID = "";
}

string NgnApplication::getModel ()
{
#ifndef OS_LINUX    
    return XStringToUTF8( s_pSystemProvider->getModel() );
#else
    return string("OS_LINUX");
#endif      
}

string NgnApplication::getCanalID()
{
    return sCanalID;
}

void NgnApplication::setCanalID( string canalID )
{
    sCanalID = canalID;
}


string NgnApplication::getBrand()
{
#ifndef OS_LINUX    
    return XStringToUTF8( s_pSystemProvider->getBrand()  );
#else
    return string("OS_LINUX_unknow");
#endif    
}

string NgnApplication::getCPUChip()
{
#ifdef WIN32
	//maybe crash:lebo
	return string("OS_WIN_unknow"); //getGPUname();
#elif !(defined OS_LINUX)
    return XStringToUTF8(s_pSystemProvider->getCpuChip() );
#else
    return string("OS_LINUX_unknow");
#endif
}

string NgnApplication::getLogPath()
{
	return CombinePath(NgnApplication::getInstance()->getDocumentPath(), "ymrtc_log.txt");
}

string NgnApplication::getBackupLogPath()
{
	return CombinePath(NgnApplication::getInstance()->getDocumentPath() , "ymrtc_log_bak.txt");
}
void NgnApplication::setUserLogPath(const string &userPath)
{
    mUserLogPath = userPath;
}
string NgnApplication::getUserLogPath()
{
    return NgnApplication::mUserLogPath;
}
string NgnApplication::getVideoSnapshotPath()
{
    return CombinePath(NgnApplication::getInstance()->getDocumentPath(), "ymrtc_video_snapshot.yuv");
}
string NgnApplication::getZipLogPath()
{
	return CombinePath(NgnApplication::getInstance()->getDocumentPath() , "ymrtc_log.zip");
}

void NgnApplication::setSysName (const string &sysname)
{
    sSysName = sysname;
}
string NgnApplication::getSysName ()
{
    return sSysName;
}
                         
string NgnApplication::getSysVersion ()
{
#ifndef OS_LINUX
    return XStringToUTF8(s_pSystemProvider->getSystemVersion() );
#else
    return string("OS_LINUX_unknow");
#endif    
}

int NgnApplication::getSDKVersion ()
{
    return sSdkVersion;
}

void NgnApplication::setSDKVersion (int sdkVersion)
{
    sSdkVersion = sdkVersion;
}
#ifndef TARGET_OS_IPHONE
bool NgnApplication::useSetModeToHackSpeaker ()
{
    return (isSamsung () && !isSamsungGalaxyMini () && getSDKVersion () <= 7) ||

           getModel().compare ("blade") == 0 || // ZTE Blade

           getModel().compare ("htc_supersonic") == 0 || // HTC EVO

           getModel().compare ("U8110") == 0 || // Huawei U8110
           getModel().compare ("U8150") == 0    // Huawei U8110
    ;
}


/**
 * Whether the stack is running on a Samsung Galaxy Mini device
 *
 * @return true if the stack is running on a Samsung Galaxy Mini device and
 *         false otherwise
 */
bool NgnApplication::isSamsungGalaxyMini ()
{
    return getModel().compare ("gt-i5800") == 0;
}

/**
 * Whether the stack is running on a Samsung Galaxy Mini device
 *
 * @return true if the stack is running on a Samsung Galaxy Mini device and
 *         false otherwise
 */
bool NgnApplication::isSamsung ()
{
    return getModel().find ("gt-") == 0 || getModel().find ("samsung") != -1 || getModel().find ("sgh-") == 0 || getModel().find ("sph-") == 0 || getModel().find ("sch-") == 0;
}

/**
 * Whether the stack is running on a HTC device
 *
 * @return true if the stack is running on a HTC device and false otherwise
 */
bool NgnApplication::isHTC ()
{
    return getModel().find ("htc") == 0;
}

/**
 * Whether the stack is running on a ZTE device
 *
 * @return true if the stack is running on a ZTE device and false otherwise
 */
bool NgnApplication::isZTE ()
{
    return getModel().find ("zte") == 0;
}

/**
 * Whether the stack is running on a LG device
 *
 * @return true if the stack is running on a LG device and false otherwise
 */
bool NgnApplication::isLG ()
{
    return getModel().find ("lg-") == 0;
}

/**
 * Whether the stack is running on a Toshiba device
 *
 * @return true if the stack is running on a Toshiba device and false
 *         otherwise
 */
bool NgnApplication::isToshiba ()
{
    return getModel().find ("toshiba") == 0;
}

/**
 * Whether the stack is running on a Hovis box
 *
 * @return true if the stack is running on a Hovis box and false otherwise
 */
bool NgnApplication::isHovis ()
{
    return getModel().find ("hovis_box_") == 0;
}


bool NgnApplication::isSetModeAllowed ()
{
    if (isZTE () || isLG ())
    {
        return true;
    }
    bool bFound = false;
    for (int i = 0; i < SLES2FRIEND_BUIDMODELS_NUM; i++)
    {
        if (sSLEs2FriendlyBuildModels[i].compare (getModel()) == 0)
        {
            bFound = true;
            break;
        }
    }
    return bFound;
}

bool NgnApplication::isAGCSupported ()
{
    return isSamsung () || isHTC ();
}
#endif



void NgnApplication::setDeviceIMEI (const string &deviceIMEI)
{
    sDeviceIMEI = deviceIMEI;
}
string NgnApplication::getDeviceIMEI ()
{
    return sDeviceIMEI;
}

string NgnApplication::getCPUArch ()
{
#ifndef OS_LINUX
    return XStringToUTF8(s_pSystemProvider->getCpuArchive() );
#else
    return string("OS_LINUX_unknow");
#endif    
}

#ifndef TARGET_OS_IPHONE
void NgnApplication::setGLEsVersion (int GlEsVersion)
{
    sGlEsVersion = GlEsVersion;
}

int NgnApplication::getGlEsVersion ()
{
    return sGlEsVersion;
}


bool NgnApplication::isGlEs2Supported ()
{
    return getGlEsVersion () >= 0x20000;
}

bool NgnApplication::isSLEs2Supported ()
{
    return (getSDKVersion () >= 9);
}

bool NgnApplication::isSLEs2KnownToWork ()
{
    return true;
}
#endif

void NgnApplication::setPackageName (const string &packageName)
{
    s_pSystemProvider->setPackageName( UTF8TOXString(packageName ) );
}

string NgnApplication::getPackageName ()
{
#ifndef OS_LINUX
    return XStringToUTF8( s_pSystemProvider->getPackageName() );
#else
    return string("OS_LINUX");
#endif    
}

void NgnApplication::setUUID (const string &UUID)
{
    mUUID = UUID;
}

string NgnApplication::getUUID ()
{
    return mUUID;
}

void NgnApplication::setDocumentPath (const string &path)
{
    mDocumentPath = path;
}
string NgnApplication::getDocumentPath ()
{
    return NgnApplication::mDocumentPath;
}
SDKValidate_Platform NgnApplication::getPlatform ()
{
#if TARGET_OS_IPHONE
    return SDKValidate_Platform_IOS;
#elif TARGET_OS_OSX
    return SDKValidate_Platform_OSX;
#elif defined(ANDROID)
    return SDKValidate_Platform_Android;
#elif defined(WIN32)
    return SDKValidate_Platform_Windows;
#elif defined(OS_LINUX)
    return SDKValidate_Platform_Linux;
#else
	return SDKValidate_Platform_Unknow;
#endif
}

string NgnApplication::getPlatformName()
{
    string strName = "";
    SDKValidate_Platform plat = getPlatform();
    switch ( plat ) {
        case SDKValidate_Platform_IOS:
            strName = "IOS";
            break;
        case SDKValidate_Platform_OSX:
            strName = "OSX";
            break;
        case SDKValidate_Platform_Android:
            strName = "Android";
            break;
        case SDKValidate_Platform_Windows:
            strName = "Windows";
            break;
        case SDKValidate_Platform_Linux:
            strName = "Linux";
            break;
        case SDKValidate_Platform_Unknow:
            strName = "Unknow";
            break;
        default:
            break;
    }
    
    return strName;
}

void NgnApplication::setAppKey (string strAppKey)
{
    sAppKey = strAppKey;
}
string NgnApplication::getAppKey ()
{
    return sAppKey;
}
//void NgnApplication::setAppSecret (string strAppSecret)
//{
//    sAppSecret = strAppSecret;
//}
//string NgnApplication::getAppSecret ()
//{
//    return sAppSecret;
//}

void NgnApplication::setScreenOrientation (SCREEN_ORIENTATION_E orentation) {
    screenOrientation = orentation;
}

SCREEN_ORIENTATION_E NgnApplication::getScreenOrientation() {
    return screenOrientation;
}
