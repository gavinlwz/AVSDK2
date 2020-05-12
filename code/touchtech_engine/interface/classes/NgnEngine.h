//
//  NgnEngine.h
//  cocos2d-x sdk
//
//  Created by wanglei on 15/8/7.
//  Copyright (c) 2015å¹´ youme. All rights reserved.
//

#ifndef __cocos2d_x_sdk__NgnEngine__
#define __cocos2d_x_sdk__NgnEngine__

#include <string>
using namespace std;
#include "INgnNetworkService.h"
#include "NgnApplication.h"
#include "NgnMemoryConfiguration.hpp"
const string NGNENGINE_TAG = "NgnEngine";


class NgnEngine
{
    protected:
    NgnEngine ();
    ~NgnEngine ();

    protected:
    static NgnEngine *sInstance;
    bool mStarted = false;
    INgnNetworkService *mPNetworkService = NULL;
    private:
    static bool sInitialized;

    public:
    /**
     * Gets an instance of the NGN engine. You can call this function as many as
     * you need and it will always return th same instance.
     *
     * @return An instance of the NGN engine.
     */
    static NgnEngine *getInstance ();

    static void destroy ();


    void initialize ();
    /**
     * Starts the engine. This function will start all underlying services (SIP,
     * XCAP, MSRP, History, ...). You must call this function before trying to
     * use any of the underlying services.
     *
     * @return true if all services have been successfully started and false
     *         otherwise
     */
    bool start ();

    /**
     * Stops the engine. This function will stop all underlying services (SIP,
     * XCAP, MSRP, History, ...).
     *
     * @return true if all services have been successfully stopped and false
     *         otherwise
     */
    bool stop ();

    /**
     * Checks whether the engine is started.
     *
     * @return true is the engine is running and false otherwise.
     * @sa @ref start() @ref stop()
     */
    bool isStarted ();


    /**
     * Gets the network service
     *
     * @return the network service
     */
    INgnNetworkService *getNetworkService ();
};

#endif /* defined(__cocos2d_x_sdk__NgnEngine__) */
