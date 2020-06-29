//
//  NgnMemoryConfiguration.hpp
//  youme_voice_engine
//
//  Created by joexie on 16/1/22.
//  Copyright © 2016年 youme. All rights reserved.
//

#ifndef NgnMemoryConfiguration_hpp
#define NgnMemoryConfiguration_hpp

#include <stdio.h>
#include <map>
#include <mutex>
#include "YouMeCommon/XAny.h"

//内存配置文件

class CNgnMemoryConfiguration
{
public:
    static CNgnMemoryConfiguration* getInstance();
    bool SetConfiguration(const std::string&strKey,const youmecommon::CXAny& value);
    
    template <class T>
     T GetConfiguration(const std::string& strKey,const T& defaultValue)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::map<std::string, youmecommon::CXAny>::iterator it =  m_configureMap.find(strKey);
        if (it == m_configureMap.end()) {
            return defaultValue;
        }
        return youmecommon::CXAny::XAny_Cast<T>(it->second);
    }
    
    void Clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_configureMap.clear();
    }
private:
    CNgnMemoryConfiguration()
    {
        
    }
    std::map<std::string, youmecommon::CXAny> m_configureMap;
    std::mutex m_mutex;
};


#endif /* NgnMemoryConfiguration_hpp */
