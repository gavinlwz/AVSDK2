//
//  AVRuntimeMonitor.cpp
//  YouMeVoiceEngine
//
//  Created by aeron on 17/7/14.
//  Copyright (c) 2015年 youme. All rights reserved.
//

#include "AVRuntimeMonitor.h"
#include "YouMeVoiceEngine.h"
#include "NgnMemoryConfiguration.hpp"
#include "NgnConfigurationEntry.h"

/*
 ********************AVRuntimeMonitor************************
 */

AVRuntimeMonitor *AVRuntimeMonitor::sInstance = NULL;

AVRuntimeMonitor *AVRuntimeMonitor::getInstance ()
{
    if (NULL == sInstance)
    {
        sInstance = new AVRuntimeMonitor ();
    }
    return sInstance;
}

void AVRuntimeMonitor::destroy ()
{
    delete sInstance;
    sInstance = NULL;
}

#define AVMONITER_RETRY_COUNT 3

AVRuntimeMonitor::AVRuntimeMonitor() :
    m_iRetryCount(AVMONITER_RETRY_COUNT),
    m_iConsumerPeriod(0),
    m_iProducerPeriod(0),
    m_iConsumerCurrentCount(0ll),
    m_iConsumerHistoryCount(0ll),
    m_iProducerCurrentCount(0ll),
    m_iProducerHistoryCount(0ll) {
    
}

AVRuntimeMonitor::~AVRuntimeMonitor()
{
    UnInit();
}

void AVRuntimeMonitor::ConsumerCountAdd()
{
    if (m_iConsumerPeriod > 0) {
        m_iConsumerCurrentCount++;
    }
}

void AVRuntimeMonitor::ProducerCountAdd()
{
    if (m_iProducerPeriod > 0) {
        m_iProducerCurrentCount++;
    }
}

void AVRuntimeMonitor::ConsumerMonitering()
{
    int i = 0;
    while(1) {
        m_consumerCond.WaitTime(m_iConsumerPeriod);
        uint64_t current = m_iConsumerCurrentCount;
        uint64_t history = m_iConsumerHistoryCount;
        if (m_bConsumerAbort){
            return;
        }
        if (current > 1) {
            if (current > history) {
                uint64_t temp = current;
                m_iConsumerHistoryCount = temp;
                i = 0;
                continue;
            } else {
                TSK_DEBUG_INFO("Consumer Monitor retry:%d", i);
                if (i++ < m_iRetryCount) {
                    continue;
                }else {
                    break;
                }
            }
        }
    }
    CYouMeVoiceEngine::getInstance()->pauseChannel();
    CYouMeVoiceEngine::getInstance()->resumeChannel();
}

void AVRuntimeMonitor::ProducerMonitering()
{
    int i = 0;
    while(1) {
        m_producerCond.WaitTime(m_iProducerPeriod);
        uint64_t current = m_iProducerCurrentCount;
        uint64_t history = m_iProducerHistoryCount;
        if (m_bProducerAbort){
            return;
        }
        if (current > 1) {
            if (current > history) {
                uint64_t temp = current;
                m_iProducerHistoryCount = temp;
                i = 0;
                continue;
            } else {
                TSK_DEBUG_INFO("Producer Monitor retry:%d", i);
                if (i++ < m_iRetryCount) {
                    continue;
                }else {
                    break;
                }
            }
        }
    }
    CYouMeVoiceEngine::getInstance()->pauseChannel();
    CYouMeVoiceEngine::getInstance()->resumeChannel();
}

void AVRuntimeMonitor::StartConsumerMoniterThread()
{
    StopConsumerMoniterThread();
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_iConsumerPeriod > 0) {
        TSK_DEBUG_INFO("StartConsumerMoniterThread OK");
        m_consumerMoniterThread = std::thread(&AVRuntimeMonitor::ConsumerMonitering, this);
    }
}

void AVRuntimeMonitor::StartProducerMoniterThread()
{
    StopProducerMoniterThread();
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_iProducerPeriod > 0) {
        TSK_DEBUG_INFO("StartProducerMoniterThread OK");
        m_producerMoniterThread = std::thread(&AVRuntimeMonitor::ProducerMonitering, this);
    }
}

void AVRuntimeMonitor::StopConsumerMoniterThread()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bConsumerAbort = true;

    if (m_consumerMoniterThread.joinable()) {
        m_consumerCond.SetSignal();
        if (m_consumerMoniterThread.get_id() == std::this_thread::get_id()) {
            m_consumerMoniterThread.detach();
        }
        else {
            m_consumerMoniterThread.join();
        }
        TSK_DEBUG_INFO("StopConsumerMoniterThread OK");
    }
    m_bConsumerAbort = false;
    m_iConsumerCurrentCount = 0ull;
    m_iConsumerHistoryCount = 0ull;
    m_consumerCond.Reset();
}

void AVRuntimeMonitor::StopProducerMoniterThread()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bProducerAbort = true;
    
    if (m_producerMoniterThread.joinable()) {
        m_producerCond.SetSignal();
        if (m_producerMoniterThread.get_id() == std::this_thread::get_id()) {
            m_producerMoniterThread.detach();
        }
        else {
            m_producerMoniterThread.join();
        }
        TSK_DEBUG_INFO("StopProducerMoniterThread OK");
    }
    m_bProducerAbort = false;
    m_iProducerCurrentCount = 0ull;
    m_iProducerHistoryCount = 0ull;
    m_producerCond.Reset();
}

void AVRuntimeMonitor::ResetMembers()
{
    m_consumerCond.Reset();
    m_producerCond.Reset();
    
    m_iConsumerPeriod = 0ull;
    m_iProducerPeriod = 0ull;
    
    m_iConsumerCurrentCount = 0ull;
    m_iConsumerHistoryCount = 0ull;
    m_iProducerCurrentCount = 0ull;
    m_iProducerHistoryCount = 0ull;
    
    m_bConsumerAbort = false;
    m_bProducerAbort = false;
}

void AVRuntimeMonitor::Init()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    ResetMembers();
    
    uint32_t consumerPeriod = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::AVMONITOR_CONSUMER_PEROID_MS, NgnConfigurationEntry::DEF_AVMONITOR_CONSUMER_PEROID_MS);
    if (consumerPeriod > 0) {
        m_iConsumerPeriod = consumerPeriod;
    }
    
    uint32_t producerPeriod = CNgnMemoryConfiguration::getInstance()->GetConfiguration(NgnConfigurationEntry::AVMONITOR_PRODUCER_PEROID_MS, NgnConfigurationEntry::DEF_AVMONITOR_PRODUCER_PEROID_MS);
    if (producerPeriod > 0) {
        m_iProducerPeriod = producerPeriod;
    }
}

void AVRuntimeMonitor::UnInit()
{
    StopConsumerMoniterThread();
    StopProducerMoniterThread();
    ResetMembers();
}
