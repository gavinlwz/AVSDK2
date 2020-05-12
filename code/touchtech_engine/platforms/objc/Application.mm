//
//  Application.mm
//  YouMeVoiceEngine
//
//  Created by wanglei on 15/9/17.
//  Copyright (c) 2015年 youme. All rights reserved.
//
#ifndef MAC_OS
#import <UIKit/UIDevice.h>
#import <UIKit/UIApplication.h>
#endif

#include <mach/machine.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#import "HeadsetMonitor.h"
#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import "Application.h"

#include <string>



YouMeApplication *YouMeApplication::mPInstance = NULL;

YouMeApplication *YouMeApplication::getInstance ()
{
    if (mPInstance == nil)
    {
        mPInstance = new YouMeApplication ();
    }
    return mPInstance;
}
void YouMeApplication::destroy ()
{
    delete mPInstance;
    mPInstance = nil;
}


YouMeApplication::YouMeApplication ()
{
    m_deviceNameMap[("iPhone3,1")] = ("iPhone 4");
    m_deviceNameMap[("iPhone3,2")] = ("iPhone 4");
    m_deviceNameMap[("iPhone3,3")] = ("iPhone 4");
    m_deviceNameMap[("iPhone4,1")] = ("iPhone 4S");
    m_deviceNameMap[("iPhone5,1")] = ("iPhone 5");
    m_deviceNameMap[("iPhone5,2")] = ("iPhone 5");
    //以上iphone 可以是ios6系统
    m_deviceNameMap[("iPhone5,3")] = ("iPhone 5C");
    m_deviceNameMap[("iPhone5,4")] = ("iPhone 5C");
    m_deviceNameMap[("iPhone6,1")] = ("iPhone 5S");
    m_deviceNameMap[("iPhone6,2")] = ("iPhone 5S");
    m_deviceNameMap[("iPhone7,2")] = ("iPhone 6");
    m_deviceNameMap[("iPhone7,1")] = ("iPhone 6 Plus");
    m_deviceNameMap[("iPhone8,1")] = ("iPhone 6s");
    m_deviceNameMap[("iPhone8,2")] = ("iPhone 6s Plus");
    
    m_deviceNameMap[("iPad2,1")] = ("iPad 2 WiFi");
    m_deviceNameMap[("iPad2,2")] = ("iPad 2 GSM");
    m_deviceNameMap[("iPad2,3")] = ("iPad 2 CDMA");
    m_deviceNameMap[("iPad2,4")] = ("iPad 2 WiFi 2");
    m_deviceNameMap[("iPad2,5")] = ("iPad mini WiFi");
    m_deviceNameMap[("iPad2,6")] = ("iPad mini WiFi + Cellular");
    m_deviceNameMap[("iPad2,7")] = ("iPad mini WiFi + Cellular");
    
    m_deviceNameMap[("iPad3,1")] = ("iPad 3 WiFi");
    m_deviceNameMap[("iPad3,2")] = ("iPad 3 WiFi + Cellular");
    m_deviceNameMap[("iPad3,3")] = ("iPad 3 WiFi + Cellular");
    m_deviceNameMap[("iPad3,4")] = ("iPad 4 WiFi");
    m_deviceNameMap[("iPad3,5")] = ("iPad 4 WiFi + Cellular");
    m_deviceNameMap[("iPad3,6")] = ("iPad 4 WiFi + Cellular");
    
    //以上iPAD 可以是ios6 系统
    m_deviceNameMap[("iPad4,1")] = ("iPad Air WiFi");
    m_deviceNameMap[("iPad4,2")] = ("iPad Air WiFi + Cellular");
    m_deviceNameMap[("iPad4,3")] = ("iPad Air WiFi + Cellular");
    m_deviceNameMap[("iPad4,4")] = ("iPad mini2 WiFi");
    m_deviceNameMap[("iPad4,5")] = ("iPad mini2 WiFi + Cellular");
    m_deviceNameMap[("iPad4,7")] = ("iPad mini3 WiFi");
    m_deviceNameMap[("iPad4,8")] = ("iPad mini3 CDMA");
    m_deviceNameMap[("iPad4,9")] = ("iPad mini3 TD-LTE");
    m_deviceNameMap[("iPad5,1")] = ("iPad mini4 WiFi");
    m_deviceNameMap[("iPad5,2")] = ("iPad mini4 TD-LTE");
    m_deviceNameMap[("iPad5,3")] = ("iPad Air2 WiFi");
    m_deviceNameMap[("iPad5,4")] = ("iPad Air2 TD-LTE");
    m_deviceNameMap[("iPad6,7")] = ("iPad Pro");
    m_deviceNameMap[("iPad6,8")] = ("iPad Pro LTE");
    
    
    m_deviceNameMap[("iPod4,1")] = ("iPod 4");
    m_deviceNameMap[("iPod5,1")] = ("iPod 5");
    m_deviceNameMap[("iPod7,1")] = ("iPod 6");

    [[HeadsetMonitor getInstance] start];
}
YouMeApplication::~YouMeApplication ()
{
}

