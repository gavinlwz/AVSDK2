//
//  AVRuntimeMonitor.cpp
//  YouMeVoiceEngine
//
//  Created by aeron on 17/7/14.
//  Copyright (c) 2015年 youme. All rights reserved.
//

#ifndef __YouMeVoiceEngine__AVRuntimeMonitor__
#define __YouMeVoiceEngine__AVRuntimeMonitor__

#include <atomic>
#include <thread>
#include <mutex>
#include "YouMeCommon/XCondWait.h"

class AVRuntimeMonitor
{
public:

    static AVRuntimeMonitor *getInstance ();
    static void destroy();
    
    void Init();
    void UnInit();
    
    void ConsumerCountAdd();
    void ProducerCountAdd();
    
    void StartConsumerMoniterThread();
    void StartProducerMoniterThread();
    
    void StopConsumerMoniterThread();
    void StopProducerMoniterThread();
    
    void ConsumerMonitering();
    void ProducerMonitering();

private:

    AVRuntimeMonitor();
    ~AVRuntimeMonitor();
    
    void ResetMembers();

    static AVRuntimeMonitor *sInstance;
    std::mutex m_mutex;
#if defined(__mips__)
    uint32_t m_iRetryCount;
    uint32_t m_iConsumerPeriod;
    uint32_t m_iProducerPeriod;
    
    uint64_t m_iConsumerCurrentCount;
    uint64_t m_iConsumerHistoryCount;
    uint64_t m_iProducerCurrentCount;
    uint64_t m_iProducerHistoryCount;
    
    bool m_bConsumerAbort;
    bool m_bProducerAbort;
#else
    std::atomic<uint32_t> m_iRetryCount;
    std::atomic<uint32_t> m_iConsumerPeriod;
    std::atomic<uint32_t> m_iProducerPeriod;
    
    std::atomic<uint64_t> m_iConsumerCurrentCount;
    std::atomic<uint64_t> m_iConsumerHistoryCount;
    std::atomic<uint64_t> m_iProducerCurrentCount;
    std::atomic<uint64_t> m_iProducerHistoryCount;
    
    std::atomic<bool> m_bConsumerAbort;
    std::atomic<bool> m_bProducerAbort;
#endif

    std::thread m_consumerMoniterThread;
    std::thread m_producerMoniterThread;

    youmecommon::CXCondWait m_consumerCond;
    youmecommon::CXCondWait m_producerCond;
};

#endif /* defined(__YouMeVoiceEngine__AVRuntimeMonitor__) */
