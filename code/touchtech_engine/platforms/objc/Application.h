//
//  Application.h
//  YouMeVoiceEngine
//
//  Created by wanglei on 15/9/17.
//  Copyright (c) 2015年 youme. All rights reserved.
//
#include <string>
#include <map>
#include "NgnApplication.h"
class YouMeApplication
{
    public:
    static YouMeApplication *getInstance ();
    static void destroy ();

    public:
    std::string getPackageName ();
    std::string getAppName ();
    std::string getAppVersion ();
    std::string getDocumentPath ();
    std::string getCachePath ();

    std::string getUUID ();

    std::string getModel ();
    std::string getSysName ();
    std::string getSystemVersion ();
    int getSystemVersionMainInt ();
    std::string getCpuArchive();
    SCREEN_ORIENTATION_E getScreenOrientation();

    private:
    std::string getBundleValueByName (const std::string &name);

    private:
    static YouMeApplication *mPInstance;

    private:
    YouMeApplication ();
    ~YouMeApplication ();
    std::map<std::string,std::string> m_deviceNameMap;
};
