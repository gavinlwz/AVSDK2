//
//  main.m
//  perf_test
//
//  Created by wanglei on 15/12/31.
//  Copyright © 2015年 tencent. All rights reserved.
//
#include "IYouMeVoiceEngine.h"
#include "VoiceTest.hpp"
#include "stdio.h"
#include "IMyYouMeInterface.h"
int main (int argc, const char *argv[])
{
    string strServerIP;
    //获取参数
    if (argc >= 2)
    {
        strServerIP = string(argv[1]);
    }
    else
    {
        printf("例子：./perf_test 112.74.86.10(MCU IP) 50(在线人数) 0(是否加入同一个房间)\n");
        _exit(1);
    }
    int onlineNum = 50;
    if (argc >= 3)
    {
        onlineNum = stoi(argv[2]);
    }
    
    int sameRoom = 0;
    if(argc == 4)
    {
        sameRoom = stoi(argv[3]);
    }
    for (int i = 0; i < onlineNum; i++)
    {
        IYouMeVoiceEngine* pEngine = new IYouMeVoiceEngine();
        SetFixedIP(strServerIP);
        char szRoomID[3] = {0};
        snprintf(szRoomID, 3, "%d",i);
        if (sameRoom)
        {
            snprintf(szRoomID, 3, "001");
        }
        CVoiceTest *pCallback = new CVoiceTest (pEngine,szRoomID);
        pEngine->init(pCallback,pCallback,"","");
        sleep(1);
    }
    
    printf ("I am test!\n");
    sleep(1000000000);
    return 0;
}
