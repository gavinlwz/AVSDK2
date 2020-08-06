//
//  NgnMemoryConfiguration.cpp
//  youme_voice_engine
//
//  Created by joexie on 16/1/22.
//  Copyright © 2016年 youme. All rights reserved.
//

#include "NgnMemoryConfiguration.hpp"


bool CNgnMemoryConfiguration::SetConfiguration(const std::string&strKey,const youmecommon::CXAny& value)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_configureMap[strKey] = value;
    return  true;
}

CNgnMemoryConfiguration* CNgnMemoryConfiguration::getInstance()
{
    static CNgnMemoryConfiguration*s_instance = NULL;
    if (NULL == s_instance) {
        s_instance = new CNgnMemoryConfiguration;
    }
    return  s_instance;
}