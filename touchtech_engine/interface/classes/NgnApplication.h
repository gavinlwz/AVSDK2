//
//  NgnApplication.h
//  cocos2d-x sdk
//
//  Created by wanglei on 15/8/7.
//  Copyright (c) 2015å¹´ youme. All rights reserved.
//

#ifndef __cocos2d_x_sdk__NgnApplication__
#define __cocos2d_x_sdk__NgnApplication__
#include <string>
#include "version.h"
using namespace std;
#include "SDKValidateTalk.h"
#include <YouMeCommon/CrossPlatformDefine/IYouMeSystemProvider.h>

const string NGNAPPLICATION_TAG = "NgnApplication";

const int SLES2FRIEND_BUIDMODELS_NUM = 5;
const string sSLEs2FriendlyBuildModels[SLES2FRIEND_BUIDMODELS_NUM] = {
    "galaxy nexus", /* 4.1 */
    "gt-i9100",     /* Galaxy SII, 4.0.4 */
    "gt-s5570i",    /* 2.3.0 */
    "xt890",        /* Motorola Razer i 4.0.4 */
    "lg-p970"       /* 2.3.4 */
};

const string sSLEs2UnFriendlyBuildModels[] = {
    "gt-s5360", /* 2.3.6 :robotic */
};

typedef enum{
    portrait = 0,
    landscape_right,
    upside_down,
    landscape_left
} SCREEN_ORIENTATION_E;

class NgnApplication
{

    private:
    static NgnApplication *sInstance;
    static IYouMeSystemProvider* s_pSystemProvider;
    static bool s_bYouMeStartup;
    static void Startup();


    private:
    string sDeviceIMEI;

    int sSdkVersion=SDK_NUMBER;
    
    string sCanalID;


    string sAppKey;
//    string sAppSecret;
    uint64_t uBusinessID;

#ifndef TARGET_OS_IPHONE
    int sGlEsVersion=0;
#endif

    string sSysName;

    string mUUID;

    string mDocumentPath;
    
    string mUserLogPath;

    
    SCREEN_ORIENTATION_E screenOrientation;
    private:
    NgnApplication ();

    public:
    static NgnApplication *getInstance ();

    public:
    void initPara ();

    public:
#ifndef TARGET_OS_IPHONE
    bool useSetModeToHackSpeaker ();

    /**
     * Whether the stack is running on a Samsung Galaxy Mini device
     *
     * @return true if the stack is running on a Samsung Galaxy Mini device and
     *         false otherwise
     */
    bool isSamsungGalaxyMini ();

    /**
     * Whether the stack is running on a Samsung Galaxy Mini device
     *
     * @return true if the stack is running on a Samsung Galaxy Mini device and
     *         false otherwise
     */
    bool isSamsung ();

    /**
     * Whether the stack is running on a HTC device
     *
     * @return true if the stack is running on a HTC device and false otherwise
     */
    bool isHTC ();

    /**
     * Whether the stack is running on a ZTE device
     *
     * @return true if the stack is running on a ZTE device and false otherwise
     */
    bool isZTE ();

    /**
     * Whether the stack is running on a LG device
     *
     * @return true if the stack is running on a LG device and false otherwise
     */
    bool isLG ();

    /**
     * Whether the stack is running on a Toshiba device
     *
     * @return true if the stack is running on a Toshiba device and false
     *         otherwise
     */
    bool isToshiba ();

    /**
     * Whether the stack is running on a Hovis box
     *
     * @return true if the stack is running on a Hovis box and false otherwise
     */
    bool isHovis ();

    bool isSetModeAllowed ();

    bool isAGCSupported ();

    void setGLEsVersion (int GlEsVersion);

    int getGlEsVersion ();

    bool isGlEs2Supported ();

    bool isSLEs2Supported ();

    bool isSLEs2KnownToWork ();

#endif
    unsigned int getLocalBaseTime();
	static std::string CombinePath(const std::string& strDir, const std::string& strFileName);

    string getModel ();

    void setSysName (const string &sysname);
    string getSysName ();

    void setSysVersion (const string &sysversion);
    string getSysVersion ();

    int getSDKVersion ();
    void setSDKVersion (int sdkVersion);

    string getCanalID();
    void setCanalID( string canalID );

    void setDeviceIMEI (const string &deviceIMEI);

    string getDeviceIMEI ();

    string getCPUArch ();

    string getBrand();

    void setPackageName (const string &packageName);

    string getPackageName ();

    void setUUID (const string &UUID);

    string getUUID ();

    void setDocumentPath (const string &path);
    string getLogPath();
    string getBackupLogPath();
    void setUserLogPath(const string &userPath);
    string getUserLogPath();
    string getVideoSnapshotPath();
    string getZipLogPath();
    string getDocumentPath ();
    
    string getCPUChip();
    
    SDKValidate_Platform getPlatform ();
    string getPlatformName();
    
    void setAppKey (string strAppKey);
    string getAppKey ();
//    void setAppSecret (string strAppSecret);
//    string getAppSecret ();
    unsigned int m_iLocalBaseTime;

    void setScreenOrientation (SCREEN_ORIENTATION_E orentation);
    SCREEN_ORIENTATION_E getScreenOrientation();
    
#ifdef WIN32
	std::string creatLogDir(std::string dir);
    std::string getGPUname();
#endif

};

#endif /* defined(__cocos2d_x_sdk__NgnApplication__) */