std::string YouMeApplication::getDocumentPath ()
{
    //获得应用程序沙盒的Documents目录，官方推荐数据文件保存在此
    NSArray *path = NSSearchPathForDirectoriesInDomains (NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *doc_path = [path objectAtIndex:0];
    const char *path_cstr = [[NSFileManager defaultManager] fileSystemRepresentationWithPath:doc_path];
    std::string documentDirectory = path_cstr;
    return documentDirectory;
}

std::string YouMeApplication::getCachePath()
{
    //获得应用程序沙盒的Documents目录，官方推荐数据文件保存在此
    NSArray *path = NSSearchPathForDirectoriesInDomains (NSCachesDirectory, NSUserDomainMask, YES);
    NSString *doc_path = [path objectAtIndex:0];
    const char *path_cstr = [[NSFileManager defaultManager] fileSystemRepresentationWithPath:doc_path];
    std::string documentDirectory = path_cstr;
    return documentDirectory;
}

std::string YouMeApplication::getPackageName ()
{
    return getBundleValueByName ("CFBundleIdentifier");
}

std::string YouMeApplication::getAppName ()
{
    return getBundleValueByName ("CFBundleName");
}

std::string YouMeApplication::getAppVersion ()
{
    return getBundleValueByName ("CFBundleShortVersionString");
}

std::string YouMeApplication::getBundleValueByName (const std::string &name)
{
    NSDictionary *infoDictionary = [[NSBundle mainBundle] infoDictionary];
    if (infoDictionary == nil)
    {
        return "";
    }
    NSString *value = [infoDictionary objectForKey:[NSString stringWithUTF8String:name.c_str ()]];
    infoDictionary = nil;
    if (value == nil)
    {
        return "";
    }

    NSLog (@"name:%s,value:%s", name.c_str (), [value UTF8String]);

    return [value UTF8String];
}

std::string YouMeApplication::getUUID ()
{

    NSString *UUID = nil;
#ifndef MAC_OS
    UUID = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
#endif
    if (UUID == nil) {
        return "";
    }
    return [UUID UTF8String];
}

std::string YouMeApplication::getModel ()
{
    std::string strResult;
#ifndef MAC_OS
    size_t size;
    sysctlbyname("hw.machine", NULL, &size, NULL, 0);
    if (0 == size) {
        return "";
    }
    char *pszBuffer = (char*)malloc(size);
    sysctlbyname("hw.machine", pszBuffer, &size, NULL, 0);
    std::map<std::string, std::string>::iterator it = m_deviceNameMap.find(pszBuffer);
    if (it != m_deviceNameMap.end()) {
        strResult = it->second;
    }
    else
    {
        strResult = pszBuffer;
    }
    free(pszBuffer);
#endif
    return strResult;
}

std::string YouMeApplication::getSysName ()
{
    NSString *systemName = nil;
#ifndef MAC_OS
    systemName = [[UIDevice currentDevice] systemName];
#endif
    if (systemName == nil) {
        return "";
    }
    return [systemName UTF8String];
}

std::string YouMeApplication::getSystemVersion ()
{
    NSString *systemVersion = nil;
#ifndef MAC_OS
    systemVersion = [[UIDevice currentDevice] systemVersion];
#endif
    if (systemVersion == nil) {
        return "";
    }
    return [systemVersion UTF8String];
}

int YouMeApplication::getSystemVersionMainInt () {
    NSString *versionString = nil;
#ifdef MAC_OS
    NSDictionary * sv = [NSDictionary dictionaryWithContentsOfFile:@"/System/Library/CoreServices/SystemVersion.plist"];
    versionString = [sv objectForKey:@"ProductVersion"];
#else
    versionString = [[UIDevice currentDevice] systemVersion];
#endif
    int index = [versionString rangeOfString:@"."].location;
    return [[versionString substringToIndex:index] intValue];
}

std::string YouMeApplication::getCpuArchive ()
{
    NSString *cpuArchive = nil;
#ifndef MAC_OS
    size_t size;
    cpu_type_t type;
    cpu_subtype_t subtype;
    size = sizeof(type);
    sysctlbyname("hw.cputype", &type, &size, NULL, 0);
    
    size = sizeof(subtype);
    sysctlbyname("hw.cpusubtype", &subtype, &size, NULL, 0);
    
    // values for cputype and cpusubtype defined in mach/machine.h
    if (type == CPU_TYPE_X86)
    {
        return "x86";
    }
    else if(type == CPU_TYPE_I386)
    {
        return "i386";
    }
    else if(type == CPU_TYPE_X86_64)
    {
        return "x86_64";
    }
    else if (type == CPU_TYPE_ARM)
    {
        char szArchive[20] = {0};
        sprintf(szArchive, "ARM_%d",subtype);
        return szArchive;
    }
    else if(type == CPU_TYPE_ARM64)
    {
        char szArchive[20] = {0};
        sprintf(szArchive, "ARM64_%d",subtype);
        return szArchive;
    }
#endif
    if (cpuArchive ==nil) {
        return "";
    }
    return [cpuArchive UTF8String];
}

SCREEN_ORIENTATION_E YouMeApplication::getScreenOrientation() {
    SCREEN_ORIENTATION_E orientation = portrait;
#ifndef MAC_OS
    switch([[UIApplication sharedApplication] statusBarOrientation]) {
        case UIInterfaceOrientationPortrait:
            orientation = portrait;
            break;
        case UIInterfaceOrientationPortraitUpsideDown:
            orientation = upside_down;
            break;
        case UIInterfaceOrientationLandscapeLeft:
            orientation = landscape_left;
            break;
        case UIInterfaceOrientationLandscapeRight:
            orientation = landscape_right;
            break;
        default:
            break;
    }
#endif
    return orientation;
}

